# Recipe R3 — CFL three ways: cuts / MCF relaxation / Lagrangian

> **Counterpart in the source tree:**
> `tests/CapacitatedFacilityLocation/` — one executable
> (`CFL_test`) and three configuration folders, `cuts/`, `MCF/`,
> `LD/`, each holding the same set of files with different
> contents. This recipe is a reading of that tester.

## Goal

Solve the *same* Capacitated Facility Location instance in three
completely different ways — a strengthened MILP, a fast min-cost
flow relaxation, and a Lagrangian dual — **without recompiling**,
by editing only the configuration files. This is the recipe that
shows, at full strength, the SMS++ separation of *what the model
is* from *how it is solved*: one program, three solution
strategies, selected entirely by configuration.

## Concepts used

- `R3Block`: producing a relaxation and keeping it in sync —
  Chapter 10 (and `UpdateSolver`).
- `Configuration`: `BlockConfig` to pick the formulation,
  `BlockSolverConfig` to pick the solver — Chapter 11.
- Sub-`Block` decomposition (the KskForm tree) — Chapter 12.
- `LagBFunction` / `LagrangianDualSolver` — Chapter 14.

## The pattern

The driver is configuration-agnostic. In skeleton form (the
`CFL_test` of the source tree, condensed):

```cpp
// B1 is the original CFL problem, read from the instance file
auto * B1 = dynamic_cast< CapacitatedFacilityLocationBlock * >(
              Block::deserialize( instance_file ) );

// B2 is an R3Block of B1; WHICH one is read from R3BCfg.txt
Configuration * r3bc = /* deserialized from R3BCfg.txt */ ;
Block * B2 = B1->get_R3_Block( r3bc );

// configure the two Blocks (formulation, options) from BPar1/BPar2.txt
B1->set_BlockConfig( /* from BPar1.txt */ );
B2->set_BlockConfig( /* from BPar2.txt */ );

// attach a Solver to each, from BSPar1/BSPar2.txt
/* BlockSolverConfig from BSPar1.txt */ ->apply( B1 );
/* BlockSolverConfig from BSPar2.txt */ ->apply( B2 );

// keep B2 in sync with B1 automatically (Chapter 10)
B1->register_Solver( new UpdateSolver( B2 , r3bc ) );

if( niter == 0 ) {                 // just solve B2 and compare
 B2->get_registered_solvers().front()->compute();
}
else {                             // a slope-scaling loop
 for( Index h = 0 ; h < niter ; ++h ) {
  B2->get_registered_solvers().front()->compute();  // solve the relaxation
  B1->map_back_solution( B2 , r3bc );                // bring solution to B1
  // ... use the (fractional) solution to nudge B1's facility costs;
  //     each chg is forwarded to B2 by the UpdateSolver ...
 }
}
```

The whole behaviour is decided by four configuration files —
`R3BCfg.txt` (which R3Block), `BPar2.txt` (which formulation /
options for it), `BSPar2.txt` (which solver) — and it is *those*,
not the code, that differ between the three folders.

## The three configurations

| | `cuts/` | `MCF/` | `LD/` |
|---|---|---|---|
| `R3BCfg.txt` (R3Block) | `0` — a copy `CFLB` | `1` — an `MCFBlock` flow relaxation | `0` — a copy `CFLB` |
| `BPar2.txt` (formulation) | Natural (StdForm, `wf=0`) with strong forcing cuts enabled | (none — `MCFBlock` has no formulation choice) | Knapsack (KskForm, `wf=1`) |
| `BSPar2.txt` (solver) | `CPXMILPSolver` | `MCFSolver<MCFCplex>` | `LagrangianDualSolver` (inner `BundleSolver` on the per-facility knapsacks) |
| what it computes | the bound of the natural formulation strengthened by the dynamically separated $x_{ij} \le y_i$ cuts | a weak but very cheap lower bound, from the continuous flow relaxation | the Lagrangian bound (in theory, *equal* to the LP+cuts bound) and a convexified primal solution |

The three sit at different points of the bound-quality / cost
trade-off:

- **`cuts/`** — a `:MILPSolver` on the natural formulation, with
  the strong forcing constraints $x_{ij} \le y_i$ separated on
  demand (Chapter 12 mentioned this dynamic-constraint group). A
  strong bound, at the highest per-solve cost.
- **`MCF/`** — the `MCFBlock` flow relaxation of §10.5, solved by
  a network-specialised `MCFSolver`. The weakest bound and the
  most fractional solution, but obtained extremely fast.
- **`LD/`** — the Knapsack formulation solved by
  `LagrangianDualSolver`: the master's customer-satisfaction
  constraints are dualised, leaving one `LagBFunction` per
  facility (over its `BinaryKnapsackBlock` sub-`Block`), driven by
  a `BundleSolver` (Chapters 12, 14, Recipe R4). A strong bound
  and a good convexified solution, at a cost between the other
  two.

