# Appendix B — Writing a new `:Solver`

This appendix shows how to write a new `:Solver`, using a trivial
first-fit greedy heuristic for the `BinPackingBlock` of Appendix A
as the worked example. As there, the code is illustrative rather
than production-grade.

## B.1 `Solver` versus `CDASolver`

The first choice is the base class (Chapter 6):

- derive from `Solver` for a `:Solver` that produces *primal*
  solutions only;
- derive from `CDASolver` ("Convex Duality Aware") when the
  `:Solver` also produces *dual* solutions / bounds with an
  associated dual object — as `MCFSolver` and the `MILPSolver`
  family do.

A greedy Bin Packing heuristic produces a feasible assignment and
no dual information, so it derives from `Solver`.

## B.2 The `compute()` contract

`compute(bool changedvars)` is the heart of a `:Solver`
(Chapter 6, §6.3). Its obligations:

- run the algorithm and return an `sol_type`. An *exact* solver
  returns `kOK` / `kUnbounded` / `kInfeasible` when it concludes;
  a *heuristic* one that finds a feasible solution but cannot
  prove optimality returns `kLowPrecision`; resource-limited stops
  return `kStopTime` / `kStopIter`; unrecoverable errors return a
  value $\ge$ `kError` (§6.3).
- honour `changedvars` (§6.1): with `false`, the caller guarantees
  the relevant `Variable` values are unchanged since the last call,
  so the `:Solver` may resume; with `true` it must assume they may
  have changed. (A one-shot heuristic can simply ignore the hint
  and recompute.)
- record the solution so that it can later be read. A `:Solver`
  deposits its solution into the `Block` — for a `:Block` with
  abstract `Variable`s, by writing their values; the user then
  reads it through the `Block` (§6.9).

## B.3 Parameters

A `:Solver` inherits the parameter machinery of §6.4
(`set_par` / `get_par` over integer / double / string slots:
`intMaxIter`, `dblMaxTime`, ...) and may extend it with its own
enum values. A simple heuristic may need none and can rely on the
defaults; a `:Solver` that does add parameters extends
`int_par_type_S` / `dbl_par_type_S` past their `...LastAlgPar`
sentinels, exactly as `DPBinaryKnapsackSolver` adds `dblReopt`
(Recipe R2).

## B.4 Reacting to `Modification`s

The framework fills the `:Solver`'s pending list with the
`Modification`s issued since it was attached (Chapter 8). A
`:Solver` consumes that list at the start of `compute()` and reacts:
an exact, reoptimizing solver uses the fine-grained information to
warm-start (Chapter 13); a one-shot heuristic may simply note "the
data changed, recompute from scratch", clearing the list. Either
way, the `:Solver` is responsible for emptying its own list.

## B.5 Optional: `compute_async` and `State`

`compute_async()` comes for free from `ThinComputeInterface`
(§17.2); a `:Solver` only needs to ensure its `compute()` is
thread-safe with respect to the `Block` (the recursive locking of
§17.1). A `:Solver` with meaningful internal state implements
`get_State()` / `put_State()` for checkpointing and warm-starting
(§17.3); one without — like the greedy heuristic — returns
`nullptr` and pays nothing.

## B.6 Worked example: a first-fit heuristic for `BinPackingBlock`

The heuristic reads the physical representation of the
`BinPackingBlock` (the sizes and capacity, through the accessors of
§A.2), places each item into the first bin that has room (opening a
new bin when none does), and deposits the resulting assignment into
the `Block`. It also computes a cheap *lower* bound — no packing can
use fewer than $\lceil (\sum_i s_i) / C \rceil$ bins — alongside the
*upper* bound given by the feasible solution it just built (the
number of bins it actually used). It reports these separately
through `get_lb()` / `get_ub()`, and returns `kOK` when they happen
to coincide (the greedy solution is then provably optimal) or
`kLowPrecision` otherwise (feasible, but optimality not proven):

