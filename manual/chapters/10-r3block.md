# 10. R3Block: Reformulation, Relaxation, Restriction

[Source: `SMS++/include/Block.h` (R3 Block methods),
`UpdateSolver.h`;
`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`]

## 10.1 Concept

A central design objective of SMS++ is that a model may need to be
*transformed* for algorithmic reasons, producing a different
`Block` that is related to the original in one of three ways
([Source: `Block.h:67-85`]):

- a **Reformulation** — a different `Block` that encodes a problem
  whose optimal solutions are optimal also for the original
  `Block` (the two are equivalent, but one may be easier to work
  with);
- a **Relaxation** — a different `Block` whose optimal value
  provides a valid global lower bound (for a minimisation problem;
  upper bound for maximisation) on the optimal value of the
  original, while hopefully being easier to solve;
- a **Restriction** — a different `Block` whose feasible region
  is a strict subset of that of the original, which again may make
  it easier to solve (at the price of possibly cutting off the
  true optimum).

The three transformations are collectively called the **R3 Block**
of the original `Block` (Reformulation / Relaxation /
Restriction). The set of R3 Blocks a given `Block` supports is
decided by the `Block` itself; the base `Block` class provides no
general R3 Block. A concrete `:Block` that wishes to support one
must implement it explicitly — there is no automatic mechanism
("forget 'auto' here", as the slide decks put it).

## 10.2 Why an R3 Block, and not a copy of the `Variable`s

The need for a dedicated mechanism, rather than "just copy the
relevant `Variable`s and `Constraint`s", comes straight from the
identity rule of §4.2: the *name* of a `Variable` is its memory
address, so a copy of a `Variable` is, necessarily, a *different*
`Variable`. An R3 Block therefore lives in its own, fresh set of
`Variable`s and `Constraint`s, entirely disjoint from those of the
original. This is exactly what makes an R3 Block useful for
algorithmic purposes (branch, fix, relax, solve a separate copy in
a separate thread, ...): it is genuinely independent, and operating
on it does not perturb the original.

But independence creates a problem: a solution found on the R3
Block lives in the R3 Block's `Variable`s, and is of no direct use
to a `:Solver` working on the original. The framework's answer is
a small family of *mapping* methods that move solution and
modification information back and forth between an original `Block`
and one of its R3 Blocks.

## 10.3 The R3 Block API

All of the following are virtual methods of `Block`; a concrete
`:Block` overrides the ones it chooses to support and leaves the
rest to throw.

- `get_R3_Block(Configuration* r3bc, Block* base, Block* father)`
  produces an R3 Block. The `Configuration*` selects *which* R3
  Block (a `:Block` may offer several); the `base` and `father`
  parameters are discussed in detail below.
- `map_back_solution(Block* R3B, Configuration* r3bc,
  Configuration* solc)` takes the solution currently held in the
  R3 Block `R3B` and writes the corresponding solution into the
  original `Block`. The `solc` selects which part of the solution
  to map (primal, dual, both).
- `map_forward_solution(Block* R3B, ...)` is the reverse: it takes
  the solution of the original `Block` and writes it into the R3
  Block.
- `map_forward_Modification(Block* R3B, c_p_Mod mod, ...)` takes a
  `Modification` that occurred on the original `Block` and applies
  the corresponding change to the R3 Block, so that the two stay
  in sync. `map_forward_Modifications(...)` does the same for a
  whole list.
- `map_back_Modification(Block* R3B, c_p_Mod mod, ...)` is the
  reverse direction.

In every case the `Configuration* r3bc` passed to a mapping method
must be the *same* (or an identical) `Configuration` that was used
to produce `R3B` in `get_R3_Block`, so that the `Block` knows
which kind of R3 Block it is dealing with.

A `:Block` is, as always, free to support only part of this
interface. The base-class default `get_R3_Block` (with a null
`Configuration`) recursively produces a copy by asking each
sub-`Block` for its own copy, but a leaf `:Block` must implement
even the copy itself.

### The `base` and `father` parameters

The two trailing parameters of `get_R3_Block` exist to make the
construction of an R3 Block composable along a class hierarchy and
along a `Block` tree.

`base` lets the construction happen *piecemeal*. Consider a class
`B1 : Block` that already provides some R3 Block — say, the copy —
and a derived class `B2 : B1`. It is sensible for `B2` to want to
offer the same R3 Block, and wasteful for it to re-implement the
part that `B1` already knows how to handle. The pattern the `base`
parameter supports is: `B2::get_R3_Block` allocates the R3 Block
(an object of class `B2`, in the copy case), then calls
`B1::get_R3_Block(r3bc, that_object)` to have the `B1`-specific
part of the new object managed (copied) by `B1`, while `B2` takes
care of only the `B2`-specific part. In other words, `base` is "an
existing object that the method should populate, rather than
allocate from scratch". A `:Block` that accepts a `base` will have
requirements on its concrete type, depending on `r3bc`: for the
copy, `base` must be of a class derived from the one whose
`get_R3_Block` is being called (any `B1`-or-derived for
`B1::get_R3_Block`, so a `B2` qualifies).

