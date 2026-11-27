# 6. Solver {#ch-6}

[Source: [`SMS++/include/Solver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solver.h), [`CDASolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/CDASolver.h),
[`ThinComputeInterface.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h),
[`MILPSolver/include/MILPSolver.h`](https://gitlab.com/smspp/milpsolver/-/blob/develop/include/MILPSolver.h)]

We are concerned in this chapter with the `Solver`, i.e., the algorithm that
is attached to a `Block` and solves it (or, more generally, "computes" on it),
and with the uniform interface through which a program drives any `:Solver`,
whatever the problem and the method. We first describe what a `Solver` is and
how it interacts with its `Block` ([§6.1](06-solver.md#sec-6-1)), and the
distinction between specialized and general-purpose `:Solver`s
([§6.2](06-solver.md#sec-6-2)); then the return codes of `compute()`
([§6.3](06-solver.md#sec-6-3)), the parameters
([§6.4](06-solver.md#sec-6-4)), the asynchronous computation
([§6.5](06-solver.md#sec-6-5)) and the checks of feasibility and optimality
([§6.6](06-solver.md#sec-6-6)). An example on `MCFBlock`
([§6.7](06-solver.md#sec-6-7)), the outline of the interface
([§6.8](06-solver.md#sec-6-8)) and the idioms
([§6.9](06-solver.md#sec-6-9)) follow. The chapter ends with the back-ends
of `MILPSolver`, the general-purpose `:Solver` of the running examples, and
with the class of problems that each of them accepts
([§6.10](06-solver.md#sec-6-10)).

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

to have the `Solver` solve (again) its `Block`; the returned value is one of
the codes of the enum `sol_type`, which summarizes the outcome (cf.
[§6.3](06-solver.md#sec-6-3)). Note that the parameter `changedvars` is *not*
about whether the target has changed (which the pending `Modification`s say),
but about whether the *values* of the `Variable`s on which the computation
depends may have changed since the previous call. These are the `Variable`s of
the abstract representation of the target and of its sub-`Block`s, plus those
of the ancestors that appear in a `Constraint` of the target, which the target
treats as constants whose current value matters. With `changedvars == true`
(the default) the `Solver` has to assume that these values may have changed
and read them again. With `changedvars == false` the caller *guarantees* that
they have not changed since the last call, and the `Solver` can resume the
previous computation from where it stopped, if it has kept the state of its
search. This guarantee is the responsibility of the caller, which the `Solver`
is not required to check.

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
family, which reads an abstract representation that is a Mixed-Integer
Quadratically-Constrained Quadratic Program (MI-QCQP), i.e., one whose
`Objective` and rows have a linear or a (possibly non-convex) quadratic
`Function`. It builds the matrix form of the problem and hands it, through a
derived class, to one of the MIP solvers (CPLEX, Gurobi, SCIP, HiGHS, MOSEK)
or of the convex QP solvers, each of which accepts only a subset of the class
([§6.10](06-solver.md#sec-6-10)). A general-purpose `:Solver` is
typically (and sometimes considerably) slower than a specialized one on the
same problem, but it works on any `:Block` that can expose a suitable abstract
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
`Solver::sol_type` and `CDASolver` are those of
[Table 6.1](06-solver.md#tab-6-1).

<span id="tab-6-1"></span>**Table 6.1.** The return codes of `compute()`.

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
  maximum number of threads that `compute()` can spawn, 0 meaning
  that it only uses the thread that calls it) and `intEverykIt` (how
  often, in iterations, the events of type `eEverykIteration` are
  called, 0 meaning never), all three from `ThinComputeInterface`,
  plus `intMaxSol` (the maximum number of different solutions to keep
  and report on demand) and `intLogVerb` (the verbosity of the log, 0
  meaning silent) from `Solver`, and `intMaxDSol` (the same as
  `intMaxSol` for the dual solutions) from `CDASolver`;
- of type `double`: `dblMaxTime` (the maximum time, in seconds, of the
  next call, wall-clock or CPU as the `:Solver` decides, after which
  `kStopTime` is returned) and `dblEveryTTm` (how often, in time, the
  events of type `eEveryTTime` are called, 0 meaning never), from
  `ThinComputeInterface`, plus, from `Solver`, `dblRelAcc` and
  `dblAbsAcc` (the relative and absolute accuracies required to
  declare a solution optimal), `dblUpCutOff` and `dblLwCutOff` (the
  upper and lower cutoffs) and `dblRAccSol`, `dblAAccSol` and
  `dblFAccSol` (the relative error, the absolute error and the
  violation of the constraints allowed in any reported primal
  solution), to which `CDASolver` adds `dblRAccDSol`, `dblAAccDSol`
  and `dblFAccDSol` for the dual solutions;
- of type `std::string`: `strLogFileName` (the name of the file of
  the log), from `Solver`.

The defaults are `intMaxIter` $= \infty$, `intMaxThread` $= 0$,
`intEverykIt` $= 0$, `intMaxSol` $= 1$, `intLogVerb` $= 0$, `intMaxDSol`
$= 1$, `dblMaxTime` $= \infty$, `dblEveryTTm` $= 0$, `dblRelAcc`
$= 10^{-6}$, `dblAbsAcc` $= \infty$, `dblUpCutOff` $= +\infty$,
`dblLwCutOff` $= -\infty$, `dblRAccSol` $=$ `dblAAccSol` $= \infty$ and
`dblFAccSol` $= 0$, and the same for the dual counterparts. The base
classes only give these defaults, while whether a parameter is handled is
up to the concrete `:Solver`. Besides the events of the two periodic
types above, the events of type `eEverySolution` are called each time
the `:Solver` writes in the `Block` one of several successive solutions
out of which its result is made ([§9.7](09-solution.md#sec-9-7)).

A specific `:Solver` typically extends the enums with further parameters (for
instance, `DPBinaryKnapsackSolver` adds `dblReopt`, `LagrangianDualSolver` a
number of parameters that control the inner method, and `MILPSolver` those of
[§6.10](06-solver.md#sec-6-10)). All the parameters are set by
`set_par( idx , value )`, overloaded on the type of the value, and read by
`get_int_par( idx )`, `get_dbl_par( idx )`, `get_str_par( idx )`,
`get_vint_par( idx )` and so on (each also by the name of the parameter). They
can also be collected in a
[`ComputeConfig`](https://smspp.gitlab.io/smspp-project/da/daf/class_s_m_spp__di__unipi__it_1_1_compute_config.html)
object, which is loaded from a text file or from netCDF (cf. [Chapter
11](11-configuration.md#ch-11)) and applied with `set_ComputeConfig()`.

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
`is_feasible()` and `is_optimal()` (the latter, for a convex problem, may
amount to checking that the dual solution is feasible and satisfies the
complementary slackness with the primal one). Both take a boolean
`useabstract`: with `useabstract == false` (the default) the `Block` checks
the current solution against its *physical* representation, which is
typically much cheaper, while with `useabstract == true` it uses its
*abstract* representation if it has been built (cf. [Chapter
7](07-physical-abstract.md#ch-7)), the flag being a hint that the `Block`
may ignore if it must. The two answers should agree up to the numerical
tolerance, and they legitimately differ when some dynamic `Constraint`s or
`Variable`s have not been generated in the abstract representation. The
testers typically call `is_feasible()` on the solution that a `:Solver` has
written into the `Block`, as a final check (see, e.g., `UCBlock/test`). Why
both checks exist is discussed in [Chapter 7](07-physical-abstract.md#ch-7),
and how they are affected by the abstract `Variable`s, which the `:Block`s
of the running examples always construct, in
[§9.7](09-solution.md#sec-9-7).

## 6.7 Example: attaching an `MCFSolver` to an `MCFBlock` {#sec-6-7}

The example below continues the `MCFBlock` of
[§4.5](04-block.md#sec-4-5): given the instance loaded there in `mcf`, it
attaches to it a specialized `MCFSolver` whose template parameter is
`MCFSimplex`, the network simplex of the `MCFClass` library, solves it and
reads the result.

```cpp
// mcf is the MCFBlock of §4.5, already load()-ed
Solver * solver = Solver::new_Solver( "MCFSolver<MCFSimplex>" );
mcf.register_Solver( solver );

const int code = solver->compute( /* changedvars = */ true );
if( code == Solver::kOK ) {
 // the value from the Solver, the flows from the MCFSolution that it
 // fills out of its own data, without writing in the MCFBlock
 std::cout << "objective = " << solver->get_var_value() << '\n';
 std::unique_ptr< MCFSolution > sol(
   static_cast< MCFSolution * >( solver->get_Solution() ) );
 for( auto xa : sol->get_x() )
  std::cout << "  flow = " << xa << '\n';
 }
else  // kInfeasible, kUnbounded, a stop or an error (cf. §6.3)
 std::cout << "solver returned " << code << '\n';

mcf.unregister_Solver( solver , /* deleteold = */ true );
```

Four remarks are in order. First, `MCFSolver` is a template on the algorithm
of `MCFClass`: `MCFSimplex` is one choice, and `RelaxIV`, `CS2`, `SPTree`
(only for the instances with the structure of a shortest path tree),
`MCFZIB` and `MCFCplex` (each in a header of its own) are the others, the
choice of the template argument being made at compile time; the factory
registers each instance under its own name, e.g., `"MCFSolver<RelaxIV>"`,
provided that `MCFClass` has been built with that algorithm. Second,
`register_Solver()` does *not* take ownership of the `Solver *`, which is
only put in the list of the registered `Solver`s; ownership remains with the
caller, while the flag `deleteold == true` of `unregister_Solver()` (whose
default is `false`) has the `Block` delete the `Solver` once detached. Third,
the primal solution is not written into the `ColVariable`s of the `Block`:
`MCFSolver` is a "physical" `:Solver` (`is_get_Solution_physical()` returns
`true`), whose `get_Solution()` fills an `MCFSolution` out of the data of the
`MCFClass` object, the caller owning the result
(cf. [§9.7](09-solution.md#sec-9-7)). The methods `get_x( ... )` and
`get_objective_value()` of the `MCFBlock`, instead, read its `ColVariable`s,
and they become meaningful only after a call to `solver->get_var_solution()`,
which writes the flows there (the `ColVariable`s exist, since `load()`
constructs them, while the `Objective` requires `generate_objective()`).
Fourth, replacing `"MCFSolver<MCFSimplex>"` by the name of a general-purpose
`:MILPSolver` that accepts a Linear Program (say, `"HiGHSMILPSolver"`, cf.
[§6.10](06-solver.md#sec-6-10)) requires no change to the code around it.
However, `MILPSolver` *needs* the abstract representation, which it generates
(`generate_abstract_variables()`, `generate_abstract_constraints()`,
`generate_objective()`) when it is registered, and its `get_Solution()` goes
through the `ColVariable`s of the `Block` (it calls `get_var_solution()` and
then `MCFBlock::get_Solution()`); this is the trade-off that
[Chapter 7](07-physical-abstract.md#ch-7) makes precise. Also, the potentials
that `get_pi()` of the `MCFBlock` gives after `get_dual_solution()` are the
same whichever of the two `:Solver`s wrote them, according to the conventions
on the signs of the dual values of
[Chapter 9](09-solution.md#ch-9).

## 6.8 Outline of the interface {#sec-6-8}

The main public methods of `Solver`, grouped by purpose, are the
following:

- *attachment*: `set_Block( Block * )`, `get_Block()`, and
  `set_excluded_blocks( ... )` and `set_excluded_objectives( ... )`, which
  leave parts of the target out of the computation;
- *computation*: `compute( bool changedvars )`,
  `compute_async( bool changedvars )`;
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

The specification of each method, including the contract of each
parameter and the signature of each callback, is in [Doxy:
`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html)
and [Doxy:
`CDASolver`](https://smspp.gitlab.io/smspp-project/d7/d8b/class_s_m_spp__di__unipi__it_1_1_c_d_a_solver.html).

## 6.9 Idioms {#sec-6-9}

Once `compute()` has returned `kOK`, the convention of the framework
is that the primal solution is read through the `Block` (from its
`Variable`s, through its methods, or from the `Solution` objects
produced by its `get_Solution()`). The `Solver` is the agent that put
it there, rather than the place from which to read it back, and
therefore specialized and general-purpose `:Solver`s are
interchangeable for whoever reads the solution. An exception is a
"physical" `:Solver`, whose `Solver::get_Solution()` returns the
`:Solution` without writing into the `Block`
([§6.7](06-solver.md#sec-6-7) and [§9.7](09-solution.md#sec-9-7));
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

## 6.10 The back-ends of `MILPSolver` {#sec-6-10}

`MILPSolver` reads an abstract representation in which every `Variable` is a
`ColVariable`, the `Objective` is an `FRealObjective`, each `Constraint` is
either a `OneVarConstraint` or an `FRowConstraint`, and the `Function` of the
`Objective` and of each `FRowConstraint` is a `LinearFunction`, a
`DQuadFunction` (diagonal quadratic) or a `QuadFunction` (general quadratic,
not necessarily convex). It tells the three apart by `memo_cast()`
(since SMS++ 0.9.0, [§8.3](08-modification-janus.md#sec-8-3)), and out
of them it builds the problem

$$
\min \{\, f(x) \,:\, l \le A x \le u \,,\;
          \underline{x} \le x \le \overline{x} \,,\; x_j \in \mathbb{Z}
          \text{ for } j \in J \,\}
$$

where $f$ is linear or quadratic, the rows of $A$ may have a quadratic part
and $J$ is the set of the integer `ColVariable`s (the sense of the
`Objective` is passed to the engine, while the check of the optimality
conditions below treats a maximization as the minimization of $-f$). The
base class only builds and updates these data,
while each derived class passes them to an engine, processes the
`Modification`s that change them and reads back the solution; the derived
classes are in the factory under their class names, and which of them are
compiled is decided by the options `MILPSolver_USE_<X>` of CMake, ON by
default but effective only if the engine is found, with the minimum versions
of [Table 6.2](06-solver.md#tab-6-2). Yet, each engine accepts only a part of
the class above, as the table summarizes from the documentation of each
header; in particular, the convex QP solvers take no quadratic row, and they
take integer `ColVariable`s only if these are relaxed by `intRelaxIntVars`
(see below).

<span id="tab-6-2"></span>**Table 6.2.** The derived classes of
`MILPSolver`. "Relaxed" means that the integer `ColVariable`s are accepted
only with `intRelaxIntVars` nonzero; "rays" and "Farkas" say whether the
class can give, respectively, a direction of unboundedness of the primal
(`get_var_direction()`) and a certificate of its infeasibility
(`get_dual_direction()`).

| Class | Engine (minimum version) | Problem class | Integers | Rays | Farkas |
|---|---|---|---|---|---|
| `CPXMILPSolver` | CPLEX (12.8) | MI-QCQP, within the convexity requirements of CPLEX | yes | yes | yes |
| `GRBMILPSolver` | Gurobi (10.0) | MI-QCQP | yes | yes | yes |
| `SCIPMILPSolver` | SCIP (7.0) | MI-QCQP, also with non-convex quadratic rows | yes | yes | yes |
| `HiGHSMILPSolver` | HiGHS (1.8) | LP, MILP, convex QP; no quadratic row, no MIQP | yes | yes | yes |
| `MOSEKMILPSolver` | MOSEK (11.0.1) | LP, convex QP and QCQP, and their mixed-integer versions | yes | yes | yes |
| `PIPSMILPSolver` | PIPS-IPM++ | LP, convex QP with a Hessian separable by sub-`Block`; at least one sub-`Block` | relaxed | no | no |
| `DAQPMILPSolver` | DAQP (0.8.2) | convex QP, dense | relaxed | no | no |
| `QPOASESMILPSolver` | qpOASES (3.2) | convex QP, dense | relaxed | no | no |
| `ClarabelMILPSolver` | Clarabel.cpp (0.11.1) | convex QP, sparse | relaxed | yes | yes |
| `ProxQPMILPSolver` | ProxQP of proxsuite (0.4) | convex QP, dense or sparse | relaxed | yes | no |
| `PIQPMILPSolver` | PIQP (0.6) | convex QP, dense or sparse | relaxed | after PIQP 0.6.4 | after PIQP 0.6.4 |
| `OSQPMILPSolver` | OSQP (1.0) | convex QP, sparse | relaxed | yes | yes |
| `QPMADMILPSolver` | qpmad (1.5) | strictly convex QP, dense | relaxed | no | no |
| `QuadProgMILPSolver` | quadprog | strictly convex QP, dense | relaxed | no | no |
| `HPIPMMILPSolver` | HPIPM (0.1.2) | convex QP, dense | relaxed | no | no |
| `PERMONMILPSolver` | PermonQP on PETSc (3.22) | convex QP, large and sparse, Hessian positive definite on the null space of the equalities | relaxed | no | no |

All of them give dual solutions of a continuous problem. Note that "convex
QP" includes the Linear Programs, since the Hessian is only required to be
positive semidefinite, save for three classes: `QPMADMILPSolver` and
`QuadProgMILPSolver` need it positive definite, and on a Linear Program they
end with `kError` unless it is regularized, while the methods of
`PERMONMILPSolver` may not converge on it. Furthermore,
`PIPSMILPSolver` needs a `Block` with at least one sub-`Block`; hence, not
every `:MILPSolver` solves an arbitrary Linear Program. Each header lists the
other limitations of its engine (e.g., whether it has a time limit, a warm
start, a detection of infeasibility) and its specific parameters, while
`CPXMILPSolver`, `GRBMILPSolver`, `SCIPMILPSolver`, `HiGHSMILPSolver`,
`MOSEKMILPSolver` and `PIPSMILPSolver` also accept in the `ComputeConfig`
the parameters of their engine, under the names that the engine gives them.
Also, a continuous QP that the active-set method of HiGHS fails to solve is
solved again from scratch, then with larger regularizations of the Hessian,
and `HiGHSMILPSolver` throws `std::runtime_error` if none of these attempts
solves it.

The main parameters that `MILPSolver` adds to those of
[§6.4](06-solver.md#sec-6-4) decide how the integrality and the dynamic
`Constraint`s are treated. With `intRelaxIntVars` 0 (the default) the
problem is solved as it is; with 1 its continuous relaxation is solved (the
integer `ColVariable`s are passed to the engine as continuous ones) and no
`Constraint` is separated; with 2 the continuous relaxation is solved as
well, within a separation loop driven by the base class. In a mixed-integer
problem, a nonzero `intCutSepPar` has the back-ends that support it
(`CPXMILPSolver`, `GRBMILPSolver` and `SCIPMILPSolver`) call
`generate_dynamic_constraints()` of the `Block` from a callback of the
branch-and-bound, when the bits of its value say (as each header
specifies), and give the `Constraint`s so generated to the engine as user
cuts or lazy constraints. `HiGHSMILPSolver` uses its callback only to check
the cutoffs: it throws `std::runtime_error` when user cuts are asked for,
and the lazy constraints it separates are not passed to the engine, whose
public interface cannot take them from a callback. `MOSEKMILPSolver`, whose
engine has no such callback, throws `std::invalid_argument` on a
mixed-integer problem with `intCutSepPar` asking for separation (any of its
three lowest bits set).
The loop of `intRelaxIntVars` 2, instead, only needs the solution of
continuous problems, and therefore it works with any back-end; it is the
one used to compute the bound of a continuous relaxation strengthened by
separated `Constraint`s, as in [Recipe R3](R3-cfl-three-ways.md#rec-R3).

<span id="alg-6-1"></span>**Algorithm 6.1** (`compute()` of `MILPSolver`).

```text
 1  procedure COMPUTE()
 2     process the pending Modifications, changing the problem of the engine
 3     σ ← the status of the solution of the problem by the engine
 4     if the engine stopped at dblUpCutOff or dblLwCutOff then σ ← kCutOff
 5     if intRelaxIntVars = 2 then
 6        for k = 1, …, intMaxCutPasses − 1 while σ = kOK do
 7           write the solution x̄ in the ColVariables of the Block
 8           have the Block generate its dynamic Constraints violated by x̄
 9           if no Constraint has been generated then break
10           process the new Modifications (the new rows go to the engine)
11           σ ← the status of the solution of the new problem
12     if σ = kOK and the check of the optimality conditions is on then
13        if a residual of the solution exceeds its tolerance then σ ← kError
14     return σ
```

[Algorithm 6.1](06-solver.md#alg-6-1) is implemented by
`MILPSolver::compute()` in
[`MILPSolver.cpp`](https://gitlab.com/smspp/milpsolver/-/blob/develop/src/MILPSolver.cpp),
while the solution of the problem at lines 3 and 11 is the task of each
derived class. Note that the engine solves at most `intMaxCutPasses` problems
(1000 by default), and that the dynamic `Constraint`s of line 8 are those that
`generate_dynamic_constraints()` of the `Block` produces; what and how much to
separate is decided by the `Block`, typically according to the `Configuration`
of its dynamic `Constraint`s (cf. [Chapter 11](11-configuration.md#ch-11)).
Line 4 concerns a mixed-integer solve only, since `MILPSolver` does not check
the cutoffs on a continuous problem, nor with `intRelaxIntVars` 2.

The check of lines 12-13 is on when `dblKKTTol` is positive (it is 0 by
default, save for `PERMONMILPSolver`, where it is $10^{-6}$; besides, some
back-ends check by themselves the solutions that their engine may have got
wrong). The solution is then written in the `Block`, and three residuals are
measured on it. Let $\pi = -y$ be the multipliers of the rows, $y$ being the
dual values that their `get_dual()` gives, and $z = \nabla f(x) - A^\top
\pi$ those of the bounds. The primal residual is the largest violation of a
row, of a bound or (in a mixed-integer problem) of the integrality, each
divided by $\max\{ 1 , |\text{side}| \}$. The dual residual is the largest
part of a $\pi_i$ or of a $z_j$ whose sign points to an infinite side or
bound, divided by $\max\{ 1 , \| \nabla f(x) \|_\infty \}$. The gap is

$$
\Big( \sum_i | \pi_i ( A_i x - b_i ) | + \sum_j | z_j ( x_j - \beta_j ) |
\Big) \Big/ \max\{ 1 , | f(x) | \}
$$

where $b_i$ and $\beta_j$ are the side and the bound to which the sign of
the multiplier points; for a convex QP, it is the difference between the
primal and the dual value. The dual residual and the gap are only measured
on a continuous (or relaxed) problem with linear rows, an `Objective` whose
`Function` is a `C05Function`, and an engine that gives the dual solution.
If a residual exceeds the tolerance, `compute()` returns `kError`, while the
solution can still be read and `get_kkt_residuals()` gives the three values.

The dual values that the engines return are written in the `RowConstraint`s
with the sign changed, so that they follow the convention of
`RowConstraint`. Similarly, the multipliers of the columns of a certificate
of infeasibility are $-A^\top y$, rather than the reduced costs
$c - A^\top y$ of a dual point. Both conventions, and how they relate to
those of the specialized `:Solver`s of `MCFBlock`, are stated in
[Chapter 9](09-solution.md#ch-9). Note also that
`get_dual_direction_value()`, which a back-end provides when
`has_dual_direction_value()` is `true` (CPLEX, Gurobi, MOSEK, Clarabel, PIQP
and OSQP), gives the value of the certificate as the engine reports or
computes it; this
is in general different from the constant of the cut that the certificate
defines, which is the sum, over the rows and the bounds, of $-\pi_i b_i$
with $b_i$ the side to which the multiplier points.
