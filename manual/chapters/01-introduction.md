# 1. Introduction {#ch-1}

## 1.1 What SMS++ is {#sec-1-1}

SMS++ is a C++-20 software framework for *structured* mathematical
optimization, designed primarily for researchers and practitioners who
need to build, combine and solve mathematical models in which the
problem structure is exploited algorithmically rather than ignored.
The version covered by this manual is **0.6.0**, released in December
2025, and is distributed under the GNU Lesser General Public Licence
v3.0.

The framework rests on a small number of abstractions, none of which
is original in isolation: nested model decomposition, separation of
problem semantics from problem syntax, lazy propagation of changes,
algorithmic reformulation. What is, in our view, less common is that
SMS++ pursues all of them at the same time, in a single coherent
class hierarchy, and at a level of generality that does not assume a
specific problem class.

The cornerstone of the framework is the abstract class
[`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html),
which represents "a (fragment of a) mathematical model with a
well-understood semantic". A concrete `:Block` derived from it
encodes a problem class with specific structure (a min-cost flow
problem, a knapsack problem, a unit-commitment problem, a stochastic
multi-stage program, an investment problem, ...). A `Block` can
contain any number of `Variable`s, `Constraint`s and a single
`Objective`, organised in *groups* (single, vectors, multi-dimensional
arrays, or lists for the dynamic case). It can also contain any
number of nested sub-`Block`, recursively, and it can in turn be a
sub-`Block` of a father `Block`. Each `Block` admits two
representations of the underlying mathematical model: a *physical*
one, consisting of the problem data in its natural form (a graph, a
table of weights and profits, a network of generation units...), and
an *abstract* one, consisting of the explicit `Variable`s,
`Constraint`s and `Objective` that a general-purpose solver would
expect. The two representations coexist; the abstract one is built on
demand.

A
[`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html)
is, in SMS++, anything that can compute on a `Block`. Any number of
`Solver` can be attached to the same `Block`. A specialised `:Solver`
written for a specific `:Block` reads the physical representation
directly; a general-purpose `:Solver` reads the abstract
representation; the two coexist with no compromise on either side.
The interface is uniform: a `Solver` can return optimality,
infeasibility, unboundedness, time-limited or iteration-limited
termination; it can deliver any number of (approximately optimal,
approximately feasible) solutions on demand; it can be invoked
synchronously or asynchronously through `compute_async()`.