`father` is used in the converse situation, when `base` is *not*
supplied and the R3 Block therefore has to be allocated inside the
method. In that case `father` is the pointer to the father `Block`
of the newly created R3 Block, to be passed to its constructor so
that the R3 Block is correctly nested in the surrounding tree.
(`Block` is abstract and cannot itself be instantiated, which is
why the base-class default implementation requires a non-null
`base` whenever the `Block` has inner `Block`s: it has no way to
allocate the right concrete type otherwise; `Block.h:4761-4775`.)

### Dynamic `Variable`s and `Constraint`s in an R3 Block

A subtlety worth stating explicitly concerns *dynamic*
`Constraint`s and `Variable`s (§4.3), because their semantics
interacts with R3 Blocks in a way that is easy to get wrong.

A dynamic `Constraint` is conceptually "there even when it is not
there": it contributes to defining the feasible region of the
`Block`, so *every* feasible solution must satisfy *all* of them,
whether or not they have been explicitly constructed. The reason
not all of them are materialised is purely operational — there can
be very many, and only the subset that is useful for algorithmic
reasons (the ones a separation procedure has found to be violated,
say) is explicitly generated. Likewise, a dynamic `Variable` is
"there even when it is not there": every dynamic `Variable` that
has not been explicitly generated is assumed to be present in the
`Block` at its default value (most often zero), under the
assumption that this does not invalidate the feasibility of the
explicitly-constructed part of the solution. Only the dynamic
`Variable`s actually needed to encode the optimal solution need be
materialised.

The consequence for R3 Blocks is that generating dynamic
`Variable`s/`Constraint`s may require complex separation or
pricing procedures. These are surely available to the *original*
`Block`, but may or may not be available to the *R3* `Block`. If
they are, the R3 Block may generate dynamic stuff independently
and have it "imported back" into the original via
`map_back_Modification` (`Block.h:4750-4759`). If they are not,
dynamic `Variable`s/`Constraint`s can only be generated on the
original `Block` and then, where supported, pushed forward to the
R3 Block via `map_forward_Modification`. A `:Block` author who
provides an R3 Block over a model with dynamic components must
decide, and document, which of the two directions its R3 Block
supports — and `map_back_solution` is, accordingly, allowed to do
a *best-effort* job when the R3 Block holds dynamic components the
original has not (yet) generated.

## 10.4 The trivial case: the copy

The simplest R3 Block is the **copy** (clone): a new `Block` of
the same type, holding the same data, in its own fresh
`Variable`s and `Constraint`s. The copy is a Reformulation in the
trivial sense (it encodes exactly the same problem) and "should
always work" for any reasonable `:Block`. Even so, the copy is not
free: because of the identity rule, the copy's `Variable`s are
distinct objects, and `map_back_solution` from the copy to the
original is a genuine value-by-value transfer, not a pointer
aliasing.

For `MCFBlock`, `get_R3_Block(nullptr)` produces a copy
(`MCFBlock.h:1096-1105`); `map_back_solution` / `map_forward_solution`
move the flows, potentials and reduced costs between the two,
with a `solc` selecting "primal only" / "dual only" / "both".

## 10.5 The non-trivial case: CFL's MCF flow relaxation

The instructive R3 Block is the one offered by
`CapacitatedFacilityLocationBlock`: besides the copy, it can
produce a **`MCFBlock` that represents the continuous flow
relaxation** of the CFL problem
(`CapacitatedFacilityLocationBlock.h:1048-1099`). The
`Configuration*` selects it:

- `SimpleConfiguration<int>(0)` (or `nullptr`): the copy, another
  `CapacitatedFacilityLocationBlock`;
- `SimpleConfiguration<int>(1)`: an `MCFBlock` flow relaxation;
- `SimpleConfiguration<int>(2)`: the same flow relaxation, but
  with extra "artificial" arcs that keep it feasible for every
  facility-opening pattern.

The flow relaxation is a genuine *Relaxation* in the R3 sense: its
optimal value is a valid lower bound on the CFL optimum, and it is
much cheaper to solve (a min-cost flow rather than a MILP). The
graph it builds has `f_n_facilities + f_n_customers + 1` nodes —
one per facility, one per customer, plus a super-source — and
`f_n_facilities * (f_n_customers + 1)` arcs: a "facility arc" from
the super-source to each facility (capacity = facility capacity,
cost = fixed cost / capacity) and a "transportation arc" from each
facility to each customer (capacity infinite, cost = unitary
transportation cost). When the `(2)` variant is requested, an
extra arc from the super-source to each customer, with very large
cost, absorbs any unmet demand and so guarantees feasibility.

This same construction does double duty in the framework: it is
the user-visible R3 Block described here, *and* it is the
internal engine of the "Benders friendly" formulation of CFL
(BenForm, §3.5), where the `MCFBlock` is wrapped in a
`BendersBFunction` to produce Benders cuts (Chapter 14, Recipe
R5). A single, carefully written `get_R3_Block` thus pays off
twice.

`CapacitatedFacilityLocationBlock` implements
`map_back_solution`, `map_forward_solution` and
`map_forward_Modification` for both the copy and the MCF R3 Block
(with the documented exception that `map_back_Modification` to the
MCF relaxation is not implemented — the relaxation is a one-way
target in that respect).

