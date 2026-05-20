# Glossary

A compact, alphabetical reference for the SMS++ terms used in this
manual. Each entry gives the working definition and a pointer to the
chapter where the term is developed; class names link, where useful,
to the chapter rather than to Doxygen, which remains the authoritative
API reference.

**Abstract representation.** The explicit `Variable` / `Constraint` /
`Objective` description of a `:Block`, built on demand by the
`generate_abstract_*()` methods. General-purpose `Solver`s read it;
specialised ones usually do not. The counterpart of the *physical
representation* (Chapter 7).

**AModification.** The abstract base for `Modification`s that describe
a change made through the *abstract* face of a `:Block` (a `Variable`,
`Constraint`, `Function` …). Its `concerns_Block()` is `true` until
the `:Block` has reflected the change into its physical data
(Chapter 8).

**BendersBFunction.** A `Function` that is *also* a `Block`, computing
the value function of an inner "base" `Block` parameterised by an
affine map `M(y) = A y + b` applied to the right/left-hand sides of
some of its `RowConstraint`s. Each evaluation solves the inner
`Block`; its linearizations are Benders optimality or feasibility
cuts (Chapter 14).

**Block.** The cornerstone abstraction: a nested unit carrying the
*semantics* of a mathematical sub-problem and, on demand, an abstract
representation of it. A `Block` has a father, any number of static
sub-`Block`s, `Variable` / `Constraint` / `Objective` groups, and any
number of registered `Solver`s (Chapter 4).

**BlockConfig.** A `Configuration` that configures the *generation* of
a `:Block`'s abstract representation; the `[C/O/R]BlockConfig` family
selects which constraints (**C**), objective (**O**) and whether the
configuration recurses into sub-`Block`s (**R**) are built
(Chapter 11).

**BlockSolverConfig.** A `Configuration` that says which `Solver`s are
attached to a `Block` and with which parameters; switching solver is
"one line" of `BlockSolverConfig` (Chapters 6, 11).

**C05Function / C15Function.** Refinements of `Function` exposing,
respectively, first-order information (values and linearizations, not
necessarily from continuous gradients) and second-order information
(partial Hessians) (Chapter 13).

**CDASolver.** "Convex Duality Aware" `Solver`: one that, besides a
primal solution, produces (possibly several) dual solutions / valid
bounds. `MCFSolver` and the `MILPSolver` family are `CDASolver`s
(Chapter 6).

**Change.** *Status — beta.* A serialisable, transmissible, often
invertible description of an edit to a `:Block`; unlike a
`Modification` (a notification of a change that has already happened),
a `Change` carries the data needed to *apply* the edit to a fresh
`:Block` and to undo it (Chapter 16).

**ColVariable.** The concrete `Variable` representing a single real or
integer decision variable, with a type (`kContinuous`,
`kBinary`, `kInteger`, …), a value, and fix/unfix state
(Chapter 5).

**compute().** The method, inherited from `ThinComputeInterface`, by
which a `Solver` (or a `Function`, or any computable object) performs
its computation and returns a `sol_type` status (Chapter 6).

**concerns_Block().** A flag on a `Modification` that is `true` while a
change made through the abstract face still has to be folded back into
the physical data; the `:Block`'s `add_Modification()` resets it once
done (Chapter 8).

**Configuration.** A tree-shaped object mirroring the `Block` tree that
parameterises generation, solving, and computation;
`SimpleConfiguration<T>`, the `[C/O/R]BlockConfig` family,
`BlockSolverConfig`, and `ComputeConfig` are its concrete forms
(Chapter 11).

**Constraint.** A primitive living inside a `Block`. `RowConstraint`
has a real-valued left/right-hand side; `FRowConstraint` carries a
`Function`; `OneVarConstraint` (e.g. `LB0Constraint`) bounds a single
`ColVariable` (Chapter 5).

**Dynamic vs static.** Static `Variable` / `Constraint` groups are
fixed in number and may live in vectors or `boost::multi_array`s;
dynamic ones may be added and removed at run time and must live in
`std::list`s, because a SMS++ object's identity is its memory address
(Chapters 4, 5).

**Factory.** A class-name (`std::string`) → object map allowing a
`:Block` / `:Solver` / `:Configuration` / `:Modification` / `:Change`
/ `:Function` to be created by name and reconstructed from netCDF;
registered with the `SMSpp_insert_in_factory_*` macros (Chapter 18).
Distinct from the *methods factory* (below).

**FRealObjective.** The concrete `Objective` carrying a `Function` to
be minimised or maximised (Chapter 5).

