# 6. Solver {#ch-6}

[Source: `SMS++/include/Solver.h`, `CDASolver.h`,
`ThinComputeInterface.h`]

## 6.1 Concept {#sec-6-1}

A
[`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html)
is the abstract base class of any algorithm that can "compute" on a
`Block`: typically, produce one or more (approximately feasible,
approximately optimal) solutions, or prove that no solution exists, or
that the model is unbounded. This class derives from
[`ThinComputeInterface`](https://smspp.gitlab.io/smspp-project/d9/d1b/class_s_m_spp__di__unipi__it_1_1_thin_compute_interface.html),
the abstraction of SMS++ for "anything that has a `compute()` method
whose call is expected to be costly".

A `Solver` is *attached* to one `Block`, its target, and it is
responsible for solving the target together with its sub-`Block`s
(recursively). The part of the tree of `Block`s that lies *above* the
target (the father, the other descendants of the ancestors, and the
`Variable`s and `Constraint`s of the ancestors) is unknown to it, save
for the `Variable`s of the ancestors discussed below; if the target is
a root, the `Solver` sees the complete model. Interactions between a
`Block` and a `Solver` are of four kinds.

Registration. The `Block` calls `Solver::set_Block( Block * )`
(typically inside `Block::register_Solver( Solver * )`) to attach the
`Solver`, and unregistering it calls `set_Block( nullptr )`; any
number of `:Solver`s can be attached to the same `Block`.

Notification of the changes. Every change of the target, or of any of
its sub-`Block`s, that happens after the `Solver` has been attached
reaches it as a `Modification`, which the `Solver` puts in its list of
pending `Modification`s (cf. [Chapter 8](08-modification-janus.md#ch-8))
and processes the next time it is invoked.

Computation. The user calls

```cpp
int compute( bool changedvars = true ) override = 0;
```

to have the `Solver` solve (again) its `Block`; the returned value is
one of the codes of the enum `sol_type`, which summarizes the outcome
(cf. [§6.3](06-solver.md#sec-6-3)). Note that the parameter
`changedvars` is *not* about whether the target has changed (which the
pending `Modification`s say), but about whether the *values* of the
`Variable`s on which the computation depends may have changed since
the previous call. These are the `Variable`s of the abstract
representation of the target and of its sub-`Block`s, plus those of
the ancestors that appear in a `Constraint` of the target, which the
target treats as constants whose current value matters. With
`changedvars == true` (the default) the `Solver` has to assume that
these values may have changed and read them again. With `changedvars
== false` the caller *guarantees* that they have not changed since the
last call, and the `Solver` can resume the previous computation from
where it stopped, if it has kept the state of its search. This
guarantee is the responsibility of the caller, which the `Solver` is
not required to check.

Retrieval of the solutions. After `compute()` returns, the user calls
`new_var_solution()` and `get_var_solution()` to go through the primal
solutions found by the `Solver`, which `get_var_solution()` writes
into the `Variable`s of the `Block`, or `get_Solution()` to have the
current one as a `Solution` object ([§3.3](03-mental-model.md#sec-3-3)
and [Chapter 9](09-solution.md#ch-9)). Similarly, a
[`CDASolver`](https://smspp.gitlab.io/smspp-project/d7/d8b/class_s_m_spp__di__unipi__it_1_1_c_d_a_solver.html)
("Convex Duality Aware") provides `new_dual_solution()` and
`get_dual_solution()` for one or more dual solutions.

Some `:Solver`s are *exact*, in the sense that, given unbounded
computational resources and no error, they always eventually return
one among `kOK`, `kUnbounded` and `kInfeasible` (although "eventually"
may well be after the thermal death of the universe). An exact
`:Solver` is not required to do so within a *bounded* budget: when
forced to stop early it may return `kStopTime`, `kStopIter` or
`kCutOff`, as a heuristic one would. A *heuristic* `:Solver` differs
in that it cannot guarantee optimality even with unbounded resources,
and it may therefore return `kLowPrecision` when no limit on the
resources has been reached.

## 6.2 Specialized and general-purpose Solvers {#sec-6-2}

A *specialized* `:Solver` is written for a specific `:Block` class, and it
reads the *physical* representation directly, i.e., the data of the problem in
the form in which the `:Block` stores them (a graph for `MCFBlock`, the
weights, profits and capacity for `BinaryKnapsackBlock`). It is typically
fast, since it knows the structure and can exploit it, and the price of the
speed is that it works only for that `:Block`. The running examples comprise
[`MCFSolver< MCFC >`](https://smspp.gitlab.io/smspp-project/d2/dd2/class_s_m_spp__di__unipi__it_1_1_m_c_f_solver.html)
(a wrapper of an algorithm of `MCFClass`) for `MCFBlock` and
[`DPBinaryKnapsackSolver`](https://smspp.gitlab.io/smspp-project/d5/d6a/class_s_m_spp__di__unipi__it_1_1_d_p_binary_knapsack_solver.html)
(a dynamic program native to SMS++) for `BinaryKnapsackBlock`.

A *general-purpose* `:Solver` does not know the specific class of
problems: it reads the *abstract* representation, i.e., the
`Variable`s, `Constraint`s and `Objective` of [Chapter
5](05-variable-constraint-objective.md#ch-5), and applies a general
algorithm to it. Our running example is the
[`MILPSolver`](https://smspp.gitlab.io/smspp-project/d7/d97/class_s_m_spp__di__unipi__it_1_1_m_i_l_p_solver.html)
family, which builds the matrix form of a MILP out of the abstract
representation and hands it, through a derived class, to one of the
MIP solvers (CPLEX, Gurobi, MOSEK, HiGHS, SCIP) or of the convex QP
solvers. A general-purpose `:Solver` is typically (and sometimes
considerably) slower than a specialized one on the same problem, but
it works on any `:Block` that can expose a suitable abstract
representation.

Switching between the two is typically a change of one line in the
`BlockSolverConfig` (cf. [Chapter 11](11-configuration.md#ch-11)), and
code that only depends on the interface of `Solver` (that calls
`compute()` and reads the solutions) does not change.

## 6.3 The return codes {#sec-6-3}

`Solver::compute()` returns a value of the enum `Solver::sol_type`,
which extends `ThinComputeInterface::compute_type` with values
specific to optimization. The three values `kUnEval` (3), `kOK` (10)
and `kError` (18) of `compute_type` divide the codes into four
ranges, and it is the range, not only the specific value, that
carries the meaning:

- a code $\le$ `kUnEval` means that the computation has not finished.
  Here `kUnEval` means that `compute()` has not been called (or that
  what it computed has been lost since, say because the `Block` has
  changed), and `kStillRunning` (2) that `compute()` is still running,
  say when it is called again from an event handler before the
  previous call has returned. These values are not normally returned
  *by* `compute()`, which by definition has finished when it returns.
  They are the values that the state of a running `compute()` can
  take, as seen by the event handlers;
- a code in (`kUnEval`, `kOK`], i.e., `kUnEval` excluded and `kOK`
  included, means that the computation succeeded. `kOK` (an optimal
  solution within the required accuracy) is the prototypical success,
  and the other values of the range are other kinds of success, such
  as `kUnbounded` (the model is provably unbounded) and `kInfeasible`
  (the model is provably infeasible). In all of these cases the
  `Solver` has a *certificate* of its claim, whose form depends on
  the class of problems and is not exposed by a uniform interface;
- a code in (`kOK`, `kError`), both excluded, means that the
  computation stopped early in a recoverable state. The main codes are
  `kStopTime` (the time limit has been reached), `kStopIter` (the
  iteration limit has been reached) and `kCutOff` (one of the cutoffs
  `dblUpCutOff` and `dblLwCutOff` has been reached, as requested,
  which is not a certificate of optimality). After `kStopTime` or
  `kStopIter`, calling `compute()` again with a fresh budget lets the
  search proceed from where it stopped;
- a code $\ge$ `kError` means that the computation hit an error for
  which no recourse is readily available, and that calling `compute()`
  again is not expected to solve it. Such an error may be numerical (a
  linear system that cannot be factorized, iterates that diverge) or
  structural, such as `kBlockLocked` (the `Block` could not be locked,
  see [Chapter 17](17-parallel.md#ch-17)). This range also holds
  `kLowPrecision`, by which an intrinsically heuristic `:Solver`
  reports that it found at least one feasible solution, but could not
  reach the required accuracy because of its nature (not of a limit on
  the resources). `kError` is the smallest value of the range, and
  each concrete `:Solver` may extend the range from `kLastSolverError`
  onwards with specific codes, which describe what went wrong (say,
  that the memory was exhausted, or that a callback raised an
  exception). Code that uses a `Solver` should therefore treat *any*
  value $\ge$ `kError` as an error, and consult the documentation of
  the specific `:Solver` only when it wants to tell the different
  errors apart.

The values defined by `ThinComputeInterface::compute_type`,
`Solver::sol_type` and `CDASolver` are the following:

| Value | Range | Meaning |
|---|---|---|
| `kStillRunning` | $<$ `kUnEval` | `compute()` is still running (seen by the event handlers). |
| `kUnEval` | sentinel | `compute()` has not been called yet. |
| `kUnbounded` | $\le$ `kOK` | The model is provably unbounded: the `Solver` can produce feasible solutions of arbitrarily large (or small) value. |
| `kInfeasible` | $\le$ `kOK` | The model is provably infeasible. |
| `kBothInfeasible` | $\le$ `kOK` | Both the primal and the dual are infeasible (defined by `CDASolver`). |
| `kOK` | sentinel | An optimal solution within the required accuracy has been found. |
| `kStopTime` | $<$ `kError` | Stopped because `dblMaxTime` was reached; calling `compute()` again resets the timer. |
| `kStopIter` | $<$ `kError` | Stopped because `intMaxIter` was reached; calling `compute()` again resets the count of the iterations. |
| `kCutOff` | $<$ `kError` | Stopped because a cutoff was reached; `get_lb()` and `get_ub()` tell which one, and by how much. |
| `kError` | sentinel | Generic unrecoverable error; a concrete `:Solver` typically extends the range with more specific codes. |
| `kBlockLocked` | $>$ `kError` | The lock of the `Block` could not be acquired. |
| `kLowPrecision` | $>$ `kError` | A feasible solution was found, but its optimality could not be certified, because the `:Solver` is intrinsically heuristic (not because of a limit on the resources). |

## 6.4 Parameters {#sec-6-4}

A `Solver` has a uniform interface for its parameters, inherited from
`ThinComputeInterface` and extended for optimization. Each parameter
is identified by the value of an enum, and it is of one of six types:
`int`, `double`, `std::string`, and the vectors of each of them. The
main predefined parameters are the following:

- of type `int`: `intMaxIter` (the maximum number of iterations of
  the next call to `compute()`, after which `kStopIter` is returned;
  what an iteration is depends on the `:Solver`), `intMaxThread` (the
  maximum number of threads that `compute()` can spawn) and
  `intEverykIt` (how often the events of type `eEverykIteration` are
  called), all three from `ThinComputeInterface`, plus `intMaxSol`
  (the maximum number of different solutions to keep and report on
  demand) and `intLogVerb` (the verbosity of the log, 0 meaning
  silent) from `Solver`;
- of type `double`: `dblMaxTime` (the maximum wall-clock time of the
  next call, after which `kStopTime` is returned) and `dblEveryTTm`
  (how often the events of type `eEveryTTime` are called), from
  `ThinComputeInterface`, plus, from `Solver`, `dblRelAcc` and
  `dblAbsAcc` (the relative and absolute accuracies required to
  declare a solution optimal), `dblUpCutOff` and `dblLwCutOff` (the
  upper and lower cutoffs) and `dblRAccSol`, `dblAAccSol` and
  `dblFAccSol` (the relative error, the absolute error and the
  violation of the constraints allowed in any reported primal
  solution), to which `CDASolver` adds `intMaxDSol` and the
  corresponding `dbl*DSol` parameters for the dual solutions;
- of type `std::string`: `strLogFileName` (the name of the file of
  the log), from `Solver`.

A specific `:Solver` typically extends the enums with further
parameters (for instance, `DPBinaryKnapsackSolver` adds `dblReopt`,
and `LagrangianDualSolver` a number of parameters that control the
inner method). All the parameters are set by `set_par( idx , value )`,
overloaded on the type of the value, and read by `get_int_par( idx )`,
`get_dbl_par( idx )`, `get_str_par( idx )`, `get_vint_par( idx )` and
so on (each also by the name of the parameter). They can also be
collected in a
[`ComputeConfig`](https://smspp.gitlab.io/smspp-project/da/daf/class_s_m_spp__di__unipi__it_1_1_compute_config.html)
object, which is loaded from a text file or from netCDF (cf. [Chapter
11](11-configuration.md#ch-11)) and applied with
`set_ComputeConfig()`.

## 6.5 Asynchronous computation {#sec-6-5}

The method `compute_async( bool changedvars = true )`, inherited from
`ThinComputeInterface`, runs `compute()` as a separate task and
returns a `std::future< int >`, from which the caller later retrieves
the return code. This presumes that `compute()` can run concurrently
with whatever else is done on the `Block`, and the framework provides
recursive locks on the `Block` to make this manageable
(cf. [Chapter 17](17-parallel.md#ch-17)).

## 6.6 Checks of feasibility and optimality {#sec-6-6}

Two methods of the `Block` check the *result* of a computation:
`is_feasible()` and `is_optimal()` (the latter, for a convex problem,
also checks the feasibility of the dual solution and the complementary
slackness). Both take a boolean `useabstract`: with `useabstract ==
false` (the default) the `Block` checks the current solution against
its *physical* representation, which is typically much cheaper, while
with `useabstract == true` it checks it against its *abstract*
representation, which therefore has to have been built (cf. [Chapter
7](07-physical-abstract.md#ch-7)). The two answers should agree up to
the numerical tolerance, and they may genuinely differ only in
borderline cases (for instance, when some dynamic `Constraint`s have
not been generated in the abstract representation). The testers
typically call `is_feasible()` on the solution that a `:Solver` has
written into the `Block`, as a final check (see, e.g.,
`UCBlock/test`). Why both checks exist, and how the `Variable`s that
are always constructed (Status in [§3.3](03-mental-model.md#sec-3-3))
affect them, is discussed (rather at length) in [Chapter
7](07-physical-abstract.md#ch-7).

## 6.7 Example: attaching an `MCFSolver` to an `MCFBlock` {#sec-6-7}

The example below continues the `MCFBlock` of
[§4.5](04-block.md#sec-4-5): it builds the same instance and attaches
to it a specialized `MCFSolver` whose template parameter is
`MCFSimplex`, the network simplex of the `MCFClass` library.

```cpp
#include "MCFBlock.h"
#include "MCFSolver.h"
#include "MCFSimplex.h"

#include <iostream>
#include <memory>

using namespace SMSpp_di_unipi_it;

int main()
{
 // construct the MCFBlock and load the instance (as in §4.5)
 MCFBlock mcf;
 MCFBlock::Subset pSn = { 1 , 2 , 1 };
 MCFBlock::Subset pEn = { 2 , 3 , 3 };
 MCFBlock::Vec_FNumber pU = { 3.0 , 3.0 , 3.0 };
 MCFBlock::Vec_CNumber pC = { 2.0 , 3.0 , 4.0 };
 MCFBlock::Vec_FNumber pB = { -2.0 , 0.0 , +2.0 };
 mcf.load( 3 , 3 , pEn , pSn , pU , pC , pB );

 // construct the specialised Solver and attach it to the MCFBlock
 auto * solver = new MCFSolver< MCFSimplex >();
 mcf.register_Solver( solver );

 // ask the Solver to compute
 const int code = solver->compute( /* changedvars = */ true );

 // interpret the return code
 if( code == Solver::kOK ) {
  // optimal solution found: the value is asked to the Solver, the
  // flows to the MCFSolution that it fills out of its own data
  // structures, without writing into the ColVariable of the MCFBlock
  std::cout << "objective = " << solver->get_var_value() << '\n';
  std::unique_ptr< MCFSolution > sol(
    static_cast< MCFSolution * >( solver->get_Solution() ) );
  const auto & x = sol->get_x();
  for( std::size_t a = 0 ; a < x.size() ; ++a )
   std::cout << "  flow on arc " << a << " = " << x[ a ] << '\n';
  }
 else if( code == Solver::kInfeasible ) {
  std::cout << "infeasible.\n";
  }
 else if( code == Solver::kUnbounded ) {
  std::cout << "unbounded.\n";
  }
 else {
  std::cout << "solver returned " << code << ".\n";
  }

 // detach and clean up
 mcf.unregister_Solver( solver , /* deleteold = */ true );
 return( 0 );
 }
```

Four remarks are in order. First, `MCFSolver` is a template on the algorithm
of `MCFClass`: `MCFSimplex` is one choice, and `RelaxIV`, `CS2`, `SPTree`,
`MCFZIB` and `MCFCplex` (each in a header of its own) are the others, the
choice being made at compile time (the factory registers each instance under
its own name, e.g., `"MCFSolver<RelaxIV>"`). Second, `register_Solver()` does
*not* take ownership of the `Solver *`, which is only put in the list of the
registered `Solver`s; ownership remains with the caller, while the flag
`deleteold == true` of `unregister_Solver()` (whose default is `false`) has
the `Block` delete the `Solver` once detached. Third, the primal solution is
not written into the `ColVariable`s of the `Block`: `MCFSolver` is a
"physical" `:Solver` (`is_get_Solution_physical()` returns `true`), whose
`get_Solution()` fills an `MCFSolution` out of the data of the `MCFClass`
object, the caller owning the result (cf. [§3.3](03-mental-model.md#sec-3-3)).
The methods `get_x( ... )` and `get_objective_value()` of the `MCFBlock`,
instead, read its `ColVariable`s, and they become meaningful only after a call
to `solver->get_var_solution()`, which writes the flows there (the
`ColVariable`s exist, since `load()` constructs them, while the `Objective`
requires `generate_objective()`). Fourth, replacing `MCFSolver< MCFSimplex >`
by a general-purpose `:MILPSolver` (say, `CPXMILPSolver`) requires no change
to the code around it: the user calls `mcf.register_Solver( milp )` and
`milp->compute()`, and reads the solution as before. However, `MILPSolver`
*needs* the abstract representation, which it has generated
(`generate_abstract_variables()`, `generate_abstract_constraints()`,
`generate_objective()`) when it is registered, and that its `get_Solution()`
goes through the `ColVariable`s of the `Block` (it calls `get_var_solution()`
and then `MCFBlock::get_Solution()`); this is the trade-off that [Chapter
7](07-physical-abstract.md#ch-7) makes precise.

## 6.8 Outline of the interface {#sec-6-8}

The main public methods of `Solver`, grouped by purpose, are the
following:

- *attachment*: `set_Block( Block * )`, `get_Block()`, and
  `set_excluded_blocks( ... )` and `set_excluded_objectives( ... )`, which
  leave parts of the target out of the computation;
- *computation*: `compute( bool changedvars )`, `compute_async( bool
  changedvars )`;
- *parameters*: `set_par( idx , value )` (for `int`, `double`,
  `std::string` and their vectors), `get_int_par( idx )`,
  `get_dbl_par( idx )`, `get_str_par( idx )` and so on, plus
  `set_ComputeConfig( const ComputeConfig * )` for the bulk
  configuration;
- *primal solutions*: `has_var_solution()`, `new_var_solution()`,
  `get_var_solution( Configuration * )`, `is_var_feasible()`,
  `get_var_value()`, `get_Solution( Configuration * )`,
  `is_get_Solution_physical()`, and `has_var_direction()` and
  `get_var_direction( ... )` for the directions of unboundedness;
- *dual solutions* (of a `CDASolver` only): `has_dual_solution()`,
  `new_dual_solution()`, `get_dual_solution( Configuration * )`,
  `is_dual_feasible()`, and the corresponding methods for the dual
  directions;
- *bounds*: `get_lb()`, `get_ub()`;
- *events*: `set_event_handler( type , handler )` and
  `reset_event_handler( type , id )`, for the callbacks of the user
  that `compute()` invokes at the given kind of event;
- *`Modification`s*: `concerned_by()`, the kinds of `Modification`
  that the `:Solver` reads ([§4.6](04-block.md#sec-4-6) and
  [Appendix B](B-writing-solver.md#sec-B-4)), and
  `inhibit_Modification()`;
- *log*: `set_log( std::ostream * )` and `set_par( intLogVerb , level )`.

For the specification of each method, including the contract of each
parameter and the signature of each callback, is in [Doxy:
`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html)
and [Doxy:
`CDASolver`](https://smspp.gitlab.io/smspp-project/d7/d8b/class_s_m_spp__di__unipi__it_1_1_c_d_a_solver.html).

## 6.9 Idioms {#sec-6-9}

Note that code that only depends on the interface of `Solver` (it registers the
`Solver`, calls `compute()` and reads the solution) need not know
whether the `Solver` is specialized or general-purpose. The choice is
typically written in a `BlockSolverConfig` file, where changing it is
a matter of one line; the idiom of the framework is not to write a
different program for each choice of `Solver`.

Once `compute()` has returned `kOK`, the convention of the framework
is that the primal solution is read through the `Block` (from its
`Variable`s, through its methods, or from the `Solution` objects
produced by its `get_Solution()`). The `Solver` is the agent that put
it there, rather than the place from which to read it back, and
therefore specialized and general-purpose `:Solver`s are
interchangeable for whoever reads the solution. An exception is a
"physical" `:Solver`, whose `Solver::get_Solution()` returns the
`:Solution` without writing into the `Block`
([§3.3](03-mental-model.md#sec-3-3) and [§6.7](06-solver.md#sec-6-7));
the object is still the `:Solution` defined by the `:Block`, and the
interchangeability is preserved for whoever reads it through
`get_Solution()`.

When the choice of the `:Solver` is a parameter read at run time (say,
from a configuration file), the intended idiom is the factory of
`Solver`s:

```cpp
Solver * s = Solver::new_Solver( "MCFSolver<MCFSimplex>" );
mcf.register_Solver( s );
```

Here `new_Solver()` normalizes the name it is given (spaces in the
template arguments do not matter) and looks it up in the factory. Each
concrete `:Solver` registers in the factory by a macro at namespace
scope in its `.cpp` file (cf. [Chapter
18](18-factories-netcdf.md#ch-18)).
