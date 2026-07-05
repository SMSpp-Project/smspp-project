# [Appendix A](A-writing-block.md#app-A) — Writing a new `:Block` {#app-A}

This appendix walks through the construction of a new `:Block`
from scratch. The running example is a deliberately small
`BinPackingBlock`, chosen because the Bin Packing problem is just
rich enough to exercise every essential step and small enough to
fit in an appendix.

> **Note.** `BinPackingBlock` is a *pedagogical* example written
> for this manual; it is **not** part of the SMS++ source tree.
> The code fragments are illustrative — realistic SMS++ C++, but
> not a production class. The aim is to show the *process* of
> writing a minimum-viable `:Block`, i.e. the steps A.1–A.7 below.
> The optional pieces (a custom `:Solution`, an `R3Block`, a
> `:Change` family) are sketched in [§A.8](A-writing-block.md#sec-A-8) but not developed.

> **Tip.** This appendix covers the *code*: the class itself. For the
> *repository* around it — the CMake and makefile builds, the CI, the test
> harness and the standard boilerplate every SMS++ module shares — do not
> start from scratch: clone the
> [ModuleTemplate](https://gitlab.com/smspp/moduletemplate) repository and
> run its `init.sh`, which generates a complete module skeleton in the
> standard layout (see the wiki page
> [Creating a new module](https://gitlab.com/smspp/smspp-project/-/wikis/Creating-a-new-module)).

The Bin Packing problem: given $n$ items of sizes $s_0, \dots,
s_{n-1}$ and bins of identical capacity $C$, pack every item into a
bin so that no bin's contents exceed $C$, using as few bins as
possible. Since no more than $n$ bins are ever needed, we cap the
number of bins at $m = n$.

## A.1 The "physical" representation and a choice of "abstract" one {#sec-A-1}

A useful first distinction ([Chapter 7](07-physical-abstract.md#ch-7)) is that the *physical*
representation is not really *chosen*: it is dictated by the
semantics of the problem — the data that defines an instance and
that a specialised solver reads directly. What is genuinely a
*design decision* is the *abstract* representation, because a given
problem admits many equivalent mathematical formulations and the
`:Block` author picks one (or, as `MCFBlock` and CFL do, makes the
choice configurable, [§7.3](07-physical-abstract.md#sec-7-3)).

For Bin Packing the physical representation is dictated and tiny:
the number of items $n$, the capacity $C$, and the vector of sizes
$s$. That is all a specialised solver would need.

The abstract representation chosen here is the natural MILP (one
formulation among several possible ones):

$$\min \sum_j y_j$$

subject to $\sum_j x_{ij} = 1$ for every item $i$ (each item in
exactly one bin), $\sum_i s_i x_{ij} \le C y_j$ for every bin $j$
(capacity, and a bin counts as used when something is in it), and
$x_{ij}, y_j \in \lbrace 0,1 \rbrace$. So the abstract
representation has two `ColVariable` groups — `x`, a
`boost::multi_array< ColVariable , 2 >` of shape $n \times m$, and
`y`, a `std::vector< ColVariable >` of size $m$ — two
`FRowConstraint` groups (assignment and capacity), and one
`FRealObjective`.

## A.2 The class skeleton and factory registration {#sec-A-2}

A concrete `:Block` derives from `Block`, takes an optional father
in its constructor, and registers itself in the `Block` factory
([Chapter 18](18-factories-netcdf.md#ch-18)):

```cpp
// BinPackingBlock.h
#include "Block.h"
#include "ColVariable.h"
#include "FRowConstraint.h"
#include "FRealObjective.h"
#include "LinearFunction.h"

namespace SMSpp_di_unipi_it {

class BinPackingBlock : public Block
{
 public:
  explicit BinPackingBlock( Block * father = nullptr ) : Block( father ) {}

  virtual ~BinPackingBlock() {
   // explicitly clear the ThinVarDepInterface fields (Constraints and the
   // Objective's Function) BEFORE the ColVariable groups they point into are
   // destroyed; see the discussion below
   Constraint::clear( v_assign );
   Constraint::clear( v_cap );
   f_obj.clear();
  }

  void load( Index n , double C , std::vector< double > sizes );

  // physical-representation accessors a specialised Solver reads
  Index  get_NItems  ( void ) const { return( f_n ); }
  double get_Capacity( void ) const { return( f_C ); }
  double get_Size( Index i )  const { return( v_s[ i ] ); }

  void generate_abstract_variables( Configuration * = nullptr ) override;
  void generate_abstract_constraints( Configuration * = nullptr ) override;
  void generate_objective( Configuration * = nullptr ) override;

  void chg_size( double new_size , Index item ,
                 ModParam issueMod = eModBlck , ModParam issueAMod = eModBlck );

  // deposit a solution in compact (physical) form and reflect it into the
  // abstract Variables; implemented and discussed in Appendix B (§B.6)
  void set_assignment( const std::vector< Index > & bin_of );

  bool is_feasible( bool useabstract = false , Configuration * = nullptr )
   override;

  void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override;

  void load( std::istream & , char = 0 ) override { /* text format */ }

 private:
  // decode an abstract Modification into a physical change (§A.6)
  void guts_of_add_Modification( c_p_Mod mod , ChnlName chnl );

  // physical representation
  Index               f_n = 0;
  double              f_C = 0;
  std::vector< double > v_s;          // item sizes

  // compact (physical) solution: for each item, the index of its bin;
  // a single n-vector, as opposed to the n*m + m abstract binaries (§B.6)
  std::vector< Index >  v_bin_of;

  // abstract representation (built on demand)
  boost::multi_array< ColVariable , 2 > v_x;   // x[ i ][ j ]
  std::vector< ColVariable >            v_y;   // y[ j ]
  std::vector< FRowConstraint >         v_assign;   // one per item
  std::vector< FRowConstraint >         v_cap;      // one per bin
  FRealObjective                        f_obj;

  SMSpp_insert_in_factory_h;          // factory hook (Chapter 18)
};

}  // namespace
```

```cpp
// BinPackingBlock.cpp
SMSpp_insert_in_factory_cpp_1( BinPackingBlock );   // ctor takes 1 arg
```

The `SMSpp_insert_in_factory_cpp_1` macro (the `_1` because the
constructor takes one optional argument, the father) registers the
class so that `Block::new_Block("BinPackingBlock")` works, and runs
the class's `static_initialization()` at start-up ([Chapter 18](18-factories-netcdf.md#ch-18),
[§18.1](18-factories-netcdf.md#sec-18-1); mind the linker caveat of [§18.2](18-factories-netcdf.md#sec-18-2)).

The non-trivial destructor deserves a word, because it follows a
framework convention that is easy to miss. A `Constraint` (like any
`ThinVarDepInterface`) and the `Variable`s it is "active" in are
*doubly* linked: each side holds pointers into the other. The
framework's invariant (the `ThinVarDepInterface` documentation) is
that *`Variable`s are constructed before the things they are active
in and destroyed after them*, so that when a `Constraint` (or a
`Function` inside an `Objective`) is destroyed, the `Variable`s it
references are still live and the back-pointers can be cleaned up
safely. Inside a `:Block`, however, the order in which member
fields are destroyed is "usually not very clear" — it is the
reverse of declaration order, which is fragile to rely on — so the
recommendation is to *explicitly* clear the dependent objects in
the destructor, guaranteeing that every `Constraint` and the
`Objective`'s `Function` is destroyed (and unlinks itself) before
the `ColVariable` groups `v_x` / `v_y` they point into. `clear()`
on each object does exactly this; `Constraint.h` provides static
`Constraint::clear(...)` helpers that apply it to every group
shape — `std::vector`, `std::vector` of `std::vector`,
`boost::multi_array`, `std::list`, and their combinations — so the
whole groups can be cleared with one call each. (Were the order
left to the compiler, a `ColVariable` might be destroyed while a
`Function` still held a pointer to it, leaving a dangling
reference.)

## A.3 `load()` and the physical representation {#sec-A-3}

`load()` simply copies the data and, if any `:Solver` is already
attached, issues the "nuclear" `NBModification` ([§4.6](04-block.md#sec-4-6)) to tell it
the instance has been replaced wholesale:

```cpp
void BinPackingBlock::load( Index n , double C , std::vector< double > sizes )
{
 f_n = n;
 f_C = C;
 v_s = std::move( sizes );

 if( anyone_there() )                       // a Solver is listening
  add_Modification( std::make_shared< NBModification >( this ) );
}
```

At this point the `BinPackingBlock` is fully usable by a
*specialised* solver ([Appendix B](B-writing-solver.md#app-B)) that reads `f_n`, `f_C`, `v_s`
directly. The abstract representation does not exist yet.

## A.4 Generating the abstract representation {#sec-A-4}

The three `generate_*` overrides build the abstract face on demand
([Chapter 7](07-physical-abstract.md#ch-7)). `generate_abstract_variables()` materialises the
`ColVariable` groups, declaring each binary:

```cpp
void BinPackingBlock::generate_abstract_variables( Configuration * )
{
 if( ! v_y.empty() )  return;               // already done

 v_x.resize( boost::extents[ f_n ][ f_n ] );  // m == n bins
 for( auto & row : v_x ) for( auto & xij : row )
  xij.set_type( ColVariable::kBinary );

 v_y.resize( f_n );
 for( auto & yj : v_y ) yj.set_type( ColVariable::kBinary );

 add_static_variable( v_x , "x" );          // register the groups
 add_static_variable( v_y , "y" );
}
```

`generate_abstract_constraints()` builds the assignment and
capacity rows, each an `FRowConstraint` carrying a linear
`Function`. A `Configuration` could gate which groups are built
(as `MCFBlock` and CFL do, [§7.3](07-physical-abstract.md#sec-7-3)); here we build both:

```cpp
void BinPackingBlock::generate_abstract_constraints( Configuration * )
{
 if( ! v_assign.empty() )  return;

 // assignment: sum_j x[i][j] == 1  for each item i
 v_assign.resize( f_n );
 for( Index i = 0 ; i < f_n ; ++i ) {
  // a linear Function summing x[i][0..m-1] with coeff 1: build the
  // (Variable* , coefficient) pairs, one per bin j, then hand the new
  // LinearFunction (constant term 0) to the FRowConstraint
  LinearFunction::v_coeff_pair cp( f_n );
  for( Index j = 0 ; j < f_n ; ++j ) {
   cp[ j ].first  = & v_x[ i ][ j ];
   cp[ j ].second = 1.0;
  }
  v_assign[ i ].set_function( new LinearFunction( std::move( cp ) , 0 ) ,
                              eNoMod );      // not built yet, no Solver to tell
  v_assign[ i ].set_both( 1.0 );            // == 1
 }

 // capacity: sum_i s_i x[i][j] - C y[j] <= 0  for each bin j
 v_cap.resize( f_n );
 for( Index j = 0 ; j < f_n ; ++j ) {
  // linear Function: sum_i s_i x[i][j] - C y[j]. The n item terms come
  // first (so coefficient k corresponds to x[k][j], matching chg_size
  // and the abstract-stream decoding in add_Modification), then the
  // single -C y[j] term
  LinearFunction::v_coeff_pair cp( f_n + 1 );
  for( Index i = 0 ; i < f_n ; ++i ) {
   cp[ i ].first  = & v_x[ i ][ j ];
   cp[ i ].second = v_s[ i ];
  }
  cp[ f_n ].first  = & v_y[ j ];
  cp[ f_n ].second = - f_C;
  v_cap[ j ].set_function( new LinearFunction( std::move( cp ) , 0 ) , eNoMod );
  v_cap[ j ].set_lhs( -Inf< double >() );
  v_cap[ j ].set_rhs( 0.0 );                // <= 0
 }

 add_static_constraint( v_assign , "assign" );
 add_static_constraint( v_cap , "cap" );
}
```

`generate_objective()` sets the minimisation of $\sum_j y_j$:

```cpp
void BinPackingBlock::generate_objective( Configuration * )
{
 // linear Function: sum_j 1 * y[j]
 LinearFunction::v_coeff_pair cp( f_n );
 for( Index j = 0 ; j < f_n ; ++j ) {
  cp[ j ].first  = & v_y[ j ];
  cp[ j ].second = 1.0;
 }
 f_obj.set_function( new LinearFunction( std::move( cp ) , 0 ) , eNoMod );
 f_obj.set_sense( Objective::eMin , eNoMod );
 set_objective( & f_obj , eNoMod );
}
```

Two things to read off these snippets. First, the linear
expressions are `LinearFunction` objects ([Chapter 13](13-function-family.md#ch-13)), built from a
`LinearFunction::v_coeff_pair` — a vector of
`(ColVariable * , Coefficient)` pairs — plus a constant term;
ownership of each `new LinearFunction(...)` passes to the
`FRowConstraint` / `FRealObjective` that receives it via
`set_function()`, [§5.7](05-variable-constraint-objective.md#sec-5-7). Second, every `set_*` here is passed
`eNoMod`: the abstract representation is being built for the first
time, so there is nothing to notify — no `Modification` need (or
should) be issued ([§8.3](08-modification-janus.md#sec-8-3)). The deliberate ordering of the capacity
row's terms (the $n$ item coefficients first, then the single
$-C y_j$ term) is what lets `chg_size` ([§A.5](A-writing-block.md#sec-A-5)) and the
abstract-stream decoding in `add_Modification` ([§A.6](A-writing-block.md#sec-A-6)) address
coefficient $k$ as "the $x_{k j}$ term".

## A.5 A `chg_*()` method and its `Modification` {#sec-A-5}

A `:Block` author exposes mutators for the data that may change.
`chg_size` changes one item's size in the physical representation
and, when the abstract representation exists, mirrors the change
there — issuing the appropriate `Modification`s on each face
([Chapter 8](08-modification-janus.md#ch-8)). Giving it one of the methods-factory shapes ([§15.3](15-methods-factory.md#sec-15-3))
would also let it be called by name; here the single-item form:

```cpp
void BinPackingBlock::chg_size( double new_size , Index item ,
                                ModParam issueMod , ModParam issueAMod )
{
 if( v_s[ item ] == new_size )  return;     // nothing to do
 v_s[ item ] = new_size;                    // physical change

 // physical Modification, for a specialised Solver
 if( issue_pmod( issueMod ) )
  add_Modification( std::make_shared< BinPackingBlockMod >(
                      this , item ) , Observer::par2chnl( issueMod ) );

 // mirror into the abstract representation, if it exists: the size
 // appears as the coefficient of x[item][j] in each capacity row j,
 // which (by construction, §A.4) is coefficient number `item` of that
 // row's LinearFunction. modify_coefficient() updates it and, driven by
 // issueAMod, issues the abstract C05FunctionModLin on its own. The
 // not_dry_run() guard is what makes this safe to call from
 // add_Modification() with issueAMod == eDryRun: there the abstract change
 // has *already* happened, so we must touch the physical face only
 if( not_dry_run( issueAMod ) && ( ! v_cap.empty() ) )
  for( Index j = 0 ; j < f_n ; ++j ) {
   auto * lf = static_cast< LinearFunction * >( v_cap[ j ].get_function() );
   lf->modify_coefficient( item , new_size , issueAMod );
  }
}
```

The `BinPackingBlockMod` is the `:Block`-specific physical
`Modification` defined in [Appendix C](C-writing-modification.md#app-C).

## A.6 `add_Modification()`: catching abstract changes {#sec-A-6}

The Janus discipline ([Chapter 8](08-modification-janus.md#ch-8), [§8.4](08-modification-janus.md#sec-8-4)) requires that a change made
through the *abstract* face be reflected in the physical one. The
`:Block` does this by overriding `add_Modification()` to intercept
the abstract `Modification`s whose `concerns_Block()` is `true`,
apply the matching physical change, reset the flag, and forward:

```cpp
void BinPackingBlock::add_Modification( sp_Mod mod , ChnlName chnl )
{
 if( mod->concerns_Block() ) {
  mod->concerns_Block( false );
  guts_of_add_Modification( mod.get() , chnl );
 }
 Block::add_Modification( mod , chnl );      // base mechanics (Chapter 8)
}
```

The decoding is delegated to a private helper. The *only* abstract
change this `:Block` knows how to fold back into its physical data
is a change to an item-size coefficient in a capacity row — i.e. a
`C05FunctionModLinRngd` / `C05FunctionModLinSbst` on one of the
`v_cap[j]` `LinearFunction`s, touching an $x_{ij}$ coefficient
(index $< n$). Anything else (a change to the structural
$-C y_j$ coefficient, to an assignment row, to the objective, an
added or removed `Variable`, ...) has no physical counterpart, so
the helper throws:

```cpp
void BinPackingBlock::guts_of_add_Modification( c_p_Mod mod , ChnlName chnl )
{
 // helper: which capacity row owns this Function? (npos if none)
 auto cap_row = [ this ]( const Function * f ) -> Index {
  for( Index j = 0 ; j < f_n ; ++j )
   if( v_cap[ j ].get_function() == f )  return( j );
  return( Inf< Index >() );
 };

 // helper: fold one changed coefficient k of a capacity row back into v_s.
 // k must be an x-term (k < f_n); the n-th coefficient is the structural
 // -C * y[j] and must never change through the abstract face
 auto fold = [ & ]( LinearFunction * lf , Index k ) {
  if( k >= f_n )
   throw( std::logic_error( "BinPackingBlock: structural coefficient "
                            "changed through the abstract face" ) );
  // chg_size with eNoBlck on the physical face (tell specialised Solvers)
  // and eDryRun on the abstract face (it has already changed); §8.4
  chg_size( lf->get_coefficient( k ) , k , make_par( eNoBlck , chnl ) ,
            eDryRun );
 };

 if( auto tmod = dynamic_cast< const C05FunctionModLinRngd * >( mod ) ) {
  auto * lf = dynamic_cast< LinearFunction * >( tmod->function() );
  if( ( ! lf ) || ( cap_row( lf ) == Inf< Index >() ) )
   throw( std::logic_error( "BinPackingBlock: unsupported abstract change" ) );
  for( Index k = tmod->range().first ; k < tmod->range().second ; ++k )
   fold( lf , k );
  return;
 }

 if( auto tmod = dynamic_cast< const C05FunctionModLinSbst * >( mod ) ) {
  auto * lf = dynamic_cast< LinearFunction * >( tmod->function() );
  if( ( ! lf ) || ( cap_row( lf ) == Inf< Index >() ) )
   throw( std::logic_error( "BinPackingBlock: unsupported abstract change" ) );
  for( auto k : tmod->subset() )  fold( lf , k );
  return;
 }

 // any other abstract Modification has no physical counterpart here
 throw( std::logic_error( "BinPackingBlock: unsupported abstract change" ) );
}
```

Two honest caveats. First, *failing loud* is deliberate: a `:Block`
is free to support only some abstract changes and to throw for the
rest, exactly as `MCFBlock` does for arc add/remove ([§8.4](08-modification-janus.md#sec-8-4)).
Second, a subtlety peculiar to this formulation: the size $s_i$ is
*shared* across all $m$ capacity rows, but the abstract face
exposes it as $m$ independent coefficients. When the change arrives
on a single row $j^{\star}$, `fold` calls `chg_size` with `eDryRun`,
which (by the guard added in [§A.5](A-writing-block.md#sec-A-5)) updates only the physical
$v_s$ and the originating row, leaving the sibling rows momentarily
stale. A production-grade `BinPackingBlock` would either reject
per-row coefficient edits outright or re-propagate the new size to
all $m$ rows; the minimal example shows the mechanism on the row
that changed and flags the consequence rather than hiding it.

## A.7 `is_feasible()` on the physical representation {#sec-A-7}

Finally, `is_feasible()` checks a candidate solution. Written on
the *physical* representation it is cheap ([§7.5](07-physical-abstract.md#sec-7-5)): read each item's
bin assignment from the abstract `x` variables (or from wherever
the solution lives), and verify that every item is assigned to
exactly one bin and that no bin exceeds capacity.

```cpp
bool BinPackingBlock::is_feasible( bool useabstract , Configuration * )
{
 std::vector< double > load( f_n , 0.0 );
 for( Index i = 0 ; i < f_n ; ++i ) {
  int assigned = -1;
  for( Index j = 0 ; j < f_n ; ++j )
   if( v_x[ i ][ j ].get_value() > 0.5 ) {  // item i in bin j
    if( assigned >= 0 )  return( false );    // assigned twice
    assigned = int( j );
    load[ j ] += v_s[ i ];
   }
  if( assigned < 0 )  return( false );       // unassigned
 }
 for( Index j = 0 ; j < f_n ; ++j )
  if( load[ j ] > f_C + 1e-9 )  return( false );  // over capacity
 return( true );
}
```

The `useabstract` flag ([§7.5](07-physical-abstract.md#sec-7-5)) would select an
alternative path that walks the `FRowConstraint`s and calls their
`feasible()`; for a leaf `:Block` like this the physical check
above is the one a specialised solver wants.

## A.8 Optional pieces (sketched) {#sec-A-8}

A minimum-viable `:Block` ends at [§A.7](A-writing-block.md#sec-A-7). A "complete" `:Block`
typically adds, each only when the use case calls for it:

- a `:Solution` ([Chapter 9](09-solution.md#ch-9)) — a `BinPackingSolution` storing the
  per-item bin assignment, so a solution can be saved, restored
  and combined without going through the abstract `Variable`s;
- an `R3Block` ([Chapter 10](10-r3block.md#ch-10)) — at least the trivial copy, via
  `get_R3_Block` plus `map_back_solution` / `map_forward_solution`;
- a `:Change` family ([Chapter 16](16-change.md#ch-16)) — for serialisable / undoable
  edits; **Status — beta** ([Appendix C](C-writing-modification.md#app-C)).

None of these is required to have a working, solver-attachable
`:Block`; they are the difference between a teaching example and a
production module.

## A.9 Checklist {#sec-A-9}

To recapitulate, a minimum-viable `:Block` needs:

1. a class deriving from `Block`, with a father-taking constructor
   and the `SMSpp_insert_in_factory_h` / `_cpp_k` macros ([§A.2](A-writing-block.md#sec-A-2),
   [Chapter 18](18-factories-netcdf.md#ch-18));
2. the physical representation as member data, populated by
   `load()` / `deserialize()`, issuing `NBModification` on reload
   ([§A.3](A-writing-block.md#sec-A-3));
3. the `generate_abstract_*()` overrides building the abstract
   face on demand ([§A.4](A-writing-block.md#sec-A-4));
4. `chg_*()` mutators issuing physical (and, where relevant,
   abstract) `Modification`s ([§A.5](A-writing-block.md#sec-A-5));
5. `add_Modification()` catching abstract changes and keeping the
   physical face in sync, or throwing for unsupported ones ([§A.6](A-writing-block.md#sec-A-6));
6. `is_feasible()` (and, for an optimisation `:Block`,
   `get_objective_sense()` and valid bounds) on the physical
   representation ([§A.7](A-writing-block.md#sec-A-7)).
