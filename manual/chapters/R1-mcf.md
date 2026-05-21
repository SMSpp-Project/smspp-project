# [Recipe R1](R1-mcf.md#rec-R1) — Solving a Min-Cost Flow instance {#rec-R1}

> **Counterpart in the source tree:** `MCFBlock/test/test.cpp`.
> The program below is a didactic distillation of that tester; the
> tester itself is the runnable, fully-checked version.

## Goal

Take a small directed graph with arc costs, arc capacities and
node deficits; build an `MCFBlock`; solve it with the specialised
`MCFSolver`; read back the optimal flows and the objective value;
then swap the specialised solver for a general-purpose
`:MILPSolver` without changing the surrounding logic. This is the
"hello, world" of SMS++: the smallest complete program that builds
a `Block`, attaches a `Solver`, and reads a solution.

## Concepts used

- `Block` construction and `load(...)` — [Chapter 4](04-block.md#ch-4).
- `Solver` attachment, `compute()`, `sol_type` — [Chapter 6](06-solver.md#ch-6).
- Physical vs abstract representation (why the `MILPSolver`
  variant needs `generate_*`) — [Chapter 7](07-physical-abstract.md#ch-7).
- Reading the solution through the `Block`'s physical accessors —
  [Chapter 9](09-solution.md#ch-9).
- (Variation) selecting the solver through a `BlockSolverConfig` —
  [Chapter 11](11-configuration.md#ch-11).

## The code

```cpp
#include <iostream>

#include "MCFBlock.h"
#include "MCFSolver.h"
#include "MCFSimplex.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 // ---- build the MCFBlock ----------------------------------------
 // a 3-node, 3-arc instance:
 //
 //     (0) --arc0,c=2--> (1) --arc1,c=3--> (2)
 //      \                                  ^
 //       `------------ arc2,c=4 -----------'
 //
 // node deficits b = (-2, 0, +2): node 0 produces 2 units, node 2
 // consumes 2; all arcs have capacity 3.

 MCFBlock mcf;                                  // a root MCFBlock

 MCFBlock::Subset      pSn = { 0 , 1 , 0 };     // arc starting nodes
 MCFBlock::Subset      pEn = { 1 , 2 , 2 };     // arc ending nodes
 MCFBlock::Vec_FNumber pU  = { 3.0 , 3.0 , 3.0 };  // arc capacities
 MCFBlock::Vec_CNumber pC  = { 2.0 , 3.0 , 4.0 };  // arc costs
 MCFBlock::Vec_FNumber pB  = { -2.0 , 0.0 , +2.0 };  // node deficits

 // NOTE: load() takes ending nodes (pEn) before starting nodes (pSn)
 mcf.load( 3 , 3 , pEn , pSn , pU , pC , pB );

 // ---- attach a specialised Solver and solve ---------------------
 auto * solver = new MCFSolver< MCFSimplex >();
 mcf.register_Solver( solver );

 const int status = solver->compute();

 // ---- read the solution -----------------------------------------
 if( status == Solver::kOK ) {
  std::cout << "objective = " << mcf.get_objective_value() << '\n';

  MCFBlock::Vec_FNumber x( mcf.get_NArcs() );
  mcf.get_x( x.begin() );                       // optimal flows
  for( MCFBlock::Index a = 0 ; a < mcf.get_NArcs() ; ++a )
   std::cout << "  flow on arc " << a << " = " << x[ a ] << '\n';

  MCFBlock::Vec_CNumber pi( mcf.get_NNodes() );
  mcf.get_pi( pi.begin() );                      // node potentials (dual)
  for( MCFBlock::Index n = 0 ; n < mcf.get_NNodes() ; ++n )
   std::cout << "  potential of node " << n << " = " << pi[ n ] << '\n';
  }
 else if( status == Solver::kInfeasible )
  std::cout << "infeasible\n";
 else if( status == Solver::kUnbounded )
  std::cout << "unbounded\n";
 else
  std::cout << "solver returned status " << status << '\n';

 // ---- clean up --------------------------------------------------
 mcf.unregister_Solver( solver , /* deleteold = */ true );
 return 0;
}
```

## Walk-through

- The five arrays are the *physical* representation of the
  min-cost flow instance ([Chapter 7](07-physical-abstract.md#ch-7)): start/end node of each arc,
  arc capacities, arc costs, node deficits. `load(...)` copies
  them into the `MCFBlock` ([Chapter 4](04-block.md#ch-4)); recall its idiosyncratic
  parameter order, ending nodes before starting nodes.
- `new MCFSolver< MCFSimplex >()` constructs the specialised
  solver, templated on the classical primal-simplex MCFClass
  algorithm; `register_Solver` enrols it on the `MCFBlock`
  ([Chapter 6](06-solver.md#ch-6)). Registration does *not* transfer ownership of the
  pointer; the `deleteold = true` flag on `unregister_Solver` is
  what asks the framework to `delete` it at the end.
- `solver->compute()` runs the algorithm and returns an
  `sol_type` ([Chapter 6](06-solver.md#ch-6), [§6.3](06-solver.md#sec-6-3)); `kOK` means an optimal solution
  was found.
- The solution is read *through the `MCFBlock`*, not through the
  solver (the convention of [§6.9](06-solver.md#sec-6-9) and [Chapter 9](09-solution.md#ch-9)):
  `get_objective_value()`, `get_x(...)` for the primal flows, and
  `get_pi(...)` for the dual node potentials. `get_rc(...)` would
  give the arc reduced costs; together the three are the natural
  primal–dual data of a min-cost flow, exactly what an
  `MCFSolution` would store ([§9.5](09-solution.md#sec-9-5)).
- Note that this program never calls `generate_abstract_*()`: the
  specialised `MCFSolver` reads the physical representation
  directly and needs no abstract `Variable`s or `Constraint`s.
  (The flows are nonetheless deposited into the always-materialised
  abstract `Variable`s as well — the [§7.6](07-physical-abstract.md#sec-7-6) "half-baked" caveat —
  but the recommended way to read them is the physical accessor
  used here.)

## Expected output

On this instance the cheapest way to move 2 units from node 0 to
node 2 is the direct arc 2 (unit cost 4, total 8), which is
cheaper than the two-arc path 0→1→2 (unit cost 2+3 = 5, total 10).
So:

```
objective = 8
  flow on arc 0 = 0
  flow on arc 1 = 0
  flow on arc 2 = 2
  potential of node 0 = ...
  potential of node 1 = ...
  potential of node 2 = ...
```

The flows and the objective are determined; the exact node
potentials depend on the dual solution the algorithm reports
(several are optimal), so they are shown elided here.

## Variations

**Swap to a general-purpose `:MILPSolver`.** A min-cost flow is a
linear program, so it can also be solved by any `:MILPSolver`
(e.g. `CPXMILPSolver`, `SCIPMILPSolver`, `HiGHSMILPSolver`). The
*surrounding logic does not change* — construct the solver,
`register_Solver`, `compute()`, read the solution through the
`MCFBlock` — but a general-purpose solver reads the *abstract*
representation, which must therefore be built first:

```cpp
 mcf.generate_abstract_variables();
 mcf.generate_abstract_constraints();
 mcf.generate_objective();
 auto * milp = new SCIPMILPSolver();    // or CPX/HiGHS/GRB
 mcf.register_Solver( milp );
 milp->compute();
 // ... read mcf.get_objective_value(), mcf.get_x(...) exactly as before
```

The optimal value is the same (8); the solver is slower on this
trivial instance but works on any `:Block` that can expose an
abstract representation. This is the swap promised in [§6.2](06-solver.md#sec-6-2) and
[§6.9](06-solver.md#sec-6-9).

**Select the solver from a configuration file.** Rather than
hard-coding the choice of solver in the program, the idiomatic
SMS++ way is to read a `BlockSolverConfig` from a text file
([Chapter 11](11-configuration.md#ch-11)) and `apply()` it to the `MCFBlock`. The same
executable then solves with `MCFSolver`, `CPXMILPSolver`, or any
other registered `:Solver` purely by editing the configuration
file — no recompilation. [Recipe R3](R3-cfl-three-ways.md#rec-R3) uses exactly this pattern to
solve a CFL problem three different ways with one executable.

**Reoptimize after a data change.** Change an arc cost with
`mcf.chg_cost(NewCost, arc)` ([Chapter 8](08-modification-janus.md#ch-8)) and call `compute()`
again: the `MCFSolver` reoptimizes from its previous state rather
than starting over. This is the first taste of the reoptimization
theme that Recipes [R2](R2-knapsack-reopt.md#rec-R2) and [R4](R4-cfl-lagrangian.md#rec-R4) develop further.
```cpp
 mcf.chg_cost( 1.0 , 2 );    // arc 2 becomes cheaper still
 solver->compute();          // warm restart
```
