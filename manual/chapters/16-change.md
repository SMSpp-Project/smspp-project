# 16. Change {#ch-16}

[Source: [`SMS++/include/Change.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h),
[`SMS++/include/AbstractChange.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/AbstractChange.h),
[`SMS++/include/ChangeSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ChangeSolver.h),
[`SMS++/include/GlobalInformation.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/GlobalInformation.h),
[`SMS++/include/BranchingRule.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BranchingRule.h),
[`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h),
[`BranchAndXSolver/include/BranchAndXSolver.h`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/include/BranchAndXSolver.h);
the tests of [`BranchAndXSolver/test/test.cpp`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/test/test.cpp)
and of [`tests/BinaryKnapsackBlock`](https://gitlab.com/smspp/tests/-/tree/develop/BinaryKnapsackBlock)]

We are concerned in this chapter with the `Change`, i.e., the object that
represents a change of a `Block` before it is made, and with the algorithms
written in terms of `Change`s, foremost the generic Branch-and-X (B&X) of
`BranchAndXSolver`, in which nothing depends on the `:Block` being solved. We
first define the concept and its undo, and prove that undoing a sequence of
`Change`s restores the state it started from ([§16.1](16-change.md#sec-16-1)).
Then we distinguish the abstract from the specific `:Change`s
([§16.2](16-change.md#sec-16-2)), explain why the concept exists alongside the
`Modification` ([§16.3](16-change.md#sec-16-3)), and list the `:Change`s of
the catalog around the reference example of `BinaryKnapsackBlock`
([§16.4](16-change.md#sec-16-4)). We introduce the two traits of the core and
the B&X built on them in [§16.5](16-change.md#sec-16-5), while
[§16.6](16-change.md#sec-16-6) discusses when a `Change` is to be preferred to
a `Modification`.

Most of the chapter is then devoted to the B&X. In
[§16.7](16-change.md#sec-16-7) we give the contract of the traits
`ChangeSolver` and `RelaxationSolver`, with the protocol of the statuses and
of the cutoff, and the `GlobalInformation` through which the cooperating
`:Solver`s share what is global to the search. We then state the B&X and prove
its correctness in [§16.8](16-change.md#sec-16-8), and give the exploration of
the tree, as pseudocode, in [§16.9](16-change.md#sec-16-9).
[§16.10](16-change.md#sec-16-10) discusses the strategies, the reoptimization
across `compute()` calls and the parallel exploration. Finally, the components
for choosing the branching variable and for estimating the children are in
[§16.11](16-change.md#sec-16-11), the `:RelaxationSolver`s of the catalog in
[§16.12](16-change.md#sec-16-12), and the notes and references in
[§16.13](16-change.md#sec-16-13).

## 16.1 Concept {#sec-16-1}

A
[`Change`](https://smspp.gitlab.io/smspp-project/db/d4d/class_s_m_spp__di__unipi__it_1_1_change.html)
is, like a `Modification`, an object that describes a change to a `Block`. The
two are duals of each other in *time*
([`Change.h:69-98`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L69-98)):
a `Modification` ([Chapter 8](08-modification-janus.md#ch-8)) *notifies* of a
change that has already occurred in a `Block`, so that the listening
`:Solver`s can react, while a `Change` *represents* a change that may be
applied later, carrying all the data needed to perform it.

This difference in orientation has a concrete consequence: since a `Change`
has to be able to *enact* the change, and not merely describe one that some
other code already made, it must contain *all the new data* of the change.
A `Modification` can be a thin notification ("cost of arc 3 changed"),
while a `Change` must be a self-contained instruction ("set the cost of arc 3
to 7.0").

The single defining operation is

```cpp
virtual Change * apply( Block * block ,
                        bool doUndo = false ,
                        ModParam issueMod = eNoBlck ,
                        ModParam issueAMod = eNoBlck ) = 0;
```

which applies the change to `block` (issuing `Modification`s in the usual way,
governed by the two `ModParam`s) and, if `doUndo` is `true`, returns an
*UndoChange*, i.e., a new `:Change` that, applied to `block`, restores it to
the state it was in before
([`Change.h:413-430`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L413-430));
the caller owns the UndoChange.

Formally, if $s$ denotes the state of a `Block` (its data, and therefore the
problem it encodes), a `Change` $c$ is a partial map $s \mapsto c(s)$. It is
defined where `apply()` does not throw: a `:Change` refuses, by an exception,
a `Block` of the wrong type, data of the wrong size, and the states in which
it could not produce its undo (cf. [§16.2](16-change.md#sec-16-2)). The
UndoChange $c^{-1}_s$ depends on $s$, since it records the data that $c$
overwrites, and it is required to satisfy $c^{-1}_s(c(s)) = s$. For some
`:Change`s, however, the undo gives back a state that encodes the same problem
(the same feasible region and the same objective function) without being the
same state of the objects; we write $s \equiv s'$ for two such states. That
this contract composes, which the search schemes of
[§16.3](16-change.md#sec-16-3) need, is made precise by the following lemma.

<span id="lem-16-1"></span>**Lemma 16.1** (Undoing a sequence of `Change`s).
*Let $C_1, \ldots, C_n$ be `Change`s applied in this order to a `Block` in
state $s_0$, each with `doUndo` true, leading to the states
$s_1, \ldots, s_n$, and let $U_k$ be the UndoChange returned by $C_k$, so that
$U_k$ applied in state $s_k$ gives a state $U_k(s_k) \equiv s_{k-1}$, where a
`Change` defined at a state $s$ is assumed to be defined at every
$s' \equiv s$ and to give two states $c(s) \equiv c(s')$. Then applying
$U_n, U_{n-1}, \ldots, U_1$ in this order in state $s_n$ gives a state
$\equiv s_0$, and it gives $s_0$ itself if $U_k(s_k) = s_{k-1}$ for each $k$.
In particular, the UndoChange of a `GroupChange` of $C_1, \ldots, C_n$, which
is the `GroupChange` of $U_n, \ldots, U_1$, restores the state before the
`GroupChange`, under the same hypotheses, provided that each sub-`Change`
either returns its UndoChange or leaves the state unchanged.*

*Proof.* By induction on $j = n, n - 1, \ldots, 0$, the state $t_j$ after
applying $U_n, \ldots, U_{j+1}$ satisfies $t_j \equiv s_j$. This holds for
$j = n$, and if it holds for $j$, then
$U_j(t_j) \equiv U_j(s_j) \equiv s_{j-1}$, by the assumption on the `Change`s
and by hypothesis. In the exact case $t_j = s_j$ and
$U_j(t_j) = U_j(s_j) = s_{j-1}$, with no need of the assumption. For $j = 0$
the claim follows. `GroupChange::apply()` with `doUndo` applies its
sub-`Change`s in order and pushes each UndoChange at the front of the new
`GroupChange`, which therefore holds them in reverse order; a sub-`Change`
that returns no UndoChange contributes nothing to it, and if a sub-`Change`
throws, the partial UndoChange is deleted and the exception propagated, the
sub-`Change`s before it remaining applied (since SMS++ 0.9.0 for the missing
UndoChange and for the deletion of the partial one;
[`Change.h:571-596`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L571-596)).
Hence, the first part applies to it, the sub-`Change`s that return no
UndoChange being, by hypothesis, the identity on the states where they are
applied. $\square$

We remark that the hypothesis is on each $U_k$ in the state $s_k$ in which it
was produced: an UndoChange applied after other changes of the `Block`
restores what it recorded (e.g., the old weights of its items), not the state
before its `Change`.

<span id="rem-16-1"></span>**Remark 16.1.** The specific `:Change`s of the
catalog ([Table 16.1](16-change.md#tab-16-1)) satisfy the hypothesis of
[Lemma 16.1](16-change.md#lem-16-1) with equality in each state they accept.
In fact, those of `BinaryKnapsackBlock` and `SATBlockChange` refuse to fix a
variable that is fixed and to unfix one that is not, since the undo could not
tell the previous state, and the undo of a `CSPBlockChange` sets back the
previous domains. `AbstractChange` is exact on the "natural" states, with two
exceptions. On a variable that is not fixed, the undo of `eUnfixX` fixes it
(at its current value), and then the hypothesis fails; the undo of `eChgObj`
on a variable that was not in the objective `Function` leaves it there with a
zero coefficient, which gives an equivalent state. Similarly, the first bound
that `LagrangianDualRelaxationSolver` puts on a variable with
`intApplyStrategy` `eMaster` ([§16.12](16-change.md#sec-16-12)) becomes a new
dual pair of a `LagBFunction`, and its undo resets the constant of that pair
to the bound of the variable. Again, the redundant pair that is left gives an
equivalent state, on which a later bound on the same variable acts as on the
original one.

## 16.2 Abstract and specific `Change`s {#sec-16-2}

Like `Modification`s and `Block`s, `Change`s come in two flavors. An
*abstract* `:Change` operates on the abstract representation and therefore
applies to any `Block`; a *specific* `:Change` is written for a particular
`:Block` and uses its specialized interface to change its physical
representation directly.

In the core, the abstract `:Change` is
[`AbstractChange`](https://smspp.gitlab.io/smspp-project/db/d74/class_s_m_spp__di__unipi__it_1_1_abstract_change.html),
whose types are `eChgObj`, `eChgSense`, `eChgIntegrality`, `eFixX`, `eUnfixX`,
`eChgLB` and `eChgUB`
([`AbstractChange.h:52-82`](https://gitlab.com/smspp/smspp/-/blob/develop/include/AbstractChange.h#L52-82)).
It identifies the `ColVariable` and the `Objective` it acts upon by their
`AbstractPath` relative to the `Block` it is applied to, and hence the same
object applies to any identical copy of that `Block`. Some types have
conditions: `eChgObj` needs an `FRealObjective` whose `Function` is a
`LinearFunction` or a `DQuadFunction` (in which case the new quadratic
coefficient is given as well), and a variable not yet in the `Function` is
added to it. Also, `eChgLB` and `eChgUB` write the new bound in the left- or
right-hand side of the first `BoxConstraint`, `LBConstraint` or
`UBConstraint` active on the variable, and throw if there is none. Finally,
`eFixX` on a variable fixed at another value unfixes it first, and whoever
reads the fixings receives both steps. Only `issueAMod` is used, since the
change is made on the abstract representation.

A specific `:Change` can support at most the changes that its target `:Block`
supports. It may deliberately support *fewer*, for a reason peculiar to
`Change`: producing the UndoChange of an arbitrary change can be complicated
or impossible, and a `:Change` that wants to guarantee an undo may be
restricted to the subset of changes it can reliably invert
([`Change.h:88-98`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L88-98)).
The `:Change`s of `BinaryKnapsackBlock`, e.g., refuse to fix an item that is
already fixed.

## 16.3 Why `Change` exists {#sec-16-3}

### Serialization and distribution

If a `Change` only ever applied a change in the same process that created it,
it would add little over calling the `:Block`'s `chg_*()` methods directly.
Unlike a `Modification`, however, a `Change` is a plain object that holds all
its data and no reference to a `Block`, and therefore it can be written to
netCDF and read back independently of any `Block`. `Change` carries a
class-name factory of the same kind as those of `Block`, `Solver` and
`Solution` ([Chapter 18](18-factories-netcdf.md#ch-18)), with the static
methods
([`Change.h:153-312`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h#L153-312))
`Change::new_Change( name )`, which constructs an empty `:Change` of the given
class, and `Change::new_Change( group )`, which constructs the right concrete
type out of the `"type"` attribute of a netCDF group and fills it by its
`deserialize( group )`. As for files, `serialize_f( file )` appends the
`Change` to an open file as the group `Change_<n>`, where $n$ is the number of
groups that the file already has, and the static
`Change::deserialize( file , n )` reads back the $n$-th `Change` of a file.
Similarly, `serialize( filename )` writes the `Change` as the only one of a
new file, and `Change::deserialize(
"filename[n]" )` reads the $n$-th one of the named file. Each leaf `:Change`
of the catalog ([§16.4](16-change.md#sec-16-4)) but
`BinaryKnapsackBlockSbstChange` can be serialized and read back. Two further
exceptions concern the `Change`s of the B&X. A `GroupChange` writes only its
type and cannot be read back (its `deserialize()` throws), and a
`LagrangianChange` writes only the data of the `AbstractChange` it is, so that
it is read back as a `LagrangianChange` without the multipliers and the record
of the branching that it carries for its children
([§16.12](16-change.md#sec-16-12)).

Unlike a `Modification`, a `Change` can therefore be stored and transmitted.
One intended use is message-passing distributed computation, in which a
process computes a `Change`, serializes it and ships it to another process
holding a copy of the `Block`, which deserializes and `apply()`s it.
`:Solver`s distributed in this sense are not part of SMS++ at present; within
one process, however, the parallel exploration of `BranchAndXSolver` already
uses `Change`s as plain data, since its workers apply the `Change`s found
along the path to a node to their copies of the `Block`
([§16.10](16-change.md#sec-16-10)).

### Support for generic search schemes

A second reason, and at present the main one, for `Change` to exist is that it
makes it possible to write search algorithms in terms of abstract *moves* that
are independent of the `:Block` they are applied to. A branch-and-bound, a
Branch-and-X (B&X) scheme, a large-neighborhood search and a local-search
metaheuristic are all, at heart, "apply a move, explore, then undo or commit".
If the move is a `Change`, the *search driver* need know nothing about the
concrete `:Block`: it applies `Change`s and rolls them back, and the same
driver works for any problem class that supplies the appropriate `:Change`s.
The undo facility of [§16.1](16-change.md#sec-16-1) makes this roll-back
rather cheap, since by [Lemma 16.1](16-change.md#lem-16-1) the
UndoChanges applied in reverse order restore any previous state of the path,
wherever the `:Change`s support the undo.

The B&X case shows why the abstraction is needed: the `Change`s encode the
*branching rule*, and they are produced by the `:Solver` that solves the
relaxation. Because that `:Solver` may be a *specialized* one, it can
construct branching `Change`s tailored to the structure of the problem and
also to its ability to reoptimize after the branch, choosing moves that its
warm-start machinery (cf. the reoptimization vocabulary of [Chapter
13](13-function-family.md#ch-13)) can usually absorb cheaply. The search
driver, instead, stays generic: it receives `Change`s from the relaxation
`:Solver`, applies them to descend the tree, and uses the UndoChanges to
backtrack, with no code specific to the problem. This
decoupling of a generic B&X driver from a source of branching `Change`s
specific to the problem and to the `Solver` is the main use of the `Change`
concept in SMS++, and it is described from [§16.5](16-change.md#sec-16-5) on.

## 16.4 The reference example: `BinaryKnapsackBlock`'s `Change`s {#sec-16-4}

`BinaryKnapsackBlock` is the reference implementation of the `Change` family.
It ships three specific `:Change` classes:

- [`BinaryKnapsackBlockChange`](https://smspp.gitlab.io/smspp-project/d8/d50/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block_change.html),
  the base, which holds a type (`eChgWeight`, `eChgProfit`,
  `eChgIntegrality`, `eChgCapacity`, `eFixX`, `eUnfixX`, `eChgSense`
  or `eEmpty`) and a vector of data;
- `BinaryKnapsackBlockRngdChange`, a change of a contiguous range of
  items;
- `BinaryKnapsackBlockSbstChange`, a change of an arbitrary subset of
  items,

mirroring, on the `Change` side, the `Rngd` / `Sbst` distinction that the
`Modification` family draws ([Chapter 8](08-modification-janus.md#ch-8)) and
that the methods factory draws ([Chapter 15](15-methods-factory.md#ch-15)).
Each carries the new data (weights, profits, capacity, fixings) for the items
it covers, can be serialized to netCDF, applied to a `BinaryKnapsackBlock`,
and produce its UndoChange within the subset it supports. Any other `Block`
makes `apply()` throw `std::invalid_argument`, as does a data vector whose
size differs from that of the range. For a change of weights the UndoChange is
the change with the old weights of the same items, which satisfies the
hypothesis of [Lemma 16.1](16-change.md#lem-16-1) with equality. For instance,
if `bkb` is a `BinaryKnapsackBlock` of 3 items whose weights are 1, 2 and 3,
we can write

```cpp
// new weights for the items [ 0 , 3 ); in a distributed setting it would
// rather be read with Change::new_Change() out of a netCDF group
BinaryKnapsackBlockRngdChange chg( BinaryKnapsackBlockChange::eChgWeight ,
                                   { 5 , 6 , 7 } , Block::Range( 0 , 3 ) );

Change * undo = chg.apply( &bkb , true );  // weights 5 , 6 , 7
// ... explore the changed Block ...
undo->apply( &bkb );                       // back to the weights 1 , 2 , 3
delete undo;
```

The other `:Change` classes of the catalog are listed in [Table
16.1](16-change.md#tab-16-1).

<span id="tab-16-1"></span>**Table 16.1.** The `:Change` classes in `develop`,
with their types and the UndoChange that `apply()` returns.

| Class | Module | Types | UndoChange |
|---|---|---|---|
| `AbstractChange` | core | `eChgObj`, `eChgSense`, `eChgIntegrality`, `eFixX`, `eUnfixX`, `eChgLB`, `eChgUB` | the same type with the old data; `eUnfixX` after `eFixX` of a free variable, `eFixX` after `eUnfixX` ([Remark 16.1](16-change.md#rem-16-1)) |
| [`GroupChange`](https://smspp.gitlab.io/smspp-project/d9/d1c/class_s_m_spp__di__unipi__it_1_1_group_change.html) | core | a list of `Change`s applied in order | the `GroupChange` of the undos in reverse order |
| `BinaryKnapsackBlock[Rngd/Sbst]Change` | `BinaryKnapsackBlock` | as above | the old data; `eUnfixX` after `eFixX` and vice versa |
| [`SATBlockChange`](https://smspp.gitlab.io/smspp-project/d9/d02/class_s_m_spp__di__unipi__it_1_1_s_a_t_block_change.html) | `SATBlock` | `eFixX`, `eUnfixX` of some $x$ | the other type, at the previous values |
| [`CSPBlockChange`](https://smspp.gitlab.io/smspp-project/d2/d63/class_s_m_spp__di__unipi__it_1_1_c_s_p_block_change.html) | `CSPBlock` | `eAssign`, `eRemove`, `eSplitLE`, `eSplitGT`, `eSetDomain` of some domains | `eSetDomain` with the previous domains |
| [`LagrangianChange`](https://smspp.gitlab.io/smspp-project/d7/da2/class_s_m_spp__di__unipi__it_1_1_lagrangian_change.html) | `LagrangianDualSolver` | those of `AbstractChange` | as `AbstractChange`, or as in [Remark 16.1](16-change.md#rem-16-1) |

`CSPBlockChange` (since SMS++ 0.9.0) restricts or restores the current domains
of some variables of a `CSPBlock`, as a branching does. With the value $d_k$
given for the variable $v_k$, `eAssign` makes the domain of $v_k$ the value
$d_k$ only, `eRemove` takes $d_k$ out of it, `eSplitLE` keeps its values not
larger than $d_k$ and `eSplitGT` those larger; `eSetDomain` gives each
variable the values listed for it, and it is the undo of the others. A
`LagrangianChange` is an `AbstractChange` that the `apply()` of a
`LagrangianDualRelaxationSolver` handles in its own way
([§16.12](16-change.md#sec-16-12)); applied directly to a `Block`, this has to
be an `AbstractBlock`.

## 16.5 `ChangeSolver`, `RelaxationSolver` and `BranchAndXSolver` {#sec-16-5}

[Source: [`SMS++/include/ChangeSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ChangeSolver.h),
[`BranchAndXSolver/include/BranchAndXSolver.h`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/include/BranchAndXSolver.h)]

The search schemes of [§16.3](16-change.md#sec-16-3) rest on two *traits* of
the core,
[`ChangeSolver`](https://smspp.gitlab.io/smspp-project/d9/d60/class_s_m_spp__di__unipi__it_1_1_change_solver.html)
and
[`RelaxationSolver`](https://smspp.gitlab.io/smspp-project/db/d72/class_s_m_spp__di__unipi__it_1_1_relaxation_solver.html).
A `ChangeSolver` can `apply()` a `Change` to the `Block` it works on, or to
its internal state, returning the UndoChange on request. A `RelaxationSolver`
is a `ChangeSolver` that solves a *relaxation* of the problem; besides its
bound, it gives the solutions of the original problem that it finds, and it
can `branch()`, i.e., produce the `Change`s that generate the children of the
current node. Their contract is the subject of [§16.7](16-change.md#sec-16-7).

[`BranchAndXSolver`](https://smspp.gitlab.io/smspp-project/d9/d56/class_s_m_spp__di__unipi__it_1_1_branch_and_x_solver.html)
is the generic driver built on them (the Relaxation-Agnostic Branch-and-X). A
node of its enumeration tree is identified by the `Change` that generates it
from its father, and it is evaluated by the `:Solver`s that a
`BlockSolverConfig` describes: one or more `:RelaxationSolver`s, which give
the bound and branch the node, and zero or more plain `:ChangeSolver`s acting
as primal heuristics.

## 16.6 `Change` versus `Modification`: which to use {#sec-16-6}

For ordinary, in-process model edits (the "change a cost, re-solve" of
Chapters [8](08-modification-janus.md#ch-8) and [10](10-r3block.md#ch-10)) the
`Modification` mechanism, driven by the `:Block`'s `chg_*()` methods, remains
the right tool, and is the one the rest of this manual uses. `Change` is the
right tool when one needs one of the things a `Modification` cannot provide:

- to *serialize* an edit and apply it elsewhere or later;
- to *transmit* an edit across a process boundary, or to apply it to
  several copies of a `Block`;
- to *undo* an edit cheaply through an automatically generated
  inverse;
- to write a search over moves that is independent of the `:Block`,
  as the B&X of the following sections.

Hence, a `:Block` author adds a `Change` family when the `:Block` is to be
solved by a search of this kind, or when its edits are to be stored or shipped
([Appendix C](C-writing-modification.md#app-C)). Otherwise, one may use the
`AbstractChange` of the core, as long as the changes needed are among those of
its types.

## 16.7 The traits `ChangeSolver` and `RelaxationSolver` {#sec-16-7}

Neither trait derives from `Solver`: a concrete class derives from one of them
*alongside* its problem-specific `:Solver` base, plainly on both sides (the
trait `RelaxationSolver` inherits `ChangeSolver` virtually, so that a class
reaching it along several paths has a single `ChangeSolver` sub-object). One
holding plain `Solver` pointers discovers the trait by a `dynamic_cast`
cross-cast, which succeeds on the objects whose class derives from both.
`BranchAndXSolver` keeps, for each inner `Solver`, the pair of its two
personalities, and it refuses (by an exception) a `BlockSolverConfig`
describing a `Solver` that is not a `:ChangeSolver`. We list below the public
methods of the two traits
([`ChangeSolver.h:156-331`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ChangeSolver.h#L156-331)).

```cpp
// ChangeSolver
virtual Change * apply( Change * chg , bool doUndo = false ) = 0;
virtual void set_global_information( GlobalInformation * gi );

// RelaxationSolver : public virtual ChangeSolver
virtual std::vector< Change * > branch() = 0;
[[nodiscard]] virtual Solver::OFValue get_true_lb( void );
[[nodiscard]] virtual Solver::OFValue get_true_ub( void );
[[nodiscard]] virtual bool has_true_var_solution( void );
[[nodiscard]] virtual bool new_true_var_solution( void );
virtual void get_true_var_solution( Configuration * solc = nullptr ) = 0;
virtual Solution * get_true_solution( Configuration * solc = nullptr ) = 0;
```

The `apply()` of a `ChangeSolver` applies the `Change` and, with `doUndo`,
returns the (caller-owned) `Change` that brings back the `Block` and the
internal state of the `:Solver`. In the "simplest" case the implementation
just forwards to `Change::apply()` on the `Block`. Yet, a `:Solver` can rather
intercept the `Change`s it understands and apply them to its internal state
without touching the `Block`, as `GreedyRelaxationBinaryKnapsackSolver` does
(and `LagrangianDualRelaxationSolver` with `intApplyStrategy` `eMaster`),
which makes moving along a sequence of related problems "cheap". The
`GlobalInformation` given by `set_global_information()` is lent, not owned,
and `nullptr` means that the `:Solver` must work as if the information did not
exist. The tightenings that a `:Solver` makes at its discretion are folded
into its branching `Change`s when they are local to a node, and go into the
`GlobalInformation` when they are global
([`ChangeSolver.h:159-175`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ChangeSolver.h#L159-175)).

A `RelaxationSolver` gives the value of the relaxation, a valid bound on the
optimal value of the node, through the plain `get_lb()` / `get_ub()` of its
`Solver` personality. Its true bounds `get_true_lb()` / `get_true_ub()` are
the values of solutions of the original problem that it has found ($-\infty$
and $+\infty$ by default). Whether one is available is stated by
`has_true_var_solution()` (true by default, which suits a relaxation that
always produces a feasible rounding), and `new_true_var_solution()` (false by
default) produces another one. Finally, `get_true_var_solution()` writes it in
the `Variable`s of the `Block`, under the same locking rules as
`Solver::get_var_solution()`, while `get_true_solution()` returns it as a
caller-owned `Solution`, possibly without passing through the `Block`.

After `compute()`, the `branch()` of a `RelaxationSolver` returns the
caller-owned `Change`s each of which, applied in the current state, generates
one of the children of the current node, the most "promising" first (e.g., for
a binary variable, "$x = 1$" before "$x = 0$" if fixing to 1 changes the
relaxation the most); the order matters, since the caller may explore the
children in that order. These `Change`s must be such that the children cover
the part of the node that may contain solutions better than the incumbent (cf.
[§16.8](16-change.md#sec-16-8)). As a rule, the vector is not supposed to be
empty, with one exception that the catalog uses: a `RelaxationSolver` whose
`compute()` has *solved* the node, i.e., has found there a solution of the
original problem within its tolerance of the bound, may return no `Change`,
and `BranchAndXSolver` then takes the node as a leaf.
`LagrangianDualRelaxationSolver` does so when its Lagrangian solution has no
fractional variable and the gap is closed (since SMS++ 0.9.0). Conversely,
`SATSolver` throws when all the variables are fixed, but such a node is solved
by its `compute()` and fenced by its bound before `branch()` is called.

### The statuses and the cutoff

The status that the `compute()` of a `:RelaxationSolver` returns determines
what `BranchAndXSolver` may do with the node, as we summarize in [Table
16.2](16-change.md#tab-16-2) (since SMS++ 0.9.0 for the branching on a partial
result and for the treatment of `kCutOff`, which in 0.8.0 fenced the node in
any case). Before the evaluation of a node each relaxation is given the
incumbent as its cutoff (`dblLwCutOff` for a maximization, `dblUpCutOff` for a
minimization), so that one that proves that the node cannot improve on the
incumbent may stop there, returning `kCutOff`.

<span id="tab-16-2"></span>**Table 16.2.** The effect of the status of a
`:RelaxationSolver` on the evaluation of a node; $D(\nu)$ is the bound of the
node ([§16.8](16-change.md#sec-16-8)).

| Status | Effect on the node |
|---|---|
| `kOK` | the bound is used, and the relaxation may branch the node |
| `kStopTime`, `kStopIter`, `kCutOff`, or any other in $(\mathtt{kOK}, \mathtt{kError})$ | the bound is used; the relaxation may branch the node only if $D(\nu)$ is finite and it has a solution of the relaxation, otherwise the result is *partial* |
| `kLowPrecision` | the bound and the true solution are used, but the relaxation never branches the node: the result is partial |
| `kInfeasible` | the node is fenced as infeasible |
| any other (e.g., `kError`, `kUnbounded`) | the evaluation stops, and the whole solve ends with that status |

A partial result does not stop the solve. Indeed, the node is fenced if its
bound allows it; otherwise, if no relaxation of the node can branch it, the
node is *set aside* with its bound, which enters the bound of the solve as
that of an open node, and an unresolved `kCutOff` is recorded as
`kLowPrecision`. A failed certificate, instead, ends the solve with `kError`:
this is the case of a bound that is NaN, of a true solution whose value is not
finite, of a bound that the value of the true solution exceeds (for a
maximization) by more than the tolerance, and of an improving true solution
that `get_true_solution()` does not give.

### GlobalInformation

A
[`GlobalInformation`](https://smspp.gitlab.io/smspp-project/db/d3b/class_s_m_spp__di__unipi__it_1_1_global_information.html)
([`GlobalInformation.h:421-512`](https://gitlab.com/smspp/smspp/-/blob/develop/include/GlobalInformation.h#L421-512))
is a registry, keyed by name, of objects `Collection< T >` of arbitrary
(heterogeneous) type `T`. Whoever first needs a piece of global information
declares it with `add_to_Universe< T >( name )` (which throws if the name is
taken, by a `Collection` of any type), and from then on any holder of the
`GlobalInformation` obtains the same `Collection` by `get_from_Universe< T >(
name )`, or `nullptr` if there is none of that name and type. This returns a
`std::shared_ptr`, which keeps the `Collection` valid even if
`remove_from_Universe()` drops it concurrently. A `Collection< T >` is a map
from string keys to values of type `T`, guarded by a shared mutex (many
readers or one writer), with `read()` / `write()` and the functor forms
`read_with()` / `write_with()`. In the specialization `Collection<
std::atomic< T > >`, instead, the values are read and written lock-free
through a reference cached once, which suits the "hot" scalars read at each
step by everybody (typically, the incumbent). There is no `erase()`, and
therefore a cached reference is never invalidated.

Two design rules make the sharing sound. First, the data are *append-only and
monotone*: entries are added and improved, never removed (the incumbent only
gets better within a `compute()`, a pool of cuts only grows), and hence a
reader working on a stale value is never wrong, only less effective. Second,
the values are held by the `Collection` or through shared-ownership handles;
thus, an object consumed by several `:Solver`s at once (e.g., the `Change` of
a cut) stays valid for each of them. The reserved names are `AtomicScalars`
and `AtomicFlags`. The first is a `Collection< std::atomic< double > >` whose
key `Incumbent` holds the value of the best solution found by anybody (not
finite while there is none). `AtomicFlags`, in turn, is a
`Collection< std::atomic< bool > >` whose key `LocalFixingAllowed` states
whether a tightening that depends on the incumbent or on the data (e.g., a
reduced-cost fixing folded into a branching `Change`) is currently sound; it
is true when absent.

`BranchAndXSolver` owns one `GlobalInformation`, and it declares there the two
reserved `Collection`s when it is given a `Block`. It writes the incumbent in
it whenever this improves, and lends it to each `:RelaxationSolver` it
creates. It also fixes the name `GlobalCuts` and the key `Cuts` of a pool of
globally valid cuts (a
`Collection< std::vector< std::shared_ptr< Change > > >`), which it neither
creates nor reads, as a channel between relaxations. No `:RelaxationSolver` of
the catalog uses it at present, while
`LagrangianDualRelaxationSolver` adds a `Collection` of its own for the
columns that its fixings purge ([§16.12](16-change.md#sec-16-12)).

## 16.8 Branch-and-X: formulation and correctness {#sec-16-8}

We now state the B&X as the code does (the class comment,
[`BranchAndXSolver.h:92-257`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/include/BranchAndXSolver.h#L92-257),
gives an older form of the scheme, without the inherited bound, with the
brancher taken as the relaxation attaining the bound, and with children that
partition the node). Its `Block` encodes the problem $$ z^* = \max \{ f(x) : x
\in X \} $$ (a minimization being the mirror image, which the sense of the
`Objective` selects). It is solved by recursively partitioning $X$ and
bounding each part through a relaxation $\hat{X} \supseteq X$ over which
maximizing $f$ is "easy". We denote by $\bar{z}$ the value of the best
feasible solution found so far (the *incumbent*), $-\infty$ while there is
none. For a node $\nu$ of the enumeration tree, $X_\nu \subseteq X$ is the
subproblem it represents and $D(\nu) \geq \max \{ f(x) : x \in X_\nu \}$ its
bound.

*The enumeration tree.* A node $\nu$ is identified by the `Change` $c_\nu$
that generates it from its father $p(\nu)$. It owns $c_\nu$, the undo
$c_\nu^{-1}$ that goes back to the father, the `Change`s that generate its
children and its bound; hence
$$
X_\nu = ( c_\nu \circ c_{p(\nu)} \circ \cdots \circ c_{r'} ) X ,
$$
where $r'$ is the child of the root $r$ on the path to $\nu$. A `GroupChange`
$g = \langle c_1, \ldots, c_k \rangle$ is applied as the single `Change`
$c_k \circ \cdots \circ c_1$, and its undo is
$g^{-1} = c_1^{-1} \circ \cdots \circ c_k^{-1}$ ([Lemma
16.1](16-change.md#lem-16-1)). Thus, a branching decision that touches several
things at once is a `Change` like any other.

*Bounding and pruning.* With the tolerances $\varepsilon_a$ = `dblAbsAcc` and
$\varepsilon_r$ = `dblRelAcc` of `BranchAndXSolver`, let
$$
\varepsilon(\bar{z}) = \max \{ \varepsilon_a , \varepsilon_r
                               \max \{ | \bar{z} | , 1 \} \} ,
$$
where the default $\varepsilon_a = +\infty$ is taken as "not active", i.e., as
$\varepsilon_a = 0$ (plugged in literally, it would prune each node as soon as
there is an incumbent). A node $\nu$ is pruned when its bound cannot improve
on the incumbent within the tolerance, i.e., when
$$
\mathrm{PRUNE}( D(\nu) , \bar{z} ) \;\equiv\; \bar{z} > -\infty
\;\;\mbox{and}\;\; D(\nu) \leq \bar{z} + \varepsilon( \bar{z} ) ,
$$
and also when its relaxation is empty; in both cases we say that the node is
*fenced*. Note that a node is never pruned by its bound while there is no
incumbent.

*The `:Solver`s of a node.* A node is evaluated by the inner `:Solver`s that
the `BlockSolverConfig` of `strNameOfBlockSolverConfigurationFile` describes.
These are a set $\mathcal{R}$ of one or more `:RelaxationSolver`s, each giving
a bound $D_R(\nu)$ and possibly a true solution of value $\underline{z}_R$,
and a set $\mathcal{H}$ of zero or more plain `:ChangeSolver`s, the primal
heuristics, each giving only a feasible solution $x_H$ and never a bound; the
`BlockSolverConfig` must describe at least one `:RelaxationSolver`.
`BranchAndXSolver` creates and owns the inner `:Solver`s, and attaches them to
its `Block` without registering them. It forwards to each of them the
`Modification`s it receives that are of its concern, as the `Block` would do,
and accordingly its `concerned_by()` is the union of theirs. For the bound of
the node one takes the tightest of those of the relaxations,
$$
D(\nu) = \min \{ D(p(\nu)) , \min \{ D_R(\nu) : R \in \mathcal{R} \} \} ,
$$
since the bound of the father is valid for the child as well. The
incumbent, in turn, is updated with the best solution found by any `:Solver`
of the node,
$$
\bar{z} \leftarrow \max \{ \bar{z} \,,\,
                     \max \{ \underline{z}_R : R \in \mathcal{R} \} \,,\,
                     \max \{ f( x_H ) : H \in \mathcal{H} \} \} .
$$
A heuristic therefore changes neither the partition nor the bounds, only
$\bar{z}$; yet, a "good" incumbent found early is often decisive for the
pruning. Among the relaxations that may branch the node (cf. [Table
16.2](16-change.md#tab-16-2)), taken in the order of the `BlockSolverConfig`,
the one that branches it (the *brancher*) is the last whose bound has strictly
improved $D(\nu)$, or the first if none has.

*The covering of the children.* Let $c_1, \ldots, c_k$ be the `Change`s that
the brancher returns at $\nu$ when the incumbent is $\bar{z}$. We require of
the `RelaxationSolver` that
$$
\{ x \in X_\nu : f(x) > \bar{z} \} \subseteq \bigcup_{i=1}^k X_{\nu c_i} ,
$$
which holds in particular when the children partition the node,
$X_\nu = \bigcup_i X_{\nu c_i}$. This somewhat weaker form allows a branching
to fold in the local fixings that depend on the incumbent, as the reduced-cost
pegging of `GreedyRelaxationBinaryKnapsackSolver` does ([Proposition
16.4](16-change.md#prop-16-4)). We call *leaves* of the tree the nodes that
have been generated but never branched, i.e., those that are open, fenced or
set aside ([§16.7](16-change.md#sec-16-7)), and those whose brancher has
returned no `Change`.

<span id="prop-16-1"></span>**Proposition 16.1** (The leaves cover the
improving solutions). *Assume that (i) each bound is valid, i.e.,
$D_R(\nu) \geq f(x)$ for all $x \in X_\nu$, and a relaxation or a heuristic
returns `kInfeasible` only if $X_\nu = \emptyset$, (ii) each branching
satisfies the covering condition above, and (iii) the `:Solver`s moved to a
node $\nu$ ([§16.9](16-change.md#sec-16-9)) solve the problem $X_\nu$. Then,
at any time of a `compute()`, each $x \in X$ with $f(x) > \bar{z}$, for the
current incumbent $\bar{z}$, belongs to $X_\nu$ for some leaf $\nu$.*

*Proof.* By induction on the steps of the exploration. At the start the root
is the only leaf, and $X_r = X$. An improvement of the incumbent can only
shrink the set of the points $x$ with $f(x) > \bar{z}$, and fencing a node or
setting it aside does not change the set of the leaves. Branching a leaf
$\nu$, when the incumbent is $\bar{z}_t$, replaces it with its children
(including those fenced at their creation, which are leaves too). By (ii),
each point of $X_\nu$ better than $\bar{z}_t$, and hence each point better
than any later incumbent $\bar{z} \geq \bar{z}_t$, belongs to the region of a
child, and (iii) ensures that the regions of the children are those that the
`:Solver`s evaluate. $\square$

As we have seen in [§16.1](16-change.md#sec-16-1), hypothesis (iii) holds when
the `Change`s of the tree satisfy the hypotheses of [Lemma
16.1](16-change.md#lem-16-1), since a move between two nodes applies undos and
`Change`s along the path joining them ([Algorithm
16.3](16-change.md#alg-16-3)). From this we obtain the correctness of the B&X,
under one further hypothesis on the nodes whose brancher returns no `Change`.

<span id="prop-16-2"></span>**Proposition 16.2** (Correctness of the B&X).
*Under the hypotheses of [Proposition 16.1](16-change.md#prop-16-1), assume
that $\varepsilon_r \leq 1$ and that (iv) a brancher returns no `Change` at
$\nu$ only if it has found there a true solution of value at least
$D(\nu) - \varepsilon_R$, for some $\varepsilon_R \geq 0$; let
$\hat{\varepsilon} = \max \{ \varepsilon(\bar{z}) , \varepsilon_R \}$. If
`compute()` returns `kOK`, then $z^* \leq \bar{z} + \hat{\varepsilon}$, with
$\bar{z}$ the value of the solution it gives; if it returns `kInfeasible`,
then $X = \emptyset$. If it stops with any other status in
$[\mathtt{kOK}, \mathtt{kError})$ or with `kLowPrecision`, then
$z^* \leq \max \{ \bar{z} + \hat{\varepsilon} , \bar{D} \}$, where $\bar{D}$
is the largest bound of the nodes open or set aside in a serial exploration,
and the bound of the root in a parallel one.*

*Proof.* Let $x \in X$ with $f(x) > \bar{z}$ (if there is none,
$z^* \leq \bar{z}$). By [Proposition 16.1](16-change.md#prop-16-1),
$x \in X_\nu$ for a leaf $\nu$, and $f(x) \leq D(\nu)$ by (i). The leaf cannot
be fenced as infeasible, by (i). If it was fenced by its bound when the
incumbent was $\bar{z}_t \leq \bar{z}$, then
$f(x) \leq \bar{z}_t + \varepsilon(\bar{z}_t)$, which is at most
$\bar{z} + \varepsilon(\bar{z})$, since $z \mapsto z + \varepsilon(z)$ is
nondecreasing for $\varepsilon_r \leq 1$. If its brancher returned no
`Change`, the true solution found there has made the incumbent at least
$D(\nu) - \varepsilon_R$, and hence $f(x) \leq \bar{z} + \varepsilon_R$.
Otherwise the leaf is open or set aside at the end, which gives the last claim
(in a parallel exploration, $D(\nu)$ is at most the bound of the root, since a
node inherits the bound of its father). With `kOK`, each such node satisfies
$\mathrm{PRUNE}( D(\nu) , \bar{z} )$ (lines 26-28 of [Algorithm
16.2](16-change.md#alg-16-2)), and therefore
$f(x) \leq \bar{z} + \varepsilon(\bar{z})$. Finally, `kInfeasible` is returned
in place of `kOK` when there is no incumbent. Then no node has been fenced by
its bound, none has had an empty branching, by (iv), and none is left open or
set aside, since $\mathrm{PRUNE}$ is false without an incumbent; hence, all
the leaves are infeasible, and $X = \emptyset$ by [Proposition
16.1](16-change.md#prop-16-1). $\square$

The tolerance $\varepsilon_R$ is the one to which the relaxations close a
node; for `LagrangianDualRelaxationSolver` it is the `dblRelAcc` of the
solver, which need not be that of `BranchAndXSolver`. We remark that the
covering condition in the weaker form holds only for the incumbent in force
when the branching is made. This is fine within a `compute()`, where the
incumbent only improves, but not for a tree retained across calls, and for
this reason `BranchAndXSolver` clears the flag `LocalFixingAllowed`
([§16.7](16-change.md#sec-16-7)) whenever it retains the tree
([§16.10](16-change.md#sec-16-10)).

We now turn to the bounds that `BranchAndXSolver` reports, which follow the
same lines. For a maximization, `get_lb()` is the incumbent, while
`get_ub()` is the largest among the incumbent and the bounds of the nodes left
open, set aside or fenced (the leaves with an empty branching excluded, whose
bound is within $\varepsilon_R$ of the incumbent by (iv)), which reduces to
the incumbent after a proof of infeasibility. The best solution is written in
the `Block` by `get_var_solution()`, and `has_var_solution()` states whether
the last `compute()` has found one. A `compute()` with no change since
a previous one that ended with `kOK`, `kInfeasible` or `kUnbounded` returns
that result again, without exploring.

## 16.9 The exploration of the tree {#sec-16-9}

We describe the exploration as a single loop over a set $\mathcal{O}$ of open
nodes, whose discipline is the strategy ([§16.10](16-change.md#sec-16-10)). It
is driven by the evaluation of a node and by the move of the `:Solver`s
between two nodes, and we give the three procedures in this order. They are
written for a maximization; "within the time left" means that, for that call,
the `dblMaxTime` of an inner `Solver` is the smaller between the value set for
it and the time left to the solve (since SMS++ 0.9.0).

<span id="alg-16-1"></span>**Algorithm 16.1** (EVALUATE, the evaluation of a
node).

```text
 1  procedure EVALUATE(ν)                       ▷ the solvers are at ν
 2     D(ν) ← D(p(ν)), or +∞ at the root;  B ← nil;  s̄ ← kOK
       give each R ∈ 𝓡 the cutoff z̄
 3     for each R ∈ 𝓡, in the order of the BlockSolverConfig do
 4        s ← R.compute(), within the time left
 5        if no time was left to run R then return kStopTime
 6        if s = kInfeasible then fence ν as infeasible;  return kOK
 7        if s ∉ [kOK, kError) ∪ {kLowPrecision} then return s
 8        d ← the bound of R;  if d is NaN then return kError
 9        if R has a true solution x̂, of value ẑ, then
10           if ẑ is not finite or d < ẑ − ε(ẑ) then return kError
11           if ẑ > z̄ then z̄ ← ẑ;  x̄ ← x̂
12        better ← ( d < D(ν) );  if better then D(ν) ← d
13        ready ← s ∈ [kOK, kError) and ( s = kOK or
                    ( D(ν) < +∞ and R has a solution of the relaxation ) )
14        if ready and ( better or B = nil ) then B ← R
15        if PRUNE(D(ν), z̄) then fence ν by its bound;  return kOK
16        if not ready and s̄ = kOK then s̄ ← s (kLowPrecision if s = kCutOff)
17     if B = nil then return s̄                ▷ a partial result
18     for each H ∈ 𝓗 do
19        s ← H.compute(), within the time left
20        if s = kInfeasible then fence ν as infeasible;  return kOK
21        if s ≠ kOK then return s
22        if H has a feasible x with f(x) > z̄ then z̄ ← f(x);  x̄ ← x
23     return kOK                               ▷ B is the brancher of ν
```

In the code, the procedure corresponds to `computeRelaxations()` and
`computeHeuristic()` in
[`BranchAndXSolver.cpp:283-491`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/src/BranchAndXSolver.cpp#L283-491).
We now comment on the lines that are (perhaps) not obvious. Line 2 gives every
relaxation the incumbent at the start of the evaluation as its cutoff
([§16.7](16-change.md#sec-16-7)), and hence an incumbent improved by a
relaxation at line 11 is not passed to the later ones. Lines 6-7 are the first
rows of [Table 16.2](16-change.md#tab-16-2), and lines 8-10 check the
certificates, after which a true solution improving on the incumbent replaces
it (line 11). Line 12 keeps the bound inherited from the father unless the
relaxation improves it, and lines 13-14 choose the brancher as
[§16.8](16-change.md#sec-16-8) prescribes. Line 15 is tested after each
relaxation, and hence the later ones are not run on a node already fenced.
Line 16 records the first partial status, which line 17 returns if no
relaxation can branch the node, the caller then setting it aside. Heuristics
run only on a node that is neither fenced nor set aside. One that proves the
node empty fences it (line 20), while one that returns another status but
`kOK` ends the evaluation with that status, as a relaxation does (line 21).
With `intThreadForDifferentSolvers` > 1 and several relaxations (or
heuristics), their `compute()` run concurrently; their results are then
reduced in the order of lines 6-16, which gives the outcome of the serial loop
except that a node found empty by one relaxation is fenced without using the
true solutions of the others. An eager root is always evaluated serially.

<span id="alg-16-2"></span>**Algorithm 16.2** (EXPLORE, the serial
exploration).

```text
 1  procedure EXPLORE
 2     z̄ ← −∞;  𝓤 ← ∅                          ▷ 𝓤: the nodes set aside
 3     r ← the root;  D(r) ← +∞;  𝓞 ← {r}
 4     if the protocol is eager then
 5        s ← EVALUATE(r);  if s ≠ kOK then return s
 6        if r is not fenced then β(r) ← B.branch()
 7     while 𝓞 ≠ ∅ and no budget is exhausted do
 8        ν ← the node of 𝓞 that the strategy selects;  remove it from 𝓞
 9        if ν has not been evaluated and PRUNE(D(ν), z̄) then fence ν;  continue
10        MOVE(the solvers, to ν)
11        if ν has not been evaluated then
12           s ← EVALUATE(ν)
13           if s is partial then 𝓤 ← 𝓤 ∪ {ν};  fence ν;  continue
14           if s ≠ kOK then return s
15           if ν is not fenced then β(ν) ← B.branch()
16        if ν is fenced or PRUNE(D(ν), z̄) then fence ν;  continue
17        for each c ∈ β(ν), in their order do
18           μ ← the child of ν by c, with D(μ) ← D(ν) and the estimate of c
19           if the protocol is lazy then keep μ;  continue
20           MOVE(the solvers, to μ);  s ← EVALUATE(μ)
21           if s is partial then 𝓤 ← 𝓤 ∪ {μ};  fence μ
22           else if s ≠ kOK then return s
23           else if μ is not fenced and not PRUNE(D(μ), z̄) then
                β(μ) ← B.branch();  keep μ
24           MOVE(the solvers, to ν)
25        put the kept children in 𝓞, in reverse order if 𝓞 is a stack
26     if some ν ∈ 𝓞 has not PRUNE(D(ν), z̄) then
27        return kStopIter if the node budget is exhausted, else kStopTime
28     if some ν ∈ 𝓤 has not PRUNE(D(ν), z̄) then
          return kStopTime if out of time, else the status of the first in 𝓤
29     return kOK
```

[Algorithm 16.2](16-change.md#alg-16-2) is implemented by `treeSolve()` and `explore()` in
[`BranchAndXSolver.cpp:1103-1494`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/src/BranchAndXSolver.cpp#L1103-1494),
and `compute()` turns a `kOK` with no incumbent into `kInfeasible`. When a
node is evaluated is decided by the *bounding protocol*
(`intBoundingProtocol`). With the eager one (the default) a node is evaluated
as soon as it is created, inside the branching of its father (lines 20-23). It
then enters $\mathcal{O}$ only if its bound can improve on the incumbent, and
the open set holds the true bound of each node, by which a best-first
discipline orders them. The `Change`s of a kept child are obtained from its
brancher at once, while the `:Solver`s are still at the child whose
relaxation they come from. With the lazy protocol a node enters $\mathcal{O}$
unevaluated, carrying the bound of its father (line 19), and it is evaluated
when it is extracted (line 12). However, the incumbent found in the meantime
may fence it without any evaluation (line 9), before the `:Solver`s are moved
to it, which may save the evaluation of quite a few nodes and the moves to
them. The root is extracted and evaluated as any other node.
Either way a node is evaluated at most once, and the protocol changes when the
inner `:Solver`s are run, not what is explored.

At line 25, ties apart, the first of β(ν) comes out first among the kept
children, unless the discipline orders them by bound or by estimate
([§16.10](16-change.md#sec-16-10)). Line 7 ends the loop when $\mathcal{O}$ is
empty, or when one of the two budgets is over. Here, the time budget
`dblMaxTime` covers the whole solve, while the node budget `intMaxNodes`
counts the extractions from $\mathcal{O}$, and the loop stops when
`intMaxNodes` $- 1$ of them have been made. In the parallel exploration
([Algorithm 16.5](16-change.md#alg-16-5)) it counts instead the extractions of
the ramp-up and the nodes that the workers evaluate depth-first. A node with
no kept child is a leaf: the `:Solver`s go back to its father, and, unless the
tree is retained, the ancestors left without children are deleted on the way.
A node set aside is fenced for the exploration (lines 13 and 21), and it only
counts at the end, in the status (line 28) and in the bound of the solve. At
line 28, the status is that of the first node set aside, with no priority
among the partial statuses.

<span id="alg-16-3"></span>**Algorithm 16.3** (MOVE, the move of the
`:Solver`s between two nodes).

```text
 1  procedure MOVE(s, t)                         ▷ the solvers are at s
 2     P ← ∅                                     ▷ the nodes to descend
 3     while depth(s) ≠ depth(t) do
 4        if depth(s) > depth(t) then apply c_s⁻¹ to each solver;  s ← p(s)
 5        else P ← P ∪ {t};  t ← p(t)
 6     while s ≠ t do
 7        apply c_s⁻¹ to each solver;  s ← p(s)
 8        P ← P ∪ {t};  t ← p(t)
 9     for each u ∈ P, from the shallowest to the deepest do
10        if c_u⁻¹ is not known then
11           c_u⁻¹ ← the undo returned by the first solver, with doUndo
12           apply c_u to the other solvers
13        else apply c_u to each solver
```

In the code, MOVE is `ExploringNode::moveBetweenNodes()`
([`BranchAndXSolver.h:1559-1600`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/include/BranchAndXSolver.h#L1559-1600)).
It climbs from the deeper of the two nodes to the depth of the other, then
from both until they meet at their lowest common ancestor, applying the undos
on the side of $s$; finally, it descends to $t$ applying the `Change`s. Only
the first `:ChangeSolver` produces the undo of a node, which is then applied
to all of them. This presumes that a `Change` has the same effect on each of
the inner `:Solver`s, which holds when, as is the case in `BranchAndXSolver`,
they are all at the same node. A node entered for the first time gets its undo
at line 11, and this is how a node created by the lazy protocol, into which
the `:Solver`s have never descended, obtains it. A `:ChangeSolver` that
applies the `Change`s to its internal state never locks nor reads the `Block`
in a move, and this is where the efficiency of the enumeration (possibly)
comes from.

### The CSV log

When a log stream is set ([§6.8](06-solver.md#sec-6-8)), the serial
exploration writes one line per evaluated node, in CSV form, as soon as its
fate is decided, under the header

```text
iter,node,father,level,incumbent,dualBound,elapsedTime,evaluationTime,boundPruned,infeasible
```

In order, we find the number of the extraction during which the node was
evaluated (0 for an eager root), the progressive names of the node and of its
father ($-1$ for the root), its depth, the incumbent and the bound of the
node. They are followed by the time since the start of the solve, that of the
evaluation of this node, whether it was fenced or set aside, and whether it
was found infeasible; the nodes fenced by the lazy protocol without evaluation
have no line, nor do the leaves of a retained frontier re-evaluated by
[Algorithm 16.4](16-change.md#alg-16-4). Whatever the exploration, the log of
each `compute()` ends with the line `[BranchAndXSolver] nodes_evaluated=`
followed by the value of `get_evaluated_nodes()`, i.e., the number of the
evaluations started by the last `compute()`.

## 16.10 Strategies, reoptimization and parallelism {#sec-16-10}

*Strategies.* By strategy (`intSolveMethod`) we only mean the rule that
selects the next node in $\mathcal{O}$ at line 8 of [Algorithm
16.2](16-change.md#alg-16-2), and the open list implements it. This is a stack
for depth-first (`DFS`), a queue for breadth-first (`BFS`) and a priority
queue on the bounds for best-first (`BestFS`, the default). `BestFSDive` adds
dives to best-first, the search following the first child of the node just
branched down to a leaf and leaving its siblings to the priority queue.
Finally, best-estimate (`BestEstimate`) uses a priority queue on the
estimates. Each node has as estimate the one that the brancher of its father
gives of it, if it derives from `BranchEstimator` (cf.
[§16.11](16-change.md#sec-16-11)), and the bound of the node otherwise (that
of its father while it is unevaluated); it
is never taken better than the bound of the node. Note that the nodes are
still pruned by their bounds, and hence the estimates only change the order of
the exploration. Best-first explores the fewest nodes for a given relaxation,
since no node with $D(\nu) \leq z^*$ is ever branched, but it stores the whole
of $\mathcal{O}$. Depth-first only stores the current path and its siblings,
and hence it is usually the safer choice on "deep" trees.

*Reoptimization.* With `intReoptimize` > 0 (0 by default) a serial exploration
keeps the tree and its *frontier*, i.e., the leaves that are fenced, set aside
or left open, alive across `compute()` calls. A new `compute()` then
re-evaluates the frontier instead of deriving the tree again, provided that
the changes since the last solve leave the tree valid. Each `Modification`
received in the meantime is examined separately (those in a
`GroupModification` included). One that `keeps_tree()` rejects, or a physical
one that does not declare a change of the `Variable`s, of the `Constraint`s or
of the `Objective`, makes the next `compute()` solve from scratch. The test
`keeps_tree()`
([`BranchAndXSolver.cpp:1981-2006`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/src/BranchAndXSolver.cpp#L1981-2006))
rejects three kinds of change. The first is a change of the data of the
`Variable`s, which a move along the tree might undo, since the undos restore
the data they recorded. Then comes a change of integrality that may enlarge
the feasible region, since a variable that becomes continuous may have been
branched upon as an integer one (and $x \leq 0$, $x \geq 1$ do not cover
$(0, 1)$). Finally, it rejects the removal of `Variable`s or of `Constraint`s,
which may be those that the `Change`s act upon. Any other change is a
relaxation or a restriction of the problem, of which the re-evaluation of the
frontier takes care, an added `Variable` being free in each node and an added
`Constraint` holding in each node. We now make the requirement precise.

<span id="prop-16-3"></span>**Proposition 16.3** (A retained tree under a
change). *Let a tree be retained from a solve in which no local fixing has
been made, and let the data change so that (a) each `Change` of the tree still
applies to the new data, with the same meaning, and (b) for each branched node
$\nu$, with children $\nu c_1, \ldots, \nu c_k$, the new regions satisfy
$X'_\nu \subseteq \bigcup_i X'_{\nu c_i}$. Then the leaves of the tree cover
the new feasible region $X'$, and re-evaluating them, as [Algorithm
16.4](16-change.md#alg-16-4) does, the conclusion of [Proposition
16.1](16-change.md#prop-16-1) holds for the new problem throughout the next
`compute()`, under the hypotheses (i)-(iii) for the new problem.*

*Proof.* Let $x \in X' = X'_r$. Starting from the root, by (b) each branched
node whose new region contains $x$ has a child whose new region contains it.
Since the tree is finite, a leaf whose new region contains $x$ is reached, and
by (a) the `:Solver`s moved to that leaf solve its new region. Since no local
fixing has been made, the covering of each branching does not depend on the
incumbent of the previous solve, and the induction of [Proposition
16.1](16-change.md#prop-16-1) starts from a set of leaves that covers $X'$,
instead of from the root. $\square$

Under any change that keeps the variables and their integrality, condition (b)
holds for a branching that partitions the integer values of the variables it
acts upon, as $x \leq \lfloor v \rfloor$ and $x \geq \lceil v \rceil$ for an
integer variable, or the fixings of a binary variable at 0 and at 1; the tests
of `keeps_tree()` are meant to ensure (a). Since the incumbent of the previous
solve cannot be relied upon, each `compute()` starts with no incumbent.
While the tree is retained `BranchAndXSolver` also clears
`LocalFixingAllowed`, and therefore the relaxations do not fold into their
branchings the fixings that depend on the incumbent.

<span id="alg-16-4"></span>**Algorithm 16.4** (REOPTIMIZE, the re-evaluation
of the frontier of a retained tree).

```text
 1  procedure REOPTIMIZE                    ▷ z̄ ← −∞, the tree is retained
 2     F ← the leaves of the frontier that are not infeasible
 3     if the changes may enlarge the feasible region then
 4        F ← F ∪ the leaves fenced as infeasible
 5     sort F by nonincreasing bound, as found in the previous solve
 6     for each ν ∈ F do
 7        MOVE(the solvers, to ν);  D(ν) ← +∞;  s ← EVALUATE(ν)
 8        if s is partial then set ν aside, in the frontier;  continue
 9        if s ≠ kOK then MOVE(the solvers, to r);  return s
10        if ν is not fenced and not PRUNE(D(ν), z̄) then
11           if β(ν) = ∅ then β(ν) ← B.branch()
12           put ν in 𝓞
13        else put ν back in the frontier
14     continue with lines 7-29 of Algorithm 16.2
```

[Algorithm 16.4](16-change.md#alg-16-4) is implemented by `reseedFrontier()`
([`BranchAndXSolver.cpp:1003-1099`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/src/BranchAndXSolver.cpp#L1003-1099)).
The interior of the tree is not evaluated again, and a leaf is re-evaluated
from scratch (line 7), since its old bound refers to the old data. A leaf
fenced as infeasible stays fenced, with no evaluation at all, unless the
changes may enlarge the feasible region (line 3); otherwise, the region of an
empty node can only shrink. By the sort of line 5, the leaves where the
optimum most likely lies come first; hence, the incumbent (hopefully) improves
early and the rest of the frontier is often fenced again. A leaf reopened
keeps the branching `Change`s of the previous solve, if it had any (line 11).
Of course, in a binary enumeration the frontier is about as large as the
interior, and therefore retaining the tree usually pays off when the
evaluation of a node is "expensive", or when few nodes reopen, and it may use
a lot of memory.

*Parallelism across the `:Solver`s of a node.* With
`intThreadForDifferentSolvers` > 1, the `compute()` of the several relaxations
and heuristics of a node run concurrently on that many threads. Yet, the
reduction of their results stays serial (cf. the comment to [Algorithm
16.1](16-change.md#alg-16-1)). This is active in the serial explorations, but
for an eager root, which is evaluated serially, and it typically pays only
when several costly inner `:Solver`s are attached.

*Parallelism across the tree.* With `intSolveMethod` = `DFS` and the inherited
`intMaxThread` $= K > 1$, `compute()` runs the parallel depth-first
exploration of [Algorithm 16.5](16-change.md#alg-16-5) with $K$ workers,
instead of [Algorithm 16.2](16-change.md#alg-16-2). Each worker drives a
private set of inner `:Solver`s on a private `Block` (since SMS++ 0.9.0).
Worker 1 uses the inner `:Solver`s of `BranchAndXSolver`, and each other
worker a set created out of the same `BlockSolverConfig` and registered to an
identical copy of the `Block`, obtained by `get_R3_Block( nullptr )`
([§10.4](10-r3block.md#sec-10-4)), and therefore applying the `Change`s needs
no synchronization. The `Change`s apply to any of the copies, since they
identify what they change by indices or by `AbstractPath`s relative to the
`Block`; a `Block` that gives no copy, instead, cannot be explored in
parallel, and `compute()` throws.

<span id="alg-16-5"></span>**Algorithm 16.5** (PARALLEL-DFS, the parallel
depth-first exploration with $K$ workers).

```text
 1  procedure PARALLEL-DFS(K)
 2     give the workers 2, ..., K a copy of the Block, each with its solvers
 3     EVALUATE(r) with the solvers of the Block;  β(r) ← B.branch();  Q ← (r)
 4     while Q ≠ ∅ and |Q| < 4K and no budget is exhausted do   ▷ the ramp-up
 5        ν ← the first node of Q;  remove it;  MOVE(the solvers, to ν)
 6        if not PRUNE(D(ν), z̄) then
 7           for each c ∈ β(ν) do
 8              evaluate the child μ of ν by c, with D(μ) ← D(ν)
 9              if μ is neither fenced nor set aside then
                   β(μ) ← B.branch();  append μ to Q
10     MOVE(the solvers, to r)
11     each worker repeats, until Q = ∅ and no worker is busy:
12        take the first node ν of Q, waiting while Q = ∅
13        apply to its solvers the Changes of the path from r to ν
14        if not PRUNE(D(ν), z̄) then
15           for each c ∈ β(ν) do
16              if this worker has explored a child of ν and 2|Q| < K then
17                 evaluate the child μ by c;  if kept, β(μ) ← B.branch()
                   and append μ to Q
18              else DFS(the child of ν by c)
19        apply to its solvers the undos of the path from ν to r
20  procedure DFS(μ)                              ▷ recursive, in a worker
21     if a budget is exhausted then stop
22     move the solvers to μ;  s ← EVALUATE(μ)
23     if s is partial then set μ aside
24     else if μ is not fenced and not PRUNE(D(μ), z̄) then
25        for each c ∈ B.branch() do DFS(the child of μ by c)
26     move the solvers back to p(μ)
```

In the code, the procedure is split between `ParallelDFSSolve()` and
`workerDFS()` in
[`BranchAndXSolver.cpp:1500-1892`](https://gitlab.com/smspp/branchandxsolver/-/blob/develop/src/BranchAndXSolver.cpp#L1500-1892).
The ramp-up (lines 3-9) expands the tree in the order of discovery, with the
`:Solver`s of the `Block`, until the pool $Q$ holds $4K$ open subtrees. Each
node of the pool has its bound, its branching `Change`s and its undo already
computed, and a worker claims it by applying the `Change`s of its path (line
13). Each worker always claims the oldest node of the pool, which keeps the
order of the sequential search and makes the runs replicable ([Archibald et
al. 2018](Z-bibliography.md#ref-archibald2018)). A worker donates a branch of
its subtree to the pool, evaluated as in the ramp-up, when the pool holds
fewer than $K/2$ nodes (line 16). Only the incumbent, whose
updates are made under a mutex, and the pool are shared, and an error of one
worker stops all. Unlike the serial one, the parallel exploration retains no
tree (`intReoptimize` is ignored) and writes no CSV line per node; when
stopped by a budget, it claims as bound of the solve only that of the root,
which may be rather weak. Of course, it is subject to the usual anomalies of a
parallel branch-and-bound ([Gendron and Crainic
1994](Z-bibliography.md#ref-gendron1994)): a worker finding a good incumbent
early may let the others prune subtrees that the sequential search would enter
(and vice versa), and hence the speedups may be superlinear as well as below
one.

*Parameters.* [Table 16.3](16-change.md#tab-16-3) lists the parameters of
`BranchAndXSolver`; the inherited `intMaxIter` is left to the inner
`:Solver`s, since a node may take several of their iterations.

<span id="tab-16-3"></span>**Table 16.3.** The parameters of
`BranchAndXSolver`, with their defaults.

| Parameter | Default | Meaning |
|---|---|---|
| `intSolveMethod` | `BestFS` (2) | `DFS` (0), `BFS` (1), `BestFS` (2), `BestFSDive` (3), `BestEstimate` (4) |
| `intBoundingProtocol` | `Eager` (0) | `Eager`: a node is evaluated when created; `Lazy` (1): when extracted |
| `intThreadForDifferentSolvers` | 1 | threads over the inner `:Solver`s of a node (positive) |
| `intReoptimize` | 0 | > 0: the tree is retained across `compute()` calls (serial explorations) |
| `intMaxNodes` | `INT_MAX` | the node budget; $\leq 0$ means no limit |
| `intMaxThread` (inherited) | 0 | $K > 1$ with `DFS`: $K$ workers of the parallel exploration |
| `dblMaxTime` (inherited) | $+\infty$ | the time budget of the whole solve, in seconds |
| `dblRelAcc` (inherited) | $10^{-6}$ | $\varepsilon_r$ of the pruning |
| `dblAbsAcc` (inherited) | $+\infty$ (inactive) | $\varepsilon_a$ of the pruning |
| `strNameOfBlockSolverConfigurationFile` | `""` | the file of the `BlockSolverConfig` of the inner `:Solver`s |

A new `BlockSolverConfig` replaces the inner `:Solver`s of the previous one.
Giving `BranchAndXSolver` another `Block` drops all that belongs to the old
one (inner `:Solver`s, retained tree, best solution, and the pool of the
global cuts). A complete example is
the `ComputeConfig`
[`tests/BinaryKnapsackBlock/BX-baseCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/BinaryKnapsackBlock/BX-baseCfg.txt),
whose `BlockSolverConfig`
[`InnerBSCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/BinaryKnapsackBlock/InnerBSCfg.txt)
attaches a single `GreedyRelaxationBinaryKnapsackSolver`.

## 16.11 Branching rules and estimates {#sec-16-11}

[Source: [`SMS++/include/BranchingRule.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BranchingRule.h)]

The components of `BranchingRule.h` help a `:RelaxationSolver` choose the
variable to branch upon and estimate its children, knowing nothing of the
enumerative `Solver` nor of the relaxation. Each `:RelaxationSolver` describes
its candidates and decides which component to use (typically, by one of its
parameters), and it gives strong branching the way to evaluate the children of
a candidate. We call candidate (`BranchCandidate`) a variable fractional in a
point of the relaxation, which is described by its index for the
`:RelaxationSolver`, its value in that point and in a second one (possibly the
same), its cost in the objective of its block, its position among the
variables of the block, in $[0, 1]$, and the index of the block.

*Features.* `BranchingFeatures` sorts the candidates by nonincreasing
fractionality (stably), and gives each of them 8 features that do not depend
on the scale of the data. These are (i) its fractionality, (ii) its value in
the point of the candidates, (iii) its value in the second point, (iv) the
absolute difference of the two, (v) its cost over the largest absolute cost of
the candidates, (vi) its position in its block, (vii) the fraction of the
variables fixed at the node and (viii) its rank over the number of candidates
less one.

*Strong branching.* `StrongBranching` computes, by the function that the
`:RelaxationSolver` gives, the bounds $L^0_v$ and $L^1_v$ of the two children
of each candidate $v$. It scores the candidate by the product
$$
s_v = \max \{ L^0_v - L , \delta \} \cdot \max \{ L^1_v - L , \delta \} ,
\qquad \delta = 10^{-6} \max \{ | L | , 1 \} ,
$$
of the improvements over the bound $L$ of the node (with the sign of the sense
of the problem), a child found empty counting as an infinite improvement
([Achterberg 2007](Z-bibliography.md#ref-achterberg2007)). Ties go to
the first candidate, i.e., to the most fractional. It can also append to a CSV
file one line per candidate, out of which a rule can be learned.

*Learned branching.* `LearnedBranching` scores a candidate of features
$\phi_v$ by the perceptron
$\sigma(\phi_v) = w_2^\top \tanh( W_1 \phi_v + b_1 ) + b_2$ with one hidden
layer. Its model is
read from a text file that holds the line `LBRModel <nin> <nhid>`, followed by
the rows of $W_1$, the biases $b_1$, the weights $w_2$ and the bias $b_2$.

*Estimates.* A `:RelaxationSolver` that also derives from `BranchEstimator`
gives, right after each `branch()`, an estimate of the value of the best
solution in the region of each child (NaN for none), which the best-estimate
search uses ([§16.10](16-change.md#sec-16-10)); an estimate is no bound. In
`PseudoCosts` the estimates are those of the best-estimate search of [Bénichou
et al. 1971](Z-bibliography.md#ref-benichou1971). The pseudocost $\Psi^d_v$ of
the candidate $v$ in the direction $d$ (0 "down", 1 "up") is the mean, over
the branchings on $v$ observed so far, of the worsening of the bound from the
node to its child in that direction per unit of the distance $\delta^d_v$
moved by the value of $v$, i.e., $x_v - \lfloor x_v \rfloor$ down and $\lceil
x_v \rceil - x_v$ up. The estimate of the child in the direction $d$ of a node
of bound $L$ branched upon $b$ is then $L$ worsened by $$ \Psi^d_b \,
\delta^d_b + \sum_{v \neq b} \min \{ \Psi^0_v \, \delta^0_v , \Psi^1_v \,
\delta^1_v \} , $$ i.e., by the cheapest rounding of each fractional
candidate, that of $b$ being forced by the child. A candidate with no
observation in a direction takes its prior, if one is given, and otherwise the
mean of the pseudocosts of all the candidates in that direction (0 if there is
none). In `LearnedPseudoCosts` the priors come from two models of
`LearnedBranching` read from one file, the first for the "down" pseudocost and
the second for the "up" one, both relative to $\max \{ | L | , 1 \}$, a
negative output counting as 0.

## 16.12 The `:RelaxationSolver`s of the catalog {#sec-16-12}

We now describe in turn the `:RelaxationSolver`s in `develop`, whose
parameters are listed in [Table 16.4](16-change.md#tab-16-4); no heuristic
`:ChangeSolver` is in the catalog at present.

### `GreedyRelaxationBinaryKnapsackSolver`

[`GreedyRelaxationBinaryKnapsackSolver`](https://smspp.gitlab.io/smspp-project/d3/d0d/class_s_m_spp__di__unipi__it_1_1_greedy_relaxation_binary_knapsack_solver.html)
solves the continuous relaxation of a `BinaryKnapsackBlock` by the greedy
fractional fill along the items sorted by nonincreasing efficiency ([Dantzig
1957](Z-bibliography.md#ref-dantzig1957)). This fill also identifies the
*critical* item, the one that is fractional, and the greedy solution with the
critical item rounded away is feasible: it is the true solution of the
relaxation. The solver applies the fixings to its internal state, not to the
`Block`, and `branch()` gives two children that fix the critical
item to 0 and to 1. With `intIncremental` = 0 (the default) each `compute()`
resumes the greedy fill from the first item, in the efficiency order, that has
entered or left the relaxation since the last solve, finding the critical item
by a binary search on the part of the fill before it (since SMS++ 0.9.0;
before, from the first item). With `intIncremental` = 1 the greedy fill is
kept across the `Change`s: a fixing updates the residual capacity and the
profit of the fill, and the position from which the next `compute()` resumes
the search of the critical item, each child carrying the state of its fill.
Any `Modification` of the `Block`, instead, makes the next `compute()` rebuild
the fill from the data.

With `intIncremental` = 0, when there is an incumbent and `LocalFixingAllowed`
is true, each child also carries the reduced-cost pegging valid in its subtree
([Martello and Toth 1990](Z-bibliography.md#ref-martello1990)), in a
`GroupChange` with the fixing of the critical item (since SMS++ 0.9.0; in
0.8.0 the incumbent was read where nobody wrote it, and no item was pegged);
we justify it by the following statement.

<span id="prop-16-4"></span>**Proposition 16.4** (Reduced-cost pegging). *In
the normalized instance at a node (a maximization with positive data, the
fixed items removed and the capacity $C$ reduced accordingly), let $c$ be the
critical item, $\rho = p_c / w_c$ its efficiency, $U$ the value of the
continuous relaxation and $\bar{x}$ its solution. For each free item
$j \neq c$, every solution $x$ of the continuous relaxation with
$x_j = 1 - \bar{x}_j$ has value at most $U - | p_j - \rho w_j |$. Hence, if
$U - | p_j - \rho w_j | \leq \bar{z}$, fixing $x_j = \bar{x}_j$ in the subtree
loses no solution better than $\bar{z}$.*

*Proof.* Since $\rho \geq 0$ and $\sum_i w_i x_i \leq C$, each solution $x$ of
the continuous relaxation has
$$
\sum_i p_i x_i \leq \rho C + \sum_i ( p_i - \rho w_i ) x_i
\leq \rho C + \sum_{i \neq j} \max \{ 0 , p_i - \rho w_i \}
+ ( p_j - \rho w_j ) x_j .
$$
The greedy solution takes the items before $c$, for which
$p_i - \rho w_i \geq 0$, leaves out those after $c$, for which
$p_i - \rho w_i \leq 0$, and fills the capacity with $c$, whose term is 0;
hence $U = \rho C + \sum_i \max \{ 0 , p_i - \rho w_i \}$. If $\bar{x}_j = 1$,
then $p_j - \rho w_j \geq 0$, and $x_j = 0$ makes the last term 0, i.e.,
$\max \{ 0 , p_j - \rho w_j \} - | p_j - \rho w_j |$. If $\bar{x}_j = 0$, then
$p_j - \rho w_j \leq 0$, and $x_j = 1$ makes it
$p_j - \rho w_j = \max \{ 0 , p_j - \rho w_j \} - | p_j - \rho w_j |$. In both
cases the value of $x$ is at most $U - | p_j - \rho w_j |$, and the integer
solutions with $x_j = 1 - \bar{x}_j$ in the subtree are among these $x$.
$\square$

The pegging depends on the incumbent, and therefore it satisfies the covering
condition of [§16.8](16-change.md#sec-16-8) only in its weaker form; for this
reason it is made only while `LocalFixingAllowed` is true. In practice, the
test is guarded by a tolerance of $10^{-9}$ relative to the incumbent.
Also, no item is looked at when $U$ exceeds the incumbent by more than the
largest margin $| p_j - \rho w_j |$ of the items, nor in a block of
consecutive items whose largest margin is below the excess. With `intIncremental` = 1 there is
no pegging; however, the child that takes the critical item is not produced
when the item does not fit the capacity left by the fixed items, again only
while `LocalFixingAllowed` is true.

### `LagrangianDualRelaxationSolver`

[`LagrangianDualRelaxationSolver`](https://smspp.gitlab.io/smspp-project/d5/dc5/class_s_m_spp__di__unipi__it_1_1_lagrangian_dual_relaxation_solver.html)
solves at each node the Lagrangian Dual of the `Block`, with the primal
recovery of `PrimalProximalHeur`, which gives the true bounds and solution
([Chapter 14](14-lag-benders-bfunction.md#ch-14)); it is also a
`BranchEstimator`. Its `branch()` picks a binary variable of the sub-`Block`s
whose value in the convexified Lagrangian solution is fractional. It gives two
`LagrangianChange`s that identify the variable by its `AbstractPath` relative
to the `Block`, which needs the sub-`Block`s not to be copied into the
Lagrangian Dual (`int_LDSlv_iBCopy` = 0). Where the branching is applied is
decided by `intApplyStrategy`. With `eMaster` (the default) the bound of each
child, $x \leq \lfloor v \rfloor$ or $x \geq \lceil v \rceil$, becomes a new
dual pair of the `LagBFunction` of the sub-`Block` of the variable, i.e., it
is relaxed with a dedicated multiplier; a later bound on the same variable
changes the constant term of that pair. With `eSubproblem` the variable is
fixed in its sub-`Block` at $\lfloor v \rfloor$ or $\lceil v \rceil$. In this
case, the columns of the global pool of the `LagBFunction` that the fixing
makes infeasible are kept in a `Collection` of the `GlobalInformation`, which
this strategy needs, and they are given back to the `LagBFunction` when the
variable is unfixed. Fixing gives the same bound, as we now show: the two
strategies differ in the Lagrangian Dual that the inner `Solver` solves, not
in its optimal value, and the price of fixing is that the sub-`Block`s are
solved with the variable fixed, which their `:Solver`s have to support.

<span id="prop-16-5"></span>**Proposition 16.5** (Fixing in the sub-`Block`
against relaxing the bound). *Let the `Block` be
$\max \{ c x : G x \leq d ,\; x \in X_1 \times \cdots \times X_m \}$, with
finite sets $X_h$, whose constraints $G x \leq d$ are relaxed in the
Lagrangian Dual, and let a child of a node fix a binary variable $x_j$ of the
sub-`Block` $k$ at $b \in \{ 0, 1 \}$. Then the optimal value of the
Lagrangian Dual of the child with `eSubproblem` is equal to that with
`eMaster`.*

*Proof.* By [Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1) and the
remark after it, the optimal value of a Lagrangian Dual is that of the
convexified relaxation, with $\max \emptyset = -\infty$. With `eMaster` it is
$\max \{ c x : G x \leq d ,\; x \in Y \}$, where $Y$ is $\mathrm{conv}(X_1)
\times \cdots \times \mathrm{conv}(X_m)$ intersected with $\{ x_j \leq 0 \}$
for $b = 0$ or with $\{ x_j \geq 1 \}$ for $b = 1$. In both cases this is the
intersection with $\{ x_j = b \}$, since $0 \leq x_j \leq 1$ on
$\mathrm{conv}(X_k)$. With `eSubproblem` it is the same with
$\mathrm{conv}(X_k)$ replaced by $\mathrm{conv}(X_k \cap \{ x_j = b \})$,
which equals $\mathrm{conv}(X_k) \cap \{ x_j = b \}$, since this is a face of
$\mathrm{conv}(X_k)$ and the vertices of a face of the convex hull of a finite
set belong to the set; hence, the two problems coincide. $\square$

Of course, the statement concerns the exact optimal values, while the
Lagrangian Dual is solved only to the accuracy of its inner `Solver`. Each
child starts from the Lagrangian multipliers of its father when the inner
`Solver` supports `intRstAlg`. In fact, the `LagrangianChange`s of a branching
carry them (as the "center"), `apply()` queues them, and they are restored
just before the next inner solve, the multipliers added since being set to
zero. As for the statuses, `compute()` returns `kOK` when the convexified
solution is there and has a fractional variable among those that may be
branched upon. It also returns `kOK` when there is none and the node is solved
(the Lagrangian Dual solved and a feasible solution within `dblRelAcc` of its
bound), in which case `branch()` returns no `Change`. A node that can be
neither branched nor fenced gets `kLowPrecision` (or the status of the
Lagrangian Dual, if it has failed), and `kCutOff` is returned as it is (cf.
[Table 16.2](16-change.md#tab-16-2)).

The variable to branch upon is chosen by `intBranchStrategy` among the binary
variables of the groups that `vstrBranchGroups` names, all of them if it is
empty. An entry may also be `name[i]` or `name[i:j]`, and the entries are
priorities (since SMS++ 0.9.0), the candidates being those of the first entry
that has a fractional variable. Typically, a group whose variables are
determined by those of the others (e.g., start-up variables by the commitment
ones) need not be branched upon, and leaving it out also spares the `:Solver`
of the sub-`Block` fixings that it may not support. An entry with no variable
(no group of that name, or a range past the end of the group) is refused, by
an exception. The chosen variable is the most fractional (`eMostFractional`,
the default), or the best of the `intStrongCands` most fractional ones by
strong branching (`eStrongBranching`, whose data are appended to
`strStrongLog` if it is not empty), or the one that the model of
`strBranchModel` ranks first among the `intStrongCands` most fractional ones
(`eLearned`). With `intBranchEstimate` = 1 the solver gives the estimates of
the pseudocosts, with the priors of `strEstimateModel`
([§16.11](16-change.md#sec-16-11)); these are learned from the bound of each
child solved after its father branched, and from those of strong branching.
With `intHeurFreq` = $h$ (since SMS++ 0.9.0) the heuristic of
`PrimalProximalHeur` runs at the first `compute()` and then at one every $h$
of them (0 meaning at the first only). In the other calls only the Lagrangian
Dual of the original objective is solved, which gives the bound and the
convexified solution that the branching needs.

`LagrangianDualRelaxationSolverML` is in the library
`SMS++::LagrangianDualSolverML`, built only if Torch is found, and it adds
`intBranchStrategy` = `eOnline`. In the first `intLearnNodes` nodes it
branches by strong branching, keeping the features of their candidates and
their scores. Then it fits on those data the perceptron of `eLearned`, with
`intHidden` hidden units, by `intTrainEpochs` epochs of Adam with step
`dblLearnRate` on the listwise loss of [Cao et al.
2007](Z-bibliography.md#ref-cao2007), and from then on it branches on the
candidate that the model scores best. Both the data and the model are kept in
the `GlobalInformation`; thus, all the solvers of a search (e.g., those of the
workers of a parallel exploration) gather their data together and use the same
model, which the first of them to reach `intLearnNodes` nodes fits, while a
solver with no `GlobalInformation` keeps its own. Once fitted, the model can
be written to `strOnlineModel`, and used by `eLearned` in another run.

### `SATSolver`

Finally, the `SATSolver`s of `SATBlock` (one per SAT solver, CaDiCaL and
MiniSat) are `:RelaxationSolver`s as well, and they apply the
`SATBlockChange`s to the `Block`. A node is a set of fixed variables, which
the SAT solver sees as assumptions; hence, the cores found below a node keep
holding while its fixings are there. With `intMaxIter` set, the weighted
MaxSAT algorithm stops after that many calls of the SAT solver in its main
loop, but in a node whose variables are all fixed, and its relaxation is that
of the cores found so far. Then `compute()` returns `kOK`, with `get_lb()`
that bound and `get_ub()` the value of the best solution found, and the node
is closed when the two meet. Its `branch()` fixes the unfixed variable that
appears in the most soft assumptions of the cores, the first child taking the
value it has in the best solution found. If `strBranchRule` names a
`SATBranchRule` (which reads the file `strBranchRuleFile`), this rule chooses
first; an unknown name of `strBranchRule` is an error.

<span id="tab-16-4"></span>**Table 16.4.** The `:RelaxationSolver`s of the
catalog and their parameters, with their defaults.

| `:RelaxationSolver` | Parameter | Default |
|---|---|---|
| `GreedyRelaxationBinaryKnapsackSolver` | `intIncremental` | 0 |
| `LagrangianDualRelaxationSolver` | `intApplyStrategy` | `eMaster` (0); `eSubproblem` (1) |
| | `intBranchStrategy` | `eMostFractional` (0); `eStrongBranching` (1), `eLearned` (2), `eOnline` (3) |
| | `intStrongCands` | 10 |
| | `intBranchEstimate` | 0 |
| | `intHeurFreq` | 1 |
| | `strStrongLog`, `strBranchModel`, `strEstimateModel` | `""` |
| | `vstrBranchGroups` | empty (all the groups) |
| `LagrangianDualRelaxationSolverML` | `intLearnNodes`, `intTrainEpochs`, `intHidden`, `intLearnSeed` | 20, 300, 16, 0 |
| | `dblLearnRate` | $10^{-2}$ |
| | `strOnlineModel` | `""` |
| `SATSolver` | `strBranchRule`, `strBranchRuleFile` | `""` |

## 16.13 Notes and references {#sec-16-13}

The B&X of `BranchAndXSolver` is a branch-and-bound in the sense of the
classical survey [Lawler and Wood 1966](Z-bibliography.md#ref-lawler1966),
and we refer to [Morrison et al. 2016](Z-bibliography.md#ref-morrison2016)
for a survey of the strategies of search, branching and pruning that it
offers. In `BranchAndXSolver`, however, the relaxation, the branching and the
tightenings are left to the `:RelaxationSolver`s, which communicate with the
driver only through `Change`s and a `GlobalInformation`. For the parallel
exploration we follow the synthesis of [Gendron and Crainic
1994](Z-bibliography.md#ref-gendron1994), and its workers claim the subtrees
in the order that [Archibald et al.
2018](Z-bibliography.md#ref-archibald2018) show to give replicable runs. As
for the relaxations, the continuous relaxation of the knapsack problem is due
to [Dantzig 1957](Z-bibliography.md#ref-dantzig1957), and the reduced-cost
pegging of [Proposition 16.4](16-change.md#prop-16-4) is described in
[Martello and Toth 1990](Z-bibliography.md#ref-martello1990). Pseudocosts
and the best-estimate search come from [Bénichou et al.
1971](Z-bibliography.md#ref-benichou1971), strong branching is studied in
[Achterberg et al. 2005](Z-bibliography.md#ref-achterberg2005), and the
product score of its candidates comes from [Achterberg
2007](Z-bibliography.md#ref-achterberg2007). Learning a branching rule from
the data of strong branching is proposed in [Khalil et al.
2016](Z-bibliography.md#ref-khalil2016), and the listwise loss used to fit it
online in [Cao et al. 2007](Z-bibliography.md#ref-cao2007). Finally, the
convexification argument behind [Proposition 16.5](16-change.md#prop-16-5) is
that of [Geoffrion 1974](Z-bibliography.md#ref-geoffrion1974).
