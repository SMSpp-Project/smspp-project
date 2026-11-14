# Recipe R3: CFL three ways, cuts, MCF relaxation, Lagrangian {#rec-R3}

> *Counterpart in the source tree:*
> `tests/CapacitatedFacilityLocationBlock/`, i.e., one executable
> (`CFL_test`) and, in the same directory, one list of formulations per
> arrangement, given with `-F` (`CFLBForm-cuts.txt`, `CFLBForm.txt` and
> `CFLBForm-ld.txt` for the `cuts`, `MCF` and `LD` arrangements below),
> which names the configuration files of each `Block`; the batteries
> `batches/batch-cuts`, `batches/batch` and `batches/batch-ld` run them
> on sets of instances. This recipe is a reading of that tester.

## Goal

Solve the *same* Capacitated Facility Location instance in three
different ways (a cut-strengthened linear relaxation, a min-cost flow
relaxation, and a Lagrangian dual) without recompiling, by editing only
the configuration files. This is the recipe that shows the separation
in SMS++ of *what the model is* from *how it is solved*: one program,
three solution strategies, selected by configuration alone; and, since
in each arrangement the tester compares two bounds that the theory says
are equal, it is also a check of the lemmas of
[Chapter 10](10-r3block.md#ch-10) and
[Chapter 14](14-lag-benders-bfunction.md#ch-14).

## Concepts used

- R3 Blocks, and an `UpdateSolver` keeping them in sync
  ([Chapter 10](10-r3block.md#ch-10));
- a `BlockConfig` to pick the formulation and a `BlockSolverConfig` to
  pick the `Solver` ([Chapter 11](11-configuration.md#ch-11));
- the tree of sub-`Block`s of KskForm ([Chapter 12](12-sub-block.md#ch-12));
- `LagBFunction` and `LagrangianDualSolver`
  ([Chapter 14](14-lag-benders-bfunction.md#ch-14)).

## The procedure

The driver is configuration-agnostic. Condensed, `CFL_test` does the
following.

1. Read the instance into a `CapacitatedFacilityLocationBlock` $B_1$
   (from an ORLib-style text file or from a netCDF file).
2. Read the list of formulations of `-F`: for $B_1$, a `BlockConfig` and
   a `BlockSolverConfig`; then, for each R3 Block to be compared with it,
   the `Configuration` it is made with (`R3BCfg-*.txt`), its
   `BlockConfig` and its `BlockSolverConfig`.
3. For each R3 Block, call $B_2 \leftarrow$
   `B1->get_R3_Block( r3bc )` with its `Configuration`.
4. Apply the `BlockConfig`s to $B_1$ and to each $B_2$, and generate
   their abstract representations.
5. Apply the `BlockSolverConfig`s to $B_1$ and to each $B_2$, and
   register on $B_1$, for each $B_2$, an `UpdateSolver( B2 , r3bc )`
   ([§10.6](10-r3block.md#sec-10-6)).
6. If the number of slope-scaling iterations (`-s`) is 0, solve $B_1$
   and each $B_2$, and compare their lower bounds, with the relative
   tolerance of `-T` ($10^{-5}$ by default); then change the data of
   $B_1$ at random a number of times, the `UpdateSolver`s forwarding
   each change to the $B_2$, solve again and compare again.
7. Otherwise, run the slope-scaling loop of
   [§10.7](10-r3block.md#sec-10-7): solve $B_2$, read its solution into
   its `Variable`s, `map_back_solution( B2 , r3bc )` into $B_1$, change
   the fixed costs of $B_1$ (forwarded to $B_2$), and repeat.

The behaviour is decided by the list of formulations and by the five
configuration files it names, and it is *those*, not the code, that
differ between the three arrangements.

## The three configurations

| | `cuts` | `MCF` | `LD` |
|---|---|---|---|
| list of formulations (`-F`) | `CFLBForm-cuts.txt` | `CFLBForm.txt` | `CFLBForm-ld.txt` |
| $B_1$: formulation | `CFLBCfg-NF-SFsep.txt` | `CFLBCfg-NF.txt` | `CFLBCfg-NF-SF.txt` |
| $B_1$: `Solver` | `CFLBSCfg-cuts.txt` | `CFLBSCfg-LP.txt` | `CFLBSCfg-ld.txt` |
| $B_2$: R3 Block | `R3BCfg-copy.txt` (0, a copy) | `R3BCfg-MCF.txt` (1, an `MCFBlock`) | `R3BCfg-copy.txt` (0, a copy) |
| $B_2$: formulation | `CFLBCfg-KF-SFsep.txt` | `MCFBCfg.txt` | `CFLBCfg-KF-LD.txt` |
| $B_2$: `Solver` | `CFLBSCfg-cuts.txt` | `MCFBSCfg.txt` | `CFLBSCfg-ld-LDS.txt` |
| bound compared | strong LP, twice | weak LP and flow relaxation | strong LP and Lagrangian dual |

The rows of the table read as follows. In the formulations, NF is the
natural formulation and KF the knapsack one (`wf` 0 and 1 in the
`f_static_variables_Configuration`); "SF" means that the third bit of
the `f_static_constraints_Configuration` (value 7) adds the group of the
strong forcing constraints $X_{ij} \le Y_i$, as dynamic `Constraint`s
separated on demand, and "sep" that the
`f_dynamic_constraints_Configuration` gives the separation parameters
(at most 375 constraints per round, with violation at least
$10^{-4}$). `CFLBSCfg-cuts.txt` and `CFLBSCfg-ld.txt` attach a
`GRBMILPSolver` with `intRelaxIntVars` 2 and `intCutSepPar` 1, i.e., the
continuous relaxation solved as a pure LP, with the `MILPSolver` driving
the separation loop by `generate_dynamic_constraints()` after each LP;
`CFLBSCfg-LP.txt` attaches a `HiGHSMILPSolver` with `intRelaxIntVars` 1
on the continuous relaxation; `MCFBSCfg.txt` attaches 7 `:Solver`s of
the min-cost flow, two `MCFSolver`s and the `MCFLemonSolver`s among
them; and `CFLBSCfg-ld-LDS.txt` attaches a
`LagrangianDualSolver` whose inner `BundleSolver` works on the
knapsacks of the facilities.

## Why the two bounds coincide

With the notation of [§10.5](10-r3block.md#sec-10-5), call *weak LP*
the continuous relaxation of (10.1), i.e., $Y \in [0, 1]^m$, and
*strong LP* the weak LP with the constraints $X_{ij} \le Y_i$ added.
Each arrangement compares two bounds that are equal in theory, which
is what the batteries check on every instance.

**Lemma R3.1.** *For the splittable CFL (`wf` 1), the three
arrangements compare equal values: (i) in `cuts`, both $B_1$ and $B_2$
compute the strong LP; (ii) in `MCF`, $B_1$ computes the weak LP and
$B_2$ the flow relaxation, which is equal to it; (iii) in `LD`, $B_1$
computes the strong LP and $B_2$ the Lagrangian dual of the knapsack
formulation with respect to the constraints $\sum_i X_{ij} = 1$, which
is equal to it.*

*Proof.* (i) The natural and the knapsack formulations have the same
constraints, written in the parent or in the sub-`Block`s of the
facilities, and the separation loop adds violated constraints
$X_{ij} \le Y_i$ until none is violated by more than its tolerance;
hence, both compute the strong LP, up to that tolerance. (ii) This is
Lemma 10.3. (iii) For a facility $i$, let
$K_i = \{ (X_{i\cdot}, Y_i) : \sum_j D_j X_{ij} \le Q_i Y_i ,\;
X_{i\cdot} \in [0, 1]^n ,\; Y_i \in \{0, 1\} \}$ be the set of its
knapsack. It is the union of the two polytopes $\{(0, 0)\}$ and
$P_i \times \{1\}$, with $P_i = \{ X_{i\cdot} \in [0,1]^n :
\sum_j D_j X_{ij} \le Q_i \}$, and therefore its convex hull is
$\{ (\lambda x, \lambda) : x \in P_i ,\; \lambda \in [0, 1] \}$, which
is the set $S_i = \{ (X_{i\cdot}, Y_i) : 0 \le X_{ij} \le Y_i \le 1 ,\;
\sum_j D_j X_{ij} \le Q_i Y_i \}$. Indeed, a point $(\lambda x,
\lambda)$ satisfies the constraints of $S_i$ since $x \in P_i$; and
conversely a point of $S_i$ with $Y_i > 0$ is $(Y_i x, Y_i)$ with
$x = X_{i\cdot} / Y_i \in P_i$, while one with $Y_i = 0$ has
$X_{i\cdot} = 0$. Theorem 14.2 holds when $X$ is a finite union of
polytopes, as $K = \prod_i K_i$ is, since a linear function attains its
maximum over such a set at one of the finitely many vertices of the
polytopes, whose convex hull is $\mathrm{conv}(K)$, and its proof
applies verbatim to the set of these vertices. Hence, the Lagrangian
dual is the optimum of the objective of (10.1) over
$\{ \sum_i X_{ij} = 1 \} \cap \prod_i S_i$, which is the strong LP.
$\square$

Hence, the three arrangements reach the bounds by three different
routes (an LP engine with a separation loop, a network simplex, a
bundle method on the dual) and agree on the value. Note that the
strong LP and the weak LP differ, the strong one being at least as
large, and that the flow relaxation of `MCF` is therefore the weakest
of the three bounds, while it is the cheapest to compute. For the
unsplittable CFL (`wf` 5) the knapsacks have binary $X$, their convex
hulls are in general strictly smaller than $S_i$, and the Lagrangian
dual is at least as strong as the strong LP, and possibly stronger.

## Walk-through

- $B_1$ is the CFL problem itself; $B_2$ is whatever `R3BCfg-*.txt`
  asks `get_R3_Block()` to produce: a copy of $B_1$ in the `cuts` and
  `LD` arrangements (so that $B_2$ can be given a different formulation
  and solved without disturbing $B_1$), or the `MCFBlock` flow
  relaxation in the `MCF` one ([§10.5](10-r3block.md#sec-10-5)).
- The `BlockConfig`s decide the *formulation*: in `cuts`, $B_2$ is the
  knapsack formulation with the strong constraints separated on demand,
  while $B_1$ is the natural one with the same constraints; in `LD`,
  $B_2$ is the knapsack formulation, which grows the
  `BinaryKnapsackBlock` sub-`Block`s of
  [Chapter 12](12-sub-block.md#ch-12); in `MCF`, $B_2$ is an
  `MCFBlock` and needs no choice of formulation.
- The `BlockSolverConfig`s decide the *`Solver`*, and switching the
  whole strategy is a matter of pointing the driver at a different list
  of formulations.
- The `UpdateSolver` registered on $B_1$ is what makes both the random
  changes of step 6 and the slope-scaling loop work: each change of the
  data of $B_1$ is mapped forward to $B_2$
  ([§10.6](10-r3block.md#sec-10-6)), so that the relaxation stays
  faithful to the changed problem (Lemma 10.4) and the bounds can be
  compared again.
- `map_back_solution( B2 , r3bc )` brings the solution of $B_2$ back
  into $B_1$, where the slope-scaling heuristic reads it.

## Expected behaviour

In each arrangement the tester prints the two lower bounds, and they
agree within the tolerance of `-T`, before and after each random
change (Lemma R3.1); the `MCF` bounds are the lowest, and the `cuts`
and `LD` ones coincide. The exact numbers depend on the instance; the
point that survives every instance is that the same executable produced
all three by configuration alone.

*Why a MIP engine may print different numbers.* The list
`CFLBForm-ldvscuts.txt` compares the `LD` bound with that of $B_1$
solved by `CFLBSCfg-ldvscuts.txt`, which has the default
`intRelaxIntVars` 0 and a node limit of 1: the problem is then declared
to the back-end as a MILP, and the back-end (Gurobi, say) applies its
MIP presolve, its bound strengthening and its own cuts at the root
node, which can move the reported bound away from the strong LP value,
in either direction with respect to the `LD` one. That extra
strengthening, and not a difference in the relaxation, is what makes
the two numbers differ, and it is why `batches/batch-ldvscuts` is not
among the tests of `ctest`. With `intRelaxIntVars` 2, as in the `cuts`
and `LD` arrangements, the back-end solves a pure LP and the
`MILPSolver` drives the separation loop (as long as `intCutSepPar` is
nonzero, at most `intMaxCutPasses` times, [§6.4](06-solver.md#sec-6-4)),
and the equality of Lemma R3.1 holds, which is what `batches/batch-ld`
checks.

## Variations

*Another MIP back-end.* In `CFLBSCfg-cuts.txt`, replacing
`GRBMILPSolver` with `CPXMILPSolver`, `SCIPMILPSolver` or
`HiGHSMILPSolver` changes the underlying engine with no other change,
the open-source HiGHS being the one that needs no commercial licence.

*The slope-scaling heuristic.* A nonzero number of iterations with `-s`
turns the "solve and compare" path into the slope-scaling loop of step
7: solve the relaxation, map the solution back, adjust the fixed costs
of $B_1$, repeat. With the `MCF` or `LD` relaxation this produces a
feasible CFL solution, i.e., an upper bound to set against the lower
one, from the same driver.

*Lagrangian with a heuristic, or Benders.*
[Recipe R4](R4-cfl-lagrangian.md#rec-R4) takes the `LD` arrangement
further, adding `PrimalProximalHeur` to turn the convexified solution
into a feasible one, and [Recipe R5](R5-cfl-benders.md#rec-R5) solves
the same CFL through the Benders formulation; both are reached, again,
by changing configuration rather than code.
