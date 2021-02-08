# SMS++ API reference

![To boldly model (and solve) what no one has modeled (and solved) before](SMSpp_logo_mid_noback.png)

SMS++ is a set of C++ classes intended to provide a system for modeling complex,
block-structured mathematical models (in particular, but not exclusively,
single-real-objective optimization problems), and solving them (in particular,
but not exclusively) via sophisticated, structure-exploiting algorithms such
as decomposition approaches and structured Interior-Point methods.

## Getting started

- The [SMS++ Project repository](https://gitlab.com/smspp/smspp-project)
  contains all the information on how to build and install the SMS++ Project.

- The [SMS++ Project Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)
  contains troubleshooting information and additional guides for
  developers and maintainers.

## A general description of SMS++

### Design Objectives of SMS++

The aim of SMS++ is to provide a system catering for the concepts that:

- the model and the solution approaches may need be tightly integrated;

- within complex solution approaches, different solvers may be needed with
  reference to the same (part of the) model;

- within complex solution approaches, different parts of the model can
  change arbitrarily, and solvers need to be able to exploit the previous
  solution information and the list of changed data in order to (try to)
  reoptimize efficiently (insomuch as it is possible);

- different solvers may need different representations of the same
  underlying (part of the) model, which therefore need be kept in
  synch when they change;

- complex solution approaches may proceed in parallel.

The current design of SMS++ is still partly tentative; a number of features
have already been identified where substantial improvements are likely
necessary. Yet, insomuch as it offers a number of features that, to the best
of our knowledge, are not provided by any of its alternatives, we believe it
reasonable to make it available. Feedback on the design decisions is highly
welcome, and the system may already offer compelling benefits in some use
cases.

### Alternatives to SMS++

Current systems for modeling mathematical models typically falls into two
opposite approaches:

- *General-purpose Algebraic Modeling Languages*, commercial ones like
  [AMPL](https://ampl.com) and [GAMS](https://www.gams.co/), or open-source
  ones like [Coliop](http://www.coliop.org) and [ZIMPL](http://zimpl.zib.de).
  These try to abstract away as much as possible the mathematical model from
  the underlying solver, thereby taking away from the user all the effort
  from interfacing with specific solvers tackling their idiosyncrasies.
  However, in order to do to they typically "flatten" all the structure
  available in the model, making it almost unreachable from the underlying
  solver. Some extensions have been proposed for specific forms of structure,
  mainly the block one, e.g. in the [Structured Modeling Language
  (SML)](https://www.maths.ed.ac.uk/ERGO/sml). An alternative approach has
  been to re-construct the structure backwards from the "flat" model, as
  [Generic Column Generation (GCG)](http://www.or.rwth-aachen.de/gcg) does,
  possibly taking hints from the user about how to do that.

- The alternative has typically been to *directly interface to a specific
  solver via its APIs*; all modern general-purpose (and specialized)
  solvers, both commercial ones like
  [Cplex](https://www.ibm.com/analytics/cplex-optimizer),
  [GuRoBi](http://www.gurobi.com) and [MOSEK](https://www.mosek.com), and
  open-source ones like [Cbc](https://projects.coin-or.org/Cbc) and
  [SCIP](http://scip.zib.de), offer one or more APIs, usually for
  different languages (among which, surely C/C++). This allows to directly
  address all the structure-exploiting features of the solver, if any, but
  directly exposed the user to its complexity and idiosyncrasies.

- A somewhat "in the middle" approach is that of *modeling systems embedded
  in general purpose languages*, such as
  [FlopC++](https://projects.coin-or.org/FlopC++) and
  [COIN-Reharse](https://github.com/coin-or/Rehearse) for C++,
  [PuLP](https://pythonhosted.org/PuLP) and
  [Pyomo](http://www.pyomo.org) for Python,
  [JuMP](https://github.com/JuliaOpt/JuMP.jl) for Julia and
  [YALMIP](https://yalmip.github.io) for Matlab. This allows to better deal
  with parts of the model that have specific structure and allow for
  specialized solution method, and thereby implement sophisticated
  structure-exploiting algorithms. However, at their hearth these systems
  typically still construct "flat" models, and have little explicit support
  for managing the structure in the model. Some exception to this rule are
  [StructJuMP](https://github.com/StructJuMP/StructJuMP.jl), an extension
  of JuMP tailored to two-stage stochastic optimization problems, and
  [BlockDecomposition](https://github.com/atoptima/BlockDecomposition.jl),
  yet another JuMP extension developed to work with the
  [Coluna](https://github.com/atoptima/Coluna.jl) package, perhaps the
  closest in spirit to SMS++.

### Main features of SMS++

SMS++ mostly belongs to the latter group, but pushes a number of features
further by trying to bring closer the model and the solution algorithms
(after all, mathematical optimization used to be known as "mathematical
programming" for a good reason). It is a modeling system based on a set
of C++ classes, where a model is represented as a `Block`: a base abstract
class meant to represent the general concept of "a part of a mathematical
model with a well-understood identity". That is, derived classes will in
principle each represent a model with a specific structure (say, a
Knapsack, a Traveling Salesman Problem, a SemiDefinite program, ...), which
in principle will allow to develop specialised solution algorithms for their
solution. These are supposed to be implemened as classes derived from
`Solver`, an abstract base class setting the general interface between a
`Block` representing a mathematical model and any algorithm designed to
solve (i.e., produce solutions for) it. The slightly specialized derived
`CDASolver` class represents solution algorithms that exploit convex
structures (and, in particular, duality) in the model.

Upon this quite classical setting, SMS++ adds a number of twists:

- A `Block` can have any number of inner `Block`, recursively, representing
  separate parts of the model. As a consequence, `Solver` can be attached
  to the individual inner `Block` (recursively ) of a larger `Block`,
  easing the implementation of solution algorithms based on advanced
  techniques (like decomposition ones).

- Specialized `Solver` will be able to exploit the *semantically defined*
  structure of the `Block`, i.e., the "physical representation" of the
  instance (a set of items with weights, a weighted graph, a list of
  square matrices, ...) to develop their solution approaches. However, each
  specialized `Block` is also supposed to be able to provide an "abstract
  representation" of itself in terms of its `Variable`, `Constraint` and
  `Objective`, in order to be able to either exploit general-purpose `Solver`
  for large classes of mathematical models (say, Mixed-Integer Linear
  Programs), or to implement solution methods that mix general-purpose
  approaches and ad-hoc `Solver` for more specific structures.

- `Variable`, `Constraint` and `Objective` are *abstract* classes that try
  to establish the minimal possible interface of a modeling system, making
  as little assumptions as possible on the actual form of the corresponding
  mathematical objects. This should allow to extend the system to many
  different applications and mathematical structures.

- `Variable` and `Constraint` of a `Block` can be both static and dynamic,
  in order to allow strategies where they can be dynamically generated
  while providing the `Solver` with information about which ones are never
  going to change. Both static and dynamic `Variable` / `Constraint` can be
  single or arranged in `std::vector` or `boost::multi_array`; dynamic ones
  are always within `std::list` since *the "name" of a `Variable` /
  `Constraint` is its pointer*, and therefore `Variable` / `Constraint` can
  never be moved in memory.

- To allow tackling actual applications, a "mininal" set of  *concrete*
  implementations are provided by the "core" system, such as
  `ColVariable` (a single real `Variable`, possibly with integrality
  restrictions), `RowConstraint` (a `Constraint` of the form "LHS <= ( single
  real expression ) <= RHS", with a corresponding *real dual value*), and
  `RealObjective` (a real-valued objective function). Also, a general
  `Function` class is provided for expressing a very general real-valued
  function concept, together with its derived `C05Function` (providing
  linearizations, i.e., first-order information, but not necessarily
  continuous one) and `C15Function` (providing quadratic models, i.e.,
  second-order information, but not necessarily continuous one). With this,
  `FRowConstraint` and `FRealObjective` are defined as `RowConstraint`
  and `RealObjective` taking a generic `Function` to implement them. A few
  "simple" `Function` are also provided, like `LinearFunction` and
  `DQuadFunction`, which then allows to represent the all-important class
  of Mixed-Integer Linear Problems. Also, the class `OneVarConstraint` and
  some further specializations is separately derived from `RowConstraint`
  to implement the case where the "single real expression" is a single
  `ColVariable`, i.e., "box constraints". However, all these are just one
  possible implementation of the corresponding concept, with the
  development of alternatives being possible and encouraged.

- By means of the above classes SMS++ can also be used in a way akin to
  (althugh somewhat elegant than) a standard algebraic modelling language;
  a specific `AbstractBlock` is provided that, unlike regular `Block`,
  allows to only have the "abstract representation" and it to be set by
  "the user", without the class making assumptions on it save (for the
  time being) on the kind of supported `Variable`, `Constraint` and
  `Objective` (the concrete ones above).

- Changes in the data of a `Block`, be it the "physical representation" or
  the "abstract representation" (`Variable`, `Constraint`, `Objective`,
  `Function` ...), are treated in a "fully lazy" way, i.e., are recorded into
  objects of (classed derived from the base) class `Modification`, that are
  then shipped to all `Solver` objects registered to the `Block` and all its
  ancestors. `Modification` are sent around via smart pointers so that memory
  management is trivial. The `Modification` mechanics also provides crucial
  support for the mechanism whereby if the "abstract representation" of a
  `Block` is changed, then the "physical representation" must be modified
  accordingly.

- `Block` supports coarse-grained asynchronous computation by providing
  `lock()` and `read_lock()` methods; these automatically perform the same
  operation on all inner `Block`. Lock operations establish an "owner",
  which allows to ensure that the `Block` is not manipulated by different
  actors running in the same thread (`std::mutex` is used for managing
  different threads, but it would not work in that context). `Solver` also
  supports asynchronous computation by having the list of outstanding
  `Modification` only being modified under and "active guard" (`std::atomic`)
  and a mechanism whereby it can "borrow" the "identity" of an existing
  "owner" for the case where a `Solver` attached to a `Block` needs to
  delegate tasks to `Solver` attached to inner `Block` (recursively).

- `Block` supports (although only in the interface, not in any practical
  implementation) the concept that a `Block` can produce different versions
  of itself that are either equivalent (a Reformulation), a Relaxation or
  a Restriction. The interface caters for the fact that both solution
  information (see `Solution`) and `Modification` can be mapped back and forth
  from a `Block` to any one of its "R3 `Block`", although this is all predicated
  on the fact that the `Block` itself implements this.

- Since `Block` is an arbitrarily large tree-structured object, configuring
  it and attaching all needed `Solver` to all the necessary inner `Block` is
  a complex task. For this a `Configuration` class is provided that is
  intended to represent objects for collecting many (structured) algorithmic
  parameters. The class is the basis for `BlockConfig` (collecting parameters
  for different tasks that a `Block` has to accomplish), `ComputeConfig`
  (collecting algorithmic parameters for `Solver` and "complex" `Function`),
  and `BlockSolverConfig` (collecting all the `Solver` attached to a `Block`
  and all their `ComputeConfig`). `BlockConfig` and `BlockSolverConfig` have
  derived classes that allow to cater for the arbitrary tree-structure.

- The solution status of a `Block` (the value of its `Variable`) can be saved
  in appropriate objects derived from the base `Solution` class, and then
  read back from these in the `Block`. The `Solution` objects can possibly save
  any subset of the information (as dictated by a `Configuration` object) and
  dual solution if available. General `ColVariableSolution`,
  `RowConstraintSolution` and `ColRowSolution` are provided for doing this
  only using the "abstract representation", for primal, dual or both
  solution information respectively.

- `Block`, `Solver` and `Configuration` all provide a factory to dynamically
  generate object of any derived class by just passing it the string of the
  classname (provided the class is registered to the factory, for which support
  is provided with some macros).

- `Block` and `Configuration` are supposed to be able to serialise and
  deserialise themselves in particular using the industry-standard
  structured [netCDF](https://www.unidata.ucar.edu/software/netcdf) files;
  this may be useful also for the future planned support to distributed
  coarse-grained asynchronous computation on non-shared-memory systems.

### Long-term Objectives of SMS++

The aim of SMS++ is primarily to provide a modelling system that allows
experienced users to develop solution approaches advancing the state-of-the-art
of optimization.

However, a significant objective is also to promote a higher re-use of
solution approaches for specific problems. A large amount of highly valuable
research is routinely undertaken into many different specific mathematical
structures (of optimization problems) that leads to significant results. Yet,
it is often the case that these results have a much smaller impact than they
could have due to the fact that the research in question requires complex
implementations. These are performed by the original researchers, but in
many cases they end up not being usable by other interested parties due to
the lack of a standardised interface for "any" optimization problem
solved by "any" algorithm. By trying to provide at least some initial steps
in the direction of these, SMS++ hopes to provide means for valuable work
on specific structures to be re-used, with significant benefits for all the
involved parties and the community in general.
