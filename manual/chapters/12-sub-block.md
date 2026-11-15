# 12. Sub-Block and the recursive flow of Modifications {#ch-12}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h);
[`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h),
[`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h)]

[Chapter 4](04-block.md#ch-4) introduced the `Block` tree in the
abstract, [Chapter 5](05-variable-constraint-objective.md#ch-5) fixed
the composition rule for `Objective`s, and
[Chapter 8](08-modification-janus.md#ch-8) described how
`Modification`s travel. We now put these together into the picture of
a recursive tree of `Block`s, and state its consequences for the data,
for the solving, and for concurrency.

## 12.1 The recursive Block tree {#sec-12-1}

A `Block` may contain any number of sub-`Block`s, each of which may
in turn contain its own sub-`Block`s, to any depth. The structure
is a tree: every `Block` has at most one father (the root has
none), and the sub-`Block`s of a given `Block` are held in its
`v_Block` vector of pointers ([§4.4](04-block.md#sec-4-4)).

The defining restriction ([§4.3](04-block.md#sec-4-3)) is that
sub-`Block`s are always *static*: there is no `Modification` that adds
or removes a sub-`Block`, analogous to those of the dynamic `Variable`s
and `Constraint`s. The tree is built with the `Block`, or chosen by
`set_structure()` when it is a modelling choice
([§11.3](11-configuration.md#sec-11-3)), or grown when the abstract
representation is generated (as `CapacitatedFacilityLocationBlock`
does in KskForm, [§12.5](12-sub-block.md#sec-12-5)); all this is
meant to happen before any `Solver` is attached. Changing it afterwards
is the "nuclear option" of an `NBModification`, after which nothing that
a `Solver` knows of the `Block` is worth keeping. The restriction costs
little in expressiveness (a model whose number of components varies is
usually better expressed with a fixed set of sub-`Block`s, some of
which are "switched off"), and it simplifies the bookkeeping of the
framework, in particular the locking and the routing of the
`Modification`s described below.

## 12.2 How a model is split across the tree {#sec-12-2}

The point of the tree is that a structured model is *decomposed*
across it: each sub-`Block` carries the part of the model that is
local to it, and the parent carries only what *links* its
sub-`Block`s together.

Two composition rules, already stated in earlier chapters, make
the tree add up to a single coherent model:

- the `Variable`s of a sub-`Block` are, conceptually, also
  `Variable`s of the parent ([§4.1](04-block.md#sec-4-1));
- the `Objective`s sum up the tree ([§5.4](05-variable-constraint-objective.md#sec-5-4)): the overall objective
  of a `Block` is its own `Objective` plus the `Objective`s of all
  its sub-`Block`s, recursively. A parent that carries no
  `Objective` of its own still has a well-defined overall
  objective, made up entirely of those of its sub-`Block`s.

Both rules can be qualified by a *weight*. A `Block` has a weight
of its `Objective` (`set_objective_weight()`, 1 by default), by
which its `Objective` counts in any problem it is part of (e.g.,
the probability of the scenario it describes times the discount
factor of its period). It also has a weight of its `Variable`s
(`set_variable_weight()`), by which the coefficients of its
`Variable`s, and of those of its sub-`Block`s, are multiplied in
the `Constraint`s outside of it (e.g., the number of identical
copies of a unit), and `set_weight()` sets both, i.e., scales the
whole `Block`. The `Block` itself is not changed: a `:Solver`
attached to an ancestor multiplies the `Objective` of the `Block`
by the product of the `Objective` weights along the path up to the
ancestor, the latter excluded (`get_objective_weight( ancestor )`),
and similarly for the `Variable` weights
(`get_variable_weight( cblock )`), while a `:Solver` attached to the
`Block` does not see its weight. Weights are finite and
non-negative (0 makes the `Block` count for nothing, and
`std::invalid_argument` is thrown otherwise). A change of them issues a
`BlockWeightMod`, and it is refused with `std::logic_error`, before
anything changes, if a `:Solver` attached to an ancestor does not handle
it (`Solver::handles_weight_change()`, `false` by default). The
ancestors are looked at up to the first one that is a `Function` (a
`LagBFunction` or a `BendersBFunction` of which the `Block` is the inner
`Block`, which tells those above it of the change as a change of
itself). The signatures are

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include "BinaryKnapsackBlock.h"
#include "BlockSolverConfig.h"

#include <fstream>

using namespace SMSpp_di_unipi_it;

int main( int argc , char ** argv )
{
 CapacitatedFacilityLocationBlock cfl;
 std::ifstream in( argv[ 1 ] );  // a CFL instance in ORLib format
 cfl.load( in );

 // the Knapsack Formulation: one BinaryKnapsackBlock per facility
 auto bc = dynamic_cast< BlockConfig * >(
   Configuration::deserialize( "CFLBCfg-KF-LD.txt" ) );
 bc->apply( &cfl );
 delete bc;
 cfl.generate_abstract_variables();    // grows the sub-Block tree
 cfl.generate_abstract_constraints();  // the linking constraints
 cfl.generate_objective();

 // the tree is now populated: navigate it
 const auto nf = cfl.get_number_nested_Blocks();  // == get_NFacilities()
 for( Block::Index i = 0 ; i < nf ; ++i ) {
  auto bk = static_cast< BinaryKnapsackBlock * >( cfl.get_nested_Block( i ) );
  std::cout << "facility " << i << ": " << bk->get_NItems() << " items"
            << std::endl;
  }

 // a Solver at the root
 auto bsc = dynamic_cast< BlockSolverConfig * >(
   Configuration::deserialize( "BSPar-LDS.txt" ) );
 bsc->apply( &cfl );
 bsc->clear();

 // a change in a leaf sub-Block reaches the Solver of the root: the
 // BinaryKnapsackBlockMod issued by bk0 goes up to cfl (Lemma 12.1)
 auto bk0 = static_cast< BinaryKnapsackBlock * >( cfl.get_nested_Block( 0 ) );
 bk0->chg_weight( 5.0 , 2 );

 bsc->apply( &cfl );
 delete bsc;
 return( 0 );
 }
```

where `get_objective_weight( nullptr )` and
`get_variable_weight( nullptr )` multiply up to the root, included, and
`get_variable_weight( cblock )` stops at the closest ancestor that is
also `cblock` or an ancestor of it (it is 1 if the `Block` is `cblock`
or one of its descendants, since a `Constraint` of a `Block` is not
outside of it); both throw `std::invalid_argument` if the two `Block`s
are not in the relation required.

What may, at first, look like a third rule ("a `Constraint` lives
in the parent and links the `Variable`s of its children") is in
fact only the most common pattern, and the framework does not impose
it; the actual rule is more permissive:

> a `Constraint` defined anywhere in the `Block` tree may
> reference `Variable`s defined anywhere in the tree.

Compositionality makes one *expect* the references to follow
inclusion (`Variable`s of two different sub-`Block`s coupled by a
`Constraint` placed in their common parent), and a well-designed
`:Block` library will largely follow that expectation, because it
keeps each `Block` understandable in isolation. But it is not a
hard requirement. A `Constraint` in the parent may legitimately
involve `Variable`s of the parent together with `Variable`s of a
child; and a `Constraint` *in a child* may even reference a
`Variable` of the parent. The latter happens, for instance,
with the linearized representation of a `PolyhedralFunctionBlock`,
whose rows $v \ge a_i x + b_i$ are written on `Variable`s $x$ of
another `Block`, and with a `DCNetworkBlock` nested in a
`DesignNetworkBlock`, whose rows use the design `Variable`s of the
latter (`set_design_variables()`). Such cross-references require
specific support (the sub-`Block` exposes a setter for the "external"
`Variable`, so that the parent, or whoever builds the model, can wire
it in), but they are possible and sometimes the natural way to express
a problem.

The data of the problem is split accordingly: data that belongs to
one component lives in the corresponding sub-`Block`; data that
expresses the coupling lives wherever the modeller chose to place
the linking `Constraint`s, most often (but not necessarily) in the
common parent.

## 12.3 The recursive flow of Modifications {#sec-12-3}

A `:Solver` attached to a `Block` is responsible for solving the
*entire* sub-tree rooted at that `Block` ([§6.1](06-solver.md#sec-6-1)). It follows that
such a `:Solver` must be told about changes occurring *anywhere*
in that sub-tree, not only in the `Block` it is directly attached
to.

This is what the framework does: `Block::add_Modification( mod , chnl )`
first hands the `Modification` to the father, which does the same,
recursively up to the root, and then gives it to each `:Solver`
registered to the `Block` whose `concerned_by()` says that it reads
that kind of `Modification` ([`Block.cpp:769-814`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L769-814)). The "anyone
listening" flag `f_at` ([§8.7](08-modification-janus.md#sec-8-7))
records, at each `Block`, whether some ancestor has a `:Solver`
registered, and `add_Modification()` returns at once if neither
`f_at` holds nor the `Block` has a `:Solver` of its own
(`anyone_there()`), so that a `Block` whose sub-tree nobody is solving
does not waste effort. The result is the following.

**Lemma 12.1.** *Assume the invariant (I): for every `Block` $C$,
`f_at` of $C$ is true if and only if some proper ancestor of $C$ has a
registered `Solver`. Let a `Modification` $\mu$ be passed to
`add_Modification()` of a `Block` $B$ on the default channel, and let
each `Solver` be registered to at most one `Block`. Then $\mu$ is given
exactly once to each `Solver` $S$ registered to $B$ or to an ancestor of
$B$ such that $\mu$ is of concern to `S->concerned_by()`, and to no
other `Solver`; the `Solver`s of an ancestor receive it before those of
its descendants.*

*Proof.* By induction on the depth $d$ of $B$ in its tree. Let $P$ be
the path from $B$ to the root. If `anyone_there()` is false at $B$,
then $B$ has no `Solver` and, by (I), no proper ancestor has one, so the
set of the statement is empty, and indeed $\mu$ is given to nobody.
Otherwise, $B$ passes $\mu$ to its father $F$, if any, on the same
default channel; by the inductive hypothesis applied to $F$ (depth
$d - 1$) this gives $\mu$ exactly once to each concerned `Solver` of the
Blocks of $P$ other than $B$, ancestors first (if `anyone_there()` is
false at $F$, by (I) and the definition there are none, and the
hypothesis holds trivially). Then $B$ gives $\mu$ to each of its own
concerned `Solver`s, once since `register_Solver()` does not register a
`Solver` twice. No other `Solver` is reached, since each `Block` only
gives $\mu$ to its own `Solver`s and only the Blocks of $P$ are visited,
each once. $\square$

The invariant (I) is maintained by `register_Solver()` and
`unregister_Solver[s]()`, which set `f_at` in the sub-tree when the
first `Solver` of a `Block` arrives and reset it when the last one
leaves (if nobody listens from above), and by `add_nested_Block()`,
which gives a new sub-`Block` the `f_at` of the place it lands in. On a
channel opened at an ancestor $A$ ([§8.6](08-modification-janus.md#sec-8-6))
the walk changes in one place, since $A$ adds $\mu$ to the
`GroupModification` of the channel and stops. Hence, the `Solver`s of $A$ and of its
ancestors receive $\mu$ inside that `GroupModification` when the channel
is closed, while those of the Blocks strictly between $B$ and $A$, and
of $B$, still receive it at once.

Conversely, by Lemma 12.1 a `:Solver` attached to a sub-`Block` is not
told about changes happening in the parent or in sibling sub-`Block`s
(they are above or outside its sub-tree,
[§6.1](06-solver.md#sec-6-1)). This is what makes it sound to attach a specialised `:Solver` to a single leaf
sub-`Block` while a different `:Solver` solves the whole tree from
the root: each sees exactly the changes relevant to its own
responsibility.

## 12.4 Locking propagates down the tree {#sec-12-4}

Concurrency control ([Chapter 17](17-parallel.md#ch-17)) is also
recursive. Acquiring a write lock on a `Block` with
`lock( const void * owner )` locks all of its sub-`Block`s, recursively
(`lock_sub_block()`), and fails, releasing what it had taken, if one of
them cannot be locked; the analogous `read_lock()` and
`read_lock_sub_block()` acquire shared read locks down the tree. The rationale is the same as for `Modification`
routing: a `:Solver` operating on a `Block` operates on its whole
sub-tree, so to obtain exclusive access it must lock the whole
sub-tree, not just the root of it.

This recursive locking is also what underpins the immediacy
guarantee of [§8.4](08-modification-janus.md#sec-8-4): an abstract change to a `Block` is performed
while the `Block` is locked, hence while its entire sub-tree is
locked, so no concurrent mutation can be interleaved between the
change and the `Block`'s reaction to it.

## 12.5 Example: CFL in the Knapsack Formulation {#sec-12-5}

The Knapsack Formulation (KskForm) of
`CapacitatedFacilityLocationBlock` is the canonical small example
of a non-trivial tree. Selected by setting
`f_static_variables_Configuration` to a `SimpleConfiguration< int >`
of value 1 ([§11.8](11-configuration.md#sec-11-8)), it makes the CFL
`Block` grow, in `generate_abstract_variables()`, `f_n_facilities`
sub-`Block`s, each a
[`BinaryKnapsackBlock`](https://smspp.gitlab.io/smspp-project/dc/d2f/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block.html),
one per facility, and the split is the one described in
[§12.2](12-sub-block.md#sec-12-2):

- the sub-`Block` of facility $i$ has $n + 1$ `Variable`s, the
  transportation `Variable`s $X_{ij}$, $j \in J$, in the natural order,
  and last the design `Variable` $Y_i$, and its knapsack constraint
  is the capacity constraint $\sum_j D_j X_{ij} - Q_i Y_i \le 0$ of
  the facility;
- the parent
  [`CapacitatedFacilityLocationBlock`](https://smspp.gitlab.io/smspp-project/dc/d34/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_block.html)
  carries only the customer-satisfaction *linking* constraints
  $\sum_i X_{ij} = 1$, which couple `Variable`s living in
  different sub-`Block`s;
- the overall objective is the sum of the knapsack objectives
  $\sum_j C_{ij} X_{ij} + F_i Y_i$ of the facilities (the
  `FRealObjective` of the parent has a `LinearFunction` with no
  `Variable` in this formulation).

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include "BinaryKnapsackBlock.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 CapacitatedFacilityLocationBlock cfl;
 cfl.load( /* ... a CFL instance ... */ );

 // select the Knapsack Formulation: the Block will grow one
 // BinaryKnapsackBlock sub-Block per facility
 auto * bc = new BlockConfig;
 bc->f_static_variables_Configuration = new SimpleConfiguration< int >( 1 );
 cfl.set_BlockConfig( bc );
 cfl.generate_abstract_variables();    // builds the sub-Block tree
 cfl.generate_abstract_constraints();  // builds the linking constraints
 cfl.generate_objective();

 // the tree is now populated: navigate it
 const auto nf = cfl.get_number_nested_Blocks();  // == f_n_facilities
 for( Block::Index i = 0 ; i < nf ; ++i ) {
  auto * bk =
    dynamic_cast< BinaryKnapsackBlock * >( cfl.get_nested_Block( i ) );
  // bk is the knapsack sub-Block for facility i
  }

 // a change in a leaf sub-Block reaches a Solver attached at the root:
 // (suppose a Solver has been registered on cfl)
 auto * bk0 =
   dynamic_cast< BinaryKnapsackBlock * >( cfl.get_nested_Block( 0 ) );
 bk0->chg_weight( /* new weight */ 5.0 , /* item */ 2 , eModBlck );
 // the BinaryKnapsackBlockMod issued by bk0 travels up to cfl and is
 // delivered to every Solver registered on cfl (the root of bk0's tree)

 return( 0 );
 }
```

The example is the configuration that
[Recipe R4](R4-cfl-lagrangian.md#rec-R4) solves with a
`LagrangianDualSolver`: the `:Solver` attached to the root `cfl`
dualises the linking constraints, leaving the per-facility
knapsacks as independent sub-problems, each solvable by a
`DPBinaryKnapsackSolver` attached to its sub-`Block`. The
recursive flow of the `Modification`s (Lemma 12.1) keeps that
arrangement correct, since a change of the data of a facility,
wherever it is made, reaches the `:Solver` of the root. Note that
`chg_weight()` has `eNoBlck` as default for both its parameters, which
issues the `Modification` when somebody listens, as here.

## 12.6 Idioms {#sec-12-6}

*Coupling in the parent, locality in the children.* As
[§12.2](12-sub-block.md#sec-12-2) made precise, the framework imposes no
restriction on where a `Constraint` lives relative to the `Variable`s it
references; there is, however, a strong expectation born of logical
composability. A sub-`Block` that is self-contained (referencing
external `Variable`s only through explicit, supported setters, and only
where the model requires it) stays understandable in isolation and
reusable across contexts, whereas one that "reaches sideways" into a
sibling for no compelling reason makes both `Block`s harder to reason
about and to reuse. Place linking
`Constraint`s where they make the decomposition clearest; that is
usually, but not always, the common parent.

*The `:Solver` of a sub-tree at its root.* Since `Modification`s flow
upward (Lemma 12.1), a `:Solver` that
must account for the whole model is attached at the root; a
specialised `:Solver` for one component is attached at that
component's sub-`Block`. The two coexist precisely because each is
told only about the changes within its own responsibility.

*No dynamic sub-`Block`s.* If the number of components must vary, model the maximum set of sub-`Block`s once
and switch components on and off through their data (fixing
`Variable`s, relaxing `Constraint`s), rather than trying to add or
remove sub-`Block`s at runtime.
