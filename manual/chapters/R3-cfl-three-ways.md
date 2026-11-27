# Recipe R3: CFL three ways: cuts, MCF relaxation, Lagrangian {#rec-R3}

[Source: [`tests/CapacitatedFacilityLocationBlock/test.cpp`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/test.cpp),
[`SMS++/include/R3BSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/R3BSolver.h),
the configuration files of [`tests/CapacitatedFacilityLocationBlock`](https://gitlab.com/smspp/tests/-/tree/develop/CapacitatedFacilityLocationBlock)]

We are concerned in this recipe with solving the *same* Capacitated Facility
Location (CFL) instance in three different ways (a cut-strengthened linear
relaxation, a min-cost flow relaxation and a Lagrangian dual) with one
executable, `CFL_test`, and no recompilation, by changing only the
configuration files. This is the recipe that shows the separation in SMS++ of
*what the model is* from *how it is solved*. Since in each arrangement the
tester compares bounds that are equal in theory, it is also a check of the
lemmas of [Chapter 10](10-r3block.md#ch-10) and
[Chapter 14](14-lag-benders-bfunction.md#ch-14). The three arrangements are
given to `CFL_test` by the lists of formulations [`CFLBForm-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-cuts.txt), [`CFLBForm.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm.txt) and [`CFLBForm-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt) (option
`-F`), and the batteries [`batches/batch-cuts`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-cuts),
[`batches/batch`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch) and [`batches/batch-ld`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ld)
run them on sets of instances. After the concepts used, we describe what the
tester does and the configuration files of each arrangement, prove that the
bounds compared are equal, comment the procedure and its output, and discuss
a few variations.

## Concepts used

- R3 Blocks, and the `R3BSolver` (since SMS++ 0.9.0) that makes one and keeps
  it in sync with the original `Block`
  ([Chapter 10](10-r3block.md#ch-10));
- a `BlockConfig` to pick the formulation and a `BlockSolverConfig` to
  pick the `:Solver`s ([Chapter 11](11-configuration.md#ch-11));
- the tree of sub-`Block`s of KskForm ([Chapter 12](12-sub-block.md#ch-12));
- `LagBFunction` and `LagrangianDualSolver`
  ([Chapter 14](14-lag-benders-bfunction.md#ch-14)).

## The procedure

The list of formulations names only two files, the `BlockConfig` and the
`BlockSolverConfig` of the CFL problem $B_1$, and the R3 Blocks with which
$B_1$ is compared are made by the `:Solver`s that this `BlockSolverConfig`
attaches. Besides a `:MILPSolver` of $B_1$, these are one or more
`R3BSolver`s ([§10.9](10-r3block.md#sec-10-9)), each of which solves an R3
Block $B_2$ of $B_1$ in its place. The extra `Configuration` of the
`ComputeConfig` of an `R3BSolver` (a file `R3BSCfg-*.txt`) gives how $B_2$ is
made, its `BlockConfig` and its `BlockSolverConfig`, the first `:Solver` of
which is the inner one, while its parameter `intR3BKind` decides which
bounds of the inner `:Solver` are passed on
([Table 10.1](10-r3block.md#tab-10-1)). Condensed, `CFL_test` then does
what [Algorithm R3.1](R3-cfl-three-ways.md#alg-R3-1) states.

<span id="alg-R3-1"></span>**Algorithm R3.1** (the CFL tester).

```text
 1  procedure CFL-TEST(instance, -F, -s, -T, -n)
 2     B₁ ← the CFL problem read from the instance (text or netCDF file)
 3     apply to B₁ the BlockConfig of -F and generate its Variables
 4     apply to B₁ the BlockSolverConfig of -F   ▷ each R3BSolver makes its B₂
 5     SOLVE-ALL()
 6     for h = 1, …, n while all the comparisons have passed do
 7        apply 4 rounds of random changes to B₁   ▷ forwarded to each B₂
 8        SOLVE-ALL()
 9     print whether all the comparisons have passed
10
11  procedure SOLVE-ALL()
12     if -s = 0 then
13        compute each Solver of B₁, in the order of the BlockSolverConfig
14        compare all their lower bounds, Solver k with tolerance max{εₖ, -T}
15     else
16        compute the Solvers of B₁ that are not R3BSolvers
17        for each R3BSolver of B₁ do run the slope-scaling heuristic with it
```

Algorithm R3.1 is implemented by `main()` and `SolveBoth()` in
[`test.cpp`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/test.cpp),
and the comparison at line 14 by `cross_check()` in
[`tests/common_utils.h`](https://gitlab.com/smspp/tests/-/blob/develop/common_utils.h).
At line 14 the bound of Solver $k$ has to agree with the largest of the lower
bounds and with the smallest of the upper bounds that the others give, within
$\max\{ \varepsilon_k , T \}$ relative to $\max\{ 1 , |\cdot| \}$. Here $T$
is the value of `-T`, $10^{-5}$ by default, and $\varepsilon_k$ is the
relative accuracy `dblRelAcc` that Solver $k$ has been asked for, unless the
option `-E` declares another one (the tester gives no scale of the bounds to
the function that computes it, which therefore does not look at `dblAbsAcc`).
For an `R3BSolver` this tolerance is its own `dblRelAcc` ($10^{-6}$ by
default), not that of its inner `:Solver`, and the `R3BSolver`s are compared
as any other `:Solver`, with the bound that their `intR3BKind` lets through.
Thus, the behavior is decided by the configuration files, which differ
between the three arrangements while the code stays the same.

## The three configurations

<span id="tab-R3-1"></span>**Table R3.1.** The configuration files of the
three arrangements; $B_2$ denotes the R3 Block of each `R3BSolver`, and the
row "inner `:Solver`" gives the `:Solver`s of each $B_2$.

| | `cuts` | `MCF` | `LD` |
|---|---|---|---|
| list of formulations (`-F`) | [`CFLBForm-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-cuts.txt) | [`CFLBForm.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm.txt) | [`CFLBForm-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt) |
| $B_1$: formulation | [`CFLBCfg-NF-SFsep.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-NF-SFsep.txt) | [`CFLBCfg-NF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-NF.txt) | [`CFLBCfg-NF-SF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-NF-SF.txt) |
| $B_1$: `:Solver`s | [`CFLBSCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-cuts.txt) | [`CFLBSCfg-LP.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-LP.txt) | [`CFLBSCfg-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld.txt) |
| `:MILPSolver` of $B_1$ | `GRBMILPSolver`, [`MILPCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/MILPCfg-cuts.txt) | `HiGHSMILPSolver`, `intRelaxIntVars` 1 | `GRBMILPSolver`, `intRelaxIntVars` 2, `intCutSepPar` 1 |
| `R3BSolver`s | 1, [`R3BSCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BSCfg-cuts.txt) | 7, [`R3BSCfg-MCF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BSCfg-MCF.txt) and its variants | 1, [`R3BSCfg-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BSCfg-ld.txt) |
| $B_2$: made by | [`R3BCfg-copy.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BCfg-copy.txt) (0, a copy) | [`R3BCfg-MCF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BCfg-MCF.txt) (1, an `MCFBlock`) | [`R3BCfg-copy.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BCfg-copy.txt) (0, a copy) |
| $B_2$: formulation | [`CFLBCfg-KF-SFsep.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-KF-SFsep.txt) | [`MCFBCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/MCFBCfg.txt) | [`CFLBCfg-KF-LD.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-KF-LD.txt) |
| inner `:Solver` | `GRBMILPSolver`, [`MILPCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/MILPCfg-cuts.txt) | `MCFSolver<MCFSimplex>`, `<RelaxIV>`, `<MCFCplex>`, `MCFLemonSolver` with four algorithms | `LagrangianDualSolver` of [`CFLBSCfg-ld-LDS.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld-LDS.txt) |
| `intR3BKind` | 0 | 1 | 0 |
| bounds compared | strong LP, twice | weak LP and flow relaxation | strong LP and Lagrangian dual |

The rows of the table read as follows. In the formulations, NF is the natural
formulation and KF the knapsack one (`wf` 0 and 1 in the
`f_static_variables_Configuration`). "SF" means that the third bit of the
`f_static_constraints_Configuration` (value 7) adds the group of the strong
forcing constraints $X_{ij} \le Y_i$, as dynamic `Constraint`s separated on
demand, and "sep" that the `f_dynamic_constraints_Configuration` gives the
separation parameters (at most 375 constraints per round, with violation at
least $10^{-4}$). The default 15 of
[`CFLBCfg-NF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-NF.txt)
also declares the group, but its `:MILPSolver`, with `intRelaxIntVars` 1,
separates none of them.
[`MILPCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/MILPCfg-cuts.txt)
sets `intRelaxIntVars` 2, `intCutSepPar` 1 and `intMaxCutPasses` 10000, i.e.,
the continuous relaxation solved as a pure LP within the separation loop of
[Algorithm 6.1](06-solver.md#alg-6-1), and the `GRBMILPSolver` of
[`CFLBSCfg-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld.txt)
does the same, with `intMaxCutPasses` at its default 1000. In `MCF`, each of
the seven `R3BSolver`s of
[`CFLBSCfg-LP.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-LP.txt)
has a file `R3BSCfg-MCF*.txt` that differs only in the inner `:Solver`, which
is `MCFSolver<MCFSimplex>`, `MCFSolver<RelaxIV>` or `MCFSolver<MCFCplex>` (the
last one only if `MCFClass` has been built with CPLEX), or
`MCFLemonSolverNetworkSimplex<MCFListDigraph,double,double>` and the analogous
`:Solver`s of the other three algorithms of abLEMON (`CycleCanceling`,
`CostScaling` and `CapacityScaling`); there $B_2$ is a relaxation of $B_1$,
and only its lower bound is passed on. Finally,
[`CFLBSCfg-ld-LDS.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld-LDS.txt)
attaches a `LagrangianDualSolver` whose inner `BundleSolver` works on the
knapsacks of the facilities.

## Why the bounds coincide

With the notation of [§10.5](10-r3block.md#sec-10-5), call *weak LP*
the continuous relaxation of (10.1), i.e., $Y \in [0, 1]^m$, and
*strong LP* the weak LP with the constraints $X_{ij} \le Y_i$ added.
Each arrangement compares bounds that are equal in theory, and the batteries
check the equality on each of their instances.

<span id="lem-R3-1"></span>**Lemma R3.1.** *For the splittable CFL (bit 2 of
`wf` zero, i.e., `wf` 0 or 1 in the three arrangements), the three
arrangements compare equal values: (i) in `cuts`, both $B_1$ and $B_2$ compute
the strong LP; (ii) in `MCF`, $B_1$ computes the weak LP and $B_2$ the flow
relaxation, which is equal to it; (iii) in `LD`, $B_1$ computes the strong LP
and $B_2$ the Lagrangian dual of the knapsack formulation with respect to the
constraints $\sum_i X_{ij} = 1$, which is equal to it.*

*Proof.* (i) The natural and the knapsack formulations have the same
constraints, written in the parent or in the sub-`Block`s of the facilities,
and the separation loop adds violated constraints $X_{ij} \le Y_i$ until none
is violated by more than its tolerance (which we assume to happen within
`intMaxCutPasses` passes and the time limit); hence, both compute the strong
LP, up to that tolerance. (ii) This is [Lemma 10.3](10-r3block.md#lem-10-3).
(iii) For a facility $i$, let $K_i = \{ (X_{i\cdot}, Y_i) : \sum_j D_j X_{ij}
\le Q_i Y_i ,\; X_{i\cdot} \in [0, 1]^n ,\; Y_i \in \{0, 1\} \}$ be the set of
its knapsack. It is the union of the two polytopes $\{(0, 0)\}$ and $P_i
\times \{1\}$, with $P_i = \{ X_{i\cdot} \in [0,1]^n : \sum_j D_j X_{ij} \le
Q_i \}$, and therefore its convex hull is $\{ (\lambda x, \lambda) : x \in P_i
,\; \lambda \in [0, 1] \}$, which is the set $S_i = \{ (X_{i\cdot}, Y_i) : 0
\le X_{ij} \le Y_i \le 1 ,\; \sum_j D_j X_{ij} \le Q_i Y_i \}$. Indeed, a
point $(\lambda x, \lambda)$ satisfies the constraints of $S_i$ since $x \in
P_i$; and conversely a point of $S_i$ with $Y_i > 0$ is $(Y_i x, Y_i)$ with $x
= X_{i\cdot} / Y_i \in P_i$, while one with $Y_i = 0$ has $X_{i\cdot} = 0$.
Since $K = \prod_i K_i$ is a finite union of polytopes, its convex hull
$\prod_i S_i$ is a polytope, and [Theorem
14.1](14-lag-benders-bfunction.md#thm-14-1) applies: the Lagrangian dual is
the optimum of the objective of (10.1) over $\{ \sum_i X_{ij} = 1 \} \cap
\prod_i
S_i$, which is the strong LP. $\square$

Hence, the three arrangements reach the bounds by different routes (an LP
engine with a separation loop, seven algorithms of min-cost flow, a bundle
method on the dual) and agree on the value. Note that the strong LP and the
weak LP differ, the strong one being at least as large, and that the flow
relaxation of `MCF` is therefore the weakest of the three bounds, while it is
the cheapest to compute. For the unsplittable CFL (bit 2 of `wf` set, `wf` 4
or 5) the knapsacks have binary $X$, their convex hulls are in general
strictly smaller than $S_i$, and the Lagrangian dual is at least as strong as
the strong LP, and possibly stronger.

## Walk-through

- $B_1$ is the CFL problem, and $B_2$ is the R3 Block that `get_R3_Block()`
  makes out of the first element of the extra `Configuration` of an
  `R3BSolver`: a copy of $B_1$ in the `cuts` and `LD` arrangements (so that
  $B_2$ can be given a different formulation and solved without disturbing
  $B_1$), or the `MCFBlock` flow relaxation in the `MCF` one
  ([§10.5](10-r3block.md#sec-10-5)).
- The `BlockConfig`s decide the *formulation*: in `cuts`, $B_2$ is the
  knapsack formulation with the strong constraints separated on demand,
  while $B_1$ is the natural one with the same constraints; in `LD`, $B_2$
  is the knapsack formulation, which grows the `BinaryKnapsackBlock`
  sub-`Block`s of [Chapter 12](12-sub-block.md#ch-12); in `MCF`, $B_2$ is an
  `MCFBlock` and needs no choice of formulation.
- The `BlockSolverConfig`s decide the *`:Solver`s*, those of $B_1$ and,
  through the `R3BSolver`s, those of each $B_2$; switching the whole strategy
  is a matter of pointing the tester at a different list of formulations,
  and adding a `:Solver` to a comparison is a matter of adding a line to a
  `BlockSolverConfig`, as the seven `R3BSolver`s of `MCF` show.
- The `R3BSolver`s make the random changes of line 7 of [Algorithm
  R3.1](R3-cfl-three-ways.md#alg-R3-1) meaningful: each change of the data of
  $B_1$ is mapped forward to each $B_2$, so that each relaxation stays
  faithful to the changed problem ([Lemma 10.4](10-r3block.md#lem-10-4), and
  the remark after [Lemma 10.5](10-r3block.md#lem-10-5)) and the bounds can be
  compared again. The tester does not change the data of $B_2$; it reads the
  bounds through the `R3BSolver`s, and in the slope-scaling heuristic it reads
  the solution of $B_2$ mapped back into $B_1$ by their `get_var_solution()`.

## Expected behavior

In each arrangement all the lower bounds agree within the tolerances of line
14 of [Algorithm R3.1](R3-cfl-three-ways.md#alg-R3-1), at the first solve and
after each round of changes ([Lemma R3.1](R3-cfl-three-ways.md#lem-R3-1)); the
`MCF` bounds are the lowest, and the `cuts` and `LD` ones coincide.

The batteries select the changes by the bit-mask of the option `-k`.
[`batches/batch`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch)
and
[`batches/batch-cuts`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-cuts)
set it to 831, i.e., the changes of the costs, capacities and demands and the
closing and reopening of facilities (bits 0 to 5), some of them made through
the abstract representation (bit 8), and the bit 9;
[`batches/batch-ld`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ld)
sets it to 257, i.e., only the facility costs, with bit 8. The bit 9 is
required by the `MCF` arrangement: it keeps the facility costs nonnegative and
scales the absolute tolerances of the `:Solver`s of each $B_2$ to the data,
without which the weak LP and the flow relaxation drift apart after a few
rounds.

After the command line, which the batteries print, the tester prints by
default only a final line stating whether all the comparisons have passed.
With the option `-v`, or for a comparison that fails, it also prints at each
solve one line with the time of
each `:Solver`, the bound of each one (`S0` is the first `:Solver` of the
`BlockSolverConfig`) and the verdict,

```
t₀ - t₁ - … | S0 = v₀  S1 = v₁  …  -> OK(f)
```

where the verdict `OK(f)` means that the bounds agree and pin the optimum,
and `KO` that they do not. After a failure before the last round, the
tester also writes $B_1$
and each $B_2$ in the text files `CFL1.txt`, `CFL2.txt` and so on, and
stops. The numbers depend on the instance, while the three comparisons are
always made by the same executable, with different configuration files.

*Why a MIP engine may print different numbers.* The list
[`CFLBForm-ldvscuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ldvscuts.txt)
compares the `LD` bound with that of $B_1$ solved by
[`CFLBSCfg-ldvscuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ldvscuts.txt),
whose `GRBMILPSolver` has the default `intRelaxIntVars` 0 and a node limit
of 1. The problem is then declared to the engine as a MILP, and the bound that
it reports at the end of the root node is that of the problem after its MIP
presolve, with the separated strong constraints; the reductions of the
presolve can move this bound away from the strong LP value. The difference
with the `LD` bound then has no definite sign, and it is larger than the
tolerance on some instances, which is why
[`batches/batch-ldvscuts`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ldvscuts)
is not among the tests of `ctest`. With `intRelaxIntVars` 2, as in the `cuts`
and `LD` arrangements, the engine solves pure LPs within the separation loop
of `MILPSolver`, at most `intMaxCutPasses` times
([§6.10](06-solver.md#sec-6-10)), and the equality of [Lemma
R3.1](R3-cfl-three-ways.md#lem-R3-1) holds, as
[`batches/batch-ld`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ld)
checks.

## Variations

*Another MIP engine.* Replacing `GRBMILPSolver` with `CPXMILPSolver`,
`SCIPMILPSolver` or `HiGHSMILPSolver` in [`CFLBSCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-cuts.txt) and [`R3BSCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BSCfg-cuts.txt) changes the engine of
the `cuts` arrangement with no change to the program (HiGHS, which is
open-source, needs no commercial license). The parameters
`Threads`, `Cuts` and `NodeLimit` of [`MILPCfg-cuts.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/MILPCfg-cuts.txt), however, are those of Gurobi, and
they have to be replaced by the corresponding ones of the new engine.

*The slope-scaling heuristic.* A nonzero number of iterations with `-s` turns
line 17 of [Algorithm R3.1](R3-cfl-three-ways.md#alg-R3-1) into the
slope-scaling heuristic of [Algorithm 10.1](10-r3block.md#alg-10-1), run with
the R3 Block of each `R3BSolver` in turn (the `BlockSolverConfig` must then
also attach to $B_1$ a `:Solver` that is not an `R3BSolver`). With the `MCF`
or `LD` relaxation this gives an upper bound to set against the lower one,
from the same tester.

*Lagrangian with a heuristic, or Benders.*
[Recipe R4](R4-cfl-lagrangian.md#rec-R4) takes the `LD` arrangement
further, adding `PrimalProximalHeur` to turn the convexified solution
into a feasible one, and [Recipe R5](R5-cfl-benders.md#rec-R5) solves
the same CFL through the Benders formulation; both are reached, again,
by changing configuration rather than code.
