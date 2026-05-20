# 3. The mental model

Before turning to the precise definitions of Part II, this chapter
gives a high-level picture of the SMS++ framework. Its goal is to
build the reader's intuition: what a `Block` is for, what role a
`Solver` plays, why two representations of the same model coexist,
how a `Block` typically lives across its lifetime, and how the three
running examples used throughout the manual fit into this picture.
None of what follows is rigorous in the sense of Part II; everything
will be made precise there.

## 3.1 Block: a sub-problem with semantics

The cornerstone of SMS++ is the abstract class
[`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html).
The intuition behind it is captured by a single sentence: *a `Block`
is a fragment of a mathematical model with a well-understood
semantic*. "Fragment" because a `Block` is rarely the whole model;
it is more often one of several pieces that compose into a larger
model. "Semantic" because what a `Block` represents is not a list of
variables and constraints in some general-purpose syntax, but a
specific *kind* of problem: a min-cost flow, a knapsack, a
unit-commitment, a two-stage stochastic program. The kind is encoded
by the concrete `:Block` class derived from the abstract `Block`.

![A `Block` carries the semantics of a mathematical sub-problem.](../figures/SMSglobal0a.svg)

A concrete `:Block` knows its problem class. It knows what the data
of an instance looks like (a graph, a table of weights, a network of
generation units), how to load it from a text or binary file, how to
serialise it back, what makes a candidate solution feasible for it,
what the optimal value of the problem is conceptually, and what
algorithmic operations (changes to the data, reformulations, copies)
are meaningful for it. A user of the framework who wants to *solve*
an instance of that problem class typically does not write a new
`:Block`; the existing `:Block` of the relevant class is used and
configured. A user who wants to *introduce a new problem class*
writes a new `:Block`; this is the topic of Appendix A.

## 3.2 Solver: anything that can compute on a Block

Once a `Block` carries an instance, the framework needs an algorithm
to compute on it. In SMS++ the role is played by
[`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html),
an abstract class that represents "anything that can compute on a
`Block`". A concrete `:Solver` is one specific algorithm: a simplex
implementation for min-cost flow problems, a dynamic-programming
procedure for binary knapsack, a state-of-the-art commercial MIP
solver, a Lagrangian dual driver, a bundle method, a stochastic dual
dynamic programming engine.

![Any number of `:Solver`s may attach to a `Block`; changes flow to them as `Modification`s.](../figures/SMSglobal1a.svg)

Any number of `:Solver` can be attached to the same `Block` at the
same time. A specialised `:Solver` is one written for a specific
`:Block` class: it knows the problem and can exploit the structure;
the price is that it works only for that one `:Block`. A
general-purpose `:Solver` knows nothing about the specific structure
but can handle a large family of `:Block` through the abstract
representation (Section 3.3). The two coexist: the user picks, at
configuration time, which `:Solver` is attached to a given `:Block`.
Switching from one to the other is usually a one-line change to a
configuration file (Chapter 11), with no change to user code.

Any concrete `:Solver` exposes a uniform interface, regardless of how
it works internally. It accepts integer / double / string parameters;
it can be told to stop after a time limit, an iteration limit, or a
target accuracy; it can be invoked synchronously or asynchronously;
when it terminates it reports a return code (optimal, infeasible,
unbounded, time limit, iteration limit, error, ...); on demand it
yields one or more solutions of the problem, primal and (where it
makes sense) dual. The `Solver` interface, in this sense, is the
*lingua franca* of SMS++.

## 3.3 Two representations of the same model

A `Block` represents a mathematical model. SMS++ allows the same
`Block` to expose two distinct representations of that model, which
coexist and refer to the same underlying problem.

The first is the **physical** representation, sometimes called the
*semantic* representation: the problem data in its natural form. For
a min-cost flow problem this is a directed graph plus arc costs and
capacities plus node deficits, stored as compact arrays. For a
knapsack problem it is a capacity, a vector of weights, a vector of
profits. For a unit-commitment problem it is a network, a set of
generation units each described by its own technological parameters,
a demand profile. The physical representation is the form a domain
expert would write down on a whiteboard.

