# Appendix C — Writing a new `:Modification` / `:Change`

The last appendix covers the two notification/edit objects a new
`:Block` author may need to define: a `:Modification` (always, if
the `:Block` supports any change at all) and, optionally, a
`:Change` (for serialisable / undoable / transmissible edits). The
running example continues with `BinPackingBlock` (Appendix A).

## C.1 Why a `:Block` needs its own `:Modification`s

A `:Block` that exposes any `chg_*()` mutator must, when the change
happens and a `:Solver` is listening, tell that `:Solver` *what*
changed (Chapter 8). The base `Modification` hierarchy already
covers the generic cases — `NBModification` for a wholesale
`load()` (§4.6), the abstract-stream `C05FunctionMod` /
`VariableMod` / `RowConstraintMod` for changes made through the
abstract face — but a *physical* change to a `:Block`'s own data
that a specialised `:Solver` should react to needs a
`:Block`-specific `:Modification` carrying the description of that
change.

## C.2 Naming and structure conventions

SMS++ follows consistent naming for `:Block`-specific
`Modification`s (Chapter 8, §8.2):

- `<Block>Mod` — the base class for that `:Block`'s physical
  `Modification`s (e.g. `MCFBlockMod`, `BinaryKnapsackBlockMod`);
- `<Block>RngdMod` — a *range*-based change: a contiguous
  `[start, stop)` interval of the data changed;
- `<Block>SbstMod` — a *subset*-based change: an arbitrary set of
  indices changed;
- `<Block>ModAdd` / `<Block>ModRmv` — additions / removals, where
  the `:Block` supports dynamic objects.

A physical `:Modification` derives from `Modification` (not from
`AModification`); its `concerns_Block()` is `false`, because the
change in the `:Block` has already happened by the time the object
is constructed (§8.3). It carries just enough to let a `:Solver`
identify what changed — for `BinPackingBlock`, the index (or range,
or subset) of the items whose size was modified:

```cpp
// BinPackingBlock.h (alongside the Block)
namespace SMSpp_di_unipi_it {

class BinPackingBlockMod : public Modification
{
 public:
  BinPackingBlockMod( BinPackingBlock * blk , Block::Index item )
   : f_block( blk ) , f_item( item ) {}

  Block * get_Block( void ) const override { return( f_block ); }
  Block::Index item( void ) const { return( f_item ); }

 private:
  BinPackingBlock * f_block;
  Block::Index      f_item;     // which item's size changed
};

}  // namespace
```

A `RngdMod` / `SbstMod` would carry a `Range` / `Subset` instead of
a single index, mirroring the `Rngd` / `Sbst` shapes of the methods
factory (§15.3). This is the object `BinPackingBlock::chg_size`
issued in §A.5.

## C.3 Registering with the factory

A plain `:Modification` does not generally need to be in a factory
(it is constructed by the `:Block`, dispatched, consumed, and
deallocated when the last holder releases its `shared_ptr`, §8.1).
It is `:Change` and the serialisable objects that need the
class-name factory. A `:Modification` that *does* need to be
reconstructed from netCDF would use the same
`SMSpp_insert_in_factory_*` macros as everything else (Chapter 18).

## C.4 Defining a `:Change` (optional)

> **Status — beta.** The `Change` mechanism (Chapter 16) is in
> `develop` and its interface may change. Add a `:Change` family
> only when the use case calls for serialisable, transmissible, or
> undoable edits (a distributed solver, a move-based search).

A `:Change` for `BinPackingBlock` would represent a *prospective*
size change — carrying the new size(s) so it can be `apply()`-ed to
any compatible `BinPackingBlock`, serialised to netCDF, and (where
the change is invertible) produce its UndoChange:

```cpp
class BinPackingBlockRngdChange : public Change
{
 public:
  // ... constructor storing the affected range and the new sizes ...

  Change * apply( Block * block , bool doUndo = false ,
                  ModParam issueMod = eNoBlck ,
                  ModParam issueAMod = eNoBlck ) override
  {
   auto * bp = dynamic_cast< BinPackingBlock * >( block );
   // if doUndo, first capture the current sizes into an inverse Change
   Change * undo = doUndo ? /* a Change restoring the old sizes */ : nullptr;
   // apply the new sizes through the Block's own mutator
   for( /* each affected item i with new size s_i */ )
    bp->chg_size( s_i , i , issueMod , issueAMod );
   return( undo );
  }

  void serialize( netCDF::NcGroup & group ) const override { /* ... */ }
  void deserialize( const netCDF::NcGroup & group ) override { /* ... */ }

 private:
  SMSpp_insert_in_factory_h;     // Change::new_Change("...") and netCDF
};
```

```cpp
SMSpp_insert_in_factory_cpp_0( BinPackingBlockRngdChange );
```

The essential points (Chapter 16): `apply()` enacts the change
through the `:Block`'s own `chg_*()` interface (so the `:Change`
can support at most what the `:Block` supports), optionally returns
an UndoChange when `doUndo` is set, and the class is factory- and
netCDF-registered so it can be reconstructed from a serialised
description on another process or at a later time. A `:Change` that
cannot cheaply build its inverse simply declines to support undo
for those cases (§16.2).

## C.5 Checklist

- Define a `<Block>Mod` (and `Rngd` / `Sbst` variants as needed)
  deriving from `Modification`, carrying the description of a
  physical change, with `concerns_Block() == false` (§C.2). Issue
  it from the corresponding `chg_*()` mutator (§A.5).
- Rely on the framework's existing abstract-stream `Modification`s
  (`C05FunctionMod`, `VariableMod`, ...) for changes made through
  the abstract face; intercept and mirror them in
  `add_Modification()` (§A.6).
- Add a `:Change` family **only** if serialisable / transmissible /
  undoable edits are needed; register it in the factory and
  implement `apply()` (with optional undo), `serialize()`,
  `deserialize()`. **Status — beta.**

---

*This is the end of the SMS++ User Manual, version 0.1, for SMS++
0.6.0.*
