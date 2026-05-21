# 17. Parallel and asynchronous computation {#ch-17}

[Source: `SMS++/include/Block.h` (locking),
`ThinComputeInterface.h` (`compute_async`, `State`),
`Solver.h`; `BundleSolver/include/ParallelBundleSolver.h`]

SMS++ is designed for *coarse-grained* parallelism: the unit of
concurrent work is the solution of a `Block` (or sub-`Block`), not
a fine-grained data-parallel loop. This chapter collects the
mechanisms that make such parallelism safe and efficient — locking,
asynchronous evaluation, and solver state — and is candid about
what is not yet handled.

## 17.1 Locking a `Block` {#sec-17-1}

Because several threads may want to read or modify the same
`Block` tree, SMS++ provides a locking protocol on `Block`:

- `lock(owner)` acquires an *exclusive* (write) lock on behalf of
  `owner`, a `const void *` that identifies the locking entity
  (often, but not necessarily, derived from the thread). The lock
  is **recursive over the tree**: locking a `Block` automatically
  locks all of its sub-`Block`s, recursively (`lock_sub_block`),
  because an entity that holds a `Block` must hold its whole
  sub-tree to operate on it safely ([Chapter 12](12-sub-block.md#ch-12)).
- `unlock(owner)` releases it (and the sub-tree).
- `read_lock()` / `read_unlock()` acquire and release a *shared*
  (read) lock; any number of concurrent readers are allowed, as
  long as no writer holds the `Block`.

The lock records an **owner** rather than relying on a plain
`std::mutex`, for a specific reason: the same entity may need to
re-enter a `Block` it already holds (a `Solver` recursing into a
sub-`Block`, say), and a plain mutex would deadlock on
re-entry. Recording the owner lets the protocol distinguish
"someone else holds this" from "I already hold this".

The one rule the user must respect is **direction**: lock attempts
must always proceed *top-down* the tree, because locks naturally
travel downward and a bottom-up attempt can deadlock against a
top-down one. The corollary is convenient: *to lock any set of
`Block`s safely, lock their nearest common ancestor* — one of the
contenders is then guaranteed to succeed (`Block.h:1700-1714`).

> **Status — under development.** Write starvation is not yet
> handled: a steady stream of readers can, in principle, keep a
> writer waiting indefinitely. Applications that mix frequent reads
> with occasional writes should be aware of this.

This same recursive locking underpins the immediacy guarantee of
[§8.4](08-modification-janus.md#sec-8-4): an abstract change is applied while the `Block` (hence its
whole sub-tree) is locked, so no concurrent mutation can interleave
between a change and the `Block`'s reaction to it.

### Lending an ID

The recursive lock raises a problem the moment a `:Solver`
*delegates*. Suppose a `:Solver` has `lock()`-ed a `Block` to
perform an atomic computation, and in the course of it wants a
*sub-`Solver`* to work on a part of the tree — a sub-`Block`, say.
The sub-`Solver` needs to `lock()` that sub-`Block`; but the
sub-`Block` is *already* locked, as part of the sub-tree the outer
`:Solver` holds. The outer `:Solver` cannot simply release it
either: doing so would risk another entity grabbing it before the
outer `:Solver`'s atomic computation is finished.

The framework resolves this by letting the outer `:Solver` **lend
its owner ID** to the inner `:Solver`(s). A lock attempt made with
the lent ID is recognised as coming from the entity that already
holds the lock — so it succeeds (re-entrantly) instead of blocking
— rather than as contention from a different owner. The outer
`:Solver` thus delegates work on a part of the tree it holds,
without releasing the lock and without deadlocking against itself.

The mechanism only solves the *locking* side of delegation. It is
the lending `:Solver`'s responsibility to ensure, by other means,
that the inner `:Solver`(s) it has lent the ID to actually behave
— that they confine themselves to the part of the tree they were
delegated, do not corrupt the outer computation's invariants, and
release what they should. The lent ID grants access; it does not
grant good behaviour.

## 17.2 Asynchronous computation {#sec-17-2}

Any `ThinComputeInterface` — so any `Solver`, but also any
`Function` such as a `LagBFunction` or `BendersBFunction` — can be
evaluated asynchronously. `compute_async(changedvars)` is a thin
wrapper that launches `compute()` in a separate task and returns a
`std::future< int >` on which the caller can later wait for the
`sol_type` result (`ThinComputeInterface.h:1271`). This is the
primitive on top of which coarse-grained parallel solution is
built: a driver can fire off the `compute_async()` of several
independent sub-problems and collect their results as the futures
resolve.

For this to be safe, a `:Solver`'s `compute()` must be
*thread-safe* with respect to the `Block` it solves; the recursive
locking of [§17.1](17-parallel.md#sec-17-1) — including the lent-ID mechanism above — is what
makes that achievable when a `:Solver` delegates to inner
`:Solver`s on sub-`Block`s.

## 17.3 Solver `State` and checkpointing {#sec-17-3}

A `ThinComputeInterface` may expose a
[`State`](https://smspp.gitlab.io/smspp-project/d6/dff/class_s_m_spp__di__unipi__it_1_1_state.html)
object — a snapshot of its internal algorithmic state — through
`get_State()` / `put_State()` (`ThinComputeInterface.h:1980-2018`).
A `State` can be serialised to netCDF, stored, and later restored
into a (compatible) `:Solver`, which serves three purposes:

- **checkpointing**: a long-running solve can save its `State`
  periodically and resume from it after an interruption;
- **reoptimization across invocations / processes**: a `State`
  captured from one `:Solver` can warm-start another, which is
  especially valuable in the distributed and SDDP settings where
  the same kind of sub-problem is solved at many nodes;
- **restoring the *same* `:Solver` to an earlier condition.**
  A `State` is useful even within a single `:Solver`, when that
  `:Solver` is invoked under different conditions in an order that
  is not topological. The prototypical case is a branch-and-X
  search with a non-topological visit order: the driver solves a
  node with a `:Solver`, then jumps to an unrelated part of the
  tree, then comes back to explore one of that node's children. To
  resume the children efficiently, one wants the `:Solver` put
  back into the condition it was in *right after the node's
  children were generated* — for an LP relaxation `:Solver`, for
  instance, holding the optimal basis at that node, so the child
  re-solves by a few dual-simplex pivots rather than from scratch.
  Capturing the node's `State` when it is first solved and
  re-installing it when a child is later visited is exactly what
  makes this possible.

A `:Solver` with no meaningful internal state simply returns
`nullptr` from `get_State()`; the mechanism then costs nothing.

## 17.4 Inline example: a parallel Lagrangian dual {#sec-17-4}

The Knapsack Formulation of CFL (Chapters [12](12-sub-block.md#ch-12), [14](14-lag-benders-bfunction.md#ch-14)) decomposes into
one independent Lagrangian knapsack per facility; evaluating the
Lagrangian function at a given multiplier vector means solving all
of those knapsacks, which are independent and therefore
parallelisable. The
[`ParallelBundleSolver`](https://smspp.gitlab.io/smspp-project/df/d0d/_parallel_bundle_solver_8h.html)
— the parallel variant of `BundleSolver` — exploits exactly this:
it evaluates the per-facility `LagBFunction`s concurrently
(through their `compute_async()`), gathers the linearizations, and
drives the bundle method as usual.

From the user's point of view, switching from serial to parallel
solution of the Lagrangian dual is, once again, a *configuration
change*: the `BlockSolverConfig` ([Chapter 11](11-configuration.md#ch-11)) names
`ParallelBundleSolver` instead of `BundleSolver`, and sets the
number of threads. The decomposition structure ([Chapter 12](12-sub-block.md#ch-12)), the
`LagBFunction`s ([Chapter 14](14-lag-benders-bfunction.md#ch-14)), the recursive locking ([§17.1](17-parallel.md#sec-17-1)) and
the asynchronous evaluation ([§17.2](17-parallel.md#sec-17-2)) do the rest; the surrounding
user code does not change. This is the payoff the energy-system
results in the slide decks rely on: thirty-seven scenarios solved
on as many processes, each with a parallel bundle method inside.

## 17.5 Idioms {#sec-17-5}

**Lock the nearest common ancestor.** To operate atomically on a
set of `Block`s, do not lock them one by one (risking a deadlock
on ordering); lock their nearest common ancestor, which locks the
whole sub-tree and is guaranteed deadlock-free against other
top-down lockers.

**Let the framework lend the owner identity.** When a `:Solver`
recurses into sub-`Block`s, rely on the framework's "lent ID"
mechanism rather than inventing a new owner per level; this is
what keeps the recursive locks re-entrant.

**Return `nullptr` from `get_State()` unless state is real.** Only
implement `State` for a `:Solver` that genuinely benefits from
checkpointing or warm-starting; otherwise the default `nullptr`
keeps the machinery free.

**Remember that the formulation determines the available
parallelism.** Once a parallel `:Solver` is applicable, selecting
it is indeed just a `BlockSolverConfig` change. But *whether* it is
applicable is decided earlier, by the *form* of the `Block`: the
formulation chosen for a `:Block` determines which `:Solver`s can
be attached to it, and hence what parallelism is available. CFL is
the clear illustration — only the Knapsack Formulation exposes the
block-diagonal structure that `LagrangianDualSolver` (and thus
`ParallelBundleSolver`) requires; the Standard and Benders
formulations do not, and no configuration switch can make a
parallel bundle method apply to them. So the genuine lever for
parallelism is often the *formulation*, chosen via the
`BlockConfig` ([Chapter 11](11-configuration.md#ch-11)), with the parallel `:Solver` selection
in the `BlockSolverConfig` being the easy second step that the
formulation has made possible.
