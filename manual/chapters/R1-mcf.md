# Recipe R1: solving a Min-Cost Flow instance {#rec-R1}

> *Counterpart in the source tree:* `MCFClassSolver/test/test.cpp`.
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

- the construction of a `Block` and `load()`
  ([Chapter 4](04-block.md#ch-4));
- registering a `Solver`, `compute()` and its return value
  ([Chapter 6](06-solver.md#ch-6));
- the physical and the abstract representation, i.e., where the
  solution is written and why the `MILPSolver` variant needs the
  `generate_*()` calls ([Chapter 7](07-physical-abstract.md#ch-7));
- reading the solution through the physical accessors of the `Block`
  ([Chapter 9](09-solution.md#ch-9));
- (variation) selecting the `Solver` through a `BlockSolverConfig`
  ([Chapter 11](11-configuration.md#ch-11)).

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

 // the arcs give the node "names", 1 to n, i.e., node k is k + 1
 MCFBlock::Subset      pSn = { 1 , 2 , 1 };     // arc starting nodes
 MCFBlock::Subset      pEn = { 2 , 3 , 3 };     // arc ending nodes
 MCFBlock::Vec_FNumber pU  = { 3.0 , 3.0 , 3.0 };  // arc capacities
 MCFBlock::Vec_CNumber pC  = { 2.0 , 3.0 , 4.0 };  // arc costs
 MCFBlock::Vec_FNumber pB  = { -2.0 , 0.0 , +2.0 };  // node deficits

 // NOTE: load() takes ending nodes (pEn) before starting nodes (pSn)
 mcf.load( 3 , 3 , pEn , pSn , pU , pC , pB );

  // load() has constructed the flow Variable; the Constraint hold the
 // potentials, and the Objective is what get_objective_value() computes
 mcf.generate_abstract_variables();   // nothing to do, already there
 mcf.generate_abstract_constraints();
 mcf.generate_objective();

 // ---- attach a specialised Solver and solve ---------------------
 auto * solver = new MCFSolver< MCFSimplex >();
 mcf.register_Solver( solver );

 const int status = solver->compute();

 // ---- read the solution -----------------------------------------
 if( status == Solver::kOK ) {
  solver->get_var_solution();                   // flows into mcf
  solver->get_dual_solution();                  // potentials into mcf

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

- The five arrays are the *physical* representation of the min-cost
  flow instance ([Chapter 7](07-physical-abstract.md#ch-7)): the start
  and end node of each arc, the arc capacities, the arc costs and the
  node deficits (a negative deficit is a supply, a positive one a
  demand). `load(...)` copies
  them into the `MCFBlock` ([Chapter 4](04-block.md#ch-4)); recall its idiosyncratic
  parameter order, ending nodes before starting nodes, and that the
  nodes of the arcs are named from 1 to $n$ (`load(...)` throws
  `std::invalid_argument` otherwise), while the deficits are indexed
  from 0.
- `new MCFSolver< MCFSimplex >()` constructs the specialised
  solver, templated on the network simplex of MCFClass;
  `register_Solver()` enrols it on the `MCFBlock`
  ([Chapter 6](06-solver.md#ch-6)). Registration does *not* transfer ownership of the
  pointer; the `deleteold = true` flag on `unregister_Solver` is
  what asks the framework to `delete` it at the end.
- `solver->compute()` runs the algorithm and returns an
    `int` status ([Chapter 6](06-solver.md#ch-6),
  [§6.3](06-solver.md#sec-6-3)), and `kOK` means that an optimal
  solution was found.
- The solution is read *through the `MCFBlock`*, not through the
  solver (the convention of [§6.9](06-solver.md#sec-6-9) and [Chapter 9](09-solution.md#ch-9)): `get_var_solution()`
  and `get_dual_solution()` write it in the `MCFBlock`, then
  `get_objective_value()`, `get_x(...)` for the primal flows, and
  `get_pi(...)` for the dual node potentials. `get_rc(...)` would
  give the arc reduced costs; together the three are the primal and
  dual data of a min-cost flow, which an `MCFSolution` stores
  ([§9.5](09-solution.md#sec-9-5)).
- The specialised `MCFSolver` reads the physical representation
  directly and needs no abstract `Variable`s or `Constraint`s to
  solve. Yet the `MCFBlock` keeps the flows in its `ColVariable`s,
  which `load()` constructs (the "half-baked" caveat of
  [§7.6](07-physical-abstract.md#sec-7-6)), and the potentials in the
  duals of its flow conservation `Constraint`s, which exist only once
  generated. Hence, without `generate_abstract_constraints()` the call
  `get_dual_solution()` silently writes nothing and `get_pi()` throws
  `std::logic_error`, and without `generate_objective()`
  `get_objective_value()` returns $+\infty$, while the flows are
  written and read in any case. The optimal value alone
  is also given, with no abstract representation, by
  `solver->get_ub()`.

## Expected output

On this instance the cheapest way to move 2 units from node 0 to node 2
is the direct arc 2 (unit cost 4, total 8), cheaper than the path
0, 1, 2 (unit cost 2 + 3 = 5, total 10); since all capacities are 3,
both routes can carry the 2 units, and the optimum is 8 with all the
flow on arc 2.

```
objective = 8
  flow on arc 0 = 0
  flow on arc 1 = 0
  flow on arc 2 = 2
  potential of node 0 = ...
  potential of node 1 = ...
  potential of node 2 = ...
```

The flows and the objective are determined, since the optimal flow is
unique (any unit moved to the path costs 1 more). The potentials depend
on the dual solution that the algorithm reports, which is not unique:
any potentials that make the reduced cost of arc 2 zero and those of
arcs 0 and 1 nonnegative satisfy the complementary slackness conditions
(arc 2 being the only one with positive flow, and none being
saturated), and therefore they are elided here.

## Variations

*A general-purpose `:MILPSolver`.* A min-cost flow is a linear
program, and therefore it can also be solved by any `:MILPSolver`
(e.g., `CPXMILPSolver`, `SCIPMILPSolver`, `HiGHSMILPSolver`). The
surrounding logic does not change (construct the `Solver`,
`register_Solver()`, `compute()`, read the solution through the
`MCFBlock`), but a general-purpose `Solver` reads the *abstract*
representation, which the three `generate_*()` calls above have
already built and which here is not optional:

```cpp
 auto * milp = new SCIPMILPSolver();    // or CPX/HiGHS/GRB
 mcf.register_Solver( milp );
 milp->compute();
 milp->get_var_solution();
 // ... read mcf.get_objective_value(), mcf.get_x(...) exactly as before
```

The optimal value is the same (8); the `Solver` is slower on this
instance but works on any `:Block` that can expose an
abstract representation. This is the swap promised in [§6.2](06-solver.md#sec-6-2) and
[§6.9](06-solver.md#sec-6-9).

*The `Solver` from a configuration file.* Rather than
hard-coding the choice of solver in the program, the idiomatic
SMS++ way is to read a `BlockSolverConfig` from a text file
([Chapter 11](11-configuration.md#ch-11)) and `apply()` it to the `MCFBlock`. The same
executable then solves with `MCFSolver`, `CPXMILPSolver`, or any
other registered `:Solver` purely by editing the configuration
file, with no recompilation. The installed tool `smspp_mcfblock_solver`
(`tools/mcfblock_solver/`) is such an executable: it reads an
`MCFBlock` from a DIMACS or netCDF file, and the `:Solver`s from a
`BlockSolverConfig` file (`-S`, `BSCfg.txt` by default). [Recipe R3](R3-cfl-three-ways.md#rec-R3) uses exactly this pattern to
solve a CFL problem three different ways with one executable.

*Reoptimization after a data change.* Change an arc cost with
`mcf.chg_cost( NCost , arc )` ([Chapter 8](08-modification-janus.md#ch-8))
and call `compute()` again: the `MCFSolver` reoptimizes from its
previous state rather than starting over (with the cost of arc 2 at 1
the optimum becomes 2, the flow staying on arc 2). Recipes
[R2](R2-knapsack-reopt.md#rec-R2) and [R4](R4-cfl-lagrangian.md#rec-R4)
develop the reoptimization theme further.


```cpp
 mcf.chg_cost( 1.0 , 2 );    // arc 2 becomes cheaper still
 solver->compute();          // warm restart
```