The second is the **abstract** representation: an explicit list of
`Variable` objects, an explicit list of `Constraint` objects, an
explicit `Objective` object — that is, the model in the form a
general-purpose solver would expect. For the same min-cost flow
problem this is a vector of flow variables, a vector of
flow-conservation row constraints, a vector of box constraints, a
linear objective. The abstract representation is what the framework
hands over when a general-purpose `:Solver` (an LP solver, a MIP
solver) is attached.

```
              physical                     abstract
              data                         Variable/Constraint/Objective
              -------                      ---------------
              graph G                      x[i,j] for arc (i,j)
              cost C[i,j]                  sum x[i,j] - sum x[j,i] = b[i]
              capacity U[i,j]              0 <= x[i,j] <= U[i,j]
              deficit b[i]                 min sum C[i,j] x[i,j]
```

![The two faces of a `Block`: the physical (semantic) data and the abstract `Variable` / `Constraint` / `Objective` representation.](../figures/SMSglobal4a.svg)

Why two representations? Because a specialised `:Solver` does not
need the explicit `Variable` and `Constraint`: it knows the problem
and can read the physical data directly, much faster and at much
lower memory cost. A general-purpose `:Solver`, on the other hand,
*does* need the explicit `Variable` and `Constraint`: it does not
know what the problem is and has no choice but to consume the model
in its abstract form. Having only one of the two would force a
compromise; having both is the simplest design that lets the user
keep both kinds of `:Solver` on the same `Block` without paying the
abstract-representation cost when only a specialised `:Solver` is in
use.

The abstract representation is built **on demand**. The `Constraint`s
and the `Objective` are constructed only when a `:Solver` that needs
them is attached. The methods that build them are
`generate_abstract_variables()`,
`generate_abstract_constraints()`,
`generate_dynamic_variables()`,
`generate_dynamic_constraints()`,
`generate_objective()`. Each accepts a `Configuration` that gates
optional parts of the construction. Chapter 7 makes this precise.

> **Status — under development.** In the current implementation the
> `Variable`s of the abstract representation are an exception to the
> "on demand" rule: they are always materialised, because every
> `:Solver` reports its solution by *writing into* the `Variable`s of
> the `Block`. This means that even a `:Block` solved exclusively by
> a specialised `:Solver` that reads only the physical representation
> still has to construct the abstract `Variable`s, just so that the
> `:Solver` has somewhere to write the solution. The framework
> framing — "physical and abstract coexist, neither is privileged" —
> is therefore not, today, a faithful description of the
> implementation: at present the abstract `Variable`s are privileged
> and unavoidable.
>
> The intended resolution, expected in a future revision, is to let
> a `:Solver` return a `Solution` object directly and to let the
> `:Block` *adopt* that `Solution` instead of writing it into the
> abstract `Variable`s. Once that mechanism is in place, a `:Block`
> attached to a specialised `:Solver` only will be able to skip
> constructing the abstract `Variable`s altogether. This is one of
> several examples in the manual where the API is not yet fully
> settled (see also Chapter 16 on `Change`).

Keeping the two representations in sync, when one is modified
through one face and a `:Solver` is reading the other, is non-trivial
and is the topic of Chapter 8 (the *Janus discipline*).

## 3.4 The lifecycle of a Block

A `Block` typically goes through five stages along its lifetime:
*build*, *configure*, *attach*, *solve*, *modify*. After the first
*solve* it loops over *modify* and *solve* zero or more times. The
last two stages — *modify* and *re-solve* — are not an afterthought;
they are explicit design objectives, and a good deal of the
framework's internal machinery exists to make them efficient.

![The lifecycle of a `Block`: build, configure, attach a `:Solver`, solve, then loop over modify and re-solve.](../figures/block-lifecycle.svg)

