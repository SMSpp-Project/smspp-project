# Recipe R4: CFL by Lagrangian decomposition, with `PrimalProximalHeur` {#rec-R4}

> *Counterpart in the source tree:*
> [`tests/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt) (the `LD`
> arrangement of the `CFL_test` executable of
> [Recipe R3](R3-cfl-three-ways.md#rec-R3), run by [`batches/batch-ld`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/batches/batch-ld)).

## Goal

Solve a Capacitated Facility Location problem by *Lagrangian
decomposition*: put the problem in its knapsack formulation, dualise
the customer satisfaction constraints with a `LagrangianDualSolver`,
and obtain a strong lower bound together with a convexified primal
solution. Then turn that convexified (fractional) solution into a
feasible one by replacing the `Solver` with `PrimalProximalHeur`,
which is a change of one line of configuration. This recipe shows the
decomposition machinery of Chapters [12](12-sub-block.md#ch-12) and
[14](14-lag-benders-bfunction.md#ch-14) at work.

## Concepts used

- the tree of sub-`Block`s of KskForm ([Chapter 12](12-sub-block.md#ch-12));
- `LagBFunction` and `LagrangianDualSolver`
  ([Chapter 14](14-lag-benders-bfunction.md#ch-14));
- `BundleSolver` and the reoptimization vocabulary
  ([Chapter 13](13-function-family.md#ch-13));
- the configuration of the formulation and of the `Solver`
  ([Chapter 11](11-configuration.md#ch-11)).

## What the configuration sets up

The driver is the `CFL_test` of [Recipe R3](R3-cfl-three-ways.md#rec-R3),
and only the files that [`CFLBForm-ld.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBForm-ld.txt) names for the R3 Block matter
here.

- [`CFLBCfg-KF-LD.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBCfg-KF-LD.txt) puts the copy of the CFL `Block` in the knapsack
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

What `LagrangianDualSolver` does with this
([§14.2](14-lag-benders-bfunction.md#sec-14-2)) is to build a hidden
`LagrangianDualBlock`, to wrap the `BinaryKnapsackBlock` of each
facility in a `LagBFunction` that dualises the linking constraints,
and to run the inner `BundleSolver` on the sum of those
`LagBFunction`s. Each `LagBFunction` is evaluated by solving its
knapsack, by a `CoreDPBinaryKnapsackSolver` in [`LPBSCfg-K-DP.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/LPBSCfg-K-DP.txt) or
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
   master CFL Block: customer satisfaction linking constraints (dualised)
```

## The setup, in code

In a program the same arrangement reads, with the same configuration
files:

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include "BlockSolverConfig.h"

#include <fstream>
#include <iostream>

using namespace SMSpp_di_unipi_it;

int main( int argc , char ** argv )
{
 CapacitatedFacilityLocationBlock cfl;
 std::ifstream in( argv[ 1 ] );  // a CFL instance in ORLib format
 cfl.load( in );

 // the Knapsack Formulation (wf = 1), whose static constraints value 1
 // builds the customer satisfaction ("sat") linking constraints alone
 auto bc = dynamic_cast< BlockConfig * >(
   Configuration::deserialize( "CFLBCfg-KF-LD.txt" ) );
 bc->apply( &cfl );
 delete bc;
 cfl.generate_abstract_variables();    // one BinaryKnapsackBlock each
 cfl.generate_abstract_constraints();  // the "sat" constraints
 cfl.generate_objective();

 // a LagrangianDualSolver, whose ComputeConfig names the inner
 // BundleSolver and the BlockSolverConfig of the knapsacks
 auto bsc = dynamic_cast< BlockSolverConfig * >(
   Configuration::deserialize( "CFLBSCfg-ld-LDS.txt" ) );
 bsc->apply( &cfl );
 bsc->clear();

 auto lds = static_cast< CDASolver * >( cfl.get_registered_solvers().front() );
 lds->compute();  // solves the Lagrangian dual
 std::cout << "Lagrangian bound = " << lds->get_lb() << std::endl;
 lds->get_var_solution();  // the convexified solution in cfl

 bsc->apply( &cfl );  // cleanup (Lemma 11.2)
 delete bsc;
 return( 0 );
 }
```

## Walk-through