A theoretical point worth making explicit: the bound that `cuts/`
and `LD/` compute is, *in theory, exactly the same*. Dualising the
customer-satisfaction constraints leaves a per-facility knapsack
subproblem; the Lagrangian dual bound therefore equals the value
of the natural LP *strengthened by the convex hull of those
knapsacks* — which is precisely what the strong forcing cuts
$x_{ij} \le y_i$ approach. This is the standard Lagrangian /
Dantzig–Wolfe equivalence, and it means `cuts/` and `LD/` are two
routes to one and the same bound, differing in *how* they reach it
(branch-and-cut on a MILP engine versus a bundle method on the
dual), not in the bound itself. The `MCF/` flow relaxation is the
genuinely *weaker* one.

## Walk-through

- `B1` is the genuine CFL problem; `B2` is whatever `R3BCfg.txt`
  asks `get_R3_Block` to produce — *a copy of `B1`* in the `cuts/`
  and `LD/` cases (so that `B2` can be configured into a different
  *formulation* and solved without disturbing `B1`), or an
  *`MCFBlock` flow relaxation* in the `MCF/` case (§10.5).
- The two `set_BlockConfig` calls decide the *formulation*: in
  `cuts/`, `B2` is the natural MILP with the strong cuts enabled;
  in `LD/`, `B2` is the Knapsack formulation, which grows the
  per-facility `BinaryKnapsackBlock` sub-`Block` tree of Chapter
  12; in `MCF/`, `B2` is already an `MCFBlock` and needs no
  formulation choice.
- The two `BlockSolverConfig`s decide the *solver*. Switching the
  whole strategy is a matter of pointing the driver at a different
  folder.
- The `UpdateSolver` registered on `B1` is what makes the
  slope-scaling loop work: when the loop nudges `B1`'s facility
  costs, the change is `map_forward`-ed to `B2` automatically
  (Chapter 10), so the relaxation stays faithful to the (modified)
  problem without any explicit re-synchronisation.
- `map_back_solution(B2, r3bc)` brings `B2`'s (relaxation)
  solution back into `B1`, where the heuristic reads it.

## Expected behaviour

This recipe is about *qualitative* contrast, not a single number.
On a typical CFL instance one observes:

- `MCF/` returns the lowest (weakest) lower bound, fastest, with
  the most fractional transportation solution;
- `cuts/` and `LD/` return the *same* bound in theory (the
  Lagrangian / Dantzig–Wolfe equivalence above), `LD/` also
  producing a markedly less fractional convexified solution;
- both are far above the `MCF/` bound.

**On why `cuts/` and `LD/` may nonetheless print *different*
numbers.** A reader running the tester will often see the `cuts/`
bound come out slightly *better* than the `LD/` one, which seems
to contradict the equivalence just stated. The discrepancy is not
in the mathematics but in what the MIP engine does: with the
default `intRelaxIntVars == 0`, `cuts/` declares the problem to
the back-end *as a MILP*, and the back-end (CPLEX, say) applies
its MIP presolve / bound-strengthening at the root node, which can
tighten the reported bound beyond the pure LP-with-cuts value.
That extra strengthening, not a difference in the underlying
relaxation, is what makes the two numbers differ.

To recover the equivalence *exactly*, set `intRelaxIntVars == 2`
in the `CPXMILPSolver`'s `ComputeConfig` (in `cuts/BSPar2.txt`):
in that mode (`MILPSolver.h`, §6.4) the back-end solves the
problem *as a pure LP* — no MIP presolve — and `MILPSolver` itself
drives the cut-separation loop, calling
`generate_dynamic_constraints()` after each LP solve. With MIP
preprocessing thus switched off, the `cuts/` bound coincides with
the `LD/` Lagrangian bound, as the theory predicts.

The exact numbers depend on the instance and the installed MIP
back-end; the point that survives every instance is that **the
same executable produced all three** by configuration alone.

## Variations

**Swap the MIP back-end.** In `cuts/BSPar2.txt`, replacing
`CPXMILPSolver` with `SCIPMILPSolver`, `GRBMILPSolver` or
`HiGHSMILPSolver` (one line) changes the underlying MIP engine
with no other change — the open-source HiGHS being the choice
that needs no commercial licence.

**Run the slope-scaling heuristic.** Passing a non-zero `niter`
turns the "solve once and compare" path into the slope-scaling
loop sketched above: solve the relaxation, map the solution back,
adjust `B1`'s costs, repeat. With the `MCF/` or `LD/` relaxation
this produces a feasible CFL solution (an *upper* bound) to set
against the lower bound, all from the one driver.

**Go to Lagrangian + heuristic, or to Benders.** Recipe R4 takes
the `LD/` configuration further, adding `PrimalProximalHeur` to
turn the convexified solution into a feasible one; Recipe R5
solves the *same* CFL by the Benders formulation instead. Both are
reached, again, by changing configuration rather than code.
