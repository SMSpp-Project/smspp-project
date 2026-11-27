# Appendix D: concept-to-Block map {#app-D}

[Source: the registrations in the factories
(`SMSpp_insert_in_factory_cpp_*`) in the `src/` of each module, and the
class comments of the headers named in [§D.2](D-concept-block-map.md#sec-D-2)]

We are concerned in this appendix with the cross-index of the manual: for each
concept, the `Block` (or other class) that carries the worked example and the
sections where the example is developed. The manual threads three running
`Block`s through the whole of Part II and Part III, using whichever one shows
a given concept most clearly: `MCFBlock` (a leaf `Block` with a wrapper-style
specialized `Solver`), `BinaryKnapsackBlock` (a leaf `Block` with a *native*
specialized `Solver`), and `CapacitatedFacilityLocationBlock` (a non-leaf
`Block` with four formulations, a non-trivial `R3Block`, and a hidden
`BendersBFunction`). The table below maps the concepts to them;
[§D.1](D-concept-block-map.md#sec-D-1) lists the `:Block`s and `:Solver`s that
every module of the distribution registers, and
[§D.2](D-concept-block-map.md#sec-D-2) describes the modules that no chapter
develops, together with two results on their decompositions that follow from
[Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1).

| Concept | Illustrated mainly by | Section(s) |
|---|---|---|
| `Block` tree, the identity-is-address rule | `MCFBlock` | [§4](04-block.md#ch-4) |
| `Variable` / `Constraint` / `Objective` | `BinaryKnapsackBlock` | [§5](05-variable-constraint-objective.md#ch-5) |
| Specialized `Solver` on a leaf `Block` (wrapper) | `MCFSolver` | [§6](06-solver.md#ch-6) |
| Specialized `Solver` on a leaf `Block` (native) | `DPBinaryKnapsackSolver` | [§6](06-solver.md#ch-6), [R2](R2-knapsack-reopt.md#rec-R2) |
| `is_feasible` / `is_dual_feasible` / `is_optimal` | `MCFBlock` | [§6](06-solver.md#ch-6), [§7](07-physical-abstract.md#ch-7) |
| Physical vs abstract representation | `MCFBlock`, `BinaryKnapsackBlock` | [§7](07-physical-abstract.md#ch-7) |
| `Modification`, abstract `Modification`, the Janus discipline | `MCFBlock`, `BinaryKnapsackBlock` | [§8](08-modification-janus.md#ch-8) |
| Typing a `Modification` by `memo_cast()` (since SMS++ 0.9.0) | `BinPackingBlock` (pedagogical) | [§8](08-modification-janus.md#ch-8), [§A.6](A-writing-block.md#sec-A-6) |
| `Solution` (`Block`-specific, physical and abstract) | `MCFSolution`, `BinaryKnapsackSolution` | [§9](09-solution.md#ch-9) |
| R3 Block (non-trivial relaxation) | CFL, `MCFBlock` flow relaxation | [§10](10-r3block.md#ch-10), [R3](R3-cfl-three-ways.md#rec-R3) |
| Keeping an R3 Block in sync (`UpdateSolver`), and solving a `Block` through it (`R3BSolver`, since SMS++ 0.9.0) | CFL, `MCFBlock` flow relaxation | [§10.6](10-r3block.md#sec-10-6), [§10.9](10-r3block.md#sec-10-9), [R3](R3-cfl-three-ways.md#rec-R3), [R4](R4-cfl-lagrangian.md#rec-R4), [R5](R5-cfl-benders.md#rec-R5) |
| `Configuration` tree, `[C/O/R]BlockConfig` | CFL formulations | [§11](11-configuration.md#ch-11) |
| Sub-`Block` composition and recursive `Modification` flow | CFL / KskForm with `BinaryKnapsackBlock` sub-`Block`s | [§12](12-sub-block.md#ch-12), [R4](R4-cfl-lagrangian.md#rec-R4) |
| The `Function` family (C05/C15, linear / quadratic / polyhedral) | `MCFBlock`, `BinaryKnapsackBlock` | [§13](13-function-family.md#ch-13) |
| `LagBFunction` wrapping a `Block` | CFL / KskForm | [§14](14-lag-benders-bfunction.md#ch-14), [R4](R4-cfl-lagrangian.md#rec-R4) |
| `BendersBFunction` wrapping a `Block` | CFL / BenForm | [§14](14-lag-benders-bfunction.md#ch-14), [R5](R5-cfl-benders.md#rec-R5) |
| The methods factory | `BinaryKnapsackBlock::chg_weights` | [§15](15-methods-factory.md#ch-15), [R2](R2-knapsack-reopt.md#rec-R2) |
| `Change` | `BinaryKnapsackBlockChange` | [§16](16-change.md#ch-16), [R2](R2-knapsack-reopt.md#rec-R2) |
| Parallel / asynchronous computation (`lock`, `compute_async`, `State`) | CFL / KskForm with a parallel `BundleSolver` | [§17](17-parallel.md#ch-17) |
| Factories and netCDF [de]serialization | all three `Block`s | [§18](18-factories-netcdf.md#ch-18), [R3](R3-cfl-three-ways.md#rec-R3) |
| Factories across shared libraries, and plugins (since SMS++ 0.9.0) | `BinPackingBlock` (pedagogical) | [§18.7](18-factories-netcdf.md#sec-18-7), [§18.8](18-factories-netcdf.md#sec-18-8), [§A.2](A-writing-block.md#sec-A-2) |
| `LagrangianDualSolver` | CFL / KskForm | [R4](R4-cfl-lagrangian.md#rec-R4) |
| `PrimalProximalHeur` | CFL / KskForm | [R4](R4-cfl-lagrangian.md#rec-R4) |
| The nondifferentiable `:Solver`s (bundle, subgradient, Frank-Wolfe) | CFL / KskForm, `GeneralizedAssignmentBlock` (since SMS++ 0.9.0) | [§19](19-nondifferentiable.md#ch-19), [R4](R4-cfl-lagrangian.md#rec-R4), [§D.2](D-concept-block-map.md#sec-D-2) |
| Benders cuts generated on the request of the master `:MILPSolver` | CFL / BenForm | [R5](R5-cfl-benders.md#rec-R5) |
| `BendersDecompositionSolver` | CFL as a two-level `AbstractBlock` | [§14.6](14-lag-benders-bfunction.md#sec-14-6), [R5](R5-cfl-benders.md#rec-R5) |
| Writing a new `:Block`, `:Solver`, `:Modification` / `:Change` from scratch | `BinPackingBlock` (pedagogical) | Appendices [A](A-writing-block.md#app-A), [B](B-writing-solver.md#app-B), [C](C-writing-modification.md#app-C) |

Read the table in either direction: a reader following a concept can
jump to the `Block` that exercises it, and a reader interested in one
of the three `Block`s can collect, by scanning the middle column, every
place that `Block` appears in the manual.

## D.1 The modules of the distribution {#sec-D-1}

The concepts above are not confined to the three running `Block`s: every
module of the umbrella project ([§2.2](02-installation.md#sec-2-2)) exercises
some of them, and its sources are a further worked example of each. For the
reader who wants one, the table lists the modules of the umbrella with the
`:Block`s and the `:Solver`s they register in the factories ([Chapter
18](18-factories-netcdf.md#ch-18)); the classes of the core library are in the
first row, and the bases of a module that are not in the factories (e.g.,
`NetworkBlock`, `SATSolver`) are left out. A `:Solver` that depends on an
optional external library is in the factory only if the module has been built
with it ([Chapter 2](02-installation.md#ch-2)); the rules of `SubGradSolver`
and the branching rule of `SATBlock` are in factories of their own, and are
named in the configuration of the `:Solver` that uses them.

| Module | `:Block`s | `:Solver`s |
|------|----------|----------|
| `SMS++` (core) | `AbstractBlock`, `PolyhedralFunctionBlock`, `MasterProblemBlock`, `BendersBlock`, `LagBFunction` and `BendersBFunction` (also `C05Function`s) | `UpdateSolver`, `R3BSolver`, `BoxSolver`, `FakeSolver` |
| `MCFBlock` | `MCFBlock` | |
| `MCFClassSolver` | | `MCFSolver` (a template over the `MCFClass` solvers) |
| `MCFLemonSolver` | | `MCFLemonSolverNetworkSimplex`, `MCFLemonSolverCycleCanceling`, `MCFLemonSolverCostScaling`, `MCFLemonSolverCapacityScaling`, each as `< GR , C , V >` with `GR` either `SmartDigraph` or `MCFListDigraph` |
| `MMCFBlock` | `MMCFBlock`, `MMCFNetworkDesignBlock` | |
| `BinaryKnapsackBlock` | `BinaryKnapsackBlock` | `DPBinaryKnapsackSolver`, `CoreDPBinaryKnapsackSolver`, `COMBOBinaryKnapsackSolver`, `RECORDBinaryKnapsackSolver` (these two with COMBO and RECORD), `GreedyRelaxationBinaryKnapsackSolver` |
| `MultiKnapsackAssignBlock` | `MultiKnapsackAssignBlock` | |
| `GeneralizedAssignmentBlock` | `GeneralizedAssignmentBlock` | |
| `CapacitatedFacilityLocationBlock` | `CapacitatedFacilityLocationBlock` | |
| `UCBlock` | `UCBlock`, `UnitBlock`, `ThermalUnitBlock`, `NuclearUnitBlock`, `HydroUnitBlock`, `HydroSystemUnitBlock`, `BatteryUnitBlock`, `IntermittentUnitBlock`, `SlackUnitBlock`, `ConversionUnitBlock`, `DCNetworkBlock`, `ACNetworkBlock`, `OTSNetworkBlock`, `ECNetworkBlock`, `DesignNetworkBlock` | `ThermalUnitDPSolver`, `ThermalUnitExtDPSolver`, `NuclearUnitExtDPSolver` |
| `InvestmentBlock` | `InvestmentBlock`, `InvestmentFunction` (also a `C05Function`) | |
| `StochasticBlock` | `StochasticBlock`, `ScenarioReductionBlock` | |
| `TwoStageStochasticBlock` | `TwoStageStochasticBlock` | |
| `MultiStageStochasticBlock` | `MultiStageStochasticBlock` | |
| `SDDPBlock` | `SDDPBlock` | `SDDPSolver`, `ParallelSDDPSolver`, `SDDPGreedySolver`, `SDDPSimulationSolver` |
| `ScenarioReductionSolver` | | `ScenarioReductionSolver`, `CSSCScenarioReductionSolver` |
| `LukFiBlock` | `LukFiBlock` | |
| `SVMBlock` | `SVCBlock`, `SVRBlock` | `SMOSolver`, `LIBSVMSolver`, `LIBLINEARSolver` |
| `SingleFlowDCRBlock` | `SingleFlowDCRBlock`, `MultiFlowDCRBlock` | `SingleFlowDCRBendersSolver` |
| `SATBlock` | `SATBlock` | `CaDiCaLSATSolver`, `MiniSATSolver`; the branching rule `GQSAT` in the library `SATBlockML`, built where Torch is |
| `CSPBlock` (since SMS++ 0.9.0) | `CSPBlock` | `FlatZincCSPSolver` |
| `SatellitesBlock` | `SatelliteBlock`, `SingleTargetBlock`, `MultiTargetBlock`, `MultiTargetBlockv2`, `ConstellationBlock`, `DiscreteSatelliteBlock`, `DiscreteConstellationBlock` | `SatelliteSolver`, `DiscreteSatelliteSolver` |
| `HQPBlock` | `HQPBlock`, `HQPDistBlock` | `HQPSolver`, `HQPStageSolver` |
| `MILPSolver` | | `MILPSolver` (which only builds the matrix form), `CPXMILPSolver`, `GRBMILPSolver`, `SCIPMILPSolver`, `HiGHSMILPSolver`, `MOSEKMILPSolver`, `ClarabelMILPSolver`, `DAQPMILPSolver`, `HPIPMMILPSolver`, `OSQPMILPSolver`, `PERMONMILPSolver`, `PIPSMILPSolver`, `PIQPMILPSolver`, `ProxQPMILPSolver`, `QPMADMILPSolver`, `QPOASESMILPSolver`, `QuadProgMILPSolver` |
| `BundleSolver` | | `BundleSolver`, `ParallelBundleSolver`; `BundleSolverML` in the library `BundleSolverML`, built where Torch is |
| `BDMinQuadSolver` (not distributed) | | `BDMinQuadSolver` (a `:Solver` of the primal `MasterProblemBlock`) |
| `SubGradSolver` | | `SubGradSolver`, with the step-size rules `Polyak`, `ColorTV`, `FumeroTV`, `PrimalDual` and the deflection rules `Volume`, `PrimalDual` |
| `FrankWolfeSolver` | | `FrankWolfeSolver`, `IntegralityBarrierSolver` |
| `LagrangianDualSolver` | | `LagrangianDualSolver`, `PrimalProximalHeur`, `LagrangianDualRelaxationSolver`, `FakeCDASolver` (the default inner `Solver`, which does nothing); `LagrangianDualRelaxationSolverML` in the library `LagrangianDualSolverML`, built where Torch is |
| `BendersDecompositionSolver` | | `BendersDecompositionSolver` |
| `BranchAndXSolver` | | `BranchAndXSolver` |

## D.2 Notes on the other modules {#sec-D-2}

The modules developed in [§6.10](06-solver.md#sec-6-10) and in
[Chapter 19](19-nondifferentiable.md#ch-19) are recalled here in a short
paragraph each, with a link to their treatment, while the modules that
no chapter develops are described in full. For each of the latter we
give the model it represents, its `:Solver`s with their algorithm in a
sentence or two, the optional dependencies, the sources, and the shipped
configurations that show it in use; the details are in the headers.

*`SubGradSolver`.* The deflected, projected and possibly incremental
subgradient methods of
[Frangioni et al. 2017](Z-bibliography.md#ref-frangioni2017), on the
same `Block`s that `BundleSolver` reads, with the stepsize and
deflection rules registered in factories of their own; they are
developed in [§19.6](19-nondifferentiable.md#sec-19-6), and the module
has no external dependency.

*`FrankWolfeSolver` and `IntegralityBarrierSolver`.* The first minimizes
a differentiable function of the `Variable`s of the sub-`Block`s, plus
their `Objective`s, over the product of the convex hulls of their
feasible regions, the `:Solver`s of the sub-`Block`s acting as linear
minimization oracles; the second is a heuristic for a feasible point of
a mixed-integer linear program, which minimizes by `FrankWolfeSolver` a
nonconvex function whose global minima are the integer points. Both are
developed in [§19.7](19-nondifferentiable.md#sec-19-7), and the module
has no external dependency.

*The `:Solver`s that learn.* `BundleSolverML` is a `BundleSolver` that
sets the proximal parameter $t$ by a small neural network
([§19.4](19-nondifferentiable.md#sec-19-4)), and
`LagrangianDualRelaxationSolverML` a `LagrangianDualRelaxationSolver`
that learns its branching rule while the branch-and-bound proceeds
([§16.12](16-change.md#sec-16-12)); the branching rule `GQSAT` of
`SATBlock` is described with that module below. All three need Torch:
each is in a separate library (`BundleSolverML`,
`LagrangianDualSolverML`, `SATBlockML`), built only where Torch is
found, which a program has to link in order to use them.

*`BDMinQuadSolver`.* A `:Solver` of the primal `MasterProblemBlock` of
`BundleSolver`, in the proximal and in the doubly stabilized forms, by
the active-set method of
[Frangioni 1996](Z-bibliography.md#ref-frangioni1996), developed in
[§19.5](19-nondifferentiable.md#sec-19-5). The module is not
distributed, since its repository is private, and it is built only
where its submodule has been initialized by hand.

*The back-ends of `MILPSolver`.* The derived classes of `MILPSolver`,
with the class of problems each accepts and the CMake options that
build them, are in [§6.10](06-solver.md#sec-6-10) and
[Table 6.2](06-solver.md#tab-6-2); each engine is an optional
dependency, and PIPS-IPM++, the engine of `PIPSMILPSolver`, is
available on Linux only. Only `CPXMILPSolver`, `GRBMILPSolver` and
`SCIPMILPSolver` give a mixed-integer master the dynamic `Constraint`s
of its `Block` from a callback of the branch-and-cut
([Recipe R5](R5-cfl-benders.md#rec-R5)): `HiGHSMILPSolver` throws when
asked for user cuts, and it cannot yet give HiGHS the lazy constraints
it separates. The separation loop of `intRelaxIntVars` 2, which
`MILPSolver` itself runs around each solve of a continuous problem,
needs no callback and works with any back-end.

*`SATBlock`.* This `Block` represents the weighted partial MaxSAT
problem ([`SATBlock.h`](https://gitlab.com/smspp/satblock/-/blob/develop/include/SATBlock.h)). There are $n$ Boolean variables,
and clauses in the DIMACS convention, each with a weight, which is
infinite for a hard clause and finite and nonnegative for a soft one;
each variable also has a cost $c_i$ of any sign, paid if it is true.
One looks for values that satisfy the hard clauses and minimize the
weights of the violated soft ones plus the costs. Its abstract
representation is the MILP
$$
\min \Bigl\{ \sum_c w_c r_c + \sum_i c_i x_i \;:\;
\sum_{i \in P_c} x_i - \sum_{i \in N_c} x_i + r_c \ge 1 - |N_c|
\;\; \forall c ,\; x \in \{0, 1\}^n ,\; r \in \{0, 1\}^m \Bigr\} ,
$$
where $P_c$ and $N_c$ are the variables that appear in the clause $c$ as they
are and negated, and $r_c$ is fixed to 0 for a hard clause. The variables may
be dealt out to groups, out of which `set_structure()` builds one
sub-`SATBlock` per group. With `kRelaxation`, the clauses that link the groups
are rows of the father. With `kDecomposition`, each linking clause goes to the
group having most of its variables, which gets a copy of the variables of the
others, and the rows of the father are the equalities between the copies and
the originals; [Proposition D.1](D-concept-block-map.md#prop-D-1) below
compares the two Lagrangian bounds. `CaDiCaLSATSolver` and `MiniSATSolver`
solve the satisfiability problem of the hard clauses or, with `intMaxSAT` 1,
the MaxSAT problem by the core-guided OLL method of [Andres et al.
2012](Z-bibliography.md#ref-andres2012) and [Morgado et al.
2014](Z-bibliography.md#ref-morgado2014), incrementally over the calls. They
are also `:RelaxationSolver`s ([Chapter 16](16-change.md#ch-16)) for a
`BranchAndXSolver` that fixes the variables, and may delegate the choice of
the branching variable to a branching rule given by name, such as the `GQSAT`
of the library `SATBlockML`, a policy learned after [Kurin et al.
2020](Z-bibliography.md#ref-kurin2020) and run by Torch. CaDiCaL and MiniSat
are optional dependencies, each `:Solver` being built with its library. In the
battery of `SATBlock`,
[`BPar-R.txt`](https://gitlab.com/smspp/tests/-/blob/develop/SATBlock/BPar-R.txt)
and
[`BPar-D.txt`](https://gitlab.com/smspp/tests/-/blob/develop/SATBlock/BPar-D.txt)
give the two structures, relaxed by the `LagrangianDualSolver` of
`BSPar-LD.txt`, and `OLLCfg.txt` configures the OLL method.

*`SVMBlock`.* The abstract `SVMBlock`, with its concrete `SVCBlock` and
`SVRBlock`, represents the training of a Support Vector Machine for
classification and for regression
([`SVMBlock.h`](https://gitlab.com/smspp/svmblock/-/blob/develop/include/SVMBlock.h)). Both problems are instances of
one parametric problem over $N$ dual indices,
$$
\min_{w, b, \xi} \; \tfrac{1}{2} \lVert w \rVert^2 + C \sum_{k = 0}^{N - 1}
\xi_k^p \;:\; s_k ( \langle w , x_{i(k)} \rangle + b ) + \xi_k \ge r_k ,\;
\xi_k \ge 0 ,
$$
with $p \in \{1, 2\}$, to which the term $\tfrac{1}{2} b^2$ is added if
the bias is regularized. At the choice of the `Configuration`, the
abstract representation is this primal form (for the linear kernel
only), its Wolfe dual
$\max \{ - q^\top \alpha - \tfrac{1}{2} \alpha^\top Q \alpha :
0 \le \alpha \le u ,\; s^\top \alpha = 0 \}$, the only form for a
nonlinear kernel (without the equality if the bias is regularized), or
the dual with the weights $z = Z^\top \alpha$ made explicit; all three
have the same optimal value. For the linear kernel there are also two
structures, a consensus and a Benders one, whose chunks are
sub-`Block`s. `SMOSolver` reads the physical representation, solves the
dual by the two-multiplier step of
[Platt 1998](Z-bibliography.md#ref-platt1998) with the working set and
the stopping thresholds of
[Keerthi et al. 2001](Z-bibliography.md#ref-keerthi2001), and
reoptimizes after a change of the data. `LIBSVMSolver` and
`LIBLINEARSolver` wrap the two libraries, which are optional
dependencies. The battery of `SVMBlock` cross-checks these `:Solver`s
with `:MILPSolver`s on each formulation, and with a
`LagrangianDualSolver` on the consensus structure; its `test_bds.cpp`
sets the consensus and the Benders structures against each other on the
same instance.

*`CSPBlock`.* This `Block` represents a constraint
satisfaction problem of constraint programming
([`CSPBlock.h`](https://gitlab.com/smspp/cspblock/-/blob/develop/include/CSPBlock.h)). Each of its $n$ variables has a finite
universe of integer values and a current domain within it, and each of
its $m$ constraints is of one of the kinds of a catalog (positive and
negative table, linear, all-different, element, cumulative), hard or soft
with a weight, and possibly inactive; a cost $c_{v,d}$ of any sign is paid
if the variable $v$ takes the value $d$. One looks for values in the
current domains that satisfy the active hard constraints and minimize
the costs plus the weights of the violated active soft constraints. The
abstract representation has one integer `ColVariable` per variable,
bounded by the extremes of its current domain, one `FRowConstraint` per
linear hard constraint with $\le$ or $=$, and one `:CSPConstraint` per
other hard constraint, which checks it and carries a linear relaxation
of it. Soft constraints, holes in the domains and costs are not in it,
and there is no abstract `Objective`. A `CSPBlockChange` represents the
changes of the domains that a branching makes. `FlatZincCSPSolver`
writes the `CSPBlock` as a MiniZinc model, runs the MiniZinc driver on
it as a separate process with the backend given in its configuration
(e.g., CP-SAT, Gecode or Chuffed), and reads back the last complete
assignment
([`FlatZincCSPSolver.h`](https://gitlab.com/smspp/cspblock/-/blob/develop/include/FlatZincCSPSolver.h)). It solves exactly the hard problems with
at most a linear objective in one variable; for a solution it cannot
prove optimal, e.g., with soft constraints, which the backend enforces
as hard ones, it returns `kLowPrecision`. The MiniZinc bundle is a
dependency at run time only, installed by `INSTALL.sh` and
`INSTALL.ps1`; the tests of the module are in its `test/`
directory.

*`GeneralizedAssignmentBlock` and `MultiKnapsackAssignBlock`.* The first
represents the Generalized Assignment Problem
$$
\min \Bigl\{ \sum_{i = 1}^M \sum_{j = 1}^N C_{ij} x_{ij} \;:\;
\sum_{i = 1}^M x_{ij} = 1 \;\; j = 1, \ldots, N ,\;
\sum_{j = 1}^N A_{ij} x_{ij} \le B_i \;\; i = 1, \ldots, M ,\;
x \in \{0, 1\}^{M \times N} \Bigr\}
$$
of [Cattrysse and Van Wassenhove 1992](Z-bibliography.md#ref-cattrysse1992)
([`GeneralizedAssignmentBlock.h`](https://gitlab.com/smspp/generalizedassignmentblock/-/blob/develop/include/GeneralizedAssignmentBlock.h)).
It has one minimization `BinaryKnapsackBlock` per agent $i$, whose items are
the jobs with weights $A_{ij}$ and profits $C_{ij}$, and the father holds only
the $N$ assignment rows. The second represents the Multiple Knapsack
Assignment Problem of [Kataoka and Yamada
2014](Z-bibliography.md#ref-kataoka2014), in which each of $M$ knapsacks is
given at most one of $R$ classes of items and holds items of that class only
([`MultiKnapsackAssignBlock.h`](https://gitlab.com/smspp/multiknapsackassignblock/-/blob/develop/include/MultiKnapsackAssignBlock.h)).
It has one `BinaryKnapsackBlock` per pair of a knapsack $i$ and a class $k$,
whose capacity is 0 and whose first item is the binary $y_{ik}$, with weight
$-C_i$ and profit 0; the father holds the assignment rows and the rows $\sum_k
y_{ik} \le 1$. Neither module has a specialized `:Solver`: a `:MILPSolver`
solves the problem out of the abstract representation, while a
`LagrangianDualSolver` relaxes the rows of the father and leaves independent
knapsacks to the `:Solver`s of `BinaryKnapsackBlock`. The batteries of both
modules compare the two approaches
([`BSPar.txt`](https://gitlab.com/smspp/tests/-/blob/develop/GeneralizedAssignmentBlock/BSPar.txt):
a `GRBMILPSolver` and two `LagrangianDualSolver`s, over a `BundleSolver` and
over a `SubGradSolver`). For the Generalized Assignment Problem, the
Lagrangian bound is that of the Dantzig-Wolfe reformulation ([Dantzig and
Wolfe 1960](Z-bibliography.md#ref-dantzig1960)), as [Proposition
D.2](D-concept-block-map.md#prop-D-2) states.

These two decompositions rest on
[Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1), which is stated
for a maximization with equality rows. For rows $G x \le d$ with
multipliers $y \ge 0$ its proof applies verbatim, the dual constraints
becoming $\sum_k \lambda_k ( d - G x^k ) \ge 0$, and a minimization is
the maximization of the opposite objective. In the first statement,
$v_R$ and $v_D$ denote the bounds of the Lagrangian duals of the rows
of the father under the structures `kRelaxation` and `kDecomposition`.

<span id="prop-D-1"></span>**Proposition D.1.** *Let the variables of
a `SATBlock` be dealt out to groups, and let its hard clauses be
satisfiable. Then $v_D \ge v_R$.*

*Proof.* For each group $g$, let $X_g$ be the set of the binary points $(x_g,
r_g)$ that satisfy the clauses of $g$, i.e., those whose variables are all in
$g$, $r_g$ being the $r_c$ of these clauses; let $L$ be the set of the linking
clauses. With `kRelaxation`, the sub-`Block` of $g$ holds $X_g$ and the $r_c$
of the soft linking clauses whose first literal is in $g$, which appear in no
clause of it, while the father holds the rows of the clauses of $L$. By
[Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1) (in the form for
inequalities), $v_R$ is then the minimum of the objective over the set $R$ of
the points with $(x_g, r_g) \in \mathrm{conv}(X_g)$ for all $g$, $r_c \in [0,
1]$ for the soft $c \in L$, and the rows of the clauses of $L$. With
`kDecomposition`, the sub-`Block` of $g$ holds the set $Y_g$ of the binary
points $(x_g, x'_g, r_g, r_{L_g})$ that satisfy the clauses of $g$ and those
of the set $L_g \subseteq L$ of the linking clauses assigned to $g$, the
latter written with the copies $x'_g$, which have cost 0. By [Theorem
14.1](14-lag-benders-bfunction.md#thm-14-1), $v_D$ is the minimum of the same
objective over the points with $(x_g, x'_g, r_g, r_{L_g}) \in
\mathrm{conv}(Y_g)$ for all $g$ and with the copies equal to their originals.
A solution of the hard clauses gives a point of both sets, which are therefore
not empty, as [Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1) requires.
Now take a point of the second set. The projection of $\mathrm{conv}(Y_g)$ on
$(x_g, r_g)$ is the convex hull of the projection of $Y_g$, which is contained
in $X_g$; hence, $(x_g, r_g) \in \mathrm{conv}(X_g)$, and also $r_c \in [0,
1]$. The row of each $c \in L_g$ is a linear inequality satisfied by every
point of $Y_g$, hence by every point of $\mathrm{conv}(Y_g)$, and, with the
copies equal to the originals, it is the row of $c$ in the father of
`kRelaxation`. Thus, the point without its copies belongs to $R$, with the
same objective value, and the minimum over $R$ is not larger than the minimum
over the second set. $\square$

The inequality may be strict, since `kDecomposition` convexifies each
linking clause together with the clauses of the group it is assigned
to, while `kRelaxation` imposes it only as a row on the convex hulls of
the groups. The price is a different set of multipliers, one per copy
of a variable in a group other than that of the variable instead of one per
linking clause.

<span id="prop-D-2"></span>**Proposition D.2.** *Let
$K_i = \{ x_i \in \{0, 1\}^N : \sum_j A_{ij} x_{ij} \le B_i \}$ be the
feasible set of the knapsack of the agent $i$, and let the linear
relaxation of the Dantzig-Wolfe reformulation of the Generalized
Assignment Problem over the sets $K_i$ be feasible. Then its value
equals the bound of the Lagrangian dual of the assignment rows,
$\max_u \{ \sum_j u_j + \sum_i \min \{ \sum_j ( C_{ij} - u_j ) x_{ij}
: x_i \in K_i \} \}$, and both equal
$\min \{ \sum_{ij} C_{ij} x_{ij} : \sum_i x_{ij} = 1 \;\; \forall j ,\;
x_i \in \mathrm{conv}(K_i) \;\; \forall i \}$.*

*Proof.* The set $X = K_1 \times \cdots \times K_M$ is finite, and
$\mathrm{conv}(X) = \mathrm{conv}(K_1) \times \cdots \times
\mathrm{conv}(K_M)$. [Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1),
applied to the maximization of $- \sum_{ij} C_{ij} x_{ij}$ over $X$ with the
equality rows $\sum_i x_{ij} = 1$ and with the multipliers $u = - y$, gives
that the Lagrangian dual bound is the minimum of $\sum_{ij} C_{ij} x_{ij}$
over $\mathrm{conv}(X)$ and the assignment rows, whenever this problem is
feasible. The linear relaxation of the Dantzig-Wolfe reformulation has a
variable $\lambda_i^k \ge 0$ per point $x_i^k$ of $K_i$, the rows $\sum_k
\lambda_i^k = 1$ for each $i$, and the assignment rows written in $x_i =
\sum_k \lambda_i^k x_i^k$. As $\lambda_i$ ranges over the simplex, $x_i$
ranges over $\mathrm{conv}(K_i)$, with the same objective value; therefore,
the two problems are feasible together and have the same value. $\square$

Hence, the bound is at least that of the continuous relaxation of the
problem, and it may be better, since $\mathrm{conv}(K_i)$ is in general
strictly contained in the continuous relaxation of the knapsack (which
does not have the integrality property). Note that the Dantzig-Wolfe
master is never formed: the knapsacks are solved by the specialized
`:Solver`s of `BinaryKnapsackBlock`.
