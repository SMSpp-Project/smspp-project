# 8. Modification and the Janus discipline {#ch-8}

[Source: [`SMS++/include/Modification.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Modification.h), [`Observer.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Observer.h), [`Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h),
[`SMSTypedefs.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSTypedefs.h), [`SMS++/src/Block.cpp`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp), [`MCFBlock/include/MCFBlock.h`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h)]

We are concerned in this chapter with the way in which the changes of a
`Block` reach the `:Solver`s attached to it, since both the physical and the
abstract representation of a `Block` can change after a `:Solver` has been
attached. [Chapter 6](06-solver.md#ch-6) introduced the list of pending
`Modification`s through which the changes arrive; here we make the machinery
precise and prove the properties of the delivery on which `:Block`s and
`:Solver`s rely, and we state the discipline by which a `:Block` keeps its two
faces in step (the *Janus discipline*), together with the parameters of type
`ModParam` by which the user tunes it.

After the concept ([§8.1](08-modification-janus.md#sec-8-1)) and the
hierarchy of the classes ([§8.2](08-modification-janus.md#sec-8-2)), we
describe the two streams of `Modification`s, their delivery and the way in
which a receiver recognizes their type
([§8.3](08-modification-janus.md#sec-8-3)), and then the Janus discipline
with its consistency property ([§8.4](08-modification-janus.md#sec-8-4)).
The values of `ModParam` ([§8.5](08-modification-janus.md#sec-8-5)), the
channels that batch the `Modification`s into groups
([§8.6](08-modification-janus.md#sec-8-6)) and the filter by which a change
reaches only those who read it ([§8.7](08-modification-janus.md#sec-8-7))
follow; [§8.8](08-modification-janus.md#sec-8-8) gives an example,
[§8.9](08-modification-janus.md#sec-8-9) the idioms, and
[§8.10](08-modification-janus.md#sec-8-10) points to `Change`.

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
`Solver::add_Modification( sp_Mod & )`, which appends it to the list of the
`:Solver`. The first is called by the `:Block`, or by one of its `Variable`s,
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
  the group at once. Every `GroupModification` enters the system
  through the channels of [§8.6](08-modification-janus.md#sec-8-6), i.e.,
  through `open_channel()`, which constructs one or adopts the one it is
  given.
- `VariableGroupMod` (line 1107) is a `GroupModification` that groups
  the `Modification`s of several `Variable`s.

Each concrete `:Block` typically defines `:Modification`s for the
changes that its physical representation supports. For `MCFBlock` they
are `MCFBlockMod` ([`MCFBlock.h:2423`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L2423)), the base of all its physical
changes, and its two derived classes. `MCFBlockRngdMod`
([`MCFBlock.h:2528`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L2528)) is a change of a contiguous range of arcs or nodes
(new costs, capacities or deficits, or arcs opened, closed, added or
removed), and `MCFBlockSbstMod` ([`MCFBlock.h:2579`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L2579)) is the same change
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
([`MCFBlock.h:1768-1821`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L1768-1821)).

The *abstract* stream consists of the `Modification`s issued by the
abstract representation of the `:Block` when something in it changes:
a `Variable` is fixed, a `Constraint` is relaxed, a coefficient of a
`Function` is changed, a dynamic `Constraint` is added or removed.
They derive from `AModification`, and their `concerns_Block()` is
`true` unless the issuer has been instructed otherwise (cf.
[§8.5](08-modification-janus.md#sec-8-5)); with the value `true` the
receiving `:Block` is expected to react by changing its physical
representation accordingly (cf.
[§8.4](08-modification-janus.md#sec-8-4)).

A general-purpose `:Solver`, such as a `:MILPSolver`, processes the abstract
stream and has no use for the physical one (it would not know what to do with
an `MCFBlockRngdMod`); since it declares that it reads only the abstract
kinds, the physical `Modification`s do not even reach it ([Lemma
8.1](08-modification-janus.md#lem-8-1)). Conversely, a specialized `:Solver`
typically processes the physical stream, and it must ignore the abstract
`Modification`s that come from a change that has produced both (reacting to
the same change twice would be wrong), which it can do by declaring through
`concerned_by()` that it reads `eModPhys` only (as the
`:BinaryKnapsackSolver`s do), or by skipping them in its code (as `MCFSolver`
does).

### The delivery

Routing is done by the base method `Block::add_Modification( mod , chnl )`
([`SMS++/src/Block.cpp:777-823`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L777-823)), which the overrides of the `:Block`s call at
their end (the channels are discussed in
[§8.6](08-modification-janus.md#sec-8-6)).

<span id="alg-8-1"></span>**Algorithm 8.1** (delivery of a `Modification`).

```text
 1  procedure ADD-MODIFICATION(B, mod, chnl)
 2     if nobody listens to B then return            ▷ anyone_there()
 3     if chnl = 0 then chnl ← the default channel of B
 4     if chnl ≠ 0 then
 5        if chnl is a channel defined in B then
 6           append mod to the current group of chnl;  return
 7        if B has no father then throw invalid_argument
 8     if B has a father F then
 9        call the add_Modification() of F on (mod, chnl)   ▷ a virtual call
10     for each Solver S registered with B do
11        if is-of-concern(changes of mod, kinds read by S) then
12           append mod to the pending list of S
```

Here `is_of_concern( c , reads )` is `true` if `reads` comprises all
the kinds (`eModAnything`) or if `c` changes everything (`eModAll`).
Otherwise, it is `true` if `c` is physical and `reads` comprises
`eModPhys`, or if `c` is abstract and has a kind in common with
`reads`; a `GroupModification` is of concern if any of its
sub-`Modification`s is (`is_of_concern_to()`).

<span id="lem-8-1"></span>**Lemma 8.1** (Delivery). *Let `mod` be passed to
`Block::add_Modification( mod , 0 )` of a `Block` $B$ with no default
channel set in $B$ or in its ancestors, and let the overrides of
`add_Modification()` of the `:Block`s on the path from $B$ to the root
call the base method with the same `mod` and channel 0. Then a
`Solver` $S$ receives `mod` (once) if and only if $S$ is registered
with $B$ or with one of its ancestors and `mod` is of concern to
`S.concerned_by()`; no other `Solver` receives it.*

*Proof.* Let $B = B_0, B_1, \dots, B_h$ be the path from $B$ to the root. A
call on $B_k$ forwards `mod` to $B_{k+1}$ (through the override, which calls
the base method again) and then hands it to the `Solver`s of $B_k$ that are
concerned, so the calls form a chain along the path, each $B_k$ being visited
at most once. If `anyone_there()` is `false` on $B_k$, then by [Lemma
4.1](04-block.md#lem-4-1) no `Solver` is registered with $B_k$ or with any of
its ancestors, and the chain stops there with no `Solver` missed; otherwise
the chain proceeds. Hence the `Solver`s visited are those registered with the
$B_k$ reached, which comprise all those registered on the path that can exist,
and each of them is handed `mod` once, if and only if the test of concern
succeeds. A `Solver` registered elsewhere is never visited. $\square$

Since `register_Solver()` refuses to register a `Solver` twice with the same
`Block`, and a `Solver` is attached to one `Block` at a time (`set_Block()`),
"once" is literal. A `Solver` that has called `inhibit_Modification( true )`
is still handed `mod`, but discards it. A
`:Solver` relies on [Lemma
8.1](08-modification-janus.md#lem-8-1) when it treats its pending list as the
exact record of the changes of its target that are of concern to it.

### Recognizing the type of a Modification

Whoever processes a `Modification` (a `:Solver` going through its pending
list, the override of `add_Modification()` of a `:Block`, a `Solution` that
has to `adapt()` to it) has first to recognize its type, since what it needs
to know is in the derived class. The natural tool is a `dynamic_cast`, tried
against each of the few types that the receiver can handle. This is not free:
a `:Solver` dispatches the same few types a great many times, and when the
classes come from different shared libraries a `dynamic_cast` may have to
compare the names of the types instead of their addresses. For this reason the
core provides (since SMS++ 0.9.0) the two functions

```cpp
template< class P , class B >
P memo_cast( B * p );

template< class T , class B >
std::shared_ptr< T > memo_pointer_cast( const std::shared_ptr< B > & p );
```

which give the same result as `dynamic_cast< P >( p )` and
`std::dynamic_pointer_cast< T >( p )`, respectively, but make the real cast
only once for each dynamic type of `* p` that they meet.

<span id="alg-8-2"></span>**Algorithm 8.2** (cast remembered per dynamic
type).

```text
 1  function MEMO-CAST⟨T, B⟩(p)                   ▷ p points to a B
 2     if p = nil then return nil
 3     C ← the table of the pair (T, B) of the running thread
 4     k ← the type_info of the dynamic type of *p
 5     if k ∉ C then
 6        t ← dynamic_cast⟨T*⟩(p)
 7        if t ≠ nil then C[k] ← address(t) − address(p)
 8        else C[k] ← FAIL
 9     if C[k] = FAIL then return nil
10     return the T* at address(p) + C[k]
```

It is implemented by `memo_cast()`
([`SMSTypedefs.h:111-147`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSTypedefs.h#L111-147)), while `memo_pointer_cast()` applies it
to the stored pointer and returns a `std::shared_ptr` that shares the
ownership of `p`. The table is `thread_local`, and therefore it needs no
lock; its keys are the addresses of the `std::type_info` objects, and since a
type may have a distinct one in each shared library, at worst it gets one
entry per library, each with a correct value. The correctness of the
remembered offset rests on the layout of the classes, as follows.

<span id="lem-8-2"></span>**Lemma 8.2** (Remembered casts). *Let a thread
make calls `memo_cast< T * >( p )` with the same pair (`T`, `B`), and let
the dynamic type $D$ of `* p` at each of them be such that (i) no class in
the hierarchy of $D$ has a virtual base, and (ii) $D$ has a single
subobject of type `B` (`B` itself if $D$ is `B`). Then each of the calls
returns `dynamic_cast< T * >( p )`.*

*Proof.* If `p` is null both return null (line 2). Otherwise the result of
`dynamic_cast< T * >( p )` depends only on $D$, on `T` and on which `B`
subobject of the object of type $D$ the pointer `p` points to: the cast either
fails or returns a subobject of that object. By (ii) the `B` subobject is the
only one of its type. By (i) its position in the object of type $D$, as well
as that of the subobject returned by the cast, is fixed by the layout of the
class $D$, and it is the same for every object of type $D$, whether the object
is complete or (while it is being constructed, when `typeid` gives $D$) a part
of a larger one. Hence, for all the calls with the same key $k$, either the
cast fails or its result lies at the same distance $\delta$ from `p`. The
first call with key $k$ makes the cast and stores FAIL or $\delta$ (lines
6-8), and this call and every later one with key $k$ return null or `p` $+\,
\delta$ (lines 9-10), i.e., the result of `dynamic_cast`. Since the table
belongs to the thread, no other thread writes into it in between. $\square$

Both hypotheses are needed. Hypothesis (i) is needed because the position of
a virtual base inside an object of class $C$ depends on the class of the
complete object of which the $C$ is a part, while during the construction of
that object `typeid` gives $C$ in both cases. Hypothesis (ii) is needed
because with two subobjects of type `B` (say, $D$ derives from $B_1$ and
$B_2$, both of which derive non-virtually from `B`) two pointers to objects of
type $D$ may point to different `B` subobjects, at different distances from
the result, under the same key. In the hierarchies on which the modules of
SMS++ use it (those of `Modification`, `Variable`, `Function`, `Constraint`,
`Objective` and `Block`) no class has a virtual base and, as far as their
headers show, none derives twice from the same class. The virtual bases are in
hierarchies of `Solver` (e.g., `BinaryKnapsackSolver` derives virtually from
`Solver`, and `RelaxationSolver` from `ChangeSolver`), on which `memo_cast()`
is not used.

The core uses it almost everywhere it recognizes a type in this way, starting
from `Solver::add_Modification()`, which tests for an `NBModification` by
`memo_pointer_cast()` to empty the pending list
([`Solver.h:2281-2293`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solver.h#L2281-2293)).
A new `:Solver`, `:Block` or `:Solution` can do the same wherever it would
write `dynamic_cast` or `std::dynamic_pointer_cast` on a `Modification`
([Appendix C](C-writing-modification.md#app-C)).

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
([`Block.h:5795-5806`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5795-5806)).

<span id="alg-8-3"></span>**Algorithm 8.3** (the override of
`add_Modification()` in a `:Block`).

```text
 1  procedure ADD-MODIFICATION of a :Block(B, mod, chnl)
 2     if mod concerns the Block then
 3        mark mod as not concerning the Block      ▷ concerns_Block(false)
 4        make the physical change that mod describes, by the methods of B
 5           with issueMod = make_par(eNoBlck, chnl) and issueAMod = eDryRun
 6     ADD-MODIFICATION(B, mod, chnl)                ▷ Algorithm 8.1
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

This scheme relies on one property: the abstract `Modification` reaches its
`:Block` *immediately*, before it can be packed into any `GroupModification`
(cf. [Lemma 8.3](08-modification-janus.md#lem-8-3) below). Were it buffered,
i.e., packed into an open `GroupModification` (cf.
[§8.6](08-modification-janus.md#sec-8-6)) and delivered only when the group is
closed, the abstract representation might have undergone arbitrarily many
further changes by the time the `:Block` saw it. The `Modification` might, for
instance, refer to a dynamic `Variable` that has been deleted since, which the
`:Block` would have to detect and handle. We say that an abstract
`Modification` is *issued in* a `Block` $B$ if it is issued by a `Variable`,
`Constraint`, `Objective` or `Function` of the abstract representation of $B$
(not of a sub-`Block`). Such an object passes it to the virtual
`add_Modification()` of $B$, either directly or through the `FRowConstraint`
or `FRealObjective` that observes the `Function`.

<span id="lem-8-3"></span>**Lemma 8.3** (Immediacy). *Let $B$ follow the
scheme of the override, and let an abstract `Modification` `mod` be issued in
$B$, on any channel. Then the override of $B$ runs on `mod`, before `mod` is
packed into any `GroupModification`, and before it reaches any `Solver` or any
ancestor of $B$. Moreover, a `GroupModification` that reaches the override of
$B$ through the tree has all its sub-`Modification`s issued in proper
descendants of $B$; in particular, the override of a leaf `:Block` receives no
`GroupModification` other than those passed to it from the outside (say, by a
caller that forwards a group it received).*

*Proof.* By definition, `mod` is passed to the virtual
`add_Modification()` of $B$, i.e., to its override, which handles it
first and calls `Block::add_Modification()` only afterwards. The
packing into a group (line 6 of
[Algorithm 8.1](08-modification-janus.md#alg-8-1)) and the delivery to the
`Solver`s and to the father happen only inside the base method. Hence
the override sees `mod` before any of them. As for the second claim, every
`GroupModification` enters the system through `open_channel()`, which
constructs one or adopts the one it is given (as `PolyhedralFunctionBlock`
does with a `VariableGroupMod`). When its channel is closed in some
`Block` $D$, it is dispatched by the qualified call
`Block::add_Modification()` of $D$ ([`SMS++/src/Block.cpp:926`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L926)), which
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
structures of $B$ in the state in which `mod` was issued, which simplifies the
overrides. A leaf `:Block` never has to handle a `GroupModification` issued in
it, nor to reason about what else may have changed in the meantime, since it
reacts to one atomic abstract `Modification` at a time; a non-leaf `:Block` has
to handle `GroupModification`s only as far as they refer to changes inside its
sub-`Block`s. (The override of `add_Modification()` of `MCFBlock` handles
no `GroupModification`; `MCFBlock::map_forward_Modification()` of [Chapter
10](10-r3block.md#ch-10) unpacks those it is given, and replays each
sub-`Modification` on the copy by the methods of the latter.)

### Consistency of the two faces

We can now state what the discipline guarantees. We call a `:Block` $B$
*Janus-conforming* if it satisfies three conditions. (J1) Its override of
`add_Modification()` follows the scheme above. (J2) Each of its methods that
change the physical data, called with `issueAMod` other than `eDryRun` while
the corresponding part of the abstract representation exists, applies the same
change to the abstract representation, through the methods of the latter and
with the parameter `un_ModBlock( issueAMod )`; with `issueAMod == eDryRun` it
changes only the physical data, and in both cases it issues its physical
`Modification` if and only if `issue_pmod( issueMod )` holds. (J3) For each
abstract change that the override accepts, the physical change it makes is the
one that, by (J2), would have produced that same abstract change. We say that
the two faces of $B$ are *consistent* when the abstract representation is the
one the `:Block` would generate out of its current physical data, as
[Proposition 7.1](07-physical-abstract.md#prop-7-1) describes for `MCFBlock`.

<span id="prop-8-1"></span>**Proposition 8.1** (Janus consistency). *Let $B$
be a Janus-conforming `:Block` whose two faces are consistent. Consider one
change of the data of $B$, made either (a) by a method of $B$ called with
`issueAMod` other than `eDryRun`, or (b) by a method of its abstract
representation called with `eModBlck` and accepted by the override of $B$.
Then, afterwards, the two faces of $B$ are consistent again, each face has
been changed once, and each `Solver` listening to $B$ receives at most one
physical and at most one abstract `Modification` for the change, neither of
which has `concerns_Block() == true`.*

*Proof.* In case (a), by (J2) the method changes the physical data once and
the abstract representation once, which keeps them consistent. The abstract
change is made with `un_ModBlock( issueAMod )`, which is never `eModBlck`;
hence the abstract `Modification`, if issued at all, has `concerns_Block() ==
false`, and the override of $B$ only forwards it. By construction, the
physical `Modification` has `concerns_Block() == false`. By [Lemma
8.1](08-modification-janus.md#lem-8-1), each of the two `Modification`s
reaches each listening `Solver` at most once. In case (b), the abstract
representation has been changed once by the method. By [Lemma
8.3](08-modification-janus.md#lem-8-3) the override of $B$ sees the abstract
`Modification` immediately, with `concerns_Block() == true`; it resets it to
`false` and, by (J1), makes the physical change with `issueAMod == eDryRun`,
which by (J2) changes the physical data only, and by (J3) restores the
consistency. The physical `Modification` is issued (if anyone is listening)
with `concerns_Block() == false`, and the abstract one is then forwarded with
`concerns_Block() == false`. Again by [Lemma
8.1](08-modification-janus.md#lem-8-1), each of them reaches each listening
`Solver` at most once. $\square$

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
translate. `MCFBlock`, for instance, does not allow arcs to be added or
removed through the abstract representation, since this would require changing
the structure of the flow conservation `FRowConstraint`s; nor does it allow a
flow `ColVariable` to be fixed at a value other than 0, or its type to be
changed. The refusal is explicit: the override throws `std::invalid_argument`
(or `std::logic_error`, for a change of the type of a flow `ColVariable` or
the fixing of a deleted arc) with a message that describes what was refused.
The convention is therefore to fail loudly: a `:Block` that wants to support
a particular abstract change must do so explicitly.

## 8.5 The four values of `ModParam` {#sec-8-5}

Every method that may issue a `Modification` takes one or two
parameters of type `ModParam`: `issueMod` for the physical
`Modification` and `issueAMod` for the abstract one. The default is
`eModBlck` for the methods of the abstract representation
(`ColVariable::is_fixed()`, the methods of `RowConstraint` and of
`LinearFunction`, and so on), and `eNoBlck` for the methods of the
`:Block`s that change their physical data (`MCFBlock::chg_cost()` and
so on). The enum `amododification_type` ([`Modification.h:1237-1242`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Modification.h#L1237-1242))
has four values, listed in
[Table 8.1](08-modification-janus.md#tab-8-1).

<span id="tab-8-1"></span>**Table 8.1.** The values of `ModParam`.

| `ModParam` | Value | Meaning |
|---|---|---|
| `eDryRun` | 0 | The method does *not* make the change, and therefore issues no `Modification`; it serves to switch off one face of a method that changes both, when the change on that face has already been made by other means. For the `issueMod` of such a method the meaning is the one the `:Block` documents (for `MCFBlock`, no physical `Modification` is issued, while the data still follow the abstract change, see [`MCFBlock.h:1754-1765`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L1754-1765)). |
| `eNoMod` | 1 | The method makes the change but issues no `Modification`; to be used only when it is certain that neither the `:Block` nor any `:Solver` will ever need the information (typically, while a new `:Block` is filled in, before any `:Solver` is attached). |
| `eNoBlck` | 2 | The method makes the change and issues the `Modification` only if anyone is listening (`anyone_there()`), with `concerns_Block() == false`, by which the receiving `:Block` knows not to apply the change again to its physical representation. |
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
([`Observer.h:333-453`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Observer.h#L333-453), [`Block.h:5837-5876`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5837-5876)).

### Opening and closing a channel

Before a sequence of related changes, one can *open a channel* on a `:Block`:

```cpp
Observer::ChnlName c = blk.open_channel();  // open a fresh channel
//  ... make several related changes, all passing channel c ...
blk.close_channel( c );               // close it: the group is shipped
```

`open_channel()` (with the default argument 0) creates a new
`GroupModification`, gives the channel a new name and records the pair in the
`Block`, in which we say that the channel is *defined*; the name is returned.
A `Modification` is sent *on* the channel `c` by passing `make_par( issueMod ,
c )` to the method that issues it, or by setting `c` as the *default channel*
of the `Block` with `set_default_channel( c )`, after which a plain `eModBlck`
or `eNoBlck` is routed to `c`. The content of a channel can also be thrown
away: `close_channel( c , force , true )` deletes what `close_channel( c ,
force )` would have dispatched, and nobody receives anything.
`clear_channel( c )`, instead, empties the current group of the channel and
leaves it open, which keeps a long sequence of changes known to have no net
effect from piling up in it.

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

<span id="lem-8-4"></span>**Lemma 8.4** (Channels). *Let a session of a
channel `c` defined in $D$ be given, during which the `Modification`s sent on
`c` are issued in $D$ or in its descendants, and the overrides on the way call
the base method with the channel they received. Then: (i) at the end of the
session, a single `GroupModification` $G$ is passed to the qualified
`Block::add_Modification()` of $D$, and the `GroupModification`s nested in $G$
correspond one to one to the calls of `open_channel( c )` of the session, each
being the father of those opened while it was the current one; (ii) every
`Modification` sent on `c` during the session is a sub-`Modification` of
exactly one group of $G$, namely the one current when it reached $D$, in the
order in which it reached $D$; (iii) a `Solver` registered with the issuer or
with a `Block` between it and $D$ ($D$ excluded) receives each such
`Modification` immediately and alone, while one registered with $D$ or with an
ancestor of $D$ receives it only inside $G$, once, if $G$ is of concern to it,
at the end of the session.*

*Proof.* (i) The pairs (name, current group) of the channels defined in $D$
are kept in `v_GroupMod`. `open_channel( 0 )` pushes a new pair with a group
without father. `open_channel( c )` with `c` defined in $D$ (or reaching $D$
from a descendant, which passes it upwards) sets the father of the new group
to the current one, appends the new group to it, and makes it current
([`SMS++/src/Block.cpp:845-865`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L845-865)).
`close_channel( c )` without `force`, on a current group with a father, makes
the father current again
([`SMS++/src/Block.cpp:928-948`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L928-948)).
The current groups therefore form a stack whose bottom is the root group, and
each group is appended to its father once, when opened. At the closure in root
mode or forced, the code climbs from the current group to the root and passes
the root to `Block::add_Modification()` of $D$ once, after having removed the
channel
([`SMS++/src/Block.cpp:896-926`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L896-926)).
(ii) By [Algorithm 8.1](08-modification-janus.md#alg-8-1), a `Modification`
sent on `c` from a `Block` $B$ of the sub-tree of $D$ is forwarded upwards,
since `c` is not defined below $D$, until it reaches $D$. There it is appended
to the current group and the climb stops; this happens once, when it reaches
$D$, and appending preserves the order. (iii) At each `Block` strictly below
$D$ on the climb, the base method hands the `Modification` to the concerned
`Solver`s after forwarding it, which gives the first claim (the issuer
included, whose base method also hands it to its own `Solver`s). Since the
climb stops at $D$, the `Solver`s of $D$ and of its ancestors receive none of
them during the session, while at its end $G$ is delivered to them through $D$
as in [Lemma 8.1](08-modification-janus.md#lem-8-1). $\square$

We remark two more facts. First, at the end of a session $G$ is passed to $D$
on channel 0, which `Block::add_Modification()` replaces by the default
channel of $D$, if one is set: a `Solver` then receives $G$ only when that
other channel is closed. Second, when a nested group has `concerns_Block() ==
true` its closure raises that of its father, up to the root, and the
`changes()` of each group become the "or" of those of its sub-groups, and
therefore the test of concern of [Lemma 8.1](08-modification-janus.md#lem-8-1)
sees what the group changes altogether.

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

The price of batching is the property that [Lemma
8.3](08-modification-janus.md#lem-8-3) guarantees to the `:Block` and [Lemma
8.4](08-modification-janus.md#lem-8-4) (iii) denies to the `:Solver`s of $D$
and above. When `Modification`s are packed into a group, such a `:Solver` sees
them together, possibly long after the first was issued, and after the objects
they refer to have changed further (or, if dynamic, have been removed). A
`:Solver` that processes `GroupModification`s therefore needs, in general,
somewhat more logic than one that processes single `Modification`s, since by
the time it processes a buffered `Modification` the `Variable` or `Constraint`
it refers to may have changed again.

## 8.7 The filter of the listeners {#sec-8-7}

A `:Block` with no `:Solver` attached to it or to any of its ancestors is, by
construction, silent: nobody is listening, nobody will react to any
`Modification`, and issuing one is wasted work. Whether anyone is listening is
answered by `anyone_there()`, which reads the flag `f_at` of
[§4.6](04-block.md#sec-4-6) ([Lemma 4.1](04-block.md#lem-4-1)) and the list of
the `Solver`s of the `Block`. The methods called with `issueMod == eNoBlck`
consult it before constructing the `Modification`, and if it is `false` they
construct none; so do the methods of the physical face, whatever their
`issueMod` (`Observer::issue_pmod()`). The methods of the abstract
representation called with the default `eModBlck`, instead, issue the
`Modification` anyway, because its `concerns_Block() == true` has to reach the
`:Block` even when no `:Solver` is listening ([Proposition
8.1](08-modification-janus.md#prop-8-1), case (b)). If nobody is listening,
the base `Block::add_Modification()` then drops it, after the override has
seen it. For this reason a concrete `:Block` need not override
`anyone_there()` to return `true` whenever its abstract representation exists:
the abstract `Modification`s issued with `eModBlck` reach it regardless of the
registered `:Solver`s. `MCFBlock` and `BinaryKnapsackBlock` used to carry such
an override, which is now commented out in their headers with this
explanation.

### What a listener reads

Actually, the filter is finer than "is anyone there". The method
`Modification::changes()` gives the kind of a change as a `ModConcern` bit
mask. Its bits record whether the change is physical (`eModPhys`) and whether
it affects the data (`eModVarData`), the integrality (`eModVarType`) or the
set (`eModVarSet`) of the `Variable`s, the sides (`eModCnsSide`), the
coefficients (`eModCnsCoef`) or the set (`eModCnsSet`) of the `Constraint`s,
or the `Objective` (`eModObj`). Further bits give the possible effect
(`eRegnShrink` and `eRegnGrow` for the feasible region, `eObjUp` and
`eObjDown` for the objective). A physical `Modification` may also set the bits
of the abstract kinds, to describe what it changes of the model (e.g.,
`ePhysSides` for the capacities of an `MCFBlock`), and it is physical all the
same; the bits are to be read through the static methods of `Modification`
(`is_physical()`, `may_shrink_region()` and the like), not one by one.
Symmetrically, a `:Solver` declares the kinds it reads by
`Solver::concerned_by()` (all of them in the base class, only the abstract
ones for a `:MILPSolver`), a `:Block` declares those it reads by
`Block::concerned_by()`, and `Block::concerned()` is the "or" of what is read
by anyone listening to a `:Block`. A `:Block` passes each `:Solver` only the
`Modification`s that are of concern to it ([Lemma
8.1](08-modification-janus.md#lem-8-1)). A physical `Modification` reaches
only those who read `eModPhys`, whatever its other bits, unless it changes
everything (`eModAll`, as an `NBModification` does); an abstract one reaches
those who read one of its kinds. Before building a `Modification` at all, a
method can call `anyone_there_for( what )`, or the variants of `issue_mod()`
and `issue_pmod()` that take the kind; a change that nobody reads then costs
(almost) no work. With `eModBlck`, the variant of `issue_mod()` still issues,
since the `:Block` has to see the change. For what an author of a
`:Modification` has to declare, see [§C.2](C-writing-modification.md#sec-C-2).

## 8.8 Example: a change of cost in `MCFBlock`, seen from both faces {#sec-8-8}

The example below changes a cost of an `MCFBlock` through the physical
face, then closes an arc through the abstract face. We observe the
resulting `Modification`s through a `FakeSolver`, a `:Solver` that
only stores the `Modification`s it receives, and whose
`concerned_by()` is that of the base class (all the kinds).

```cpp
// mcf is an MCFBlock loaded as in §4.5, with at least two arcs; FakeSolver
// accumulates the Modification it receives and does nothing with them
auto * tap = new FakeSolver();
mcf.register_Solver( tap );

// build the abstract representation, so that the abstract stream is seen
mcf.generate_abstract_variables();
mcf.generate_abstract_constraints();
mcf.generate_objective();

// physical face: lower the cost of arc 0
mcf.chg_cost( 1.0 , 0 , eModBlck , eModBlck );

// abstract face: close arc 1 by fixing its flow at 0
ColVariable * x1 = mcf.i2p_x( 1 );
x1->set_value( 0.0 );
x1->is_fixed( true , eModBlck );

for( auto & mod : tap->get_Modification_list() )
 std::cout << *mod << '\n';

mcf.unregister_Solver( tap , true );  // unregister and delete the tap
```

The physical change issues an `MCFBlockRngdMod` of type `eChgCost` on the
range $[0, 1)$, and, since the `Objective` has been generated, `chg_cost()`
also changes the coefficient of arc 0 in the `LinearFunction` of the
`Objective`, which issues a `C05FunctionModLinRngd` with `concerns_Block() ==
false` (the `eModBlck` passed is downgraded by `un_ModBlock()`). The abstract
change issues a `VariableMod` with `concerns_Block() == true`, which the
override of `MCFBlock::add_Modification()` intercepts. This override checks
that the `ColVariable` is fixed at 0 (any other value would be refused) and
calls `close_arc( 1 , eNoBlck , eDryRun )`, which issues a physical
`MCFBlockRngdMod` marking the arc as closed; then it forwards the
`VariableMod` with `concerns_Block()` reset to `false`. Hence each change
produces two `Modification`s, one per face, as in [Proposition
8.1](08-modification-janus.md#prop-8-1); a listener of the physical face only
(an `MCFSolver`, which receives the abstract ones as well, since it does not
restrict `concerned_by()`) skips them, and one of the abstract face only (a
`:MILPSolver`) does not receive the physical ones. For the user, both faces
end up consistent whichever face the change came through, and both streams
carry it. Besides serving as a tap, as here, `FakeSolver` is useful to code
that has to *defer* the reaction to the changes (say, while a sub-`Block` is
changed in bulk, the changes being replayed later).

## 8.9 Idioms {#sec-8-9}

The default value of `issueMod` and `issueAMod` of the methods of the abstract
representation is `eModBlck` because it is the safe one ([Proposition
8.1](08-modification-janus.md#prop-8-1)); any other value is an optimization,
to be used only when it is clear what the listeners need.

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
is to group them by a channel, so that a `:Solver` can react to the
group at once, which is often more efficient than reacting to each of
them in turn.

## 8.10 Forward reference to `Change` {#sec-8-10}

The present design of `Modification` has one limitation: a `Modification`
carries enough information to *describe* a change, but not always enough to
*replay* it on a different `:Block`, and in general it cannot be serialized in
a form that crosses the boundary of a process. The concept of
[`Change`](https://smspp.gitlab.io/smspp-project/db/d4d/class_s_m_spp__di__unipi__it_1_1_change.html)
(cf. [Chapter 16](16-change.md#ch-16)) addresses this limitation: a `Change`
carries the data needed to apply the change to any copy of a `:Block`, and it
can be serialized to netCDF and deserialized as a plain object. Where the
`:Block` supports it, it can also be *undone* through an inverse `Change`,
generated automatically. It serves two purposes
([§16.3](16-change.md#sec-16-3)): generic search schemes, such as the
Branch-and-X of `BranchAndXSolver`, and (in the future) distributed
computation by message passing. The `:Change`
classes of the core and of the modules are listed in [Table
16.1](16-change.md#tab-16-1).
