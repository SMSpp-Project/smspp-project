# 3. The mental model {#ch-3}

Before the precise definitions of Part II, this chapter gives an
overall picture of the framework, meant to build the intuition of the
reader. We describe what a `Block` is for, what role a `Solver` plays,
why two representations of the same model coexist, how a `Block`
typically lives, and how the three running examples of the manual fit
into this picture. Nothing in this chapter is meant to be rigorous; we
make everything precise in Part II.

## 3.1 Block: a sub-problem with a semantic {#sec-3-1}

The central abstraction of SMS++ is the class
[`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html),
whose intuition is captured by one sentence: a `Block` is a fragment
of a mathematical model with a well-understood semantic. It is a
"fragment" because a `Block` is seldom the complete model, being more
often one of several pieces that are composed into a larger one. It
has a "semantic" because it represents a specific *kind* of problem (a
min-cost flow, a knapsack, a Unit Commitment, a two-stage stochastic
program), rather than a list of variables and constraints written in
some general-purpose syntax; the kind is encoded by the concrete
`:Block` class derived from `Block`.

![A `Block` carries the semantics of a mathematical
sub-problem.](../figures/SMSglobal0a.svg)

A concrete `:Block` knows its class of problems. It knows what the
data of an instance look like (a graph, a table of weights, a network
of generating units) and how to load them from a file and save them
back; it also knows what makes a candidate solution feasible, what the
optimal value of the problem is (conceptually), and which operations
on it (changes of the data, reformulations, copies) are meaningful.
One who wants to *solve* an instance of that class of problems does
not usually write a new `:Block`, but uses and configures the existing
one; one who wants to *introduce* a new class of problems writes a new
`:Block`, which is the subject of [Appendix
A](A-writing-block.md#app-A).

## 3.2 Solver: anything that can compute on a Block {#sec-3-2}

Once a `Block` holds an instance, an algorithm is needed to compute on
it. This is the role of
[`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html),
an abstract class that represents "anything that can compute on a
`Block`". A concrete `:Solver` is one specific algorithm (a simplex
method for min-cost flow problems, a dynamic programming procedure for
the knapsack problem, a commercial MIP solver, a Lagrangian dual
method, a bundle method, an SDDP engine).

![Any number of `:Solver`s may attach to a `Block`; changes flow to
them as `Modification`s.](../figures/SMSglobal1a.svg)