- The `BlockConfig` selects KskForm
  ([§11.8](11-configuration.md#sec-11-8)), and the `generate_*()` calls
  build the tree of the `BinaryKnapsackBlock`s
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
  while the bundle keeps the linearizations of Lemma 14.1, which do not
  depend on the point.

## From a bound to a feasible solution: `PrimalProximalHeur`

`LagrangianDualSolver` returns a bound and a convexified primal
solution, which is in general fractional, hence not a feasible CFL
solution. To obtain a feasible one,
[`PrimalProximalHeur`](https://smspp.gitlab.io/smspp-project/d4/d41/class_s_m_spp__di__unipi__it_1_1_primal_proximal_heur.html),
which derives from `LagrangianDualSolver` and implements the
primal-proximal heuristic of Daniilidis and Lemaréchal (see the
Bibliography), augments at each iteration the Lagrangian objective with
a quadratic proximal term $M \lVert x - \hat x \rVert^2$ that pushes
the convexified solution towards a point $\hat x$ (the proximal
center), and accumulates the feasible integer solutions found along the
way. The term can be passed to the inner `LagrangianDualSolver` as a
change of the linear costs because of the following identity.

**Lemma R4.1.** *For $x \in \{0, 1\}^n$ and any $\hat x \in
\mathbb{R}^n$, $\lVert x - \hat x \rVert^2 = \sum_i (1 - 2 \hat x_i)
x_i + \lVert \hat x \rVert^2$; hence, on binary points the proximal term
is an affine function of $x$.*

*Proof.* $\lVert x - \hat x \rVert^2 = \sum_i ( x_i^2 - 2 \hat x_i x_i +
\hat x_i^2 )$, and $x_i^2 = x_i$ for $x_i \in \{0, 1\}$. $\square$

Hence, the heuristic only changes the objective coefficients of the
binary `Variable`s by $M (1 - 2 \hat x_i)$ (the constant being
irrelevant), which the `LagBFunction`s pass to their knapsacks as
ordinary changes of costs. Switching to it is a change of one line in
[`CFLBSCfg-ld-LDS.txt`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/CFLBSCfg-ld-LDS.txt), replacing `LagrangianDualSolver` with
`PrimalProximalHeur`. The new `Solver` adds a few parameters, chiefly
`dbl_penaltyFactor` (the $M$, 0 by default), and gives its own meaning
to `intMaxSol` (how many of the feasible solutions found are kept) and
to `intMaxIter`, which counts the iterations of the heuristic, those of
each inner Lagrangian dual being set in the `ComputeConfig` of the
inner `:Solver`; all the parameters of `LagrangianDualSolver` are
available besides ([`PrimalProximalHeur.h`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/PrimalProximalHeur.h)).

## Expected behaviour

- `LagrangianDualSolver` returns a strong lower bound, equal to the
  bound of the strong LP of [Recipe R3](R3-cfl-three-ways.md#rec-R3) for
  the splittable CFL (Lemma R3.1), together with a convexified primal
  solution that is optimal for the strong LP (Theorem 14.2) but, in
  general, not integer.
- `PrimalProximalHeur` returns the same bound together with feasible
  integer solutions, i.e., valid upper bounds, so that the gap between
  the two can be reported. The quality of the feasible solutions depends
  on the penalty factor $M$, the main tuning parameter of the
  heuristic.

## Variations

*The leaf `Solver`.* The knapsacks of the facilities can be solved by
`CoreDPBinaryKnapsackSolver` or `DPBinaryKnapsackSolver` (exact, for
integer weights) or by a `:MILPSolver`; this is a configuration choice
on the leaf `Block`s (the file named by `str_LagBF_BSCfg`), not a
change of the decomposition.

*In parallel.* Naming `ParallelBundleSolver` instead of
`BundleSolver` as the inner `:Solver` (`str_LDSlv_ISName`, a line
already present, commented out, in the file) evaluates the
`LagBFunction`s of the facilities concurrently
([§17.4](17-parallel.md#sec-17-4)), which is sound since the knapsacks
are independent.

*Tuning the heuristic.* `dbl_penaltyFactor` trades the pressure
towards integrality against the fidelity to the convexified solution:
too small an $M$ may not lead to any feasible point, while too large an
$M$ pins the solution to a possibly poor proximal center.
