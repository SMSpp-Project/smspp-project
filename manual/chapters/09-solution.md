# 9. Solution {#ch-9}

[Source: `SMS++/include/Solution.h`, `ColVariableSolution.h`,
`RowConstraintSolution.h`, `ColRowSolution.h`;
`MCFBlock/include/MCFBlock.h` (`MCFSolution`)]

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
stored, primal and dual. This choice is permanent: the `Solution` will
only ever store that part. With `emptys == true` (the default) the
`Solution` is returned empty, to be filled by a later `read()`, while
with `emptys == false` it already holds the current solution. The base
`Block::get_Solution()` returns `nullptr`, i.e., a `:Block` that does
not override it has no `Solution`. By the contract of `read()` and
`write()` in `Solution.h`, it is an error to read or write a part that
the `Block` does not have (say, dual values when the `Constraint`s
have not been generated). Concrete `:Solution`s throw in this case,
with the exception noted in [§9.4](09-solution.md#sec-9-4).

## 9.2 The Block of a Solution {#sec-9-2}

A `Solution` is bound to the `Block` that created it: unless the
specific `:Solution` declares otherwise, it can read and write the
solution of *that very* `Block` only, not even of another `Block` of
the same class (as the comment of `Block::get_Solution()` states). A
`:Solution` that allows the exchange between distinct `Block`s (for
instance, between a `Block` and a copy of it produced as an `R3Block`,
see [Chapter 10](10-r3block.md#ch-10)) must state it in its
documentation, and the exchange is then to be used with care.

The property on which the uses of a `Solution` rest is the following
contract, which all the `:Solution`s are required to satisfy, and
which Lemma 7.2 uses.

*Round trip (R).* Let $c$ be returned by `B.get_Solution( solc ,
false )` (or emptied and then filled by `c.read( B )`). After any
sequence of writes of values into the `Variable`s and the duals of
$B$ that adds or removes none of them, `c.write( B )` gives each
value of the part selected by `solc` back the value it had when $c$
was filled.

For the abstract `Solution`s of the core we can prove that the
contract is a consequence of the code.

**Lemma 9.1** (Round trip of `ColVariableSolution`). *A
`ColVariableSolution` satisfies (R) for the values of the
`ColVariable`s, provided that the `Block` and its sub-`Block`s have
the same groups of `Variable`s, with the same sizes, at the time of
`read()` and of `write()`.*

*Proof.* `read()` goes through the static groups of `Variable`s of the
`Block` with `for_each_as< ColVariable >()`, storing `get_value()` of
each element in the order of the storage, then through the dynamic
groups in the same way, then recursively through the sub-`Block`s
(`SMS++/src/ColVariableSolution.cpp:370-418`). `write()` goes through
the same groups in the same order, and calls `set_value()` with the
stored value of each element
(`SMS++/src/ColVariableSolution.cpp:420-441`). Since the order of
`for_each_as()` is the order of the storage, which is part of the
contract of `BaseGroup` (cf.
[§5.5](05-variable-constraint-objective.md#sec-5-5)), and the groups
have not changed, the $k$-th value written goes to the `ColVariable`
from which the $k$-th value was read. `set_value()` assigns the value,
unless the `ColVariable` is fixed, in which case it keeps the value it
is fixed at if the new one is equal to it within a relative $10^{-6}$,
and throws `std::domain_error` otherwise. A `ColVariable` that is
fixed now was fixed when it was read, since the writes of values in
(R) do not fix or unfix anything; also, its value cannot have changed
in between, since a fixed `ColVariable` does not accept a different
value. Hence the stored value is its current one, no exception is
thrown, and each `ColVariable` gets back the value it had. $\square$

## 9.3 Serialization and combination {#sec-9-3}

Beyond `read()` and `write()`, the base interface of `Solution`
provides the following.

Serialization to netCDF: `serialize( netCDF::NcGroup & )`, `serialize(
const std::string & filename , bool replace = true )` (which writes
the `Solution` as the group `Solution_<k>` of the file, replacing the
file or adding to it), and the corresponding `deserialize( const
netCDF::NcGroup & )`. A `Solution` can thus be written to disk and
read back independently of its `Block`. Also, the static methods
`Solution::deserialize( const std::string & filename )` and
`Solution::new_Solution( const netCDF::NcGroup & )` construct the
right `:Solution` out of the name of the class stored in the group,
through the factory of `Solution`s, as the factories of `Block`s and
`Solver`s do for their hierarchies (cf. [Chapter
18](18-factories-netcdf.md#ch-18)).

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

**Lemma 9.2** (Convex combinations of feasible solutions). *Let the
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
components), and a "rounding" heuristic then has to repair it. Also,
some `Solution`s cannot be meaningfully scaled or summed. A `Solution`
whose values are combinatorial by nature (say, a path stored as a
predecessor function) cannot represent a scaled path; `scale()` may
then return a `Solution` of a *different*, "more general" type (say, a
flow on each arc), which one can scale and sum with others. For the
contract, see the comments of `scale()` and `sum()`
(`Solution.h:540-626`).

Two further parts of the base interface concern a `Solution` that is
kept while its `Block` changes. First, a `Solution` knows whether it
holds a solution or a *direction*, i.e., a ray of the feasible region
along which the `Objective` is unbounded (`is_direction()`, set by
`read()` from `Block::is_direction()`, or by whoever fills it).
`clone()`, `scale()` and the serialization keep the flag, while a
`sum()` is a direction only if all its terms are. Second, `adapt(
block , mod , dropped )` informs the `Solution` that its `Block` has
undergone the `Modification` `mod` (cf. [Chapter
8](08-modification-janus.md#ch-8)). The answer is `kUnchanged` (the
`Solution` still fits), `kAdapted` (the values of the removed dynamic
`Variable`s or `Constraint`s, or of what a physical `Modification`
removes, have been dropped, and written in `dropped`) or `kInvalid`
(the `Solution` does not fit the `Block` any more, as after an
`NModification`); `adapts()` returns the kinds of `Modification` that
a `:Solution` has to look at. Whether the values left are still
feasible is not the business of `adapt()`, but of
`Block::is_sol_feasible()` (cf.
[§7.5](07-physical-abstract.md#sec-7-5)). Both global pools of
`LagBFunction` and `BendersBFunction` use this to keep the `Solution`s
they store in step with their inner `Block`.

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
representation has one of these forms, can use them directly. Unlike
what the contract of [§9.1](09-solution.md#sec-9-1) would suggest,
`ColVariableSolution::read()` does not throw when the groups of the
`Block` differ from those it holds: it re-initializes on the structure
of the `Block` and reads it, while `write()` throws `std::logic_error`
on a mismatch.

## 9.5 The physical Solutions of the Blocks {#sec-9-5}

A concrete `:Block` typically defines a `:Solution`, which stores the
solution in terms of its *physical* representation, usually far more
compactly than the abstract one; each of the three running examples
has one. `MCFSolution` (`MCFBlock.h:2682`) stores the flows of the
arcs and/or the potentials of the nodes, i.e., the natural primal and
dual data of a min-cost flow problem.
[`BinaryKnapsackSolution`](https://smspp.gitlab.io/smspp-project/dc/d61/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_solution.html)
stores the values of the items (as `double`s, which can be scaled and
summed), and
[`CapacitatedFacilityLocationSolution`](https://smspp.gitlab.io/smspp-project/db/d25/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_solution.html)
stores the values of the design ($y$) and transportation ($x$)
variables, the part to keep being chosen by a `Configuration`. Such a
`Solution` reads and writes the physical data of the `Block` directly,
without needing the abstract representation; this makes it the vehicle
of the change of interface discussed in
[§9.7](09-solution.md#sec-9-7).

## 9.6 Example: a CFL solution to netCDF and back {#sec-9-6}

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include <fstream>

using namespace SMSpp_di_unipi_it;

int main( int argc , char ** argv )
{
 CapacitatedFacilityLocationBlock cfl;
 std::ifstream in( argv[ 1 ] );        // a CFL instance in text format
 cfl.load( in );

 // ... solve cfl with some Solver (see Recipe R3), so that a
 // solution sits in it ...

 // a Solution holding the current solution (emptys == false)
 Solution * sol = cfl.get_Solution( nullptr , false );

 // save it to a netCDF file, as its group Solution_0
 sol->serialize( "cfl-solution.nc4" );

 // ... later, possibly in another run: the factory constructs the
 // right :Solution out of the class name stored in the file
 Solution * sol2 = Solution::deserialize( "cfl-solution.nc4" );

 // put the stored solution back into the Block
 sol2->write( & cfl );

 // a convex combination of two Solutions of the SAME Block
 Solution * mix = sol->scale( 0.5 );
 mix->sum( sol2 , 0.5 );

 delete sol;
 delete sol2;
 delete mix;
 return( 0 );
}
```

Here `get_Solution()` returns a `Solution *` that is in fact a
`CapacitatedFacilityLocationSolution` (the return type is the base
one, since `Block` cannot know the classes derived from `Solution`).
The `Solution` makes the round trip through netCDF without any
reference to the `Block`, and `scale()` and `sum()` combine solutions
of the *same* `Block`, never of different ones.

## 9.7 Status — under development: closing the half-baked Solution {#sec-9-7}

In this chapter we close the discussion of
[§3.3](03-mental-model.md#sec-3-3) and
[§7.6](07-physical-abstract.md#sec-7-6) on the role of `Solution` with
respect to the abstract `Variable`s. The standard way for a `:Solver`
to report its result is `get_var_solution()`, which writes the
solution into the abstract `Variable`s of the `Block`. After
`compute()` returns `kOK` the optimal values are in the
`ColVariable`s, from which the user (or the `Block`) reads them, or
`Block::get_Solution()` copies them into a `Solution`. Since the
`Variable`s are the channel through which the solution travels, the
present `:Block`s always construct them, even when they are solved
only by a specialized `:Solver` that reads the physical representation
and would otherwise not need them; the `Constraint`s and the
`Objective` are built on demand, the `Variable`s are not.

The `Solution`s of this chapter make the way out possible, and its
side of the solution is in the interface: `Solver::get_Solution(
Configuration * )` returns a `Solution` specific to the `:Block` (an
`MCFSolution`, a `BinaryKnapsackSolution`, and so on), whose ownership
passes to the caller. Its default implementation goes through the
`Variable`s (it `lock()`s the `Block`, calls `get_var_solution()` and
then `Block::get_Solution()`). A "physical" `:Solver`, instead,
re-implements it to fill the `Solution` out of its data structures,
without writing into the `Block` and hence without `lock()`-ing it,
and several `:Solver`s attached to the same `Block` can then produce
their `Solution`s in parallel; it declares this by returning `true`
from `is_get_Solution_physical()`, which the caller can query *before*
the call. `MCFSolver`, `MCFLemonSolver`, the `:Solver`s of
`BinaryKnapsackBlock` and `ThermalUnitDPSolver`, among others, do
this, and the `Solution` so obtained can be checked against the
physical data by `Block::is_sol_feasible()` and `is_sol_optimal()`
(cf. [§7.5](07-physical-abstract.md#sec-7-5)). Yet, the step that
removes the asymmetry has not happened: the `:Block`s of the running
examples still construct their `Variable`s in `load(...)`, and a
`Block` attached only to specialized `:Solver`s does not yet do
without them. Indeed, the physical `Solution`s of
[§9.5](09-solution.md#sec-9-5) are designed for that step, since each
of them reads and writes the physical data without touching the
abstract representation; this is needed to solve a `Block`, and to
communicate its solution, with no abstract `Variable` at all.

Until that step is taken, code that uses SMS++ should follow the
convention of [§7.7](07-physical-abstract.md#sec-7-7). The abstract
`Variable`s are the present channel of the solution, but the solution
is best read through the methods of the `Block` or through a
`Solution` obtained by `get_Solution()` (of the `Block` or of the
`:Solver`), without assuming that the `Variable`s will remain the
channel. **Status — under development.**

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

On the side of the `Block` the entry point is `Block::get_Solution(
Configuration * solc , bool emptys )` (cf. [§9.1](09-solution.md#sec-9-1)),
and on the side of the `Solver` it is `Solver::get_Solution(
Configuration * solc )` (cf. [§9.7](09-solution.md#sec-9-7)).

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

`scale()` and `sum()` are defined for solutions of one and the same
`Block` (and the result is feasible only under the hypotheses of
Lemma 9.2); combining solutions of different `Block`s, even of the same
class, is outside the contract.
