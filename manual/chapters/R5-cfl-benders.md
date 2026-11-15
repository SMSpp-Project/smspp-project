# Recipe R5: CFL by Benders cuts with a user-cut callback {#rec-R5}

> *Counterpart in the source tree:*
> `tests/CapacitatedFacilityLocationBlock/CFLBForm-ben.txt` (the
> Benders arrangement of the `CFL_test` executable of
> [Recipe R3](R3-cfl-three-ways.md#rec-R3), run by `batches/batch-ben`).

## Goal

Solve a Capacitated Facility Location problem by *Benders
decomposition*: put it in its Benders formulation, where the master
keeps only the design variables and an epigraphic variable, and the
transportation sub-problem is a hidden `MCFBlock` wrapped in a
`BendersBFunction`. A `:MILPSolver` solves the master, and its cut
callback (or its own separation loop) asks the CFL `Block` for Benders
optimality cuts and, in the feasibility-cuts variant, feasibility cuts.
This recipe shows the `BendersBFunction` of
[Chapter 14](14-lag-benders-bfunction.md#ch-14) and the generation of
dynamic constraints of [Chapter 7](07-physical-abstract.md#ch-7) working
together.

## Concepts used

- the MCF flow relaxation as the inner sub-problem
  ([§10.5](10-r3block.md#sec-10-5));
- `BendersBFunction` ([§14.3](14-lag-benders-bfunction.md#sec-14-3));
- `generate_dynamic_constraints()` on demand
  ([§7.3](07-physical-abstract.md#sec-7-3));
- the configuration, including the `f_extra_Configuration` of the
  hidden inner `Block` ([§11.6](11-configuration.md#sec-11-6)).

## What the configuration sets up

Again the driver is `CFL_test`. The list `CFLBForm-ben.txt` gives the
original `Block` $B_1$ the natural formulation (`CFLBCfg-NF.txt`) solved
as a continuous relaxation (`CFLBSCfg-ben.txt`, a `GRBMILPSolver` with
`intRelaxIntVars` 1), and its R3 Block $B_2$, a copy, the following
files.

- `CFLBCfg-BF.txt` puts the copy in the *Benders formulation*
  (`SimpleConfiguration< int >` of value 3, the feasibility-cuts variant;
  value 2 selects the slack-arcs variant). The master abstract representation
  then has only the design variables $Y_i$, a single epigraphic variable $v$,
  the bound $v \ge \mathrm{LB}(v)$, and a *dynamic* group of Benders cuts; the
  transportation sub-problem is held in a hidden `BendersBFunction` wrapping
  an `MCFBlock` (the flow relaxation of [§10.5](10-r3block.md#sec-10-5), with
  the facility arcs at cost 0 and no constant node), built by
  `build_BendersBFunction()` and *not* a sub-`Block` of the CFL `Block`. Its
  `f_dynamic_constraints_Configuration`, a
  `SimpleConfiguration< std::pair< int , double > >` of value (1, $10^{-6}$),
  gives the relative tolerance of the separation: a cut is added only if its
  violation at the current point exceeds $10^{-6} \max \{ |v^*| , 1 \}$, at
  most one per call (the first value is ignored in this formulation).
- That hidden `BendersBFunction` and its inner `MCFBlock` cannot be reached by
  the recursive configuration machinery (the inner `Block` has no father), and
  therefore they are configured through `f_extra_Configuration`
  ([§11.6](11-configuration.md#sec-11-6)). In `CFLBCfg-BF.txt` it is a
  `SimpleConfiguration< std::pair< Configuration * , Configuration * > >`
  whose first element is a `SimpleConfiguration< std::pair< int , double > >`
  of value (2, 100), of which only the scale 100 of the slack arcs is used,
  and whose second is the `ComputeConfig` of the `BendersBFunction`, whose
  extra `Configuration` is a map with the key `"BlockSolverConfig"`, the
  `BlockSolverConfig` of the inner `MCFBlock`. The shipped file attaches there
  a `GRBMILPSolver` solving the `MCFBlock` as an LP (`intRelaxIntVars` 1),
  rather than an `MCFSolver`, whose duals on closed or zero-capacity arcs,
  which the cuts depend on, are not reliable.
- `BenBSCfg.txt` attaches the master `Solver`, a `GRBMILPSolver` in the
  shipped configuration (any `:MILPSolver` works), with
  `intRelaxIntVars` 2, `intCutSepPar` 1 and `intMaxCutPasses` 10000: it
  solves the continuous relaxation of the master with the separation
  loop of `MILPSolver` ([§6.4](06-solver.md#sec-6-4)), and its bound is
  compared with that of $B_1$. With `intRelaxIntVars` 0 and a nonzero
  `intCutSepPar` the same master is solved as a MILP, the cuts being
  emitted by the callback of the branch-and-cut.

## How a solve proceeds

1. The `:MILPSolver` works on the master over $(Y, v)$.
2. Whenever it has a candidate design $\hat Y$ (an LP solution in the
   separation loop, a node or an incumbent in the branch-and-cut), it
   calls `generate_dynamic_constraints()` of the CFL `Block`
   ([§7.3](07-physical-abstract.md#sec-7-3)).
3. That evaluates the `BendersBFunction` at $\hat Y$, i.e., one solve of
   the hidden `MCFBlock` with the capacities of the facility arcs set to
   $Q_i \hat Y_i$, which gives the transportation cost
   $\varphi(\hat Y)$ and a linearization
   ([§14.3](14-lag-benders-bfunction.md#sec-14-3)).
4. A Benders cut is added to the dynamic group of the master if it is
   violated: an *optimality* cut from the dual of the `MCFBlock` when
   $\hat Y$ is feasible for the transportation problem, or (in the
   feasibility-cuts variant) a *feasibility* cut from the Farkas ray
   when it is not. A first optimality cut, at $Y = (1, \ldots, 1)$, is
   generated with the abstract constraints, so that the master starts
   bounded.
5. The master is solved again, constrained by the new cut, until no
   violated cut is found.

The loop is driven by the master `:MILPSolver`: SMS++ supplies the
evaluation of the `BendersBFunction` and the cut, the `:MILPSolver` the
separation loop or the branch-and-cut with its callback.

## Why the two bounds coincide

The battery `batches/batch-ben` checks, on the small ORLib instances,
that the bound of $B_2$ equals the continuous relaxation of the natural
formulation computed on $B_1$, i.e., the weak LP of
[§10.5](10-r3block.md#sec-10-5). This is the following statement, in
which $\varphi(Y)$ is the optimal value of the inner `MCFBlock` with
the capacities $Q_i Y_i$ ($+\infty$ if it is infeasible), and $\epsilon$
is the relative tolerance of the separation.

**Lemma R5.1.** *(i) The weak LP equals
$\min \{ F Y + \varphi(Y) : Y \in [0, 1]^m \}$ (with the fixings of
$Y$). (ii) When the separation loop of the feasibility-cuts variant
stops, with master solution $(\hat Y, \hat v)$, the master value
$F \hat Y + \hat v$ is not larger than the weak LP, and not smaller
than the weak LP minus $\epsilon \max \{ |\hat v| , 1 \}$.*

*Proof.* (i) For a fixed $Y$, the weak LP minimizes
$\sum_{ij} C_{ij} X_{ij}$ over the $X$ with $\sum_i X_{ij} = 1$,
$\sum_j D_j X_{ij} \le Q_i Y_i$ and $X \in [0, 1]^{m \times n}$, and the
flow bijection $\varphi_{ij} = D_j X_{ij}$ of the proof of Lemma 10.3,
with the facility arcs at cost 0 and capacity $Q_i Y_i$, shows that this
minimum is $\varphi(Y)$; minimizing then over $Y$ gives the claim. (ii)
Every cut in the master is valid by Lemma 14.3: an optimality cut
satisfies $L(Y) \le \varphi(Y)$ for all $Y$, and a feasibility cut is
satisfied by every $Y$ at which the transportation problem is feasible.
Hence, every $(Y, \varphi(Y))$ with $\varphi(Y) < +\infty$ is feasible
for the master, whose value is therefore at most the minimum of (i). At
the stop, no cut is violated at $\hat Y$ by more than the tolerance,
which implies, first, that $\hat Y$ is feasible for the transportation
problem (otherwise the Farkas ray gives a violated feasibility cut, by
Lemma 14.3 (ii)), and, second, that $\hat v \ge \varphi(\hat Y) -
\epsilon \max \{ |\hat v| , 1 \}$, since the optimality cut at $\hat Y$
is exact there (Lemma 14.3 (i)). Then $F \hat Y + \hat v \ge F \hat Y +
\varphi(\hat Y) - \epsilon \max \{ |\hat v| , 1 \}$, which is at least
the minimum of (i) minus the same quantity. $\square$

In the slack-arcs variant $\varphi$ is finite everywhere, the slack
arcs absorbing the demand that the open facilities cannot serve at
their large cost, and the same argument gives the minimum of
$F Y + \varphi(Y)$ with that $\varphi$, which is at most the weak LP
(Lemma 10.3, variant 2) and equals it when the scale of the slack arcs
is large enough for them to be unused at the optimum. With
`intRelaxIntVars` 0 the same argument, applied at every integer
incumbent, shows that the branch-and-cut of the master solves the CFL
itself: for an integer $\hat Y$ the optimality cut is exact, and the
master and the original problem have the same optimal value.

## Walk-through

- The Benders formulation is selected as the other CFL formulations are,
  by one integer in `f_static_variables_Configuration`
  ([§11.8](11-configuration.md#sec-11-8)), the values 2 and 3 choosing
  the slack-arcs or the feasibility-cuts variant.
- The hidden `MCFBlock` is the same flow relaxation that the `MCF`
  arrangement of [Recipe R3](R3-cfl-three-ways.md#rec-R3) uses directly
  ([§10.5](10-r3block.md#sec-10-5)); here it is internal to the
  `BendersBFunction` rather than a user-visible R3 Block.
- The emission of the cuts rides on the existing mechanism of the
  dynamic constraints ([§7.3](07-physical-abstract.md#sec-7-3)): there
  is no Benders-specific plumbing in the driver, only a
  `generate_dynamic_constraints()` that the `:MILPSolver` calls.

## `BendersDecompositionSolver`

In this recipe the cuts are emitted on the request of the master
`:MILPSolver`, and therefore no Benders-specific `:Solver` is needed.
Since version 0.7.0 the master can also be built automatically by
`BendersDecompositionSolver`
([§14.3](14-lag-benders-bfunction.md#sec-14-3)), the Benders
counterpart of `LagrangianDualSolver`
([Recipe R4](R4-cfl-lagrangian.md#rec-R4)). On the CFL this is exercised
by `tests/CapacitatedFacilityLocationBlock/test_bds.cpp` (the
`CFL_BDS_test` of `ctest`, built where `BendersDecompositionSolver` is),
which writes the instance as a two-level `AbstractBlock` (the design
`Variable`s at the root, the transportation subproblems below) and
compares the value given by both regimes (`BSPar-BDS.txt` for the
convex one, `BSPar-BDS-MILP.txt` and `BSPar-BDS-Pareto.txt` for the
MILP one, without and with the Pareto-optimal cuts of Magnanti and Wong,
see the Bibliography) with that of the monolithic solve of the same
model and with that of the Benders formulation of this recipe. The two
arrangements differ in where the cuts go: with the callback they enter
the branch-and-cut of the master while it proceeds, while with
`BendersDecompositionSolver` the master is solved again after each
round of cuts (MILP regime), or stabilized by a bundle method (convex
regime).

## Expected behaviour

In the shipped arrangement the tester prints the two continuous bounds,
which agree within the tolerance of `-T` (Lemma R5.1). With the master
solved as a MILP, the Benders formulation reaches the optimal value of
the CFL, as the monolithic MILP of the natural formulation does,
solving a sequence of transportation problems instead of carrying the
transportation variables in the master. Whether this is faster than the
monolithic MILP depends on the instance; Benders decomposition usually
pays off when the design space is small with respect to the
transportation one (few facilities, many customers), a regime in which
CFL instances often are.

## Variations

*Slack arcs or feasibility cuts.* Value 2 builds the inner `MCFBlock`
with slack arcs of large cost, so that $\varphi(\hat Y)$ is finite for
every design and only optimality cuts are emitted; value 3 builds it
without them, so that an infeasible $\hat Y$ yields a feasibility cut
from the Farkas ray ([§3.5](03-mental-model.md#sec-3-5),
[§14.3](14-lag-benders-bfunction.md#sec-14-3)). The feasibility-cuts
variant keeps the exact bound of Lemma R5.1 and detects infeasibility,
which is why the battery uses it; the slack-arcs variant is the choice
when a finite $\varphi$ is preferred to an explicit report of
infeasibility.

*Another master back-end.* As in
[Recipe R3](R3-cfl-three-ways.md#rec-R3), the `GRBMILPSolver` in
`BenBSCfg.txt` can be replaced by `CPXMILPSolver`, `SCIPMILPSolver` or
`HiGHSMILPSolver` with no other change.

*The inner MCF `Solver`.* The `:Solver` that evaluates the hidden
`MCFBlock` is set through the `ComputeConfig` carried in
`f_extra_Configuration` ([§11.6](11-configuration.md#sec-11-6));
forgetting it is the most common setup error of BenForm, since BenForm
requires a `:Solver` registered to the inner `Block`, and a BenForm
`Block` without it throws `std::logic_error`.
