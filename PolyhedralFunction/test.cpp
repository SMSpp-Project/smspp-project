/*--------------------------------------------------------------------------*/
/*-------------------------- File test.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing PolyhedralFunction
 *
 * A "random" PolyhedralFunction is constructed and put as the only Objective
 * of an otherwise "empty" Block. The same PolyhedralFunction is represented
 * in terms of linear inequalities for another otherwise "empty" Block. The
 * two Block are solved by a BundleSolver and a MILPSolver, respectively,
 * and the results are compared. The two Block are then repeatedly randomly
 * modified "in the same way", and re-solved several times.
 *
 * \version 1.30
 *
 * \date 10 - 11 - 2020
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- MACROS ----------------------------------*/
/*--------------------------------------------------------------------------*/

#define LOG_LEVEL 0
// 0 = only pass/fail
// 1 = result of each test
// 2 = + solver log
// 3 = + save LP file
// 4 = + print data

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) cout << x
 #define CLOG1( y , x ) if( y ) cout << x

 #if( LOG_LEVEL >= 2 )
  #define LOG_ON_COUT 1
  // if nonzero, the BundleSolver log is sent on cout rather than on a file
 #endif
#else
 #define LOG1( x )
 #define CLOG1( y , x )
#endif

/*--------------------------------------------------------------------------*/

#define HAVE_CONSTRAINTS 2
// if HAVE_CONSTRAINTS == 1, then about 50% of the variables will have a
// non-negativity constraint implemented via ColVariable::is_positive()
// if HAVE_CONSTRAINTS == 2, then about 50% of the variables will have
// bound constraints; of these, 33% will only have 0 lower bound, 33% will
// only have random upper bound, and the rest will have both. of the
// remaining 50% of the variables, another 50%  will have a
// non-negativity constraint implemented via ColVariable::is_positive()

/*--------------------------------------------------------------------------*/

#define DETACH_NDO 0
// if nonzero, the Solver attched to the NDOBlock is detached and re-attached
// to it at all iterations

#define DETACH_LP 0
// if nonzero, the Solver attched to the LPBlock is detached and re-attached
// to it at all iterations

/*--------------------------------------------------------------------------*/

#define SKIP_BEAT 0
// if nonzero, the two Block are not solved at every round of changes, but
// only every SKIP_BEAT + 1 rounds. this allows changes to accumulate, and
// therefore puts more pressure on the Modification handling of the Solver
// (in case this tries to do "smart" things rather than dumbly processing
// each one in turn)
//
// note that the number of rounds of changes is them multiplied by
// SKIP_BEAT + 1, so that the input parameter still dictates the number of
// Block solutions

/*--------------------------------------------------------------------------*/

#define PANICMSG { cout << endl << "something very bad happened!" << endl; \
		   exit( 1 ); \
                   }

#define PANIC( x ) if( ! ( x ) ) PANICMSG

#define USECOLORS 1
#if( USECOLORS )
 #define RED( x ) "\x1B[31m" #x "\033[0m"
 #define GREEN( x ) "\x1B[32m" #x "\033[0m"
#else
 #define RED( x ) #x
 #define GREEN( x ) #x
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define DYNAMIC_VARS 0
// if 1, half of the variables are dynamic

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <fstream>
#include <sstream>
#include <iomanip>

#include <random>

#include "AbstractBlock.h"

#include "BlockSolverConfig.h"

#include "BundleSolver.h"

#if( LOG_LEVEL >= 3 )
 #include "CPXMILPSolver.h"
#endif

#include "PolyhedralFunction.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace std;
using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

using Index = Block::Index;
using c_Index = Block::c_Index;

using Range = Block::Range;
using c_Range = Block::c_Range;

using Subset = Block::Subset;
using c_Subset = Block::c_Subset;

using FunctionValue = Function::FunctionValue;
using c_FunctionValue = Function::c_FunctionValue;

using MultiVector = PolyhedralFunction::MultiVector;
using RealVector = PolyhedralFunction::RealVector;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

const double scale = 10;
const char *const logF = "log.bn";

const FunctionValue INF = SMSpp_di_unipi_it::Inf< FunctionValue >();

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

AbstractBlock * LPBlock;   // the problem expressed as an LP

AbstractBlock * NDOBlock;  // the problem expressed via PolyhedralFunction

bool convex = true;        // true if the PolyhedralFunction is convex

double bound = 1000;       // a tentative bound to detect unbounded instances

FunctionValue BND;         // the bound in the PolyhedralFunction (if any)

Index nvar = 10;           // number of variables
#if DYNAMIC_VARS > 0
 Index nsvar;              // number of static variables
 Index ndvar;              // number of dynamic variables
#else
 #define nsvar nvar        // all variables are static
#endif

Index m;                   // number of rows

std::mt19937 rg;           // base random generator
std::uniform_real_distribution<> dis( 0.0 , 1.0 );

MultiVector A;
RealVector b;

