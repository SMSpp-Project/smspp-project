# Glossary

This glossary lists in alphabetical order the SMS++ terms used in this
manual, each with its working definition and a pointer to the chapter
where the term is developed. Class names link, where useful, to the
chapter rather than to Doxygen, which remains the authoritative API
reference.

**Abstract representation.** The explicit `Variable` / `Constraint` /
`Objective` description of a `:Block`, built on demand by the
`generate_*()` methods. General-purpose `Solver`s read it;
specialized ones usually do not. The counterpart of the *physical
representation* ([Chapter 7](07-physical-abstract.md#ch-7)).

**AbstractChange.** The `Change` that edits the abstract
representation of any `:Block` (the coefficients and the sense of its
`Objective`, the integrality, fixing and bounds of its `ColVariable`s),
with no knowledge of its physical data; the generic
counterpart of the `:Block`-specific `Change` families
([Chapter 16](16-change.md#ch-16)).

**AModification.** The abstract base for `Modification`s that describe
a change made through the *abstract* face of a `:Block` (e.g., of a
`Variable`, a `Constraint` or a `Function`). Its `concerns_Block()` is
`true` when the `:Block` has not seen the change yet, and the
`add_Modification()` of the `:Block` sets it to `false` once it has
reflected the change into its physical data
([Chapter 8](08-modification-janus.md#ch-8)).

**BendersBFunction.** A `Function` that is *also* a `Block`, computing
the value function of an inner "base" `Block` parameterized by an
affine map $M(x) = A x + b$ of the outer `Variable`s $x$, which gives
the sides of some of its `RowConstraint`s. Each evaluation solves the inner
`Block`; its linearizations are Benders optimality or feasibility
cuts ([Chapter 14](14-lag-benders-bfunction.md#ch-14)).

**Block.** The central abstraction: a nested unit carrying the
*semantics* of a mathematical subproblem and, on demand, an abstract
representation of it. A `Block` has a father, any number of static
sub-`Block`s, `Variable` / `Constraint` / `Objective` groups, and any
number of registered `Solver`s ([Chapter 4](04-block.md#ch-4)).

**BlockConfig.** A `Configuration` with ten slots that configure a
`:Block`: its structure, the generation of its abstract representation,
its feasibility and optimality checks, the part of its solution that is
saved, and an extra slot of its own; differential `BlockConfig`s
override slot by slot ([Lemma 11.1](11-configuration.md#lem-11-1)); the `[C/O/R]BlockConfig` family
adds the `ComputeConfig`s of its `Constraint`s (C) and of its
`Objective` (O), and the `BlockConfig`s of its sub-`Block`s
(R), so that the configuration recurses down the tree
([Chapter 11](11-configuration.md#ch-11)).

**BlockSolverConfig.** A `Configuration` that gives which `Solver`s are
attached to a `Block` and with which parameters (Chapters
[6](06-solver.md#ch-6), [11](11-configuration.md#ch-11)).

**BlockWeightMod.** The `Modification` issued when the weight of the
`Objective` or of the `Variable`s of a `Block` changes; see *Weight*
([§12.2](12-sub-block.md#sec-12-2)).

**Branch-and-X.** The generic enumeration of `BranchAndXSolver`, whose
nodes are identified by `Change`s and evaluated by the
`:RelaxationSolver`s and the heuristic `:ChangeSolver`s named in its
configuration; the node that a relaxation cannot branch and its bound
cannot prune is set aside, and its bound counts in that of the solve
([§16.8](16-change.md#sec-16-8), [§16.9](16-change.md#sec-16-9)).

**C05Function / C15Function.** Refinements of `Function` exposing,
respectively, first-order information (values and linearizations, not
necessarily from continuous gradients) and second-order information
(approximations of the Hessian) ([Chapter 13](13-function-family.md#ch-13)).

**CDASolver.** "Convex Duality Aware" `Solver`: one that, besides a
primal solution, produces (possibly several) dual solutions / valid
bounds. `MCFSolver` and the `MILPSolver` family are `CDASolver`s
([Chapter 6](06-solver.md#ch-6)).

**Change.** A transmissible, often invertible description of an edit to
a `:Block`; unlike a `Modification` (a notification of a change that has
already happened), a `Change` carries the data needed to *apply* the
edit to a fresh `:Block` and, through its UndoChange, to undo it.
`Change`s compose into a `GroupChange` ([Chapter
16](16-change.md#ch-16)). Most `Change`s are serialized and read back,
`GroupChange` excepted, and the nodes of a Branch-and-X are identified
by `Change`s.

**ChangeSolver / RelaxationSolver.** Two traits of `ChangeSolver.h`
that a concrete `:Solver` derives from alongside its `Solver`
base: a `ChangeSolver` can apply a `Change` (and the UndoChange it
returns) to the problem it solves, possibly to its internal state
only, so as to move along a sequence of related problems; a `RelaxationSolver`
is a `ChangeSolver` that solves a relaxation, may give feasible
solutions of the original problem, and can `branch()`, i.e., produce
the `Change`s that generate the children of the current node
([Chapter 16](16-change.md#ch-16)).

**Channel.** A sequence of `Modification`s that a `Block` collects into
a `GroupModification` instead of dispatching them one by one, opened
by `open_channel()` and closed by `close_channel()`; `add_Modification()`
takes the name of the channel as its second argument
([Chapter 8](08-modification-janus.md#ch-8)).

**ColVariable.** The concrete `Variable` representing a single real or
integer decision variable, with a type (e.g., `kContinuous`,
`kBinary`, `kInteger`), a value, and fix/unfix state
([Chapter 5](05-variable-constraint-objective.md#ch-5)).

**compute().** The method, inherited from `ThinComputeInterface`, by
which a `Solver` (or a `Function`, or any computable object) performs
its computation and returns a `sol_type` status ([Chapter 6](06-solver.md#ch-6)).

**concerns_Block().** A flag on a `Modification` that is `true` while a
change made through the abstract face still has to be folded back into
the physical data; the `:Block`'s `add_Modification()` resets it once
done ([Chapter 8](08-modification-janus.md#ch-8)).

**Configuration.** A tree-shaped object mirroring the `Block` tree that
parameterizes generation, solving, and computation;
`SimpleConfiguration< T >`, the `[C/O/R]BlockConfig` family,
`BlockSolverConfig`, and `ComputeConfig` are its concrete forms ([Chapter
11](11-configuration.md#ch-11)).

**Constraint.** A primitive living inside a `Block`. `RowConstraint`
has a real-valued left/right-hand side; `FRowConstraint` carries a
`Function`; `OneVarConstraint` (e.g., `LB0Constraint`) bounds a single
`ColVariable`. The one dual variable of a `RowConstraint` is the
coefficient of its expression in the Lagrangian function (one value
suffices by [Lemma 9.3](09-solution.md#lem-9-3), and its sign is that
of [§9.4](09-solution.md#sec-9-4)).

**Core point.** A point in the relative interior of the feasible set of
the master at which `BendersDecompositionSolver` generates a
Pareto-optimal cut; it starts at the initial point and moves towards
each incumbent ([§14.6](14-lag-benders-bfunction.md#sec-14-6)).

**Deflection.** The convex combination of the current subgradient and of
the previous direction that gives the direction of `SubGradSolver`,
chosen by one of the rules of its factory
([§19.6](19-nondifferentiable.md#sec-19-6)).

**Direction (of a `Block`).** A ray of the feasible region along which
the `Objective` is unbounded, of which a `Block` is told by
`is_direction( true )`; `has_directions()` says whether the `Block`
accepts it ([Chapter 7](07-physical-abstract.md#ch-7)).

**Dual value.** The single value of a `RowConstraint` that stands for
its two multipliers, i.e., the coefficient of the row in the Lagrangian
function; its sign, and the signs with which each `:Solver` writes it,
are those of [§9.4](09-solution.md#sec-9-4).

**Dynamic vs static.** Static `Variable` / `Constraint` groups are
fixed in number and may live in vectors or `boost::multi_array`s;
dynamic ones may be added and removed at run time and must live in
`std::list`s, because an SMS++ object's identity is its memory address
(Chapters [4](04-block.md#ch-4), [5](05-variable-constraint-objective.md#ch-5)).

**Event.** A handler that a `:Solver` calls at given points of its
`compute()`, set by `set_event_handler()` for one of the types of
`ThinComputeInterface::event_type`: just before terminating, every $k$
iterations, every $T$ seconds, or each time a partial solution is
written into the `Block` ([Chapter 6](06-solver.md#ch-6)).

**Factory.** A map from a class name (`std::string`) to a constructor,
allowing a `:Block`, `:Solver`, `:Configuration`, `:Solution`,
`:Change` or `:State` to be created by name and reconstructed from
netCDF; registered with the `SMSpp_insert_in_factory_*` macros; one map
per factory serves all the libraries of a program, and a `:Block` or a
`:Solver` that no linked library registers may be supplied by a
*plugin* ([Chapter 18](18-factories-netcdf.md#ch-18)). Distinct from
the *methods factory* (below).

**Frank-Wolfe gap.** The decrease of the linearized objective from the
iterate to the point returned by the linear minimization oracles, an
upper bound on the distance of the value of the iterate from the optimal
one ([§19.7](19-nondifferentiable.md#sec-19-7)).

**FRealObjective.** The concrete `Objective` carrying a `Function` to
be minimized or maximized ([Chapter 5](05-variable-constraint-objective.md#ch-5)).

**Function.** A real-valued (extended-real) function of `ColVariable`s
that must be `compute()`-d; the family runs from `Function` to
`C05Function` and `C15Function`, with leaf implementations such as `LinearFunction`,
`DQuadFunction`, and `PolyhedralFunction` ([Chapter 13](13-function-family.md#ch-13)).

**GlobalInformation.** A registry of named, typed and thread-safe
collections shared by the `:Solver`s that cooperate in a solve, e.g.,
the incumbent and the global cuts of a Branch-and-X
([§16.7](16-change.md#sec-16-7)).

**GroupChange.** The `Change` made of a sequence of `Change`s, which it
owns and applies in order as one; since its elements may be
`GroupChange`s themselves, `Change`s nest arbitrarily
([Chapter 16](16-change.md#ch-16)).

**GroupModification.** The `Modification` made of a group of logically
related `Modification`s, as collected by a *channel*; its
`concerns_Block()` is the "or" of those of its elements
([Chapter 8](08-modification-janus.md#ch-8)).

**Hard / easy component.** A component of the function minimized by
`BundleSolver` known only through its oracle, and approximated in the
master by its linearizations, or one whose compact convex description is
put directly in the master ([§19.2](19-nondifferentiable.md#sec-19-2)).

**Janus discipline.** The rule that a `:Block`'s two faces (physical
and abstract) be kept consistent: a `chg_*()` mutator updates the
physical data and mirrors the change into the abstract one, while a
change arriving through the abstract face is intercepted by
`add_Modification()` and folded back into the physical data
([Chapter 8](08-modification-janus.md#ch-8)).

**LagBFunction.** A `Function` that is *also* a `Block`, computing the
Lagrangian (dual) function of an inner "base" `Block` with respect to
a set of linear terms; its linearizations correspond to
(approximately) optimal inner solutions, which allows a bundle
method to optimize the dual ([Chapter 14](14-lag-benders-bfunction.md#ch-14)).

**Leaf Block.** A `Block` with no sub-`Block`s (`MCFBlock`,
`BinaryKnapsackBlock`); the opposite of a *non-leaf* (composite)
`Block` such as `CapacitatedFacilityLocationBlock` in its knapsack
formulation (Chapters [4](04-block.md#ch-4), [12](12-sub-block.md#ch-12)).

**Linear minimization oracle.** The `:Solver` of a sub-`Block` that
`FrankWolfeSolver` uses to minimize a linear function over the feasible
region of that sub-`Block` ([§19.7](19-nondifferentiable.md#sec-19-7)).

**Linearization.** A first-order piece of information produced by a
`C05Function` at an evaluation point (for a convex function, an
$\varepsilon$-subgradient defining an affine minorant, [Lemma 13.1](13-function-family.md#lem-13-1));
pools of linearizations
drive bundle methods and Benders/Lagrangian schemes (Chapters [13](13-function-family.md#ch-13), [14](14-lag-benders-bfunction.md#ch-14)).

**Master problem.** The problem whose solution gives the next trial
point of a stabilized method, i.e., the cutting-plane model plus a
stabilizing term; in SMS++, a `MasterProblemBlock`
([§19.2](19-nondifferentiable.md#sec-19-2)).

**memo_cast().** The function template of `SMSTypedefs.h` that gives
what `dynamic_cast` gives under the hypotheses of [Lemma
8.2](08-modification-janus.md#lem-8-2) (since SMS++ 0.9.0;
[§8.3](08-modification-janus.md#sec-8-3)).

**Meta-configuration.** A configuration given by class name rather than
by position in the tree of `Block`s, i.e., a map from the name of a
`:Block` to the `BlockConfig` or `BlockSolverConfig` to be applied to
every descendant of that class of the `Block` it is given to, the
entry `"*"` being the default, besides its own fields for that `Block`
(`MetaBlockConfig`, `MetaBlockSolverConfig`; [Chapter
11](11-configuration.md#ch-11)).

**Methods factory.** A map from a name (`std::string`) to a function, of the
`chg_*()` / `set_*()` methods (and of the queries) of the `:Block`s,
grouped by signature, registered with `register_method()` and
retrieved with `get_method()`, so that type-agnostic code can change
the data of a `:Block` through a base `Block*` ([Chapter 15](15-methods-factory.md#ch-15)).

**MI-QCQP.** Mixed-Integer Quadratically Constrained Quadratic Program:
the class of problems that the abstract `MILPSolver` accepts, i.e., a
quadratic objective and linear or quadratic rows of `ColVariable`s,
some of them integer; each concrete back-end accepts a subclass of it
([Chapter 6](06-solver.md#ch-6)).

**Mirror.** An `AbstractBlock` built by `mirror()` as a copy of the
abstract representation of any `Block`, which makes it an R3 Block of
that `Block` without the `Block` implementing anything ([Lemma
10.2](10-r3block.md#lem-10-2), [§10.4](10-r3block.md#sec-10-4)).

**Modification.** A notification that something changed in a `Block`;
the framework appends it to the pending list of every `Solver`
registered with the `Block` or an ancestor and concerned by its kind
([Lemma 8.1](08-modification-janus.md#lem-8-1)), and the `Solver` consumes it when it reacts. `NBModification` is the wholesale
"reload" notification ([Chapter 8](08-modification-janus.md#ch-8)).

**ModParam.** The argument by which a method that changes a `Block`
chooses whether to issue a `Modification`: `eDryRun` (no change), `eNoMod`
(the change without a `Modification`), `eNoBlck` (a `Modification`
whose `concerns_Block()` is `false`) and `eModBlck` (one whose
`concerns_Block()` is `true`)
([Chapter 8](08-modification-janus.md#ch-8)).

**netCDF.** The hierarchical binary format SMS++ uses for
serialization; nested `Block`s map to nested netCDF groups, inspectable
with `ncdump` / `ncview` / Panoply ([Chapter 18](18-factories-netcdf.md#ch-18)).

**Objective.** The primitive expressing what a `Block` optimizes;
sub-`Block` objectives are summed into the father's ([Chapter 5](05-variable-constraint-objective.md#ch-5)).

**Pareto-optimal cut.** A Benders cut not dominated, on the feasible set
of the master, by any other cut given by optimal dual solutions of the
same subproblem ([§14.6](14-lag-benders-bfunction.md#sec-14-6)).

**Phase one (of a `BendersBFunction`).** A second `BendersBFunction`
whose inner `Block` minimizes the weighted violation of the coupling
constraints of a copy of the original one, and whose linearization gives
the feasibility cut when the inner `:Solver` provides no unbounded dual
direction ([Chapter 14](14-lag-benders-bfunction.md#ch-14)).

**Physical representation.** The problem data of a `:Block` in its
natural, semantic form (a graph and costs for `MCFBlock`; weights,
profits, capacity for `BinaryKnapsackBlock`). Dictated by the problem,
not chosen; read directly by specialized `Solver`s. The counterpart of
the *abstract representation* ([Chapter 7](07-physical-abstract.md#ch-7)).

**Plugin.** The shared library of a module, together with its table
(a `.smspp` file naming the classes it registers), which the factories
load when asked for a `:Block` or a `:Solver` that the program is not
linked to; the tables are looked for in `SMSpp_PLUGIN_PATH` and in the
directories of the build and of the installation (since SMS++ 0.9.0;
[§18.8](18-factories-netcdf.md#sec-18-8)).

**Plugin table.** A file with extension `.smspp`, written by the build
next to a plugin, each of whose lines names a `:Block` or a `:Solver`
and the plugin that registers it; the tables are looked for first in the
directories of `SMSpp_PLUGIN_PATH`
([§18.8](18-factories-netcdf.md#sec-18-8)).

**PolyhedralFunction.** A `C05Function` that *is* a polyhedral
(piecewise-linear) function, useful as the recipient of
externally-generated cuts ([Chapter 13](13-function-family.md#ch-13)).

**Primal recovery.** The solve, within `PrimalProximalHeur`, of the
problem restricted by fixing the binary `Variable`s at a rounded point,
coupling constraints included, which turns that point into a feasible
solution ([Recipe R4](R4-cfl-lagrangian.md#rec-R4)).

**Query.** An adapter of a const member function of a `:Block`,
registered and looked up by name in the methods factory as the setters
are, that reads data back from the `:Block` ([Chapter
15](15-methods-factory.md#ch-15)).

**R3 Block.** A "Reformulation, Relaxation, or Restriction" `Block`
produced by another `Block` to represent an algorithmic transformation
of itself, kept in sync (where required) by the `map_*` methods and
`UpdateSolver`. CFL's flow relaxation as an `MCFBlock` is the running
example ([Chapter 10](10-r3block.md#ch-10)).

**R3BSolver.** A `Solver` that solves the `Block` it is attached to
through one of its R3 Blocks, configured from file only: the extra
`Configuration` of its `ComputeConfig` gives how to make the R3 Block,
its `BlockConfig` and the `BlockSolverConfig` of its inner `Solver`;
`Modification`s are mapped forward as they come, solutions are mapped
back, and `intR3BKind` chooses which bounds of the R3 Block are bounds of
the `Block` (since SMS++ 0.9.0; [Chapter 10](10-r3block.md#ch-10)).

**Serious step / null step.** The two outcomes of an iteration of a
stabilized method: the stability center moves to the trial point when
this gives a sufficient decrease of the function, and stays where it is
otherwise, the new linearizations improving the model
([§19.3](19-nondifferentiable.md#sec-19-3)).

**Size Variable.** The `Variable` standing for a size parameter $k$
that multiplies some data of a `:Block`, owned by it
(`get_size_variable()`) or given by another `Block`
(`set_size_variable()`); a mirror can be sized by one
([§10.4](10-r3block.md#sec-10-4)).

**sol_type.** The status enum returned by `compute()`: `kOK`,
`kInfeasible`, `kUnbounded`, `kStopTime`, `kStopIter`, `kCutOff`, and
the values $\ge$ `kError`, among which `kLowPrecision`
([Chapter 6](06-solver.md#ch-6), [§B.2](B-writing-solver.md#sec-B-2)).

**Solution.** A `Block`-specific (not `Solver`-specific) object that
can read a solution from / write it into a `Block`, serialize it, and
be linearly combined; abstract forms (`ColVariableSolution`,
`RowConstraintSolution`) and physical forms
(e.g., `MCFSolution`) coexist ([Chapter 9](09-solution.md#ch-9)).

**Solver.** Any algorithm able to `compute()` on a `Block`; specialized
`Solver`s exploit a specific `:Block`'s structure, general-purpose ones
operate on the abstract representation. Many `Solver`s may be attached
to one `Block` ([Chapter 6](06-solver.md#ch-6)).

**Stability center.** The point around which the master problem of a
stabilized method is stabilized; it moves only at a serious step
([§19.2](19-nondifferentiable.md#sec-19-2)).

**State.** The checkpointable internal state of a
`ThinComputeInterface` (typically a `Solver`), used for
warm-starting and reoptimization across `compute()` calls
([Chapter 17](17-parallel.md#ch-17)).

**Sub-Block.** A statically-nested `Block` inside another; data is
split between a `Block` and its sub-`Block`s, and `Modification`s flow
upward to the `Solver`s of every ancestor ([Chapter 12](12-sub-block.md#ch-12)).

**ThinComputeInterface / ThinVarDepInterface.** The two lightweight
mix-in interfaces underlying the framework: the first gives an object a
`compute()` and a parameter machinery (so `Solver`, `Function`,
`Constraint` and `Objective` are all computable); the second gives an
object the notion of being "active" in a set of `Variable`s (so
`Constraint`, `Objective` and `Function` track their variables) (Chapters [5](05-variable-constraint-objective.md#ch-5), [6](06-solver.md#ch-6), [13](13-function-family.md#ch-13)).

**UndoChange.** The `Change` returned by `apply()` with `doUndo`, which
restores the state before that `apply()`; UndoChanges applied in
reverse order restore any earlier state ([Lemma 16.1](16-change.md#lem-16-1),
[Chapter 16](16-change.md#ch-16)).

**UpdateSolver.** A `Solver` whose job is not to optimize but to
forward `Modification`s from one `Block` to another, which keeps an R3
Block in sync with its original as far as the mapping methods allow
([Lemma 10.4](10-r3block.md#lem-10-4), [Chapter 10](10-r3block.md#ch-10)).

**Variable.** The primitive representing a decision variable;
`ColVariable` is the concrete single-variable form. A `Function` never
copies a `Variable`, it references it by address ([Chapter 5](05-variable-constraint-objective.md#ch-5)).

**Weight (of a `Block`).** The factor by which the `Objective` of a
`Block` (`set_objective_weight()`), or the coefficients of its
`Variable`s in the `Constraint`s outside of it
(`set_variable_weight()`), count in the problems of its ancestors,
1 by default; a `:Solver` attached to the `Block` itself does not
see it. A change issues a `BlockWeightMod`, refused if a `:Solver`
of an ancestor does not handle it ([§12.2](12-sub-block.md#sec-12-2)).