Changes to a `Block` propagate to all `Solver` listening to it (and
to its ancestors) through the
[`Modification`](https://smspp.gitlab.io/smspp-project/d1/d3c/class_s_m_spp__di__unipi__it_1_1_modification.html)
mechanism: each `Solver` maintains a list of pending `Modification`s
that it consumes when it is next invoked, and reacts to them as best
it can (reoptimizing, if possible). The framework guarantees that a
change made via either representation produces the appropriate
`Modification` for the other representation too; we call this the
**Janus discipline**, and it is one of the central ideas of SMS++
([Chapter 8](08-modification-janus.md#ch-8)). A new, complementary concept, `Change`, augments the
`Modification` notification with the data needed to *apply* the
change to a different copy of the `Block` and, if requested, to
*undo* it. **Status — beta**: only a small number of `:Change` classes
exist at version 0.6.0 (in particular for
[`BinaryKnapsackBlock`](https://smspp.gitlab.io/smspp-project/dc/d2f/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block.html)),
and the interface is subject to change.

The framework supports *algorithmic reformulation* of a `Block`
through the `R3Block` mechanism: a `Block` can produce another
`Block` that represents an equivalent reformulation, a relaxation, or
a restriction of itself, with explicit support for moving solutions
and `Modification`s between the original and the reformulation
([Chapter 10](10-r3block.md#ch-10)). It supports *configuration* through the recursive
`BlockConfig` / `BlockSolverConfig` machinery ([Chapter 11](11-configuration.md#ch-11)), and
*coarse-grained parallel computation* through recursive locks,
asynchronous compute methods, and Solver checkpointing ([Chapter 17](17-parallel.md#ch-17)).
It serialises `Block`s, `Solution`s, `Configuration`s and `Change`s
to netCDF, a binary hierarchical file format that maps naturally onto
the nested structure of a `Block` tree ([Chapter 18](18-factories-netcdf.md#ch-18)).

Existing concrete `:Block` classes in the public modules of the
project cover, among others, min-cost flow (`MCFBlock`),
multicommodity flow and design (`MMCFBlock`), capacitated facility
location (`CapacitatedFacilityLocationBlock`), single- and
mixed-integer binary knapsack (`BinaryKnapsackBlock`), unit
commitment in its many flavours (`UCBlock`, `ThermalUnitBlock`,
`BatteryUnitBlock`, `IntermittentUnitBlock`, `DCNetworkBlock`,
`SlackUnitBlock`), stochastic multi-stage programs (`SDDPBlock`,
`StochasticBlock`, `TwoStageStochasticBlock`), strategic investment
(`InvestmentBlock`), and several others. Existing `:Solver` classes
include wrappers to external libraries (`MCFSolver` wrapping
`MCFClass`, `LEMONSolver` wrapping LEMON, the `MILPSolver` family
wrapping CPLEX, Gurobi, HiGHS and SCIP, `SDDPSolver` wrapping StOpt),
SMS++-native specialised solvers
(`DPBinaryKnapsackSolver`, `ThermalUnitDPSolver`,
`SDDPGreedySolver`), and SMS++-native generic structure-exploiting
solvers (`LagrangianDualSolver` and its derived `PrimalProximalHeur`,
`BundleSolver` and its parallel variant `ParallelBundleSolver`).

This manual focuses on the framework, not on the catalogue. Three
`:Block` are used pervasively as worked examples — `MCFBlock`,
`BinaryKnapsackBlock`, `CapacitatedFacilityLocationBlock` — chosen
because together they exercise every major concept of SMS++ while
remaining small enough to fit in a manual.

## 1.2 What SMS++ is not {#sec-1-2}

It is useful to clarify a number of things that SMS++ *is not*, so
that the reader can decide quickly whether the framework fits the
task at hand.

SMS++ is **not an algebraic modelling language**. A `:Block` is C++
code, written explicitly by the author of that `:Block`. The
framework does provide a number of modelling-language-style
conveniences — most notably the
[`AbstractBlock`](https://smspp.gitlab.io/smspp-project/d3/dbd/class_s_m_spp__di__unipi__it_1_1_abstract_block.html)
class, which reads `.mps` and `.lp` files and exposes them as a
`Block` with a single, "syntactic" abstract representation — but this
is a convenience entry point, not the canonical use of the framework.
The canonical use is to write a `:Block` that captures the *structure*
of a problem class, in C++, once, and then to reuse it across
applications.

SMS++ is **not for the faint of heart**. The framework is written
primarily for algorithmic experts; the audience of an algebraic
modelling language — practitioners who write `min cx s.t. Ax ≤ b` and
hand it to a solver — is *not* the audience of SMS++. End users may
nonetheless benefit from the catalogue of pre-defined `:Block` and
`:Solver`: a user who needs to solve a unit-commitment problem can
load an instance into a
[`UCBlock`](https://smspp.gitlab.io/smspp-project/df/d40/class_s_m_spp__di__unipi__it_1_1_u_c_block.html)
and attach a `:MILPSolver` without ever writing a new `:Block`. Even
in this scenario, however, the framework's idioms and the role of
configuration are not always intuitive, and reading at least Part I
of this manual is recommended.

SMS++ is **not interface-stable**. Version 0.6.0 is the current
public release; the framework is under active development and a
non-negligible amount of further evolution is expected, including
changes that may not preserve backward compatibility. Concrete
`:Block` and `:Solver` are extensively tested in the project's test
suite (`tests/` and `<Block>/test/` directories), but their public
APIs may evolve. The manual flags
**Status — beta** features explicitly; mature features have no such
flag.

SMS++ is **not bundled with one solver**. The framework provides its
own native specialised solvers, but it also relies on wrappers to a
range of external libraries: CPLEX, Gurobi, HiGHS and SCIP (through
the
[`MILPSolver`](https://smspp.gitlab.io/smspp-project/d7/d97/class_s_m_spp__di__unipi__it_1_1_m_i_l_p_solver.html)
family); `MCFClass` and LEMON for min-cost flow; StOpt for
stochastic dual dynamic programming. The user chooses which of these
external dependencies to install ([Chapter 2](02-installation.md#ch-2)).

## 1.3 Audience and prerequisites {#sec-1-3}

This manual addresses three audiences, listed in decreasing order of
specificity.

The **primary** audience consists of *algorithm developers*:
researchers, PhD students, and practitioners who want to implement a
structure-exploiting solution method on top of an existing `Block`,
or to extend the framework with a new `:Block`, a new `:Solver`, a
new `:Modification` or a new `:Change`. For this audience, the
manual is meant to fill the gap between the high-level slide decks
that accompany the project and the method-by-method Doxygen
reference: it explains the *idioms* and the *conventions* that make
SMS++ a coherent whole.

A **secondary** audience consists of *Block authors*: researchers who
need to express a new structured problem class as a `:Block` and
expose it to the existing `:Solver` catalogue. This audience needs to
understand the framework deeply enough to make the design decisions
that come with a new `:Block` — what is "physical", what is
"abstract", which `:Modification` to issue, what shape of `:Solution`
to produce — and is well served by reading the entire manual, with
particular attention to Part II (concepts) and [Appendix A](A-writing-block.md#app-A) (writing a
new `:Block`).

A **tertiary** audience consists of *sophisticated end users* who
have a problem in hand, find an existing `:Block` that fits, and want
to use it through one of the existing `:Solver`s. This audience can
afford to read selectively: Part I, the relevant `Recipe` in Part
III, and the cross-referenced Doxygen pages for the specific `:Block`
and `:Solver` of interest.

The minimum technical prerequisites are: working knowledge of
**C++-20** (concepts, ranges, smart pointers, lambdas, range-based
for); familiarity with **CMake** and, for the developer-loop
workflow, with classical **Make**; a working installation of a C++-20
compiler (recent `gcc`, `clang`, or MSVC); and at least an
operational understanding of the optimization paradigms that the
`:Block` of interest implicates. For Recipes [R4](R4-cfl-lagrangian.md#rec-R4) and [R5](R5-cfl-benders.md#rec-R5) in Part III,
basic familiarity with Lagrangian and Benders decomposition is
assumed.

## 1.4 How to read this manual {#sec-1-4}

The manual is organised in four parts plus front matter and back
matter; the rationale for the architecture is given here so that the
reader can plan the reading sequence that best suits their needs.

**Part I — Foundations** (Chapters [1](01-introduction.md#ch-1)–[3](03-mental-model.md#ch-3)) sets the stage. [Chapter 1](01-introduction.md#ch-1) is
this introduction. [Chapter 2](02-installation.md#ch-2) covers installation and the first build;
it is deliberately short because the
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home) and
the umbrella project's `README.md` are the authoritative source for
the detailed steps. [Chapter 3](03-mental-model.md#ch-3) is the *mental model* chapter: it
introduces the high-level picture of `Block`, `Solver`, the two
representations, the lifecycle of a `Block`, and the three running
examples. A reader who reads only Part I should leave with a coherent
picture of what SMS++ is, even if not yet able to write code against
it.

**Part II — Concepts and Idioms** (Chapters [4](04-block.md#ch-4)–[18](18-factories-netcdf.md#ch-18)) is the core of the
manual. Each chapter introduces one concept, defines it precisely,
locates it in the source tree and the Doxygen reference, and
illustrates it with a short, compilable snippet that uses one of the
three running examples. The order of the chapters is chosen so that
*every concept is introduced before it is used*: the dependency graph
of the concepts has been linearised. Each Part II chapter is short
(typically two to four pages typeset) and self-contained.

**Part III — End-to-end Recipes** (R1–R5) collects five complete,
runnable programs that recombine the concepts introduced in Part II
into solutions to recognisable scenarios: solve an MCF instance;
reoptimize a knapsack; solve a CFL problem three different ways
(MILP, MCF relaxation, Lagrangian); solve a CFL problem with
`LagrangianDualSolver` plus `PrimalProximalHeur`; solve a CFL problem
with Benders cuts emitted via a user-cut callback. Each recipe is
three to five pages, lists the concepts used, gives the full code,
walks through it line by line, and reports expected output.

**Part IV — Developer's Guide** (Appendices [A](A-writing-block.md#app-A)–[C](C-writing-modification.md#app-C)) is the manual for
*extending* the framework: writing a new `:Block`, a new `:Solver`, a
new `:Modification` or a new `:Change`. [Appendix A](A-writing-block.md#app-A) develops a small
but complete `BinPackingBlock` from scratch as a pedagogical example.

A pragmatic reader who already knows the concepts and just wants a
working program may read Part I and jump directly to the relevant
Recipe in Part III; the recipe will name the concepts it uses, and
the reader can drill back into Part II only as needed. Conversely, a
reader who wants a complete picture should read in order, and use
the Recipes as the runnable counterparts of the concepts.

## 1.5 Relation to the Doxygen reference and to the SMS++ Wiki {#sec-1-5}

This manual is one of three sources of documentation for SMS++, each
with a specific role.

The **Doxygen reference**, at <https://smspp.gitlab.io/smspp-project/>, is the
authoritative *API reference*. Every public class, every public
method, every member field is documented there, with its complete
signature, its parameters, its return values, its preconditions and
postconditions, its exceptions, and (for the better-documented
classes) inline mathematical notes about what it computes. The
Doxygen pages are generated from the inline comments in the source
files, which are kept in sync with the code as it evolves. The
canonical way to *look up a specific method* is to consult Doxygen;
the manual you are reading does not duplicate that information.
Throughout the manual, when a class or method is introduced, the
text indicates the corresponding Doxygen URL.

The **SMS++ Wiki**, at
<https://gitlab.com/smspp/smspp-project/-/wikis/home>, is the
authoritative source for *installation, build, customisation and
troubleshooting* instructions: how to fetch the umbrella project and
its sub-modules, how to configure CMake, how to enable optional
external solver interfaces, how to deal with platform-specific
issues. [Chapter 2](02-installation.md#ch-2) of this manual gives a brief overview of the build
system; for the details, the Wiki is the reference.

This **manual** is the third complement. Its scope is the *concepts
and idioms* of SMS++ and a collection of *end-to-end recipes*. It
explains why the framework is the way it is, how the abstractions fit
together, what the recurring idioms are, and what a few
representative end-to-end uses look like. It complements, but does
not substitute for, either the Doxygen reference or the Wiki.

When the three sources of documentation disagree, the source code is
the ultimate authority, the Doxygen reference comes second, and this
manual comes third. We have tried to keep all three consistent at the
version covered (0.6.0), but in case of discrepancy the reader is
invited to file an issue against the umbrella project.

## 1.6 A note on style {#sec-1-6}

A few stylistic choices that recur throughout the manual are worth
stating upfront.

We use **understatement**. SMS++ is a research-grade software
framework with definite strengths, but also with definite limitations
that we acknowledge openly. Where a feature is in beta or planned but
not yet released, the corresponding **Status** admonition appears
explicitly. Where SMS++ does not yet support a use case that one
might reasonably expect, we say so.

We **avoid comparing SMS++ with other modelling systems**. There is
plenty of useful comparative work in the literature; the manual,
however, is about SMS++ on its own terms. A reader who wants to
position SMS++ within the broader landscape of modelling and
optimization frameworks will find that material elsewhere.

We **prefer code to abstract description** wherever a short snippet
clarifies more than a paragraph of English. Every snippet in the
manual is meant to compile against the SMS++ source tree of version
0.6.0; the snippets in Part III recipes are extracted from
runnable example programs in the `manual/examples/` directory of the
source repository.

We **cite precisely**. Whenever we make a claim about how a class
behaves, we point to either its Doxygen page or a `file:line` in the
source. The reader who wants to verify any claim should be able to do
so by following the citation.

We **mark beta and planned features explicitly**. The reader should
never have to guess whether a feature can be relied upon in a
production codebase; the manual will say so.

With these preliminaries out of the way, [Chapter 2](02-installation.md#ch-2) turns to the
practical question of how to fetch SMS++ and build it.