One can attach any number of `:Solver`s to the same `Block` at the
same time. A *specialized* `:Solver` is written for a specific
`:Block` class: it knows the problem and can exploit its structure, at
the price of working only for that `:Block`. A *general-purpose*
`:Solver` ignores the specific structure, but it can handle a large
family of `:Block`s through their abstract representation (cf.
[§3.3](03-mental-model.md#sec-3-3)). Both kinds coexist, and which
`:Solver` is attached to a given `:Block` is decided at configuration
time; switching from one to the other is usually a change of one line
in a configuration file (cf. [Chapter 11](11-configuration.md#ch-11)),
with no change to the code of the user.

All the `:Solver`s expose the same interface, however they work
internally. A `:Solver` accepts parameters of integer, real and string
type (and vectors of them), and it can be told to stop after a time
limit, an iteration limit or when a given accuracy is reached. It can
be invoked synchronously or asynchronously; when it stops, it reports
a return code (optimal, infeasible, unbounded, stopped at a limit,
error, and so on), and on demand it delivers one or more solutions of
the problem, primal and (where this makes sense) dual. In this sense
the interface of `Solver` is the common language of SMS++.

## 3.3 Two representations of the same model {#sec-3-3}

A `Block` represents a mathematical model, and SMS++ allows the same
`Block` to expose two distinct representations of it, which coexist
and refer to the same underlying problem.

The first is the *physical* representation (sometimes called the
*semantic* one), i.e., the data of the problem in their natural form.
For a min-cost flow problem these are a directed graph, the costs and
capacities of its arcs and the deficits of its nodes, stored as
compact arrays, and for a knapsack problem a capacity, a vector of
weights and a vector of profits. For a Unit Commitment problem they
are a network, a set of generating units, each described by its
technical parameters, and a profile of the demand. This is the form in
which a domain expert would write the problem on a whiteboard.

The second is the *abstract* representation, i.e., an explicit list of
`Variable` objects, an explicit list of `Constraint` objects and an
explicit `Objective` object: the model in the form a general-purpose
solver expects. For the same min-cost flow problem, these are a vector
of flow variables, a vector of flow conservation constraints, a vector
of bound constraints and a linear objective, which the framework hands
over to a general-purpose `:Solver` (an LP or a MIP solver) when it is
attached. For a graph $G = (N, A)$ with arc costs $c_{ij}$, arc
capacities $u_{ij}$ and node deficits $b_i$, the correspondence is the
following:

| physical | abstract |
|---|---|
| graph $G = (N, A)$ | one `ColVariable` $x_{ij}$ per arc $(i, j) \in A$ |
| deficits $b_i$ | $\sum_{(k,i) \in A} x_{ki} - \sum_{(i,k) \in A} x_{ik} = b_i$ for each $i \in N$ |
| capacities $u_{ij}$ | $0 \le x_{ij} \le u_{ij}$ for each $(i,j) \in A$ |
| costs $c_{ij}$ | $\min \sum_{(i,j) \in A} c_{ij} x_{ij}$ |

(a negative deficit denotes a source, a positive one a sink;
Proposition 7.1 in [§7.1](07-physical-abstract.md#sec-7-1) proves that
the abstract representation built by `MCFBlock` is this one).

![The two faces of a `Block`: the physical (semantic) data and the
abstract `Variable` / `Constraint` / `Objective`
representation.](../figures/SMSglobal4a.svg)

Two representations are needed because a specialized `:Solver` does
not need the explicit `Variable`s and `Constraint`s: it knows the
problem, and it can read the physical data directly, much faster and
with much less memory. A general-purpose `:Solver`, on the other
hand, does need them, since it does not know what the problem is and
can only consume the model in its abstract form. Having only one of
the two representations would force a compromise; having both lets
the user keep both kinds of `:Solver` on the same `Block`, without
paying the cost of the abstract representation when only a
specialized `:Solver` is in use.

Hence, the abstract representation is built *on demand*: the
`Constraint`s and the `Objective` are constructed only when a
`:Solver` that needs them is attached. They are built by
`generate_abstract_variables()`, `generate_abstract_constraints()`,
`generate_dynamic_variables()`, `generate_dynamic_constraints()` and
`generate_objective()`, each of which accepts a `Configuration` that
selects the optional parts of the construction; [Chapter
7](07-physical-abstract.md#ch-7) makes this precise.

> **Status — under development.** The `Variable`s of the abstract
> representation used to be an exception to the "on demand" rule.
> They were always constructed, because the `:Solver`s reported their
> solution by *writing* it into the `Variable`s of the `Block`; hence,
> even a `:Block` solved only by a specialized `:Solver`, which reads
> the physical representation, had to construct its abstract
> `Variable`s just to give the `:Solver` a place where to write the
> solution.
>
> The way out is `Solver::get_Solution()`, which returns a `Solution`
> object directly. Its default implementation still goes through the
> `Variable`s (it calls `get_var_solution()` and then
> `Block::get_Solution()`), but a "physical" `:Solver` re-implements
> it to fill the `:Solution` out of its data structures, without
> writing into the `Variable`s and without `lock()`-ing the `Block`,
> and it declares this by returning `true` from
> `is_get_Solution_physical()`. This is the case of `MCFSolver`,
> `MCFLemonSolver`, the `:Solver`s of `BinaryKnapsackBlock` derived
> from `BinaryKnapsackSolver`, `ThermalUnitDPSolver`,
> `ThermalUnitExtDPSolver`, and the `:Solver`s of `SVMBlock`. A
> `:Block` attached only to such `:Solver`s can therefore avoid
> constructing its abstract `Variable`s altogether. This mechanism is
> not yet used uniformly across the catalogue (`MCFBlock` and
> `BinaryKnapsackBlock` still construct their abstract `Variable`s in
> `load()`; see [§7.6](07-physical-abstract.md#sec-7-6) and
> [§9.7](09-solution.md#sec-9-7)), and this is one of the places of
> the manual where the interface is not yet settled (another is
> `Change`, see [Chapter 16](16-change.md#ch-16)).

Keeping the two representations in step, when one is modified while a
`:Solver` reads the other, is somewhat delicate, and it is the subject
of [Chapter 8](08-modification-janus.md#ch-8) (the Janus discipline).

## 3.4 The lifecycle of a Block {#sec-3-4}

A `Block` typically goes through five stages (build, configure,
attach, solve, modify), and after the first solve it loops over
modify and solve zero or more times. These last two stages are
explicit objectives of the design, and a good part of the machinery
of the framework exists to make them efficient.

![The lifecycle of a `Block`: build, configure, attach a `:Solver`,
solve, then loop over modify and
re-solve.](../figures/block-lifecycle.svg)

- *Build.* First, the `Block` is constructed and filled with data, which may
  come from arrays in memory (typically passed to a `load( ... )` method
  specific to the `:Block`), from a text file (passed to
  `load( std::istream & , char )`), or from a netCDF group (passed to
  `deserialize( const netCDF::NcGroup & )`). Only the netCDF route is supported
  uniformly across the catalogue. The text one is declared by all the
  `:Block`s, since it is pure virtual in `Block`, but it reads something
  meaningful only for those that have an established text format. Among them
  are the three running examples (`MCFBlock` reading the DIMACS format),
  `MMCFBlock`, `SATBlock` and `LukFiBlock`; the overloads from memory exist
  only where they have been written. For most other `:Block`s the intended
  routes are netCDF and the construction by the application that owns the
  `:Block`.
- *Configure.* The optional behaviors of the `Block` are set by a tree
  of `Configuration` objects that mirrors the tree of `Block`s, and
  which can be loaded from a text file or from netCDF. They comprise
  which parts of the abstract representation to generate (and with
  which options), and which `:Solver`s to attach (and with which
  parameters).
- *Attach.* One or more `:Solver`s are registered with the `Block`.
  Registration calls the `set_Block()` method of the `Solver`, where a
  `:Solver` typically reads the data of the `Block` it needs; a
  general-purpose one also has the abstract representation generated
  there, and loads it into its back-end. The computation does not
  start until the `compute()` method of the `:Solver` is called.
- *Solve.* Then `compute()` is called on a registered `:Solver`. The
  `:Solver` first processes the `Modification`s it has received since
  the previous call, then reads the data of the `Block` (physical or
  abstract, as appropriate) and runs its algorithm; it returns a code
  that describes how it stopped, after which the solutions it has
  found can be retrieved.
- *Modify and solve again.* Afterwards, the user changes the data of
  the `Block` through one of its documented methods. Each change
  produces zero or more `Modification` objects, which are dispatched
  to the `:Solver`s attached to the `Block` or to one of its
  ancestors; the next call to `compute()` of each of them starts by
  processing them, and a `:Solver` that is able to reoptimize does so.

## 3.5 The three running examples {#sec-3-5}

Three concrete `:Block`s are used throughout the manual to make the
concepts tangible; we introduce each of them briefly here, and revisit
it in Part II.

### `MCFBlock`: a leaf Block with a wrapper Solver

[`MCFBlock`](https://smspp.gitlab.io/smspp-project/d0/d51/_m_c_f_block_8h.html)
represents a linear min-cost flow problem on a directed graph. It is
a *leaf* `Block` (it has no sub-`Block`) with a small physical
representation: the arrays of the start and end nodes of the arcs,
of the costs and capacities of the arcs and of the deficits of the
nodes, plus the counters of the static and dynamic nodes and arcs.
Its graph is partly dynamic, i.e., to a static set of nodes and arcs
one can add (and later remove) dynamic nodes and arcs, up to maximum
numbers given when the instance is loaded.

`MCFBlock` is paired with the specialized template `:Solver`
[`MCFSolver< MCFC >`](https://smspp.gitlab.io/smspp-project/d2/dd2/class_s_m_spp__di__unipi__it_1_1_m_c_f_solver.html),
which wraps a concrete algorithm of the `MCFClass` library. The template
parameter `MCFC` chooses among a primal or dual network simplex
(`MCFSimplex`), a relaxation method (`RelaxIV`), a cost-scaling method
(`CS2`), a shortest path tree method for the uncapacitated case (`SPTree`),
the network simplex of ZIB (`MCFZIB`) and the network solver of CPLEX
(`MCFCplex`). This is the example of a *wrapper* `:Solver`: the implementation
lives in an external library that predates SMS++ and is developed
independently, and SMS++ exposes it through the `:Solver` interface.

Through `MCFBlock` we introduce the basics of a leaf `Block`, the construction
of the abstract representation, the two streams of `Modification`s, the
methods `is_feasible( useabstract )` and `is_optimal( useabstract )` written
on both representations, the `R3Block` that is a plain copy, and the class
`MCFSolution`.

### `BinaryKnapsackBlock`: a leaf Block with a native Solver

[`BinaryKnapsackBlock`](https://smspp.gitlab.io/smspp-project/dc/d2f/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block.html)
represents a knapsack problem with binary and continuous items
(despite the "Binary" in its name, a vector of integrality flags
declares each item either binary or continuous). It is again a *leaf*
`Block`, even smaller than `MCFBlock`: its physical data are a
capacity, a vector of weights, a vector of profits and the vector of
integrality flags.

It is paired with a *native* specialized `:Solver`,
[`DPBinaryKnapsackSolver`](https://smspp.gitlab.io/smspp-project/d5/d6a/class_s_m_spp__di__unipi__it_1_1_d_p_binary_knapsack_solver.html),
written entirely within SMS++: a classical dynamic program on the
integer items, combined with the exact greedy algorithm for the
continuous ones. This dynamic program requires the weights to be
integer (a fractional weight makes `DPBinaryKnapsackSolver` throw an
exception), while the capacity is rounded down and the profits can be
any `double`. This is the example of a *native* specialized `:Solver`.
The module also carries other native `:Solver`s derived from
`BinaryKnapsackSolver`. One is an exact *core* dynamic program
(`CoreDPBinaryKnapsackSolver`), which only enumerates the
non-dominated states around the break item (and on hard instances is
orders of magnitude faster); two hand its core to the external RECORD
and COMBO codes, and a greedy one solves the continuous relaxation
(`GreedyRelaxationBinaryKnapsackSolver`).

`BinaryKnapsackBlock` is also the reference example of the family of
`Change`s, in beta
([`BinaryKnapsackBlockChange`](https://smspp.gitlab.io/smspp-project/d8/d50/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block_change.html),
with its variants `BinaryKnapsackBlockRngdChange` and
`BinaryKnapsackBlockSbstChange`). Through it we introduce the methods
factory, the `Solution` as an object specific to a `Block`, and the
concept of `Change` that complements the one of `Modification` (cf.
[Chapter 16](16-change.md#ch-16)).

### `CapacitatedFacilityLocationBlock`: a non-leaf Block with several formulations

[`CapacitatedFacilityLocationBlock`](https://smspp.gitlab.io/smspp-project/dc/d34/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_block.html)
represents the classical capacitated facility location problem (also
known as capacitated warehouse location), and its comments describe it
as a "didactic" `Block`, meant to show several features of SMS++ at
once.

`CapacitatedFacilityLocationBlock` supports four formulations of the
problem, selected by the two lowest bits of an integer read from the
`f_static_variables_Configuration` of its `BlockConfig` (or from the
`Configuration` passed to `generate_abstract_variables()`):

- the *natural* formulation (value 0, `StdForm`), i.e., the standard
  MILP, which has no sub-`Block` and whose abstract representation
  contains the design variables $y_i$, the transportation variables
  $x_{ij}$, the constraints that satisfy the customers, the capacity
  constraints of the facilities and a linear objective;
- the *knapsack* formulation (value 1, `KskForm`), in which the
  `Block` has one sub-`Block` of type `BinaryKnapsackBlock` per
  facility, carrying the capacity constraint of the facility, its
  transportation variables and its design variable, while the
  `Block` itself only carries the constraints that link the
  sub-`Block`s (those that satisfy the customers);
- two *Benders* formulations (values 2 and 3, `BenForm`), in which the
  abstract representation only has the design variables $y_i$ and a
  single continuous epigraphic variable $v$, while the transportation
  subproblem is hidden inside a `BendersBFunction` that wraps an
  `MCFBlock` (which is not a sub-`Block`). In the first variant the
  inner `MCFBlock` has slack arcs, which make the subproblem always
  feasible, and only optimality cuts are generated; in the second it
  has none, and when the subproblem is infeasible a feasibility cut is
  generated through the vertical linearization of the
  `BendersBFunction`.

A third bit of the same integer makes the transportation variables
$x_{ij}$ integer (the *unsplittable* problem); it is only allowed
with the natural and the knapsack formulations, since the Benders
subproblem requires the splittable case.

`CapacitatedFacilityLocationBlock` also produces a nontrivial
`R3Block`: besides the plain copy, it can produce an `MCFBlock` that
represents the continuous flow relaxation of the problem. This
construction is also used internally by the Benders formulations, and
from the outside it can serve as a fast producer of lower bounds.

Through `CapacitatedFacilityLocationBlock` we introduce the
sub-`Block`s and the recursive flow of the `Modification`s, the choice
among several formulations by a `Configuration`, and the nontrivial
`R3Block`. We also introduce `LagBFunction` and `BendersBFunction`,
the two hybrids of a `Function` and a `Block` that wrap an inner
`Block`, the
[`LagrangianDualSolver`](https://smspp.gitlab.io/smspp-project/d4/dc3/class_s_m_spp__di__unipi__it_1_1_lagrangian_dual_solver.html)
and its derived class `PrimalProximalHeur`, and the callback of
`MILPSolver` that issues Benders cuts.

## 3.6 How Part II uses the three running examples {#sec-3-6}

Part II introduces one concept per chapter, in an order such that each
concept is defined before it is used. Each chapter follows the same
scheme (definition, structure, example, outline of the interface,
idioms), and uses one of the three running examples to instantiate the
concept under discussion; together, the three exercise the major
concepts of SMS++. A reader who has worked through Part II should be
able to write a new `:Block` (cf. [Appendix
A](A-writing-block.md#app-A)) or to assemble one of the recipes of
Part III.

A reader who wants to know which concept is illustrated by which
`:Block`, and in which chapter, can consult the table of
[Appendix D](D-concept-block-map.md#app-D).
