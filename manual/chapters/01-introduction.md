# 1. Introduction {#ch-1}

## 1.1 What SMS++ is {#sec-1-1}

SMS++ is a C++20 software framework for *structured* mathematical
optimization, meant for researchers and practitioners who have to
build, combine and solve models whose structure is exploited by the
algorithms rather than ignored by them. This manual covers version
0.8.0, released in November 2026, which is distributed under the GNU
Lesser General Public License v3.0.

SMS++ rests on a small number of abstractions, none of which is
original in isolation: nested decomposition of a model, separation of
the semantics of a problem from its syntax, lazy propagation of
changes, and algorithmic reformulation. Less common (arguably) is that
SMS++ pursues all of them at the same time, in a single class
hierarchy, and at a level of generality that does not assume a
specific class of problems.

The central abstraction is the class
[`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html),
which represents "a (fragment of a) mathematical model with a
well-understood semantic". A concrete `:Block` derived from it encodes
a class of problems with a specific structure (a min-cost flow
problem, a knapsack problem, a Unit Commitment problem, a stochastic
multi-stage program, an investment problem, and so on). A `Block` can
contain any number of `Variable`s and `Constraint`s and (at most) one
`Objective`, organized in *groups* (single objects, vectors,
multi-dimensional arrays, or lists for the dynamic case); it can also
contain any number of sub-`Block`s, recursively, and it can in turn be
a sub-`Block` of a father `Block`. Each `Block` admits two
representations of its model: a *physical* one, i.e., the data of the
problem in their natural form (a graph, a table of weights and
profits, a network of generating units), and an *abstract* one, i.e.,
the explicit `Variable`s, `Constraint`s and `Objective` that a
general-purpose solver expects. Both representations coexist, and the
abstract one is built on demand.

A
[`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html)
is, in SMS++, anything that can compute on a `Block`, and any number
of them can be attached to the same `Block`. A specialized `:Solver`,
written for a specific `:Block`, reads the physical representation
directly, while a general-purpose one reads the abstract
representation; the two coexist without either of them having to
compromise. Their interface is uniform: a `Solver` can report
optimality, infeasibility, unboundedness, or a stop due to a limit on
time or iterations, it can deliver on demand any number of (possibly
approximately optimal and approximately feasible) solutions, and it
can be invoked synchronously or asynchronously (through
`compute_async()`).

