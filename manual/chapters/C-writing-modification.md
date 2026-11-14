# Appendix C: writing a new `:Modification` or `:Change` {#app-C}

This appendix covers the two objects of notification and of edit that
the author of a new `:Block` may need to define: a `:Modification`
(always, if the `:Block` supports any change at all) and, optionally, a
`:Change` (for serialisable, undoable or transmissible edits). The
running example continues with `BinPackingBlock` ([Appendix A](A-writing-block.md#app-A)).

## C.1 Why a `:Block` needs its own `:Modification`s {#sec-C-1}

A `:Block` that exposes any `chg_*()` mutator must, when the change
happens and a `:Solver` is listening, tell that `:Solver` *what*
changed ([Chapter 8](08-modification-janus.md#ch-8)). The base
`Modification` hierarchy already covers the generic cases
(`NBModification` for a wholesale `load()`,
[§4.6](04-block.md#sec-4-6), and the abstract `C05FunctionMod`,
`VariableMod` and `RowConstraintMod` for the changes made through the
abstract face), but a *physical* change to a `:Block`'s own data
that a specialised `:Solver` should react to needs a
`:Block`-specific `:Modification` carrying the description of that
change.

## C.2 Naming and structure conventions {#sec-C-2}

SMS++ follows consistent naming for `:Block`-specific
`Modification`s ([Chapter 8](08-modification-janus.md#ch-8), [§8.2](08-modification-janus.md#sec-8-2)):

- `<Block>Mod`, the base class of the physical `Modification`s of
  that `:Block` (e.g., `MCFBlockMod`, `BinaryKnapsackBlockMod`);
- `<Block>RngdMod`, a change of a contiguous interval
  $[\mathit{start}, \mathit{stop})$ of the data;
- `<Block>SbstMod`, a change of an arbitrary set of indices;
- `<Block>ModAdd` and `<Block>ModRmv`, additions and removals, where
  the `:Block` supports dynamic objects.

A physical `:Modification` derives from `Modification` (not from
`AModification`); its `concerns_Block()` is `false`, because the
change in the `:Block` has already happened by the time the object
is constructed ([§8.3](08-modification-janus.md#sec-8-3)). It carries
just enough to let a `:Solver` identify what changed, i.e., for
`BinPackingBlock`, the index (or the range, or the subset) of the items
whose size was modified:

```cpp
// BinPackingBlock.h (alongside the Block)
namespace SMSpp_di_unipi_it {

class BinPackingBlockMod : public Modification
{
 public:
  // what a size change does: it is physical, it changes coefficients of
  // the (capacity) Constraints, and the region may shrink or grow
  static constexpr ModConcern size_changes =
   eModPhys | eModCnsCoef | eRegnShrink | eRegnGrow;

  BinPackingBlockMod( BinPackingBlock * blk , Block::Index item )
   : Modification( size_changes ) , f_block( blk ) , f_item( item ) {}

  Block * get_Block( void ) const override { return( f_block ); }
  Block::Index item( void ) const { return( f_item ); }

 protected:
  void print( std::ostream & output ) const override {
   output << "BinPackingBlockMod[" << this << "]: size of item " << f_item;
  }

 private:
  BinPackingBlock * f_block;
  Block::Index      f_item;     // which item's size changed
};

}  // namespace
```

Besides `get_Block()`, the one other method a `:Modification` must
implement is the protected `print()`, which `operator<<` dispatches
to. The value passed to the constructor of `Modification` is the
`ModConcern` returned by `changes()`: a bit mask saying what *kind*
of thing the `Modification` changes (`eModPhys` for the physical
representation, and `eModVarData`, `eModVarType`, `eModVarSet`,
`eModCnsSide`, `eModCnsCoef`, `eModCnsSet`, `eModObj` for the parts
of the model) and what *effect* it may have on the problem
(`eRegnShrink` / `eRegnGrow` for the feasible region, `eObjUp` /
`eObjDown` for the objective), an effect bit not set being a
guarantee. The default, `eModPhys | eEffAny` ("physical, anything may
happen"), is always correct for a physical `:Modification`; saying
more is what lets a `:Solver` that only reads some kinds be spared
the others (`Solver::concerned_by()`, [§B.4](B-writing-solver.md#sec-B-4)), lets the `:Block`
avoid issuing the `Modification` when nobody listening reads its
kind (the two-argument `issue_pmod( issueMod , kind )` used in
[§A.5](A-writing-block.md#sec-A-5)), and lets a reader keep what is still valid, e.g., a
solution when the region cannot shrink. The masks `ePhysObj`,
`ePhysSides`, `ePhysCns` and `ePhysVars` cover the common physical
cases, and the bits are to be read through the static methods of
`Modification` (`is_physical()`, `changes_only_sides()`,
`may_shrink_region()`, ...) rather than tested one by one. A
physical `:Modification` reaches only those who read `eModPhys`,
whatever other kinds it says it changes (only one that says it
changes everything, `eModAll`, as the `NBModification`, reaches
anyone): these tell what the change
is in terms of the model, but those who only read the abstract
representation see the change in the abstract `Modification`s that
come with it.

A `RngdMod` / `SbstMod` would carry a `Range` / `Subset` instead of
a single index, mirroring the `Rngd` / `Sbst` shapes of the methods
factory ([§15.3](15-methods-factory.md#sec-15-3)). This is the object `BinPackingBlock::chg_size`
issued in [§A.5](A-writing-block.md#sec-A-5).

## C.3 Registering with the factory {#sec-C-3}

A plain `:Modification` does not generally need to be in a factory,
since it is constructed by the `:Block`, dispatched, consumed, and
deallocated when the last holder releases its `shared_ptr`
([§8.1](08-modification-janus.md#sec-8-1)).
It is `:Change` and the serialisable objects that need the
class-name factory. A `:Modification` that *does* need to be
reconstructed from netCDF would use the same
`SMSpp_insert_in_factory_*` macros as everything else ([Chapter 18](18-factories-netcdf.md#ch-18)).

## C.4 Defining a `:Change` (optional) {#sec-C-4}

> *Status: beta.* The `Change` mechanism ([Chapter 16](16-change.md#ch-16)) is in
> `develop` and its interface may change. Add a `:Change` family
> only when the use case calls for serialisable, transmissible, or
> undoable edits (a distributed solver, a move-based search).

A `:Change` for `BinPackingBlock` represents a *prospective* change of
sizes, carrying the new sizes so that it can be `apply()`-ed to any
`BinPackingBlock` with enough items, serialised to netCDF, and produce
its UndoChange:

```cpp
class BinPackingBlockRngdChange : public Change
{
 public:
  BinPackingBlockRngdChange( Block::Range rng = { 0 , 0 } ,
                             std::vector< double > && sizes = {} )
   : f_rng( rng ) , v_sizes( std::move( sizes ) ) {}

  Change * apply( Block * block , bool doUndo = false ,
                  ModParam issueMod = eNoBlck ,
                  ModParam issueAMod = eNoBlck ) override
  {
   auto bp = dynamic_cast< BinPackingBlock * >( block );
   if( ( ! bp ) || ( v_sizes.size() != f_rng.second - f_rng.first ) )
    throw( std::invalid_argument( "BinPackingBlockRngdChange::apply: "
                                  "wrong Block or sizes" ) );

   BinPackingBlockRngdChange * undo = nullptr;
   if( doUndo ) {                     // record the sizes it overwrites
    std::vector< double > old( v_sizes.size() );
    for( auto i = f_rng.first ; i < f_rng.second ; ++i )
     old[ i - f_rng.first ] = bp->get_Size( i );
    undo = new BinPackingBlockRngdChange( f_rng , std::move( old ) );
    }

   // the new sizes, through the mutator of the Block
   for( auto i = f_rng.first ; i < f_rng.second ; ++i )
    bp->chg_size( v_sizes[ i - f_rng.first ] , i , issueMod , issueAMod );

   return( undo );
   }

  void serialize( netCDF::NcGroup & group ) const override {
   Change::serialize( group );        // the "type" attribute
   // ... f_rng and v_sizes as netCDF variables ...
   }

  void deserialize( const netCDF::NcGroup & group ) override {
   // ... read back what serialize() writes ...
   }

 private:
  Block::Range f_rng;                 // the items changed
  std::vector< double > v_sizes;      // their new sizes

  SMSpp_insert_in_factory_h;          // Change::new_Change() and netCDF
};
```

```cpp
SMSpp_insert_in_factory_cpp_0( BinPackingBlockRngdChange );
```

The essential points ([Chapter 16](16-change.md#ch-16)) are that
`apply()` enacts the change through the `chg_*()` interface of the
`:Block` (so that the `:Change` supports at most what the `:Block`
supports), and that it returns, when `doUndo` is set, an UndoChange
holding the sizes it overwrites, which satisfies the hypothesis of
Lemma 16.1 since applying it restores them; and the class is factory- and
netCDF-registered so it can be reconstructed from a serialised
description on another process or at a later time. A `:Change` that
cannot cheaply build its inverse simply declines to support undo
for those cases ([§16.2](16-change.md#sec-16-2)).

## C.5 Checklist {#sec-C-5}

- Define a `<Block>Mod` (and `Rngd` / `Sbst` variants as needed)
  deriving from `Modification`, carrying the description of a
  physical change, with `concerns_Block() == false`, implementing
  `get_Block()` and `print()`, and passing its `ModConcern` to the
  constructor ([§C.2](C-writing-modification.md#sec-C-2)). Issue it from the corresponding `chg_*()`
  mutator ([§A.5](A-writing-block.md#sec-A-5)).
- Rely on the framework's existing abstract-stream `Modification`s
  (`C05FunctionMod`, `VariableMod`, ...) for changes made through
  the abstract face; intercept and mirror them in
  `add_Modification()` ([§A.6](A-writing-block.md#sec-A-6)).
- Add a `:Change` family only if serialisable, transmissible or
  undoable edits are needed; register it in the factory and implement
  `apply()` (with the optional undo), `serialize()` and `deserialize()`
  (beta).

---

*This is the end of the SMS++ User Manual, version 0.1, for SMS++
0.8.0.*
