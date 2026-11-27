# Recipe R1: solving a Min-Cost Flow instance {#rec-R1}

[Source: [`MCFClassSolver/test/test.cpp`](https://gitlab.com/smspp/mcfclasssolver/-/blob/develop/test/test.cpp),
[`MCFBlock/include/MCFBlock.h`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h),
[`MCFClassSolver/include/MCFSolver.h`](https://gitlab.com/smspp/mcfclasssolver/-/blob/develop/include/MCFSolver.h)]

We are concerned in this recipe with the smallest complete use of SMS++: a
small directed graph with arc costs, arc capacities and node deficits is
loaded in an `MCFBlock`, which is solved by the specialized `MCFSolver`, and
the optimal flows, the potentials and the objective value are read back; then
the specialized `:Solver` is replaced by a general-purpose `:MILPSolver`
without changing the logic around it. The tester of `MCFClassSolver` solves
instances read from file through rounds of changes of the data, both with an
`MCFSolver` and directly with the underlying `MCFClass` object; that of
`tests/MCFBlock` compares an `MCFSolver` with a `:MILPSolver`. After the
concepts that the recipe uses, we give the code, comment it line by line, show
the expected output and discuss a few variations.

## Concepts used

- the construction of a `Block` and `load()`
  ([Chapter 4](04-block.md#ch-4));
- registering a `Solver`, `compute()` and its return value
  ([Chapter 6](06-solver.md#ch-6));
- the physical and the abstract representation, i.e., where the
  solution is written and which representation each `:Solver` reads
  ([Chapter 7](07-physical-abstract.md#ch-7));
- reading the solution through the physical accessors of the `Block`
  ([Chapter 9](09-solution.md#ch-9));
- (variation) selecting the `Solver` through a `BlockSolverConfig`
  ([Chapter 11](11-configuration.md#ch-11)).

## The code

```cpp
// a 3-node, 3-arc instance:
//
//     (1) --arc0,c=2--> (2) --arc1,c=3--> (3)
//      \                                  ^
//       `------------ arc2,c=4 -----------'
//
// node deficits b = (-2, 0, +2): node 1 produces 2 units, node 3
// consumes 2; all arcs have capacity 3

MCFBlock mcf;  // a root MCFBlock

// the arcs give the node "names", 1 to n; the arrays indexed by node
// (deficits, potentials) hold node k in position k - 1
MCFBlock::Subset pSn = { 1 , 2 , 1 };              // arc starting nodes
MCFBlock::Subset pEn = { 2 , 3 , 3 };              // arc ending nodes
MCFBlock::Vec_FNumber pU = { 3.0 , 3.0 , 3.0 };    // arc capacities
MCFBlock::Vec_CNumber pC = { 2.0 , 3.0 , 4.0 };    // arc costs
MCFBlock::Vec_FNumber pB = { -2.0 , 0.0 , +2.0 };  // node deficits
mcf.load( 3 , 3 , pEn , pSn , pU , pC , pB );      // NOTE: pEn before pSn

mcf.generate_abstract_constraints();  // whose duals hold the potentials
mcf.generate_objective();             // what get_objective_value() computes

auto * solver = dynamic_cast< CDASolver * >(
                 Solver::new_Solver( "MCFSolver<MCFSimplex>" ) );
mcf.register_Solver( solver );

if( const int status = solver->compute() ; status == Solver::kOK ) {
 solver->get_var_solution();   // flows into mcf
 solver->get_dual_solution();  // potentials into mcf
 MCFBlock::Vec_FNumber x( mcf.get_NArcs() );
 MCFBlock::Vec_CNumber pi( mcf.get_NNodes() );
 mcf.get_x( x.begin() );
 mcf.get_pi( pi.begin() );
 std::cout << "objective = " << mcf.get_objective_value() << '\n';
 for( MCFBlock::Index a = 0 ; a < mcf.get_NArcs() ; ++a )
  std::cout << "  flow on arc " << a << " = " << x[ a ] << '\n';
 for( MCFBlock::Index n = 0 ; n < mcf.get_NNodes() ; ++n )
  std::cout << "  potential of node " << n + 1 << " = " << pi[ n ] << '\n';
 }
else  // kInfeasible, kUnbounded, a stop or an error
 std::cout << "solver returned status " << status << '\n';

mcf.unregister_Solver( solver , /* deleteold = */ true );
```

## Walk-through

- The five arrays are the *physical* representation of the min-cost flow
  instance ([Chapter 7](07-physical-abstract.md#ch-7)): the start and end node
  of each arc, the arc capacities, the arc costs and the node deficits (a
  negative deficit is a supply, a positive one a demand). `load( ... )` copies
  them into the `MCFBlock` ([Chapter 4](04-block.md#ch-4)); recall its
  idiosyncratic parameter order, ending nodes before starting nodes, and that
  the nodes of the arcs are named from 1 to $n$ (`load( ... )` throws
  `std::invalid_argument` otherwise), while the deficits are indexed from 0.
- `Solver::new_Solver( "MCFSolver<MCFSimplex>" )` constructs, through the
  factory of [§6.9](06-solver.md#sec-6-9), the specialized `:Solver`
  templated on the network simplex of `MCFClass`; it is a `CDASolver`, which
  has the `get_dual_solution()` used below. `register_Solver()` attaches it
  to the `MCFBlock` ([Chapter 6](06-solver.md#ch-6)), and does *not* transfer
  the ownership of the pointer: the flag `deleteold = true` of
  `unregister_Solver()` has the `MCFBlock` delete it at the end.
- `solver->compute()` runs the algorithm and returns an `int` status
  ([§6.3](06-solver.md#sec-6-3)), `kOK` meaning that an optimal solution
  was found.
- The solution is read *through the `MCFBlock`*, not through the solver (the
  convention of [§6.9](06-solver.md#sec-6-9) and [Chapter
  9](09-solution.md#ch-9)): `get_var_solution()` and `get_dual_solution()`
  write it in the `MCFBlock`, then `get_objective_value()`, `get_x( ... )` for
  the primal flows, and `get_pi( ... )` for the dual node potentials.
  `get_rc( ... )` would give the arc reduced costs. An `MCFSolution`
  stores the flows and the potentials ([§9.5](09-solution.md#sec-9-5)).
- The specialized `MCFSolver` reads the physical representation
  directly and needs no abstract `Variable`s or `Constraint`s to
  solve. Yet the `MCFBlock` keeps the flows in its `ColVariable`s,
  which `load()` constructs (the "half-baked" caveat of
  [§7.6](07-physical-abstract.md#sec-7-6)), and the potentials in the
  duals of its flow conservation `Constraint`s, which exist only once
  generated. Hence, without `generate_abstract_constraints()` the call
  `get_dual_solution()` silently writes nothing and `get_pi()` throws
  `std::logic_error`, and without `generate_objective()`
  `get_objective_value()` returns $+\infty$, while the flows are
  written and read in any case. Note that the optimal value is also
  given, with no abstract representation, by `solver->get_ub()`.

## Expected output

On this instance the cheapest way to move 2 units from node 1 to node 3
is the direct arc 2 (unit cost 4, total 8), cheaper than the path
1, 2, 3 (unit cost 2 + 3 = 5, total 10); since all capacities are 3,
both routes can carry the 2 units, and the optimum is 8 with all the
flow on arc 2.

```
objective = 8
  flow on arc 0 = 0
  flow on arc 1 = 0
  flow on arc 2 = 2
  potential of node 1 = ...
  potential of node 2 = ...
  potential of node 3 = ...
```

The flows and the objective are determined, since the optimal flow is
unique (any unit moved to the path costs 1 more). The potentials, instead,
depend on the dual solution that the algorithm reports, which is not unique.
With the signs that `get_pi()` gives (those of the network flow literature,
cf. [Chapter 9](09-solution.md#ch-9)), the reduced cost of an arc $(i, j)$ is
$C_{ij} + \pi_i - \pi_j$. The complementary slackness conditions require it
to be zero on arc 2, whose flow is strictly between 0 and the capacity, and
nonnegative on arcs 0 and 1, whose flow is 0; hence, any potentials with
$\pi_3 = \pi_1 + 4$ and $\pi_1 + 1 \le \pi_2 \le \pi_1 + 2$ are optimal,
and the algorithm may report any of them (which is why they are elided
here).

## Variations

*A general-purpose `:MILPSolver`.* A min-cost flow is a Linear Program, and
therefore it can also be solved by any `:MILPSolver` that accepts one, i.e.,
by all those of [Table 6.2](06-solver.md#tab-6-2) save `QPMADMILPSolver` and
`QuadProgMILPSolver`, which require a positive definite Hessian,
`PERMONMILPSolver`, whose methods may not converge on a Linear Program
(cf. [§6.10](06-solver.md#sec-6-10)), and `PIPSMILPSolver`, which requires
sub-`Block`s that `MCFBlock` does not have. Only the name given to the
factory changes, say `"HiGHSMILPSolver"` in the place of
`"MCFSolver<MCFSimplex>"`: the `:MILPSolver` reads the *abstract*
representation, which it generates when it is registered (the calls of
`generate_abstract_constraints()` and `generate_objective()` in the code are
needed by `get_pi()` and `get_objective_value()`, not by the `:MILPSolver`).
It writes the solution in the `MCFBlock` through `get_var_solution()` and
`get_dual_solution()` as before. The optimal value is the same, and so are
the potentials that `get_pi()` gives for the same dual solution, since the
dual values that a `:MILPSolver` writes in the `Constraint`s and those that
`MCFSolver` writes through `set_pi()` follow the same convention (cf.
[Chapter 9](09-solution.md#ch-9)). The `:Solver` works on any `:Block` that can
expose an abstract representation of the class it accepts, which is the swap
promised in [§6.2](06-solver.md#sec-6-2).

*The algorithms of abLEMON.* The module `MCFLemonSolver` provides four more
specialized `:Solver`s of `MCFBlock`, built on the algorithms of abLEMON
([§2.1](02-installation.md#sec-2-1)):
`MCFLemonSolverNetworkSimplex`, `MCFLemonSolverCycleCanceling`,
`MCFLemonSolverCostScaling` and `MCFLemonSolverCapacityScaling`. Each of
them is registered in the factory under names that also give the graph and
the numeric types (e.g.,
`"MCFLemonSolverNetworkSimplex<SmartDigraph,double,double>"`), and can
usually restart from the state of its previous run after a change of the
data, when its parameter `kReopt` is nonzero (the default).

*The `Solver` from a configuration file.* The idiomatic SMS++ way, rather
than hard-coding the choice of solver in the program, is to read a
`BlockSolverConfig` from a text file ([Chapter 11](11-configuration.md#ch-11))
and `apply()` it to the `MCFBlock`, so that the same executable solves with
`MCFSolver`, `CPXMILPSolver`, or any other registered `:Solver` purely by
editing the configuration file, with no recompilation. The installed tool
`smspp_mcfblock_solver` (`tools/mcfblock_solver/`) is such an executable: it
reads an `MCFBlock` from a DIMACS or netCDF file, and the `:Solver`s from a
`BlockSolverConfig` file (`-S`,
[`BSCfg.txt`](https://gitlab.com/smspp/tools/-/blob/develop/mcfblock_solver/config/BSCfg.txt)
by default). [Recipe R3](R3-cfl-three-ways.md#rec-R3) uses the same pattern to
solve a CFL problem in three different ways with one executable.

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
