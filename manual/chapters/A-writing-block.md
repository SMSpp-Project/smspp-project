# Appendix A: writing a new `:Block` {#app-A}

[Source: [`Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h),
[`SMSFactory.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h),
[`SMSTypedefs.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSTypedefs.h),
[`Observer.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Observer.h),
[`Modification.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Modification.h),
[`ThinVarDepInterface.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ThinVarDepInterface.h),
[`Constraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Constraint.h),
[`LinearFunction.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/LinearFunction.h),
[`C05Function.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/C05Function.h)]

We are concerned in this appendix with the construction of a new
`:Block` from scratch, on the example of a deliberately small
`BinPackingBlock`, chosen because the Bin Packing problem is rich enough
to exercise every essential step and small enough to fit in an
appendix. The physical representation and a choice of abstract one are
discussed in [§A.1](A-writing-block.md#sec-A-1), the class skeleton and
its registration in the factory in [§A.2](A-writing-block.md#sec-A-2),
and `load()` in [§A.3](A-writing-block.md#sec-A-3). Then come the
generation of the abstract representation
([§A.4](A-writing-block.md#sec-A-4)), a `chg_*()` method with its
`Modification`s ([§A.5](A-writing-block.md#sec-A-5)), the interception
of the abstract changes ([§A.6](A-writing-block.md#sec-A-6)) and
`is_feasible()` ([§A.7](A-writing-block.md#sec-A-7)); the optional
pieces are sketched in [§A.8](A-writing-block.md#sec-A-8), and
[§A.9](A-writing-block.md#sec-A-9) is the checklist.

> *Note.* `BinPackingBlock` is a pedagogical example written for this
> manual, and it is not part of the SMS++ source tree. The fragments of
> this appendix and of Appendices B and C, put together in one
> translation unit, compile against the headers of SMS++; they are,
> however, a teaching example rather than a production class, whose aim
> is to show the *process* of writing a minimum-viable `:Block`.
> The optional pieces (a custom `:Solution`, an R3 Block, a
> `:Change` family) are sketched in [§A.8](A-writing-block.md#sec-A-8)
> but not developed.

> *Tip.* This appendix covers the *code*: the class itself. For the
> *repository* around it (the CMake and makefile builds, the CI, the
> test harness and the boilerplate that every SMS++ module shares) one
> does not start from scratch, but clones the
> [ModuleTemplate](https://gitlab.com/smspp-develop/moduletemplate)
> repository (in the group `smspp-develop`, to which the maintainers give
> access) and runs its `init.sh` (e.g., `./init.sh --name BinPackingBlock
> --author <name>`, the author being required unless git has a
> `user.name`, and `--umbrella <path>` to also register the module in a
> checkout of the umbrella project). It generates a complete module
> skeleton in the standard layout, whose name is that of the library, of
> the CMake project, of the repository and of the label of its tests, with
> a stub `:Block` already registered in the factory that the class of this
> appendix replaces. Each new source file is then listed in
> `target_sources()` of `CMakeLists.txt` and in the macros of the
> `makefile`, which do not glob, and the interface of the C++20 module,
> `src/<Name>.cppm`, is generated again by
> `SMS++/cmake/smspp_module_exports.py` whenever a header gains or loses a
> name (e.g., when `BinPackingBlockMod` of
> [Appendix C](C-writing-modification.md#app-C) is added); see the wiki
> page
> [Creating a new module](https://gitlab.com/smspp/smspp-project/-/wikis/Creating-a-new-module).

The Bin Packing problem: given $n$ items of sizes $s_0, \dots,
s_{n-1}$ and bins of identical capacity $C$, pack every item into a
bin so that no bin's contents exceed $C$, using as few bins as
possible. Since no more than $n$ bins are ever needed, we cap the
number of bins at $m = n$.

## A.1 The "physical" representation and a choice of "abstract" one {#sec-A-1}

The first distinction to draw ([Chapter 7](07-physical-abstract.md#ch-7))
is that the *physical* representation is not really *chosen*: it is
dictated by the semantics of the problem, i.e., by the data that define
an instance and that a specialized solver reads directly. The *design
decision* is rather the *abstract* representation, since a given problem
admits many equivalent mathematical formulations, among which the
`:Block` author picks one (or, as `MCFBlock` and CFL do, makes the
choice configurable, [§7.3](07-physical-abstract.md#sec-7-3)).

For Bin Packing the physical representation is dictated and tiny:
the number of items $n$, the capacity $C$ and the vector of sizes
$s$. That is all a specialized solver would need.

The abstract representation chosen here is the natural MILP (one
formulation among several possible ones):

$$\min \sum_j y_j$$

subject to $\sum_j x_{ij} = 1$ for every item $i$ (each item in
exactly one bin), $\sum_i s_i x_{ij} \le C y_j$ for every bin $j$
(capacity, and a bin counts as used when something is in it), and
$x_{ij}, y_j \in \lbrace 0,1 \rbrace$. Hence, the abstract
representation has two `ColVariable` groups (`x`, a
`boost::multi_array< ColVariable , 2 >` of shape $n \times m$, and
`y`, a `std::vector< ColVariable >` of size $m$), two
`FRowConstraint` groups (assignment and capacity), and one
`FRealObjective`. That this is a formulation of Bin Packing is the
following elementary statement.

<span id="lem-A-1"></span>**Lemma A.1.** *The binary points $(x, y)$
satisfying the constraints above are in one-to-one correspondence with the
pairs (packing, set of bins containing it), where a packing assigns each item
to one of $m$ bins without exceeding $C$; at an optimal solution, $\sum_j y_j$
is the minimum number of bins of a packing.*

*Proof.* Given $(x, y)$, the assignment constraints put each item $i$ in
the unique bin $j$ with $x_{ij} = 1$, and the capacity constraint of bin
$j$ gives $\sum_i s_i x_{ij} \le C y_j \le C$, i.e., a packing; the
bins with $y_j = 1$ contain all the used bins, since a bin with an item
of positive size has $y_j = 1$ by the same constraint. Conversely, a
packing and a set of bins containing the used ones give a feasible
$(x, y)$. Minimizing $\sum_j y_j$ sets $y_j = 0$ on every unused bin,
which leaves the minimum number of used bins over all packings; and
$m = n$ bins suffice, since putting each item alone in a bin is a
packing whenever $s_i \le C$ for all $i$. $\square$

(An item of size 0 may sit in a bin with $y_j = 0$; this changes
nothing in the value.)

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

namespace SMSpp_di_unipi_it
{

class BinPackingBlock : public Block
{
 public:
 explicit BinPackingBlock( Block * father = nullptr ) : Block( father ) { }

 virtual ~BinPackingBlock() {
  // explicitly clear the ThinVarDepInterface fields (the Constraints and
  // the Function of the Objective) before the ColVariable groups they
  // point into are destroyed; see the discussion below
  Constraint::clear( v_assign );
  Constraint::clear( v_cap );
  f_obj.clear();
  }

 void load( Index n , double C , std::vector< double > sizes );

 // physical-representation accessors a specialized Solver reads
 Index get_NItems( void ) const { return( f_n ); }
 double get_Capacity( void ) const { return( f_C ); }
 double get_Size( Index i ) const { return( v_s[ i ] ); }

 void generate_abstract_variables( Configuration * = nullptr ) override;
 void generate_abstract_constraints( Configuration * = nullptr ) override;
 void generate_objective( Configuration * = nullptr ) override;

 // the sense is fixed, also before generate_objective() is called
 int get_objective_sense( void ) const override {
  return( Objective::eMin );
  }

 void chg_size( double new_size , Index item , ModParam issueMod = eNoBlck ,
                ModParam issueAMod = eNoBlck );

 // deposit a solution in compact (physical) form and reflect it into the
 // abstract Variables; implemented and discussed in Appendix B (§B.6)
 void set_assignment( const std::vector< Index > & bin_of );

 bool is_feasible( bool useabstract = false ,
                   Configuration * = nullptr ) override;

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override;

 void load( std::istream & , char = 0 ) override { /* text format */ }
 using Block::load;  // the load() from a file name

 private:
 // decode an abstract Modification into a physical change (§A.6)
 void guts_of_add_Modification( c_p_Mod mod , ChnlName chnl );

 // physical representation
 Index f_n = 0;
 double f_C = 0;
 std::vector< double > v_s;  // item sizes

 // compact (physical) solution: for each item, the index of its bin;
 // a single n-vector, as opposed to the n*m + m abstract binaries (§B.6)
 std::vector< Index > v_bin_of;

 // abstract representation (built on demand)
 boost::multi_array< ColVariable , 2 > v_x;  // x[ i ][ j ]
 std::vector< ColVariable > v_y;            // y[ j ]
 std::vector< FRowConstraint > v_assign;    // one per item
 std::vector< FRowConstraint > v_cap;       // one per bin
 FRealObjective f_obj;

 SMSpp_insert_in_factory_h;  // factory hook (Chapter 18)
 };

}  // namespace
```

```cpp
// BinPackingBlock.cpp
#include "BinPackingBlock.h"

using namespace SMSpp_di_unipi_it;

SMSpp_insert_in_factory_cpp_1( BinPackingBlock );  // Block factory: 1 arg
```

The definitions of the methods given in the rest of this appendix belong to
the same `.cpp` file. The `SMSpp_insert_in_factory_cpp_1` macro (the `_1`
because the factory of `Block` constructs with one argument, the father,
while that of `Solver` uses `_0`) registers the class so that
`Block::new_Block("BinPackingBlock")` works, and runs the class's
`static_initialization()` at start-up
([Chapter 18](18-factories-netcdf.md#ch-18),
[§18.1](18-factories-netcdf.md#sec-18-1)). The class not being a template,
the macro also defines the empty `extern "C"` function
`SMSpp_force_load_BinPackingBlock()`. Two mechanisms (since SMS++ 0.9.0)
keep the registration reachable. In the first, through
`smspp_keep_module()`, the CMake target of a module of the same name (as
`init.sh` makes it), when built as a shared library, has every program
that links it call this function, so that no linker drops the unit
holding the registration (static libraries are linked whole by
`smspp_keep_modules()`, and the makefiles pass `--no-as-needed`, or `-u`
of the same function on Windows; [§18.7](18-factories-netcdf.md#sec-18-7)).
The second lets a program that is not linked to the module at
all still find the class with shared libraries, since `Block::new_Block()`
loads the plugin that registers it when the name is not in the factory
([§18.8](18-factories-netcdf.md#sec-18-8)). This requires the module to
have been built by its CMake target, which writes the table of its
plugin.

The destructor follows a convention of the framework. A `Constraint` (like
any `ThinVarDepInterface`) and the `Variable`s that are active in it are
*doubly* linked, each side holding pointers to the other, and the invariant of
the framework (the documentation of `ThinVarDepInterface`) is that
*`Variable`s are constructed before the objects they are active in and
destroyed after them*. The destructor of a `Constraint` removes it from the
lists of its active `Variable`s, which is useless work when these are about to
be destroyed as well; furthermore, the order in which the fields of a
`:Block` are destroyed (the reverse of the order of declaration, which here
happens to destroy `v_x` and `v_y` last) is fragile to rely on. Hence, the
dependent objects are to be cleared explicitly in the destructor, as that
documentation recommends. `clear()` is a "rough destructor": it empties the object
*without* removing it from the lists of its active `Variable`s, which is safe
because those `Variable`s are destroyed right after, and it saves that work;
`FRealObjective::clear()` empties in the same way the `Function` of the
`Objective`, which its destructor then deletes.
[`Constraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Constraint.h)
provides static `Constraint::clear( ... )` helpers that apply `clear()` to
every group shape (`std::vector`, `std::vector` of `std::vector`,
`boost::multi_array`, `std::list`, and their combinations), so that each
group is cleared with one call.

## A.3 `load()` and the physical representation {#sec-A-3}

`load()` checks the data, erases the previous instance, if any, copies the
new one and, if any `:Solver` is already attached, issues the "nuclear"
`NBModification` ([§4.6](04-block.md#sec-4-6)) to tell it that the instance
has been replaced wholesale. After `load()` a `:Block` is un-configured, as
the documentation of `Block::load()` states: in particular, its abstract
representation is not constructed. Hence, that of a previous instance is
erased, by clearing its objects as the destructor does and unregistering
its groups from the `Block`; otherwise, it would keep the old size, and the
guards of [§A.4](A-writing-block.md#sec-A-4) would prevent its regeneration.

```cpp
void BinPackingBlock::load( Index n , double C , std::vector< double > sizes )
{
 if( sizes.size() != n )
  throw( std::invalid_argument( "BinPackingBlock::load: wrong sizes" ) );

 if( ! v_y.empty() ) {  // erase the abstract representation, if any
  Constraint::clear( v_assign );
  Constraint::clear( v_cap );
  f_obj.clear();
  v_assign.clear();
  v_cap.clear();
  v_x.resize( boost::extents[ 0 ][ 0 ] );
  v_y.clear();
  reset_static_constraints();
  reset_static_variables();
  reset_objective();
  }
 v_bin_of.clear();  // the solution of the previous instance

 f_n = n;
 f_C = C;
 v_s = std::move( sizes );

 if( anyone_there() )  // a Solver is listening
  add_Modification( std::make_shared< NBModification >( this ) );
 }
```

At this point the `BinPackingBlock` is fully usable by a *specialized* solver
([Appendix B](B-writing-solver.md#app-B)) that reads `f_n`, `f_C`, `v_s`
directly. The abstract representation does not exist yet.

## A.4 Generating the abstract representation {#sec-A-4}

The three `generate_*` overrides build the abstract face on demand ([Chapter
7](07-physical-abstract.md#ch-7)). `generate_abstract_variables()`
materializes the `ColVariable` groups, declaring each binary:

```cpp
void BinPackingBlock::generate_abstract_variables( Configuration * )
{
 if( ! v_y.empty() )
  return;  // already done

 v_x.resize( boost::extents[ f_n ][ f_n ] );  // m == n bins
 for( auto && row : v_x )
  for( auto & xij : row )
   xij.set_type( ColVariable::kBinary );

 v_y.resize( f_n );
 for( auto & yj : v_y )
  yj.set_type( ColVariable::kBinary );

 add_static_variable( v_x , "x" );  // register the groups
 add_static_variable( v_y , "y" );
 }
```

`generate_abstract_constraints()` builds the assignment and capacity rows,
each an `FRowConstraint` carrying a linear `Function`. A `Configuration` could
gate which groups are built (as `MCFBlock` and CFL do,
[§7.3](07-physical-abstract.md#sec-7-3)); here we build both:

```cpp
void BinPackingBlock::generate_abstract_constraints( Configuration * )
{
 if( ! v_assign.empty() )
  return;  // already done

 generate_abstract_variables();  // the Variables come before the rows

 // assignment: sum_j x[i][j] == 1  for each item i
 v_assign.resize( f_n );
 for( Index i = 0 ; i < f_n ; ++i ) {
  // a linear Function summing x[i][0..m-1] with coeff 1: build the
  // (Variable* , coefficient) pairs, one per bin j, then hand the new
  // LinearFunction (constant term 0) to the FRowConstraint
  LinearFunction::v_coeff_pair cp( f_n );
  for( Index j = 0 ; j < f_n ; ++j ) {
   cp[ j ].first = &v_x[ i ][ j ];
   cp[ j ].second = 1.0;
   }
  v_assign[ i ].set_function( new LinearFunction( std::move( cp ) , 0 ) ,
                              eNoMod );  // no Modification while generating
  v_assign[ i ].set_both( 1.0 );         // == 1
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
   cp[ i ].first = &v_x[ i ][ j ];
   cp[ i ].second = v_s[ i ];
   }
  cp[ f_n ].first = &v_y[ j ];
  cp[ f_n ].second = -f_C;
  v_cap[ j ].set_function( new LinearFunction( std::move( cp ) , 0 ) ,
                           eNoMod );
  v_cap[ j ].set_lhs( -Inf< double >() );
  v_cap[ j ].set_rhs( 0.0 );  // <= 0
  }

 add_static_constraint( v_assign , "assign" );
 add_static_constraint( v_cap , "cap" );
 }
```

`generate_objective()` sets the minimization of $\sum_j y_j$:

```cpp
void BinPackingBlock::generate_objective( Configuration * )
{
 if( get_objective() )
  return;  // already done

 generate_abstract_variables();

 // linear Function: sum_j 1 * y[j]
 LinearFunction::v_coeff_pair cp( f_n );
 for( Index j = 0 ; j < f_n ; ++j ) {
  cp[ j ].first = &v_y[ j ];
  cp[ j ].second = 1.0;
  }
 f_obj.set_function( new LinearFunction( std::move( cp ) , 0 ) , eNoMod );
 f_obj.set_sense( Objective::eMin , eNoMod );
 set_objective( &f_obj , eNoMod );
 }
```

We remark two things. First, the linear expressions are
`LinearFunction` objects ([Chapter 13](13-function-family.md#ch-13)), built
from a `LinearFunction::v_coeff_pair`, a vector of (`ColVariable *` ,
`Coefficient`) pairs, plus a constant term; ownership of each
`new LinearFunction( ... )` passes to the `FRowConstraint` / `FRealObjective`
that receives it via `set_function()`,
[§5.7](05-variable-constraint-objective.md#sec-5-7). Second, no
`Modification` is issued while the abstract representation is generated, as
the documentation of the `generate_*()` methods of `Block` requires: the calls
of `set_function()`, `set_sense()` and `set_objective()` are passed `eNoMod`
([§8.5](08-modification-janus.md#sec-8-5),
[§8.9](08-modification-janus.md#sec-8-9)), while the other `set_*()` calls
issue nothing since the objects are not yet registered in the `Block`. The
terms of a capacity row are ordered on purpose (the $n$ item coefficients
first, then the single $-C y_j$ term), so that `chg_size`
([§A.5](A-writing-block.md#sec-A-5)) and the abstract-stream decoding in
`add_Modification` ([§A.6](A-writing-block.md#sec-A-6)) can address
coefficient $k$ as "the $x_{k j}$ term".

## A.5 A `chg_*()` method and its `Modification` {#sec-A-5}

A `:Block` author exposes mutators for the data that may change. `chg_size`
changes one item's size in the physical representation and, when the abstract
representation exists, mirrors the change there, issuing the appropriate
`Modification`s on each face ([Chapter 8](08-modification-janus.md#ch-8)).
Giving it one of the methods-factory shapes
([§15.3](15-methods-factory.md#sec-15-3)) would also let it be called by name;
here the single-item form:

```cpp
void BinPackingBlock::chg_size( double new_size , Index item ,
                                ModParam issueMod , ModParam issueAMod )
{
 if( v_s[ item ] == new_size )  return;     // nothing to do
 v_s[ item ] = new_size;                    // physical change

 // physical Modification, for a specialized Solver: issued only if
 // someone listening reads its kind (§C.2)
 if( issue_pmod( issueMod , BinPackingBlockMod::size_changes ) )
  add_Modification( std::make_shared< BinPackingBlockMod >(
                      this , item ) , Observer::par2chnl( issueMod ) );

 // mirror into the abstract representation, if it exists: the size
 // appears as the coefficient of x[item][j] in each capacity row j,
 // which (by construction, §A.4) is coefficient number `item` of that
 // row's LinearFunction. modify_coefficient() updates it and issues the
 // abstract C05FunctionModLin on its own, driven by un_ModBlock( issueAMod )
 // so that the Block does not intercept its own change (§8.4). The
 // not_dry_run() guard is what makes this safe to call from
 // add_Modification() with issueAMod == eDryRun: there the abstract change
 // has *already* happened, so we must touch the physical face only
 if( not_dry_run( issueAMod ) && ( ! v_cap.empty() ) )
  for( Index j = 0 ; j < f_n ; ++j ) {
   auto * lf = static_cast< LinearFunction * >( v_cap[ j ].get_function() );
   lf->modify_coefficient( item , new_size , un_ModBlock( issueAMod ) );
  }
}
```

The `BinPackingBlockMod` is the `:Block`-specific physical
`Modification` defined in [Appendix C](C-writing-modification.md#app-C).

## A.6 `add_Modification()`: catching abstract changes {#sec-A-6}

The Janus discipline ([Chapter 8](08-modification-janus.md#ch-8),
[§8.4](08-modification-janus.md#sec-8-4)) requires that a change made through
the *abstract* face be reflected in the physical one. The `:Block` does this
by overriding `add_Modification()` to intercept the abstract `Modification`s
whose `concerns_Block()` is `true`, apply the matching physical change, reset
the flag, and forward:

```cpp
void BinPackingBlock::add_Modification( sp_Mod mod , ChnlName chnl )
{
 if( mod->concerns_Block() ) {
  mod->concerns_Block( false );
  guts_of_add_Modification( mod.get() , chnl );
  }
 Block::add_Modification( mod , chnl );  // base mechanics (Chapter 8)
 }
```

The decoding is delegated to a private helper. The *only* abstract change this
`:Block` knows how to fold back into its physical data is a change of an
item-size coefficient in a capacity row, i.e., a `C05FunctionModLinRngd` /
`C05FunctionModLinSbst` on one of the `v_cap[j]` `LinearFunction`s, touching
an $x_{ij}$ coefficient (index $< n$). Anything else (a change of the
structural $-C y_j$ coefficient, of an assignment row or of the objective, an
added or removed `Variable`, and so on) has no physical counterpart, and the
helper throws:

```cpp
void BinPackingBlock::guts_of_add_Modification( c_p_Mod mod , ChnlName chnl )
{
 // helper: which capacity row owns this Function? (npos if none)
 auto cap_row = [ this ]( const Function * f ) -> Index {
  for( Index j = 0 ; j < f_n ; ++j )
   if( v_cap[ j ].get_function() == f )
    return( j );
  return( Inf< Index >() );
  };

 // helper: fold one changed coefficient k of a capacity row back into v_s.
 // k must be an x-term (k < f_n); the n-th coefficient is the structural
 // -C * y[j] and must never change through the abstract face
 auto fold = [ & ]( LinearFunction * lf , Index k ) {
  if( k >= f_n )
   throw( std::logic_error( "BinPackingBlock: structural coefficient "
                            "changed through the abstract face" ) );
  // chg_size with eNoBlck on the physical face (tell specialized Solvers)
  // and eDryRun on the abstract face (it has already changed); §8.4
  chg_size( lf->get_coefficient( k ) , k , make_par( eNoBlck , chnl ) ,
            eDryRun );
  };

 if( auto tmod = memo_cast< const C05FunctionModLinRngd * >( mod ) ) {
  auto * lf = memo_cast< LinearFunction * >( tmod->function() );
  if( ( ! lf ) || ( cap_row( lf ) == Inf< Index >() ) )
   throw( std::logic_error( "BinPackingBlock: unsupported abstract change" ) );
  for( Index k = tmod->range().first ; k < tmod->range().second ; ++k )
   fold( lf , k );
  return;
  }

 if( auto tmod = memo_cast< const C05FunctionModLinSbst * >( mod ) ) {
  auto * lf = memo_cast< LinearFunction * >( tmod->function() );
  if( ( ! lf ) || ( cap_row( lf ) == Inf< Index >() ) )
   throw( std::logic_error( "BinPackingBlock: unsupported abstract change" ) );
  for( auto k : tmod->subset() )
   fold( lf , k );
  return;
  }

 // any other abstract Modification has no physical counterpart here
 throw( std::logic_error( "BinPackingBlock: unsupported abstract change" ) );
 }
```

The types are recognized by `memo_cast()` (since SMS++ 0.9.0,
[§8.3](08-modification-janus.md#sec-8-3)), whose hypotheses
([Lemma 8.2](08-modification-janus.md#lem-8-2)) hold here, since the
hierarchies of `Modification` and of `Function` have no virtual base.

Two caveats are in order. First, failing loudly is deliberate: a `:Block` is
free to support only some abstract changes and to throw for the rest, as
`MCFBlock` does for the addition and removal of arcs
([§8.4](08-modification-janus.md#sec-8-4)). Second, there is a subtlety
peculiar to this formulation: the size $s_i$ is shared by all the $m$ capacity
rows, but the abstract face exposes it as $m$ independent coefficients. When
the change arrives on a single row $j^{\star}$, `fold` calls `chg_size` with
`eDryRun`, which (by the guard of [§A.5](A-writing-block.md#sec-A-5)) updates
only the physical `v_s`, the row $j^{\star}$ having already changed, and
leaves the other rows with the old size; hence, after such a change the
abstract representation no longer encodes a Bin Packing problem in the sense
of [Lemma A.1](A-writing-block.md#lem-A-1). A production-grade
`BinPackingBlock` would either reject per-row coefficient edits or propagate
the new size to all the $m$ rows; the minimal example shows the mechanism and
states the consequence. We also remark that this `:Block` intercepts only the
abstract `Modification`s with `concerns_Block()` `true`, as the scheme of
`Block::add_Modification()` prescribes. A `:Block` that can be the inner
`Block` of a `LagBFunction` and has a specialized `:Solver` has to translate
also the changes of its `Objective`, which the `LagBFunction` issues with
`concerns_Block()` `false` when it moves the Lagrangian costs, as
`BinaryKnapsackBlock` does. The case does not arise here, since the
`Objective` of `BinPackingBlock` has no physical counterpart.

## A.7 `is_feasible()` on the physical representation {#sec-A-7}

Finally, `is_feasible()` checks the solution that the `Variable`s of the
`Block` encode ([§7.5](07-physical-abstract.md#sec-7-5)). Written on the
*physical* representation it is cheap: it takes the bin of each item from the
abstract `x` `Variable`s if they have been generated, since every `:Solver`
writes its solution there, and otherwise from the compact solution `v_bin_of`
(set by `set_assignment()`,
[§B.6](B-writing-solver.md#sec-B-6)), and verifies that every item is in one
bin and that no bin exceeds the capacity. The parameter `useabstract`, by
which the check would be made on the abstract representation, is only a hint
that a `:Block` may ignore (the documentation of `Block::is_feasible()`
allows it), and this one does.

```cpp
bool BinPackingBlock::is_feasible( bool /* useabstract */ , Configuration * )
{
 // the bin of each item: read off the x, or the compact solution
 auto bin_of = v_bin_of;
 if( ! v_y.empty() ) {
  bin_of.assign( f_n , f_n );  // f_n means "no bin"
  for( Index i = 0 ; i < f_n ; ++i )
   for( Index j = 0 ; j < f_n ; ++j )
    if( v_x[ i ][ j ].get_value() > 0.5 ) {  // item i in bin j
     if( bin_of[ i ] < f_n )
      return( false );  // assigned twice
     bin_of[ i ] = j;
     }
  }

 if( bin_of.size() != f_n )  // no solution yet
  return( false );

 std::vector< double > load( f_n , 0.0 );
 for( Index i = 0 ; i < f_n ; ++i ) {
  if( bin_of[ i ] >= f_n )
   return( false );  // unassigned
  load[ bin_of[ i ] ] += v_s[ i ];
  }

 for( Index j = 0 ; j < f_n ; ++j )
  if( load[ j ] > f_C + 1e-9 )
   return( false );  // over capacity

 return( true );
 }
```

The check reads the `Variable`s but not the `FRowConstraint`s; a `:Block`
that honors `useabstract` walks its `FRowConstraint`s and checks their values
against their sides (the base class does not do it, since its default only
queries the sub-`Block`s). By [Lemma A.1](A-writing-block.md#lem-A-1) the two checks agree
on binary points, up to the tolerance. The absolute tolerance $10^{-9}$ is
hard-coded here, where a production `:Block` would read it from the argument
`fsbc`, else from the `f_is_feasible_Configuration` of its `BlockConfig`
([§11.3](11-configuration.md#sec-11-3)), and with neither would use the
relative tolerance `Block::DefaultFeasTol` ($10^{-12}$).

## A.8 Optional pieces (sketched) {#sec-A-8}

A minimum-viable `:Block` ends at [§A.7](A-writing-block.md#sec-A-7). A
"complete" `:Block` typically adds, each only when the use case calls for it:

- the netCDF format of the `:Block`
  ([Chapter 18](18-factories-netcdf.md#ch-18)), which the tools and the
  batteries of `tests/` read: `serialize( netCDF::NcGroup & )` writes the
  physical data in a group, and `deserialize( const netCDF::NcGroup & )`
  reads them back, ending with the call of `Block::deserialize( group )`,
  which issues the `NBModification` and therefore has to come after the
  data have changed; `print()` gives a human-readable form of the instance;
- a `:Solution` ([Chapter 9](09-solution.md#ch-9)), a
  `BinPackingSolution` storing the bin of each item, so that a solution
  can be saved, restored and combined without going through the
  abstract `Variable`s;
- an R3 Block ([Chapter 10](10-r3block.md#ch-10)), at least the copy,
  through `get_R3_Block()`, `map_back_solution()` and
  `map_forward_solution()` (the mirror of
  [§10.4](10-r3block.md#sec-10-4) being available anyway);
- a `:Change` family ([Chapter 16](16-change.md#ch-16)), for
  serializable or undoable edits
  ([Appendix C](C-writing-modification.md#app-C)).

None of these is required to have a working, solver-attachable
`:Block`; they are the difference between a teaching example and a
production module.

## A.9 Checklist {#sec-A-9}

To recapitulate, a minimum-viable `:Block` needs:

1. a class deriving from `Block`, with a father-taking constructor and the
   `SMSpp_insert_in_factory_h` / `_cpp_k` macros
   ([§A.2](A-writing-block.md#sec-A-2), [Chapter
   18](18-factories-netcdf.md#ch-18));
2. the physical representation as member data, populated by
   `load()` / `deserialize()`, issuing `NBModification` on reload
   ([§A.3](A-writing-block.md#sec-A-3));
3. the `generate_abstract_*()` overrides building the abstract
   face on demand ([§A.4](A-writing-block.md#sec-A-4));
4. `chg_*()` mutators issuing physical (and, where relevant,
   abstract) `Modification`s ([§A.5](A-writing-block.md#sec-A-5));
5. `add_Modification()` catching abstract changes and keeping the physical
   face in sync, or throwing for unsupported ones
   ([§A.6](A-writing-block.md#sec-A-6));
6. `is_feasible()` on the physical representation
   ([§A.7](A-writing-block.md#sec-A-7)), and, for an optimization
   `:Block`, `get_objective_sense()` if the sense is known before the
   `Objective` is generated ([§A.2](A-writing-block.md#sec-A-2)); the
   valid bounds `get_valid_lower_bound()` and `get_valid_upper_bound()`,
   which the example omits, are optional.