```cpp
// FirstFitBPSolver.h
#include "Solver.h"
#include "BinPackingBlock.h"

namespace SMSpp_di_unipi_it {

class FirstFitBPSolver : public Solver
{
 public:
  void set_Block( Block * block ) override {
   f_bp = dynamic_cast< BinPackingBlock * >( block );
   if( block && ! f_bp )
    throw( std::invalid_argument( "FirstFitBPSolver: not a BinPackingBlock" ) );
   Solver::set_Block( block );
  }

  int compute( bool /* changedvars */ = true ) override {
   // consume any pending Modifications: this heuristic just restarts,
   // so we clear the list and recompute from scratch
   v_mod.clear();

   const Index  n = f_bp->get_NItems();
   const double C = f_bp->get_Capacity();
   std::vector< double > load( n , 0.0 );      // current load of each bin
   std::vector< Index >  bin_of( n , 0 );      // bin chosen for each item

   double total = 0;                           // sum of all item sizes
   Index  nbins = 0;                           // number of bins opened

   for( Index i = 0 ; i < n ; ++i ) {
    const double s = f_bp->get_Size( i );
    if( s > C )  return( kInfeasible );         // item bigger than a bin
    total += s;
    Index j = 0;
    while( load[ j ] + s > C + 1e-9 )  ++j;     // first bin with room
    load[ j ] += s;
    bin_of[ i ] = j;
    if( j + 1 > nbins )  nbins = j + 1;         // a new bin was opened
   }

   // deposit the solution into the Block (compact + abstract; §B.6)
   f_bp->set_assignment( bin_of );

   // upper bound: the value of the feasible solution just built
   f_ub = OFValue( nbins );
   // lower bound: no packing can use fewer than ceil( total / C ) bins
   f_lb = std::ceil( total / C );

   // if the two bounds coincide, the greedy solution is provably optimal
   return( f_lb == f_ub ? kOK : kLowPrecision );
  }

  // report the two bounds separately (BinPackingBlock is a minimization)
  OFValue get_lb( void ) override { return( f_lb ); }
  OFValue get_ub( void ) override { return( f_ub ); }

  // a one-shot heuristic keeps no warm-startable state
  // (get_State() inherits the nullptr default)

 private:
  BinPackingBlock * f_bp = nullptr;
  OFValue f_lb = -Inf< OFValue >();   // valid lower bound (none yet)
  OFValue f_ub =  Inf< OFValue >();   // best feasible value (none yet)

  SMSpp_insert_in_factory_h;     // so Solver::new_Solver("FirstFitBPSolver") works
};

}  // namespace
```

```cpp
// FirstFitBPSolver.cpp
SMSpp_insert_in_factory_cpp_0( FirstFitBPSolver );  // ctor takes 0 args
```

The `set_assignment` helper the solver calls is the `Block`-side
method declared in §A.2. It is worth seeing in full, because it
makes a point that is easy to gloss over: the *compact* (physical)
form of a solution and the *abstract* form generally have very
different shapes, and translating between them is real work the
`:Block` must do. Here the compact form is a single $n$-vector
(`v_bin_of[i]` = the bin holding item $i$); the abstract form is
the $n\times m$ matrix of `x` binaries plus the $m$-vector of `y`
binaries. `set_assignment` stores the compact form and then
*derives* the abstract one from it — in particular it has to infer
which bins are used (the `y[j]`s) from the per-item assignment,
which is not given explicitly in the compact representation:

```cpp
// BinPackingBlock.cpp
void BinPackingBlock::set_assignment( const std::vector< Index > & bin_of )
{
 v_bin_of = bin_of;                           // store the compact solution

 // reflect it into the abstract Variables, if they have been generated
 if( v_y.empty() )  return;                    // no abstract face to fill

 for( auto & row : v_x )                       // start from all-zero ...
  for( auto & xij : row )  xij.set_value( 0 );
 for( auto & yj : v_y )    yj.set_value( 0 );

 for( Index i = 0 ; i < f_n ; ++i ) {
  const Index j = v_bin_of[ i ];
  v_x[ i ][ j ].set_value( 1 );                // item i is placed in bin j
  v_y[ j ].set_value( 1 );                     // hence bin j counts as used
 }
}
```

A few points to read off:

- `set_Block` downcasts to the concrete `:Block` the solver
  understands and rejects anything else — the standard guard of a
  *specialised* `:Solver`.
- `compute()` returns `kInfeasible` with a certificate-by-construction
  (an item larger than any bin); otherwise it returns `kOK` when the
  cheap lower bound matches the bins actually used — the greedy
  solution is then *provably* optimal — and `kLowPrecision` when it
  does not, honestly signalling a feasible but not proven-optimal
  solution (§6.3). The two bounds are exposed separately through the
  inherited `get_lb()` / `get_ub()`.
- the solution is written *into the `Block`* through `set_assignment`,
  which keeps both the compact physical solution and the abstract
  `x` / `y` values, so the user can read it back through the `Block`
  in either form, as for any `:Solver` (§6.9).
- `SMSpp_insert_in_factory_cpp_0` (zero-argument constructor)
  registers the `:Solver` so it can be named in a
  `BlockSolverConfig` (Chapter 11) — and so it is subject to the
  linker caveat of §18.2.

This heuristic is intentionally minimal: no parameters, no dual
information, no reoptimization, no async state. It is a complete,
attachable `:Solver` nonetheless — enough to make a
`BinPackingBlock` solvable, which (as Chapter 2, §2.4 noted) is the
precondition for testing the `:Block` at all.
