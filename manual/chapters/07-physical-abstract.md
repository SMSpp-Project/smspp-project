# 7. Physical vs Abstract representation {#ch-7}

[Source: `SMS++/include/Block.h`, `MCFBlock/include/MCFBlock.h`,
`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`]

This chapter makes precise what [Chapter 3](03-mental-model.md#ch-3) introduced informally:
a `Block` carries two representations of the same mathematical model
and the framework's machinery is designed to let both coexist
without forcing the user to pick one. It also closes the
"`Variable`s are always materialised" caveat already flagged in
[§3.3](03-mental-model.md#sec-3-3), by stating the current limitation precisely and indicating
the direction the API is intended to take.

## 7.1 Two faces of the same model {#sec-7-1}

A concrete `:Block` exposes its model along two complementary
faces.

The **physical** representation, sometimes called the *semantic*
representation, is the problem data in its natural form. The
`:Block` stores it as ordinary C++ data members. For `MCFBlock` the
physical representation is essentially the seven pieces
`NNodes`, `NArcs`, two arrays of node indices `SN[a]` and `EN[a]`
that encode the start and end nodes of every arc `a`, a vector of
arc capacities `U[a]`, a vector of arc costs `C[a]`, and a vector
of node deficits `B[i]`. For `BinaryKnapsackBlock` the physical
representation is a capacity `f_C`, a vector of weights `v_W`, a
vector of profits `v_P` and a boolean integrality vector `v_I`.
For `CapacitatedFacilityLocationBlock` the physical representation
is a triple of dimensions (`f_n_facilities`, `f_n_customers`,
`f_max_facilities`), the demand and capacity vectors, the
fixed-cost vector and the transportation-cost matrix.

The **abstract** representation, sometimes called the *syntactic*
representation, is the same model expressed as the explicit
`Variable`s, `Constraint`s and `Objective` introduced in
[Chapter 5](05-variable-constraint-objective.md#ch-5): it is the form a general-purpose solver would expect.
For `MCFBlock` the abstract representation consists of one
`std::vector< ColVariable >` of size `NArcs` for the arc flows,
one `std::vector< FRowConstraint >` of size `NStaticNodes` (and
possibly a `std::list< FRowConstraint >` for the dynamic nodes)
for the flow-conservation constraints, optionally a
`std::vector< LB0Constraint >` of size `NStaticArcs` (and possibly
a `std::list< LB0Constraint >` for dynamic arcs) for the arc
capacity bound constraints, and an `FRealObjective` carrying a
linear `Function` for the total flow cost. The latter two groups
are constructed only when needed: the `LB0Constraint`s are skipped
when all capacities are infinite, and the `Objective` shape can
be tuned between "dense" and "sparse" by an option (cf.
`generate_objective()` in [§7.3](07-physical-abstract.md#sec-7-3)).

The two representations refer to the same model and are
*conceptually equivalent*: a feasible solution in one is a
feasible solution in the other, with the same objective value, up
to numerical tolerance.

## 7.2 Why two representations {#sec-7-2}

The reason for keeping both faces around is that different
`:Solver`s have different needs.

A *specialised* `:Solver` written for a specific `:Block` knows
the problem and reads the physical representation directly. The
`MCFSolver< MCFC >` of [Chapter 6](06-solver.md#ch-6), for instance, hands the seven
arrays of `MCFBlock` to an underlying `MCFClass` algorithm; it
does not look at the abstract representation at all. The
specialised path is typically the fastest one and the cheapest
one in memory.

A *general-purpose* `:Solver` such as `:MILPSolver`, by contrast,
knows nothing about min-cost flow specifically. It reads the
abstract representation: it iterates the `Variable`s, builds the
constraint matrix from the linear-`Function`-carrying
`FRowConstraint`s, walks the `Objective`, and hands the resulting
MILP to its underlying back-end (CPLEX, Gurobi, HiGHS, SCIP).
This path works on any `:Block` that exposes a suitable abstract
representation, at the price of carrying the explicit
`Variable`/`Constraint`/`Objective` objects in memory.

Having only the physical face would lock out general-purpose
`:Solver`s; having only the abstract face would forbid the speed
gains of specialisation. SMS++ keeps both faces around and lets
the user pick, on a per-`Solver` basis, which one to consume.

## 7.3 On-demand construction of the abstract representation {#sec-7-3}

The physical representation exists as soon as the `:Block` has
been loaded (cf. [§4.5](04-block.md#sec-4-5)). The abstract representation, by contrast,
is *built on demand*: a `:Block` does not pay the cost of
constructing it until a `:Solver` that needs it is attached. The
construction is triggered by five virtual methods that every
concrete `:Block` overrides:

- `generate_abstract_variables(Configuration*)` constructs the
  static `Variable` groups.
- `generate_abstract_constraints(Configuration*)` constructs the
  static `Constraint` groups.
- `generate_dynamic_variables(Configuration*)` constructs the
  dynamic `Variable` lists (typically empty initially).
- `generate_dynamic_constraints(Configuration*)` constructs the
  dynamic `Constraint` lists.
- `generate_objective(Configuration*)` constructs the `Objective`.

Each takes a `Configuration*` that *gates* optional parts of the
construction. The `:Block` may interpret it as it wishes; the
canonical pattern is that the `Configuration*` is a
`SimpleConfiguration< int >` whose `f_value` is a bit-mask of
"which groups to materialise". For instance, `MCFBlock` interprets
the `Configuration*` of `generate_abstract_constraints` to decide
whether the arc-bound `LB0Constraint`s are skipped (which is
allowed only if all arc capacities are infinite, in which case the
bound is already encoded in the `ColVariable`s' `kNonNegative`
type); see `MCFBlock.h:506-571`. `CapacitatedFacilityLocationBlock`
goes further and uses the `Configuration*` of
`generate_abstract_variables` to select one of the four
formulations described in [§3.5](03-mental-model.md#sec-3-5); see
`CapacitatedFacilityLocationBlock.h:472-625`.

When the same `Configuration` parameters need to be applied across
a whole `Block` tree, the recursive `[C/O/R]BlockConfig` family of
[Chapter 11](11-configuration.md#ch-11) is the supported way to do it; it carries the
`Configuration` for `generate_abstract_variables`,
`generate_abstract_constraints`, `generate_dynamic_*`,
`generate_objective`, plus a few other slots, and propagates them
recursively into the sub-`Block`s.

The user-facing rule is simple: if only specialised `:Solver`s
are attached, the abstract representation need not be constructed
at all (with the caveat in [§7.6](07-physical-abstract.md#sec-7-6) below); as soon as a `:Solver`
that reads the abstract face is attached, the corresponding
`generate_*` methods are called once and the abstract face becomes
populated. The two faces are then kept in sync by the mechanisms
of [Chapter 8](08-modification-janus.md#ch-8).

## 7.4 The lifetime of the abstract representation {#sec-7-4}

Once constructed, the abstract representation lives for as long as
the `:Block` lives, with two exceptions.

First, a `:Block` may be *reset* by another `load(...)` call (or
by `deserialize(...)`), which replaces the problem instance
wholesale. The framework convention is that `load(...)` issues a
single `NBModification` (the "nuclear option" of [§4.6](04-block.md#sec-4-6)) to every
listening `:Solver`, and the abstract representation is rebuilt
the next time it is needed. The `:Block`'s `generate_*` methods
are *not* called automatically; they are called when a `:Solver`
that needs them next reads it. This is consistent with the
"on demand" rule.

Second, dynamic groups of `Variable` or `Constraint` may be added
to or removed from the abstract representation at any time
through the dedicated `add_dynamic_variable`,
`remove_dynamic_variable`, `add_dynamic_constraint`,
`remove_dynamic_constraint` operations. These are the support for
column generation and row generation; they issue dedicated
`Modification`s (`BlockModAdd` / `BlockModRmv`, see [Chapter 8](08-modification-janus.md#ch-8)) and
are consumed by `:Solver`s that know how to handle them.

## 7.5 Feasibility and optimality checks across both faces {#sec-7-5}

Three closely related virtual methods on the `Block` class
encapsulate the duality between the two representations:
`is_feasible(useabstract, Configuration*)`,
`is_dual_feasible(useabstract, Configuration*)`,
`is_optimal(useabstract, Configuration*)`. All three take a
boolean `useabstract`:

- `useabstract = false` (the default) asks the `:Block` to check
  the current candidate solution against the *physical*
  representation. This path is typically much cheaper: the
  `:Block` walks its own arrays directly and applies the
  problem-specific definition of feasibility. For `MCFBlock`,
  `is_feasible(false)` calls `flow_feasible(...)` and
  `bound_feasible(...)`, both of which read the arc-flow values
  and check them against the physical data
  (`MCFBlock.h:980-1054`).
- `useabstract = true` asks the `:Block` to check against the
  *abstract* representation by iterating the `Constraint`s and
  calling `feasible()` on each. This path only works if the
  abstract representation has been built, and is generally
  slower; its principal use is by general-purpose `:Solver`s that
  do not know the physical representation and have no faster
  alternative.

The two answers must agree up to numerical tolerance under the
`Configuration*` parameter that sets the tolerance; the
`Configuration*` is documented per `:Block` and typically reads
from the `:Block`'s `BlockConfig::f_is_feasible_Configuration`.
The two answers may, however, *genuinely* disagree in edge cases
where the abstract representation has not been fully synchronised
yet — most notably when dynamic `Constraint`s have not yet been
generated. The `MCFBlock` source comments
(`MCFBlock.h:4280-4330`) discuss this in detail and we refer the
reader to that source for the precise contract.

`is_optimal` is exactly `is_feasible && is_dual_feasible &&
complementary_slackness`; it requires *both* the primal and the
dual solutions to have been written into the appropriate
`Variable`s and `Constraint`s, which in turn requires the
abstract representation to carry both groups.

## 7.6 Status — under development: the half-baked `Solution` {#sec-7-6}

The framing of [§7.1](07-physical-abstract.md#sec-7-1), "two faces coexist and neither is privileged",
is not, at version 0.6.0, a faithful description of the
implementation: it is the framing we are working towards.

What is true today is the following:

- `Constraint`s and the `Objective` of the abstract representation
  are *genuinely on demand*: they are constructed only if a
  `:Solver` that needs them is attached, and the cost of
  constructing them is paid only when it is needed.
- `Variable`s of the abstract representation are *always
  materialised*, regardless of whether any `:Solver` actually
  reads them, because every `:Solver` reports its solution by
  *writing into* the abstract `Variable`s of the `Block`. Even an
  `MCFBlock` attached to an `MCFSolver< MCFSimplex >` that has
  never asked for the abstract representation must still have a
  `std::vector< ColVariable >` of size `NArcs` allocated, just so
  that the `:Solver` has somewhere to deposit the optimal flows.

The asymmetry is visible to any reader of the framework code: it
explains why a `:Block`'s `generate_abstract_variables()` is
called even when the only `:Solver` registered is specialised,
and why a `:Block` whose constructor allocates no `Variable` is
nonetheless not the canonical way to build a memory-thin
specialised-only `:Block`.

The intended resolution, in a future revision of the API, is to
let a `:Solver` return a `Solution` object *directly* and to let
the `:Block` *adopt* that `Solution`, instead of writing the
solution component-by-component into the abstract `Variable`s.
Under that scheme a `:Block` attached to a specialised `:Solver`
only would be able to skip the `generate_abstract_variables()`
step entirely, the `Solver` would hand back a `:Block`-specific
`Solution` (such as `MCFSolution`; full discussion in
[Chapter 9](09-solution.md#ch-9)) at the end of `compute()`, and the `Block` would
absorb it into its physical representation without intermediation.

The above is one of several places where the SMS++ public API is
not yet fully settled at version 0.6.0; the convention adopted by
the manual is to flag such places with **Status — under
development** and to let the reader know that user code written
today should not assume the asymmetry will persist. [Chapter 9](09-solution.md#ch-9)
returns to this point with the corresponding `Solution`-side
discussion.

## 7.7 Idioms {#sec-7-7}

**Build the abstract representation lazily.** User code does not
need to call `generate_abstract_*()` and `generate_objective()`
explicitly; the framework invokes them when the first
abstract-using `:Solver` is registered and the corresponding
information is requested. Calling them by hand is only useful in
two situations: when the user wants the abstract representation
to be present for inspection or for the `is_feasible(true)` check,
and when the user wants to pass a `Configuration*` that differs
from the one carried by the `:Block`'s `BlockConfig`.

**Use `is_feasible(false)` for cheap sanity checks.** When the
solution sitting in the `:Block`'s `Variable`s has been put there
by a specialised `:Solver`, the physical-path `is_feasible(false)`
is both cheaper and more reliable; the abstract path adds nothing
beyond what the specialised `:Solver` already knows. Reserve
`is_feasible(true)` for cases where the abstract representation is
the only one available, or for debugging discrepancies between the
two.

**Treat the abstract `Variable`s as the solution channel, not the
problem definition.** Until the API is reworked along the lines
of [§7.6](07-physical-abstract.md#sec-7-6), the abstract `Variable`s of a `:Block` are best thought
of as "the place where the `:Solver` writes the solution", not as
"the syntactic spelling of the problem". When the abstract face
of a `:Block` is needed in full (e.g. for an `:MILPSolver` to
operate on), the framework constructs it on demand; user code
that reads the solution should go through the `:Block`'s physical
accessors, not through the `Variable`s.
