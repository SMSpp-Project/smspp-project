# [Recipe R5](R5-cfl-benders.md#rec-R5) — CFL via Benders cuts with a user-cut callback {#rec-R5}

> **Counterpart in the source tree:**
> `tests/CapacitatedFacilityLocation/Ben/` (the `Ben` configuration
> folder of the `CFL_test` executable of [Recipe R3](R3-cfl-three-ways.md#rec-R3)).

## Goal

Solve a Capacitated Facility Location problem by *Benders
decomposition*: put it in its Benders formulation, where the master
keeps only the design variables and an epigraphic variable, and the
transportation sub-problem is a hidden `MCFBlock` wrapped in a
`BendersBFunction`. An `:MILPSolver` solves the master, and its
user-cut callback emits Benders optimality (and, in the
feasibility-cuts variant, feasibility) cuts on demand. This recipe
shows the `BendersBFunction` of [Chapter 14](14-lag-benders-bfunction.md#ch-14) and the dynamic-constraint
generation of [Chapter 7](07-physical-abstract.md#ch-7) working together.

## Concepts used

- The MCF flow relaxation as the inner sub-problem — [Chapter 10](10-r3block.md#ch-10),
  [§10.5](10-r3block.md#sec-10-5).
- `BendersBFunction` — [Chapter 14](14-lag-benders-bfunction.md#ch-14), [§14.3](14-lag-benders-bfunction.md#sec-14-3).
- On-demand `generate_dynamic_constraints()` — [Chapter 7](07-physical-abstract.md#ch-7), [§7.3](07-physical-abstract.md#sec-7-3).
- `Configuration`, including `f_extra_Configuration` for the hidden
  inner `Block` — [Chapter 11](11-configuration.md#ch-11), [§11.6](11-configuration.md#sec-11-6).

## What the configuration sets up

Again the driver is `CFL_test`; the `Ben/` files arrange:

- `BPar2.txt` puts the CFL `Block` in the **Benders formulation**
  (`SimpleConfiguration<int>` value `3`, the *feasibility-cuts*
  variant; value `2` selects the slack-arcs variant). The master
  abstract representation then has only the design variables $y_i$,
  a single epigraphic variable $v$, the bound $v \ge \mathrm{LB}(v)$,
  and a *dynamic* group of Benders cuts; the transportation
  sub-problem is held in a hidden `BendersBFunction` wrapping an
  `MCFBlock` (the very flow relaxation of [§10.5](10-r3block.md#sec-10-5)), built by
  `build_BendersBFunction()` and *not* a sub-`Block` of the CFL
  `Block`.
- That hidden `BendersBFunction` and its inner `MCFBlock` cannot be
  reached by the recursive configuration machinery (the inner
  `Block` has no father), so they are configured through
  `f_extra_Configuration` ([§11.6](11-configuration.md#sec-11-6)): in `Ben/BPar2.txt` it is a
  `SimpleConfiguration<std::pair<Configuration*,Configuration*>>`
  whose first element carries the R3-Block / slack-scaling
  parameters and whose second is the `ComputeConfig` of the
  `BendersBFunction` (which in turn configures, and attaches a
  `:Solver` to, the inner `MCFBlock`).
- `BSPar2.txt` attaches the master solver — a `GRBMILPSolver`
  (Gurobi) in the shipped configuration; any `:MILPSolver` works.

## How a solve proceeds

1. The `:MILPSolver` works on the master MIP over $(y, v)$.
2. Whenever it has a candidate design $\hat y$, its user-cut /
   lazy callback fires and calls the CFL `Block`'s
   `generate_dynamic_constraints()` ([§7.3](07-physical-abstract.md#sec-7-3)).
3. That evaluates the `BendersBFunction` at $\hat y$ — *one solve
   of the hidden `MCFBlock`* — to obtain the transportation cost
   $\varphi(\hat y)$ and a linearization ([§14.3](14-lag-benders-bfunction.md#sec-14-3)).
4. A Benders cut is added to the master's dynamic-constraint group:
   an *optimality* cut from the `MCFBlock`'s dual when $\hat y$ is
   feasible for the transportation problem, or — in the
   feasibility-cuts variant — a *feasibility* cut from the Farkas
   ray when it is not.
5. The master continues, now constrained by the new cut, until no
   violated cut is found and the incumbent is optimal.

The whole loop is driven by the master `:MILPSolver`; SMS++
supplies the `BendersBFunction` evaluation and the cut, the
`:MILPSolver` supplies the branch-and-cut and the callback hook.

## Walk-through

- The Benders formulation is selected exactly as the other CFL
  formulations are — a single integer in
  `f_static_variables_Configuration` ([§11.8](11-configuration.md#sec-11-8)) — value `2` or `3`
  choosing the slack-arcs or feasibility-cuts variant.
- The hidden `MCFBlock` is the same flow relaxation that [Recipe R3](R3-cfl-three-ways.md#rec-R3)'s
  `MCF/` configuration uses directly ([§10.5](10-r3block.md#sec-10-5)); here it is *internal*
  to the `BendersBFunction` rather than a user-visible R3Block, the
  double duty noted in [§10.5](10-r3block.md#sec-10-5).
- The cut emission rides on the existing dynamic-constraint
  mechanism ([§7.3](07-physical-abstract.md#sec-7-3)): no special Benders plumbing in the driver, just
  a `generate_dynamic_constraints()` that the `:MILPSolver`'s
  callback calls.

## Status — planned: `BendersDecompositionSolver`

This recipe works *today* because the cuts are emitted by the
master `:MILPSolver`'s own user-cut callback. What does **not** yet
exist is a `BendersDecompositionSolver` that would build an
enclosing Benders master automatically for any suitable `Block` —
the Benders counterpart of what `LagrangianDualSolver` does for the
Lagrangian dual ([Recipe R4](R4-cfl-lagrangian.md#rec-R4)). It is documented in the project's
plans but is not released at version 0.6.0; until it is, the
explicit user-cut-callback arrangement of this recipe is the way to
run Benders on a CFL `Block`. **Status — planned.**

## Expected behaviour

The Benders master converges to the *same optimum* as the direct
MILP of [Recipe R3](R3-cfl-three-ways.md#rec-R3)'s `cuts/` (it is the same problem, decomposed),
solving a sequence of cheap `MCFBlock` transportation problems
instead of carrying the transportation variables in the master.
Whether this is faster than the monolithic MILP depends on the
instance: Benders pays off when the design space is small relative
to the transportation one (few facilities, many customers), which
is the regime CFL often inhabits.

## Variations

**Slack-arcs versus feasibility-cuts.** Value `2` builds the inner
`MCFBlock` with big-M slack arcs, so $\varphi(\hat y)$ is finite for
*every* design and only optimality cuts are emitted; value `3`
builds it without slack arcs, so an infeasible $\hat y$ yields a
feasibility cut from the Farkas ray ([§3.5](03-mental-model.md#sec-3-5), [§14.3](14-lag-benders-bfunction.md#sec-14-3)). The
feasibility-cuts variant preserves infeasibility detection and is,
on the shipped test instances, both faster and at least as robust
under modifications; the slack-arcs variant is the choice when a
finite $\varphi$ is preferred over an explicit infeasibility report.

**Swap the master MIP back-end.** As in [Recipe R3](R3-cfl-three-ways.md#rec-R3), the
`GRBMILPSolver` in `BSPar2.txt` can be replaced by
`CPXMILPSolver`, `SCIPMILPSolver` or `HiGHSMILPSolver` with no
other change.

**Configure the inner MCF solver.** The `:Solver` that evaluates
the hidden `MCFBlock` is set through the `ComputeConfig` carried in
`f_extra_Configuration` ([§11.6](11-configuration.md#sec-11-6)); forgetting to provide it is the
most common BenForm setup error, since BenForm *requires* a
`:Solver` attached to the inner `Block`.
