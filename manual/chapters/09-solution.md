# 9. Solution {#ch-9}

[Source: [`SMS++/include/Solution.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solution.h), [`ColVariableSolution.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ColVariableSolution.h),
[`RowConstraintSolution.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/RowConstraintSolution.h), [`ColRowSolution.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ColRowSolution.h);
[`RowConstraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/RowConstraint.h); [`MCFBlock/include/MCFBlock.h`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h) (`MCFSolution`)]

We are concerned in this chapter with `Solution`, the object that stores a
solution of a `Block` apart from the `Block` itself, so that it can be kept,
restored, written to a file or combined with other solutions; since the dual
values are part of a solution, we also state the convention on their signs,
which every `Solver` that writes them has to follow.

After the concept and the methods at its heart
([§9.1](09-solution.md#sec-9-1)), we state the round-trip contract on which
the uses of a `Solution` rest, and prove it for `ColVariableSolution`
([§9.2](09-solution.md#sec-9-2)). The serialization and the linear
combinations of solutions, with the condition under which a combination of
feasible solutions is feasible, are in [§9.3](09-solution.md#sec-9-3). The
`Solution`s of the core, which work on the abstract representation, and the
sign of the dual values are in [§9.4](09-solution.md#sec-9-4), and the
physical `Solution`s of the `:Block`s in [§9.5](09-solution.md#sec-9-5).
After an example ([§9.6](09-solution.md#sec-9-6)), we discuss the part of the
interface that is still moving ([§9.7](09-solution.md#sec-9-7)), and close
with an outline of the interface ([§9.8](09-solution.md#sec-9-8)) and the
idioms ([§9.9](09-solution.md#sec-9-9)).

## 9.1 Concept {#sec-9-1}

A
[`Solution`](https://smspp.gitlab.io/smspp-project/d8/d63/class_s_m_spp__di__unipi__it_1_1_solution.html)
is an object that stores a solution of a `Block` (typically the values
of its `Variable`s and, where applicable, the dual values attached to
its `Constraint`s), in a form that can be saved, restored, serialized
or combined with other solutions. A basic design decision is that a
`Solution` is specific to a `Block`, not to a `Solver`: the same
`Block` produces the same kind of `Solution` whichever `:Solver`
computed it. A solution of a min-cost flow problem is an `MCFSolution`
whether it was found by an `MCFSolver< MCFSimplex >` or by a
general-purpose `:MILPSolver`; the `Solution` knows how to read the
solution out of an `MCFBlock` and how to write it back into one,
whatever way the values got there.

The two methods at the heart of the interface are

```cpp
virtual void read( const Block * const block );
virtual void write( Block * const block );
```

which, respectively, read the current solution out of `block` and
store it in the `Solution`, and write the stored solution back into
`block`. A `Solution` is normally constructed by the `Block` whose
solution it stores, through

```cpp
virtual Solution * get_Solution( Configuration * solc = nullptr ,
                                 bool emptys = true );
```

whose `Configuration *` selects a *part* of the solution (only the
primal values, only the duals, only some groups of `Variable`s). With
`solc == nullptr` the corresponding `Configuration` of the
`BlockConfig` is used, and if there is none the complete solution is
stored, primal and dual; this choice is permanent, i.e., the `Solution`
will only ever store that part. With `emptys == true` (the default) the
`Solution` is returned empty, to be filled by a later `read()`, while
with `emptys == false` it already holds the current solution. The base
`Block::get_Solution()` returns `nullptr`, i.e., a `:Block` that does
not override it has no `Solution`. Reading or writing a part that the
`Block` does not have (say, dual values when the `Constraint`s have not
been generated) is an error by the contract of `read()` and `write()` in
[`Solution.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solution.h), and concrete `:Solution`s throw in this case, with
the exception noted in [§9.4](09-solution.md#sec-9-4).

## 9.2 The Block of a Solution {#sec-9-2}

A `Solution` is bound to the `Block` that created it: unless the
specific `:Solution` declares otherwise, it can read and write the
solution of *that very* `Block` only, not even of another `Block` of
the same class (as the comment of `Block::get_Solution()` states). A
`:Solution` that allows the exchange between distinct `Block`s (for
instance, between a `Block` and a copy of it produced as an R3 Block,
see [Chapter 10](10-r3block.md#ch-10)) must state it in its
documentation, and the exchange is then to be used with care.

The property on which the uses of a `Solution` rest is the following
contract, which all the `:Solution`s are required to satisfy, and
which [Lemma 7.1](07-physical-abstract.md#lem-7-1) uses.

*Round trip (R).* Let $c$ be returned by `B.get_Solution( solc , false )` (or
emptied and then filled by `c.read( B )`). After any sequence of writes of
values into the `Variable`s and the duals of $B$ that adds or removes none of
them, `c.write( B )` gives each value of the part selected by `solc` back the
value it had when $c$ was filled.

For the abstract `Solution`s of the core we can prove that the
contract is a consequence of the code.

<span id="lem-9-1"></span>**Lemma 9.1** (Round trip of `ColVariableSolution`).
*A `ColVariableSolution` satisfies (R) for the values of the `ColVariable`s,
provided that the `Block` and its sub-`Block`s have the same groups of
`Variable`s, with the same sizes, at the time of `read()` and of `write()`.*

*Proof.* `read()` goes through the static groups of `Variable`s of the
`Block` with `for_each_as< ColVariable >()`, storing `get_value()` of
each element in the order of the storage, then through the dynamic
groups in the same way, then recursively through the sub-`Block`s
([`SMS++/src/ColVariableSolution.cpp:372-420`](https://gitlab.com/smspp/smspp/-/blob/develop/src/ColVariableSolution.cpp#L372-420)). `write()` goes through
the same groups in the same order, and calls `set_value()` with the
stored value of each element
([`SMS++/src/ColVariableSolution.cpp:424-445`](https://gitlab.com/smspp/smspp/-/blob/develop/src/ColVariableSolution.cpp#L424-445)). Since the order of
`for_each_as()` is the order of the storage, which is part of the
contract of `BaseGroup` (cf.
[§5.5](05-variable-constraint-objective.md#sec-5-5)), and the groups
have not changed, the $k$-th value written goes to the `ColVariable`
from which the $k$-th value was read. `set_value()` assigns the value,
unless the `ColVariable` is fixed, in which case it keeps the value it
is fixed at if the new one differs from it by at most
$10^{-6} \max\{ 1 , |v| \}$, $v$ being the value at which it is fixed (a
relative tolerance, which is absolute below 1), and throws
`std::domain_error` otherwise. A `ColVariable` that is
fixed now was fixed when it was read, since the writes of values in
(R) do not fix or unfix anything; also, its value cannot have changed
in between, since a fixed `ColVariable` does not accept a different
value. Hence the stored value is its current one, no exception is
thrown, and each `ColVariable` gets back the value it had. $\square$

## 9.3 Serialization and combination {#sec-9-3}

Beyond `read()` and `write()`, the base interface of `Solution`
provides the following.

Serialization to netCDF: `serialize( netCDF::NcGroup & )`,
`serialize( const std::string & filename , bool replace = true )` (which
writes the `Solution` as the group `Solution_<k>` of the file, replacing the
file or adding to it), and the corresponding
`deserialize( const netCDF::NcGroup & )`, by which a `Solution` can be written
to disk and read back independently of its `Block`. The static methods
`Solution::deserialize( const std::string & filename )` and
`Solution::new_Solution( const netCDF::NcGroup & )` also construct the right
`:Solution` out of the name of the class stored in the group, through the
factory of `Solution`s, as the factories of `Block`s and `Solver`s do for
their hierarchies (cf. [Chapter 18](18-factories-netcdf.md#ch-18)).

Copying: `clone( bool empty = false )` produces a copy of the
`Solution`, or an empty one of the same type and configuration if
`empty == true`.

Linear combination: `scale( double factor )` returns a new, scaled
copy, and `sum( const Solution * solution , double multiplier )` adds
a scaled `solution` to this one; together, they form weighted sums,
and in particular convex combinations, of solutions of the same
`Block`. Convex combinations are central to several methods of
optimization (the convexified primal solution produced by a Lagrangian
dual, for instance; see [Chapter
14](14-lag-benders-bfunction.md#ch-14) and [Recipe
R4](R4-cfl-lagrangian.md#rec-R4)), which is why the operation is part
of the base interface. Their use rests on the following classical
fact, whose hypothesis is the one to check before relying on it.

<span id="lem-9-2"></span>**Lemma 9.2** (Convex combinations of feasible
solutions). *Let the
feasible region $X$ of a `Block` be convex (for instance, its
`Constraint`s are linear and none of its `ColVariable`s is integer),
let $s_1$ and $s_2$ be `ColVariableSolution`s of it whose values
$x_1$ and $x_2$ lie in $X$, and let $\alpha \in [0, 1]$. Then the
`Solution` $m$ obtained as `m = s1->scale( alpha )` followed by
`m->sum( s2 , 1 - alpha )` holds $\alpha x_1 + (1 - \alpha) x_2$,
which lies in $X$; if moreover the `Objective` is linear (convex for
minimization), its value at $m$ is equal to (not larger than) the same
combination of the values at $s_1$ and $s_2$.*

*Proof.* `scale()` multiplies each stored value by `alpha`, and
`sum()` adds to each stored value `multiplier` times the corresponding
one of the other `Solution` (the two having the same structure);
hence, $m$ holds $\alpha x_1 + (1 - \alpha) x_2$ componentwise. This
point lies in $X$ by the definition of a convex set; the statement on
the `Objective` is the definition of a linear (convex) function.
$\square$

Without convexity the combination may well be infeasible (two distinct
binary solutions combined with $\alpha = 1/2$ have fractional
components), and a "rounding" heuristic then has to repair it. Some
`Solution`s, furthermore, cannot be meaningfully scaled or summed:
a `Solution` whose values are combinatorial by nature (say, a path stored
as a predecessor function) cannot represent a scaled path. Its `scale()` may
then return a `Solution` of a *different*, "more general" type (say, a
flow on each arc), which one can scale and sum with others. The contract
is in the comments of `scale()` and `sum()`
([`Solution.h:540-626`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solution.h#L540-626)).

Two further parts of the base interface concern a `Solution` that is kept
while its `Block` changes. First, a `Solution` knows whether it holds a
solution or a *direction*, i.e., a ray of the feasible region along which the
`Objective` is unbounded (`is_direction()`, set by `read()` from
`Block::is_direction()`, or by whoever fills it). `clone()`, `scale()` and the
serialization keep the flag, while a `sum()` is a direction only if all its
terms are. Second, `adapt( block , mod , dropped )` informs the `Solution`
that its `Block` has undergone the `Modification` `mod` (cf. [Chapter
8](08-modification-janus.md#ch-8)). The answer is `kUnchanged` (the `Solution`
still fits), `kAdapted` (the values of the removed dynamic `Variable`s or
`Constraint`s, or of what a physical `Modification` removes, have been
dropped, and written in `dropped`) or `kInvalid` (the `Solution` does not fit
the `Block` any more, as after an `NModification`); `adapts()` returns the
kinds of `Modification` that a `:Solution` has to look at. Whether the values
left are still feasible is not the business of `adapt()`, but of
`Block::is_sol_feasible()` (cf. [§7.5](07-physical-abstract.md#sec-7-5)). Both
global pools of `LagBFunction` and `BendersBFunction` use this to keep the
`Solution`s they store in step with their inner `Block`.

## 9.4 The abstract Solutions of the core {#sec-9-4}

Three concrete `:Solution`s of the core work on the *abstract*
representation of a `Block`, and are therefore reusable by any
`:Block` whose abstract representation has the right form.
[`ColVariableSolution`](https://smspp.gitlab.io/smspp-project/da/d85/class_s_m_spp__di__unipi__it_1_1_col_variable_solution.html)
stores the *primal* solution of a `Block` whose `Variable`s are all
`ColVariable`s, i.e., their values;
[`RowConstraintSolution`](https://smspp.gitlab.io/smspp-project/dd/d09/class_s_m_spp__di__unipi__it_1_1_row_constraint_solution.html)
stores the *dual* solution of a `Block` whose `Constraint`s are all
`RowConstraint`s, i.e., their dual values; and
[`ColRowSolution`](https://smspp.gitlab.io/smspp-project/de/d2a/class_s_m_spp__di__unipi__it_1_1_col_row_solution.html)
stores both. A `Block` that defines no `:Solution`, but whose abstract
representation has one of these forms, can use them by overriding
`get_Solution()` to construct one (the base one returns `nullptr`, with
which the default `is_sol_feasible()` of
[§7.5](07-physical-abstract.md#sec-7-5) throws `std::logic_error`). Unlike
what the contract of [§9.1](09-solution.md#sec-9-1) would suggest,
`ColVariableSolution::read()` does not throw when the groups of the
`Block` differ from those it holds: it re-initializes on the structure
of the `Block` and reads it, while `write()` throws `std::logic_error`
on a mismatch. Within a cell of a dynamic group, however, a different number
of elements is tolerated: the `Variable`s beyond the stored values get their
default value.

### Dual values and their signs

A `RowConstraint` $l \le g(x) \le u$ with $-\infty < l < u < \infty$ stands
for two constraints, and yet it holds a single dual value (`get_dual()`,
`set_dual()`), the value that a `RowConstraintSolution` stores. The reason is
the following classical fact, given in the comment of `RowConstraint`
([`RowConstraint.h:108-290`](https://gitlab.com/smspp/smspp/-/blob/develop/include/RowConstraint.h#L108-290)). Consider the problem

$$
(P) \qquad \min \{\, f(x) \;:\; l \le g(x) \le u \,,\; x \in X \,\}
$$

where $X$ is any set and $g : X \to \mathbb{R}$ is any function. With the
multipliers $w \ge 0$ of $l \le g(x)$ and $z \ge 0$ of $g(x) \le u$, its
Lagrangian function is

$$
L(w, z) = \min \{\, f(x) + w (l - g(x)) + z (g(x) - u) \;:\; x \in X \,\}
        = w l - z u + \min \{\, f(x) + (z - w) g(x) \;:\; x \in X \,\}
$$

and the Lagrangian dual of $(P)$ is $(D)$
$\max \{\, L(w, z) : w \ge 0 \,,\, z \ge 0 \,\}$.

<span id="lem-9-3"></span>**Lemma 9.3** (Non-dominated multipliers). *Let
$(\bar{w}, \bar{z})$ be feasible for $(D)$, with $\bar{w} > 0$ and
$\bar{z} > 0$, and let $w' = \max \{ 0 , \bar{w} - \bar{z} \}$ and
$z' = \max \{ 0 , \bar{z} - \bar{w} \}$. Then $(w', z')$ is feasible for
$(D)$, $w' z' = 0$, $z' - w' = \bar{z} - \bar{w}$, and
$L(w', z') \ge L(\bar{w}, \bar{z})$, strictly if $L(\bar{w}, \bar{z})$ is
finite. Hence, when the optimal value of $(D)$ is finite, no optimal
solution of $(D)$ has both multipliers positive.*

*Proof.* Clearly $w', z' \ge 0$, and at most one of them is positive, since
$\bar{w} - \bar{z}$ and $\bar{z} - \bar{w}$ cannot both be; checking the
two cases gives $z' - w' = \bar{z} - \bar{w}$. The inner minimum of $L$
depends on $(w, z)$ only through $z - w$, and therefore it has the same value
at the two points. As for the other term, $w l - z u = w (l - u) - (z - w)
u$, whose second part is again the same at the two points. Since
$w' < \bar{w}$ (because $\bar{z} > 0$) and $l - u < 0$, one has
$w' (l - u) > \bar{w} (l - u)$. If the inner minimum is infinite the two
values are both equal to it, and otherwise $L(w', z') > L(\bar{w}, \bar{z})$.
The last claim follows, since the value of an optimal solution is finite and
cannot be improved upon. $\square$

Hence no generality is lost (at least as far as optimality is concerned) by
assuming that a `RowConstraint` holds a non-dominated pair, i.e., one with
$w z = 0$, and storing the single value

$$
d = z - w \,,
$$

i.e., the coefficient of $g(x)$ in $L$, out of which $z = \max \{ 0 , d \}$
and $w = \max \{ 0 , -d \}$ are recovered: $d > 0$ means that the multiplier
of the upper side $u$ is the positive one, $d < 0$ that it is that of the
lower side $l$. An equality ($l = u$) has a single multiplier, given by the
same formula. For a maximization problem the Lagrangian function is $\max \{\,
f(x) + w (g(x) - l) + z (u - g(x)) : x \in X \,\}$, in which the coefficient
of $g(x)$ is $w - z$; hence, the stored value is $d = w - z$, and $d > 0$
means that the lower side is the active one. When $l > u$ the problem is
empty and $(D)$ is unbounded along the direction that increases both
multipliers, which is represented, for both the minimization and the
maximization, by $d = -1$.

The convention belongs to the `RowConstraint`, and every `Solver` that writes
the dual values in it is required to follow it, whatever the sign of the
multipliers that its algorithm produces. A `:MILPSolver`, for instance,
obtains from its back-end one multiplier $\pi_i$ per row with the sign of
the LP solvers, i.e., for a minimization problem, the coefficient of
$- g_i(x)$ in the Lagrangian, and therefore it writes $- \pi_i$ in the row
([`MILPSolver.cpp:5166-5168`](https://gitlab.com/smspp/milpsolver/-/blob/develop/src/MILPSolver.cpp#L5166-5168)). In the same way, it writes
the opposite of the reduced cost of a column as the dual value of the bound
constraint of its `ColVariable`. For a dual direction (a certificate of
infeasibility), however, the values it writes for the columns are those of
the ray of the homogeneous system, in which the `Objective` has no part.

A `:Block` may expose its dual values with the signs natural to its problem,
and then it changes the sign on the way. The flow conservation
constraint of node $i$ of an `MCFBlock` is the equation
$\sum_{a : EN_a = i} x_a - \sum_{a : SN_a = i} x_a = b_i$ of
[§7.1](07-physical-abstract.md#sec-7-1). Its potentials $\pi_i$ and
reduced costs $RC_a = C_a + \pi_{SN_a} - \pi_{EN_a}$ have the signs of the
network flow literature (with them, an optimal flow has $x_a = 0$ where
$RC_a > 0$ and $x_a = u_a$ where $RC_a < 0$), and they are the opposite of
the dual values of the corresponding `Constraint`s. Hence `get_pi()` returns
minus the dual value of the flow conservation constraint of the node, and
`get_rc()` minus that of the bound constraint of the arc (or $RC_a$ computed
out of the potentials, if the bound constraints have not been generated),
while `set_pi()` and `set_rc()` change the sign as they write
([`MCFBlock.h:147-160`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L147-160)). Thus, `get_pi()` and `get_rc()` give the same
values after `MCFSolver` or `MCFLemonSolver`, which call `set_pi()` with the
potentials of their algorithms, and after a `:MILPSolver`, which writes the
dual values in the `Constraint`s. This holds provided the flow conservation
`Constraint`s have been generated, since the potentials are stored as their
dual values; otherwise `set_pi()` drops them and `get_pi()` throws
`std::logic_error`.

## 9.5 The physical Solutions of the Blocks {#sec-9-5}

A concrete `:Block` typically defines a `:Solution`, which stores the
solution in terms of its *physical* representation, usually far more
compactly than the abstract one; each of the three running examples
has one. `MCFSolution` ([`MCFBlock.h:2681`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L2681)) stores the flows of the
arcs and/or the potentials of the nodes, i.e., the natural primal and
dual data of a min-cost flow problem (the potentials with the signs of
`get_pi()`).
[`BinaryKnapsackSolution`](https://smspp.gitlab.io/smspp-project/dc/d61/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_solution.html)
stores the values of the items (as `double`s, which can be scaled and
summed), and
[`CapacitatedFacilityLocationSolution`](https://smspp.gitlab.io/smspp-project/db/d25/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_solution.html)
stores the values of the design ($y$) and transportation ($x$)
variables, with the part to keep chosen by a `Configuration`. Such a
`Solution` reads and writes the physical data of the `Block` directly,
without needing the abstract representation; this makes it the vehicle
of the change of interface discussed in
[§9.7](09-solution.md#sec-9-7).

## 9.6 Example: a CFL solution to netCDF and back {#sec-9-6}

```cpp
// cfl is a CapacitatedFacilityLocationBlock that a Solver has solved, so
// that a solution sits in it (see Recipe R3)

// a Solution holding the current solution (emptys == false), saved to a
// netCDF file as its group Solution_0
Solution * sol = cfl.get_Solution( nullptr , false );
sol->serialize( "cfl-solution.nc4" );

// later, possibly in another run: the factory constructs the right
// :Solution out of the class name stored in the file, and it is written
// back into the Block
Solution * sol2 = Solution::deserialize( "cfl-solution.nc4" );
sol2->write( &cfl );

// a convex combination of two Solution of the SAME Block
Solution * mix = sol->scale( 0.5 );
mix->sum( sol2 , 0.5 );

delete sol;
delete sol2;
delete mix;
```

Here `get_Solution()` returns a `Solution *` that is in fact a
`CapacitatedFacilityLocationSolution` (the return type is the base
one, since `Block` cannot know the classes derived from `Solution`).
The `Solution` makes the round trip through netCDF without any
reference to the `Block`, while `scale()` and `sum()` combine solutions
of the *same* `Block` only.

## 9.7 Closing the half-baked Solution {#sec-9-7}

> **Status — under development.** The arrangement described in this section
> is not settled at version 0.8.0, and code written today should not assume
> that it will persist.

In this section we close the discussion of
[§3.3](03-mental-model.md#sec-3-3) and
[§7.6](07-physical-abstract.md#sec-7-6) on the role of `Solution` with
respect to the abstract `Variable`s. The standard way for a `:Solver`
to report its result is `get_var_solution()`, which writes the
solution into the abstract `Variable`s of the `Block`. After
`compute()` returns `kOK` the optimal values are in the
`ColVariable`s, from which the user (or the `Block`) reads them, or
`Block::get_Solution()` copies them into a `Solution`. Since the
`Variable`s are the channel through which the solution travels, the
`:Block`s of the running examples construct them in `load( ... )`, even
when they are solved only by a specialized `:Solver` that reads the
physical representation and would otherwise not need them, while the
`Constraint`s and the `Objective` are built on demand. Even an `MCFBlock`
attached only to an `MCFSolver< MCFSimplex >` has its
`std::vector< ColVariable >` of size `NStaticArcs` allocated, where the
`:Solver` can put the optimal flows.

The `Solution`s of this chapter make the way out possible, and its side of the
solution is in the interface: `Solver::get_Solution( Configuration * )`
returns a `Solution` specific to the `:Block` (an `MCFSolution`, a
`BinaryKnapsackSolution`, and so on), whose ownership passes to the caller.
Its default implementation goes through the `Variable`s: it `lock()`s the
`Block`, calls `get_var_solution()`, or `get_var_direction()` if only a
direction is available, then `Block::get_Solution()`, and `unlock()`s it
([`Solver.h:1782-1801`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solver.h#L1782-1801)). A
"physical" `:Solver`, instead, re-implements it to fill the `Solution` out of
its data structures, without writing into the `Block` and hence without
`lock()`-ing it, and several `:Solver`s attached to the same `Block` can then
produce their `Solution`s in parallel; it declares this by returning `true`
from `is_get_Solution_physical()`, which the caller can query *before* the
call. This is the case of `MCFSolver`, `MCFLemonSolver`, the
`:BinaryKnapsackSolver`s, `ThermalUnitDPSolver`, `ThermalUnitExtDPSolver`,
and the `:Solver`s of `SVMBlock` (`SMOSolver`, `LIBSVMSolver` and
`LIBLINEARSolver`), and the `Solution` so obtained can be checked against
the physical data by `Block::is_sol_feasible()` and `is_sol_optimal()`
(cf. [§7.5](07-physical-abstract.md#sec-7-5)). Yet, the step that removes the
asymmetry has not happened: the `:Block`s of the running examples still
construct their `Variable`s in `load( ... )`, and a `Block` attached only to
specialized `:Solver`s does not yet do without them. The physical
`Solution`s of [§9.5](09-solution.md#sec-9-5) are designed for that step,
since each of them reads and writes the physical data without touching the
abstract representation; this is needed to solve a `Block`, and to communicate
its solution, with no abstract `Variable` at all.

A `:Solver` whose result is made of several solutions, computed one after
the other on the same `Block` (say, the mean over the simulated scenarios of
a policy), has a further way of handing them out: it calls the events of type
`eEverySolution` of `ThinComputeInterface` (whose handlers are set by
`set_event_handler()`, cf. [§6.8](06-solver.md#sec-6-8)) each time it has
written one of them in the `Block`, as `get_var_solution()` and
`get_dual_solution()` would write it. While such an event runs, no other part
of the `compute()` uses that `Block`. The handler reads the solution, together
with the data of the problem it belongs to, out of
`Solver::get_solution_Block()`, which is the `Block` of the `:Solver` unless
the `compute()` solves each problem on a copy of it, in which case it is that
copy
([`Solver.h:1925-1944`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solver.h#L1925-1944)).

Until that step is taken, code that uses SMS++ should follow the
convention of [§7.7](07-physical-abstract.md#sec-7-7): although the
abstract `Variable`s are the present channel of the solution, the
solution is best read through the methods of the `Block` or through a
`Solution` obtained by `get_Solution()` (of the `Block` or of the
`:Solver`), without assuming that the `Variable`s will remain the
channel.

## 9.8 Outline of the interface {#sec-9-8}

| Method | Purpose |
|---|---|
| `read(const Block *)` | copy the current solution of the `Block` into this `Solution` |
| `write(Block *)` | write the stored solution back into the `Block` |
| `serialize(NcGroup &)`, `serialize(filename, replace)` | save to netCDF |
| `deserialize(const NcGroup &)` | restore from netCDF |
| `Solution::deserialize(filename)`, `Solution::new_Solution(...)` | factory: construct a `:Solution` from a file, a group or a class name |
| `clone(bool empty)` | copy (or empty copy) of the `Solution` |
| `scale(double)` | scaled copy |
| `sum(const Solution *, double)` | add a scaled `Solution` to this one |
| `is_direction()`, `is_direction(bool)` | whether it holds a direction rather than a solution |
| `adapt(block, mod, dropped)`, `adapts()` | follow a `Modification` of the `Block` |
| `drop_dynamic_values(...)`, `drop_physical_values(...)` | drop the values of what a `Modification` removes (called by `adapt()`) |

On the side of the `Block` the entry point is
`Block::get_Solution( Configuration * solc , bool emptys )`
(cf. [§9.1](09-solution.md#sec-9-1)), and on the side of the `Solver` it is
`Solver::get_Solution( Configuration * solc )`
(cf. [§9.7](09-solution.md#sec-9-7)).

## 9.9 Idioms {#sec-9-9}

The intended place where to obtain a `Solution` is
`Block::get_Solution()`: the `Block` knows which `:Solution` fits it,
while the `:Solver` is merely the agent that put the values in place.
An exception is a "physical" `:Solver` (`is_get_Solution_physical()`),
whose `Solver::get_Solution()` gives the same kind of `:Solution`
without writing into the `Variable`s (cf. [§9.7](09-solution.md#sec-9-7)).

The part of the solution that a `Solution` stores is fixed when the
object is created: to store a different part one creates a different
`Solution`, with a different `Configuration`, rather than expecting to
reconfigure an existing one.

`scale()` and `sum()` are defined for solutions of one and the same `Block`
(and the result is feasible only under the hypotheses of [Lemma
9.2](09-solution.md#lem-9-2)); combining solutions of different `Block`s, even
of the same class, is outside the contract.