ColVariable * vLP;                 // pointer to v LP variable

std::vector< ColVariable > * xLP;  // pointer to (static) x LP variables
#if DYNAMIC_VARS > 0
 std::list< ColVariable > * xLPd;  // pointer to (dynamic) x LP variables
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

// convex ==> minimize ==> negative numbers

static double rs( double x ) { return( convex ? -x : x ); }

/*--------------------------------------------------------------------------*/

template<class T>
static void Str2Sthg( const char* const str , T &sthg )
{
 istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/

static double rndfctr( void )
{
 // return a random number between 0.5 and 2, with 50% probability of being
 // < 1
 double fctr = dis( rg ) - 0.5;
 return( fctr < 0 ? - fctr : fctr * 4 );
 }

/*--------------------------------------------------------------------------*/

static void GenerateA( Index nr , Index nc )
{
 A.resize( nr );

 for( auto & Ai : A ) {
  Ai.resize( nc );
  for( auto & aij : Ai )
   aij = scale * ( 2 * dis( rg ) - 1 );
  }
 }

/*--------------------------------------------------------------------------*/

static void Generateb( Index nr )
{
 b.resize( nr );

 for( auto & bj : b )
  bj = scale * nvar * ( 2 * dis( rg ) - 1 ) / 4;
 }

/*--------------------------------------------------------------------------*/

static void GenerateAb( Index nr , Index nc )
{
 // rationale: the solution x^* will be more or less the solution of some
 // square sub-system A_B x = b_B. We want x^* to be "well scaled", i.e.,
 // the entries to be ~= 1 (in absolute value). The average of each row A_i
 // is 0, the maximum (and minimum) expected value is something like
 // scale * nvar / 2. So we take each b_j in +- scale * nvar / 4

 GenerateA( nr , nc );
 Generateb( nr );
 }

/*--------------------------------------------------------------------------*/

static void GenerateBND( void )
{
 // rationale: we expect the solution x^* to have entries ~= 1 (in absolute
 // value, and the coefficients of A are <= scale (in absolute value), so
 // the LHS should be at most around - scale * nvar; the RHS can add it
 // a further - scale * nvar / 4, so we expect - (5/4) * scale * nvar to
 // be a "natural" LB. We therefore set the LB to a mean of 1/2 of that
 // (tight) 33% of the time, a mean of 2 times that (loose) 33% of the time,
 // and -INF the rest

 if( dis( rg ) <= 0.333 ) {   // "tight" bound
  BND = rs( dis( rg ) * 5 * scale * nvar / 4 );
  return;
  }

 if( dis( rg ) <= 0.333 ) {  // "loose" bound
  BND = rs( dis( rg ) * 5 * scale * nvar );
  return;
  }

 BND = INF;
 }

/*--------------------------------------------------------------------------*/

static Subset GenerateRand( Index m , Index k )
{
 // generate a sorted random k-vector of unique integers in 0 ... m - 1

 Subset rnd( m );
 std::iota( rnd.begin() , rnd.end() , 0 );
 std::shuffle( rnd.begin() , rnd.end() , rg );    
 rnd.resize( k );
 sort( rnd.begin() , rnd.end() );

 return( std::move( rnd ) );
 }

/*--------------------------------------------------------------------------*/

static void ConstructLPConstraint( Index i , FRowConstraint & ci ,
				   bool setblock = true )
{
 // construct constraint ci out of A[ i ] and b[ i ]:
 //
 // in the convex case, the constraint is
 //
 //          b[ i ] <= vLP - \sum_j Ai[ j ] * xLP[ j ] <= INF
 //
 // in the concave case, the constraint is
 //
 //          - INF <= vLP - \sum_j Ai[ j ] * xLP[ j ] <= b[ i ]
 //
 // note: constraints are constructed dense (elements == 0, which are
 //       anyway quite unlikely, are ignored) to make things simpler
 //
 // note: variable x[ i ] is given index i + 1, variable v has index 0

 if( convex ) {
  ci.set_lhs( b[ i ] );
  ci.set_rhs( INF );
  }
 else {
  ci.set_lhs( - INF );
  ci.set_rhs( b[ i ] );
  }
 LinearFunction::v_coeff_pair vars( nvar + 1 );
 Index j = 0;

 // first, v
 vars[ j ] = std::make_pair( vLP , 1 );

 // then, static x
 for( ; j < nsvar ; ++j )
  vars[ j + 1 ] = std::make_pair( &((*xLP)[ j ] ) , - A[ i ][ j ] );

 #if DYNAMIC_VARS > 0
  // finally, dynamic x
  auto xLPdit = xLPd.begin();
  for( ; j < nvar ; ++j , ++xLPdit )
   vars[ j + 1 ] = std::make_pair( &(*xLPdit) , - A[ i ][ j ] );
 #endif

 ci.set_function( new LinearFunction( std::move( vars ) ) );
 if( setblock )
  ci.set_Block( LPBlock );
 }

/*--------------------------------------------------------------------------*/

static void ChangeLPConstraint( Index i , FRowConstraint & ci , ModParam iAM )
{
 // change the constant == LHS or RHS of the constraint (depending on convex)
 if( convex )
  ci.set_lhs( b[ i ] , iAM );
 else
  ci.set_rhs( b[ i ] , iAM );

 // now change the coefficients, except that of v that is always 1
 LinearFunction::Vec_FunctionValue coeffs( nvar );

 for( Index j = 0 ; j < nvar ; ++j )
  coeffs[ j ] = - A[ i ][ j ];

 auto f = static_cast< LinearFunction * >( ci.get_function() );
 f->modify_coefficients( std::move( coeffs ) , Range( 1 , nvar + 1 ) , iAM );
 }

/*--------------------------------------------------------------------------*/

static void SetGlobalBound( void )
{
 if( BND == INF )
  if( convex )
   NDOBlock->set_valid_lower_bound( -bound , true );
  else
   NDOBlock->set_valid_upper_bound( bound , true );
 else
  if( convex )
   NDOBlock->set_valid_lower_bound( -BND );
  else
   NDOBlock->set_valid_upper_bound( BND );
 }

/*--------------------------------------------------------------------------*/

static void printAb( const MultiVector & tA , const RealVector & tb ,
		     double bound )
{
 PANIC( ( tA.size() == tb.size() ) || ( tA.size() + 1 == tb.size() ) );
 PANIC( tA.size() == m );
 for( auto & tai : tA )
  PANIC( tai.size() == nvar );

 cout << "n = " << nvar << ", m = " << m;
 if( std::abs( bound ) == INF )
  cout << " (no bound)" << endl;
 else
  cout << ", bound = " << bound << endl;

 for( Index i = 0 ; i < m ; ++i ) {
  cout << "A[ " << i << " ] = [ ";
  for( Index j = 0 ; j < nvar ; ++j )
   cout << tA[ i ][ j ] << " ";
   cout << "], b[ " << i << " ] = " << tb[ i ] << endl;
  }
 }

/*--------------------------------------------------------------------------*/

static bool SolveBoth( void ) 
{
 try {
  // solve the LPBlock- - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Solver * slvrLP = (LPBlock->get_registered_solvers()).front();
  #if DETACH_LP
   LPBlock->unregister_Solver( slvrLP );
   LPBlock->register_Solver( slvrLP , true );  // push it to the front
  #endif
  int rtrnLP = slvrLP->compute( false );
  bool hsLP = ( ( rtrnLP >= Solver::kOK ) && ( rtrnLP < Solver::kError ) )
              || ( rtrnLP == Solver::kLowPrecision );
  double foLP = hsLP ? ( convex ? slvrLP->get_ub() : slvrLP->get_lb() )
                     : ( convex ? INF : -INF );

  // solve the NODBlock - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Solver * slvrNDO = (NDOBlock->get_registered_solvers()).front();
  #if DETACH_NDO
   NDOBlock->unregister_Solver( slvrNDO );
   NDOBlock->register_Solver( slvrNDO );
  #endif
  int rtrnNDO = slvrNDO->compute( false );
  bool hsNDO = ( ( rtrnNDO >= Solver::kOK ) && ( rtrnNDO < Solver::kError ) )
              || ( rtrnNDO == Solver::kLowPrecision );
  double foNDO = hsNDO ? ( convex ? slvrNDO->get_ub() : slvrNDO->get_lb() )
                       : ( convex ? INF : -INF );

  if( hsLP && hsNDO && ( abs( foLP - foNDO ) <= 2e-7 *
			 max( double( 1 ) , abs( max( foLP , foNDO ) ) ) ) ) {
   LOG1( "OK(f)" << endl );
   return( true );
   }

  if( hsLP && ( rtrnNDO == Solver::kUnbounded ) ) {
   /* Weird case: the LP found an optimal solution but the NDO declared the
    * problem unbounded below. This may be because the tentative lb is too
    * high, check it this actually is the case and if so declare the
    * run a success (but also decrease the lb). */
   if( ( convex && ( foNDO <= bound * ( 1 + 1e-9 ) ) ) ||
       ( ( ! convex ) && ( foNDO >= bound * ( 1 + 1e-9 ) ) ) ) {
    LOG1( "OK(?bound?)" << endl );
    bound *= 2;
    if( convex )
     NDOBlock->set_valid_lower_bound( -bound );
    else
     NDOBlock->set_valid_upper_bound( bound );
    return( true );
    }
   }

  if( ( rtrnLP == Solver::kInfeasible ) &&
      ( rtrnNDO == Solver::kInfeasible ) ) {
    LOG1( "OK(?e?)" << endl );
    return( true );
    }

  if( ( rtrnLP == Solver::kUnbounded ) &&
      ( rtrnNDO == Solver::kUnbounded ) ) {
   LOG1( "OK(u)" << endl );
   return( true );
   }

  #if( LOG_LEVEL >= 1 )
   cout << "LPBlock = ";
   if( hsLP )
    cout << foLP;
   else
    if( rtrnLP == Solver::kInfeasible )
     cout << "    Unfeas(?)";
    else
     if( rtrnLP == Solver::kUnbounded )
      cout << "      Unbounded";
     else
      cout << "      Error!";

   cout << " ~ NDOBlock = ";
   if( hsNDO )
    cout << foNDO;
   else
    if( rtrnNDO == Solver::kInfeasible )
     cout << "    Unfeas(?)";
    else
     if( rtrnNDO == Solver::kUnbounded )
      cout << "      Unbounded";
     else
      cout << "      Error!";
   cout << endl;
  #endif

  return( false );
  }
 catch( exception &e ) {
  cerr << e.what() << endl;
  exit( 1 );
  }
 catch(...) {
  cerr << "Error: unknown exception thrown" << endl;
  exit( 1 );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char **argv )
{
 // reading command line parameters - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 assert( SKIP_BEAT >= 0 );

 long int seed = 0;
 Index wchg = 127;
 double dens = 4;  
 double p_change = 0.5;
 Index n_change = 10;
 Index n_repeat = 40;

 switch( argc ) {
  case( 8 ): Str2Sthg( argv[ 7 ] , p_change );
  case( 7 ): Str2Sthg( argv[ 6 ] , n_change );
  case( 6 ): Str2Sthg( argv[ 5 ] , n_repeat );
  case( 5 ): Str2Sthg( argv[ 4 ] , dens );
  case( 4 ): Str2Sthg( argv[ 3 ] , nvar );
  case( 3 ): Str2Sthg( argv[ 2 ] , wchg );
  case( 2 ): Str2Sthg( argv[ 1 ] , seed );
             break;
  default: cerr << "Usage: " << argv[ 0 ] <<
	   " seed [wchg nvar dens #rounds #chng %chng]"
 		<< endl <<
           "       wchg: what to change, coded bit-wise [127]"
		<< endl <<
           "             0 = add rows, 1 = delete rows "
		<< endl <<
           "             2 = modify rows, 3 = modify constants"
		<< endl <<
           "             4 = change global lower/upper bound"
          #if DYNAMIC_VARS > 0
		<< endl <<
           "             5 = add variables, 6 = delete variables"
	  #endif
	        << endl <<
           "       nvar: number of variables [10]"
	        << endl <<
           "       dens: rows / variables [4]"
	        << endl <<
           "       #rounds: how many iterations [40]"
	        << endl <<
           "       #chng: number changes [10]"
	        << endl <<
           "       %chng: probability of changing [0.5]"
	        << endl;
	   return( 1 );
  }

 if( nvar < 1 ) {
  cout << "error: nvar too small";
  exit( 1 );
  }

 #if DYNAMIC_VARS > 0
  nsvar = nvar / 2;      // half of the variables are dynamic
  ndvar = nvar - nsvar;  // the other half are static
 #endif

 m = nvar * dens;
 if( m < 1 ) {
  cout << "error: dens too small";
  exit( 1 );
  }

 rg.seed( seed );  // seed the pseudo-random number generator

 // constructing the data of the problem- - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // choosing whether convex or concave: toss a(n unbiased, two-sided) coin
 convex = ( dis( rg ) < 0.5 );

 // construct the matrix m x nvar matrix A and the m-vector b

 GenerateAb( m , nvar );
 GenerateBND();

 cout.setf( ios::scientific, ios::floatfield );
 cout << setprecision( 10 );

 #if( LOG_LEVEL >= 4 )
  printAb( A , b , BND );
 #endif

 // construction and loading of the objects - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // construct the LP- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  LPBlock = new AbstractBlock();

  // construct the Variable
  xLP = new std::vector< ColVariable >( nsvar );
  #if DYNAMIC_VARS > 0
   xLPd = new std::list< ColVariable >( ndvar );
  #endif

  vLP = new ColVariable;
  vLP->set_Block( LPBlock );

  // construct the m dynamic Constraint
  auto ALP = new std::list< FRowConstraint >( m );
  auto ALPit = ALP->begin();
  for( Index i = 0 ; i < m ; )
   ConstructLPConstraint( i++ , *(ALPit++) );

  // construct the static lower bound Constraint
  auto LBc = new BoxConstraint( LPBlock , vLP , -INF , INF );
  if( BND != INF ) {
   if( convex )
    LBc->set_lhs( -BND );
   else
    LBc->set_rhs( BND );
   }

  // construct the Objective
  auto objLP = new FRealObjective();
  objLP->set_function( new LinearFunction( { std::make_pair( vLP , 1 ) } ) );
  objLP->set_sense( convex ? Objective::eMin : Objective::eMax , eNoMod );
  
  // now set the Variable, Constraint and Objective in the AbstractBlock
  LPBlock->add_static_variable( *vLP );
  #if DYNAMIC_VARS > 0
   LPBlock->add_dynamic_variable( *xLPd );
  #endif
  LPBlock->add_static_variable( *xLP , "x" );
  LPBlock->add_dynamic_constraint( *ALP );
  LPBlock->add_static_constraint( *LBc );
  LPBlock->set_objective( objLP );
  }

 // construct the NDO problem - - - - - - - - - - - - - - - - - - - - - - - -
 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  NDOBlock = new AbstractBlock();

  // construct the Variable
  auto xNDO = new std::vector< ColVariable >( nsvar );
  PolyhedralFunction::VarVector vars( nvar );
  auto vit = vars.begin();
  for( auto & xi : *xNDO )
   *(vit++) = & xi;
  #if DYNAMIC_VARS > 0
   auto xNDOd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xNDOd )
    *(vit++) = & xi;
  #endif

  // construct the Objective
  auto PF = new PolyhedralFunction( std::move( vars ) ,	std::move( A ) ,
				    std::move( b ) );
  if( BND != INF )
   PF->modify_bound( rs( BND ) );
  PF->set_is_convex( convex , eNoMod );
  auto objNDO = new FRealObjective();
  objNDO->set_function( PF );
  objNDO->set_sense( convex ? Objective::eMin : Objective::eMax , eNoMod );

  // now set the Variable and Objective in the AbstractBlock
  NDOBlock->add_static_variable( *xNDO , "x" );
  #if DYNAMIC_VARS > 0
   NDOBlock->add_dynamic_variable( *xNDOd );
  #endif
  NDOBlock->set_objective( objNDO );

  SetGlobalBound();  // set lower bound, be it "hard" or "conditional"
  }

 // define bound constraints- - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 #if HAVE_CONSTRAINTS == 1
 {
  auto LPx = LPBlock->get_static_variable_v< ColVariable >( "x" );
  auto NDOx = NDOBlock->get_static_variable_v< ColVariable >( "x" );
  for( Index i = 0 ; i < nvar ; ++i )
   if( dis( rg ) < 0.5 ) {
    (*LPx)[ i ].is_positive( true , eNoMod );
    (*NDOx)[ i ].is_positive( true , eNoMod );
    }
  }
 #endif
 #if HAVE_CONSTRAINTS == 2
 {
  auto LPx = LPBlock->get_static_variable_v< ColVariable >( "x" );
  auto NDOx = NDOBlock->get_static_variable_v< ColVariable >( "x" );
  auto LPbnd = new std::list< BoxConstraint >;
  auto NDObnd = new std::list< BoxConstraint >;
  for( Index i = 0 ; i < nvar ; ++i )
   if( dis( rg ) < 0.5 ) {
    LPbnd->resize( LPbnd->size() + 1 );
    NDObnd->resize( NDObnd->size() + 1 );
    LPbnd->back().set_variable( & (*LPx)[ i ] );
    NDObnd->back().set_variable( & (*NDOx)[ i ] );
    auto p = dis( rg );
    auto lhs = p < 0.666 ? 0 : -INF;
    auto rhs = p > 0.333 ? dis( rg ) : INF;
    LPbnd->back().set_lhs( lhs , eNoMod );
    NDObnd->back().set_lhs( lhs , eNoMod );
    LPbnd->back().set_rhs( rhs , eNoMod );
    NDObnd->back().set_rhs( rhs , eNoMod );
    }
   else
    if( dis( rg ) < 0.5 ) {
     (*LPx)[ i ].is_positive( true , eNoMod );
     (*NDOx)[ i ].is_positive( true , eNoMod );
     }

  LPBlock->add_dynamic_constraint( *LPbnd );
  NDOBlock->add_dynamic_constraint( *NDObnd );
  }
 #endif
 
 // attach the Solver to the Block- - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 {
  // for LPBlock do this by reading an appropriate BlockSolverConfig from
  // file and apply() it to the LPBlock
  ifstream MILPParFile( "MILPPar.txt" );
  if( ! MILPParFile.is_open() ) {
   cerr << "Error: cannot open file MILPPar.txt" << endl;
   return( 1 );
   }

  auto msc = new BlockSolverConfig;
  MILPParFile >> *( msc );
  MILPParFile.close();

  msc->apply( LPBlock );
  delete msc;
  }

 {
  // for NDOBlock do this by reading appropriate BlockSolverConfig from
  // files and apply() it to the NDOBlock
  ifstream BundleParFile( "BundlePar.txt" );
  if( ! BundleParFile.is_open() ) {
   cerr << "Error: cannot open file BundlePar.txt" << endl;
   return( 1 );
   }

  auto bsc = new BlockSolverConfig;
  BundleParFile >> *( bsc );
  BundleParFile.close();

  bsc->apply( NDOBlock );
  delete bsc;
  }

 // open log-file - - - - - - - - - - -  - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 #if( LOG_LEVEL >= 2 )
  #if( LOG_ON_COUT )
   ((NDOBlock->get_registered_solvers()).front())->set_log( &cout );
  #else
   ofstream LOGFile( logF , ofstream::out );
   if( ! LOGFile.is_open() )
    cerr << "Warning: cannot open log file """ << logF << """" << endl;
   else {
    LOGFile.setf( ios::scientific, ios::floatfield );
    LOGFile << setprecision( 10 );
    ((NDOBlock->get_registered_solvers()).front())->set_log( &LOGFile );
    }
  #endif

  #if( LOG_LEVEL >= 3 )
   ((LPBlock->get_registered_solvers()).front())->set_par(
	                      CPXMILPSolver::strOutputFile , "LPBlock.lp" );
  #endif
 #endif

 // first solver call - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LOG1( "First call: " );

 bool AllPassed = SolveBoth();
 
 // main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // now, for n_repeat times:
 // - up to n_change rows are added
 // - up to n_change rows are deleted
 // - up to n_change rows are modified
 // - up to n_change rows are modified
 //
 // then the two problems are re-solved

 for( Index rep = 0 ; rep < n_repeat * ( SKIP_BEAT + 1 ) ; ) {
  LOG1( rep << ": ");

  // add rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 1 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = Index( dis( rg ) * n_change ) ) {
    LOG1( "added " << tochange << " rows - " );

    GenerateAb( tochange , nvar );

    // add them to the LP
    vLP = LPBlock->get_static_variable< ColVariable >( 0 );
    xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
    #if DYNAMIC_VARS > 0
     xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
    #endif

    std::list< FRowConstraint > nc( tochange );
    auto ncit = nc.begin();
    for( Index i = 0 ; i < tochange ; )
     ConstructLPConstraint( i++ , *(ncit++) );
    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );
    LPBlock->add_dynamic_constraints( *cnst , nc );

    // add them to the NDO
    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );
     
    if( tochange == 1 )
     PF->add_row( std::move( A[ 0 ] ) , b[ 0 ] );
    else
     PF->add_rows( std::move( A ) , b );

    // update m
    m += tochange;

    // sanity checks
    PANIC( m == PF->get_A().size() );
    PANIC( m == cnst->size() );
    }

  // delete rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 2 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = min( m - 1 , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "deleted " << tochange << " rows" );

    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );
    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );
    
    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     LOG1( "(r) - " );

     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     // remove them from the LP
     LPBlock->remove_dynamic_constraints( *cnst , Range( strt , stp ) );
 
     // remove them from the NDO
     if( tochange == 1 )
      PF->delete_row( strt );
     else
      PF->delete_rows( Range( strt , stp ) );
     }
    else {  // in the other 50% of the cases, do a sparse change
     LOG1( "(s) - " );
     Subset nms( GenerateRand( m , tochange ) );

     // remove them from the LP
     if( tochange == 1 )
      LPBlock->remove_dynamic_constraint( *cnst , std::next( cnst->begin() ,
							     nms[ 0 ] ) );
     else
      LPBlock->remove_dynamic_constraints( *cnst , Subset( nms ) , true );
    
     // remove them from the NDO
     if( tochange == 1 )
      PF->delete_row( nms[ 0 ] );
     else
      PF->delete_rows( std::move( nms ) , true );
     }

    // update m
    m -= tochange;

    // sanity checks
    PANIC( m == PF->get_A().size() );
    PANIC( m == cnst->size() );
    }

  // modify rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 4 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = std::min( m , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "modified " << tochange << " rows" );

    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );

    GenerateAb( tochange , nvar );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     LOG1( "(r) - " );

     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     // modify them in the LP
     vLP = LPBlock->get_static_variable< ColVariable >( 0 );
     xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
     #if DYNAMIC_VARS > 0
      xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     #endif
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     // send all the Modification to the same channel
     Observer::ChnlName chnl = LPBlock->open_channel();
     const auto iAM = Observer::make_par( eModBlck , chnl );

     auto cit = std::next( cnst->begin() , strt );
     for( Index i = 0 ; i < tochange ; ++i )
      ChangeLPConstraint( i , *(cit++) , iAM );

     LPBlock->close_channel( chnl );

     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_row( strt , std::move( A[ 0 ] ) , b[ 0 ] );
     else
      PF->modify_rows( std::move( A ) , b , Range( strt , stp ) );
     }
    else {  // in the other 50% of the cases, do a sparse change
     LOG1( "(s) - " );
     Subset nms( GenerateRand( m , tochange ) );

     // modify them in the LP
     vLP = LPBlock->get_static_variable< ColVariable >( 0 );
     xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
     #if DYNAMIC_VARS > 0
      xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     #endif
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     // send all the Modification to the same channel
     Observer::ChnlName chnl = LPBlock->open_channel();
     const auto iAM = Observer::make_par( eModBlck , chnl );

     Index prev = 0;
     auto cit = cnst->begin();
     for( Index i = 0 ; i < tochange ; ++i ) {
      cit = std::next( cit , nms[ i ] - prev );
      prev = nms[ i ];
      ChangeLPConstraint( i , *cit , iAM );
      }

     LPBlock->close_channel( chnl );

     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_row( nms[ 0 ] , std::move( A[ 0 ] ) , b[ 0 ] );
     else
      PF->modify_rows( std::move( A ) , b , std::move( nms ) , true );
     }
    }

  // modify constants - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 8 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = std::min( m , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "modified " << tochange << " constants" );

    Generateb( tochange );

    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );
     
    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     LOG1( "(r) - " );

     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     // change them in the LP
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     auto cit = std::next( cnst->begin() , strt );
     if( convex )
      for( Index i = 0 ; i < tochange ; )
       (*(cit++)).set_lhs( b[ i++ ] );
     else
      for( Index i = 0 ; i < tochange ; )
       (*(cit++)).set_rhs( b[ i++ ] );
 
     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_constant( strt , b[ 0 ] );
     else
      PF->modify_constants( b , Range( strt , stp ) );
     }
    else {  // in the other 50% of the cases, do a sparse change
     LOG1( "(s) - " );
     Subset nms( GenerateRand( m , tochange ) );

     // change them in the LP
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     Index prev = 0;
     auto cit = cnst->begin();
     if( convex )
      for( Index i = 0 ; i < tochange ; ) {
       cit = std::next( cit , nms[ i ] - prev );
       prev = nms[ i ];
       (*cit).set_lhs( b[ i++ ] );
       }
     else
      for( Index i = 0 ; i < tochange ; ) {
       cit = std::next( cit , nms[ i ] - prev );
       prev = nms[ i ];
       (*cit).set_rhs( b[ i++ ] );
       }

     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_constant( nms[ 0 ] , b[ 0 ] );
     else
      PF->modify_constants( b , std::move( nms ) , true );
     }
    }

  // modify bound - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 16 ) && ( dis( rg ) <= p_change ) ) {
   LOG1( "modified bound - " );

   GenerateBND();

   // change it in the LP
   auto cnst = LPBlock->get_static_constraint< BoxConstraint >( 0 );
   if( convex )
    cnst->set_lhs( -BND );
   else
    cnst->set_rhs( BND );

   // modify it in the NDO
   auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
   PANIC( PF );

   PF->modify_bound( rs( BND ) );

   SetGlobalBound();  // set lower bound, be it "hard" or "conditional"
   }

 // add variables- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #if DYNAMIC_VARS > 0
  if( ( wchg & 32 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = Index( dis( rg ) * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " variables - " );

    GenerateA( m , tochange );

    // add them in the LP
    std::list< ColVariable > nxLPd( tochange );
    std::vector< ColVariable * > nxp( tochange );
    auto nxlpit = nxLPd.begin();
    for( Index i = 0 ; i < tochange ; )
     nxp[ i++ ] = &(*(nxlpit++));

    LPBlock->add_dynamic_variables(
	      *(LPBlock->get_dynamic_variable< ColVariable >( 0 )) , nxLPd );

    auto cnst_it =
             LPBlock->get_dynamic_constraint< FRowConstraint >( 0 )->begin();
    if( tochange == 1 )
     for( Index i = 0 ; i < m ; ++i ) {
      auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
      PANIC( fi );
      fi->add_variable( nxp[ 0 ] , - A[ i ][ 0 ] );
      }
    else
     for( Index i = 0 ; i < m ; ++i ) {
      auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
      PANIC( fi );
      LinearFunction::v_coeff_pair ncp( tochange );
      for( Index j = 0 ; j < ncp.size() ; ++j ) {
       ncp[ j ].first = nxp[ j ];
       ncp[ j ].second = - A[ i ][ j ];
       }
      fi->add_variables( std::move( ncp ) );
      }

    // add them in the NDO
    std::list< ColVariable > nxNDOd( tochange );
    auto nxndit = nxNDOd.begin();
    for( Index i = 0 ; i < tochange ; )
     nxp[ i++ ] = &(*(nxndit++));

    NDOBlock->add_dynamic_variables(
	    *(NDOBlock->get_dynamic_variable< ColVariable >( 0 )) , nxNDOd );

    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );

    if( tochange == 1 )
     PF->add_variable( nxp[ 0 ] , A[ 0 ] );
    else
     PF->add_variables( std::move( nxp ) , std::move( A ) );

    // update ndvar
    ndvar += tochange;

    // sanity checks
    PANIC( ndvar == PF->get_num_active_var() );
    for( auto & ai : *PF->get_A() )
     PANIC( ndvar == ai.size() );
    PANIC( ndvar ==
	         LPBlock->get_dynamic_variable< ColVariable >( 0 )->size() );
    PANIC( ndvar ==
	        NDOBlock->get_dynamic_variable< ColVariable >( 0 )->size() );
    for( auto & ci :
	          *(LPBlock->get_dynamic_constraint< FRowConstraint >( 0 )) )
     PANIC( ndvar == ci.get_num_active_var() );
    }
   }

  // remove variables - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 64 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = min( ndvar , Index( dis( rg ) * n_change ) );
   if( tochange ) {
    LOG1( "removed " << tochange << " variables" );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     LOG1( "(r) - " );

     Index strt = dis( rg ) * ( ndvar - tochange );
     Index stp = strt + tochange;

     // remove them from the LP
     auto xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     auto cnst_it =
             LPBlock->get_dynamic_constraint< FRowConstraint >( 0 )->begin();
     if( tochange == 1 )
      for( Index i = 0 ; i < m ; ++i ) {
       auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
       PANIC( fi );
       fi->remove_variable( strt + 1 );
       }
     else
      for( Index i = 0 ; i < m ; ++i ) {
       auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
       PANIC( fi );
       fi->remove_variables( Range( strt + 1 , stp + 1 ) ) , true );
       }

    LPBlock->remove_dynamic_variables( *xLPd , Range( strt , stp ) );

     // remove them from the NDO
     auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
     PANIC( PF );

     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 )
      PF->remove_variable( strt );
     else
      PF->remove_variables( Range( strt , stp ) );

     NDOBlock->remove_dynamic_variables( *xNDOd , Range( strt , stp ) );
     }
    else {  // in the other 50% of the cases, do a sparse change
     LOG1( "(s) - " );
     Subset nms( GenerateRand( ndvar , tochange ) );

     // remove them from the LP
     auto xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     auto cnst_it =
             LPBlock->get_dynamic_constraint< FRowConstraint >( 0 )->begin();
     if( tochange == 1 ) {
      for( Index i = 0 ; i < m ; ++i ) {
       auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
       PANIC( fi );
       fi->remove_variable( nms[ 0 ] + 1 );
       }

      auto vp = &(*std::next( xLPd->begin() , nms[ 0 ] ));
      LPBlock->remove_dynamic_variable( *xLPd , vp );
      }
     else {
      for( Index i = 0 ; i < m ; ++i ) {
       auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
       PANIC( fi );
       Subset nms1( nms );
       for( auto & n1i : nms1 )
	++n1i;
       fi->remove_variables( std::move( nms1 ) , true );
       }

      LPBlock->remove_dynamic_variables( *xLPd , Subset( nms ) );
      }

     // remove them from the NDO
     auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
     PANIC( PF );

     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      PF->remove_variable( nms[ 0 ] );

      auto vp = &(*std::next( xNDOd->begin() , nms[ 0 ] ));
      NDOBlock->remove_dynamic_variable( *xNDOd , vp );
      }
     else {
      PF->remove_variables( Subset( nms ) );
      NDOBlock->remove_dynamic_variables( *xNDOd , std::move( nms ) );
      }
     }

    // update ndvar
    ndvar -= tochange;

    // sanity checks
    PANIC( ndvar == PF->get_num_active_var() );
    for( auto & ai : *PF->get_A() )
     PANIC( ndvar == ai.size() );
    PANIC( ndvar ==
	         LPBlock->get_dynamic_variable< ColVariable >( 0 )->size() );
    PANIC( ndvar ==
	        NDOBlock->get_dynamic_variable< ColVariable >( 0 )->size() );
    for( auto & ci :
	          *(LPBlock->get_dynamic_constraint< FRowConstraint >( 0 )) )
     PANIC( ndvar == ci.get_num_active_var() );
    }
   }
  #endif

  // if verbose, print out stuff- - - - - - - - - - - - - - - - - - - - - - -

  #if( LOG_LEVEL >= 3 )
   ((LPBlock->get_registered_solvers()).front())->set_par(
		                  CPXMILPSolver::strOutputFile , "LPBlock-" +
		                  std::to_string( rep ) + ".lp" );
   #if( LOG_LEVEL >= 4 )
    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );
    printAb( PF->get_A() , PF->get_b() ,
	     convex ? PF->get_global_lower_bound()
	            : PF->get_global_upper_bound() );
   #endif
  #endif

  // finally, re-solve the problems- - - - - - - - - - - - - - - - - - - - -
  // ... every SKIP_BEAT + 1 rounds

  if( ! ( ++rep % ( SKIP_BEAT + 1 ) ) )
   AllPassed &= SolveBoth();
  #if( LOG_LEVEL >= 1 )
  else
   cout << endl;
  #endif

  }  // end( main loop )- - - - - - - - - - - - - - - - - - - - - - - - - - -
     // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( AllPassed )
  cout << GREEN( All tests passed!! ) << endl;
 else
  cout << RED( Shit happened!! ) << endl;
 
 // destroy objects and vectors - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // unregister (and delete) all Solvers attached to the Blocks
 NDOBlock->unregister_Solvers();
 LPBlock->unregister_Solvers();

 // delete the Blocks
 delete NDOBlock;
 delete LPBlock;

 // terminate - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( 0 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*------------------------ End File test.cpp -------------------------------*/
/*--------------------------------------------------------------------------*/
