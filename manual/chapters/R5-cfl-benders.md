# Recipe R5: CFL by Benders cuts generated on demand {#rec-R5}

[Source: [`tests/CapacitatedFacilityLocationBlock/CFLBForm-ben.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ben.txt)
and the files it leads to, run by the `CFL_test` of
[Recipe R3](R3-cfl-three-ways.md#rec-R3) in
[`batches/batch-ben`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ben);
[`test_bds.cpp`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/test_bds.cpp);
[`CapacitatedFacilityLocationBlock.h`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h),
[`BendersBFunction.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BendersBFunction.h)]

We are concerned in this recipe with the solution of a Capacitated Facility
Location problem by *Benders decomposition*: the problem is put in its Benders
formulation, where the master keeps only the design variables and an
epigraphic variable, and the transportation subproblem is a hidden `MCFBlock`
wrapped in a `BendersBFunction`. A `:MILPSolver` solves the master, and its
separation loop (or the lazy-constraint callback of its branch-and-cut) asks
the CFL `Block` for Benders optimality cuts and, in the feasibility-cuts
variant, feasibility cuts. The recipe shows the `BendersBFunction` of [Chapter
14](14-lag-benders-bfunction.md#ch-14) and the generation of dynamic
constraints of [Chapter 7](07-physical-abstract.md#ch-7) working together.

## Concepts used

- the MCF flow relaxation as the inner subproblem
  ([§10.5](10-r3block.md#sec-10-5));
- `BendersBFunction` ([§14.3](14-lag-benders-bfunction.md#sec-14-3));
- `generate_dynamic_constraints()` on demand
  ([§7.3](07-physical-abstract.md#sec-7-3));
- the configuration, including the `f_extra_Configuration` of the
  hidden inner `Block` ([§11.6](11-configuration.md#sec-11-6)).

## What the configuration sets up

Again the driver is `CFL_test`. The list [`CFLBForm-ben.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ben.txt) gives the
original `Block` $B_1$ the natural formulation ([`CFLBCfg-NF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-NF.txt)) and
the `BlockSolverConfig` [`CFLBSCfg-ben.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ben.txt), which registers a
`GRBMILPSolver` solving $B_1$ as a continuous relaxation
(`intRelaxIntVars` 1) and an `R3BSolver` (since SMS++ 0.9.0,
[§10.9](10-r3block.md#sec-10-9)). The latter
solves $B_1$ through an R3 Block $B_2$, a copy, that it makes out of the
files named by
[`R3BSCfg-ben.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BSCfg-ben.txt) ([`R3BCfg-copy.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/R3BCfg-copy.txt) and the two files below),
as in [Recipe R4](R4-cfl-lagrangian.md#rec-R4).

- [`CFLBCfg-BF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-BF.txt) puts the copy in the *Benders formulation*
  (`SimpleConfiguration< int >` of value 3, the feasibility-cuts variant;
  value 2 selects the slack-arcs variant). The master abstract representation
  then has only the design variables $Y_i$, a single epigraphic variable $v$,
  the bound $v \ge \mathrm{LB}(v)$, and a *dynamic* group of Benders cuts; the
  transportation subproblem is held in a hidden `BendersBFunction` wrapping
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
  ([§11.6](11-configuration.md#sec-11-6)). In [`CFLBCfg-BF.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-BF.txt) it is a
  `SimpleConfiguration< std::pair< Configuration * , Configuration * > >`
  whose first element is a `SimpleConfiguration< std::pair< int , double > >`
  of value (2, 100), of which only the scale 100 of the slack arcs is used,
  and whose second is the `ComputeConfig` of the `BendersBFunction`, whose
  extra `Configuration` is a map with the key `"BlockSolverConfig"`, the
  `BlockSolverConfig` of the inner `MCFBlock`. The shipped file attaches there
  a `GRBMILPSolver` solving the `MCFBlock` as an LP (`intRelaxIntVars` 1).
  The reason, given in a comment of the file, for not using an
  `MCFSolver` is that the cuts depend on the duals of the closed or
  zero-capacity arcs, on which those of `MCFSolver` are not considered
  reliable.
- [`BenBSCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/BenBSCfg.txt) attaches the master `Solver`, a `GRBMILPSolver` in the
  shipped configuration, with `intRelaxIntVars` 2, `intCutSepPar` 1 and
  `intMaxCutPasses` 10000: it solves the continuous relaxation of the
  master with the separation loop that `MILPSolver` runs around each
  solve of the LP
  ([`MILPSolver.h`](https://gitlab.com/smspp/milpsolver/-/blob/develop/include/MILPSolver.h), `intRelaxIntVars`), and
  its bound is compared with that of $B_1$. With `intRelaxIntVars` 0 and
  an `intCutSepPar` whose bits ask for the separation of lazy constraints
  at each feasible solution (e.g., bit 2 in `GRBMILPSolver`, as the header
  of each back-end specifies), the same master is solved as a MILP, the
  cuts being emitted by the callback of the branch-and-cut, which only
  `CPXMILPSolver`, `GRBMILPSolver` and `SCIPMILPSolver` install
  (`HiGHSMILPSolver` installs one that only checks the cutoffs, since HiGHS
  does not yet accept constraints from a callback;
  [§D.2](D-concept-block-map.md#sec-D-2)).

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
   when it is not. A first cut, at the point where every facility that is
   not fixed is open (the fixed ones keeping their value), is added
   unconditionally together with the abstract constraints, so that the
   master starts bounded and the `:MILPSolver` enters its separation.
5. The master is solved again, constrained by the new cut, until no
   violated cut is found.

The loop is driven by the master `:MILPSolver`: SMS++ supplies the
evaluation of the `BendersBFunction` and the cut, the `:MILPSolver` the
separation loop or the branch-and-cut with its callback.

## Why the two bounds coincide

The battery [`batches/batch-ben`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ben) checks, on the small ORLib instances,
that the bound of $B_2$ equals the continuous relaxation of the natural
formulation computed on $B_1$, i.e., the weak LP of
[§10.5](10-r3block.md#sec-10-5). This is the following statement, in
which $\varphi(Y)$ is the optimal value of the inner `MCFBlock` with
the capacities $Q_i Y_i$ ($+\infty$ if it is infeasible), and $\epsilon$
is the relative tolerance of the separation.

<span id="lem-R5-1"></span>**Lemma R5.1.** *(i) The weak LP equals $\min \{ F
Y + \varphi(Y) : Y \in [0, 1]^m \}$ (with the fixings of $Y$). (ii) When the
separation loop of the feasibility-cuts variant stops because no cut is
violated by more than the tolerance at the master solution $(\hat Y, \hat v)$,
$\hat Y$ being feasible for the transportation problem and the inner `:Solver`
having returned `kOK` at it, the master value $F \hat Y + \hat v$ is not
larger than the weak LP, and not smaller than the weak LP minus $\epsilon \max
\{ |\hat v| , 1 \}$.*

*Proof.* (i) For a fixed $Y$, the weak LP minimizes $\sum_{ij} C_{ij} X_{ij}$
over the $X$ with $\sum_i X_{ij} = 1$, $\sum_j D_j X_{ij} \le Q_i Y_i$ and $X
\in [0, 1]^{m \times n}$, and the flow bijection $\varphi_{ij} = D_j X_{ij}$
of the proof of [Lemma 10.3](10-r3block.md#lem-10-3), with the facility arcs
at cost 0 and capacity $Q_i Y_i$, shows that this minimum is $\varphi(Y)$;
minimizing then over $Y$ gives the claim. (ii) Every cut in the master is
valid by [Lemma 14.3](14-lag-benders-bfunction.md#lem-14-3): an optimality cut
satisfies $L(Y) \le \varphi(Y)$ for all $Y$, and a feasibility cut is
satisfied by every $Y$ at which the transportation problem is feasible. Hence,
every $(Y, \varphi(Y))$ with $\varphi(Y) < +\infty$ is feasible for the
master, whose value is therefore at most the minimum of (i). At the stop, the
optimality cut at $\hat Y$, which is exact there ([Lemma
14.3](14-lag-benders-bfunction.md#lem-14-3) (i)), is not violated by more than
the tolerance, i.e., $\hat v \ge \varphi(\hat Y) - \epsilon \max \{ |\hat v| ,
1 \}$. Then $F \hat Y + \hat v \ge F \hat Y + \varphi(\hat Y) - \epsilon \max
\{
|\hat v| , 1 \}$, which is at least the minimum of (i) minus the same
quantity. $\square$

We remark that the hypotheses of (ii) are needed. A feasibility cut is
added under the same relative threshold as an optimality one, so that the
loop may stop at a $\hat Y$ at which the transportation problem is
infeasible. No cut is generated when the inner `:Solver` ends with a
status other than `kOK` or `kInfeasible`. Also, the loop of
[Algorithm 6.1](06-solver.md#alg-6-1) stops after `intMaxCutPasses`
passes.

In the slack-arcs variant $\varphi$ is finite everywhere, the slack arcs
absorbing the demand that the open facilities cannot serve at their large
cost, and the same argument gives the minimum of $F Y + \varphi(Y)$ with that
$\varphi$, which is at most the weak LP
([Lemma 10.3](10-r3block.md#lem-10-3), variant 2) and equals it
when the scale of the slack arcs is large enough for them to be unused at the
optimum. With `intRelaxIntVars` 0 and the separation of lazy constraints at
every incumbent, the same argument, applied at every integer incumbent, shows
that the branch-and-cut of the master solves the CFL itself: for an integer
$\hat Y$ the optimality cut is exact, and the master and the original problem
have the same optimal value.

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
`:MILPSolver`, and therefore no Benders-specific `:Solver` is needed. The
master can also be built automatically by `BendersDecompositionSolver`
([§14.6](14-lag-benders-bfunction.md#sec-14-6)), the Benders counterpart of
`LagrangianDualSolver` ([Recipe R4](R4-cfl-lagrangian.md#rec-R4)). The two
arrangements differ in where the cuts go: here they are generated within the
solve of the master, by the separation loop or the branch-and-cut of its
`:MILPSolver`, while `BendersDecompositionSolver` solves the master again
after each round of cuts (MILP regime), or stabilizes it by a bundle method
(convex regime). The test `CFL_BDS_test`
([`test_bds.cpp`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/test_bds.cpp),
[§14.6](14-lag-benders-bfunction.md#sec-14-6)) compares the two on
the CFL; in particular, it checks that the value of the convex regime equals
that of the Benders formulation of this recipe in its slack-arcs variant
([`BenBCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/BenBCfg.txt), value 2).

## Expected behavior

With the option `-v` (or the environment variable `verbose`), or for a
comparison that fails, `CFL_test` prints at each solve one line with the time
and the value of each of the two `:Solver`s of $B_1$ (the `GRBMILPSolver`, and
the `R3BSolver` whose inner `:Solver` works on $B_2$), followed by the
verdict; otherwise it prints only the final line, as in [Recipe
R3](R3-cfl-three-ways.md#rec-R3). The verdict is positive when the two values
agree within the larger of the tolerance of `-T` and of the accuracy required
of each `:Solver`, i.e., its `dblRelAcc` ([Lemma
R5.1](R5-cfl-benders.md#lem-R5-1)). With the master solved as a MILP, the
Benders formulation reaches the optimal value of the CFL, as the monolithic
MILP of the natural formulation does, solving a sequence of transportation
problems instead of carrying the transportation variables in the master.
Whether this is faster than the monolithic MILP depends on the instance;
Benders decomposition usually pays off when the design space is small with
respect to the transportation one (few facilities, many customers), a regime
in which CFL instances often are.

## Variations

*Slack arcs or feasibility cuts.* Value 2 builds the inner `MCFBlock` with
slack arcs of large cost, so that $\varphi(\hat Y)$ is finite for every design
and only optimality cuts are emitted; value 3 builds it without them, so that
an infeasible $\hat Y$ yields a feasibility cut from the Farkas ray
([§3.5](03-mental-model.md#sec-3-5),
[§14.3](14-lag-benders-bfunction.md#sec-14-3)). The feasibility-cuts variant
keeps the exact bound of [Lemma R5.1](R5-cfl-benders.md#lem-R5-1) and detects
infeasibility, and it is the one of the battery; the slack-arcs variant is the
choice when a finite $\varphi$ is preferred to an explicit report of
infeasibility.

*Another master back-end.* As in
[Recipe R3](R3-cfl-three-ways.md#rec-R3), the `GRBMILPSolver` in
[`BenBSCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/BenBSCfg.txt) can be replaced by `CPXMILPSolver`, `SCIPMILPSolver` or
`HiGHSMILPSolver` with no other change; of these, `CPXMILPSolver` and
`SCIPMILPSolver` can also solve the master as a MILP with the cuts in the
callback.

*The inner MCF `Solver`.* The `:Solver` that evaluates the hidden
`MCFBlock` is set through the `ComputeConfig` carried in
`f_extra_Configuration` ([§11.6](11-configuration.md#sec-11-6)), and it
cannot be omitted: BenForm requires a `:Solver` registered to the inner
`Block`, and a BenForm `Block` without it throws `std::logic_error`.
