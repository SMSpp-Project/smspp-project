# 9. Solution

[Source: `SMS++/include/Solution.h`, `ColVariableSolution.h`,
`RowConstraintSolution.h`, `ColRowSolution.h`;
`MCFBlock/include/MCFBlock.h` (`MCFSolution`)]

## 9.1 Concept

A
[`Solution`](https://smspp.gitlab.io/smspp-project/d8/d63/class_s_m_spp__di__unipi__it_1_1_solution.html)
is an object that stores a solution of a `Block` — typically the
values of the `Block`'s `Variable`s and, where applicable, the
dual values attached to its `Constraint`s — so that the solution
can be saved, restored, serialised, or combined with other
solutions. The cardinal design fact is:

> a `Solution` is **`Block`-specific**, not `Solver`-specific.

The same `Block` produces the same kind of `Solution` regardless
of which `:Solver` computed it. A min-cost flow solution is an
`MCFSolution` whether it was found by an `MCFSolver< MCFSimplex >`
or by a general-purpose `:MILPSolver`; the `Solution` knows how to
read itself out of an `MCFBlock` and write itself back into one,
and it does not care how the values got there.

The two methods at the heart of the interface are:

- `read(const Block*)` — read the current solution out of the
  given `Block` and store it inside this `Solution`;
- `write(Block*)` — write the stored solution back into the
  given `Block`.

A `Solution` is normally constructed *by* the `Block` whose
solution it stores, through `Block::get_Solution(Configuration*,
bool)`. The `Configuration*` argument lets the caller request a
*specific part* of the solution — only the primal values, only
the dual values, only a particular subset of `Variable`s — and
this choice is *permanent*: once constructed, the `Solution`
object will only ever store that part. Asking a `Solution` to
`read()` information it was not configured to hold (or that the
`Block` does not currently have, e.g. dual values when the
`Constraint`s have not been generated) is an error and throws.

## 9.2 Whose `Block` a `Solution` belongs to

A `Solution` is tightly bound to the `Block` that created it. It
is an error — and throws an exception — to `read()` or `write()`
a `Solution` against the *wrong* `Block`. "Wrong" here means more
than "wrong type": the safe contract is that a `Solution` is used
only with the very `Block` that created it, or with a `Block`
that is "identical" to it (for instance, a copy produced as an
R3 Block; Chapter 10), or, at the very least, one that is
"compatible" — same sizes in the relevant `Variable` /
`Constraint` groups.

`Solution`s are *not* meant to be exchanged between distinct
`Block`s, even of the same type. A `:Solution` class that does
allow such an exchange must say so explicitly in its own
documentation, and the exchange must be used with care.

## 9.3 Serialisation and combination

Beyond `read` / `write`, the base `Solution` interface provides:

- **Serialisation to netCDF**: `serialize(netCDF::NcGroup&)`,
  `serialize(const std::string& filename, bool replace)`, and the
  matching `deserialize(const netCDF::NcGroup&)`. A `Solution`
  can be written to disk and read back independently of its
  `Block`; the `Solution` factory
  (`Solution::new_Solution(name)`) reconstructs the correct
  `:Solution` type from the class name stored in the netCDF group,
  exactly as the `Block` and `Solver` factories do for their
  respective hierarchies (Chapter 18).
- **Cloning**: `clone(bool empty)` produces a copy of the
  `Solution` (or an empty one of the same type and configuration,
  if `empty == true`).
- **Linear combination**: `scale(double factor)` returns a scaled
  copy and `sum(const Solution*, double multiplier)` adds a scaled
  solution into this one. Together they let one form weighted sums
  — in particular *convex combinations* — of solutions of the same
  `Block`. Convex combinations are central to many optimisation
  techniques (the convexified primal solution produced by a
  Lagrangian dual, for example; Chapter 14 and Recipe R4), which
  is why the operation is part of the base interface.

A caveat applies to `scale` / `sum`: not every `Solution` can be
meaningfully scaled or summed. A *discrete* `Solution` (one whose
stored values are integral by nature) may refuse to be combined,
or may produce a "more general" `Solution` that no longer carries
the integrality. The base-class comments to `scale()` and
`sum()` (`Solution.h:417-469`) describe the contract; a
`:Solution` is free to convert itself, on demand, into a "more
general" `Solution` type when a combination would otherwise be
impossible.

## 9.4 The standard abstract `Solution`s

Three concrete `:Solution`s in the core library work off the
*abstract* representation of a `Block` and are therefore reusable
across any `:Block` whose abstract representation is of the right
shape:

- [`ColVariableSolution`](https://smspp.gitlab.io/smspp-project/da/d85/class_s_m_spp__di__unipi__it_1_1_col_variable_solution.html)
  stores the *primal* solution of any `Block` whose `Variable`s
  are all `ColVariable`s: it reads and writes their values.
- [`RowConstraintSolution`](https://smspp.gitlab.io/smspp-project/dd/d09/class_s_m_spp__di__unipi__it_1_1_row_constraint_solution.html)
  stores the *dual* solution of any `Block` whose `Constraint`s
  are all `RowConstraint`s: it reads and writes their dual
  values.
- [`ColRowSolution`](https://smspp.gitlab.io/smspp-project/de/d2a/class_s_m_spp__di__unipi__it_1_1_col_row_solution.html)
  stores both at once.

These are the "default" `Solution`s: a `Block` that has not
defined a `:Solution` of its own, but whose abstract
representation fits one of these shapes, can use them directly.

## 9.5 The `Block`-specific physical `Solution`s

A concrete `:Block` typically defines its own `:Solution` class
that stores the solution in terms of the *physical*
representation, which is usually far more compact than the
abstract one. The three running examples each ship one:

- `MCFSolution` (declared in `MCFBlock.h:2508`) stores the arc
  flows and, optionally, the node potentials and arc reduced
  costs — the natural primal and dual data of a min-cost flow
  problem.
- [`BinaryKnapsackSolution`](https://smspp.gitlab.io/smspp-project/dc/d61/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_solution.html)
  stores the per-item values and, where meaningful, the dual
  value of the knapsack constraint.
- [`CapacitatedFacilityLocationSolution`](https://smspp.gitlab.io/smspp-project/db/d25/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_solution.html)
  stores the design (`y`) and transportation (`x`) values, with a
  `Configuration`-selectable choice of which part to keep.

A `Block`-specific physical `Solution` reads and writes the
`Block`'s physical data directly, without needing the abstract
representation to have been constructed. This is the property
that makes it the right vehicle for the API change discussed in
§9.7.

## 9.6 Inline example: a CFL solution to netCDF and back

```cpp
#include "CapacitatedFacilityLocationBlock.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 CapacitatedFacilityLocationBlock cfl;
 cfl.load( /* ... a CFL instance ... */ );

 // solve cfl with some Solver (omitted; see Recipe R3) so that a
 // solution sits in cfl ...

 // ask the Block for a Solution object holding the current solution
 Solution * sol = cfl.get_Solution();   // a CapacitatedFacilityLocationSolution

 // persist it to a netCDF file
 sol->serialize( "cfl-solution.nc4" );

 // ... later, possibly in another run ...
 Solution * sol2 = cfl.get_Solution( nullptr , /* emptys = */ true );
 sol2->deserialize( /* the NcGroup read from "cfl-solution.nc4" */ );

 // restore the stored solution into the Block
 sol2->write( & cfl );

 // a convex combination of two solutions of the SAME Block
 // (here: 0.5 * sol + 0.5 * sol2), useful e.g. for rounding heuristics
 Solution * mix = sol->scale( 0.5 );
 mix->sum( sol2 , 0.5 );

 delete sol; delete sol2; delete mix;
 return 0;
}
```

The points to take away: `get_Solution()` returns a `Solution*`
that is in fact a `CapacitatedFacilityLocationSolution` (the
return type is the base `Solution*` for the usual reason that the
`:Solution` cannot be forward-declared at that point); the
`Solution` round-trips through netCDF without any reference to
the `Block`; and `scale` / `sum` combine solutions of the *same*
`Block` — never of different ones.

## 9.7 Status — under development: closing the half-baked `Solution`

This chapter is the natural home of the discussion opened in §3.3
and continued in §7.6: the *intended* role of `Solution` in
relation to the abstract `Variable`s.

The relevant facts, restated here in full:

- Every `:Solver`, today, reports its result by *writing the
  solution into the abstract `Variable`s* of the `Block`. After
  `compute()` returns `kOK`, the optimal values sit in the
  `ColVariable`s; the user (or the `Block`) reads them from there,
  or calls `get_Solution()` to snapshot them into a `Solution`
  object.
- Because the abstract `Variable`s are the channel through which
  the solution is communicated, they must *always* be
  materialised — even for a `Block` solved exclusively by a
  specialised `:Solver` that reads only the physical
  representation and would otherwise have no need for them. This
  is the asymmetry flagged in §7.6: `Constraint`s and `Objective`
  are genuinely on demand, but `Variable`s are not.

The `Solution` machinery of this chapter is precisely what makes
the *intended* fix possible. Under the planned revision of the
API, a `:Solver` would, at the end of `compute()`, hand back a
`:Block`-specific `Solution` object — an `MCFSolution`, a
`BinaryKnapsackSolution`, ... — and the `Block` would *adopt* it,
absorbing the solution directly into its physical representation.
The abstract `Variable`s would then no longer be the obligatory
communication channel; a `Block` attached to a specialised
`:Solver` only would be able to skip
`generate_abstract_variables()` entirely, holding only the
physical data and the `:Solution` that carries the result.

The `Block`-specific physical `Solution`s of §9.5 are designed
with exactly this in mind: each can read and write the `Block`'s
physical data without touching the abstract representation, which
is the prerequisite for letting a `Block` be solved, and its
solution communicated, with no abstract `Variable`s in sight.

Until that revision lands, the convention to follow in user code
is the one stated in §7.7: treat the abstract `Variable`s as the
present-day solution channel, read the solution back through the
`Block`'s physical accessors or through a `Solution` object
obtained from `get_Solution()`, and do not write code that
assumes the `Variable`s-as-channel arrangement will persist. This
remains one of the clearest signs that the SMS++ public interface
is not yet fully settled at version 0.6.0. **Status — under
development.**

## 9.8 API outline

| Method | Purpose |
|---|---|
| `read(const Block*)` | snapshot the current solution of the `Block` into this `Solution` |
| `write(Block*)` | write the stored solution back into the `Block` |
| `serialize(NcGroup&)` / `serialize(filename, replace)` | persist to netCDF |
| `deserialize(NcGroup&)` | restore from netCDF |
| `clone(bool empty)` | copy (or empty copy) of the `Solution` |
| `scale(double)` | scaled copy |
| `sum(const Solution*, double)` | add a scaled `Solution` into this one |
| `Solution::new_Solution(name)` | factory: construct a `:Solution` by class name |

The `Block`-side entry point is `Block::get_Solution(Configuration*
solc, bool emptys)`: `solc` selects which part of the solution to
keep (permanently), and `emptys == true` prepares the object
without immediately reading a solution.

## 9.9 Idioms

**Get the `Solution` from the `Block`, not from the `Solver`.**
The canonical place to obtain a `Solution` is
`Block::get_Solution()`. The `Block` knows which `:Solution` type
fits it; the `:Solver` is merely the agent that put the values in
place.

**Configure once, at construction.** The part of the solution a
`Solution` stores is fixed when the object is created. To store a
different part, create a different `Solution` object with a
different `Configuration`; do not expect to reconfigure an
existing one.

**Combine only solutions of the same `Block`.** `scale` and `sum`
are defined for solutions of one and the same `Block`. Combining
solutions of different `Block`s — even of the same type — is
outside the contract and will, at best, throw.