**Function.** A real-valued (extended-real) function of `ColVariable`s
that must be `compute()`-d; the family runs `Function → C05Function →
C15Function` with leaf implementations such as `LinearFunction`,
`DQuadFunction`, and `PolyhedralFunction` (Chapter 13).

**Janus discipline.** The rule that a `:Block`'s two faces (physical
and abstract) be kept consistent: a `chg_*()` mutator updates the
physical data and mirrors the change into the abstract one, while a
change arriving through the abstract face is intercepted by
`add_Modification()` and folded back into the physical data
(Chapter 8).

**LagBFunction.** A `Function` that is *also* a `Block`, computing the
Lagrangian (dual) function of an inner "base" `Block` with respect to
a set of linear terms; its linearizations correspond to
(approximately) optimal inner solutions, which is what lets a bundle
method optimise the dual (Chapter 14).

**Leaf Block.** A `Block` with no sub-`Block`s (`MCFBlock`,
`BinaryKnapsackBlock`); the opposite of a *non-leaf* (composite)
`Block` such as `CapacitatedFacilityLocationBlock` (Chapters 4, 12).

**Linearization.** A first-order piece of information produced by a
`C05Function` at an evaluation point — for a convex function, a
subgradient defining a supporting hyperplane; pools of linearizations
drive bundle methods and Benders/Lagrangian schemes (Chapters 13, 14).

**Modification.** A lazily-propagated notification that something
changed in a `Block`; the framework appends it to the pending list of
every `Solver` registered with the `Block` or any ancestor, and the
`Solver` consumes it when it reacts. `NBModification` is the wholesale
"reload" notification (Chapter 8).

**netCDF.** The hierarchical binary format SMS++ uses for
serialisation; nested `Block`s map to nested netCDF groups, inspectable
with `ncdump` / `ncview` / Panoply (Chapter 18).

**Objective.** The primitive expressing what a `Block` optimises;
sub-`Block` objectives are summed into the father's (Chapter 5).

**Physical representation.** The problem data of a `:Block` in its
natural, semantic form (a graph and costs for `MCFBlock`; weights,
profits, capacity for `BinaryKnapsackBlock`). Dictated by the problem,
not chosen; read directly by specialised `Solver`s. The counterpart of
the *abstract representation* (Chapter 7).

**PolyhedralFunction.** A `C05Function` that *is* a polyhedral
(piecewise-linear) function, useful as the recipient of
externally-generated cuts (Chapter 13).

**R3Block.** A "Reformulation, Relaxation, or Restriction" `Block`
produced by another `Block` to represent an algorithmic transformation
of itself, kept in sync (where required) by the `map_*` methods and
`UpdateSolver`. CFL's flow relaxation as an `MCFBlock` is the running
example (Chapter 10).

**Solution.** A `Block`-specific (not `Solver`-specific) object that
can read a solution from / write it into a `Block`, serialise it, and
be linearly combined; abstract forms (`ColVariableSolution`,
`RowConstraintSolution`) and physical forms
(`MCFSolution`, …) coexist (Chapter 9).

**Solver.** Any algorithm able to `compute()` on a `Block`; specialised
`Solver`s exploit a specific `:Block`'s structure, general-purpose ones
operate on the abstract representation. Many `Solver`s may be attached
to one `Block` (Chapter 6).

**sol_type.** The status enum returned by `compute()`: `kOK`,
`kInfeasible`, `kUnbounded`, `kLowPrecision`, `kStopTime`,
`kStopIter`, values `≥ kError`, etc. (Chapter 6).

**State.** A `Solver`'s checkpointable internal state, used for
warm-starting and reoptimization across `compute()` calls
(Chapter 17).

**Sub-Block.** A statically-nested `Block` inside another; data is
split between a `Block` and its sub-`Block`s, and `Modification`s flow
upward to the `Solver`s of every ancestor (Chapter 12).

**ThinComputeInterface / ThinVarDepInterface.** The two lightweight
mix-in interfaces underlying the framework: the first gives an object a
`compute()` and a parameter machinery (so `Block`, `Solver`,
`Function` are all computable); the second gives an object the notion
of being "active" in a set of `Variable`s (so `Constraint` and
`Function` track their variables) (Chapters 5, 6, 13).

**UpdateSolver.** A pseudo-`Solver` whose job is not to optimise but to
forward `Modification`s from one `Block` to another, the canonical glue
that keeps an `R3Block` in sync with its origin (Chapter 10).

**Variable.** The primitive representing a decision variable;
`ColVariable` is the concrete single-variable form. A `Function` never
copies a `Variable`, it references it by address (Chapter 5).