Changes to a `Block` reach the `Solver`s listening to it (or to one of
its ancestors) through the
[`Modification`](https://smspp.gitlab.io/smspp-project/d1/d3c/class_s_m_spp__di__unipi__it_1_1_modification.html)
mechanism: each `Solver` keeps a list of the `Modification`s that it
has not processed yet, and it reacts to them the next time it is
invoked (reoptimizing, if it can). A change made through either
representation produces the `Modification`s that concern the other one
too; we call this the *Janus discipline*, which is one of the central
ideas of SMS++ (cf. [Chapter 8](08-modification-janus.md#ch-8)). A
newer and complementary concept, `Change`, adds to the notification
the data needed to *apply* the change to a different copy of the
`Block` and, if requested, to *undo* it. **Status — beta**: at version
0.8.0 only a few `:Change` classes exist (those of
[`BinaryKnapsackBlock`](https://smspp.gitlab.io/smspp-project/dc/d2f/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block.html)
and of `SATBlock`, plus `AbstractChange` and `GroupChange` in the
core, the former of which changes the abstract representation of any
`Block`), and the interface may change.

SMS++ also supports the *algorithmic reformulation* of a `Block`
through the `R3Block` mechanism, by which a `Block` produces another
`Block` that represents a reformulation, a relaxation or a restriction
of the original one, together with the means to move solutions and
`Modification`s between the two (cf. [Chapter
10](10-r3block.md#ch-10)). It supports *configuration* through the
recursive `BlockConfig` and `BlockSolverConfig` classes (cf. [Chapter
11](11-configuration.md#ch-11)), and *coarse-grained parallel
computation* through recursive locks, asynchronous computation and the
checkpointing of `Solver`s (cf. [Chapter 17](17-parallel.md#ch-17)).
Finally, it serializes `Block`s, `Solution`s, `Configuration`s and
`Change`s to netCDF, a binary hierarchical file format whose groups
map naturally onto the nested structure of a tree of `Block`s (cf.
[Chapter 18](18-factories-netcdf.md#ch-18)).

The concrete `:Block` classes of the public modules cover, among
others, min-cost flow (`MCFBlock`), multicommodity flow and network
design (`MMCFBlock`), capacitated facility location
(`CapacitatedFacilityLocationBlock`), and the knapsack problem with
binary and continuous items (`BinaryKnapsackBlock`). They also cover
Unit Commitment in its many variants (`UCBlock`, `ThermalUnitBlock`,
`BatteryUnitBlock`, `IntermittentUnitBlock`, `DCNetworkBlock`,
`SlackUnitBlock`), stochastic multi-stage programs (`SDDPBlock`,
`StochasticBlock`, `TwoStageStochasticBlock`,
`MultiStageStochasticBlock`), strategic investment
(`InvestmentBlock`), and several others (e.g., Support Vector Machines
in `SVMBlock`, SAT and weighted MaxSAT in `SATBlock`, the constraint
satisfaction problems of constraint programming in `CSPBlock`, hierarchical
quadratic programs in `HQPBlock`). As for the `:Solver` classes, they
comprise wrappers of external libraries: `MCFSolver`, which wraps
`MCFClass`, `MCFLemonSolver`, which wraps abLEMON, the `MILPSolver`
family, which wraps CPLEX, Gurobi, MOSEK, HiGHS, SCIP and a number of
convex QP solvers, and `SDDPSolver`, which wraps StOpt. They also
comprise specialized solvers native to SMS++
(`DPBinaryKnapsackSolver`, `ThermalUnitDPSolver`, `SDDPGreedySolver`),
and generic native solvers that exploit the structure of the model
(`LagrangianDualSolver` and its derived class `PrimalProximalHeur`,
`BundleSolver` and its parallel variant `ParallelBundleSolver`,
`SubGradSolver`, `FrankWolfeSolver`, `BendersDecompositionSolver`,
`BranchAndXSolver`).

This manual is about the framework, not about the catalogue. Three
`:Block`s are used throughout as worked examples (`MCFBlock`,
`BinaryKnapsackBlock` and `CapacitatedFacilityLocationBlock`), since
together they exercise the major concepts of SMS++ while remaining
small enough to be described in full.

## 1.2 What SMS++ is not {#sec-1-2}

We state a few things that SMS++ is *not*, with which the reader can
decide quickly whether the framework fits the task at hand.

SMS++ is not an algebraic modeling language. A `:Block` is C++ code,
written explicitly by its author. The framework does provide some
conveniences in the style of a modeling language, the main one being
the class
[`AbstractBlock`](https://smspp.gitlab.io/smspp-project/d3/dbd/class_s_m_spp__di__unipi__it_1_1_abstract_block.html),
which reads `.mps` and `.lp` files and exposes them as a `Block` with
a single ("syntactic") abstract representation. However, this is a
convenient entry point rather than the intended use of the framework,
which is to write once, in C++, a `:Block` that captures the
*structure* of a class of problems, and then to reuse it across
applications.

SMS++ is not meant for the casual user. SMS++ is written primarily for
experts in algorithms, and the audience of an algebraic modeling
language (practitioners who write a model in the form $\min \{\, c x :
A x \le b \,\}$ and hand it to a solver) is not its primary audience.
End users may nonetheless profit from the catalogue of existing
`:Block`s and `:Solver`s: one who has to solve a Unit Commitment
problem can load an instance into a
[`UCBlock`](https://smspp.gitlab.io/smspp-project/df/d40/class_s_m_spp__di__unipi__it_1_1_u_c_block.html)
and attach a `:MILPSolver` to it without ever writing a `:Block`. Even
in this case, however, the idioms of the framework and the role of the
configuration are not always (quite) intuitive, and we recommend
reading at least Part I of this manual.

SMS++ does not have a stable interface. Version 0.8.0 is the current
public release, the framework is under active development, and further
changes, possibly not backward compatible, are expected. Its concrete
`:Block`s and `:Solver`s are extensively tested by the test suites of
the project (the `tests/` repository and the `test/` directory of each
module), but their public interfaces may still evolve; the manual
marks the features in beta explicitly, while the mature ones carry no
mark.

SMS++ is not tied to one solver. Besides its native specialized
solvers, the framework relies on wrappers of a range of external
libraries: CPLEX, Gurobi, MOSEK, HiGHS, SCIP and several convex QP
solvers (through the
[`MILPSolver`](https://smspp.gitlab.io/smspp-project/d7/d97/class_s_m_spp__di__unipi__it_1_1_m_i_l_p_solver.html)
family), `MCFClass` and abLEMON for min-cost flow, and StOpt for
Stochastic Dual Dynamic Programming (SDDP). Which of these external
dependencies to install is a choice of the user (cf. [Chapter
2](02-installation.md#ch-2)).

## 1.3 Audience and prerequisites {#sec-1-3}

We address three audiences, listed in decreasing order of
specificity.

The primary audience consists of *developers of algorithms*:
researchers, PhD students and practitioners who want to implement a
solution method that exploits the structure of an existing `Block`, or
to extend the framework with a new `:Block`, `:Solver`,
`:Modification` or `:Change`. For them, the manual is meant to fill
the gap between the slide decks that accompany the project and the
method-by-method Doxygen reference, by explaining the idioms and the
conventions that make SMS++ coherent.

A secondary audience consists of *authors of `Block`s*: researchers
who have to express a new class of structured problems as a `:Block`
and to expose it to the existing `:Solver`s. They have to understand
the framework well enough to take the decisions that come with a new
`:Block` (what is "physical" and what is "abstract", which
`:Modification`s to issue, what form of `:Solution` to produce). They
are best served by reading the complete manual, with particular
attention to Part II (the concepts) and to [Appendix
A](A-writing-block.md#app-A) (writing a new `:Block`).

A third audience consists of *sophisticated end users*, who have a
problem in hand, find an existing `:Block` that fits it, and want to
solve it with one of the existing `:Solver`s. They can read
selectively: Part I, the relevant recipe of Part III, and the Doxygen
pages of the `:Block` and of the `:Solver` of interest.

The technical prerequisites are a working knowledge of C++20
(concepts, ranges, smart pointers, lambdas, range-based `for`), some
familiarity with CMake and, for the development loop, with classical
Make, and a working C++20 compiler (a recent `gcc`, `clang` or MSVC).
One also needs at least an operational understanding of the
optimization paradigms that the `:Block` of interest involves. Recipes
[R4](R4-cfl-lagrangian.md#rec-R4) and [R5](R5-cfl-benders.md#rec-R5)
of Part III also assume some familiarity with Lagrangian and Benders
decomposition.

## 1.4 How to read this manual {#sec-1-4}

This manual is organized in four parts, plus front and back matter; we
give the rationale of the organization here, with which the reader can
plan the reading sequence that best suits their needs.

Part I (Chapters [1](01-introduction.md#ch-1) to
[3](03-mental-model.md#ch-3)) sets the stage.
[Chapter 1](01-introduction.md#ch-1) is this introduction.
[Chapter 2](02-installation.md#ch-2) covers the installation and the
first build, and it is deliberately short, since the
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home) and
the [`README.md`](https://gitlab.com/smspp/smspp-project/-/blob/develop/README.md) of the umbrella project are the authoritative source
of the detailed steps. [Chapter 3](03-mental-model.md#ch-3) gives the
*mental model* of the framework: `Block` and `Solver`, the two
representations, the lifecycle of a `Block`, and the three running
examples. A reader of Part I only should have a coherent picture of
what SMS++ is, if not yet the means to write code against it.

Part II (Chapters [4](04-block.md#ch-4) to
[18](18-factories-netcdf.md#ch-18)) is the core of the manual. Each
chapter introduces one concept, defines it, and locates it in the
source tree and in the Doxygen reference; it then states (and proves)
the properties of the mechanism on which a user can rely, and
illustrates it with a short example of use built on one of the three
running examples. Also, the chapters are ordered in such a way that
each concept is introduced before it is used.

Part III (recipes R1 to R5) collects five complete programs that
combine the concepts of Part II to solve recognizable tasks. They
solve an instance of min-cost flow, reoptimize a knapsack, and solve a
capacitated facility location (CFL) problem in three different ways
(as a MILP, through its relaxation to min-cost flow, and by Lagrangian
relaxation); they also solve the CFL with `LagrangianDualSolver` and
`PrimalProximalHeur`, and with Benders cuts issued by a callback of
the MILP solver. Each recipe lists the concepts it uses, gives the
code, comments on it and reports the expected output.

Part IV (Appendices [A](A-writing-block.md#app-A) to
[D](D-concept-block-map.md#app-D)) is the guide to *extending* the
framework with a new `:Block`, `:Solver`, `:Modification` or
`:Change`. [Appendix A](A-writing-block.md#app-A) develops from
scratch a small but complete `BinPackingBlock`, while [Appendix
D](D-concept-block-map.md#app-D) maps each concept of the manual to
the running example and to the section that exercise it.

A reader who already knows the concepts and wants a working program
may read Part I and then jump to the relevant recipe of Part III,
which names the concepts it uses, going back to Part II only as
needed. Conversely, a reader who wants the complete picture should
read in order, and use the recipes as the runnable counterparts of the
concepts.

## 1.5 Relation to the Doxygen reference and to the SMS++ Wiki {#sec-1-5}

This manual is one of three sources of documentation of SMS++, each
with a role of its own.

The Doxygen reference, at <https://smspp.gitlab.io/smspp-project/>, is
the reference of the interface: the public classes, methods and fields
are documented there with its signature, parameters, return value,
preconditions, postconditions and exceptions (and, for the better
documented classes, with the mathematics of what it computes). Its
pages are generated from the comments in the source files, which are
kept in step with the code, and they are the place where one looks up
a specific method; the manual does not duplicate them, and it gives
the Doxygen page of each class where the class is introduced.

The SMS++ Wiki, at
<https://gitlab.com/smspp/smspp-project/-/wikis/home>, is the
authoritative source of the instructions to install, build, customize
and troubleshoot the framework. It describes how to fetch the umbrella
project and its submodules, how to configure CMake, how to enable the
optional interfaces to external solvers, and how to deal with the
issues of each platform. [Chapter 2](02-installation.md#ch-2) gives an
overview of the build system, and refers to the Wiki for the details.

This manual covers the concepts and the idioms of SMS++, together with
a few complete recipes. It explains why the framework is made the way
it is and how the abstractions fit together, which properties of each
mechanism one can rely upon (and under which hypotheses), and what a
few representative complete uses look like.

When the three sources disagree, the source code is the ultimate
authority, the Doxygen reference comes second and this manual third.
We have tried to keep the three consistent at the version covered
(0.8.0); in case of discrepancy, the reader is invited to open an
issue on the umbrella project.

## 1.6 A note on style {#sec-1-6}

A few stylistic choices recur throughout the manual. Where a feature
is in beta, or planned but not yet released, the corresponding
*Status* admonition marks it explicitly, and where SMS++ does not
support a use one might reasonably expect, the text says that too. We
do not compare SMS++ with other modeling systems: there is plenty of
comparative work in the literature, but the manual is about SMS++
alone.

Code appears in two forms only: the declarations of the methods that a
user calls, and short examples of use (how to load a `Block`, attach a
`Solver`, issue a `Modification`, write a configuration file). Each
example is written against the source tree of version 0.8.0, and
compiles against its headers. How a method is implemented is given,
where it matters, as pseudocode or in prose, and what a user can rely
upon is stated as a lemma (or a proposition) with its proof.

Claims about the behavior of a class point either to its Doxygen page
or to the file (and, where useful, the line) of the source where the
behavior is implemented or documented, and one can check each of them
by following the pointer.

With these preliminaries out of the way,
[Chapter 2](02-installation.md#ch-2) turns to the practical question
of how to fetch SMS++ and build it.
