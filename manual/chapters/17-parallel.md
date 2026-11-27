# 17. Parallel and asynchronous computation {#ch-17}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h) (locking),
[`ThinComputeInterface.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h) (`compute_async`, `State`),
[`Solver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solver.h); [`BundleSolver/include/ParallelBundleSolver.h`](https://gitlab.com/smspp/bundlesolver/-/blob/develop/include/ParallelBundleSolver.h)]

We are concerned in this chapter with parallel and asynchronous
computation in SMS++, which is designed for *coarse-grained* parallelism:
the unit of concurrent work is the solution of a `Block` (or of a
sub-`Block`), instead of a fine-grained data-parallel loop. We collect the
mechanisms that make such parallelism safe, i.e., the locking of a `Block`,
the asynchronous evaluation and the state of a `Solver`, and we state what
they do not guarantee.

We first describe the locking protocol, prove that it cannot deadlock under
its rule of direction, and discuss the lending of an identity by which a
`:Solver` delegates work on a part of a tree it holds
([§17.1](17-parallel.md#sec-17-1)). Then we give the asynchronous
computation ([§17.2](17-parallel.md#sec-17-2)) and the `State` of a
`:Solver`, with its uses for checkpointing and reoptimization
([§17.3](17-parallel.md#sec-17-3)). A parallel Lagrangian dual of the CFL
illustrates how these pieces combine ([§17.4](17-parallel.md#sec-17-4)),
the other parallel `:Solver`s are listed in
[§17.5](17-parallel.md#sec-17-5), and the idioms close the chapter
([§17.6](17-parallel.md#sec-17-6)).

## 17.1 Locking a `Block` {#sec-17-1}

Because several threads may want to read or modify the same
`Block` tree, SMS++ provides a locking protocol on `Block`:

- `bool lock( const void * owner )` acquires an *exclusive* (write) lock on
  behalf of `owner`, which identifies the locking entity (usually the
  `Solver`, whose identity `f_id` is initialized with `this`). The lock is
  *recursive over the tree*: locking a `Block` locks all its sub-`Block`s, in
  a pre-order visit (`lock_sub_block()`), since an entity that holds a `Block`
  must hold all its sub-tree to operate on it safely ([Chapter
  12](12-sub-block.md#ch-12)). It returns `true` at once if `owner` already
  holds the `Block`. It returns `false` if the `Block` is write-locked by
  another owner running in the same thread, or read-locked by the same thread
  (either would otherwise deadlock), or if a sub-`Block` cannot be locked, in
  which case the sub-`Block`s it had locked are released; otherwise it sleeps
  on a mutex until the `Block` is free. It throws `std::logic_error` for a
  null or reserved `owner`.
- `void unlock( const void * owner )` releases it, and the sub-tree.
- `bool read_lock( void )` and `read_unlock()` acquire and release a *shared*
  (read) lock. Any number of concurrent readers are allowed, as long as no
  writer holds the `Block`.

The lock records an *owner*, instead of relying on a plain `std::mutex`,
since the same entity may need to re-enter a `Block` it already holds (say,
a `Solver` recursing into a sub-`Block`), and a plain mutex would deadlock on
re-entry. The recorded owner lets the protocol distinguish "someone else holds
this" from "I already hold this".

The one rule the user must respect is the *direction*: an entity that owns a
sub-`Block` never tries to own a `Block` that includes it, and no entity tries
to own two different `Block`s
([`Block.h:1879-1936`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L1879-1936)).
Under this rule the protocol cannot deadlock, as the following lemma
states.

<span id="lem-17-1"></span>**Lemma 17.1.** *Let a finite set of entities each
call `lock()` on one `Block` of the same tree, holding no lock before the call
and keeping the lock, once acquired, only for a finite time. Say that an
entity $e$ waits for an entity $e'$ if $e$ sleeps on a `Block` owned by $e'$.
Then the relation "waits for" has no cycle, and at every moment some entity
that is locking or holding a `Block` is not waiting; hence there is no
deadlock. (The statement concerns `lock()` only; the readers of `read_lock()`
are not covered by it.)*

*Proof.* Let $B_e$ be the `Block` on which $e$ called `lock()`. If $e$ sleeps
on $B_e$, it holds no `Block`, and therefore no entity waits for $e$; such an
$e$ is in no cycle, and it can only be the first element of a chain of waits.
Otherwise, since `lock()` takes a `Block` before its sub-`Block`s and keeps
what it has taken while it sleeps, an entity $e$ that waits for $e'$ owns
$B_e$ and sleeps on a `Block` $C \neq B_e$ of the sub-tree of $B_e$ owned by
$e'$. The entity $e'$ owns $C$ only as a part of its sweep from $B_{e'}$,
hence $C$ is in the sub-tree of $B_{e'}$ too, and the two sub-trees intersect;
therefore one of $B_e$, $B_{e'}$ is an ancestor of the other (or they
coincide). If $B_{e'}$ were an ancestor of $B_e$ or equal to it, then $e'$,
which visits $B_e$ before $C$ in pre-order, would own $B_e$, which is owned by
$e$: a contradiction. Hence $B_{e'}$ is a proper descendant of $B_e$, and
along any chain of the relation, from its second element on, the depth of
$B_e$ strictly increases. A cycle is therefore impossible, and since the tree
has finite depth every chain ends in an entity that does not wait, which
either completes its sweep or fails (returning `false` and releasing what it
holds), and in both cases eventually releases its `Block`s. $\square$

As a consequence, to lock any set of `Block`s safely one locks their nearest
common ancestor. If several entities do so on the same ancestor, they
contend on one `Block`, and one of them succeeds; and since the ancestor is
above all the `Block`s of the set, no entity can own one of them while waiting
for another. We remark that [Lemma 17.1](17-parallel.md#lem-17-1) gives no
guarantee of fairness.

<span id="rem-17-1"></span>**Remark 17.1.** The protocol gives no precedence
to a waiting writer
([`Block.h:2136-2208`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L2136-2208)).
The first `read_lock()` of a `Block` takes its mutex, on which a `lock()` from
another thread then sleeps, and a `read_lock()` on a `Block` that is already
read-locked joins the readers at once, without the mutex: it only updates the
count of the readers of its thread, under a short active wait on the atomic
field that records the owner. Hence, a steady stream of overlapping readers
can, in principle, keep a writer waiting indefinitely.

The immediacy of [Lemma 8.3](08-modification-janus.md#lem-8-3) does not
depend on the locks; however, it holds for one thread at a time, and a change
made by a thread while another one works on the same `Block` is safe only if
the changer holds the lock of the `Block` (or of an ancestor), which the code
does not take on its behalf.

### Lending an ID

The recursive lock raises a problem as soon as a `:Solver`
*delegates*. Suppose a `:Solver` has `lock()`-ed a `Block` to perform
an atomic computation, and in the course of it wants a sub-`Solver` to
work on a part of the tree, say, a sub-`Block`.
The sub-`Solver` needs to `lock()` that sub-`Block`; but the
sub-`Block` is *already* locked, as part of the sub-tree the outer
`:Solver` holds, and the outer `:Solver` cannot simply release it
either, since another entity could then grab it before the outer
`:Solver`'s atomic computation is finished.

The framework resolves this by letting the outer `:Solver` *lend its identity*
to the inner `:Solver`s, with `ThinComputeInterface::set_id( void * id =
nullptr )`
([`ThinComputeInterface.h:935-1015`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h#L935-1015)),
where `nullptr` ends the lease; the base implementation is empty, as is
appropriate for an object that never changes the `Block`. The inner `:Solver`
then follows the usual pattern: it checks `is_owned_by( me )` with the lent
identity, finds the `Block` already owned, does its work, and does *not*
unlock it, since the lender still assumes it is locked; a `lock()` with the
lent identity would equally return `true` at once. Thus, the outer `:Solver`
delegates work on a part of the tree it holds, without releasing the lock and
without deadlocking.

The mechanism only solves the *locking* side of delegation. It is
the responsibility of the lending `:Solver` to ensure, by other means,
that the inner `:Solver`s behave, i.e., that they confine themselves to
the part of the tree they were delegated and do not corrupt the
invariants of the outer computation. In particular, the lock no longer
protects the `Block` from concurrent access if the borrower runs in a
different thread from the lender, and any synchronization this
requires is up to the lender.

## 17.2 Asynchronous computation {#sec-17-2}

Any `ThinComputeInterface`, hence any `Solver`, but also any `Function` such
as a `LagBFunction` or a `BendersBFunction`, can be evaluated asynchronously:
`std::future< int > compute_async( bool changedvars = true )` launches
`compute()` in a separate task and returns a `std::future< int >` on which the
caller can later wait for the result
([`ThinComputeInterface.h:1286`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h#L1286)).
This is the primitive on top of which coarse-grained parallel solution is
built: a driver can fire off the `compute_async()` of several independent
subproblems and collect their results as the futures resolve.

For this to be safe, the `compute()` of a `:Solver` must be
*thread-safe* with respect to the `Block` it solves, and the recursive
locking of [§17.1](17-parallel.md#sec-17-1), including the lent identity,
makes this achievable when a `:Solver` delegates to inner `:Solver`s on
sub-`Block`s.

## 17.3 Solver `State` and checkpointing {#sec-17-3}

A `ThinComputeInterface` may expose a
[`State`](https://smspp.gitlab.io/smspp-project/d6/dff/class_s_m_spp__di__unipi__it_1_1_state.html)
object, a snapshot of its internal algorithmic state, through `virtual State *
get_State( void ) const` and `put_State( const State & )` or `put_State( State
&& )`
([`ThinComputeInterface.h:1997-2049`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinComputeInterface.h#L1997-2049)).
A `State` can be serialized to netCDF, stored, and later restored into a
(compatible) `:Solver`, for the following purposes:

- *checkpointing*: a long-running solve can save its `State`
  periodically and resume from it after an interruption;
- *reoptimization across invocations or processes*: a `State`
  captured from one `:Solver` can warm-start another, which is
  especially valuable in the distributed and SDDP settings where
  the same kind of subproblem is solved at many nodes;
- *restoring the same `:Solver` to an earlier condition*: a `State`
  is useful even within a single `:Solver`, when that
  `:Solver` is invoked under different conditions in an order that
  is not topological. The prototypical case is a branch-and-X
  search with a non-topological visit order: the driver solves a
  node with a `:Solver`, then jumps to an unrelated part of the
  tree, then comes back to explore one of that node's children. To
  resume the children efficiently, one wants the `:Solver` put
  back into the condition it was in *right after the children of the
  node were generated* (e.g., for an LP relaxation `:Solver`, holding
  the optimal basis at that node, whence the child is solved by a few
  dual-simplex pivots instead of from scratch). This is possible by
  capturing the `State` of the node when it is first solved, and
  re-installing it when a child is later visited.

A `:Solver` with no meaningful internal state returns `nullptr` from
`get_State()`, and then the mechanism has no cost.

## 17.4 Example: a parallel Lagrangian dual {#sec-17-4}

The knapsack formulation of CFL (Chapters [12](12-sub-block.md#ch-12) and
[14](14-lag-benders-bfunction.md#ch-14)) decomposes into one Lagrangian
knapsack per facility. Evaluating the Lagrangian function at a vector of
multipliers means solving all these knapsacks, which are independent, and
therefore can be solved in parallel. The
[`ParallelBundleSolver`](https://smspp.gitlab.io/smspp-project/df/d0d/_parallel_bundle_solver_8h.html),
the parallel variant of `BundleSolver` ([Chapter
19](19-nondifferentiable.md#ch-19)), exploits this: it evaluates the
`LagBFunction`s of the facilities concurrently, gathers the linearizations,
and drives the bundle method as usual. The evaluations run on a fixed set of
threads, started the first time they are needed and kept waiting between one
evaluation and the next, and hence no thread is created per evaluation. Their
number is the minimum between `intMaxThread` (the parameter of
`ThinComputeInterface` that `BundleSolver` ignores) and the number of the hard
components. The parameter `intParFrm` chooses whether the results are
consumed in completion order (faster, but not reproducible) or in a fixed
order ([§19.4](19-nondifferentiable.md#sec-19-4)).

From the point of view of the user, switching from the serial to the parallel
solution of the Lagrangian dual is a change of configuration: the
`BlockSolverConfig` ([Chapter 11](11-configuration.md#ch-11)) names
`ParallelBundleSolver` instead of `BundleSolver`, and it has to set the number
of threads (`intMaxThread`, 0 by default, i.e., a sequential run). The
decomposition structure ([Chapter 12](12-sub-block.md#ch-12)), the
`LagBFunction`s ([Chapter
14](14-lag-benders-bfunction.md#ch-14)), the recursive locking
([§17.1](17-parallel.md#sec-17-1)) and the asynchronous evaluation
([§17.2](17-parallel.md#sec-17-2)) do the rest, and the code of the user does
not change.

## 17.5 Other parallel `Solver`s {#sec-17-5}

The same pattern, i.e., independent subproblems evaluated concurrently by
a `:Solver` that holds their common ancestor, recurs elsewhere, the
`:Solver`s taking the number of threads from `intMaxThread` (0 or 1 by
default, i.e., a sequential run):

- `BendersDecompositionSolver` ([§14.6](14-lag-benders-bfunction.md#sec-14-6))
  evaluates the subproblems of a round in parallel in its MILP regime, while
  in the convex one the parallelism is that of the bundle-type master
  (`ParallelBundleSolver`); in both regimes, it attaches the `:Solver`s of
  the subproblems in parallel;
- `PrimalProximalHeur` solves the components of its consensus
  recovery in parallel;
- `FrankWolfeSolver` runs the linear minimization oracles of its
  sub-`Block`s in parallel, one per sub-`Block`, lending its identity to
  their `:Solver`s ([§17.1](17-parallel.md#sec-17-1)), and hence their locks
  never contend;
- `C05SumFunction` ([§13.5](13-function-family.md#sec-13-5)), a `Function`
  instead of a `:Solver`, can evaluate its members in parallel
  (`set_members_at_once()`), possibly on the threads of whoever drives it,
  and `ParallelBundleSolver` evaluates in this way the groups of components
  of `BundleSolver`;
- `BranchAndXSolver` explores the tree with several workers, each on a
  private copy of the `Block`, when it searches depth-first and
  `intMaxThread` is larger than 1, and it can also run the `:Solver`s of one
  node concurrently ([§16.10](16-change.md#sec-16-10)).

## 17.6 Idioms {#sec-17-6}

*The nearest common ancestor.* To operate atomically on a set of `Block`s, one
locks their nearest common ancestor instead of each of them in turn, which
could deadlock on the ordering; by [Lemma 17.1](17-parallel.md#lem-17-1) this
cannot deadlock against other top-down lockers.

*A lent identity.* When a `:Solver` delegates work on sub-`Block`s, it lends
its identity with `set_id()` instead of inventing a new owner per level, and
this keeps the recursive locks re-entrant.

*`get_State()` only for a real state.* Implementing a `State` pays off only
for a `:Solver` that benefits from checkpointing or warm-starting; otherwise,
the default `nullptr` keeps the machinery free.

*The formulation decides the parallelism.* Once a parallel `:Solver` is
applicable, selecting it is a change of the `BlockSolverConfig`; yet,
*whether* it is applicable is decided earlier, before the `:Solver`, by the
formulation chosen for the `:Block` through the `BlockConfig` ([Chapter
11](11-configuration.md#ch-11)), which determines the `:Solver`s that can be
attached to it and hence the parallelism that is available. CFL is a clear
illustration: only the knapsack formulation exposes the block-diagonal
structure that `LagrangianDualSolver` (and hence `ParallelBundleSolver` on
the dual) requires, while the natural formulation does not, and the Benders
one has a single (transportation) subproblem, i.e., no evaluations to run in
parallel.