## 10.6 Keeping an R3 Block in sync: `UpdateSolver`

When an algorithm holds both an original `Block` and one of its R3
Blocks and changes the original, the R3 Block must be updated to
match. Doing this by hand — intercepting every `Modification` on
the original and calling `map_forward_Modification` — is
mechanical and error-prone, so the framework packages it as a
`:Solver`:
[`UpdateSolver`](https://smspp.gitlab.io/smspp-project/d4/d0f/class_s_m_spp__di__unipi__it_1_1_update_solver.html).

`UpdateSolver` is a `Solver` whose only job is to forward
`Modification`s. Registered on the original `Block` with a pointer
to the R3 Block, it `map_forward`s every `Modification` it
receives to the R3 Block; registered on the R3 Block with a
pointer to the original, it `map_back`s them instead. Its
constructor takes the R3 Block, the `Configuration` used to
produce it, an options bit-mask (forward vs back; map vs pass
through unchanged; restrict to `Modification`s concerning the
`Block` itself vs its sub-`Block`s; restrict by
`concerns_Block()`), and the `issuePMod` / `issueAMod` values to
pass on to the mapping methods (`UpdateSolver.h:79-129`).

The canonical idiom — used verbatim in the CFL test (Recipe R3) —
is: build the original `Block`, produce its R3 Block via
`get_R3_Block`, register an `UpdateSolver` on the original
pointing at the R3 Block, and from then on simply mutate the
original; the R3 Block follows along automatically. The
`UpdateSolver` is, in effect, the live wire that keeps a
relaxation faithful to the model it relaxes while the model
changes underneath it (as in a slope-scaling or
branch-and-bound loop).

## 10.7 Inline example: a CFL flow relaxation kept in sync

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include "MCFBlock.h"
#include "MCFSolver.h"
#include "MCFSimplex.h"
#include "UpdateSolver.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 CapacitatedFacilityLocationBlock cfl;
 cfl.load( /* ... a CFL instance ... */ );

 // produce the MCF flow relaxation (R3 Block, "feasible" variant)
 auto r3bc = SimpleConfiguration< int >( 2 );
 Block * R3B = cfl.get_R3_Block( & r3bc );    // an MCFBlock
 auto * mcf  = dynamic_cast< MCFBlock * >( R3B );

 // attach a fast MCF Solver to the relaxation
 auto * mcfs = new MCFSolver< MCFSimplex >();
 mcf->register_Solver( mcfs );

 // keep the relaxation in sync with cfl automatically: every
 // Modification on cfl is map_forward-ed to the MCFBlock
 auto * upd = new UpdateSolver( R3B , & r3bc );
 cfl.register_Solver( upd );

 // ---- a slope-scaling-style iteration ----
 for( int it = 0 ; it < max_iters ; ++it ) {
  mcfs->compute();                       // solve the (cheap) relaxation
  cfl.map_back_solution( R3B , & r3bc ); // bring its solution into cfl
  // ... use the (fractional) relaxation solution to adjust cfl's
  //     facility costs (slope scaling); each chg_cost on cfl is
  //     automatically forwarded to the MCFBlock by `upd` ...
 }

 cfl.unregister_Solver( upd , true );
 mcf->unregister_Solver( mcfs , true );
 delete R3B;
 return 0;
}
```

The salient points: `get_R3_Block` returns an independent
`MCFBlock` (its `Variable`s are not those of `cfl`);
`map_back_solution` is what brings the relaxation's solution into
`cfl`; and the `UpdateSolver` removes the need to manually
forward each cost change — a `chg_cost` on `cfl` reaches the
`MCFBlock` through `map_forward_Modification` without any explicit
call. Recipe R3 develops this into a complete, runnable program
and compares it with the alternative of using a
`CapacitatedFacilityLocationBlock` R3 Block solved by a
`:MILPSolver` or by a `LagrangianDualSolver`.

## 10.8 Idioms

**Pass the same `Configuration` to `get_R3_Block` and to every
mapping call.** The `Block` uses `r3bc` to know which R3 Block it
is dealing with; using a different `r3bc` in `map_back_solution`
than in `get_R3_Block` is an error.

**Use `UpdateSolver` rather than hand-forwarding.** Whenever an R3
Block must track changes to its original (or vice versa),
register an `UpdateSolver` instead of intercepting
`Modification`s by hand. It is less code and it gets the
`map_forward` / `map_back` direction, the channel handling, and
the `concerns_Block()` filtering right.

**Remember that R3 Blocks are independent objects.** An R3 Block
has its own `Variable`s, its own `Constraint`s, its own
`Solver`s, and its own lifetime. It must be destroyed explicitly
(unless it was created with a `father` that owns it), and a
solution read off it means nothing to the original until it has
been `map_back`-ed.

**A Relaxation is a lower/upper bound, not the answer.** The CFL
MCF relaxation gives a bound and a (typically fractional)
solution, fast; it does not give the CFL optimum. Mapping its
solution back is the *start* of a rounding or branching
procedure, not the end of one.