- **Build.** The `Block` is constructed and populated from data.
  The data may come from in-memory arrays (typically passed to a
  `:Block`-specific `load(...)` overload), from a text file (passed
  to `load(std::istream&)`), or from a netCDF group (passed to
  `deserialize(netCDF::NcGroup)`). Of these three routes, only the
  netCDF one is supported uniformly across the `:Block` catalogue;
  the in-memory and text-file overloads exist only for those
  `:Block` classes (such as `MCFBlock` and `MMCFBlock`) that have a
  historically established input format and for which it made sense
  to support it. For most other `:Block` classes the canonical route
  is netCDF (plus, of course, programmatic construction by the
  application code that owns the `:Block`).
- **Configure.** The optional behaviours of the `Block` (which parts
  of the abstract representation to generate, with which options;
  which `:Solver` to attach; which parameters to feed them) are set
  via a `Configuration` tree that mirrors the `Block` tree. The
  configuration can be loaded from a text file or from netCDF.
- **Attach.** One or more `:Solver` are registered with the `Block`.
  Registration is a constant-time operation; the `Solver` does not
  consume the `Block` data until its `compute()` method is called.
- **Solve.** A registered `:Solver` is asked to `compute()`. It
  consumes the `Block` data (physical or abstract, as appropriate),
  runs its algorithm, and produces a return code together with a
  pool of solutions and a pool of pending `Modification`s emptied
  on entry.
- **Modify and re-solve.** The user changes the data of the `Block`
  through one of the documented mutation methods. Each mutation
  produces zero or more `Modification` objects, which are dispatched
  to every `:Solver` currently attached to the `Block` or to any of
  its ancestor `Block`s. The next call to `compute()` on any of them
  starts with that list of pending `Modification`s; if the
  `:Solver` is able to reoptimize, it does so.

## 3.5 The three running examples

Three concrete `:Block` are used throughout the manual to make the
concepts tangible. Each is introduced briefly here and revisited in
Part II.

### `MCFBlock` — a leaf Block with a wrapper Solver

