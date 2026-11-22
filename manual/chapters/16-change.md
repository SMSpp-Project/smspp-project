# 16. Change {#ch-16}

> *Status: beta.* The `Change` concept is in `develop` but its
> interface and semantics may change in incompatible ways. Only a
> small number of `:Change` classes exist at version 0.8.0
> (notably for `BinaryKnapsackBlock` and `SATBlock`, besides the
> `AbstractChange` of the core, a change of the abstract representation
> of a `Block`: the objective coefficient, the integrality, the fixing or
> a bound of a `ColVariable`, or the sense of an `Objective`, the
> `GroupChange` of the core, a sequence of `Change`s applied as one, and
> the `LagrangianChange` of `LagrangianDualRelaxationSolver`). They are
> used by the generic Branch-and-X of `BranchAndXSolver`
> ([§16.5](16-change.md#sec-16-5)). Code written against it
> today should expect to be revisited.

[Source: [`SMS++/include/Change.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h),
[`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h)]

## 16.1 Concept {#sec-16-1}

A
[`Change`](https://smspp.gitlab.io/smspp-project/db/d4d/class_s_m_spp__di__unipi__it_1_1_change.html)
is, like a `Modification`, an object that describes a change to a
`Block`. The two are duals of each other in *time*
([`Change.h:69-98`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L69-98)): a `Modification`
([Chapter 8](08-modification-janus.md#ch-8)) *notifies* of a change
that has already occurred in a `Block`, so that the listening
`:Solver`s can react, while a `Change` *represents* a change that may
be applied later, carrying all the data needed to perform it.

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

which applies the change to `block` (issuing `Modification`s in the
usual way, governed by the two `ModParam`s) and, if `doUndo` is `true`,
returns an *UndoChange*, i.e., a new `:Change` that, applied to
`block`, restores it to the state it was in before
([`Change.h:413-430`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L413-430)); the caller owns the UndoChange. That this
contract composes is what makes the search schemes of
[§16.3](16-change.md#sec-16-3) possible.

**Lemma 16.1.** *Let $C_1, \ldots, C_n$ be `Change`s applied in this
order to a `Block` in state $s_0$, each with `doUndo` true, leading to
the states $s_1, \ldots, s_n$, and let $U_k$ be the UndoChange returned
by $C_k$, so that $U_k$ applied in state $s_k$ gives $s_{k-1}$. Then
applying $U_n, U_{n-1}, \ldots, U_1$ in this order in state $s_n$ gives
$s_0$. In particular, the UndoChange of a `GroupChange` of
$C_1, \ldots, C_n$, which is the `GroupChange` of $U_n, \ldots, U_1$,
restores the state before the `GroupChange`.*

*Proof.* By induction on $j = n, n - 1, \ldots, 0$, after applying
$U_n, \ldots, U_{j+1}$ the state is $s_j$: this holds for $j = n$, and
if it holds for $j$, applying $U_j$ in state $s_j$ gives $s_{j-1}$ by
hypothesis. For $j = 0$ the state is $s_0$. `GroupChange::apply()` with
`doUndo` applies its sub-`Change`s in order and pushes each UndoChange
at the front of the new `GroupChange`, which therefore holds them in
reverse order ([`Change.h:571-588`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L571-588)). $\square$

We remark that the hypothesis is on each $U_k$ in the state $s_k$ in
which it was produced: an UndoChange applied after other changes of the
`Block` restores what it recorded (e.g., the old weights of its items),
not the state before its `Change`.

## 16.2 Abstract and specific `Change`s {#sec-16-2}

Like `Modification`s and `Block`s, `Change`s come in two flavours. An
*abstract* `:Change` operates on the abstract representation and
therefore applies to any `Block` (as `AbstractChange` does, fixing a
`ColVariable` or changing one of its bounds, its integrality or its
objective coefficient, or the sense of the `Objective`); a
*specific* `:Change` is written for a particular `:Block` and uses its
specialised interface to change its physical representation directly.

A specific `:Change` can support at most the changes that its
target `:Block` supports. It may deliberately support *fewer*, for
a reason peculiar to `Change`: producing the UndoChange of an
arbitrary change can be complicated or impossible, so a `:Change`
that wants to guarantee an undo may restrict itself to the subset
of changes it can reliably invert ([`Change.h:88-98`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L88-98)).

## 16.3 Why `Change` exists {#sec-16-3}

### Serialisation and distribution

If a `Change` only ever applied a change in the same process that
created it, it would add little over calling the `:Block`'s
`chg_*()` methods directly. Its reason for existing is that, unlike
a `Modification`,

> a `Change` is a *plain, self-contained object* that can be
> `[de]serialize`-d to and from netCDF independently of any
> `Block`.

`Change` carries a class-name factory (`Change::new_Change( name )`,
`Change::deserialize( NcGroup )`) of exactly the same kind as `Block`,
`Solver` and `Solution` ([Chapter 18](18-factories-netcdf.md#ch-18)), and a
`:Change` read from a netCDF group is reconstructed as the right concrete type
and then `apply()`-ed. This makes a `Change` something a `Modification` can
never be: a portable, storable, *transmissible* instruction. The intended
downstream use is message-passing distributed computation, in which a process
computes a `Change`, serialises it and ships it to another process holding a
copy of the `Block`, which deserialises and `apply()`s it. The `Change`
mechanism is, in this sense, the groundwork for the distributed `:Solver`s
exchanging `Change`s that the project plans but does not yet ship.

### Support for generic search schemes

A second, equally important reason for `Change` to exist is that
it makes it possible to write search algorithms in terms of
abstract *moves* that are independent of the `:Block` they are
applied to. A branch-and-bound, a Branch-and-X (B&X) scheme, a
large-neighbourhood search and a local-search metaheuristic are all,
at heart, "apply a move, explore, then undo or commit".
If the move is a `Change`, the *search driver* need know nothing
about the concrete `:Block`: it applies `Change`s and rolls them
back, and the same driver works for any problem class that
supplies the appropriate `:Change`s. The undo facility
([§16.1](16-change.md#sec-16-1)) is what makes the roll-back free:
by Lemma 16.1, the UndoChanges applied in reverse order restore any
previous state of the path, wherever the `:Change`s support the undo.

The B&X case is the most telling, and it shows why the `:Change`
abstraction is more than a convenience. The idea is that the
`Change`s encode the *branching rule*, and that they are produced by
the `:Solver` that solves the relaxation. Because
that `:Solver` may be a *specialised* one, it can construct the
branching `Change`s tailored not only to the structure of the
problem but also to its own ability to reoptimize after the branch, choosing moves that its
warm-start machinery (cf. the reoptimization vocabulary of
[Chapter 13](13-function-family.md#ch-13)) can absorb cheaply. The
search driver, meanwhile, stays completely generic: it receives
`Change`s from the relaxation `:Solver`, applies them to descend
the tree, and uses the UndoChanges to backtrack, without any
problem-specific code of its own. This decoupling of a generic B&X
driver from a source of branching `Change`s specific to the problem and
to the `Solver` is one of the more ambitious uses of the `Change`
concept, and its current form is described in
[§16.5](16-change.md#sec-16-5).

## 16.4 The reference example: `BinaryKnapsackBlock`'s `Change`s {#sec-16-4}

`BinaryKnapsackBlock` is the reference implementation of the
`Change` family at version 0.8.0. It ships three specific
`:Change` classes:

- [`BinaryKnapsackBlockChange`](https://smspp.gitlab.io/smspp-project/d8/d50/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block_change.html),
  the base, which holds a type (`eChgWeight`, `eChgProfit`,
  `eChgIntegrality`, `eChgCapacity`, `eFixX`, `eUnfixX`, `eChgSense`
  or `eEmpty`) and a vector of data;
- `BinaryKnapsackBlockRngdChange`, a change of a contiguous range of
  items;
- `BinaryKnapsackBlockSbstChange`, a change of an arbitrary subset of
  items,

mirroring, on the `Change` side, the `Rngd` / `Sbst` distinction
that the `Modification` family draws ([Chapter 8](08-modification-janus.md#ch-8)) and that the
methods factory draws ([Chapter 15](15-methods-factory.md#ch-15)). Each carries the new data
(weights, profits, capacity, fixings) for the items it covers, can
be serialised to netCDF, applied to a `BinaryKnapsackBlock`, and produce
its UndoChange within the subset it supports. Any other `Block` makes
`apply()` throw `std::invalid_argument`, as does a data vector whose
size differs from that of the range; for a change of weights the
UndoChange is the change with the old weights of the same items, which
satisfies the hypothesis of Lemma 16.1.

```cpp
#include "BinaryKnapsackBlock.h"

using namespace SMSpp_di_unipi_it;

int main( void )
{
 BinaryKnapsackBlock bkb;
 bkb.load( 3 , 10 , { 1 , 2 , 3 } , { 4 , 5 , 6 } , { true , true , true } );

 // a Change setting new weights on the items [ 0 , 3 ); in a distributed
 // setting it would rather be read with Change::deserialize() out of the
 // netCDF group shipped by another process
 BinaryKnapsackBlockRngdChange chg( BinaryKnapsackBlockChange::eChgWeight ,
                                    { 5 , 6 , 7 } , Block::Range( 0 , 3 ) );

 // apply it, asking for the UndoChange to roll back later
 Change * undo = chg.apply( &bkb , true );

 // ... explore the changed Block ...

 undo->apply( &bkb );  // back to the weights 1 , 2 , 3
 delete undo;
 return( 0 );
 }
```

## 16.5 `ChangeSolver`, `RelaxationSolver` and `BranchAndXSolver` {#sec-16-5}

[Source: [`SMS++/include/ChangeSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ChangeSolver.h),
[`BranchAndXSolver/include/BranchAndXSolver.h`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/include/BranchAndXSolver.h)]

The search schemes of [§16.3](16-change.md#sec-16-3) rest on two *traits* of the core,
[`ChangeSolver`](https://smspp.gitlab.io/smspp-project/d9/d60/class_s_m_spp__di__unipi__it_1_1_change_solver.html)
and
[`RelaxationSolver`](https://smspp.gitlab.io/smspp-project/db/d72/class_s_m_spp__di__unipi__it_1_1_relaxation_solver.html).
Neither derives from `Solver`: a concrete class derives from one
of them *alongside* its problem-specific `:Solver` base, with plain
inheritance, and whoever drives it discovers the trait on a
registered `Solver` by a `dynamic_cast` cross-cast. A
`ChangeSolver` can `apply( Change * chg , bool doUndo = false )` a `Change`
to the `Block` it works on, returning the UndoChange if asked; the
simplest implementation forwards to `Change::apply()`, but a
`:Solver` can rather intercept the `Change`s it understands and
apply them *internally*, to its own state and without touching the
`Block`, which is what makes moving along a sequence of related
problems cheap. It also receives a `GlobalInformation`, through
which the cooperating `:Solver`s share what is global to the search
(the incumbent, globally valid cuts). A `RelaxationSolver` is a
`ChangeSolver` that solves a *relaxation* of the problem: besides
the relaxation value (a valid bound), it may produce true solutions
of the original problem (`get_true_lb()` / `get_true_ub()`,
`get_true_var_solution()`), and `branch()` returns the `Change`s
that generate the children of the current node, the most promising
first.

[`BranchAndXSolver`](https://smspp.gitlab.io/smspp-project/d9/d56/class_s_m_spp__di__unipi__it_1_1_branch_and_x_solver.html)
is the generic driver built on them (the Relaxation-Agnostic
Branch-and-X): nothing in it depends on the `:Block` being solved.
A node of its enumeration tree is identified by the `Change` that
generates it from its father, and owns that `Change`, its UndoChange
and its bound; a branching decision that touches several things at
once is a `GroupChange`, whose undo is the composition of the undos
in reverse order. A node is evaluated by the `:Solver`s described
by a `BlockSolverConfig` (`strNameOfBlockSolverConfigurationFile`):
one or more `:RelaxationSolver`s, the tightest of which gives the
bound and branches the node, and zero or more plain `:ChangeSolver`s
acting as primal heuristics. The tree is explored depth-first,
breadth-first, best-first (possibly with dives) or by best estimate
(`intSolveMethod`), in parallel both across the tree and across the
`:Solver`s of a node ([Chapter 17](17-parallel.md#ch-17)), and can be retained across
`compute()` calls for reoptimization (`intReoptimize`). At version
0.8.0 the `:RelaxationSolver`s are
`GreedyRelaxationBinaryKnapsackSolver` (for `BinaryKnapsackBlock`),
`LagrangianDualRelaxationSolver` (the Lagrangian dual of a `Block`,
with the primal solutions recovered by `PrimalProximalHeur`, see
[Chapter 14](14-lag-benders-bfunction.md#ch-14)), its variant `LagrangianDualRelaxationSolverML`, which
learns its branching rule online, and the `SATSolver`s of
`SATBlock`.

## 16.6 `Change` versus `Modification`: which to use {#sec-16-6}

For ordinary, in-process model edits (the "change a cost, re-solve" of
Chapters [8](08-modification-janus.md#ch-8) and
[10](10-r3block.md#ch-10)) the `Modification` mechanism, driven by the `:Block`'s `chg_*()` methods, remains the
right tool, and is the one the rest of this manual uses. `Change`
is the right tool when one needs one of the things a `Modification`
cannot provide:

- to *serialise* an edit and apply it elsewhere or later;
- to *transmit* an edit across a process boundary (the
  distributed-`:Solver` use case);
- to *undo* an edit cheaply through an automatically generated
  inverse.

Since the concept is beta and implemented for only a few
`:Block`s, a `:Block` author should regard adding a `Change`
family as an *optional* and forward-looking step ([Appendix C](C-writing-modification.md#app-C)), not
a requirement; and a user should reach for `Change` only when one
of the three needs above is actually present.
