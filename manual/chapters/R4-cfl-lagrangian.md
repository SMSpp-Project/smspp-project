# Recipe R4: CFL by Lagrangian decomposition, with `PrimalProximalHeur` {#rec-R4}

[Source: [`tests/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt)
and the files it leads to, run by the `CFL_test` of
[Recipe R3](R3-cfl-three-ways.md#rec-R3) in
[`batches/batch-ld`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ld);
[`LagrangianDualSolver.h`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h),
[`PrimalProximalHeur.h`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/PrimalProximalHeur.h),
[`R3BSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/R3BSolver.h)]

We are concerned in this recipe with the solution of a Capacitated
Facility Location problem by *Lagrangian decomposition*: the problem is
put in its knapsack formulation, the customer satisfaction constraints
are dualized by a `LagrangianDualSolver`, and the result is a strong
lower bound together with a convexified primal solution. The
convexified (fractional) solution is then turned into feasible ones by
`PrimalProximalHeur`, which derives from `LagrangianDualSolver` and takes
its place in the same configuration file, although some parameters
change meaning in the exchange. The recipe shows the decomposition
machinery of Chapters [12](12-sub-block.md#ch-12) and
[14](14-lag-benders-bfunction.md#ch-14) at work.

## Concepts used

- the tree of sub-`Block`s of KskForm ([Chapter 12](12-sub-block.md#ch-12));
- `LagBFunction` and `LagrangianDualSolver`
  ([Chapter 14](14-lag-benders-bfunction.md#ch-14));
- `BundleSolver` and the reoptimization vocabulary
  ([Chapter 13](13-function-family.md#ch-13));
- the configuration of the formulation and of the `Solver`
  ([Chapter 11](11-configuration.md#ch-11)), and the `R3BSolver` (since
  SMS++ 0.9.0) that solves a `Block` through an R3 Block of it
  ([§10.9](10-r3block.md#sec-10-9)).

## What the configuration sets up

The driver is the `CFL_test` of [Recipe R3](R3-cfl-three-ways.md#rec-R3). The
list
[`CFLBForm-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt)
names the two files of the original `Block` $B_1$: its `BlockConfig`
[`CFLBCfg-NF-SF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-NF-SF.txt)
(the natural formulation) and its `BlockSolverConfig`
[`CFLBSCfg-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld.txt),
which registers two `:Solver`s, a `GRBMILPSolver` computing the strong
continuous relaxation of $B_1$ (`intRelaxIntVars` 2, `intCutSepPar` 1) and an
`R3BSolver`. The latter solves $B_1$ through an R3 Block $B_2$ of it, which it
makes and keeps in sync out of the three files named by
[`R3BSCfg-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BSCfg-ld.txt),
the extra `Configuration` of its `ComputeConfig`:
[`R3BCfg-copy.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BCfg-copy.txt),
by which $B_2$ is a copy, and the two files below, which are therefore the
ones that matter here.

- [`CFLBCfg-KF-LD.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-KF-LD.txt) puts the copy in the knapsack
  formulation (a `SimpleConfiguration< int >` of value 1 in the
  `f_static_variables_Configuration`). As
  [Chapter 12](12-sub-block.md#ch-12) described, this grows one
  `BinaryKnapsackBlock` sub-`Block` per facility, leaving only the
  linking constraints $\sum_i X_{ij} = 1$ in the master. Its
  `f_static_constraints_Configuration` of value 1 builds these linking
  constraints, but not the abstract knapsack constraints inside the
  sub-`Block`s (bit 1, value 2, would build them): the knapsack
  `Solver`s used here read the physical data of their
  `BinaryKnapsackBlock`, and a leaf `:MILPSolver` generates the
  abstract representation of its `Block` by itself when it is
  registered.
- [`CFLBSCfg-ld-LDS.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld-LDS.txt) attaches a single `LagrangianDualSolver`, with
  a `ComputeConfig` whose parameters mostly configure the inner
  `Solver` that drives the dual (`str_LDSlv_ISName`, here
  `BundleSolver`): the norm of the stopping condition, the size of the
  bundle per component, the strategy of the proximal parameter, and so
  on. Its `str_LagBF_BSCfg` names the `BlockSolverConfig` of the
  knapsacks of the facilities, [`LPBSCfg-K-DP.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/LPBSCfg-K-DP.txt).

With this, `LagrangianDualSolver`
([§14.2](14-lag-benders-bfunction.md#sec-14-2)) builds a hidden
`LagrangianDualBlock`, wraps the `BinaryKnapsackBlock` of each facility
in a `LagBFunction` that dualizes the linking constraints, and runs the
inner `BundleSolver` on the sum of those `LagBFunction`s. Each
`LagBFunction` is evaluated by solving its knapsack, by a `CoreDPBinaryKnapsackSolver` in [`LPBSCfg-K-DP.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/LPBSCfg-K-DP.txt) or
by a `:MILPSolver` ([`LPBSCfg-K-CX.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/LPBSCfg-K-CX.txt), or [`LPBSCfg-K-LP.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/LPBSCfg-K-LP.txt) for the
continuous relaxation of the knapsack), and the solutions of the
knapsacks are combined into the convexified primal solution that the
dual returns with the bound.

```text
            LagrangianDualBlock (hidden, built by LagrangianDualSolver)
              |   BundleSolver drives the dual over the sum below
              +-- LagBFunction_0  --> BinaryKnapsackBlock (facility 0)  --> DP solver
              +-- LagBFunction_1  --> BinaryKnapsackBlock (facility 1)  --> DP solver
              +-- ...
   master CFL Block: customer satisfaction linking constraints (dualized)
```

## How a solve proceeds

The arrangement is made by applying configuration files, and no
line of the driver is specific to it; on the side of $B_2$, the pieces
are built and used in the following order.

<span id="alg-R4-1"></span>**Algorithm R4.1** (Lagrangian dual of the CFL
through an R3 Block).

```text
 1  B₂ ← the R3 Block of B₁ described by R3BCfg-copy.txt (a copy)
 2  apply CFLBCfg-KF-LD.txt to B₂               (KskForm, linking rows)
 3  generate the abstract Variables of B₂, which grows the sub-Blocks Kᵢ
 4  apply CFLBSCfg-ld-LDS.txt to B₂: it registers a LagrangianDualSolver S
 5  for each facility i:
 6      Lᵢ ← a LagBFunction holding Kᵢ, with the linking rows dualized
 7      apply LPBSCfg-K-DP.txt to Kᵢ              (the knapsack Solver)
 8  register the inner Solver (str_LDSlv_ISName) to the Block of ∑ᵢ Lᵢ
 9  compute S: the inner Solver optimizes ∑ᵢ Lᵢ over the multipliers
10  return the bound of S, and its convexified solution mapped back to B₁
```

Steps 1 to 4 are made by `R3BSolver` as soon as it has both its `Block`
and its `ComputeConfig`
([`R3BSolver.cpp`](https://gitlab.com/smspp/smspp/-/blob/develop/src/R3BSolver.cpp)). Steps 5 to 8 are made by
`LagrangianDualSolver::set_Block()`
([`LagrangianDualSolver.cpp`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/src/LagrangianDualSolver.cpp)), which makes each $K_i$ the
inner `Block` of its $L_i$ (or a copy of it, if `int_LDSlv_iBCopy` is
nonzero); the $K_i$ stay in the $L_i$ only while the inner `:Solver`
runs and while a solution is read, and are given back to $B_2$ in
between ([§14.2](14-lag-benders-bfunction.md#sec-14-2)). Step 10 is
made by `R3BSolver::get_lb()` and `R3BSolver::get_var_solution()`; both
bounds of the inner `:Solver` are valid for $B_1$, since the copy is a
Reformulation (`intR3BKind` 0, the default).

## Walk-through

- The `BlockConfig` selects KskForm
  ([§11.8](11-configuration.md#sec-11-8)), and the generation of the
  abstract representation builds the tree of the `BinaryKnapsackBlock`s
  ([§12.5](12-sub-block.md#sec-12-5)) and the linking constraints, the
  only `Constraint`s of the master, as `LagrangianDualSolver` requires
  ([§14.2](14-lag-benders-bfunction.md#sec-14-2)).
- `LagrangianDualSolver` is a `CDASolver`
  ([Chapter 6](06-solver.md#ch-6)): `compute()` solves the Lagrangian
  dual, the bound is read with `get_lb()` (the CFL being a
  minimization), and the convexified primal solution is read from the
  `Variable`s of the `Block` once `get_var_solution()` has written it
  there. (`Block::get_valid_lower_bound()` is a different thing: a bound
  that the `:Block` knows from its own data, not the one a `:Solver`
  computes.)
- The reoptimization vocabulary of
  [Chapter 13](13-function-family.md#ch-13) makes the inner loop
  efficient: the multipliers are the outer `Variable`s of the
  `LagBFunction`s, and moving them changes the Lagrangian costs of the
  knapsacks, which the knapsack `Solver`s absorb as changes of profits,
  while the bundle keeps the linearizations of
  [Lemma 14.1](14-lag-benders-bfunction.md#lem-14-1), which do not
  depend on the point.

## From a bound to a feasible solution: `PrimalProximalHeur`

`LagrangianDualSolver` returns a bound and a convexified primal
solution, which is in general fractional, hence not a feasible CFL
solution. Feasible ones are obtained by
[`PrimalProximalHeur`](https://smspp.gitlab.io/smspp-project/d4/d41/class_s_m_spp__di__unipi__it_1_1_primal_proximal_heur.html),
which derives from `LagrangianDualSolver` and implements the
primal-proximal heuristic of
[Daniilidis and Lemaréchal 2005](Z-bibliography.md#ref-daniilidis2005).
At each iteration, it augments the Lagrangian objective with a quadratic
proximal term $M \lVert x - \hat x \rVert^2$ that pushes the
convexified solution towards a point $\hat x$ (the proximal center),
and keeps the best feasible solutions found along the way. The term can
be passed to the inner `LagrangianDualSolver` as a change of the linear
costs because of the following identity.

<span id="lem-R4-1"></span>**Lemma R4.1.** *For $x \in \{0, 1\}^n$ and any
$\hat x \in \mathbb{R}^n$, $\lVert x - \hat x \rVert^2 = \sum_i (1 - 2 \hat
x_i) x_i + \lVert \hat x \rVert^2$; hence, on binary points the proximal term
is an affine function of $x$.*

*Proof.* $\lVert x - \hat x \rVert^2 = \sum_i ( x_i^2 - 2 \hat x_i x_i +
\hat x_i^2 )$, and $x_i^2 = x_i$ for $x_i \in \{0, 1\}$. $\square$

Hence, the heuristic only changes the objective coefficients of the
binary `Variable`s by $M (1 - 2 \hat x_i)$ (the constant being
irrelevant), which the `LagBFunction`s pass to their knapsacks as
ordinary changes of costs. The switch is made in
[`CFLBSCfg-ld-LDS.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld-LDS.txt) by replacing the name `LagrangianDualSolver`
with `PrimalProximalHeur`, which accepts all the parameters of the
former; however, some of them then refer to the
heuristic rather than to the inner `:Solver`
([`PrimalProximalHeur.h`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/PrimalProximalHeur.h)), and the file has to be
adapted accordingly:

- `intMaxIter`, `dblRelAcc` and `dblMaxTime` are those of the heuristic
  as a whole and are not passed to the inner `:Solver`, whose own
  iteration limit and accuracy go in the `ComputeConfig` of the file
  named by `str_LDSlv_ISCfg`;
- `intMaxThread` is the number of threads that solve the components of
  the consensus recovery (0 by default), and it is not passed to the
  inner `:Solver` either;
- `intLogVerb` encodes two verbosities as $v + 4 w$, $v \in \{0, 1, 2\}$
  being that of the heuristic and $w$ the one passed on to the inner
  `:Solver`, so that the value 2 of the shipped file gives a detailed log
  of the heuristic and a silent `BundleSolver`.

The heuristic adds a few parameters, chiefly `dbl_penaltyFactor` (the
$M$, 0 by default, with which the heuristic reduces to a warm-started
`LagrangianDualSolver`) and `intMaxSol` (how many of the feasible
solutions found are kept, 1 by default). Also, `strRecoveryBSC` is the
`BlockSolverConfig` of the *primal recovery*, i.e., of the `:Solver`
that solves the restricted problem on the `Block`, coupling constraints
included, with the binary `Variable`s driven by the proximal term fixed
at their (rounded) values; its optimum is then a feasible completion of
the integer point. If that problem is infeasible, it is solved again with
only the binaries at 1 fixed, and then with none fixed; each of these
problems is a restriction of the original one, and hence any of its
solutions gives a valid upper bound. The default of `strRecoveryBSC` is
`RecoveryCfg.txt`, installed with the module in
`share/LagrangianDualSolver`, which attaches a `GRBMILPSolver`. With the
empty string there is no recovery, and the only feasible solutions are the
iterates that happen to be feasible. The other parameters of the heuristic
are described in
[`PrimalProximalHeur.h`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/PrimalProximalHeur.h):
`strWarmStartBSC` names the `BlockSolverConfig` of a `:Solver` that computes
a warm start for the multipliers (none by default), `intUseWarmStartPSol`
takes its primal solution as the first proximal center,
`intNoRecoveryIfFeasible` skips the recovery when the point is already
feasible, `intRecoveryMIPStart` gives the recovery the best solution found as
a MIP start, and `intPenConstrBins` also penalizes the binaries that appear in
the `Constraint`s of a sub-`Block` but not in its `Objective`. Finally,
`dblMaxTime` is the time given to the heuristic as a whole, out of which the
iterations leave a reserve for the final recovery (the `dblMaxTime` of the
`:Solver` of `strRecoveryBSC`, but at most half of the total).

## Expected behavior

- `LagrangianDualSolver` returns a strong lower bound, equal to the
  bound of the strong LP of [Recipe R3](R3-cfl-three-ways.md#rec-R3) for
  the splittable CFL ([Lemma R3.1](R3-cfl-three-ways.md#lem-R3-1)),
  together with a convexified primal solution that is optimal for the
  strong LP ([Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1)) but,
  in general, not integer; `CFL_test` checks that the bound of the
  `R3BSolver` agrees with that of the `GRBMILPSolver` on $B_1$, as in
  [Recipe R3](R3-cfl-three-ways.md#rec-R3).
- `PrimalProximalHeur` returns, as lower bound, that of the Lagrangian
  dual of its first iteration, which is unpenalized (with
  `dbl_penaltyFactor` 0 this is the bound of the inner `:Solver`, as for
  `LagrangianDualSolver`); when `intUseWarmStartPSol` is nonzero and a warm
  start is given by `strWarmStartBSC`, the first iteration is penalized as
  well, and the lower bound is $-\infty$. As upper bound it returns the
  value of the best feasible solution found, which comes from the primal
  recovery of `strRecoveryBSC`; it stops when the two are within
  `dblRelAcc` of each other, or at its limits. The quality of the
  feasible solutions depends on the penalty factor $M$, the main tuning
  parameter of the heuristic.

## Variations

*The leaf `Solver`.* The knapsacks of the facilities can be solved by
`CoreDPBinaryKnapsackSolver` or `DPBinaryKnapsackSolver` (exact, for
integer weights) or by a `:MILPSolver`, a choice made in the
configuration of the leaf `Block`s (the file named by `str_LagBF_BSCfg`)
that leaves the decomposition unchanged.

*In parallel.* Naming `ParallelBundleSolver` instead of `BundleSolver` as the
inner `:Solver` (`str_LDSlv_ISName`, a line already present, commented out, in
the file) evaluates the `LagBFunction`s of the facilities concurrently
([§19.4](19-nondifferentiable.md#sec-19-4)), which is sound since the
knapsacks are independent.

*Another inner `Solver`.* A `CDASolver` that minimizes a sum of
`C05Function`s can drive the dual in place of `BundleSolver`, e.g., the
`SubGradSolver` of [§19.6](19-nondifferentiable.md#sec-19-6), whose
step-size and deflection rules are named by `strStepsize` and
`strDeflection`. No shipped file does so on the CFL; the `ComputeConfig`
[`LDCfg-SG.txt`](https://gitlab.com/smspp/tests/-/blob/develop/GeneralizedAssignmentBlock/LDCfg-SG.txt)
of the battery of `GeneralizedAssignmentBlock` (since SMS++ 0.9.0,
[§D.2](D-concept-block-map.md#sec-D-2)) does it on a `Block` of the
same shape, i.e., a set of knapsacks linked by assignment constraints.

*Tuning the heuristic.* `dbl_penaltyFactor` trades the pressure
towards integrality against the fidelity to the convexified solution:
too small an $M$ may not lead to any feasible point, while too large an
$M$ pins the solution to a possibly poor proximal center.
