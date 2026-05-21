# 6. Solver {#ch-6}

[Source: `SMS++/include/Solver.h`, `CDASolver.h`,
`ThinComputeInterface.h`]

## 6.1 Concept {#sec-6-1}

A
[`Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html)
is the abstract base class for any algorithm capable of "computing"
on a `Block`: typically, producing one or more (approximately
feasible, approximately optimal) solutions, or proving that no
solution exists, or determining that the model is unbounded. The
class derives from
[`ThinComputeInterface`](https://smspp.gitlab.io/smspp-project/d9/d1b/class_s_m_spp__di__unipi__it_1_1_thin_compute_interface.html),
which is the SMS++ abstraction of "anything that has a `compute()`
method whose call is expected to be costly".

A `Solver` is *attached* to one `Block`, its target. The `Solver`'s
responsibility is to solve the *entire* `Block`, which includes any
sub-`Block` of the target, recursively. What the `Solver` is
unaware of, by contrast, is the part of the `Block` tree that lies
*above* the target: any father `Block`, any uncles and grand-uncles,
any ancestor's `Variable`s and `Constraint`s. (If the target is a
root `Block` no part of the tree lies above it, and the `Solver`
sees the whole model.)

The principal interactions between `Block` and `Solver` are four:

- **registration / deregistration**: a `Block` calls
  `Solver::set_Block(p_Block)` (typically via
  `Block::register_Solver(Solver*)`) to attach the `Solver` to
  itself; deregistration restores `set_Block(nullptr)`. Any
  number of `:Solver`s may be attached to the same `Block`.
- **modification notification**: any change in the target `Block`
  or in any of its sub-`Block`s, recursively, that occurred after
  the `Solver` was attached is delivered as a `Modification` and
  enqueued in the `Solver`'s pending list ([Chapter 8](08-modification-janus.md#ch-8)). The
  `Solver` is responsible for consuming the list when it is next
  invoked.
- **computation**: the user calls
  `Solver::compute(bool changedvars)` to ask the `Solver` to
  (re-)solve. The return value is an `sol_type` enum that
  summarises the outcome ([§6.3](06-solver.md#sec-6-3)). The `changedvars` parameter is
  *not* about whether the target `Block` has changed (this is
  signalled via `Modification`s in the pending list, see above);
  it is about whether the *values* of any `Variable` upon which
  the computation depends may have been altered between the
  previous `compute()` and this one. The relevant `Variable`s are
  those in the abstract representation of the target `Block` (and
  of its sub-`Block`s, recursively), plus any `Variable` belonging
  to an ancestor `Block` that appears in a `Constraint` of the
  target (and that the target therefore treats as a constant
  whose current value matters; see [Chapter 8](08-modification-janus.md#ch-8)). With
  `changedvars = true` (the default) the `Solver` must assume
  that these values may have changed and re-read them as needed;
  with `changedvars = false` the caller is *guaranteeing* that
  they have not changed since the last call, which allows the
  `Solver` to resume the previous computation from where it had
  stopped, if the state of the search has been preserved. The
  guarantee is the caller's responsibility; the `Solver` is not
  required to verify it.
- **solution retrieval**: after `compute()` returns, the user
  calls `new_var_solution(...)` / `get_var_solution(...)` to
  iterate through the primal solutions the `Solver` has produced.
  Similarly, a
  [`CDASolver`](https://smspp.gitlab.io/smspp-project/d7/d8b/class_s_m_spp__di__unipi__it_1_1_c_d_a_solver.html)
  ("Convex Duality Aware") exposes
  `new_dual_solution(...)` / `get_dual_solution(...)` for one or
  more dual solutions.

A few `:Solver` are *exact*, in the sense that, given unbounded
computational resources and no error condition, they will always
eventually return one among `kOK`, `kUnbounded`, `kInfeasible`. An
exact `:Solver` is not required to do so under any *bounded*
budget: when forced to stop early, an exact `:Solver` may well
return `kStopTime`, `kStopIter` or `kLowPrecision`, exactly like a
heuristic one would. A genuinely *heuristic* `:Solver` differs in
that it cannot in principle guarantee optimality even given
unbounded resources, and may therefore return `kLowPrecision` as
its terminal status even when no resource limit has been hit.

## 6.2 Specialised versus general-purpose {#sec-6-2}

`Solver`s come in two flavours.

A **specialised** `:Solver` is written for a specific `:Block`
class. It reads the *physical* representation directly: the
problem data in the form the `:Block` stores it (a graph for
`MCFBlock`, a weights / profits / capacity triple for
`BinaryKnapsackBlock`, etc.). A specialised `:Solver` is typically
fast, because it knows the structure and can exploit it; the cost
of that speed is that it works only for that one `:Block`. The
running examples include
[`MCFSolver<MCFC>`](https://smspp.gitlab.io/smspp-project/d2/dd2/class_s_m_spp__di__unipi__it_1_1_m_c_f_solver.html)
(a wrapper around an `MCFClass` algorithm) for `MCFBlock`, and
[`DPBinaryKnapsackSolver`](https://smspp.gitlab.io/smspp-project/d5/d6a/class_s_m_spp__di__unipi__it_1_1_d_p_binary_knapsack_solver.html)
(SMS++-native dynamic programming) for `BinaryKnapsackBlock`.

A **general-purpose** `:Solver` knows nothing about the specific
problem class. It reads the *abstract* representation — the
`Variable`s, `Constraint`s, `Objective` introduced in [Chapter 5](05-variable-constraint-objective.md#ch-5) —
and applies a general algorithm to it. The running example is the
[`MILPSolver`](https://smspp.gitlab.io/smspp-project/d7/d97/class_s_m_spp__di__unipi__it_1_1_m_i_l_p_solver.html)
family, which constructs a matrix-form MILP from the abstract
representation and hands it to one of the major MIP back-ends
(CPLEX, Gurobi, HiGHS, SCIP) through a derived class. A
general-purpose `:Solver` is typically slower than a specialised
one on the same problem, but it works on any `:Block` that can
expose a suitable abstract representation.

Switching between the two is typically a one-line change in the
`BlockSolverConfig` ([Chapter 11](11-configuration.md#ch-11)). User code that depends only on
the `Solver` interface (calling `compute()` and reading the
solutions) does not need to change.

## 6.3 The `sol_type` return enum {#sec-6-3}

`Solver::compute()` returns a value of type
`Solver::sol_type`, an enum that extends
`ThinComputeInterface::compute_type` with several values specific
to optimisation. The enum is divided into four *ranges* by the
sentinel values `kUnEval`, `kOK`, `kError`; the range, not just
the specific value, carries semantic meaning. The convention is:

- values $\le$ `kUnEval` mean *the computation has not finished
  yet*. `kStillRunning` is one such value: `compute()` was
  re-entered (typically from an event handler) before the previous
  invocation had returned. `kUnEval` itself means `compute()` has
  not been called at all. These values are not normally returned
  *by* `compute()` (which by definition has finished when it
  returns); they are used internally and observed via event
  handlers.
- values strictly between `kUnEval` and `kOK` (inclusive) mean
  *the computation succeeded*. `kOK` ("optimal solution to within
  the required accuracy") is the prototypical success; other
  values in this range encode "different kinds of success", such
  as `kUnbounded` (the model is provably unbounded) and
  `kInfeasible` (the model is provably infeasible). The `Solver`
  has, in all of these cases, a *certificate* of its claim; the
  form of the certificate is problem-class-specific and not
  exposed by a uniform interface.
- values strictly between `kOK` and `kError` mean *the computation
  was stopped early in a recoverable state*. The principal codes
  are `kStopTime` (time limit hit), `kStopIter` (iteration limit
  hit), and `kLowPrecision` (a feasible solution was found, but
  optimality to within the required accuracy could not be
  certified). Calling `compute()` again with a fresh time or
  iteration budget allows the search to continue from where it
  stopped; this range is therefore not terminal.
- values $\ge$ `kError` mean *the computation hit an
  unrecoverable error and is unlikely to make progress on a
  subsequent call*. The error need not be "non-numerical": typical
  cases include numerical failures (a linear system that cannot
  be factorised, an iterate that diverges, ...) as well as
  structural failures such as `kBlockLocked` (the `Block` could
  not be acquired, see [Chapter 17](17-parallel.md#ch-17)). The convention is that
  `kError` is the *smallest* value in this range and that each
  concrete `:Solver` may extend the range past it with
  `:Solver`-specific codes that encode the exact kind of error
  (e.g. "ran out of memory", "callback raised an exception",
  ...). The user code should therefore treat *any* return value
  $\ge$ `kError` as an error, and consult the `:Solver`-specific
  documentation only when it wants to distinguish between
  different kinds.

The principal values defined by `Solver::sol_type` on top of
`ThinComputeInterface::compute_type` are:

| Value | Range | Meaning |
|---|---|---|
| `kStillRunning` | $<$ `kUnEval` | `compute()` re-entered before the previous call returned (typically from an event handler). |
| `kUnEval` | sentinel | `compute()` has not been called yet. |
| `kUnbounded` | $\le$ `kOK` | The model is provably unbounded; the `Solver` can produce feasible solutions of arbitrarily large (or small) objective on demand. |
| `kInfeasible` | $\le$ `kOK` | The model is provably infeasible. |
| `kBothInfeasible` | $\le$ `kOK` | Both the primal and the dual are infeasible. |
| `kOK` | sentinel | An optimal solution to within the required accuracy was found. |
| `kStopTime` | $<$ `kError` | Stopped because `dblMaxTime` was reached; calling `compute()` again resets the timer. |
| `kStopIter` | $<$ `kError` | Stopped because `intMaxIter` was reached; calling `compute()` again resets the iteration count. |
| `kLowPrecision` | $<$ `kError` | A feasible solution was found but optimality could not be certified; an exact `:Solver` returns this only when forced to stop early, a heuristic `:Solver` may return it as its terminal status. |
| `kBlockLocked` | $\ge$ `kError` | Could not acquire the lock on the `Block`. |
| `kError` | sentinel | Generic unrecoverable error; a concrete `:Solver` typically extends the range with more specific codes. |

## 6.4 Parameters {#sec-6-4}

A `Solver` exposes a uniform parameter interface inherited from
`ThinComputeInterface` and extended for the optimisation case.
Parameters are indexed by an integer enum and have one of three
value types: `int`, `double`, `std::string`. The principal
predefined slots are:

- *integer parameters* (`int_par_type_S`):
    - `intMaxIter` — maximum number of "iterations" the `Solver`
      may execute before returning `kStopIter`; the concept of
      "iteration" is `:Solver`-specific.
    - `intMaxSol` — maximum number of distinct solutions to keep
      and report on demand.
    - `intLogVerb` — verbosity level of the log (0 = silent).
- *double parameters* (`dbl_par_type_S`):
    - `dblMaxTime` — maximum wall-clock time before returning
      `kStopTime`.
    - `dblRelAcc`, `dblAbsAcc` — relative and absolute accuracy
      for declaring a solution optimal.
    - `dblUpCutOff`, `dblLwCutOff` — upper and lower cutoffs for
      stopping the algorithm early.
    - `dblRAccSol`, `dblAAccSol`, `dblFAccSol` — accuracies and
      constraint violations tolerated in any reported primal
      solution. The `CDASolver` adds the corresponding
      `*DSol` variants for dual solutions.

A specific `:Solver` typically extends both enums with its own
parameters (for instance, `DPBinaryKnapsackSolver` adds
`dblReopt`, and `LagrangianDualSolver` adds a number of parameters
controlling the inner method). All parameters are read and
written through `get_par(idx)` and
`set_par(idx, value)`; in addition, the same parameters can be
bundled into a
[`ComputeConfig`](https://smspp.gitlab.io/smspp-project/da/daf/class_s_m_spp__di__unipi__it_1_1_compute_config.html)
object that loads from text files or netCDF ([Chapter 11](11-configuration.md#ch-11)).

## 6.5 Asynchronous computation {#sec-6-5}

`Solver` exposes `compute_async()` (inherited from
`ThinComputeInterface`) which launches the `compute()` in a
separate thread and returns a `std::future< int >` from which the
caller can later retrieve the `sol_type`. The expectation is that
the `:Solver`'s `compute()` is thread-safe with respect to the
`Block` it is attached to; the framework provides recursive locking
on the `Block` to make this manageable ([Chapter 17](17-parallel.md#ch-17)).

## 6.6 Feasibility and optimality checks {#sec-6-6}

Three closely-related methods on the `Block` are used to verify
the *result* of a `Solver`'s computation: `is_feasible()`,
`is_dual_feasible()`, `is_optimal()`. All three take a boolean
`useabstract`:

- `useabstract = false` (the default) asks the `Block` to check
  the current solution against the **physical** representation;
  this is typically much cheaper.
- `useabstract = true` asks the `Block` to check against the
  **abstract** representation; this only works if the abstract
  representation has been built ([Chapter 7](07-physical-abstract.md#ch-7)).

The two answers should agree up to numerical tolerance. They may
genuinely disagree only in edge cases (for instance when dynamic
`Constraint`s have not yet been generated by the abstract path).
A specialised `:Solver` typically calls
`Block::is_optimal(false)` after its own `compute()` returns
`kOK`, as a final sanity check on the physical representation.

The full discussion of why both paths exist — and of the
particular asymmetry caused by the current "always materialised
`Variable`s" implementation (already flagged in [§3.3](03-mental-model.md#sec-3-3) as
**Status — under development**) — is the topic of [Chapter 7](07-physical-abstract.md#ch-7).

## 6.7 Inline example: attaching an `MCFSolver` to an `MCFBlock` {#sec-6-7}

The example below continues the `MCFBlock` of [§4.5](04-block.md#sec-4-5): it constructs
the same small instance and attaches a specialised `MCFSolver`
templated on `MCFSimplex` (the classical primal simplex
implementation shipped in the `MCFClass` external library).

```cpp
#include "MCFBlock.h"
#include "MCFSolver.h"
#include "MCFSimplex.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 // construct the MCFBlock and load the instance (as in §4.5)
 MCFBlock mcf;
 MCFBlock::Subset      pSn = { 0, 1, 0 };
 MCFBlock::Subset      pEn = { 1, 2, 2 };
 MCFBlock::Vec_FNumber pU  = { 3.0, 3.0, 3.0 };
 MCFBlock::Vec_CNumber pC  = { 2.0, 3.0, 4.0 };
 MCFBlock::Vec_FNumber pB  = { -2.0, 0.0, +2.0 };
 mcf.load( 3 , 3 , pEn , pSn , pU , pC , pB );

 // construct the specialised Solver and attach it to the MCFBlock
 auto * solver = new MCFSolver< MCFSimplex >();
 mcf.register_Solver( solver );

 // ask the Solver to compute
 const int code = solver->compute( /* changedvars = */ true );

 // interpret the return code
 if( code == Solver::kOK ) {
  // optimal solution found; primal flows have been written into the
  // ColVariable of the MCFBlock by the Solver. Read them back via the
  // MCFBlock's own physical accessors.
  std::cout << "objective = " << mcf.get_objective_value() << '\n';
  MCFBlock::Vec_FNumber x( mcf.get_NArcs() );
  mcf.get_x( x.begin() );
  for( MCFBlock::Index a = 0 ; a < mcf.get_NArcs() ; ++a )
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
 return 0;
}
```

Four things to notice.

1. The `MCFSolver` is templated on the underlying `MCFClass`
   algorithm. `MCFSimplex` is one option; `RelaxIV`,
   `MCFCplex`, `SPTree` and others are also available, each in
   its own header. The choice fixes the algorithm at compile
   time.
2. `register_Solver` *does not* take ownership of the `Solver*`;
   the call only enrols it in the `Block`'s registered-solvers
   list. Ownership is the caller's. The convenience flag
   `deleteold = true` on `unregister_Solver` asks the framework
   to `delete` the `Solver` once it has been detached.
3. The primal solution is written by the `Solver` into the
   `ColVariable`s of the `Block`. Because `MCFBlock` is here
   solved by a specialised `Solver` that has not built the
   abstract representation, this materialisation is logically
   unnecessary — but it is what currently happens (cf. [§3.3](03-mental-model.md#sec-3-3),
   "always materialised"). The recommended way to read the
   solution from a specialised `:Solver` is via the `Block`'s
   physical accessors, here `get_x(...)` and
   `get_objective_value()`, not via iterating over the
   `ColVariable`s directly.
4. Swapping `MCFSolver< MCFSimplex >` for a generic
   `:MILPSolver` (say, `CPXMILPSolver`) requires no change to
   the surrounding code: the user calls
   `mcf.register_Solver(milp)`, `milp->compute()`, then reads
   the solution as before. What does change, however, is that
   `MILPSolver` *needs* the abstract representation, which means
   `mcf.generate_abstract_variables()`,
   `mcf.generate_abstract_constraints()`,
   `mcf.generate_objective()` must have been called first. This
   is exactly the trade-off [Chapter 7](07-physical-abstract.md#ch-7) makes precise.

## 6.8 API outline {#sec-6-8}

The principal public methods of `Solver`, grouped by purpose:

- *attachment*: `set_Block(p_Block)`, `get_Block()`;
- *computation*: `compute(bool changedvars)`,
  `compute_async()`;
- *parameters*: `set_par(idx, int|double|string)`,
  `get_par(idx)`, plus `set_ComputeConfig(ComputeConfig*)` for
  bulk configuration;
- *primal solutions*: `new_var_solution(...)`,
  `get_var_solution(...)`, `sol_count()`;
- *dual solutions* (on `CDASolver` only):
  `new_dual_solution(...)`, `get_dual_solution(...)`,
  `dsol_count()`;
- *bounds and value*: `get_lb()`, `get_ub()`, `get_value()`;
- *events*: `register_event(...)` for user-callbacks invoked
  periodically by `compute()`;
- *logging*: `set_log(std::ostream*)`,
  `set_par(intLogVerb, level)`.

The full method-by-method specification, including the precise
contract of each parameter and each callback signature, is in
[Doxy: `Solver`](https://smspp.gitlab.io/smspp-project/df/d44/class_s_m_spp__di__unipi__it_1_1_solver.html)
and
[Doxy: `CDASolver`](https://smspp.gitlab.io/smspp-project/d7/d8b/class_s_m_spp__di__unipi__it_1_1_c_d_a_solver.html).

## 6.9 Idioms {#sec-6-9}

**Switching `:Solver`s is a configuration change.** User code
that depends only on the `Solver` interface — `register_Solver`,
`compute`, read the solution from the `Block` — does not need to
know whether the `Solver` is specialised or general-purpose. The
choice is typically expressed in a `BlockSolverConfig` text file,
and changing it is one line. The framework's idiom is *not* to
write a different program for each choice of `Solver`.

**Reading solutions through the `Block`, not the `Solver`.** Once
`compute()` returns `kOK`, the framework convention is that the
primal solution lives in the `Block` (in its `Variable`s, in its
physical accessors, and in its `Solution` objects produced by
`get_Solution()`). The `Solver` is the *agent* that put the
solution there; it is *not* the canonical place from which to
read it back. This makes specialised and general-purpose
`:Solver`s interchangeable from the reader's point of view.

**`Solver::new_Solver` for runtime selection.** When the choice
of `:Solver` is itself a runtime parameter (read from a
configuration file), the canonical idiom is

```cpp
Solver * s = Solver::new_Solver( "MCFSolver<MCFSimplex>" );
mcf.register_Solver( s );
```

`new_Solver` consults the `Solver` factory, which any concrete
`:Solver` registers itself in by means of a one-line macro at
namespace scope in its `.cpp` file ([Chapter 18](18-factories-netcdf.md#ch-18)).
