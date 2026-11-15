# 8. Modification and the Janus discipline {#ch-8}

[Source: [`SMS++/include/Modification.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Modification.h), [`Observer.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Observer.h), [`Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h),
[`SMS++/src/Block.cpp`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp), [`MCFBlock/include/MCFBlock.h`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h)]

Both the physical and the abstract representation of a `Block` can
change after a `:Solver` has been attached to it. [Chapter
6](06-solver.md#ch-6) introduced the list of pending `Modification`s
through which the changes reach a `:Solver`. In this chapter we make
the machinery precise and prove the properties of the delivery on
which `:Block`s and `:Solver`s rely; we also state the discipline by
which a `:Block` keeps its two faces in step (the *Janus discipline*),
together with the parameters of type `ModParam` by which the user
tunes it.

## 8.1 Concept {#sec-8-1}

A
[`Modification`](https://smspp.gitlab.io/smspp-project/d1/d3c/class_s_m_spp__di__unipi__it_1_1_modification.html)
is a small object that records a change, and that is dispatched when
anything in a `:Block`, or in its abstract representation, changes
after a `:Solver` has been attached. The notification is lazy in two
senses: the object that makes the change does not call back anyone
synchronously, but issues the `Modification` and goes on, and the
`:Solver` processes its pending `Modification`s only at its next call
to `compute()`.

Each `:Solver` that listens to the `Block` (i.e., is registered with
it or with one of its ancestors) receives a `std::shared_ptr` to the
`Modification` (the type `sp_Mod`). Since `Modification`s are always
held through shared pointers, the framework needs no global registry
of the pending ones and no explicit management of their lifetime. Each
`:Solver` keeps a list of the `Modification`s it still has to react
to, and drops each entry once it has reacted; hence, a `Modification`
is deallocated as soon as the last `:Solver` (or `:Block`) holding it
drops its pointer.

Two methods are involved: `Block::add_Modification( sp_Mod , ChnlName )`,
which routes a new `Modification` to whoever is listening, and
`Solver::add_Modification( sp_Mod )`, which appends it to the list of the
`:Solver`. the first is called by the `:Block`, or by one of its `Variable`s,
`Constraint`s and `Function`s (which hold a pointer to the object that
observes them, ultimately the `Block`); the second is called by the `Block` on
behalf of the `:Solver`.

## 8.2 The hierarchy of Modifications {#sec-8-2}

The class `Modification` is the abstract base; its main derived
classes, all in [`SMS++/include/Modification.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Modification.h), are the following.

- `Modification` (line 357) carries the `changes()` bit mask, a coarse
  description of the kind of change (cf.
  [§8.7](08-modification-janus.md#sec-8-7)), and `concerns_Block()`,
  which is `false` in the base class (and in the `Modification`s
  derived directly from it); the concrete derived classes describe
  *what* has changed, and their method `get_Block()` returns the
  `Block` that has changed.
- `AModification` (line 739) is the base of every "abstract" `Modification`,
  i.e., of those issued by the abstract representation when a `Variable` is
  fixed, a `Constraint` is relaxed, a coefficient of a `Function` is changed,
  and so on. Its `concerns_Block()` is stored in the object, `true` by
  default, and `concerns_Block( false )` resets it; the value `true` means
  that the receiving `:Block` has to react
  (cf. [§8.4](08-modification-janus.md#sec-8-4)).
- `NModification` (line 814) is the "nuclear" `Modification`, the base
  of the few cases in which the content of an object is replaced
  altogether.
- `NBModification` (line 864) is the "nuclear `Block`" one, issued by
  `load( ... )` and `deserialize( ... )` of a `:Block` when the `Block` has
  been loaded anew, so that any previous computation has to be discarded (its
  `changes()` is `eModAll`). This is the nuclear option of
  [§4.6](04-block.md#sec-4-6).
- `GroupModification` (line 931), derived from `AModification`,
  bundles several `Modification`s (its `sub_Modifications()`) into
  one; a receiving `:Solver` can react to each of them in turn or to
  the group at once. Groups are created only by the channels of
  [§8.6](08-modification-janus.md#sec-8-6) (no other code of the core
  constructs one).
- `VariableGroupMod` (line 1107) is a `GroupModification` that groups
  the `Modification`s of several `Variable`s.

Each concrete `:Block` typically defines `:Modification`s for the
changes that its physical representation supports. For `MCFBlock` they
are `MCFBlockMod` ([`MCFBlock.h:2424`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L2424)), the base of all its physical
changes, and its two derived classes. `MCFBlockRngdMod`
([`MCFBlock.h:2529`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L2529)) is a change of a contiguous range of arcs or nodes
(new costs, capacities or deficits, or arcs opened, closed, added or
removed), and `MCFBlockSbstMod` ([`MCFBlock.h:2580`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L2580)) is the same change
for an arbitrary subset of arcs or nodes. Analogous pairs exist for
`BinaryKnapsackBlock` (`BinaryKnapsackBlockMod`,
`BinaryKnapsackBlockRngdMod`, `BinaryKnapsackBlockSbstMod`), for
`CapacitatedFacilityLocationBlock` and for most other concrete
`:Block`s.

## 8.3 The two streams {#sec-8-3}

Two streams of `Modification`s arrive at the same list of each
`:Solver`, distinguished by their type and their `changes()`. The
*physical* stream consists of the `Modification`s issued by the
methods of the `:Block` that change its data (the `chg_*()`,
`close_arc()`, `add_arc()` family, and so on). These derive from
`Modification`, not from `AModification`, and their `changes()` has
the bit `eModPhys`; their `concerns_Block()` is `false`, since the
change of the `:Block` has already happened when the `Modification` is
constructed. A specialized `:Solver` processes them by recomputing
what it can: a change of costs of an `MCFBlock`, for instance, issues
an `MCFBlockRngdMod` of type `eChgCost` with the range of the changed
arcs, the new costs being read from the `:Block`
([`MCFBlock.h:1769-1822`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L1769-1822)).

The *abstract* stream consists of the `Modification`s issued by the
abstract representation of the `:Block` when something in it changes:
a `Variable` is fixed, a `Constraint` is relaxed, a coefficient of a
`Function` is changed, a dynamic `Constraint` is added or removed.
They derive from `AModification`, and their `concerns_Block()` is
`true` unless the issuer has been told otherwise (cf.
[§8.5](08-modification-janus.md#sec-8-5)); with the value `true` the
receiving `:Block` is expected to react by changing its physical
representation accordingly (cf.
[§8.4](08-modification-janus.md#sec-8-4)).

A general-purpose `:Solver`, such as a `:MILPSolver`, processes the
abstract stream and has no use for the physical one (it would not
know what to do with an `MCFBlockRngdMod`); since it declares that it
reads only the abstract kinds, the physical `Modification`s do not
even reach it (Lemma 8.1). Conversely, a specialized `:Solver`
typically processes the physical stream, and it must ignore the
abstract `Modification`s that come from a change that has produced
both (reacting to the same change twice would be wrong), which it can
do by declaring through `concerned_by()` that it reads `eModPhys`
only.

### The delivery

Routing is done by the base method `Block::add_Modification( mod , chnl )`
([`SMS++/src/Block.cpp:769-815`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L769-815)), which the overrides of the `:Block`s call at
their end. In pseudocode it reads as follows (the channels are discussed in
[§8.6](08-modification-janus.md#sec-8-6)):

```
Block::add_Modification(B, mod, chnl):
  if not B.anyone_there(): return
  if chnl = 0: chnl <- B.default_channel
  if chnl != 0:
    if chnl is a channel opened in B: append mod to its current group; return
    if B has no father: throw invalid_argument
  if B has a father F: F.add_Modification(mod, chnl)    // virtual call
  for each Solver S registered with B:
    if is_of_concern(mod.changes(), S.concerned_by()): S.add_Modification(mod)
```

Here `is_of_concern( c , reads )` is `true` if `reads` comprises all
the kinds (`eModAnything`) or if `c` changes everything (`eModAll`).
Otherwise, it is `true` if `c` is physical and `reads` comprises
`eModPhys`, or if `c` is abstract and has a kind in common with
`reads`; a `GroupModification` is of concern if any of its
sub-`Modification`s is (`is_of_concern_to()`).

**Lemma 8.1** (Delivery). *Let `mod` be passed to
`Block::add_Modification( mod , 0 )` of a `Block` $B$ with no default
channel set in $B$ or in its ancestors, and let the overrides of
`add_Modification()` of the `:Block`s on the path from $B$ to the root
call the base method with the same `mod` and channel 0. Then a
`Solver` $S$ receives `mod` (once) if and only if $S$ is registered
with $B$ or with one of its ancestors and `mod` is of concern to
`S.concerned_by()`; no other `Solver` receives it.*

*Proof.* Let $B = B_0, B_1, \dots, B_h$ be the path from $B$ to the
root. A call on $B_k$ forwards `mod` to $B_{k+1}$ (through the
override, which calls the base method again) and then hands it to the
`Solver`s of $B_k$ that are concerned, so the calls form a chain along
the path, each $B_k$ being visited at most once. If `anyone_there()`
is `false` on $B_k$, then by Lemma 4.1 no `Solver` is registered with
$B_k$ or with any of its ancestors, and the chain stops there with no
`Solver` missed; otherwise the chain proceeds. Hence the `Solver`s
visited are those registered with the $B_k$ reached, which comprise
all those registered on the path that can exist, and each of them is
handed `mod` once, if and only if the test of concern succeeds. A
`Solver` registered elsewhere is never visited. $\square$

Since `register_Solver()` refuses to register a `Solver` twice with
the same `Block`, and a `Solver` is attached to one `Block` at a time
(`set_Block()`), "once" is literal. A `:Solver` relies on Lemma 8.1
when it treats its pending list as the exact record of the changes of
its target that are of concern to it.

## 8.4 The Janus discipline {#sec-8-4}

A change of the data of a `:Block` can originate from either face. The
user may call a method of the `:Block` that changes its data, such as
`MCFBlock::chg_cost( NCost , arc , issueMod , issueAMod )`, which
changes the physical data and issues a physical `Modification` (an
`MCFBlockRngdMod`). If the abstract representation has been
constructed and `issueAMod` is not `eDryRun`, the method also changes
the corresponding coefficient of the `LinearFunction` of the
`Objective`, which issues an abstract `Modification` (a
`C05FunctionModLinRngd`); the latter has `concerns_Block() == false`,
since the `:Block` has already applied the change. Alternatively, the
user (or a `:Solver`) may change the abstract representation directly,
say by `x[ a ].is_fixed( true , eModBlck )`, which fixes the flow of
arc $a$ (at 0, the abstract way of *closing* the arc), or by changing
a `Function` that carries an abstract `Constraint` or `Objective`. The
abstract object then issues the corresponding abstract `Modification`,
with `concerns_Block() == true`, and passes it to the
`add_Modification()` of its `Block`. The override of the latter is
expected to *intercept* it, i.e., to recognize a change it knows how
to translate into its physical representation and apply it; only then
does it issue the corresponding physical `Modification`, and forward
the abstract one (with `concerns_Block()` reset to `false`) to the
listening `:Solver`s.

This is the *Janus discipline*: a `:Block` has two faces, and a change
made on either of them is propagated to the other one. Hence,
specialized and general-purpose `:Solver`s alike receive the
`Modification`s of the face they read. The scheme of the override is
fixed by the comment of `Block::add_Modification()`
([`Block.h:5752-5762`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5752-5762)):

```
SomeBlock::add_Modification(mod, chnl):
  if mod.concerns_Block():
    mod.concerns_Block(false)
    translate mod into the physical change, made by the methods of the
      :Block with issueMod = make_par(eNoBlck, chnl), issueAMod = eDryRun
  Block::add_Modification(mod, chnl)
```

With `issueMod == eNoBlck` the method performs the physical change,
and issues the physical `Modification` (with `concerns_Block() ==
false`) only if anyone is listening. With `issueAMod == eDryRun` it
does *not* touch the abstract representation again, since the change
there has already happened, and another abstract `Modification` would
count the change twice. Conversely, a change originated on the
physical face is made with `( eNoBlck , eNoBlck )`, the defaults of
the methods of the `:Block`, and the abstract face is updated as a
second action. Passing `eModBlck` there makes no difference: a
physical `Modification` is issued as under `eNoBlck`
(`Observer::issue_pmod()`), and the method downgrades the abstract one
to `eNoBlck` (`Observer::un_ModBlock()`), since the `:Block` has
already applied the change.

### The abstract Modification reaches its Block at once

This scheme relies on one property: the abstract `Modification`
reaches its `:Block` *immediately*, before it can be packed into any
`GroupModification` (cf. Lemma 8.2 below). Were it buffered, i.e.,
packed into an open `GroupModification` (cf.
[§8.6](08-modification-janus.md#sec-8-6)) and delivered only when the
group is closed, the abstract representation might have undergone
arbitrarily many further changes by the time the `:Block` saw it. The
`Modification` might, for instance, refer to a dynamic `Variable` that
has been deleted since, which the `:Block` would have to detect and
handle. We say that an abstract `Modification` is *issued in* a
`Block` $B$ if it is issued by a `Variable`, `Constraint`, `Objective`
or `Function` of the abstract representation of $B$ (not of a
sub-`Block`). Such an object passes it to the virtual
`add_Modification()` of $B$, either directly or through the
`FRowConstraint` or `FRealObjective` that observes the `Function`.

**Lemma 8.2** (Immediacy). *Let $B$ follow the scheme of the override,
and let an abstract `Modification` `mod` be issued in $B$, on any
channel. Then the override of $B$ runs on `mod`, before `mod` is
packed into any `GroupModification`, and before it reaches any
`Solver` or any ancestor of $B$. Moreover, a `GroupModification` that
reaches the override of $B$ through the tree has all its
sub-`Modification`s issued in proper descendants of $B$; in
particular, the override of a leaf `:Block` receives no
`GroupModification` other than those passed to it from the outside (as
`map_forward_Modification()` and `map_back_Modification()` of [Chapter
10](10-r3block.md#ch-10) do).*

*Proof.* By definition, `mod` is passed to the virtual
`add_Modification()` of $B$, i.e., to its override, which handles it
first and calls `Block::add_Modification()` only afterwards. The
packing into a group (the second branch of the pseudocode of
[§8.3](08-modification-janus.md#sec-8-3)) and the delivery to the
`Solver`s and to the father happen only inside the base method. Hence
the override sees `mod` before any of them. As for the second claim, a
`GroupModification` is constructed only by `open_channel()` (no other
code of the core constructs one). When its channel is closed in some
`Block` $D$, it is dispatched by the qualified call
`Block::add_Modification()` of $D$ ([`SMS++/src/Block.cpp:917`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L917)), which
does not run the override of $D$, and then calls the overrides of the
strict ancestors of $D$ only. By the pseudocode, the `Modification`s
that enter the group of a channel defined in $D$ come from `Block`s in
the sub-tree of $D$. Hence, the override of $B$ sees a group only if
$B$ is a strict ancestor of the $D$ that closed it, in which case all
the sub-`Modification`s of the group come from proper descendants of
$B$; a leaf has none. $\square$

If, furthermore, the abstract change is made by a thread that holds
the lock of $B$, as [Chapter 17](17-parallel.md#ch-17) requires of any
change of a `Block`, no other thread can change $B$ between the change
and the run of the override. The override therefore finds the data
structures of $B$ in the state in which `mod` was issued. In fact,
this is a substantial simplification. A leaf `:Block` never has to
handle a `GroupModification` issued in it, nor to reason about what
else may have changed in the meantime, since it reacts to one atomic
abstract `Modification` at a time; a non-leaf `:Block` has to handle
`GroupModification`s only as far as they refer to changes inside its
sub-`Block`s. (The override of `MCFBlock` unpacks the
`GroupModification`s it is given into their sub-`Modification`s
anyway, which is needed for those passed to it by
`map_forward_Modification()`.)

### Consistency of the two faces

What the discipline guarantees can now be stated. We call a `:Block`
$B$ *Janus-conforming* if it satisfies three conditions. (J1) Its
override of `add_Modification()` follows the scheme above. (J2) Each
of its methods that change the physical data, called with `issueAMod`
other than `eDryRun` while the corresponding part of the abstract
representation exists, applies the same change to the abstract
representation, through the methods of the latter and with the
parameter `un_ModBlock( issueAMod )`; with `issueAMod == eDryRun` it
changes only the physical data, and in both cases it issues its
physical `Modification` if and only if `issue_pmod( issueMod )` holds.
(J3) For each abstract change that the override accepts, the physical
change it makes is the one that, by (J2), would have produced that
same abstract change. We say that the two faces of $B$ are
*consistent* when the abstract representation is the one the `:Block`
would generate out of its current physical data, as Proposition 7.1
describes for `MCFBlock`.

**Proposition 8.3** (Janus consistency). *Let $B$ be a
Janus-conforming `:Block` whose two faces are consistent. Consider one
change of the data of $B$, made either (a) by a method of $B$ called
with `issueAMod` other than `eDryRun`, or (b) by a method of its
abstract representation called with `eModBlck` and accepted by the
override of $B$. Then, afterwards, the two faces of $B$ are consistent
again, each face has been changed once, and each `Solver` listening to
$B$ receives at most one physical and at most one abstract
`Modification` for the change, neither of which has `concerns_Block()
== true`.*

*Proof.* In case (a), by (J2) the method changes the physical data once and
the abstract representation once, which keeps them consistent. The abstract
change is made with `un_ModBlock( issueAMod )`, which is never `eModBlck`;
hence the abstract `Modification`, if issued at all, has
`concerns_Block() == false`, and the override of $B$ only forwards it. By
construction, the physical `Modification` has `concerns_Block() == false`. By
Lemma 8.1, each of the two `Modification`s reaches each listening `Solver` at
most once. In case (b), the abstract representation has been changed once by
the method. By Lemma 8.2 the override of $B$ sees the abstract `Modification`
immediately, with `concerns_Block() == true`; it resets it to `false` and, by
(J1), makes the physical change with `issueAMod == eDryRun`, which by (J2)
changes the physical data only, and by (J3) restores the consistency. The
physical `Modification` is issued (if anyone is listening) with
`concerns_Block() == false`, and the abstract one is then forwarded with
`concerns_Block() == false`. Again by Lemma 8.1, each of them reaches each
listening `Solver` at most once. $\square$

We remark that the hypotheses of case (b) are not idle. An abstract
change made with `eNoBlck` issues a `Modification` with
`concerns_Block() == false`, which the override of $B$ forwards
without translating it, and one made with `eNoMod` issues none: in
both cases the abstract representation changes and the physical one
does not, and the faces are no longer consistent. Hence the abstract
representation of a `:Block` is to be changed with `eModBlck` (the
default) by anyone but the `:Block`. Similarly, when the override
*refuses* a change (see below), the abstract representation has
already been changed when the exception is thrown, and the faces are
left inconsistent; the exception is meant to stop a program that
relies on a change that the `:Block` cannot represent, not to be
caught and ignored.

### Refusing an abstract change

A `:Block` may refuse an abstract change that it does not know how to
translate. `MCFBlock`, for instance, does not allow arcs to be added
or removed through the abstract representation, since this would
require changing the structure of the flow conservation
`FRowConstraint`s; nor does it allow a flow `ColVariable` to be fixed
at a value other than 0, or its type to be changed. The refusal is
explicit: the override throws `std::invalid_argument` (or
`std::logic_error`, for the forbidden changes of a flow `ColVariable`)
with a message that describes what was refused. Hence, the convention
is to fail loudly, and a `:Block` that wants to support a particular
abstract change must do so explicitly.

## 8.5 The four values of `ModParam` {#sec-8-5}

Every method that may issue a `Modification` takes one or two
parameters of type `ModParam`: `issueMod` for the physical
`Modification` and `issueAMod` for the abstract one. The default is
`eModBlck` for the methods of the abstract representation
(`ColVariable::is_fixed()`, the methods of `RowConstraint` and of
`LinearFunction`, and so on), and `eNoBlck` for the methods of the
`:Block`s that change their physical data (`MCFBlock::chg_cost()` and
so on). The enum `amododification_type` ([`Modification.h:1237-1242`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Modification.h#L1237-1242))
has four values:

| `ModParam` | Value | Meaning |
|---|---|---|
| `eDryRun` | 0 | The method does *not* make the change, and therefore issues no `Modification`; it serves to switch off one face of a method that changes both, when the change on that face has already been made by other means. |
| `eNoMod` | 1 | The method makes the change but issues no `Modification`; to be used only when it is certain that neither the `:Block` nor any `:Solver` will ever need the information (typically, while a new `:Block` is filled in, before any `:Solver` is attached). |
| `eNoBlck` | 2 | The method makes the change and issues the `Modification` only if anyone is listening (`anyone_there()`), with `concerns_Block() == false`, which tells the receiving `:Block` not to apply the change again to its physical representation. |
| `eModBlck` | 3 | The method makes the change and issues the `Modification` whether or not anyone is listening, with `concerns_Block() == true`, so that the `:Block` applies the change to its physical face if needed; it is the default of the abstract representation. |

The distinction between `eNoBlck` and `eModBlck` only matters for an
abstract `Modification`: a physical one is issued only if anyone is
listening, whichever of the two is passed (`Observer::issue_pmod()`),
and it never requires the `:Block` to react. Both parameters are
independent, and they can also carry the name of a channel in their
upper bits: `Observer::make_par( iM , chnl )` is `iM + 4 * chnl`, from
which `par2mod()` and `par2chnl()` extract the two parts. This is how
related `Modification`s are routed to a channel to be grouped (cf.
[§8.6](08-modification-janus.md#sec-8-6)); normally both parameters
are on channel 0, and the channels need not be touched.

## 8.6 Channels and `GroupModification` {#sec-8-6}

So far, a `Modification` has been dispatched the moment it is issued.
This is the default, but SMS++ also provides a mechanism to *batch* a
set of related `Modification`s and deliver them together, as a single
`GroupModification`; the mechanism is that of the *channels*
([`Observer.h:333-453`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Observer.h#L333-453), [`Block.h:5793-5832`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5793-5832)).

### Opening and closing a channel

Before a sequence of related changes, one can *open a channel* on a `:Block`:

```cpp
Observer::ChnlName c = blk.open_channel();  // open a fresh channel
//  ... make several related changes, all passing channel c ...
blk.close_channel( c );               // close it: the group is shipped
```

`open_channel()` (with the default argument 0) creates a new
`GroupModification`, gives the channel a new name and records the pair
in the `Block`, in which we say that the channel is *defined*; the
name is returned. A `Modification` is sent *on* the channel `c` by
passing `make_par( issueMod , c )` to the method that issues it, or by
setting `c` as the *default channel* of the `Block` with
`set_default_channel( c )`, after which a plain `eModBlck` or
`eNoBlck` is routed to `c`. What has been sent to a channel can also
be thrown away: `close_channel( c , force , true )` deletes what
`close_channel( c , force )` would have dispatched, and nobody
receives anything. Also, `clear_channel( c )` empties the current
group of the channel while leaving it open, which keeps a long
sequence of changes known to have no net effect from piling up in it.

### Nesting

Calling `open_channel( c )` with the name `c` of a channel that is
already open does not open an independent channel, but *nests* a new
`GroupModification` into the one currently associated with `c`, which
becomes its father; the `Modification`s sent on `c` then go into the
inner group. `close_channel( c )` closes only the innermost group, and
the channel goes back to the group above it, whose content now
comprises the closed group. Only when the outermost group (the one
created by `open_channel( 0 )`) is closed, or when a closure is
forced (`force == true`), is the nested structure dispatched;
this allows trees of related `Modification`s of any depth.
`open_channel()` also accepts a pointer to a `GroupModification` (or
to an object of a derived class), which is then adopted as the group
being opened, and which allows the caller to attach to the group
information that a `:Solver` or a `:Block` may find useful when
processing it.

This behavior is captured by the following statement, where a
*session* of a channel `c` defined in a `Block` $D$ is the sequence of
calls between `open_channel( 0 )` and the closure in root mode (or
forced) of `c`, with no `close_channel( c , ... , true )` and no
`clear_channel( c )` in between.

**Lemma 8.4** (Channels). *Let a session of a channel `c` defined in
$D$ be given, during which the `Modification`s sent on `c` are issued
in $D$ or in its descendants, and the overrides on the way call the
base method with the channel they received. Then: (i) at the end of
the session, a single `GroupModification` $G$ is passed to the
qualified `Block::add_Modification()` of $D$, and the
`GroupModification`s nested in $G$ correspond one to one to the calls
of `open_channel( c )` of the session, each being the father of those
opened while it was the current one; (ii) every `Modification` sent on
`c` during the session is a sub-`Modification` of exactly one group of
$G$, namely the one current when it reached $D$, in the order in which
it reached $D$; (iii) a `Solver` registered with a `Block` strictly
between the issuer and $D$ receives each such `Modification`
immediately and alone, while one registered with $D$ or with an
ancestor of $D$ receives it only inside $G$, once, if $G$ is of
concern to it, at the end of the session.*

*Proof.* (i) The pairs (name, current group) of the channels defined
in $D$ are kept in `v_GroupMod`. `open_channel( 0 )` pushes a new pair
with a group without father. `open_channel( c )` with `c` defined in
$D$ (or reaching $D$ from a descendant, which passes it upwards) sets
the father of the new group to the current one, appends the new group
to it, and makes it current. `close_channel( c )` without `force`, on
a current group with a father, makes the father current again
([`SMS++/src/Block.cpp:920-939`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L920-939)). The current groups therefore form a
stack whose bottom is the root group, and each group is appended to
its father once, when opened. At the closure in root mode or forced,
the code climbs from the current group to the root and passes the root
to `Block::add_Modification()` of $D$ once, after having removed the
channel ([`SMS++/src/Block.cpp:887-917`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L887-917)). (ii) By the pseudocode of
[§8.3](08-modification-janus.md#sec-8-3), a `Modification` sent on `c`
from a `Block` $B$ of the sub-tree of $D$ is forwarded upwards, since
`c` is not defined below $D$, until it reaches $D$. There it is
appended to the current group and the climb stops; this happens once,
when it reaches $D$, and appending preserves the order. (iii) At each
`Block` strictly below $D$ on the climb, the base method hands the
`Modification` to the concerned `Solver`s after forwarding it, which
gives the first claim. Since the climb stops at $D$, the `Solver`s of
$D$ and of its ancestors receive none of them during the session,
while at its end $G$ is delivered to them through $D$ as in Lemma 8.1.
$\square$

We remark two more facts. First, at the end of a session $G$ is passed
to $D$ on channel 0, which `Block::add_Modification()` replaces by the
default channel of $D$, if one is set: a `Solver` then receives $G$
only when that other channel is closed. Second, when a nested group
has `concerns_Block() == true` its closure raises that of its father,
up to the root, and the `changes()` of each group become the "or" of
those of its sub-groups, and therefore the test of concern of Lemma
8.1 sees what the group changes altogether.

### What the channels are for

Their purpose is to help a `:Solver` when several changes are
*correlated* and are best reacted to together. The prototypical
example is the change of all the coefficients of a "column" of a
model, which live in several `Constraint`s (in the `Function`s that
carry them); the change thus produces one `Modification` per
`Constraint`. A `:Solver` that receives them one at a time may redo
expensive bookkeeping once per `Modification`, while one that receives
them as a single `GroupModification` can recognize the pattern and
react once. `BundleSolver`, for instance, processes at once the
`GroupModification`s of the `Function`s it handles. This mechanism is
(arguably) not used as much as it could be, but it is available to any
`:Solver` and to any code that changes a `:Block`.

The price of batching is the property that Lemma 8.2 guarantees to the
`:Block` and Lemma 8.4 (iii) denies to the `:Solver`s of $D$ and
above. When `Modification`s are packed into a group, such a `:Solver`
sees them together, possibly long after the first was issued, and
after the objects they refer to have changed further (or, if dynamic,
have been removed). A `:Solver` that processes `GroupModification`s
therefore needs, in general, somewhat more logic than one that
processes single `Modification`s, since by the time it processes a
buffered `Modification` the `Variable` or `Constraint` it refers to
may have changed again.

## 8.7 The filter of the listeners {#sec-8-7}

A `:Block` with no `:Solver` attached to it or to any of its ancestors
is, by construction, silent: nobody is listening, nobody will react to
any `Modification`, and issuing one is wasted work. Whether anyone is
listening is answered by `anyone_there()`, which reads the flag `f_at`
of [§4.6](04-block.md#sec-4-6) (Lemma 4.1). The methods called with
`issueMod == eNoBlck` consult it before constructing the
`Modification`, and if it is `false` they construct none; so do the
methods of the physical face, whatever their `issueMod`
(`Observer::issue_pmod()`). The methods of the abstract representation
called with the default `eModBlck`, instead, issue the `Modification`
anyway, because its `concerns_Block() == true` has to reach the
`:Block` even when no `:Solver` is listening (Proposition 8.3, case
(b)). If nobody is listening, the base `Block::add_Modification()`
then drops it, after the override has seen it. For this reason a
concrete `:Block` need not override `anyone_there()` to return `true`
whenever its abstract representation exists: the abstract
`Modification`s issued with `eModBlck` reach it regardless of the
registered `:Solver`s. `MCFBlock` and `BinaryKnapsackBlock` used to
carry such an override, which is now commented out in their headers
with this explanation.

### What a listener reads

Actually, the filter is finer than "is anyone there". The method
`Modification::changes()` gives the kind of a change as a `ModConcern`
bit mask. Its bits record whether the change is physical (`eModPhys`)
and, otherwise, whether it affects the data (`eModVarData`), the
integrality (`eModVarType`) or the set (`eModVarSet`) of the
`Variable`s, the sides (`eModCnsSide`), the coefficients
(`eModCnsCoef`) or the set (`eModCnsSet`) of the `Constraint`s, or the
`Objective` (`eModObj`). Further bits give the possible effect
(`eRegnShrink` and `eRegnGrow` for the feasible region, `eObjUp` and
`eObjDown` for the objective). Symmetrically, a `:Solver` declares the
kinds it reads by `Solver::concerned_by()` (all of them in the base
class, only the abstract ones for a `:MILPSolver`), a `:Block`
declares those it reads by `Block::concerned_by()`, and
`Block::concerned()` is the "or" of what is read by anyone listening
to a `:Block`. A `:Block` passes each `:Solver` only the
`Modification`s that are of concern to it (Lemma 8.1). A physical
`Modification` reaches only those who read `eModPhys`, whatever its
other bits, unless it changes everything (`eModAll`, as an
`NBModification` does); an abstract one reaches those who read one of
its kinds. Before building a `Modification` at all, a method can call
`anyone_there_for( what )`, or the variants of `issue_mod()` and
`issue_pmod()` that take the kind; a change that nobody reads then
costs (almost) no work. With `eModBlck`, the variant of `issue_mod()`
still issues, since the `:Block` has to see the change. For what an
author of a `:Modification` has to declare, see
[§C.2](C-writing-modification.md#sec-C-2).

## 8.8 Example: a change of cost in `MCFBlock`, seen from both faces {#sec-8-8}

The example below changes a cost of an `MCFBlock` through the physical
face, then closes an arc through the abstract face. We observe the
resulting `Modification`s through a `FakeSolver`, a `:Solver` that
only stores the `Modification`s it receives, and whose
`concerned_by()` is that of the base class (all the kinds).

```cpp
#include "MCFBlock.h"
#include "FakeSolver.h"
#include <iostream>

using namespace SMSpp_di_unipi_it;

int main()
{
 // construct and load an MCFBlock (as in §4.5)
 MCFBlock mcf;
 mcf.load( 3 , 3 , MCFBlock::Subset{ 2 , 3 , 3 } ,       // pEn (nodes 1 .. n)
           MCFBlock::Subset{ 1 , 2 , 1 } ,             // pSn
           MCFBlock::Vec_FNumber{ 3.0 , 3.0 , 3.0 } ,  // pU
           MCFBlock::Vec_CNumber{ 2.0 , 3.0 , 4.0 } ,  // pC
           MCFBlock::Vec_FNumber{ -2.0 , 0.0 , +2.0 } );

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
 //   - one physical MCFBlockRngdMod of type eChgCost with
 //     rng = [0, 1),
 //   - one abstract C05FunctionModLinRngd (with concerns_Block() ==
 //     false) reflecting the change in the LinearFunction of the
 //     Objective.

 // ---- abstract-side change: close arc 1 by fixing its flow to 0 ----
 // The idiomatic abstract-face counterpart of MCFBlock::close_arc(1)
 // is fixing the corresponding ColVariable to 0. This is a pure
 // abstract change; the framework intercepts it and applies the
 // matching physical operation.
 ColVariable * x1 = mcf.i2p_x( /* arc index = */ 1 );
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
 for( auto & mod : tap->get_Modification_list() )
  std::cout << *mod << '\n';

 mcf.unregister_Solver( tap , /* deleteold = */ true );
 return( 0 );
 }
```

The physical change issues an `MCFBlockRngdMod` of type `eChgCost` on
the range $[0, 1)$, and, since the `Objective` has been generated,
`chg_cost()` also changes the coefficient of arc 0 in the
`LinearFunction` of the `Objective`, which issues a
`C05FunctionModLinRngd` with `concerns_Block() == false` (the
`eModBlck` passed is downgraded by `un_ModBlock()`). The abstract
change issues a `VariableMod` with `concerns_Block() == true`, which
the override of `MCFBlock::add_Modification()` intercepts. This
override checks that the `ColVariable` is fixed at 0 (any other value
would be refused) and calls `close_arc( 1 , eNoBlck , eDryRun )`,
which issues a physical `MCFBlockRngdMod` marking the arc as closed;
then it forwards the `VariableMod` with `concerns_Block()` reset to
`false`. Hence each change produces two `Modification`s, one per face,
as in Proposition 8.3; a listener of the physical face only (an
`MCFSolver`) ignores the abstract ones, and one of the abstract face
only (a `:MILPSolver`) ignores the physical ones. Seen from the user,
the discipline is invisible: whether the change came through
`chg_cost()` or through `ColVariable::is_fixed()`, both faces end up
consistent and both streams carry it. Besides serving as a tap, as
here, `FakeSolver` is useful to code that has to *defer* the reaction
to the changes (say, while a sub-`Block` is changed in bulk, the
changes being replayed later).

## 8.9 Idioms {#sec-8-9}

The default value of `issueMod` and `issueAMod` of the methods of the
abstract representation is `eModBlck` because it is the safe one
(Proposition 8.3); any other value is an optimization, to be used
only when it is clear what the listeners need.

`eNoMod` is to be used only while no listener exists, typically while
a `:Block` is constructed and filled in, before any `:Solver` is
registered: then the `Modification`s are useless anyway, while after
a `:Solver` has been attached a change it does not learn about makes
its results stale.

`eDryRun` switches off one face in a coordinated change, and its
intended use is inside the override of `add_Modification()` of the
`:Block`. When an abstract `Modification` has changed the abstract
face, the `:Block` updates the physical one by calling its physical
method with `issueAMod == eDryRun`, i.e., "make the physical change,
but not the abstract one again".

When one logical operation produces several `Modification`s, the idiom
is to group them by a channel, with which a `:Solver` can react to the
group at once, which is often more efficient than reacting to each in
turn.

## 8.10 Forward reference to `Change` {#sec-8-10}

Arguably, the present design of `Modification` has one notable
limitation: a `Modification` carries enough information to *describe*
a change, but not always enough to *replay* it on a different
`:Block`, and in general it cannot be serialized in a form that
crosses the boundary of a process. The concept of
[`Change`](https://smspp.gitlab.io/smspp-project/db/d4d/class_s_m_spp__di__unipi__it_1_1_change.html)
(cf. [Chapter 16](16-change.md#ch-16)) addresses this. A `Change`
carries the data needed to apply the change to any copy of a `:Block`,
and it can be serialized to netCDF and deserialized as a plain object;
where the `:Block` supports it, it can also be *undone* through an
inverse `Change`, generated automatically. Its intended use is
distributed computation by message passing; only a few `:Change`
classes exist at version 0.8.0 (`AbstractChange` and `GroupChange` in
the core, the former for the abstract representation of any `Block`,
and those of `BinaryKnapsackBlock` and `SATBlock`), and the interface
may change. **Status — beta.**
