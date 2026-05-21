# 16. Change {#ch-16}

> **Status — beta.** The `Change` concept is in `develop` but its
> interface and semantics may change in incompatible ways. Only a
> small number of `:Change` classes exist at version 0.6.0
> (notably for `BinaryKnapsackBlock`). Code written against it
> today should expect to be revisited.

[Source: `SMS++/include/Change.h`,
`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`]

## 16.1 Concept {#sec-16-1}

A
[`Change`](https://smspp.gitlab.io/smspp-project/db/d4d/class_s_m_spp__di__unipi__it_1_1_change.html)
is, like a `Modification`, an object that describes a change to a
`Block`. The two are duals of each other in *time*
(`Change.h:55-84`):

- a `Modification` ([Chapter 8](08-modification-janus.md#ch-8)) *notifies* of a change that **has
  already occurred** in a `Block`, so that listening `:Solver`s
  can react;
- a `Change` *represents* a change that **may be applied later**,
  carrying all the data needed to perform it.

This difference in orientation has a concrete consequence: a
`Change` must contain *all the new data* of the change, because it
has to be able to *enact* the change, not merely describe one that
some other code already made. A `Modification` can be a thin
notification ("cost of arc 3 changed"); a `Change` must be a
self-contained instruction ("set the cost of arc 3 to 7.0").

The single defining operation is

```cpp
Change * apply( Block * block ,
                bool doUndo = false ,
                ModParam issueMod = eNoBlck ,
                ModParam issueAMod = eNoBlck );
```

which applies the change to `block` (issuing `Modification`s in
the usual way, governed by the two `ModParam`s), and — if `doUndo`
is `true` — returns an **UndoChange**: a freshly minted `:Change`
that, applied to `block`, would restore it to the state it was in
before (`Change.h:404-416`).

## 16.2 Abstract and specific `Change`s {#sec-16-2}

Like `Modification`s and `Block`s, `Change`s come in two flavours:

- an **abstract** `:Change` operates on the abstract
  representation and therefore applies to *any* `Block` (it fixes
  a `Variable`, relaxes a `Constraint`, ...);
- a **specific** `:Change` is written for a particular `:Block`
  and uses that `:Block`'s specialised interface to change its
  physical representation directly.

A specific `:Change` can support at most the changes that its
target `:Block` supports. It may deliberately support *fewer*, for
a reason peculiar to `Change`: producing the UndoChange of an
arbitrary change can be complicated or impossible, so a `:Change`
that wants to guarantee an undo may restrict itself to the subset
of changes it can reliably invert (`Change.h:74-84`).

## 16.3 Why `Change` exists {#sec-16-3}

### Serialisation and distribution

If a `Change` only ever applied a change in the same process that
created it, it would add little over calling the `:Block`'s
`chg_*()` methods directly. Its reason for existing is that, unlike
a `Modification`,

> a `Change` is a *plain, self-contained object* that can be
> `[de]serialize`-d to and from netCDF independently of any
> `Block`.

`Change` carries a class-name factory (`Change::new_Change(name)`,
`Change::deserialize(NcGroup)`) of exactly the same kind as
`Block`, `Solver` and `Solution` ([Chapter 18](18-factories-netcdf.md#ch-18)), so a `:Change`
read from a netCDF group is reconstructed as the right concrete
type and then `apply()`-ed. This makes a `Change` something a
`Modification` can never be: a portable, storable, *transmissible*
instruction. The intended downstream use is **message-passing
distributed computation** — a process computes a `Change`,
serialises it, ships it to another process holding a copy of the
`Block`, which deserialises and `apply()`s it. The `Change`
mechanism is, in this sense, the groundwork for the distributed
`:Solver`s that the project plans but does not yet ship.

### Support for generic search schemes

A second, equally important reason for `Change` to exist is that
it makes it possible to write search algorithms in terms of
abstract **moves** that are *independent of the `:Block` they are
applied to*. A branch-and-bound, a branch-and-X (B&X) scheme, a
large-neighbourhood search, a local-search metaheuristic — all of
these are, at heart, "apply a move, explore, then undo or commit".
If the move is a `Change`, the *search driver* need know nothing
about the concrete `:Block`: it applies `Change`s and rolls them
back, and the same driver works for any problem class that
supplies the appropriate `:Change`s. The undo facility ([§16.1](16-change.md#sec-16-1)) is
what makes the roll-back free: an automatically-produced
UndoChange restores the previous state wherever the `:Change`
supports it.

The B&X case is the most telling, and it shows why the `:Change`
abstraction is more than a convenience. The idea is that the
`Change`s encode the **branching rule**, and that they are
*produced by the `:Solver` that solves the relaxation*. Because
that `:Solver` may be a *specialised* one, it can construct the
branching `Change`s tailored not only to the structure of the
problem but also to its own ability to **reoptimize efficiently**
after the branch — choosing moves that its warm-start machinery
([Chapter 13](13-function-family.md#ch-13)'s reoptimization vocabulary) can absorb cheaply. The
search driver, meanwhile, stays completely generic: it receives
`Change`s from the relaxation `:Solver`, applies them to descend
the tree, and uses the UndoChanges to backtrack, without any
problem-specific code of its own. This decoupling — a generic
B&X driver on one side, a problem-and-`Solver`-specific source of
branching `Change`s on the other — is one of the more ambitious
uses the `Change` concept is meant to enable, and it is
**currently under development**.

## 16.4 The reference example: `BinaryKnapsackBlock`'s `Change`s {#sec-16-4}

`BinaryKnapsackBlock` is the reference implementation of the
`Change` family at version 0.6.0. It ships three specific
`:Change` classes:

- [`BinaryKnapsackBlockChange`](https://smspp.gitlab.io/smspp-project/d8/d50/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block_change.html)
  — a "whole-object" change;
- `BinaryKnapsackBlockRngdChange` — a *range*-based change (a
  contiguous interval of items);
- `BinaryKnapsackBlockSbstChange` — a *subset*-based change (an
  arbitrary set of items),

mirroring, on the `Change` side, the `Rngd` / `Sbst` distinction
that the `Modification` family draws ([Chapter 8](08-modification-janus.md#ch-8)) and that the
methods factory draws ([Chapter 15](15-methods-factory.md#ch-15)). Each carries the new data
(weights, profits, capacity, fixings) for the items it covers, can
be serialised to netCDF, applied to a `BinaryKnapsackBlock`, and —
within the subset it supports — produce its UndoChange.

```cpp
#include "BinaryKnapsackBlock.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 BinaryKnapsackBlock bkb;
 bkb.load( /* ... */ );

 // build a Change that sets new weights on items [0,3)
 // (constructed directly here; in a distributed setting it would
 //  instead be deserialized from a netCDF group shipped by another
 //  process via Change::deserialize(...))
 Change * chg = /* a BinaryKnapsackBlockRngdChange carrying the new
                   weights for items 0..2 */ ;

 // apply it, asking for the inverse so we can roll back later
 Change * undo = chg->apply( & bkb , /* doUndo = */ true );

 // ... explore the modified Block ...

 // restore the original state
 undo->apply( & bkb );

 delete chg;
 delete undo;
 return 0;
}
```

## 16.5 `Change` versus `Modification`: which to use {#sec-16-5}

For ordinary, in-process model edits — the bread-and-butter "change
a cost, re-solve" of Chapters [8](08-modification-janus.md#ch-8) and [10](10-r3block.md#ch-10) — the `Modification`
mechanism, driven by the `:Block`'s `chg_*()` methods, remains the
right tool, and is the one the rest of this manual uses. `Change`
is the right tool when one needs one of the things a `Modification`
cannot provide:

- to **serialise** an edit and apply it elsewhere or later;
- to **transmit** an edit across a process boundary (the
  distributed-`:Solver` use case);
- to **undo** an edit cheaply via an automatically-generated
  inverse.

Because the concept is **beta** and implemented for only a few
`:Block`s, a `:Block` author should regard adding a `Change`
family as an *optional* and forward-looking step ([Appendix C](C-writing-modification.md#app-C)), not
a requirement; and a user should reach for `Change` only when one
of the three needs above is actually present.
