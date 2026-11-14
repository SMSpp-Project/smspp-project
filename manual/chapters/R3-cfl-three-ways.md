# [Recipe R3](R3-cfl-three-ways.md#rec-R3) — CFL three ways: cuts / MCF relaxation / Lagrangian {#rec-R3}

> **Counterpart in the source tree:**
> `tests/CapacitatedFacilityLocationBlock/` — one executable
> (`CFL_test`) and, in the same directory, one list of
> formulations per arrangement, given with `-F`
> (`CFLBForm-cuts.txt`, `CFLBForm.txt`, `CFLBForm-ld.txt`, below
> the `cuts`, `MCF` and `LD` arrangements), which names the
> configuration files of each `Block`. This recipe is a reading of
> that tester.

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
  [Chapter 10](10-r3block.md#ch-10) (and `UpdateSolver`).
- `Configuration`: `BlockConfig` to pick the formulation,
  `BlockSolverConfig` to pick the solver — [Chapter 11](11-configuration.md#ch-11).
- Sub-`Block` decomposition (the KskForm tree) — [Chapter 12](12-sub-block.md#ch-12).
- `LagBFunction` / `LagrangianDualSolver` — [Chapter 14](14-lag-benders-bfunction.md#ch-14).

## The pattern

The driver is configuration-agnostic. In skeleton form (the
`CFL_test` of the source tree, condensed):

```cpp
// B1 is the original CFL problem, read from the instance file
auto * B1 = dynamic_cast< CapacitatedFacilityLocationBlock * >(
              Block::deserialize( instance_file ) );

// the list of formulations (-F): for B1 a BlockConfig and a
// BlockSolverConfig, for its R3Block the Configuration it is made
// with (R3BCfg-*.txt), a BlockConfig and a BlockSolverConfig

// B2 is an R3Block of B1; WHICH one is read from R3BCfg-*.txt
Configuration * r3bc = /* deserialized from R3BCfg-*.txt */ ;
Block * B2 = B1->get_R3_Block( r3bc );

// configure the two Blocks (formulation, options): CFLBCfg-*.txt
B1->set_BlockConfig( /* BlockConfig of B1 */ );
B2->set_BlockConfig( /* BlockConfig of B2 */ );

// attach a Solver to each: CFLBSCfg-*.txt, MCFBSCfg*.txt
/* BlockSolverConfig of B1 */ ->apply( B1 );
/* BlockSolverConfig of B2 */ ->apply( B2 );

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

The whole behaviour is decided by the list of formulations and the
five configuration files it names — the `BlockConfig` and the
`BlockSolverConfig` of `B1`, and for `B2` the `R3BCfg-*.txt` (which
R3Block), its `BlockConfig` (which formulation / options) and its
`BlockSolverConfig` (which solver) — and it is *those*, not the
code, that differ between the three arrangements. The list may
also name several R3Blocks, each compared with `B1`.

## The three configurations

| | `cuts` | `MCF` | `LD` |
|---|---|---|---|
| list of formulations (`-F`) | `CFLBForm-cuts.txt` | `CFLBForm.txt` | `CFLBForm-ld.txt` |
| `B1`: formulation / solver | `CFLBCfg-NF-SFsep.txt` / `CFLBSCfg-cuts.txt` | `CFLBCfg-NF.txt` / `CFLBSCfg-LP.txt` | `CFLBCfg-NF-SF.txt` / `CFLBSCfg-ld.txt` |
| `B2`: R3Block | `R3BCfg-copy.txt`, `0` — a copy `CFLB` | `R3BCfg-MCF.txt`, `1` — an `MCFBlock` flow relaxation | `R3BCfg-copy.txt`, `0` — a copy `CFLB` |
| `B2`: formulation | `CFLBCfg-NF-noSF.txt`: Natural (StdForm, `wf=0`) | `MCFBCfg.txt` (`MCFBlock` has no formulation choice) | `CFLBCfg-KF-LD.txt`: Knapsack (KskForm, `wf=1`) |
| `B2`: solver | `CFLBSCfg-cuts-R3.txt`: `GRBMILPSolver`, `intRelaxIntVars = 2` (the separation loop) | `MCFBSCfg.txt`: the `MCFSolver<...>` and `MCFLemonSolver...` | `CFLBSCfg-ld-LDS.txt`: `LagrangianDualSolver` (inner `BundleSolver` on the per-facility knapsacks) |
| what `B2` computes | the bound of the natural formulation strengthened by the dynamically separated $x_{ij} \le y_i$ cuts | a weak but very cheap lower bound, from the continuous flow relaxation | the Lagrangian bound (in theory, *equal* to the LP+cuts bound) and a convexified primal solution |

In each arrangement `B1` is solved too, by a `:MILPSolver` on the
continuous relaxation of the natural formulation, and the bound of
`B2` is compared with its bound (with the relative tolerance of
`-T`, $10^{-5}$ by default).

The three sit at different points of the bound-quality / cost
trade-off:

- **`cuts`** — a `:MILPSolver` on the natural formulation, with
  the strong forcing constraints $x_{ij} \le y_i$ separated on
  demand ([Chapter 12](12-sub-block.md#ch-12) mentioned this dynamic-constraint group). A
  strong bound, at the highest per-solve cost.
- **`MCF`** — the `MCFBlock` flow relaxation of [§10.5](10-r3block.md#sec-10-5), solved by
  a network-specialised `MCFSolver`. The weakest bound and the
  most fractional solution, but obtained extremely fast.
- **`LD`** — the Knapsack formulation solved by
  `LagrangianDualSolver`: the master's customer-satisfaction
  constraints are dualised, leaving one `LagBFunction` per
  facility (over its `BinaryKnapsackBlock` sub-`Block`), driven by
  a `BundleSolver` (Chapters [12](12-sub-block.md#ch-12), [14](14-lag-benders-bfunction.md#ch-14), [Recipe R4](R4-cfl-lagrangian.md#rec-R4)). A strong bound
  and a good convexified solution, at a cost between the other
  two.

A theoretical point worth making explicit: the bound that `cuts`
and `LD` compute is, *in theory, exactly the same*. Dualising the
customer-satisfaction constraints leaves a per-facility knapsack
subproblem; the Lagrangian dual bound therefore equals the value
of the natural LP *strengthened by the convex hull of those
knapsacks* — which is precisely what the strong forcing cuts
$x_{ij} \le y_i$ approach. This is the standard Lagrangian /
Dantzig–Wolfe equivalence, and it means `cuts` and `LD` are two
routes to one and the same bound, differing in *how* they reach it
(branch-and-cut on a MILP engine versus a bundle method on the
dual), not in the bound itself. The `MCF` flow relaxation is the
genuinely *weaker* one.

## Walk-through

- `B1` is the genuine CFL problem; `B2` is whatever `R3BCfg-*.txt`
  asks `get_R3_Block` to produce — *a copy of `B1`* in the `cuts`
  and `LD` cases (so that `B2` can be configured into a different
  *formulation* and solved without disturbing `B1`), or an
  *`MCFBlock` flow relaxation* in the `MCF` case ([§10.5](10-r3block.md#sec-10-5)).
- The two `set_BlockConfig` calls decide the *formulation*: in
  `cuts`, `B2` is the natural formulation with the strong cuts
  separated on demand;
  in `LD`, `B2` is the Knapsack formulation, which grows the
  per-facility `BinaryKnapsackBlock` sub-`Block` tree of [Chapter
  12](12-sub-block.md#ch-12); in `MCF`, `B2` is already an `MCFBlock` and needs no
  formulation choice.
- The two `BlockSolverConfig`s decide the *solver*. Switching the
  whole strategy is a matter of pointing the driver at a different
  list of formulations.
- The `UpdateSolver` registered on `B1` is what makes the
  slope-scaling loop work: when the loop nudges `B1`'s facility
  costs, the change is `map_forward`-ed to `B2` automatically
  ([Chapter 10](10-r3block.md#ch-10)), so the relaxation stays faithful to the (modified)
  problem without any explicit re-synchronisation.
- `map_back_solution(B2, r3bc)` brings `B2`'s (relaxation)
  solution back into `B1`, where the heuristic reads it.

## Expected behaviour

This recipe is about *qualitative* contrast, not a single number.
On a typical CFL instance one observes:

- `MCF` returns the lowest (weakest) lower bound, fastest, with
  the most fractional transportation solution;
- `cuts` and `LD` return the *same* bound in theory (the
  Lagrangian / Dantzig–Wolfe equivalence above), `LD` also
  producing a markedly less fractional convexified solution;
- both are far above the `MCF` bound.

**On why `cuts` and `LD` may nonetheless print *different*
numbers.** A reader comparing the cut-strengthened bound with the
`LD` one with the list `CFLBForm-ldvscuts.txt` will often see them
come out slightly *different*, in either direction, which seems
to contradict the equivalence just stated. The discrepancy is not
in the mathematics but in what the MIP engine does: with the
default `intRelaxIntVars == 0`, as in the `CFLBSCfg-ldvscuts.txt`
of that list, the problem is declared to the back-end *as a
MILP*, and the back-end (Gurobi, say) applies its MIP presolve /
bound-strengthening and its own cuts at the root node, which can
move the reported bound away from the pure LP-with-cuts value.
That extra strengthening, not a difference in the underlying
relaxation, is what makes the two numbers differ (and why the
batch `batches/batch-ldvscuts` is not among the tests of `ctest`).

To recover the equivalence *exactly*, set `intRelaxIntVars == 2`
in the `:MILPSolver`'s `ComputeConfig`, as `CFLBSCfg-ld.txt` of the
`LD` arrangement (and `CFLBSCfg-cuts-R3.txt` of the `cuts` one)
does: in that mode (`MILPSolver.h`, [§6.4](06-solver.md#sec-6-4)) the back-end solves the
problem *as a pure LP* — no MIP presolve — and `MILPSolver` itself
drives the cut-separation loop, calling
`generate_dynamic_constraints()` after each LP solve (as long as
`intCutSepPar` is nonzero, at most `intMaxCutPasses` times). With
MIP preprocessing thus switched off, the cut-strengthened bound
coincides with the `LD` Lagrangian bound, as the theory predicts,
and this is what `batches/batch-ld` checks.

The exact numbers depend on the instance and the installed MIP
back-end; the point that survives every instance is that **the
same executable produced all three** by configuration alone.

## Variations

**Swap the MIP back-end.** In `CFLBSCfg-cuts-R3.txt`, replacing
`GRBMILPSolver` with `CPXMILPSolver`, `SCIPMILPSolver` or
`HiGHSMILPSolver` (one line, the others being already there,
commented out) changes the underlying MIP engine
with no other change — the open-source HiGHS being the choice
that needs no commercial licence.

**Run the slope-scaling heuristic.** Passing a non-zero number of
iterations with `-s` turns the "solve once and compare" path into the slope-scaling
loop sketched above: solve the relaxation, map the solution back,
adjust `B1`'s costs, repeat. With the `MCF` or `LD` relaxation
this produces a feasible CFL solution (an *upper* bound) to set
against the lower bound, all from the one driver.

**Go to Lagrangian + heuristic, or to Benders.** [Recipe R4](R4-cfl-lagrangian.md#rec-R4) takes
the `LD` arrangement further, adding `PrimalProximalHeur` to
turn the convexified solution into a feasible one; [Recipe R5](R5-cfl-benders.md#rec-R5)
solves the *same* CFL by the Benders formulation instead. Both are
reached, again, by changing configuration rather than code.
