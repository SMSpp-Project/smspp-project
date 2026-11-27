# Appendix C: writing a new `:Modification` or `:Change` {#app-C}

[Source: [`SMS++/include/Modification.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Modification.h),
[`SMS++/include/Change.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h),
[`SMS++/include/SMSFactory.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h)]

We are concerned in this appendix with the two objects of notification and of
edit that the author of a new `:Block` may need to define. The first is a
`:Modification`, needed whenever the `:Block` supports any change at all; the
second is a `:Change`, needed when the `:Block` is to be solved by a search
over moves, such as the Branch-and-X of [Chapter 16](16-change.md#ch-16), or
when its edits are to be undone, stored or shipped. As in [Appendix
B](B-writing-solver.md#app-B), the running example continues with
`BinPackingBlock` ([Appendix A](A-writing-block.md#app-A)).

Why a `:Block` needs its own `:Modification`s is explained in
[§C.1](C-writing-modification.md#sec-C-1), their naming and structure in
[§C.2](C-writing-modification.md#sec-C-2) and their registration in
[§C.3](C-writing-modification.md#sec-C-3); then,
[§C.4](C-writing-modification.md#sec-C-4) defines a `:Change`, and
[§C.5](C-writing-modification.md#sec-C-5) gives the checklist.

## C.1 Why a `:Block` needs its own `:Modification`s {#sec-C-1}

A `:Block` that exposes any `chg_*()` mutator must inform the listening
`:Solver`s of *what* changed, when the change happens ([Chapter
8](08-modification-janus.md#ch-8)). The base `Modification` hierarchy
already covers the generic cases: `NBModification` for a wholesale `load()`
([§4.6](04-block.md#sec-4-6)), and the abstract `C05FunctionMod`,
`VariableMod` and `RowConstraintMod` for the changes made through the abstract
face. Yet, a *physical* change of the data of a `:Block`, to which a
specialized `:Solver` should react, needs a `:Block`-specific `:Modification`
that carries the description of that change.

## C.2 Naming and structure conventions {#sec-C-2}

We name the `:Block`-specific `Modification`s by a uniform scheme
([Chapter 8](08-modification-janus.md#ch-8),
[§8.2](08-modification-janus.md#sec-8-2)):

- `<Block>Mod`, the base class of the physical `Modification`s of
  that `:Block` (e.g., `MCFBlockMod`, `BinaryKnapsackBlockMod`);
- `<Block>RngdMod`, a change of a contiguous interval
  $[\mathit{start}, \mathit{stop})$ of the data;
- `<Block>SbstMod`, a change of an arbitrary set of indices;
- additions and removals, where the `:Block` supports dynamic objects, as
  further types of the same classes (e.g., `eAddArc` and `eRmvArc` of
  `MCFBlockMod`).

A physical `:Modification` derives from `Modification` (not from
`AModification`), and its `concerns_Block()` is `false`, because the change in
the `:Block` has already happened by the time the object is constructed
([§8.3](08-modification-janus.md#sec-8-3)). It is a rather "thin" object,
which carries just enough to let a `:Solver` identify what changed, i.e., for
`BinPackingBlock`, the index (or the range, or the subset) of the items whose
size was modified:

```cpp
// BinPackingBlock.h (alongside the Block)
namespace SMSpp_di_unipi_it
{

class BinPackingBlockMod : public Modification
{
 public:
 // what a size change does: it is physical, it changes coefficients of
 // the (capacity) Constraints, and the region may shrink or grow
 static constexpr ModConcern size_changes =
   eModPhys | eModCnsCoef | eRegnShrink | eRegnGrow;

 BinPackingBlockMod( BinPackingBlock * blk , Block::Index item )
     : Modification( size_changes ) , f_block( blk ) , f_item( item ) { }

 Block * get_Block( void ) const override { return( f_block ); }
 Block::Index item( void ) const { return( f_item ); }

 protected:
 void print( std::ostream & output ) const override {
  output << "BinPackingBlockMod[" << this << "]: size of item " << f_item;
  }

 private:
 BinPackingBlock * f_block;
 Block::Index f_item;  // which item's size changed
 };

}  // namespace
```

Besides `get_Block()`, the only other method that a `:Modification` must
implement is the protected `print()`, to which `operator<<` dispatches. The
value passed to the constructor of `Modification` is the `ModConcern` returned
by `changes()`, a bit mask that states what *kind* of thing the
`Modification` changes (`eModPhys` for the physical representation, and
`eModVarData`, `eModVarType`, `eModVarSet`, `eModCnsSide`, `eModCnsCoef`,
`eModCnsSet`, `eModObj` for the parts of the model), and what *effect* it may
have on the problem (`eRegnShrink` / `eRegnGrow` for the feasible region,
`eObjUp` / `eObjDown` for the objective); an effect bit that is not set is a
guarantee. Note that the default `eModPhys | eEffAny` ("physical, anything may
happen") is always correct for a physical `:Modification`, and that declaring
more has three uses: it spares a `:Solver` that only reads some kinds the
others (`Solver::concerned_by()`, [§B.4](B-writing-solver.md#sec-B-4)), it
lets the `:Block` avoid issuing the `Modification` when no listener reads its
kind (cf. [§8.7](08-modification-janus.md#sec-8-7)), by the two-argument
`issue_pmod( issueMod , kind )` used in [§A.5](A-writing-block.md#sec-A-5),
and it lets a reader keep what is still valid, e.g., a solution when the
region cannot shrink. Four masks, `ePhysObj`, `ePhysSides`, `ePhysCns` and
`ePhysVars`, cover the common physical cases, and the bits are to be read
through the static methods of `Modification` (`is_physical()`,
`changes_only_sides()`, `may_shrink_region()`, and so on), instead of being
tested one by one. We remark that a physical `:Modification` reaches only the
listeners that read `eModPhys`, whatever other kinds it declares; only one
that declares `eModAll`, as the `NBModification` does, reaches all of them.
Yet, the other kinds that it declares describe the change in terms of the
model, while the listeners that only read the abstract representation see it
in the abstract `Modification`s that come with it.

A `RngdMod` / `SbstMod` would carry a `Range` / `Subset` in place of a single
index, mirroring the `Rngd` / `Sbst` shapes of the methods factory
([§15.3](15-methods-factory.md#sec-15-3)). The object above, in turn, is the
one that `BinPackingBlock::chg_size` issues in
[§A.5](A-writing-block.md#sec-A-5).

## C.3 Registering with the factory {#sec-C-3}

A `:Modification` is not registered in any factory. In fact, it is constructed
by the `:Block`, dispatched, consumed, and deallocated when the last holder
releases its `shared_ptr` ([§8.1](08-modification-janus.md#sec-8-1)), and
`Modification` has neither a factory nor methods for netCDF. The class-name
factory ([Chapter 18](18-factories-netcdf.md#ch-18)) is needed by `:Change`
and by the other serializable objects; hence, an edit that has to be stored or
shipped is a `:Change` ([§C.4](C-writing-modification.md#sec-C-4)).

## C.4 Defining a `:Change` (optional) {#sec-C-4}

We need `Change`s when the use case calls for undoable, serializable or
transmissible edits, as a search over moves does: the branchings of a
`:RelaxationSolver` for `BranchAndXSolver`, in particular, are `:Change`s
([§16.7](16-change.md#sec-16-7)). If the edits can be made on the abstract
representation of the `:Block`, the `AbstractChange` of the core may suffice
([§16.2](16-change.md#sec-16-2)), and no `:Change` of the `:Block` is
needed. A `:Change` for
`BinPackingBlock` represents a *prospective* change of sizes, carrying the new
sizes; hence, it can be `apply()`-ed to any `BinPackingBlock` with enough
items, serialized to netCDF, and produce its UndoChange:

```cpp
class BinPackingBlockRngdChange : public Change
{
 public:
 BinPackingBlockRngdChange( Block::Range rng = { 0 , 0 } ,
                            std::vector< double > && sizes = {} )
     : f_rng( rng ) , v_sizes( std::move( sizes ) ) { }

 Change * apply( Block * block , bool doUndo = false ,
                 ModParam issueMod = eNoBlck ,
                 ModParam issueAMod = eNoBlck ) override {
  auto bp = memo_cast< BinPackingBlock * >( block );
  if( ( ! bp ) || ( v_sizes.size() != f_rng.second - f_rng.first ) ||
      ( bp->get_NItems() < f_rng.second ) )
   throw( std::invalid_argument( "BinPackingBlockRngdChange::apply: "
                                 "wrong Block or sizes" ) );

  std::unique_ptr< BinPackingBlockRngdChange > undo;
  if( doUndo ) {  // record the sizes it overwrites
   std::vector< double > old( v_sizes.size() );
   for( auto i = f_rng.first ; i < f_rng.second ; ++i )
    old[ i - f_rng.first ] = bp->get_Size( i );
   undo = std::make_unique< BinPackingBlockRngdChange >( f_rng ,
                                                          std::move( old ) );
   }

  // the new sizes, through the mutator of the Block
  for( auto i = f_rng.first ; i < f_rng.second ; ++i )
   bp->chg_size( v_sizes[ i - f_rng.first ] , i , issueMod , issueAMod );

  return( undo.release() );
  }

 void serialize( netCDF::NcGroup & group ) const override {
  Change::serialize( group );  // the "type" attribute
  // ... f_rng and v_sizes as netCDF variables ...
  }

 void deserialize( const netCDF::NcGroup & group ) override {
  // ... read back what serialize() writes ...
  }

 private:
 Block::Range f_rng;             // the items changed
 std::vector< double > v_sizes;  // their new sizes

 SMSpp_insert_in_factory_h;  // Change::new_Change() and netCDF
 };
```

```cpp
SMSpp_insert_in_factory_cpp_0( BinPackingBlockRngdChange );
```

We now comment on the essential points (cf. [Chapter 16](16-change.md#ch-16)).
First, `apply()` enacts the change through the `chg_*()` interface of the
`:Block`, and therefore the `:Change` supports at most what the `:Block`
supports. Second, when `doUndo` is set it returns an UndoChange holding the
sizes it overwrites, which satisfies the hypothesis of [Lemma
16.1](16-change.md#lem-16-1) with equality, since applying it restores them. A
`:Change` that could not build an exact inverse in some state rather refuses
that state by an exception, as the `:Change`s of `BinaryKnapsackBlock` refuse
to fix an item that is already fixed ([§16.2](16-change.md#sec-16-2)). Third,
the class is registered in the factory and its `serialize()` calls
`Change::serialize()` first, so that one can reconstruct it out of a netCDF
group by `Change::new_Change( group )`, perhaps on another process or at a
later time. Also, the `:Change` identifies the items by their indices, never
by pointers to the objects of a particular `Block`; hence, it applies to any
identical copy of the `Block`, as the workers of the parallel exploration of
`BranchAndXSolver` require ([§16.10](16-change.md#sec-16-10)). Finally, the
type of the `Block` is recognized by `memo_cast()` (since SMS++ 0.9.0;
[§8.3](08-modification-janus.md#sec-8-3)), whose hypotheses ([Lemma
8.2](08-modification-janus.md#lem-8-2)) hold here, since the hierarchy of
`Block` has no virtual base.

## C.5 Checklist {#sec-C-5}

- Define a `<Block>Mod` (and `Rngd` / `Sbst` variants as needed) deriving
  from `Modification`, carrying the description of a physical change, with
  `concerns_Block() == false`, implementing `get_Block()` and `print()`, and
  passing its `ModConcern` to the constructor
  ([§C.2](C-writing-modification.md#sec-C-2)). Issue it from the corresponding
  `chg_*()` mutator ([§A.5](A-writing-block.md#sec-A-5)).
- Rely on the framework's existing abstract-stream `Modification`s
  (e.g., `C05FunctionMod` and `VariableMod`) for changes made through the
  abstract face; intercept and mirror them in `add_Modification()`
  ([§A.6](A-writing-block.md#sec-A-6)).
- Add a `:Change` family if undoable, serializable or transmissible edits are
  needed ([§C.4](C-writing-modification.md#sec-C-4)); register it in the
  factory and implement `apply()` (with the undo, refusing the states in which
  it could not be exact), `serialize()` (calling `Change::serialize()` first)
  and `deserialize()`.

