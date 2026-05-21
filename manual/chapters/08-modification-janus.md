# 8. Modification and the Janus discipline {#ch-8}

[Source: `SMS++/include/Modification.h`, `Observer.h`,
`MCFBlock/include/MCFBlock.h`]

A `Block` and its abstract representation can both change after a
`:Solver` has been attached to the `Block`. [Chapter 6](06-solver.md#ch-6) introduced
the *pending list* of `Modification`s through which the framework
delivers such changes to a `:Solver`; this chapter makes the
machinery precise. It also makes precise the principal idiom by
which a `:Block` keeps its physical and its abstract faces in
sync — the **Janus discipline** — and the few `ModParam` knobs
that the user has to dial the behaviour.

## 8.1 Concept {#sec-8-1}

A
[`Modification`](https://smspp.gitlab.io/smspp-project/d1/d3c/class_s_m_spp__di__unipi__it_1_1_modification.html)
is a small immutable object that records a change. It is the
notification the framework dispatches when *anything* in a
`:Block`, or in its abstract representation, is modified after a
`:Solver` has been attached. The notification is **lazy** in two
distinct senses: it is *issued* lazily by the object that performs
the change (the framework does not interrupt the change with
synchronous callbacks), and it is *consumed* lazily by the
`:Solver` (which processes its pending list only at the next call
to `compute()`).

Each `:Solver` that is registered with the `Block` — or with any
ancestor of the `Block`, recursively — receives a pointer to the
`Modification`, owned by a `std::shared_ptr`. A `Modification` is
*always* held through shared pointers, and this is precisely why
the framework needs no global registry of pending `Modification`s
and no explicit lifetime management: a `Modification` is
automatically deallocated as soon as the last `:Solver` (or
`:Block`) that has seen it releases its pointer. Each `:Solver`
keeps its own list of the `Modification`s it still has to react
to, and simply drops each entry once it has reacted; when the
last interested party has dropped it, the object is gone.

The two basic methods involved are
`Block::add_Modification(sp_Mod, ChnlName)`, which routes a fresh
`Modification` to all interested `:Solver`s, and
`Solver::add_Modification(sp_Mod, ChnlName)`, which appends the
`Modification` to the `:Solver`'s pending list. The first is
called by the `:Block` (or by one of its `Variable` / `Constraint`
/ `Function` instances); the second is called by the framework
on the `:Solver`'s behalf.

## 8.2 The hierarchy of Modifications {#sec-8-2}

The `Modification` class is the abstract base. Its principal
subclasses, all in `SMS++/include/Modification.h`, are:

- `Modification` (line 357) — the abstract base. Carries a
  pointer to the `:Block` that originated the change
  (`get_Block()`), a `concerns_Block()` flag (default `false`),
  and the channel the `Modification` is sent on. Concrete
  subclasses encode *what* changed.
- `AModification` (line 451) — the abstract "abstract"
  `Modification`. This is the base class for every `Modification`
  issued *by the abstract representation*: a `Variable` being
  fixed, a `Constraint` being relaxed, a `Function`'s coefficient
  being changed, and so on. `AModification` is exactly the kind
  of `Modification` whose `concerns_Block()` is `true` by default,
  to signal to the receiving `:Block` that it should react.
- `NModification` (line 522) — "nuclear" `Modification`. Used as
  a base for the few cases where the entire content of an object
  is replaced.
- `NBModification` (line 572) — "nuclear `Block`" `Modification`:
  issued by `Block::load(...)` and by `deserialize(...)` to
  signal that the entire `Block` has been re-loaded from scratch
  and any prior computation must be discarded. This is the
  "nuclear option" of [§4.6](04-block.md#sec-4-6).
- `GroupModification` (line 635) — a `GroupModification` bundles
  several `Modification`s into one logical unit. A receiving
  `:Solver` may either react to each sub-`Modification` in turn
  or treat the whole group as a single atomic change. This is the
  principal way to express that several individual changes have
  happened together and should be reacted to together.
- `VariableGroupMod` (line 778) — a `GroupModification` that
  specifically groups `Variable`-related Modifications; rarely
  used directly by user code, but produced by some `:Block`
  operations that touch multiple `Variable`s at once.

Each concrete `:Block` typically defines its own `:Modification`
subclasses for the changes that its physical representation
supports. For `MCFBlock`, the classes are:

- `MCFBlockMod` (`MCFBlock.h:2275`), the base for all
  `MCFBlock`-specific physical changes;
- `MCFBlockRngdMod` (`MCFBlock.h:2355`), a *range-based* change:
  a contiguous range of arcs or nodes has had its costs,
  capacities, or deficits modified, or has been added or
  removed;
- `MCFBlockSbstMod` (`MCFBlock.h:2406`), a *subset-based* change:
  an arbitrary subset of arcs or nodes has been similarly
  modified.

Analogous Rngd / Sbst pairs exist for `BinaryKnapsackBlock`
(`BinaryKnapsackBlockMod`, `BinaryKnapsackBlockRngdMod`,
`BinaryKnapsackBlockSbstMod`) and for
`CapacitatedFacilityLocationBlock` and most other concrete
`:Block` classes.

## 8.3 The two streams {#sec-8-3}

There are two parallel streams of `Modification`s, both arriving
at the same list inside each `:Solver`, distinguished by what they
say about themselves.

The **physical** stream consists of `Modification`s issued by the
`:Block`'s own mutation methods (the `chg_*()`, `fix_x()`,
`close_arc()`, `add_arc()`, ... family). These `Modification`s
descend directly from `Modification`, *not* from `AModification`;
their `concerns_Block()` returns `false`, because the change in
the `:Block` has *already happened* by the time the `Modification`
is constructed. A specialised `:Solver` consumes these and reacts
by recomputing what it can. An `MCFBlock` cost-change, for
instance, produces an `MCFBlockRngdMod` with the indices of the
changed arcs and the old / new costs (`MCFBlock.h:1665-1693`).

The **abstract** stream consists of `Modification`s issued by the
`:Block`'s abstract representation when something inside it
changes — when a `Variable` is fixed, when a `Constraint` is
relaxed, when a `Function`'s coefficient is updated, when a
dynamic `Constraint` is added or removed. These all descend from
`AModification`; their `concerns_Block()` returns `true` by
default. The `true` value of `concerns_Block()` is precisely the
flag that tells the receiving `:Block` "*you* are also expected
to react to this `Modification` by mutating your physical
representation accordingly", as discussed in [§8.4](08-modification-janus.md#sec-8-4).

A general-purpose `:Solver` such as `:MILPSolver` consumes the
abstract stream and is essentially indifferent to the physical
one (it does not know what to do with an `MCFBlockRngdMod` and
would simply discard it). Conversely, a specialised `:Solver`
typically consumes the physical stream and may safely discard
the abstract `Modification`s issued by a change that has produced
both (because reacting to the same change twice would be
incorrect).

## 8.4 The Janus discipline {#sec-8-4}

A change to a `:Block`'s data may originate from either face:

- The user calls a `:Block`-specific mutator, such as
  `MCFBlock::chg_cost(NCost, arc, issueMod, issueAMod)`. This
  changes the physical data, issues an `MCFBlockRngdMod` (the
  physical `Modification`), and — if `issueAMod` says so —
  *also* updates the corresponding coefficient in the linear
  `Function` inside `f_obj`. The update of the abstract
  representation is itself a change to a `Variable`-dependent
  `Function` and will normally trigger an abstract
  `C05FunctionMod` of its own; the framework ensures that the
  abstract `Modification` carries `concerns_Block() == false`
  in this scenario, because the `:Block` has obviously already
  applied the change and need not be told to do so again.

- The user, or a `:Solver`, calls an abstract-side mutator
  directly, such as `x[ a ].is_fixed( true , eModBlck )` to fix
  the flow on arc `a` to zero (idiomatic abstract-side way to
  *close* the arc), or an analogous mutator on a `Function`
  carrying the abstract `Constraint`/`Objective` data. This
  changes the abstract representation; the framework
  constructs the corresponding abstract `Modification` and calls
  `Block::add_Modification(sp_Mod)` to dispatch it. The
  `Modification` carries `concerns_Block() == true`. The
  concrete `:Block`'s override of `add_Modification(...)` is
  expected to *intercept* it: detect that this is a kind of
  change it knows how to translate to its physical representation,
  apply the corresponding physical change, and issue the
  corresponding physical `Modification` *before* forwarding the
  abstract `Modification` to the listening `:Solver`s. The
  abstract `Modification` is forwarded with `concerns_Block()`
  rewritten to `false` (it has done its duty of informing the
  `:Block`).

This is the **Janus discipline**: a `:Block` has two faces and
each face is responsible for telling the other when it has
changed. The mechanism is symmetric in the user-visible interface
(a user can change either face and the other will adapt) but
deliberately *not* symmetric internally — each change generates
*both* a physical and an abstract `Modification`, so that both
specialised and general-purpose `:Solver`s see exactly what they
need to see.

### The flow seen from the `:Block`

When the change originates on the abstract face, the sequence of
events at the `:Block` is precise and worth spelling out, because
it is exactly the recipe a `:Block` author has to follow (see
also [Appendix A](A-writing-block.md#app-A)) and the canonical override scheme documented at
`Block.h:5394-5402`:

1. *Someone modifies the abstract representation* — a `Variable`
   is fixed, a `Constraint`'s bound is changed, a coefficient of
   a `Function` carrying an abstract `Constraint`/`Objective` is
   updated.
2. *An abstract `Modification` is generated*, with
   `concerns_Block() == true`, and `Block::add_Modification(...)`
   is invoked with it.
3. *The concrete `:Block` reacts*: its `add_Modification(...)`
   override detects `concerns_Block() == true`, immediately sets
   it to `false`, and applies the matching change to its physical
   representation.
4. *The (now `concerns_Block() == false`) abstract `Modification`
   is forwarded onwards* — to the `:Block`'s father and to the
   listening `:Solver`s — but only if there is anyone listening
   (cf. [§8.7](08-modification-janus.md#sec-8-7)).

The reaction in step 3 typically consists of calling one or more
of the `:Block`'s own physical `chg_*()` mutators, with the two
`ModParam` arguments set as follows:

- `issueMod = eNoBlck` — perform the physical change and issue
  the physical `Modification`, but *only if there is anyone
  listening*, and with `concerns_Block() == false` (the abstract
  side has nothing to react to);
- `issueAMod = eDryRun` — do *not* touch the abstract
  representation again, because the change there has already
  happened (it is what triggered this whole sequence); issuing
  another abstract `Modification` would double-count the change.

In short: an abstract-origin change becomes, inside the `:Block`,
a physical change issued with `(eNoBlck, eDryRun)`. A
physical-origin change, conversely, is issued with `(eModBlck,
eModBlck)` (the defaults) and the abstract side is updated as a
genuine second action.

### Why the `:Block` sees the abstract `Modification` immediately

A subtle but important property holds:

> the abstract `Modification` reaches its `:Block` *immediately*,
> before it can be packed into any `GroupModification`.

This matters. If the abstract `Modification` could be buffered —
inserted into an open `GroupModification` (see [§8.6](08-modification-janus.md#sec-8-6)) and only
delivered later, when the group is closed — then by the time the
`:Block` finally "saw" it, the abstract representation could have
undergone arbitrarily many further changes. The `Modification`
might, for instance, refer to a *dynamic* `Variable` that has
since been deleted, forcing the `:Block` to carry logic to detect
whether that has happened and to cope with it.

The framework rules this out. As documented at
`Block.h:5404-5427`, the `:Block`'s `add_Modification(...)`
override sees the abstract `Modification` "naked" — *before* the
base-class machinery packs it into any `GroupModification`, even
when the `Modification` is being sent to a non-zero channel
defined in the `:Block`. Moreover, an abstract change must be
performed while the `:Block` is `lock()`-ed ([Chapter 17](17-parallel.md#ch-17)), so no
other thread can be mutating it concurrently. The combined effect
is that

> when a `:Block` processes an abstract `Modification`, the state
> of its data structures is *exactly* the state in which the
> change was issued: the change has just happened, and nothing
> else can have happened in the meantime.

This is a substantial simplification: a *leaf* `:Block` (with no
sub-`Block`) never has to handle an abstract `GroupModification`
at all, and never has to reason about "what else might have
changed since". It only ever has to react to one atomic abstract
`Modification` at a time, applied to a data structure that is in
the precise state the `Modification` describes. A non-leaf
`:Block` has to handle abstract `GroupModification`s only insofar
as they refer to changes inside its sub-`Block`s.

### Refusing an abstract change

A `:Block` is free to *refuse* an abstract change it does not
know how to translate. `MCFBlock`, for instance, does not allow
adding or removing arcs through the abstract representation (such
an operation would also require changing the structure of the
flow-conservation `FRowConstraint`s, which is more complex than
the current `add_Modification` implementation supports). The
refusal is concrete: the override throws a `std::logic_error`
with a descriptive message. The convention is that *failing loud*
is the safe default; a `:Block` that wants to support a
particular abstract change must do so explicitly.

## 8.5 The four `ModParam` values {#sec-8-5}

Every mutator that may issue a `Modification` takes one or two
`ModParam` arguments — `issueMod` for the physical
`Modification`, `issueAMod` for the abstract one — with default
value `eModBlck`. The enum
`amododification_type` (`Modification.h:908-913`) takes four
values:

| `ModParam` | Numeric | Semantics |
|---|---|---|
| `eDryRun` | 0 | The mutator must **not** perform the change, and consequently must not issue any `Modification`. Useful when a method that normally changes both the physical and the abstract face is called in a context where the change has already been applied to one face by some other means; passing `eDryRun` to the other face's parameter switches that face off. |
| `eNoMod` | 1 | The mutator **performs** the change but issues **no** `Modification`. Should be used only when the user is certain that neither the `:Block` nor any `:Solver` will ever need this information (typical case: setting up a fresh `:Block` from scratch, before any `:Solver` has been attached). |
| `eNoBlck` | 2 | The mutator performs the change and issues the `Modification`, *but only if there is anyone listening* (cf. `anyone_there()`); furthermore, `concerns_Block()` is set to `false`, telling the receiving `:Block` (if any) that it must not re-apply the change to its physical representation. This is the value the framework uses internally when forwarding an abstract `Modification` after the `:Block` has already intercepted and applied it. |
| `eModBlck` | 3 | The mutator performs the change and issues the `Modification` unconditionally, with `concerns_Block()` set to `true`. This is the default, "most conservative" value: it ensures that everybody who may need to know is informed, and it makes the abstract `Modification` carry the flag that asks the `:Block` to also apply the change to its physical face if needed. |

The two parameters `issueMod` and `issueAMod` are independent and
both default to `eModBlck`. They can also carry a *channel name*
encoded in the upper bits (`Observer::make_par(iM, chnl)`), which
is the mechanism for routing related `Modification`s to a specific
channel so that they can be grouped; channels are the subject of
[§8.6](08-modification-janus.md#sec-8-6). In normal use both parameters default to channel 0 and the
channel machinery need not be touched.

## 8.6 Channels and `GroupModification` {#sec-8-6}

So far every `Modification` has been treated as if it were
dispatched the instant it is issued. That is the default, but it
is not the only possibility: SMS++ provides a mechanism to *batch*
a set of logically related `Modification`s and deliver them
together, as a single `GroupModification`. The mechanism is built
on **channels** ([Source: `Observer.h:303-374`,
`Block.h:5429-5442`]).

### Opening and closing a channel

Whoever is about to make a sequence of related changes can *open
a channel* on a `:Block`:

```cpp
ChnlName c = blk.open_channel();      // open a fresh channel
//  ... make several related changes, all passing channel c ...
blk.close_channel( c );               // close it: the group is shipped
```

`open_channel()` (with the default argument `0`) creates a fresh
`GroupModification`, gives it a unique name, and returns that name
as the `ChnlName c`. From that moment, any `Modification` issued
*on channel `c`* is **not** dispatched to the `:Solver`s and the
ancestor `:Block`s; instead it is appended to the
`GroupModification`. The accumulated group is dispatched as one
object only when `close_channel(c)` is called.

A `Modification` is issued "on channel `c`" by passing `c` to the
`ModParam` argument of the mutator, encoded via
`Observer::make_par(issueMod, c)`. Alternatively, a `:Block`'s
*default channel* can be set once with `set_default_channel(c)`,
so that subsequent mutators called with a plain `eModBlck` /
`eNoBlck` are automatically routed to `c`.

### Nesting

The mechanism is recursive. Calling `open_channel(c)` with the
name `c` of an already-open channel does *not* open an independent
channel: it *nests* a new `GroupModification` inside the one
currently associated with `c`. Subsequent `Modification`s on `c`
go into the inner group. `close_channel(c)` then closes only the
innermost group and "returns control" to the one immediately
above; the accumulated content stays inside the outer group.
Only when the *root* `GroupModification` (the one created by the
original `open_channel(0)`) is finally closed is the whole nested
structure dispatched. This allows the construction of arbitrarily
tree-nested groups of related `Modification`s.

`open_channel()` also accepts an optional pointer to a
`GroupModification` (or to a derived class of it): if provided,
that object is "adopted" as the group being opened, which lets the
caller attach extra information that a `:Solver` or `:Block` may
find useful when processing the group.

### What it is for

The purpose is to make a `:Solver`'s life easier when several
changes are *correlated* and are best reacted to together. The
prototypical example is changing all the coefficients of a single
"column" of a model: those coefficients live in several different
`Constraint`s (in the `Function`s carrying them), so changing the
column produces one `Modification` per `Constraint`. A `:Solver`
that receives them one at a time may redo expensive bookkeeping
once per `Modification`; a `:Solver` that receives them as a
single `GroupModification` can recognise the pattern and react
once, far more efficiently. `BundleSolver`, for instance,
exploits `GroupModification`s in exactly this way.

This mechanism is, frankly, **not used as much as it could be**,
but it exists and is available to any `:Solver` and any code that
mutates a `:Block`.

### The cost of batching

The downside of batching is the very property that [§8.4](08-modification-janus.md#sec-8-4) showed to
be absent for the abstract `Modification` reaching its own
`:Block`: when `Modification`s are buffered into a
`GroupModification`, a `:Solver` may end up "seeing" many of them
at once, possibly long after the first was issued and after the
underlying objects have undergone many further changes. A
`:Solver` that consumes `GroupModification`s therefore generally
needs more complex logic than one that consumes naked
`Modification`s: it must be prepared for the fact that, by the
time it processes a buffered `Modification`, the relevant
`Variable`/`Constraint` may already have been further changed (or
even, for dynamic ones, removed). This is exactly the complication
that the "immediate delivery to the originating `:Block`"
guarantee of [§8.4](08-modification-janus.md#sec-8-4) spares a `:Block` from — but a `:Solver` that
opts into channel batching does not get that guarantee for free,
and must handle the accumulated group with the appropriate care.

## 8.7 The "anyone listening" filter {#sec-8-7}

A `:Block` with no `:Solver` attached and no `:Solver` attached
to any ancestor is, by construction, silent: no one is listening
and no one will react to any `Modification`. Issuing a
`Modification` in that case is wasted work.

The framework caches the boolean answer to "*is anyone
listening?*" in the `f_at` field of `Block`, updated by
`register_Solver(...)` / `unregister_Solver(...)` and propagated
recursively. The query method is `anyone_there()`. Mutators that
take `issueMod = eNoBlck` consult the cache before constructing
the `Modification`: if `anyone_there()` is `false`, the
`Modification` is silently dropped. Mutators called with the
default `eModBlck` do not consult the cache: they issue the
`Modification` anyway, because the `concerns_Block() == true`
flag may still need to reach the `:Block` itself even when no
`:Solver` is listening (for instance, to apply the corresponding
physical change in the Janus discipline).

A concrete `:Block` may override `anyone_there()` to return
`true` unconditionally whenever its abstract representation is
constructed (because abstract `Modification`s must then flow
regardless of the registered `:Solver`s, to keep the two faces
in sync). `MCFBlock` does exactly this; `BinaryKnapsackBlock`
also has the analogous override.

## 8.8 Inline example: a cost change in `MCFBlock`, observed from both faces {#sec-8-8}

The example below changes a cost in an `MCFBlock` via the
physical interface, then via the abstract interface, and observes
the resulting `Modification`s by attaching a simple "tap"
`:Solver` whose only role is to record everything that arrives
in its pending list.

```cpp
#include "MCFBlock.h"
#include "FakeSolver.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 // construct and load an MCFBlock (as in §4.5)
 MCFBlock mcf;
 mcf.load( 3 , 3 ,
           MCFBlock::Subset      { 1, 2, 2 } ,   // pEn
           MCFBlock::Subset      { 0, 1, 0 } ,   // pSn
           MCFBlock::Vec_FNumber { 3.0, 3.0, 3.0 } ,  // pU
           MCFBlock::Vec_CNumber { 2.0, 3.0, 4.0 } ,  // pC
           MCFBlock::Vec_FNumber { -2.0, 0.0, +2.0 } );

 // attach a tap: FakeSolver simply accumulates every Modification
 // it receives without ever doing anything with them. This is the
 // canonical way to observe the Modification stream.
 auto * tap = new FakeSolver();
 mcf.register_Solver( tap );

 // make sure the abstract representation is built, so that the
 // abstract stream will also be visible.
 mcf.generate_abstract_variables();
 mcf.generate_abstract_constraints();
 mcf.generate_objective();

 // ---- physical-side change: lower the cost of arc 0 ----
 mcf.chg_cost( /* NCost = */ 1.0 , /* arc = */ 0 ,
               /* issueMod  = */ eModBlck ,
               /* issueAMod = */ eModBlck );
 // The framework issues:
 //   - one physical MCFBlockRngdMod with rng = [0, 1) and the
 //     new cost vector,
 //   - one abstract C05FunctionMod (with concerns_Block() == false)
 //     reflecting the change in the linear Function inside f_obj.

 // ---- abstract-side change: close arc 1 by fixing its flow to 0 ----
 // The idiomatic abstract-face counterpart of MCFBlock::close_arc(1)
 // is fixing the corresponding ColVariable to 0. This is a pure
 // abstract change; the framework intercepts it and applies the
 // matching physical operation.
 ColVariable * x1 = mcf.get_Var( /* arc index = */ 1 );
 x1->set_value( 0.0 );
 x1->is_fixed( true , eModBlck );
 // The framework issues:
 //   - one abstract VariableMod with concerns_Block() == true,
 //     which mcf.add_Modification() intercepts;
 //   - mcf then performs the corresponding physical close_arc(1),
 //     issues a physical MCFBlockRngdMod marking the arc as
 //     closed, and forwards the abstract Modification with
 //     concerns_Block() rewritten to false.

 // examine the tap's pending list
 for( auto & mod : tap->get_Modifications() )
  std::cout << *mod << '\n';

 mcf.unregister_Solver( tap , /* deleteold = */ true );
 return 0;
}
```

The salient observations:

1. **Both changes produce two `Modification`s.** The framework
   guarantees this so that listeners on either face see what they
   need. A pure-physical listener (an `MCFSolver`) ignores the
   abstract one; a pure-abstract listener (an `:MILPSolver`)
   ignores the physical one.
2. **The Janus discipline is invisible from the user's vantage
   point.** Whether the change came through the physical mutator
   `chg_cost(...)` or through an abstract-face mutator such as
   `ColVariable::is_fixed(true, eModBlck)`, both faces end up
   consistent and both streams are populated.
3. **`concerns_Block()` is `true` only on abstract
   `Modification`s that have not yet been digested by the
   originating `:Block`.** After the `:Block`'s
   `add_Modification(...)` override has run, the flag is reset to
   `false` before the `Modification` is forwarded to listeners.
4. **`FakeSolver`** is a `:Solver` that does nothing except
   recording the `Modification`s it receives; the framework
   provides it as a debugging aid and as a convenience for code
   that needs to *defer* reacting (e.g. while a sub-`Block` is
   being modified in bulk and the changes will be replayed
   later).

## 8.9 Idioms {#sec-8-9}

**Default to `eModBlck`.** The default value of `issueMod` and
`issueAMod` is `eModBlck` for a reason: it is the safest. Anything
else is an optimisation that should be applied only when the user
has thought through what the listeners need.

**Use `eNoMod` only when no listener exists yet.** The natural
place to pass `eNoMod` is during the initial construction of a
`:Block`, before any `:Solver` has been registered: the absence
of listeners makes the `Modification`s useless anyway. After any
`:Solver` has been attached, `eNoMod` becomes dangerous (a
`:Solver` that does not learn about the change will produce
stale results).

**Use `eDryRun` to suppress one face in a coordinated update.**
The canonical use of `eDryRun` is inside the `:Block`'s own
`add_Modification(...)` override: when an incoming abstract
`Modification` has applied a change to the abstract face and the
`:Block` needs to update the physical face accordingly, it calls
the physical mutator with `issueMod = some_value, issueAMod =
eDryRun` to say "do the physical change, do not redo the abstract
one (it is already done)".

**Group related changes.** When a single logical operation
produces several individual `Modification`s, the canonical idiom
is to bundle them into a `GroupModification`. A `:Solver` may
then react to the group as a unit, which is often more efficient
than reacting to each component in turn.

## 8.10 Forward reference to `Change` {#sec-8-10}

The current `Modification` design has one notable limitation:
`Modification`s carry enough information to *describe* a change
but not always enough to *replay* it on a different `:Block`, and
they cannot in general be serialised in a form that survives an
inter-process boundary. The
[`Change`](https://smspp.gitlab.io/smspp-project/db/d4d/class_s_m_spp__di__unipi__it_1_1_change.html)
concept ([Chapter 16](16-change.md#ch-16)) is the framework's response: a `Change`
carries the data needed to apply the corresponding change to any
copy of a `:Block`, can be `[de]serialize`-d to netCDF as a plain
object, and (where the `:Block` chooses to support it) can be
*undone* via an automatically-generated inverse `Change`. The
intended downstream use is message-passing distributed
computation; only a small number of `:Change` classes exist at
version 0.6.0 (in particular for `BinaryKnapsackBlock`) and the
interface is subject to change. **Status — beta.**
