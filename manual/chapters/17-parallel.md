# 17. Parallel and asynchronous computation {#ch-17}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h) (locking),
[`ThinComputeInterface.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h) (`compute_async`, `State`),
[`Solver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solver.h); [`BundleSolver/include/ParallelBundleSolver.h`](https://gitlab.com/smspp/bundlesolver/-/blob/develop/include/ParallelBundleSolver.h)]

SMS++ is designed for *coarse-grained* parallelism: the unit of
concurrent work is the solution of a `Block` (or sub-`Block`), not
a fine-grained data-parallel loop. This chapter collects the
mechanisms that make such parallelism safe and efficient (locking,
asynchronous evaluation, and the state of a `Solver`), and says what is
not yet handled.

## 17.1 Locking a `Block` {#sec-17-1}

Because several threads may want to read or modify the same
`Block` tree, SMS++ provides a locking protocol on `Block`:

- `bool lock( const void * owner )` acquires an *exclusive* (write)
  lock on behalf of `owner`, which identifies the locking entity
  (usually the `Solver` itself, whose identity `f_id` is initialized
  with `this`). The lock is *recursive over the tree*: locking a
  `Block` locks all of its sub-`Block`s, in a pre-order visit
  (`lock_sub_block()`), since an entity that holds a `Block` must hold
  its whole sub-tree to operate on it safely
  ([Chapter 12](12-sub-block.md#ch-12)). It returns `true` at once if
  `owner` already holds the `Block`; it returns `false` if the `Block`
  is held by another owner running in the same thread (which would
  otherwise deadlock), or if a sub-`Block` cannot be locked, in which
  case the sub-`Block`s it had locked are released; otherwise it
  sleeps on a mutex until the `Block` is free. It throws
  `std::logic_error` for a null or reserved `owner`.
- `void unlock( const void * owner )` releases it, and the sub-tree.
- `bool read_lock( void )` and `read_unlock()` acquire and release a
  *shared* (read) lock; any number of concurrent readers are allowed,
  as long as no writer holds the `Block`.

The lock records an *owner* rather than relying on a plain
`std::mutex`, for a specific reason: the same entity may need to
re-enter a `Block` it already holds (a `Solver` recursing into a
sub-`Block`, say), and a plain mutex would deadlock on
re-entry. Recording the owner lets the protocol distinguish
"someone else holds this" from "I already hold this".

The one rule the user must respect is the *direction*: an entity that
owns a sub-`Block` never tries to own a `Block` that includes it, and
no entity tries to own two different `Block`s ([`Block.h:1880-1916`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L1880-1916)).
Under this rule the protocol cannot deadlock, which is the following
statement.

**Lemma 17.1.** *Let a finite set of entities each call `lock()` on one
`Block` of the same tree, holding no lock before the call and keeping
the lock, once acquired, only for a finite time. Say that an entity $e$
waits for an entity $e'$ if $e$ sleeps on a `Block` owned by $e'$.
Then the relation "waits for" has no cycle, and at every moment some
entity that is locking or holding a `Block` is not waiting; hence there
is no deadlock.*

*Proof.* Let $B_e$ be the `Block` on which $e$ called `lock()`. Since
`lock()` takes a `Block` before its sub-`Block`s and keeps what it has
taken while it sleeps, an entity $e$ that waits for $e'$ owns $B_e$ and
sleeps on a `Block` $C$ of the sub-tree of $B_e$ owned by $e'$. The
entity $e'$ owns $C$ only as a part of its own sweep from $B_{e'}$,
hence $C$ is in the sub-tree of $B_{e'}$ too, and the two sub-trees
intersect; therefore one of $B_e$, $B_{e'}$ is an ancestor of the
other (or they coincide). If $B_{e'}$ were an ancestor of $B_e$ or
equal to it, then $e'$, which visits $B_e$ before $C$ in pre-order,
would own $B_e$, which is owned by $e$: a contradiction. Hence $B_{e'}$
is a proper descendant of $B_e$, and along any chain of the relation
the depth of $B_e$ strictly increases. A cycle is therefore impossible,
and since the tree has finite depth every chain ends in an entity that
does not wait, which either completes its sweep or fails (returning
`false` and releasing what it holds), and in both cases eventually
releases its `Block`s. $\square$

The corollary is convenient: *to lock any set of `Block`s safely, lock
their nearest common ancestor*. If several entities do so on the same
ancestor, they contend on one `Block`, and one of them succeeds; and
since the ancestor is above all the `Block`s of the set, no entity can
own one of them while waiting for another. We remark that Lemma 17.1
says nothing about fairness, which is the subject of the following
status note.

> *Status: under development.* Write starvation is not yet
> handled: a steady stream of readers can, in principle, keep a
> writer waiting indefinitely. Applications that mix frequent reads
> with occasional writes should be aware of this.

This same recursive locking underpins the immediacy guarantee of
[§8.4](08-modification-janus.md#sec-8-4): an abstract change is applied while the `Block` (hence its
whole sub-tree) is locked, so no concurrent mutation can interleave
between a change and the `Block`'s reaction to it.

### Lending an ID

The recursive lock raises a problem the moment a `:Solver`
*delegates*. Suppose a `:Solver` has `lock()`-ed a `Block` to perform
an atomic computation, and in the course of it wants a sub-`Solver` to
work on a part of the tree, say, a sub-`Block`.
The sub-`Solver` needs to `lock()` that sub-`Block`; but the
sub-`Block` is *already* locked, as part of the sub-tree the outer
`:Solver` holds. The outer `:Solver` cannot simply release it
either: doing so would risk another entity grabbing it before the
outer `:Solver`'s atomic computation is finished.

The framework resolves this by letting the outer `:Solver` *lend its
identity* to the inner `:Solver`s, with
`ThinComputeInterface::set_id( void * id = nullptr )`
([`ThinComputeInterface.h:965-1016`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h#L965-1016); `nullptr` ends the lease, and the
base implementation does nothing, as is appropriate for an object that
never changes the `Block`). The inner `:Solver` then follows the usual
pattern: it checks `is_owned_by( me )` with the lent identity, finds the
`Block` already owned, does its work, and does *not* unlock it, since
the lender still assumes it is locked; a `lock()` with the lent
identity would equally return `true` at once. The outer
`:Solver` thus delegates work on a part of the tree it holds,
without releasing the lock and without deadlocking against itself.

The mechanism only solves the *locking* side of delegation. It is
the responsibility of the lending `:Solver` to ensure, by other means,
that the inner `:Solver`s behave, i.e., that they confine themselves to
the part of the tree they were delegated and do not corrupt the
invariants of the outer computation. In particular, the lock no longer
protects the `Block` from concurrent access if the borrower runs in a
different thread from the lender, and any synchronization this
requires is up to the lender.

## 17.2 Asynchronous computation {#sec-17-2}

Any `ThinComputeInterface`, hence any `Solver`, but also any
`Function` such as a `LagBFunction` or a `BendersBFunction`, can be
evaluated asynchronously: `std::future< int > compute_async( bool
changedvars = true )` launches `compute()` in a separate task and
returns a `std::future< int >` on which the caller can later wait for
the result ([`ThinComputeInterface.h:1287`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h#L1287)). This is the
primitive on top of which coarse-grained parallel solution is
built: a driver can fire off the `compute_async()` of several
independent sub-problems and collect their results as the futures
resolve.

For this to be safe, a `:Solver`'s `compute()` must be
*thread-safe* with respect to the `Block` it solves; the recursive
locking of [§17.1](17-parallel.md#sec-17-1), including the lent identity, is what
makes that achievable when a `:Solver` delegates to inner
`:Solver`s on sub-`Block`s.

## 17.3 Solver `State` and checkpointing {#sec-17-3}

A `ThinComputeInterface` may expose a
[`State`](https://smspp.gitlab.io/smspp-project/d6/dff/class_s_m_spp__di__unipi__it_1_1_state.html)
object, a snapshot of its internal algorithmic state, through
`virtual State * get_State( void ) const` and `put_State( const State & )`
or `put_State( State && )` ([`ThinComputeInterface.h:1998-2050`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h#L1998-2050)).
A `State` can be serialised to netCDF, stored, and later restored
into a (compatible) `:Solver`, which serves three purposes:

- *checkpointing*: a long-running solve can save its `State`
  periodically and resume from it after an interruption;
- *reoptimization across invocations or processes*: a `State`
  captured from one `:Solver` can warm-start another, which is
  especially valuable in the distributed and SDDP settings where
  the same kind of sub-problem is solved at many nodes;
- *restoring the same `:Solver` to an earlier condition*: a `State`
  is useful even within a single `:Solver`, when that
  `:Solver` is invoked under different conditions in an order that
  is not topological. The prototypical case is a branch-and-X
  search with a non-topological visit order: the driver solves a
  node with a `:Solver`, then jumps to an unrelated part of the
  tree, then comes back to explore one of that node's children. To
  resume the children efficiently, one wants the `:Solver` put
  back into the condition it was in *right after the node's
  children were generated* (for an LP relaxation `:Solver`, for
  instance, holding the optimal basis at that node, so that the child
  is solved by a few dual-simplex pivots rather than from scratch).
  Capturing the node's `State` when it is first solved and
  re-installing it when a child is later visited is exactly what
  makes this possible.

A `:Solver` with no meaningful internal state simply returns
`nullptr` from `get_State()`; the mechanism then costs nothing.

## 17.4 Example: a parallel Lagrangian dual {#sec-17-4}

The Knapsack Formulation of CFL (Chapters [12](12-sub-block.md#ch-12), [14](14-lag-benders-bfunction.md#ch-14)) decomposes into
one independent Lagrangian knapsack per facility; evaluating the
Lagrangian function at a given multiplier vector means solving all
of those knapsacks, which are independent and therefore
parallelisable. The
[`ParallelBundleSolver`](https://smspp.gitlab.io/smspp-project/df/d0d/_parallel_bundle_solver_8h.html),
the parallel variant of `BundleSolver`, exploits this:
it evaluates the per-facility `LagBFunction`s concurrently,
gathers the linearizations, and drives the bundle method as usual.
The evaluations run on a fixed set of threads, started the first
time they are needed and kept waiting between one evaluation and
the next, so that no thread is created per evaluation; their number
is the minimum between `intMaxThread` (the parameter of
`ThinComputeInterface` that `BundleSolver` ignores) and the number
of the hard components. The parameter `intParFrm` chooses how the
results are consumed: in completion order (the fastest, but the
trajectory of the algorithm then depends on the timing of the
threads and is not reproducible), or in a fixed order, which makes
the run deterministic, optionally processing exactly the same
components as the sequential `BundleSolver` or evaluating all the
components at each iteration.

From the user's point of view, switching from serial to parallel
solution of the Lagrangian dual is, once again, a *configuration
change*: the `BlockSolverConfig` ([Chapter 11](11-configuration.md#ch-11)) names
`ParallelBundleSolver` instead of `BundleSolver`, and sets the
number of threads (`intMaxThread`). The decomposition structure ([Chapter 12](12-sub-block.md#ch-12)), the
`LagBFunction`s ([Chapter 14](14-lag-benders-bfunction.md#ch-14)), the recursive locking ([§17.1](17-parallel.md#sec-17-1)) and
the asynchronous evaluation ([§17.2](17-parallel.md#sec-17-2)) do the rest; the surrounding
user code does not change. The energy-system results of the slide decks rely on this, with 37
scenarios solved on as many processes, each with a parallel bundle
method inside.

## 17.5 Other parallel `Solver`s {#sec-17-5}

The same pattern, independent sub-problems evaluated concurrently
by a `:Solver` that holds their common ancestor, recurs in several
`:Solver`s, which take the number of threads from `intMaxThread`
(whose default gives a sequential run):

- `BendersDecompositionSolver` ([§14.3](14-lag-benders-bfunction.md#sec-14-3)) evaluates the subproblems of
  a round in parallel in its MILP regime, while in the convex one
  the parallelism is that of the bundle-type master
  (`ParallelBundleSolver`);
- `PrimalProximalHeur` solves the components of its consensus
  recovery in parallel;
- `FrankWolfeSolver` runs the linear minimization oracles of its
  sub-`Block`s in parallel, each on its own sub-`Block`, lending
  its identity to their `:Solver`s ([§17.1](17-parallel.md#sec-17-1)) so that their locks
  never contend;
- `C05SumFunction` ([§13.5](13-function-family.md#sec-13-5)), which is a `Function` and not a
  `:Solver`, can evaluate its members in parallel
  (`set_members_at_once()`), possibly on the threads of whoever
  drives it, which is how
  `ParallelBundleSolver` evaluates the groups of components of
  `BundleSolver`;
- `BranchAndXSolver` ([§16.5](16-change.md#sec-16-5)) explores the enumeration tree with
  several workers, each driving its own copy of the inner
  `:Solver`s, so that applying the `Change`s needs no
  synchronization, and can also run the `:Solver`s of a single
  node concurrently (`intThreadForDifferentSolvers`), the
  reduction of their results staying serial.

## 17.6 Idioms {#sec-17-6}

*The nearest common ancestor.* To operate atomically on a set of
`Block`s, one locks their nearest common ancestor rather than each of
them in turn, which could deadlock on the ordering; by Lemma 17.1 this
cannot deadlock against other top-down lockers.

*A lent identity.* When a `:Solver` delegates work on sub-`Block`s,
it lends its identity with `set_id()` rather than inventing a new owner
per level, which is what keeps the recursive locks re-entrant.

*`get_State()` only for a real state.* A `State` is worth implementing
only for a `:Solver` that benefits from checkpointing or warm-starting;
otherwise the default `nullptr` keeps the machinery free.

*The formulation decides the parallelism.* Once a parallel `:Solver`
is applicable, selecting it is a change of the `BlockSolverConfig`. But *whether* it is
applicable is decided earlier, by the *form* of the `Block`: the
formulation chosen for a `:Block` determines which `:Solver`s can
be attached to it, and hence what parallelism is available. CFL is
the clear illustration: only the Knapsack Formulation exposes the
block-diagonal structure that `LagrangianDualSolver` (and hence
`ParallelBundleSolver` on the dual) requires, while the Standard
Formulation does not, and the Benders one has a single (transportation)
subproblem, hence nothing to evaluate in parallel. So the genuine lever for
parallelism is often the *formulation*, chosen via the
`BlockConfig` ([Chapter 11](11-configuration.md#ch-11)), with the parallel `:Solver` selection
in the `BlockSolverConfig` being the easy second step that the
formulation has made possible.
