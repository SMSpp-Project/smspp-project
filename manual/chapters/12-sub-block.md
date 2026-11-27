# 12. Sub-Block and the recursive flow of Modifications {#ch-12}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h);
[`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h),
[`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h)]

We are concerned in this chapter with the tree of `Block`s, i.e., with a
model split into nested sub-`Block`s, and with the consequences of this
structure for the data, for the solution and for concurrency.
[Chapter 4](04-block.md#ch-4) introduced the tree in the abstract,
[Chapter 5](05-variable-constraint-objective.md#ch-5) fixed the rule by
which the `Objective`s compose, and [Chapter
8](08-modification-janus.md#ch-8) described how the `Modification`s travel;
here we put these together.

We first recall that the tree is static ([§12.1](12-sub-block.md#sec-12-1)),
and we describe how a model is split across it, with the weights by which a
`Block` counts in the problems it is part of
([§12.2](12-sub-block.md#sec-12-2)). Then we recall that a `Modification`
reaches the `Solver`s of all the ancestors of the `Block` that issues it, and
only those, in which order ([§12.3](12-sub-block.md#sec-12-3)), and we show
that locking follows the tree ([§12.4](12-sub-block.md#sec-12-4)). The
knapsack formulation of the CFL illustrates all this
([§12.5](12-sub-block.md#sec-12-5)), and the idioms close the chapter
([§12.6](12-sub-block.md#sec-12-6)).

## 12.1 The recursive Block tree {#sec-12-1}

A `Block` may contain any number of sub-`Block`s, each of which may in turn
contain further sub-`Block`s, to any depth. The structure is a tree, since a
`Block` has at most one father (the root has none), and the sub-`Block`s of a
`Block` are held in its vector of pointers `v_Block`
([§4.4](04-block.md#sec-4-4)).

The defining restriction ([§4.3](04-block.md#sec-4-3)) is that
sub-`Block`s are always *static*: there is no `Modification` that adds
or removes a sub-`Block`, analogous to those of the dynamic `Variable`s
and `Constraint`s. The tree is built with the `Block`, or chosen by
`set_structure()` when it is a modeling choice
([§11.3](11-configuration.md#sec-11-3)), or grown when the abstract
representation is generated (as `CapacitatedFacilityLocationBlock`
does in KskForm, [§12.5](12-sub-block.md#sec-12-5)), and all this is
meant to happen before any `Solver` is attached. Changing the tree
afterwards is the "nuclear option" of an `NBModification`, after which a
`Solver` can keep none of what it knows of the `Block`. This restriction
costs rather little in expressiveness, since a model whose number of
components varies is usually better expressed with a fixed set of
sub-`Block`s, some of which are "switched off". It also simplifies the
bookkeeping of the framework, in particular the locking and the routing of
the `Modification`s described below.

## 12.2 How a model is split across the tree {#sec-12-2}

A structured model is *decomposed* across the tree: each sub-`Block`
carries the part of the model that is local to it, and the parent carries
only what *links* its sub-`Block`s together. Two rules of composition, stated
in earlier chapters, make the tree add up to a single model:

- the `Variable`s of a sub-`Block` are, conceptually, also
  `Variable`s of the parent ([§4.1](04-block.md#sec-4-1));
- the `Objective`s sum up the tree
  ([§5.4](05-variable-constraint-objective.md#sec-5-4)), i.e., the overall
  objective of a `Block` is its `Objective` plus those of all its
  sub-`Block`s, recursively. Hence, a parent with no `Objective` still has a
  well-defined overall objective, made of those of its sub-`Block`s.

Both rules can be qualified by a *weight*. A `Block` has a weight
of its `Objective` (`set_objective_weight()`, 1 by default), by
which its `Objective` counts in any problem it is part of (e.g.,
the probability of the scenario it describes times the discount
factor of its period). It also has a weight of its `Variable`s
(`set_variable_weight()`), by which the coefficients of its
`Variable`s, and of those of its sub-`Block`s, are multiplied in
the `Constraint`s outside of it (e.g., the number of identical
copies of a unit), and `set_weight()` sets both, i.e., it scales the
`Block`. The weights leave the data of the `Block` unchanged: a `:Solver`
attached to an ancestor multiplies the `Objective` of the `Block` by the
product of the `Objective` weights along the path up to the ancestor, the
latter excluded (`get_objective_weight( ancestor )`), and similarly for the
`Variable` weights (`get_variable_weight( cblock )`). A `:Solver` attached
to the `Block`, instead, does not see its weight. Weights are finite and
nonnegative (with 0 the `Block` does not count at all), and
`std::invalid_argument` is thrown otherwise. A change of a weight issues a
`BlockWeightMod`, and it is refused with `std::logic_error`, before anything
changes, if a `:Solver` attached to an ancestor does not handle it
(`Solver::handles_weight_change()`, `false` by default). The ancestors are
examined up to the first one that is a `Function`, i.e., a `LagBFunction` or
a `BendersBFunction` of which the `Block` is the inner `Block`, which reports
the change to those above it as a change of the `Function`. The signatures
are

```cpp
// Block
void set_objective_weight( double w , c_ModParam issueMod = eModBlck );
void set_variable_weight( double s , c_ModParam issueMod = eModBlck );
virtual void set_weight( double k , c_ModParam issueMod = eModBlck );
double get_objective_weight( void ) const;
double get_objective_weight( const Block * ancestor ) const;
double get_variable_weight( void ) const;
double get_variable_weight( const Block * cblock ) const;

// Solver
virtual bool handles_weight_change( void ) const;
```

where `get_objective_weight( nullptr )` and
`get_variable_weight( nullptr )` multiply up to the root, included, and
`get_variable_weight( cblock )` stops at the closest ancestor that is
also `cblock` or an ancestor of it (it is 1 if the `Block` is `cblock`
or one of its descendants, since a `Constraint` of a `Block` is not
outside of it). Both throw `std::invalid_argument` if the two `Block`s
are not in the relation required.

At first, there may seem to be a third rule ("a `Constraint` lives in the
parent and links the `Variable`s of its children"); yet, this is only the most
common pattern, which the framework does not impose, and the actual rule is
more permissive:

> a `Constraint` defined anywhere in the `Block` tree may
> reference `Variable`s defined anywhere in the tree.

One *expects* the references to follow inclusion (`Variable`s of two
sub-`Block`s coupled by a `Constraint` placed in their common parent), and a
well-designed library of `:Block`s largely follows that expectation, since it
keeps each `Block` understandable in isolation; however, it is not a
requirement. A `Constraint` in the parent may legitimately
involve `Variable`s of the parent together with `Variable`s of a
child; and a `Constraint` *in a child* may even reference a
`Variable` of the parent. The latter happens, for instance,
with the linearized representation of a `PolyhedralFunctionBlock`,
whose rows $v \ge a_i x + b_i$ are written on `Variable`s $x$ of
another `Block`, and with a `DCNetworkBlock` nested in a
`DesignNetworkBlock`, whose rows use the design `Variable`s of the
latter (`set_design_variables()`). Such cross-references require
specific support (the sub-`Block` exposes a setter for the "external"
`Variable`, by which the parent, or whoever builds the model, wires it in),
but they are possible, and sometimes they are the natural way to express a
problem.

The data of the problem is split accordingly: the data of a component live
in its sub-`Block`, while those of the coupling live where the linking
`Constraint`s have been placed, most often (but not necessarily) in the
common parent.

## 12.3 The recursive flow of Modifications {#sec-12-3}

A `:Solver` attached to a `Block` is responsible for solving the *entire*
sub-tree rooted at that `Block` ([§6.1](06-solver.md#sec-6-1)), and hence it
must be informed of the changes occurring *anywhere* in that sub-tree, not
only in the `Block` to which it is attached. In fact,
`Block::add_Modification( mod , chnl )` first hands the `Modification` to the
father, which does the same, recursively up to the root, and then gives it to
each `:Solver` registered to the `Block` whose `concerned_by()` accepts that
kind of `Modification`
([`Block.cpp:777-823`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L777-823)).
The "anyone listening" flag `f_at` ([§4.6](04-block.md#sec-4-6), [Lemma
4.1](04-block.md#lem-4-1)) records, at each `Block`, whether some ancestor
has a registered `:Solver`. This flag lets `add_Modification()` return at
once if neither `f_at` holds nor the `Block` has a `:Solver` (cf.
`anyone_there()`), and therefore a `Block` whose sub-tree nobody is solving
does not waste effort. By
[Lemma 8.1](08-modification-janus.md#lem-8-1), a `Modification` issued in a
`Block` $B$ on the default channel reaches exactly once each concerned
`Solver` of $B$ and of its ancestors, and no other; since the base method
forwards it to the father before handing it to its own `Solver`s, those of an
ancestor receive it first. On a channel opened at an ancestor $A$
([§8.6](08-modification-janus.md#sec-8-6)) the walk changes in one place,
since $A$ adds the `Modification` to the `GroupModification` of the channel
and stops. Hence, the `Solver`s of $A$ and of its ancestors receive it inside
that `GroupModification` when the channel is closed, while those of the
`Block`s strictly between $B$ and $A$, and of $B$, still receive it at once.

Conversely, by [Lemma 8.1](08-modification-janus.md#lem-8-1) a `:Solver`
attached to a sub-`Block` is not informed of the changes in the parent or in
the sibling sub-`Block`s, which are above or outside its sub-tree
([§6.1](06-solver.md#sec-6-1)). Hence, one can attach a specialized `:Solver`
to a single leaf sub-`Block` while another `:Solver` solves the tree from the
root, since each of them receives the changes relevant to the part it is
responsible for, and only those.

## 12.4 Locking propagates down the tree {#sec-12-4}

Concurrency control ([Chapter 17](17-parallel.md#ch-17)) is also recursive.
Acquiring a write lock on a `Block` with `lock( const void * owner )` locks
all its sub-`Block`s, recursively (`lock_sub_block()`). If one of them is
owned by another thread, `lock()` sleeps until it is unlocked, keeping the
sub-`Block`s it has already taken, and it fails, releasing them, if one cannot
be locked by the calling thread; locking therefore proceeds top-down, and it
cannot deadlock as long as an entity owning a sub-`Block` never tries to own a
`Block` that includes it ([§17.1](17-parallel.md#sec-17-1)). Similarly,
`read_lock()` and `read_lock_sub_block()` acquire shared read locks down the
tree. The reason is the same as for the routing of the `Modification`s: a
`:Solver` operating on a `Block` operates on all its sub-tree, and to obtain
exclusive access it must lock all of it, not just its root.

The recursive locking also underpins the immediacy guarantee of
[§8.4](08-modification-janus.md#sec-8-4). If an abstract change of a `Block`
is made by a thread that holds the lock of the `Block` (which the code does
not take on its behalf, [§17.1](17-parallel.md#sec-17-1)), its entire
sub-tree is locked, and no concurrent change can be interleaved between the
change and the reaction of the `Block` to it.

## 12.5 Example: CFL in the knapsack formulation {#sec-12-5}

The knapsack formulation (KskForm) of `CapacitatedFacilityLocationBlock` is
the canonical small example of a non-trivial tree. It is selected by setting
`f_static_variables_Configuration` to a `SimpleConfiguration< int >` of value
1 ([§11.8](11-configuration.md#sec-11-8)), and it makes the CFL `Block` grow,
in `generate_abstract_variables()`, one sub-`Block` per facility, each a
[`BinaryKnapsackBlock`](https://smspp.gitlab.io/smspp-project/dc/d2f/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block.html).
The sub-`Block`s can also be built earlier, before any abstract
representation, by `set_structure()` ([§11.3](11-configuration.md#sec-11-3)),
if the structure `Configuration` of the `BlockConfig` is a
`SimpleConfiguration< int >` whose value selects the knapsack formulation in
the same encoding. This lets whoever reads the tree see them at once, e.g., a
`LagrangianDualSolver` that decomposes the `Block` recursively. In both cases,
the split is the one described in [§12.2](12-sub-block.md#sec-12-2):

- the sub-`Block` of facility $i$ has $n + 1$ `Variable`s, the transportation
  `Variable`s $X_{ij}$, $j \in J$, in the natural order, and last the design
  `Variable` $Y_i$, and its knapsack constraint is the capacity constraint
  $\sum_j D_j X_{ij} - Q_i Y_i \le 0$ of the facility.
- the parent
  [`CapacitatedFacilityLocationBlock`](https://smspp.gitlab.io/smspp-project/dc/d34/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_block.html)
  carries only the customer-satisfaction *linking* constraints $\sum_i X_{ij}
  = 1$, which couple `Variable`s living in different sub-`Block`s;
- the overall objective is the sum of the knapsack objectives $\sum_j C_{ij}
  X_{ij} + F_i Y_i$ of the facilities (the `FRealObjective` of the parent has
  a `LinearFunction` with no `Variable` in this formulation).

The following fragment builds the tree out of the configuration files of the
CFL test, attaches a `LagrangianDualSolver` to its root, and changes the
data of a leaf.

```cpp
CapacitatedFacilityLocationBlock cfl;
std::ifstream in( "cap41.txt" );  // a CFL instance in ORLib format
cfl.load( in );

// the knapsack formulation: one BinaryKnapsackBlock per facility
auto bc = dynamic_cast< BlockConfig * >(
                     Configuration::deserialize( "CFLBCfg-KF-LD.txt" ) );
bc->apply( &cfl );
delete bc;
cfl.generate_abstract_variables();    // grows the sub-Block tree
cfl.generate_abstract_constraints();  // the linking constraints
cfl.generate_objective();

// a LagrangianDualSolver at the root
auto bsc = dynamic_cast< BlockSolverConfig * >(
                     Configuration::deserialize( "BSPar-LDS.txt" ) );
bsc->apply( &cfl );
bsc->clear();

// a change in a leaf sub-Block reaches the Solver of the root (Lemma 8.1):
// the LagrangianDualSolver keeps the Modification of bk0 aside, and gives
// it to the LagBFunction of bk0 at its next compute()
auto bk0 = static_cast< BinaryKnapsackBlock * >( cfl.get_nested_Block( 0 ) );
bk0->chg_weight( 5.0 , 2 );

bsc->apply( &cfl );  // a clear()-ed one unregisters and deletes the Solver
delete bsc;
```

This is, up to the `R3BSolver` through which [Recipe
R4](R4-cfl-lagrangian.md#rec-R4) reaches it, the configuration that R4 solves
with a `LagrangianDualSolver`. The `:Solver` attached to the root `cfl`
dualizes the linking constraints, leaving the knapsacks of the facilities as
independent subproblems, each solved by a `CoreDPBinaryKnapsackSolver`
attached to its sub-`Block` (the `BlockSolverConfig` `LPBSCfg-K-DP.txt` named
by `str_LagBF_BSCfg`). The recursive flow of the `Modification`s ([Lemma
8.1](08-modification-janus.md#lem-8-1)) keeps that arrangement correct, since
a change of the data of a facility, wherever it is made, reaches the `:Solver`
of the root. Note that `chg_weight()` has `eNoBlck` as default for both its
parameters, which issues the `Modification` when somebody listens, as here. We
remark that the `LagrangianDualSolver` puts each sub-`Block` inside a
`LagBFunction` only while it needs the Lagrangian dual whole (in `compute()`,
`get_var_solution()` and `get_dual_solution()`), giving it back to `cfl`
otherwise
([`LagrangianDualSolver.h:1394-1407`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h#L1394-1407));
a `Modification` that `bk0` issues meanwhile reaches `cfl` directly, and the
`LagrangianDualSolver` hands it to the `LagBFunction`, which turns it into a
change of the dual function ([Chapter 14](14-lag-benders-bfunction.md#ch-14)),
when it takes `bk0` again.

## 12.6 Idioms {#sec-12-6}

*Coupling in the parent, locality in the children.* As
[§12.2](12-sub-block.md#sec-12-2) made precise, the framework does not
restrict where a `Constraint` lives with respect to the `Variable`s it
references; however, composability creates a strong expectation. A
sub-`Block` that references external `Variable`s only through explicit
setters, and only where the model requires it, stays understandable in
isolation and reusable in other contexts, while one that "reaches sideways"
into a sibling makes both `Block`s harder to reason about and to reuse. The
linking `Constraint`s go where they make the decomposition clearest, i.e.,
usually (but not always) in the common parent.

*The `:Solver` of a sub-tree at its root.* Since the `Modification`s flow
upward ([Lemma 8.1](08-modification-janus.md#lem-8-1)), a `:Solver` that must
account for the whole model is attached at the root, and a specialized
`:Solver` for one component is attached at the sub-`Block` of that component.
The two coexist because each of them is informed only of the changes in its
part of the tree.

*No dynamic sub-`Block`s.* If the number of components must vary, one
models the largest set of sub-`Block`s once, and switches the components on
and off through their data (fixing `Variable`s, relaxing `Constraint`s),
instead of adding or removing sub-`Block`s at run time.
