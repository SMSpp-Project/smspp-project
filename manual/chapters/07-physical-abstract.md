# 7. Physical vs Abstract representation {#ch-7}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h), [`MCFBlock/include/MCFBlock.h`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h),
[`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h)]

In this chapter we make precise what [Chapter 3](03-mental-model.md#ch-3)
introduced informally: a `Block` carries two representations of the
same model, and the machinery of the framework lets both coexist
without forcing the user to pick one. It also states precisely what
is still missing for the "on demand" rule to cover the `Variable`s
(cf. [§3.3](03-mental-model.md#sec-3-3)), and the direction in which the
interface is meant to evolve.

## 7.1 Two faces of the same model {#sec-7-1}

The *physical* representation (sometimes called the *semantic* one) is
the data of the problem in their natural form, stored by the `:Block`
in ordinary fields. For `MCFBlock` these are the current numbers of
nodes and arcs `NNodes` and `NArcs`, the arrays `SN[a]` and `EN[a]` of
the start and end nodes of each arc `a` (numbered from 1), and the
vectors `U[a]` and `C[a]` of the capacities and costs of the arcs. To
these one adds the vector `B[i]` of the deficits of the nodes and the
counters of the static and dynamic nodes and arcs. For
`BinaryKnapsackBlock` they are the capacity `f_C`, the vectors of
weights `v_W` and of profits `v_P`, and the vector of integrality
flags `v_I`. For `CapacitatedFacilityLocationBlock` they are the
numbers `f_n_facilities` and `f_n_customers`, the maximum number
`f_max_facilities` of open facilities, the vectors of the demands, of
the capacities and of the fixed costs, and the matrix of the
transportation costs.

The *abstract* representation (sometimes called the *syntactic* one)
is the same model written with the `Variable`s, `Constraint`s and
`Objective` of [Chapter 5](05-variable-constraint-objective.md#ch-5),
i.e., in the form that a general-purpose solver expects. For
`MCFBlock` it consists of four parts, each of which has a
`std::vector` for the static nodes or arcs and a `std::list` for the
dynamic ones: the `ColVariable`s of the flows (`NStaticArcs` of them
in the vector), the `FRowConstraint`s of the flow conservation
constraints (`NStaticNodes` in the vector), possibly the
`LB0Constraint`s of the bounds given by the capacities, and an
`FRealObjective` carrying a `LinearFunction` for the total cost of the
flow. The `LB0Constraint`s can be omitted only when no capacity is
given (all of them are infinite) *and* the `Configuration` of
`generate_abstract_constraints()` requires it (cf.
[§7.3](07-physical-abstract.md#sec-7-3)); then the bound $x_a \ge 0$
is carried by the type `kNonNegative` of the `ColVariable`s. The
`LinearFunction` of the `Objective` is always "dense", i.e., all the
flow `ColVariable`s are active in it, even those with a zero cost.

Both representations are meant to describe the same problem, i.e., to
have the same feasible solutions with the same objective values. This
is a property that each `:Block` has to guarantee, and which one can
check on its code; for `MCFBlock` it reads as follows. Let an
`MCFBlock` have $n$ nodes and $m$ arcs, all of them static, none of
them a loop ($SN_a \ne EN_a$), and let its three groups and its
`Objective` have been generated. Let $K$ be the set of the *closed*
arcs, i.e., those whose flow `ColVariable` is fixed (the test of
`is_closed( a )`), and whose value is then 0, since `close_arc()` sets
it before fixing (`load()` closes the arcs whose given cost is
$+\infty$, giving them the cost 0). With $b_i = B_i$ ($b_i = 0$ if `B`
is empty) and $u_a = U_a$ ($u_a = +\infty$ if `U` is empty), the
physical problem is

$$
(P) \qquad \begin{aligned}
\min \;& \sum_{a \notin K} C_a x_a \\
& \sum_{a : EN_a = i} x_a - \sum_{a : SN_a = i} x_a = b_i &&
i = 1, \dots, n , \\
& 0 \le x_a \le u_a ,\;\; x_a = 0 && a \in K
\end{aligned}
$$

where a negative deficit is a source and a positive one a sink. The
abstract problem $(A)$ is the one defined by the `ColVariable`s, the
`Constraint`s and the `Objective` of the `MCFBlock`.

**Proposition 7.1** (Equivalence of the two representations of
`MCFBlock`). *Under the above hypotheses, $(P)$ and $(A)$ have the
same feasible set, and the same objective value at each feasible point.*

*Proof.* We read $(A)$ off the code of
`MCFBlock::generate_abstract_variables()`,
`generate_abstract_constraints()` and `generate_objective()`. The
`ColVariable` $x_a$ of each arc has `is_positive( true )`, i.e., the
type `kNonNegative`, which enforces $x_a \ge 0$, and it is fixed at 0
if and only if $a \in K$. The flow conservation `FRowConstraint` $E_i$
of node $i$ has `set_both( b_i )`, i.e., $l = u = b_i$. Its
`LinearFunction` has the coefficient $-1$ for each arc $a$ with $SN_a
= i$ and $+1$ for each arc with $EN_a = i$, and no `ColVariable`
appears twice in it, since no arc is a loop; hence $E_i$ is the $i$-th
equation of $(P)$. The `LB0Constraint` of arc $a$ has the right-hand
side $u_a$ and the left-hand side 0, i.e., it is the bound $0 \le x_a
\le u_a$; when these constraints are omitted, all the $u_a$ are
$+\infty$, and the bound reduces to $x_a \ge 0$, which the type of
$x_a$ enforces. Hence the feasible sets coincide. Finally, the
`LinearFunction` of the `Objective` has the coefficient $C_a$ for each
arc that is not deleted (0 for $a \in K$, by construction) and no
constant term. Since $x_a = 0$ for $a \in K$ on the feasible set, its
value there is $\sum_{a \notin K} C_a x_a$, whatever the coefficients
of the closed arcs, and the two objective values coincide. $\square$

We remark that the hypotheses can be relaxed with some (rather
tedious) bookkeeping. The dynamic nodes and arcs are handled as the
static ones, in their lists; a deleted arc keeps a fixed `ColVariable`
with a zero coefficient in the rows of its nodes (as the code of
`generate_abstract_constraints()` documents), and therefore it
contributes to neither problem. The changes made after the generation
are covered by the Janus discipline of [Chapter
8](08-modification-janus.md#ch-8), which keeps the two representations
in step (Proposition 8.3).

## 7.2 Why two representations {#sec-7-2}

Both faces exist because different `:Solver`s have different needs. A
*specialized* `:Solver`, written for a specific `:Block`, knows the
problem and reads its physical representation directly: the
`MCFSolver< MCFC >` of [Chapter 6](06-solver.md#ch-6), for instance,
hands the arrays of the `MCFBlock` to the underlying algorithm of
`MCFClass`, and never looks at the abstract representation. This is
typically the fastest way, and (usually) the one that uses the least
memory.

A *general-purpose* `:Solver`, such as a `:MILPSolver`, does not know
about min-cost flow, and reads the abstract representation instead: it
goes through the `Variable`s, builds the matrix of the constraints out
of the `FRowConstraint`s that carry a `LinearFunction`, reads the
`Objective`, and hands the resulting MILP to the underlying MIP
solver. This works on any `:Block` that exposes a suitable abstract
representation, at the price of holding the explicit `Variable`s,
`Constraint`s and `Objective` in memory. Having only the physical face
would exclude the general-purpose `:Solver`s, and having only the
abstract one would give up the speed of the specialized ones; SMS++
keeps both, and each `:Solver` reads the one it needs.

## 7.3 Construction of the abstract representation on demand {#sec-7-3}

The physical representation exists as soon as the `:Block` has been
loaded (cf. [§4.5](04-block.md#sec-4-5)), while the abstract one is
built *on demand*, and its cost is paid only when a `:Solver` that
needs it is attached. The construction is done by five virtual
methods, which the concrete `:Block`s override:

```cpp
virtual void generate_abstract_variables( Configuration * stvv = nullptr );
virtual void generate_abstract_constraints( Configuration * stcc = nullptr );
virtual void generate_dynamic_variables( Configuration * dyvv = nullptr );
virtual void generate_dynamic_constraints( Configuration * dycc = nullptr );
virtual void generate_objective( Configuration * objc = nullptr );
```

which construct, respectively, the static groups of `Variable`s, the
static groups of `Constraint`s, the dynamic groups of `Variable`s
(typically empty at first), the dynamic groups of `Constraint`s, and
the `Objective`. Each takes a `Configuration *` that selects the
optional parts of the construction, which the `:Block` interprets as
it sees fit (in fact, there is no general format); the passed
`Configuration` overrides the corresponding field of the `BlockConfig`
of the `:Block` (cf. [Chapter 11](11-configuration.md#ch-11)), which
is used when the argument is `nullptr`. Its usual form is a
`SimpleConfiguration< int >` whose `f_value` is a bit mask of the
parts to construct. For instance, `MCFBlock` uses the `Configuration`
of `generate_abstract_constraints()` to decide whether the
`LB0Constraint`s of the capacities are omitted, which is allowed only
if all the capacities are infinite ([`MCFBlock.h:526-593`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L526-593)), while
`CapacitatedFacilityLocationBlock` goes further, and uses the
`Configuration` of `generate_abstract_variables()` to choose one of
the four formulations of [§3.5](03-mental-model.md#sec-3-5)
([`CapacitatedFacilityLocationBlock.h:474-627`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h#L474-627)). When the same
`Configuration`s have to be applied to a tree of `Block`s, the
recursive `[C/O/R]BlockConfig` classes of [Chapter
11](11-configuration.md#ch-11) carry those of the five methods (plus a
few others), and propagate them into the sub-`Block`s.

For the user, the rule is that if only specialized `:Solver`s are
attached the abstract representation need not be constructed at all
(with the exception of [§7.6](07-physical-abstract.md#sec-7-6)). A
`:Solver` that reads the abstract face calls the `generate_*` methods
it needs when it is registered (as `MILPSolver::set_Block()` does),
each of which returns at once if its part has already been
constructed. Both faces are then kept in step by the mechanisms of
[Chapter 8](08-modification-janus.md#ch-8).

## 7.4 The lifetime of the abstract representation {#sec-7-4}

Once constructed, the abstract representation lives as long as the `:Block`,
with two exceptions. First, a `:Block` may be reset by another `load( ... )`
(or by `deserialize( ... )`), which replaces the instance altogether. By
convention, `load( ... )` issues a single `NBModification` (the nuclear option
of [§4.6](04-block.md#sec-4-6)) to the listening `:Solver`s, and the abstract
representation of the old instance is deleted (in `MCFBlock`, by the same code
as the destructor, after the `Constraint`s have been emptied so that they do
not unregister from `Variable`s that are being deleted anyway). Of the new
instance, only the parts that `load( ... )` constructs exist afterwards (for
`MCFBlock` and `BinaryKnapsackBlock`, the `Variable`s); the other ones are
constructed again when a `:Solver` that needs them calls the `generate_*`
methods, typically while it processes the `NBModification`, which is
consistent with the "on demand" rule.

Second, the dynamic groups of `Variable`s and `Constraint`s can grow
and shrink at any time, through `add_dynamic_variables()`,
`remove_dynamic_variable[s]()`, `add_dynamic_constraints()` and
`remove_dynamic_constraint[s]()`. These support column and row
generation; they issue the dedicated `Modification`s `BlockModAdd`
and `BlockModRmv` (cf. [Chapter 8](08-modification-janus.md#ch-8)),
which are processed by the `:Solver`s that can handle them.

## 7.5 Checks of feasibility and optimality on both faces {#sec-7-5}

Three virtual methods of `Block` express the duality between the two
representations:

```cpp
virtual bool is_feasible( bool useabstract = false ,
                          Configuration * fsbc = nullptr );
virtual bool is_optimal( bool useabstract = false ,
                         Configuration * optc = nullptr );
virtual bool is_empty( bool useabstract = false ,
                       Configuration * optc = nullptr );
```

Here `is_empty()` returns `true` if the `Block` provably has no
feasible solution and a certificate is at hand. For a convex problem
this amounts to checking that the dual solution held by the `Block` is
a ray along which the dual objective is unbounded; the base class
returns `false`.

With `useabstract == false` (the default) the `:Block` checks the
current solution against its *physical* representation. This is
typically much cheaper, since the `:Block` goes through its arrays and
applies the definition of feasibility of its problem: for `MCFBlock`,
`is_feasible( false )` calls `flow_feasible()` and `bound_feasible()`,
which check the flows against the physical data
([`MCFBlock.h:972-1014`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L972-1014)). With `useabstract == true` the `:Block`
checks the solution against its *abstract* representation, by
computing each `Constraint` and calling its `feasible()`; this
requires the abstract representation to have been built, and it is
generally slower, its main use being by general-purpose `:Solver`s
that do not know the physical representation.

The `Configuration *` gives the tolerance, and its form is documented
by each `:Block` (typically, a `SimpleConfiguration< double >`, taken
from the field `f_is_feasible_Configuration` of the `BlockConfig` when
the argument is `nullptr`). Under the same tolerance the two answers
should agree, by a property such as Proposition 7.1 of the `:Block` at
hand. They can genuinely differ only when the abstract representation
is not complete, notably when some dynamic `Constraint`s have not been
generated yet; the comment of `Block::is_feasible()`
([`Block.h:4605-4709`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L4605-4709)) gives the precise contract. For `MCFBlock`,
`is_optimal()` checks, within the tolerances, that the flows are
feasible, that the duals of the flow conservation and bound
constraints are dual feasible, and that the two satisfy the
complementary slackness conditions. This requires both the `Variable`s
and the `Constraint`s to have been generated, and the dual solution to
have been written into the `Constraint`s.

Also, the first two methods have a counterpart that checks a `Solution`
(cf. [Chapter 9](09-solution.md#ch-9)) instead of the values held by the
`Variable`s:

```cpp
virtual bool is_sol_feasible( Solution * sol , Configuration * fsbc = nullptr );
virtual bool is_sol_optimal( Solution * sol , Configuration * optc = nullptr );
```

with the same meaning of the `Configuration *`. Their names differ from those
of `is_feasible()` and `is_optimal()` on purpose: since a pointer converts
silently to `bool`, an overload with the same name would turn a call meant to
check a `Solution` into `is_feasible( true )` in any `:Block` that only
overrides the latter. The default implementation, which requires the abstract
representation, writes the `Solution` into the `Variable`s, calls
`is_feasible( true )` (or `is_optimal( true )`), and puts back what was there.
In pseudocode, for a `Block` $B$, a `Solution` $s$ and a test $T$ (one of the
two), it reads

```
check(B, s, T):
  if s = null: return false
  if s holds a direction and B cannot hold one: return false
  c <- B.get_Solution(null, false)      // all of the current state
  if c = null: throw logic_error
  d <- B.is_direction()
  if s holds a direction: B.is_direction(true)
  try: s.write(B); r <- T(B)
  catch domain_error: r <- false      // s gives a fixed Variable another value
  catch any other e: restore(B, c, d); throw e
  restore(B, c, d)
  return r

restore(B, c, d):
  B.is_direction(d)
  c.write(B); delete c
```

The property it relies upon is the round-trip contract of a
`Solution`, which all the `:Solution`s are required to satisfy (cf.
[§9.2](09-solution.md#sec-9-2)). (R) If $c$ is obtained by
`B.get_Solution( nullptr , false )`, then after any sequence of writes
into the `Variable`s (and the duals of the `Constraint`s) of $B$ that
neither adds nor removes any of them, `c.write( B )` gives each of
them back the value it had when $c$ was obtained.

**Lemma 7.2** (The default checks leave the `Block` as they find
it). *Let $B$ satisfy (R). Then `is_sol_feasible( s )` and
`is_sol_optimal( s )` either return the value of $T$ on the state written
by $s$ (or `false`), or pass on the exception that $T$ or the write of
$s$ throws when it is not a `std::domain_error`; in both cases, afterwards
the values of the `Variable`s and of the duals of $B$, and its flag
`is_direction()`, are those it had before the call.*

*Proof.* If one of the two early returns is taken, no value has been
written. Otherwise $c$ is taken before anything is written; then $s$
writes into $B$, which (writing values only) adds or removes no
`Variable` and no `Constraint`, and the test reads them. If $T$, or the
write of $s$, throws `std::domain_error` the exception is caught and the
answer is `false`; if it throws an exception of any other type, the
handler runs the restoring steps before passing it on. Hence control
reaches the restoring steps in every case: the flag is set back to the
value $d$ saved before it was changed, and `c.write( B )` restores the
values by (R). $\square$

The restoring steps are a single function, which also deletes $c$ (it
is held by a `std::unique_ptr`), called both on the normal exit and in
the handler of the exceptions of the other types
([`SMS++/src/Block.cpp:645-671`](https://gitlab.com/smspp/smspp/-/blob/develop/src/Block.cpp#L645-671)); we remark that the lemma takes for
granted that `c.write( B )` itself does not throw, as the proof of (R)
for the `ColVariableSolution` in [§9.2](09-solution.md#sec-9-2) shows.

A `:Block` that can read its `:Solution` directly overrides the two
methods, and declares it by returning `true` from
`is_sol_feasible_physical()` (or `is_sol_optimal_physical()`), in
which case the `Variable`s are not touched and need not even exist.
`MCFBlock` does this for `MCFSolution`, while `BinaryKnapsackBlock`
(for `BinaryKnapsackSolution`) and `ThermalUnitBlock` (for the
schedule of a unit) do it for feasibility only.

## 7.6 Status — under development: the half-baked `Solution` {#sec-7-6}

The picture of [§7.1](07-physical-abstract.md#sec-7-1), two faces that coexist
with neither being privileged, is not, at version 0.8.0, a faithful
description of the implementation, but the one towards which it is moving.
Today, the situation is the following. The `Constraint`s and the `Objective`
of the abstract representation are built genuinely on demand, i.e., only if a
`:Solver` that needs them is attached. The `Variable`s, instead, are *always*
constructed by the `:Block`s of the running examples, whether or not any
`:Solver` reads them, since the standard way for a `:Solver` to report its
solution is `get_var_solution()`, which *writes into* the `Variable`s of the
`Block`. Even an `MCFBlock` attached only to an `MCFSolver< MCFSimplex >` has
its `std::vector< ColVariable >` of size `NStaticArcs` allocated by
`load( ... )`, where the `:Solver` can put the optimal flows.

The way out is to let a `:Solver` return a `Solution` object *directly*,
instead of writing the solution component by component into the `Variable`s,
and this half is now in the interface.
`Solver::get_Solution( Configuration * )` returns a `Solution` specific to the
`:Block` (such as `MCFSolution`, see [Chapter 9](09-solution.md#ch-9)); a
"physical" `:Solver`, which builds it out of its data structures without
touching the `Variable`s, declares it by returning `true` from
`is_get_Solution_physical()`. `MCFSolver`, `MCFLemonSolver`, the `:Solver`s of
`BinaryKnapsackBlock` and `ThermalUnitDPSolver`, among others, do so, and the
`Solution` can then be checked by `is_sol_feasible()` and `is_sol_optimal()`
(cf. [§7.5](07-physical-abstract.md#sec-7-5)), again without the `Variable`s.
Still missing is the other half: the `:Block`s of the running examples still
construct their `Variable`s in `load( ... )`, and therefore a `:Block`
attached only to specialized `:Solver`s does not yet skip them. This is one of
several places where the interface of SMS++ is not settled at version 0.8.0,
and code written today should not assume that the present arrangement will
persist; in [Chapter 9](09-solution.md#ch-9) we return to the point from the
side of `Solution`.

## 7.7 Idioms {#sec-7-7}

Code that uses a `:Block` need not call the `generate_*()` methods,
since the `:Solver`s that need the abstract representation call them
when they are registered. Calling them by hand is useful in two cases
only: when the abstract representation has to be present for
inspection or for `is_feasible( true )`, and when a `Configuration`
different from the one in the `BlockConfig` of the `:Block` has to be
passed.

When the solution in the `Variable`s of a `:Block` has been put there
by a specialized `:Solver`, the physical check `is_feasible( false )`
is both cheaper and more reliable, since the abstract one does not add
to what the specialized `:Solver` already knows. The check
`is_feasible( true )` is meant for the cases where the abstract
representation is the only one available, and for debugging the
differences between the two.

Until the interface evolves along the lines of
[§7.6](07-physical-abstract.md#sec-7-6), the abstract `Variable`s of a
`:Block` are best thought of as the place where a `:Solver` writes the
solution, rather than as the syntactic form of the problem. When the
abstract face is needed in full (say, by a `:MILPSolver`) it is
constructed on demand, while the code that reads the solution goes
through the methods of the `:Block` or through a `Solution`.