[`MCFBlock`](https://smspp.gitlab.io/smspp-project/d0/d51/_m_c_f_block_8h.html)
represents a linear min-cost flow problem on a directed graph. It is
a *leaf* `Block` (no sub-`Block`) with a small, well-defined physical
representation: arrays of arc start and end nodes, arc costs and
capacities, node deficits, plus a few counters for static / dynamic
arcs and nodes. It supports a partly dynamic graph (a static initial
set of nodes and arcs, plus up to a configurable maximum number of
dynamic nodes and arcs that can be added or deleted at runtime).

`MCFBlock` is paired with the templated specialised `:Solver`
[`MCFSolver< MCFC >`](https://smspp.gitlab.io/smspp-project/d2/dd2/class_s_m_spp__di__unipi__it_1_1_m_c_f_solver.html),
which wraps any concrete algorithm from the legacy `MCFClass`
project. The template parameter `MCFC` lets the user choose between
simplex, relaxation, cost-scaling and other classical algorithms.
This is the SMS++ example of a *wrapper* specialised `:Solver`: the
implementation lives in an external library that predates SMS++ by
years and continues to be developed in its own right; SMS++ exposes
it through the `:Solver` interface.

Through `MCFBlock` the manual will introduce: the basics of a leaf
`Block`, the construction of the abstract representation, the two
parallel streams of `Modification`s, `is_feasible(useabstract)` and
`is_optimal(useabstract)` written on both representations, the
trivial copy `R3Block`, and the `MCFSolution` class.

### `BinaryKnapsackBlock` — a leaf Block with a native Solver

[`BinaryKnapsackBlock`](https://smspp.gitlab.io/smspp-project/dc/d2f/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block.html)
represents a mixed-integer knapsack problem (despite the "Binary" in
its name, the boolean integrality vector lets each variable be
declared either binary or continuous). It is again a *leaf* `Block`,
even smaller than `MCFBlock`: the physical data is a capacity, a
vector of weights, a vector of profits, and a boolean integrality
vector.

It is paired with a *native* specialised `:Solver`,
[`DPBinaryKnapsackSolver`](https://smspp.gitlab.io/smspp-project/d5/d6a/class_s_m_spp__di__unipi__it_1_1_d_p_binary_knapsack_solver.html),
implemented entirely inside SMS++. The implementation is a classical
dynamic programming procedure on the integer part of the problem
combined with an exact greedy algorithm for the continuous part. The
algorithm requires integer weights (any non-integer weight raises an
exception); capacities and profits can be `double`. This is the SMS++
example of a *natively-implemented* specialised `:Solver`.

`BinaryKnapsackBlock` is also the reference example of the new beta
**`Change`** family
([`BinaryKnapsackBlockChange`](https://smspp.gitlab.io/smspp-project/d8/d50/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block_change.html),
plus the `Rngd` and `Sbst` variants). Through it the manual will
introduce: the methods factory, the `Solution` class as a
Block-specific object, and the `Change` concept that complements
`Modification` (Chapter 16).

### `CapacitatedFacilityLocationBlock` — a non-leaf Block with multiple formulations

[`CapacitatedFacilityLocationBlock`](https://smspp.gitlab.io/smspp-project/dc/d34/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_block.html)
represents the basic capacitated facility location problem (also
known as capacitated warehouse location). It is explicitly described
in its own source comments as a "didactic" `Block`, designed to
showcase several SMS++ features at once.

`CapacitatedFacilityLocationBlock` supports four formulations of the
same problem, selected via a single integer in its
`f_static_variables_Configuration`:

- the **Natural Formulation** (StdForm, the standard MILP) has no
  sub-`Block`, and the abstract representation contains the design
  variables `Y[i]`, the transportation variables `X[i,j]`, the
  customer satisfaction constraints, the facility capacity
  constraints, and a linear objective;

- the **Knapsack Formulation** (KskForm) makes the `Block` grow one
  sub-`BinaryKnapsackBlock` per facility (each carrying the
  facility's capacity constraint and the transportation/design
  variables for that facility); the master `Block` carries the
  customer satisfaction linking constraints;

- the **Benders Formulation** (BenForm) has only the design
  variables `Y[i]` plus a single continuous epigraphic variable `v`
  in the master, and the transportation sub-problem is hidden inside
  a `BendersBFunction` wrapping an `MCFBlock`. Two sub-variants are
  available, one with slack arcs in the inner `MCFBlock` (the
  sub-problem is then always feasible, only optimality cuts are
  emitted) and one without (feasibility cuts are emitted via the
  vertical-linearization machinery when the inner sub-problem is
  infeasible).

In addition, `CapacitatedFacilityLocationBlock` produces a
non-trivial `R3Block`: besides the trivial copy, it can produce an
`MCFBlock` that represents the *continuous flow relaxation* of CFL.
The same construction is used internally by the BenForm formulation;
externally it can be used as a fast lower-bound producer.

Through `CapacitatedFacilityLocationBlock` the manual will introduce:
sub-`Block` and the recursive flow of `Modification`s, multiple
formulations selected by `Configuration`, the non-trivial `R3Block`,
`LagBFunction` and `BendersBFunction` as the two `Function`+`Block`
hybrids that wrap an inner `Block`, the
[`LagrangianDualSolver`](https://smspp.gitlab.io/smspp-project/d4/dc3/class_s_m_spp__di__unipi__it_1_1_lagrangian_dual_solver.html)
and the derived `PrimalProximalHeur`, and the user-cut callback
pattern of `MILPSolver` for Benders cuts.

## 3.6 How Part II reuses the three running examples

Part II — *Concepts and Idioms* — introduces one concept per chapter,
in an order that ensures every concept is defined before it is used.
Each chapter follows the same skeleton (definition, structure, inline
example, API outline, idioms) and uses one of the three running
examples to instantiate the concept under discussion. The three
examples are chosen so that they collectively exercise every major
SMS++ concept once; the reader who has worked through Part II should
be ready to write a new `:Block` (Appendix A) or to assemble one of
the Recipes in Part III.

A small but useful summary table appears at the end of the manual as
back matter; the reader who wants to know which concept is
illustrated by which `:Block` and in which chapter can consult it.
