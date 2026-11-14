# [Recipe R4](R4-cfl-lagrangian.md#rec-R4) — CFL via Lagrangian decomposition with `PrimalProximalHeur` {#rec-R4}

> **Counterpart in the source tree:**
> `tests/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt` (the
> `LD` arrangement of the `CFL_test` executable of [Recipe R3](R3-cfl-three-ways.md#rec-R3), run
> by `batches/batch-ld`).

## Goal

Solve a Capacitated Facility Location problem by *Lagrangian
decomposition*: put the problem in its Knapsack formulation, dualise
the customer-satisfaction constraints with a `LagrangianDualSolver`,
and obtain a strong lower bound together with a convexified primal
solution. Then turn that convexified (fractional) solution into a
*feasible* one by swapping in `PrimalProximalHeur` — a one-line
configuration change. This recipe shows the decomposition machinery
of Chapters [12](12-sub-block.md#ch-12) and [14](14-lag-benders-bfunction.md#ch-14) doing real work.

## Concepts used

- Sub-`Block` decomposition (the KskForm tree) — [Chapter 12](12-sub-block.md#ch-12).
- `LagBFunction` and `LagrangianDualSolver` — [Chapter 14](14-lag-benders-bfunction.md#ch-14).
- `BundleSolver` and the reoptimization vocabulary — [Chapter 13](13-function-family.md#ch-13).
- `Configuration` (the formulation and solver choice) — [Chapter 11](11-configuration.md#ch-11).

## What the configuration sets up

The driver is the same `CFL_test` of [Recipe R3](R3-cfl-three-ways.md#rec-R3); only the files
that `CFLBForm-ld.txt` names for the R3Block matter here. They
arrange:

- `CFLBCfg-KF-LD.txt` puts the (R3-copy) CFL `Block` in the **Knapsack
  formulation** (`SimpleConfiguration<int>` value `1`). As [Chapter
  12](12-sub-block.md#ch-12) described, this grows one `BinaryKnapsackBlock` sub-`Block`
  per facility, leaving only the customer-satisfaction linking
  constraints $\sum_i X_{ij} = 1$ in the master.
- `CFLBSCfg-ld-LDS.txt` attaches a single solver named
  **`LagrangianDualSolver`**, with a (large) `ComputeConfig` whose
  parameters configure the inner `BundleSolver` that drives the
  dual (norm-based stopping, bundle size per component, master-
  problem solver, the `m1`/`m2`/`m3` serious/null-step thresholds,
  ...), and whose `str_LagBF_BSCfg` names the `BlockSolverConfig`
  of the per-facility knapsacks (`LPBSCfg-K-DP.txt`).

What `LagrangianDualSolver` does with this ([Chapter 14](14-lag-benders-bfunction.md#ch-14), [§14.2](14-lag-benders-bfunction.md#sec-14-2)):
it stealthily builds a hidden `LagrangianDualBlock`, wraps each
per-facility `BinaryKnapsackBlock` in a `LagBFunction` that
dualises the linking constraints, and runs the inner `BundleSolver`
over the sum of those `LagBFunction`s. Each `LagBFunction` is
evaluated by solving its inner knapsack — by a
`CoreDPBinaryKnapsackSolver` in `LPBSCfg-K-DP.txt`, or by an
`:MILPSolver` (`LPBSCfg-K-CX.txt`, or `LPBSCfg-K-LP.txt` for the
continuous relaxation of the knapsack), as configured on the
leaf — and the resulting per-facility solutions are combined into
the *convexified* primal solution that the dual returns alongside
the bound.

```text
            LagrangianDualBlock (hidden, built by LagrangianDualSolver)
              |   BundleSolver drives the dual over the sum below
              +-- LagBFunction_0  --> BinaryKnapsackBlock (facility 0)  --> DP solver
              +-- LagBFunction_1  --> BinaryKnapsackBlock (facility 1)  --> DP solver
              +-- ...
   master CFL Block: customer-satisfaction linking constraints (dualised)
```

## The setup, in code

In a program (rather than via the configuration files) the same
arrangement is:

```cpp
#include <fstream>
#include <iostream>

#include "CapacitatedFacilityLocationBlock.h"
#include "LagrangianDualSolver.h"
#include "BlockSolverConfig.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 CapacitatedFacilityLocationBlock cfl;
 cfl.load( /* ... */ );

 // Knapsack formulation: grows one BinaryKnapsackBlock per facility.
 // We also tell generate_abstract_constraints() to build ONLY the
 // customer-satisfaction (demand) constraints at the master level (the
 // "wc" bit 0), because the facility-capacity constraints are the
 // knapsack constraints living inside the per-facility BinaryKnapsackBlock
 // sub-Blocks and must not be duplicated at the master.
 auto * bc = new BlockConfig;
 bc->f_static_variables_Configuration   = new SimpleConfiguration< int >( 1 );
 bc->f_static_constraints_Configuration = new SimpleConfiguration< int >( 1 );
 cfl.set_BlockConfig( bc );
 cfl.generate_abstract_variables();      // builds the sub-Block tree
 cfl.generate_abstract_constraints();    // only the "sat" linking constraints
 cfl.generate_objective();

 // attach a LagrangianDualSolver (its ComputeConfig, naming the
 // inner BundleSolver and the leaf knapsack solver, is read from a file)
 auto * bsc = new BlockSolverConfig;
 std::ifstream cfg( "BSPar-LDS.txt" ); bsc->load( cfg );
 bsc->apply( & cfl );

 auto * lds = cfl.get_registered_solvers().front();
 lds->compute();                                     // solve the dual

 std::cout << "Lagrangian bound = " << lds->get_lb() << '\n';
 lds->get_var_solution();   // the convexified primal solution in cfl

 bsc->clear(); bsc->apply( & cfl ); delete bsc;      // cleanup (cf. §11.8)
 return 0;
}
```

## Walk-through

- The `BlockConfig` selects KskForm ([§11.8](11-configuration.md#sec-11-8)); the
  `generate_abstract_*` calls build the per-facility
  `BinaryKnapsackBlock` tree ([§12.5](12-sub-block.md#sec-12-5)). The
  `f_static_constraints_Configuration` of value `1` is what makes
  `generate_abstract_constraints()` build *only* the
  customer-satisfaction demand constraints in the master (the `wc`
  bit-mask of [§11.3](11-configuration.md#sec-11-3) / [§7.3](07-physical-abstract.md#sec-7-3)): the facility-capacity constraints are
  not generated at the master at all, because in KskForm they are
  the knapsack constraints of the `BinaryKnapsackBlock`
  sub-`Block`s. Omitting this — letting
  `generate_abstract_constraints()` build everything — would
  wrongly duplicate the capacity constraints at the master level.
- `LagrangianDualSolver`, once attached, is a `CDASolver`
  ([Chapter 6](06-solver.md#ch-6)): `compute()` solves the Lagrangian dual, and the
  bound is read with `get_lb()`, the convexified primal solution
  from the `Block`'s variables once `get_var_solution()` has
  written it there. (`Block::get_valid_lower_bound()` is a
  different thing: a bound that the `:Block` knows from its own
  data, not the one a `:Solver` computes.)
- The reoptimization theme of [Chapter 13](13-function-family.md#ch-13) is what makes the inner
  loop efficient: as the `BundleSolver` moves the multipliers, the
  changes to each inner `BinaryKnapsackBlock` are mapped by its
  `LagBFunction` into the appropriate `FunctionMod*`, so the
  bundle is reoptimized rather than rebuilt ([§14.2](14-lag-benders-bfunction.md#sec-14-2)).

## From a bound to a feasible solution: `PrimalProximalHeur`

`LagrangianDualSolver` returns a bound and a *convexified* primal
solution, which is generally **fractional** — not a feasible CFL
solution. To obtain a feasible one,
[`PrimalProximalHeur`](https://smspp.gitlab.io/smspp-project/d4/d41/class_s_m_spp__di__unipi__it_1_1_primal_proximal_heur.html)
is a *drop-in derived solver* ([§14.2](14-lag-benders-bfunction.md#sec-14-2)): it derives from
`LagrangianDualSolver`, so it sets up the same dual, and adds the
Lagrangian-based primal-proximal heuristic on top — compute the
convexified solution, round it, add a quadratic penalty
$M \lVert x - \lceil x \rfloor \rVert^2$ to push subsequent dual
solutions towards that rounding, and iterate. Switching to it is a
one-line change in `CFLBSCfg-ld-LDS.txt`: replace
`LagrangianDualSolver` with `PrimalProximalHeur`. The new solver
adds a few parameters, chiefly `dbl_penaltyFactor` (the $M$, 0 by
default), and gives its own meaning to `intMaxSol` (how many of the
feasible solutions found are kept) and to `intMaxIter`, which counts
the iterations of the heuristic, those of each inner Lagrangian dual
being set in the `ComputeConfig` of the inner `:Solver`; all of
`LagrangianDualSolver`'s are available besides
(`PrimalProximalHeur.h`).

## Expected behaviour

On a typical CFL instance:

- `LagrangianDualSolver` returns a *strong* lower bound — equal,
  in theory, to the LP-with-strong-cuts bound of [Recipe R3](R3-cfl-three-ways.md#rec-R3)'s
  `cuts` (the Dantzig–Wolfe equivalence noted there) — together
  with a convexified primal solution that is markedly *less
  fractional* than the `MCF` relaxation's, but still not
  integer-feasible in general.
- `PrimalProximalHeur` returns the same bound plus a *feasible*
  (integer, rounded) primal solution, i.e. a valid *upper* bound,
  so the gap between the two can be reported. The quality of the
  feasible solution depends on the penalty factor $M$, which is
  the heuristic's main tuning knob.

## Variations

**Choose the leaf solver.** The per-facility knapsacks can be
solved by `CoreDPBinaryKnapsackSolver` or `DPBinaryKnapsackSolver`
(fast, exact for integer weights) or by an `:MILPSolver`; this is a
configuration choice on the leaf `Block`s (the file named by
`str_LagBF_BSCfg`), not a change to the decomposition.

**Go parallel.** Replacing `BundleSolver` with
`ParallelBundleSolver` as the inner `:Solver` (`str_LDSlv_ISName`)
evaluates the per-facility `LagBFunction`s concurrently ([Chapter 17](17-parallel.md#ch-17), [§17.4](17-parallel.md#sec-17-4)) —
the decomposition into independent knapsacks is exactly what makes
this sound.

**Tune the heuristic.** `dbl_penaltyFactor` trades feasibility
pressure against fidelity to the convexified solution; too small
an $M$ may not round to anything feasible, too large an $M$ pins
the solution to a possibly poor rounding. This is the heuristic's
known sensitivity, flagged honestly in [§14.2](14-lag-benders-bfunction.md#sec-14-2).
