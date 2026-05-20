# Recipe R2 — Reoptimizing a Binary Knapsack

> **Counterpart in the source tree:**
> `tests/BinaryKnapsackBlock/test.cpp`, which exercises the same
> `chg_*` reoptimization paths (and cross-checks the result against
> a second solver).

## Goal

Solve a small knapsack with the specialised
`DPBinaryKnapsackSolver`; then change the data *incrementally* and
re-solve, letting the solver *reoptimize* from its previous state
rather than starting over. This is the smallest illustration of
SMS++'s pervasive reoptimization theme (Chapters 8 and 13).

## Concepts used

- `Block` construction and `load(...)` — Chapter 4.
- A *native* specialised `Solver` — Chapter 6.
- `Modification` from a physical `chg_*()` change — Chapter 8.
- Reoptimization, and what a solver may reuse across a change —
  Chapter 13.
- (Variation) the `Change` family — Chapter 16.

## The code

```cpp
#include <iostream>

#include "BinaryKnapsackBlock.h"
#include "DPBinaryKnapsackSolver.h"

using namespace SMSpp_di_unipi_it;

static void solve_and_report( BinaryKnapsackBlock & bkb , Solver * s )
{
 if( s->compute() != Solver::kOK ) { std::cout << "not solved\n"; return; }
 std::cout << "objective = " << bkb.get_objective_value() << " ; x = (";
 for( BinaryKnapsackBlock::Index i = 0 ; i < bkb.get_NItems() ; ++i )
  std::cout << ( i ? "," : "" ) << bkb.get_x( i );
 std::cout << ")\n";
}

int main()
{
 // 4 items, capacity 5; weights must be integer for the DP solver
 BinaryKnapsackBlock bkb;
 const std::vector< double > W = { 2.0 , 3.0 , 4.0 , 1.0 };  // weights
 const std::vector< double > P = { 3.0 , 4.0 , 5.0 , 1.0 };  // profits
 bkb.load( 4 , /* capacity = */ 5.0 , W , P );   // maximization by default

 auto * solver = new DPBinaryKnapsackSolver();
 bkb.register_Solver( solver );

 // ---- first solve ----
 solve_and_report( bkb , solver );

 // ---- incremental change: item 2 becomes much more profitable ----
 bkb.chg_profit( 8.0 , /* item = */ 2 );   // physical change -> Modification

 // ---- re-solve: the solver reoptimizes from its previous state ----
 solve_and_report( bkb , solver );

 bkb.unregister_Solver( solver , /* deleteold = */ true );
 return 0;
}
```

## Walk-through

- `load(4, 5.0, W, P)` sets the physical data of the knapsack; the
  default sense is maximisation (`set_objective_sense(false)`
  would switch it to minimisation). The `DPBinaryKnapsackSolver`
  requires the *weights* to be integer — they are — while
  capacity and profits may be real.
- `DPBinaryKnapsackSolver` is a *native* SMS++ specialised solver
  (Chapter 6): a dynamic-programming recursion over the items, no
  external dependency. It reads the physical representation, so —
  as in R1 — no `generate_abstract_*()` call is needed, and the
  solution is read back through the `Block`'s accessors
  `get_objective_value()` / `get_x(i)`.
- `chg_profit(8.0, 2)` changes the profit of item 2 in the
  physical representation and issues the corresponding
  `BinaryKnapsackBlockMod` (Chapter 8). Because the solver is
  registered, it receives the `Modification` in its pending list.
- The second `compute()` consumes that one `Modification` and
  **reoptimizes**: a single-profit change does not invalidate the
  whole DP table, and the solver reuses what it can rather than
  rebuilding from scratch. The
  `DPBinaryKnapsackSolver` exposes a `dblReopt` parameter
  (`DPBinaryKnapsackSolver.h`) governing how aggressively it
  reoptimizes. This is the recipe-scale instance of the
  reoptimization theme that, at scale, lets a `BundleSolver`
  reuse its bundle across changes to an inner `Block` (Chapter 13,
  Recipe R4).

## Expected output

The first solve: with weights `(2,3,4,1)`, profits `(3,4,5,1)`,
capacity 5, the best subset is items {0,1} (weight 2+3 = 5, profit
3+4 = 7). After raising item 2's profit to 8, the best subset
becomes {2,3} (weight 4+1 = 5, profit 8+1 = 9):

```
objective = 7 ; x = (1,1,0,0)
objective = 9 ; x = (0,0,1,1)
```

## Variations

**Change several items at once, by range or subset.**
`chg_profits(it, Range)` and `chg_profits(it, Subset&&, bool)`
(and the analogous `chg_weights`) change a contiguous interval or
an arbitrary subset of items in one call, issuing a single
`Rngd` / `Sbst` `Modification` that the solver can react to as a
unit (Chapter 8). `chg_capacity(newC)` changes the knapsack
capacity.

**Apply the change as a `Change` (beta) instead.** Because
`BinaryKnapsackBlock` ships a `Change` family (Chapter 16), the
same edit can be expressed as a `BinaryKnapsackBlockRngdChange`,
`apply()`-ed with `doUndo = true` to obtain the inverse, the
modified block explored, and the change rolled back with the
returned UndoChange. This is the right tool when the edit must be
*serialised*, *transmitted*, or *undone* — not for an ordinary
in-process re-solve, for which the `chg_*()` path above is
simpler. **Status — beta.**

**Change the data by name, through the methods factory.**
`BinaryKnapsackBlock` registers `chg_weights`, `chg_profits` and
`chg_capacity` in the methods factory (Chapter 15), so the same
edit made above through the concrete type can also be issued by
*string name* on a base `Block*`:

```cpp
 auto fun = bkb.get_method< Block::MS_dbl_rngd >(
                              "BinaryKnapsackBlock::chg_profits" );
 fun( & bkb , std::vector< double >{ 8.0 }.begin() ,
      Block::Range( 2 , 3 ) , eModBlck , eModBlck );
```

This is exactly how type-agnostic machinery (a `StochasticBlock`
scenario realiser, a meta-configuration driver) changes a
`BinaryKnapsackBlock`'s data without a compile-time dependency on
its type.
