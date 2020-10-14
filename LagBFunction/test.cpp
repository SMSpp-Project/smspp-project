/*--------------------------------------------------------------------------*/
/*-------------------------- File test.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing LagBFunction
 *
 * Given three input parameters n, k and p:
 *
 * - k "random" PolyhedralFunction are constructed, each inside a
 *   PolyhedralFunctionBlock.
 *
 * - p random uncapacitated *max-cost* transportation problems
 *   (with *negative* costs) on complete bipartite n-graphs (n origins, n
 *   destinations) are constructed "by hand", each inside an AbstractBlock;
 *   the problems have balanced supply and demands, no capacities, and no
 *   directed cycles, hence they surely attain optimal solutions.
 *
 * - The above p AbstractBlock are inserted as inner Block, each inside a
 *   LagBFunction.
 *
 * - The above p LagBFunction are put each inside the FRealObjective of a
 *   new AbstractBlock, otherwise empty.
 *
 * - The k PolyhedralFunctionBlock (configured to use "natural"
 *   representation) and the p AbstractBlock are inserted inside a single
 *   AbstractBlock (NDOBlock), possibly with a linear function, to represent
 *   a problem of the form (for k = 1 and p = 2)
 *
 *   min { l x + f(x)
 *         + max { ( B x + c_1 ) z_1 : E z_1 = b_1  , 0 <= z_1 }
 *         + max { ( B x + c_2 ) z_2 : E z_2 = b_2  , 0 <= z_2 } }
 *
 *   Assuming that arcs are ordered lexicographically (first all the ones
 *   outgoing node 0, ordered by tail node, then all the ones outgoing node
 *   1, ...), the matrix E has the form
 *
 *             n^2
 *      | e^T  0  ...  0  |
 *   n  |  0  e^T ...  0  |
 *      |  :  :        :  |
 *      |  0  0   ... e^T |
 *      +-----------------+
 *   n  |  I  I   ...  I  |
 *
 *   corresponding to constraints (with I = J =  { 0 ... n - 1 })
 *
 *      \sum_{ j \in J } x[ i ][ j ] == s[ i ]   i \in I
 *
 *      \sum_{ i \in I } x[ i ][ j ] == d[ j ]   j \in J
 *
 *   with s[] and d[] being the vectors of supplies and demand. The matrix
 *   B has the form
 *
 *             n^2
 *   n  |  I  I   ...  I  |
 *
 *   corresponding to the fact that the cost of arc ( i , j ) is
 *
 *         c[ i ][ j ] + x[ j ]
 *
 *   i.e., x[ j ] is added to the cost of all arcs ingoing destination j.
 *
 * - A BundleSolver is attached to NDOBlock
 *
 * Then, an LP equivalent of NDOBlock is constructed into a different
 * AbstractBlock (LPBlock) with the following steps:
 *
 * - The linear objective and the k PolyhedralFunctionBlock are just
 *   copied over, the latter using the R3Block.
 *
 * - For the p LagBFunction, an LP equivalent is constructed by the
 *   following derivation:
 *
 *   min { l x + f(x)
 *         + max { ( B x + c_1 ) z_1 : E z_1 = b_1  , 0 <= z_1 }
 *         + max { ( B x + c_2 ) z_2 : E z_2 = b_2  , 0 <= z_2 } } =
 *
 *   min { l x + f(x)
 *         + min { y_1 b_1 : y_1 E >= B x + c_1 }
 *         + min { y_2 b_2 : y_2 E >= B x + c_2 } } =
 *
 *   min { l x + f(x) + y_1 b_1 + y_2 b_2 :
 *         y_1 E >= B x + c_1 , y_2 E >= B x + c_2 }
 *
 *   Since the transpose of E has the form
 *
 *              n        n
 *         | e 0 ... 0 | I |
 *   n^2   | 0 e ... 0 | I |
 *         | : :     : | : |
 *         | 0 0 ... e | I |
 *
 *   this corresponds to variables yo[ i ] and yd[ j ] for each origin and
 *   destination, with costs s[ i ] and d[ j ] respectively, as well as
 *   constraints
 *
 *      ys[ i ] + yd[ j ] - x[ j ] >= c[ i ][ j ]
 *
 *   for all i \in I and j \in J.
 *
 * - The variables ys[ i ] and yd[ j ], the objective function and the
 *   constraints (linking them with x[]) are constructed manually into an
 *   AbstractBlock for each p.
 *
 * - A MILPSolver is attached to this AbstractBlock
 *
 * The PolyhedralFunction and/or the costs and demands (not supplies) of the
 * uncapacitated transportation problems are then repeatedly randomly
 * modified "in the same way", and re-solved several times; results of the
 * two solvers are compared.
 *
 * \version 0.10
 *
 * \date 11 - 09 - 2020
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
// 5 = + save every LP for every iteration

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

#define DETACH_NDO 0
// if nonzero, the Solver attched to the NDOBlock is detached and re-attached
// to it at all iterations

#define DETACH_LP 0
// if nonzero, the Solver attched to the LPBlock is detached and re-attached
// to it at all iterations

/*--------------------------------------------------------------------------*/

#define SKIP_BEAT 3
// if nonzero, the two Block are not solved at every round of changes, but
// only every SKIP_BEAT + 1 rounds. this allows changes to accumulate, and
// therefore puts more pressure on the Modification handling of the Solver
// (in case this tries to do "smart" things rather than dumbly processing
// each one in turn)
//
// note that the number of rounds of changes is them multiplied by
// SKIP_BEAT + 1, so that the input parameter still dictates the number of
// compute() calls per (Solver per) Block

/*--------------------------------------------------------------------------*/

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

#include "BlockSolverConfig.h"

#include "BundleSolver.h"

#include "CPXMILPSolver.h"

#include "LagBFunction.h"

#include "PolyhedralFunctionBlock.h"

#include "UpdateSolver.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace std;
using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

using Index = Block::Index;

using Range = Block::Range;

using Subset = Block::Subset;

using FunctionValue = Function::FunctionValue;
using c_FunctionValue = Function::c_FunctionValue;

using MultiVector = PolyhedralFunction::MultiVector;
using RealVector = PolyhedralFunction::RealVector;
using VarVector = PolyhedralFunction::VarVector;

using v_coeff_pair = LinearFunction::v_coeff_pair;

using p_AB = AbstractBlock *;
using p_PFB = PolyhedralFunctionBlock *;

using p_LF = LinearFunction *;
using p_PF = PolyhedralFunction *;
using p_LBF = LagBFunction *;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

const double scale = 10;
const char * const logF = "log.bn";

c_FunctionValue INF = SMSpp_di_unipi_it::Inf< FunctionValue >();

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

p_AB LPBlock;              // the "linearized" representaion

p_AB NDOBlock;             // the "natural" representation

double lb = - 1000;        // a tentative LB to detect unbounded instances

FunctionValue LB;          // the "true" LB (if any)

Index nvar = 10;           // number of variables
#if DYNAMIC_VARS > 0
 Index nsvar;              // number of static variables
 Index ndvar;              // number of dynamic variables
#else
 #define nsvar nvar        // all variables are static
#endif

std::mt19937 rg;           // base random generator
std::uniform_real_distribution<> dis( 0.0 , 1.0 );

MultiVector A;             // rows
RealVector b;              // constants

MultiVector C;             // arc costs
RealVector s;              // supplies
RealVector d;              // demands

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
static void Str2Sthg( const char* const str , T &sthg )
{
 istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/

static double rndfctr( void )
{
 // a random number between 0.5 and 2, with 50% probability of being < 1
 double fctr = dis( rg ) - 0.5;
 return( fctr < 0 ? - fctr : fctr * 4 );
 }

/*--------------------------------------------------------------------------*/

static void GenerateAi( RealVector & Ai , Index nc )
{
 Ai.resize( nc );
 for( auto & aij : Ai )
  aij = scale * ( 2 * dis( rg ) - 1 );
 }

/*--------------------------------------------------------------------------*/

static void GenerateA( Index nr , Index nc )
{
 A.resize( nr );
 for( auto & Ai : A )
  GenerateAi( Ai , nc );
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

static void GenerateLB( void )
{
 // rationale: we expect the solution x^* to have entries ~= 1 (in absolute
 // value, and the coefficients of A are <= scale (in absolute value), so
 // the LHS should be at most around - scale * nvar; the RHS can add it
 // a further - scale * nvar / 4, so we expect - (5/4) * scale * nvar to
 // be a "natural" LB. We therefore set the LB to a mean of 1/2 of that
 // (tight) 33% of the time, a mean of 2 times that (loose) 33% of the time,
 // and -INF the rest

 if( dis( rg ) <= 0.333 ) {  // "tight" LB
  LB = - dis( rg ) * 5 * scale * nvar / 4;
  return;
  }

 if( dis( rg ) <= 0.333 ) {  // "loose" LB
  LB = - dis( rg ) * 5 * scale * nvar;
  return;
  }

 LB = - INF;
 }

/*--------------------------------------------------------------------------*/

static Index GenerateCi( RealVector & Ci )
{
 // 10% of costs are zero, the rest random between -10 and 0

 Index nzc = 0;
 for( auto & cij : Ci )
  if( dis( rg ) < 0.1 )
   cij = 0;
  else {
   cij = - 10 * dis( rg );
   ++nzc;
   }

 return( nzc );
 }

/*--------------------------------------------------------------------------*/

static Index GenerateCosts( void )
{
 Index nzc = 0;
 for( auto & Ci : C )
  nzc += GenerateCi( Ci );

 return( nzc );
 }

/*--------------------------------------------------------------------------*/

static void GenerateSupplies( void )
{
 s.resize( nvar );
 for( auto & si : s )
  si = 10 * dis( rg );
 }

/*--------------------------------------------------------------------------*/

static void GenerateDemands( void )
{
 d.resize( nvar );
 for( auto & di : d )
  di = 10 * dis( rg );

 auto tots = std::accumulate( s.begin() , s.end() , double( 0 ) );
 auto totd = std::accumulate( d.begin() , d.end() , double( 0 ) );
 auto r = tots / totd;
 for( auto & di : d )
  di *= r;
 }

/*--------------------------------------------------------------------------*/

static Subset GenerateSubset( Index m , Index k )
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

static void printAb( const MultiVector & tA , const RealVector & tb ,
		     double lb )
{
 cout << "n = " << nvar << ", m = " << tA.size();
 if( lb > - INF )
  cout << ", LB = " << lb << endl;
 else
  cout << ", LB = - INF" << endl;

 for( Index i = 0 ; i < tA.size() ; ++i ) {
  cout << "A[ " << i << " ] = [ ";
  for( Index j = 0 ; j < nvar ; ++j )
   cout << tA[ i ][ j ] << " ";
   cout << "], b[ " << i << " ] = " << tb[ i ] << endl;
  }
 }

/*--------------------------------------------------------------------------*/

static void printC( void )
{
 for( Index i = 0 ; i < nvar ; ++i ) {
  cout << "C[ " << i << " ] = [ ";
  for( Index j = 0 ; j < nvar ; ++j )
   cout << C[ i ][ j ] << " ";
   cout << "]" << endl;
  }
 }

/*--------------------------------------------------------------------------*/

static void printT( void )
{
 cout << "s = [ ";
 for( Index j = 0 ; j < nvar ; ++j )
  cout << s[ j ] << " ";
 cout << "]" << endl;

 cout << "d = [ ";
 for( Index j = 0 ; j < nvar ; ++j )
  cout << d[ j ] << " ";
 cout << "]" << endl;

 printC();
 }

/*--------------------------------------------------------------------------*/

static void ConstructObj( p_AB AB )
{
 // construct the Linear Objective (FRealObjective with a LinearFunction
 // inside) in the given AbstractBlock; x is the 0-th group of static
 // Variable in AB, and this is only called if nf < 0

 auto x = AB->get_static_variable_v< ColVariable >( 0 );
 #if DYNAMIC_VARS > 0
  auto xd = AB->get_dynamic_variable< ColVariable >( 0 );
 #endif

 v_coeff_pair vars( nvar );
 Index i = 0;
 // static x
 for( ; i < nsvar ; ++i )
  vars[ i ] = std::make_pair( &((*x)[ i ] ) , A[ 0 ][ i ] );

 #if DYNAMIC_VARS > 0
  // dynamic x
  auto xdit = xd.begin();
  for( ; i < nvar ; ++i , ++xdit )
   vars[ j ] = std::make_pair( &(*xdit) , A[ 0 ][ i ] );
 #endif

 AB->set_objective( new FRealObjective( AB ,
			new LinearFunction( std::move( vars ) ) ) , eNoMod );
 }

/*--------------------------------------------------------------------------*/

static void ChangeLPConstraint( Index i , FRowConstraint & ci , ModParam iAM )
{
 // change the constant == LHS of the constraint
 ci.set_lhs( b[ i ] , iAM );

 // now change the coefficients, except that of v that is always 1
 RealVector coeffs( nvar );

 for( Index j = 0 ; j < nvar ; ++j )
  coeffs[ j ] = - A[ i ][ j ];

 static_cast< p_LF >( ci.get_function() )->modify_coefficients(
		          std::move( coeffs ) , Range( 1 , nvar + 1 ) , iAM );
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

  // solve the NODBlock - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Solver * slvrNDO = (NDOBlock->get_registered_solvers()).front();
  #if DETACH_NDO
   NDOBlock->unregister_Solver( slvrNDO );
   NDOBlock->register_Solver( slvrNDO );
  #endif
  int rtrnNDO = slvrNDO->compute( false );

  if( ( rtrnLP >= Solver::kOK ) && ( rtrnLP < Solver::kError ) &&
      ( rtrnNDO >= Solver::kOK ) && ( rtrnNDO < Solver::kError ) ) {
   auto foLP = slvrLP->get_ub();
   auto foNDO = slvrNDO->get_ub();
   if( abs( foLP - foNDO )
       <= 2e-7 * max( double( 1 ) , abs( max( foLP , foNDO ) ) ) ) {
    LOG1( "OK(f)" << endl );
    return( true );
    }
   }

  if( ( rtrnLP >= Solver::kOK ) && ( rtrnLP < Solver::kError ) &&
      ( rtrnNDO == Solver::kUnbounded ) ) {
   /* Weird case: the LP found an optimal solution but the NDO declared the
    * problem unbounded below. This may be because the tentative lb is too
    * high, check it this actually is the case and if so declare the
    * run a success (but also decrease the lb). */
   if( slvrNDO->get_ub() <= lb * ( 1 + 1e-9 ) ) {
    LOG1( "OK(?lb?)" << endl );
    lb *= 2;
    NDOBlock->set_valid_lower_bound( lb , true );
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
   if( ( rtrnLP >= Solver::kOK ) && ( rtrnLP < Solver::kError ) )
    cout << slvrLP->get_ub();
   else
    if( rtrnLP == Solver::kInfeasible )
     cout << "    +INF(?)";
    else
     if( rtrnLP == Solver::kUnbounded )
      cout << "        -INF";
     else
      cout << "      Error!";

   cout << " ~ NDOBlock = ";
   if( ( rtrnNDO >= Solver::kOK ) && ( rtrnNDO < Solver::kError ) )
    cout << slvrNDO->get_ub();
   else
    if( rtrnNDO == Solver::kInfeasible )
     cout << "    +INF(?)";
    else
     if( rtrnNDO == Solver::kUnbounded )
      cout << "        -INF";
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
 }  // end( SolveBoth )

/*--------------------------------------------------------------------------*/

int main( int argc , char **argv )
{
 // reading command line parameters - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 assert( SKIP_BEAT >= 0 );

 long int seed = 0;
 Index wchg = 159;
 int nf = 1;
 int nt = 1;
 double dens = 3;
 Index n_repeat = 40;
 Index n_change = 10;
 double p_change = 0.5;

 switch( argc ) {
  case( 10 ): Str2Sthg( argv[ 9 ] , p_change );
  case( 9 ): Str2Sthg( argv[ 8 ] , n_change );
  case( 8 ): Str2Sthg( argv[ 7 ] , n_repeat );
  case( 7 ): Str2Sthg( argv[ 6 ] , dens );
  case( 6 ): Str2Sthg( argv[ 5 ] , nt );
  case( 5 ): Str2Sthg( argv[ 4 ] , nf );
  case( 4 ): Str2Sthg( argv[ 3 ] , nvar );
  case( 3 ): Str2Sthg( argv[ 2 ] , wchg );
  case( 2 ): Str2Sthg( argv[ 1 ] , seed );
             break;
  default: cerr << "Usage: " << argv[ 0 ] <<
	   " seed [wchg nvar #nf #nt dens #rounds #chng %chng]"
 		<< endl <<
           "       wchg: what to change, coded bit-wise [159]"
		<< endl <<
           "             0 = add rows, 1 = delete rows "
		<< endl <<
           "             2 = modify rows, 3 = modify constants"
		<< endl <<
           "             4 = change global lower/upper bound"
		<< endl <<
           "             5 = modify costs, 6 = modify demands"
  #if DYNAMIC_VARS > 0  
		<< endl <<
           "             7 = add variables rows, 8 = delete variables"
  #endif
	        << endl <<
           "       nvar: number of variables [10]"
	        << endl <<
           "       |#nf|: number of PolyFunction (< 0: linear function) [1]"
	        << endl <<
           "       |#nt|: number of transportation (< 0: easy comp.) [1]"
	        << endl <<
           "       dens: rows / variables [3]"
	        << endl <<
           "       #rounds: how many iterations [40]"
	        << endl <<
           "       #chng: number of changes [10]"
	        << endl <<
           "       %chng: probability of changing [0.5]"
	        << endl;
	   return( 1 );
  }

 if( nvar < 1 ) {
  cout << "error: nvar too small";
  exit( 1 );
  }

 bool HasLin = ( nf < 0 );
 nf = std::abs( nf );
 bool HasEasy = ( nt < 0 );
 nt = std::abs( nt );

 if( ( ! nf ) && ( ! nt ) ) {
  cout << "error: no sub-Block";
  exit( 1 );
  }

 #if DYNAMIC_VARS > 0
  nsvar = nvar / 2;      // half of the variables are dynamic
  ndvar = nvar - nsvar;  // the other half are static
 #endif

 Index m = nvar * dens;  // number of rows
 if( m < 1 ) {
  cout << "error: dens too small";
  exit( 1 );
  }

 // adjust lb depending on the number of components
 lb *= std::max( 1 , std::abs( nf ) );

 rg.seed( seed );  // seed the pseudo-random number generator

 cout.setf( ios::scientific, ios::floatfield );
 cout << setprecision( 10 );

 // construction and loading of the objects - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LPBlock = new AbstractBlock();
 LPBlock->set_name( "LPBlock" );

 NDOBlock = new AbstractBlock();
 NDOBlock->set_name( "NDOBlock" );

 // immediately set the global conditional lower bound
 NDOBlock->set_valid_lower_bound( lb , true );

 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  // construct the Variable in LPBlock
  auto xLP = new std::vector< ColVariable >( nsvar );
  VarVector LPvars( nvar );
  auto vit = LPvars.begin();
  for( auto & xi : *xLP )
   *(vit++) = & xi;
  #if DYNAMIC_VARS > 0
   auto xLPd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xLPd )
    *(vit++) = & xi;
  #endif

  // now set the Variable in LPBlock
   LPBlock->add_static_variable( *xLP , "x" );
  #if DYNAMIC_VARS > 0
   LPBlock->add_dynamic_variable( *xLPd , "xd" );
  #endif

   // construct the Variable in NDOBlock
  auto xNDO = new std::vector< ColVariable >( nsvar );
  VarVector NDOvars( nvar );
  vit = NDOvars.begin();
  for( auto & xi : *xNDO )
   *(vit++) = & xi;
  #if DYNAMIC_VARS > 0
   auto xNDOd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xNDOd )
    *(vit++) = & xi;
  #endif

  // now set the Variable in NDOBlock
   NDOBlock->add_static_variable( *xNDO , "x" );
  #if DYNAMIC_VARS > 0
   NDOBlock->add_dynamic_variable( *xNDOd , "xd" );
  #endif

  // construct the linear objective (in both)
  if( HasLin ) {  // if any
   A.resize( 1 );
   GenerateAi( A[ 0 ] , nvar );

   ConstructObj( LPBlock );
   ConstructObj( NDOBlock );

   #if( LOG_LEVEL >= 4 )
    cout << "L = [ ";
    for( Index j = 0 ; j < nvar ; ++j )
     cout << A[ 0 ][ j ] << " ";
    cout << "]" << endl;
   #endif
   }

  // construct the PolyhedralFunctionBlocks (in both) - - - - - - - - - - - -

  for( Index k = 0 ; k < nf ; ++k ) {
   // construct the PolyhedralFunctionBlock
   auto PFBLPk = new PolyhedralFunctionBlock( LPBlock );
   PFBLPk->set_name( "LP-PFB_" + std::to_string( k ) );

   // pass it to LPBlock
   LPBlock->add_nested_Block( PFBLPk );

   // construct the m x nvar matrix A, the m-vector b, and the bound
   GenerateAb( m , nvar );
   GenerateLB();

   #if( LOG_LEVEL >= 4 )
    cout << "PF[ " << k << " ] = " << endl;
    printAb( A , b , LB );
   #endif

   // pass all the data of the PolyhedralFunction
    PFBLPk->get_PolyhedralFunction().set_PolyhedralFunction( std::move( A ) ,
							     std::move( b ) ,
							     LB );
   // copy the PolyhedralFunctionBlock
   auto PFBNDOk = static_cast< p_PFB >(
		      PFBLPk->get_R3_Block( nullptr , nullptr , NDOBlock ) );
   PFBNDOk->set_name( "NDO-PFB_" + std::to_string( k ) );

   // pass it to NDOBlock
   NDOBlock->add_nested_Block( PFBNDOk );

   // pass the Variable to the PolyhedralFunction in LP (copy the vector)
   PFBLPk->get_PolyhedralFunction().set_variables( VarVector( LPvars ) );

   // pass the Variable to the PolyhedralFunction in NDO (copy the vector)
   PFBNDOk->get_PolyhedralFunction().set_variables( VarVector( NDOvars ) );

   // configure the PolyhedralFunctionBlock in LPBlock to use the
   // "linearised" representation
   auto bc = new BlockConfig();
   bc->f_static_variables_Configuration = new SimpleConfiguration<int>( 1 );
   PFBLPk->set_BlockConfig( bc );

   }  // end( for( k ) )

  // now construct the transportation problems (and their duals)- - - - - - -

  // first allocate memory once and for all
  C.resize( nvar );
  for( auto & Ci : C )
   Ci.resize( nvar );
  
  for( Index p = 0 ; p < nt ; ++p ) {  // for all transportation sub-Block- -
                                       // - - - - - - - - - - - - - - - - - -
   Index nzc = GenerateCosts();        // generate random costs
   GenerateSupplies();                 // generate random supplies
   GenerateDemands();                  // generate random demands

   #if( LOG_LEVEL >= 4 )
    cout << "T[ " << p << " ] = " << endl;
    printT();
   #endif

   // construct the AbstractBlock for LP transportation - - - - - - - - - - -
   auto TLPp = new AbstractBlock( LPBlock );
   TLPp->set_name( "LP-TB_" + std::to_string( p ) );

   // construct the (dual) variables ys and yd
   auto ys = new std::vector< ColVariable >( nvar );
   auto yd = new std::vector< ColVariable >( nvar );

   // construct and initialise the (potential) constraints
   auto pc = new boost::multi_array< FRowConstraint , 2 >(
					    boost::extents[ nvar ][ nvar ] );

   // ... that link with the variables in the root LPBlock
   auto xLP = LPBlock->get_static_variable_v< ColVariable >( 0 );

   for( Index i = 0 ; i < nvar ; ++i )
    for( Index j = 0 ; j < nvar ; ++j ) {
     // construct constraint ys[ i ] + yd[ j ] - x[ j ] >= c[ i ][ j ]

     (*pc)[ i ][ j ].set_lhs( C[ i ][ j ] );
     (*pc)[ i ][ j ].set_rhs( INF );

     v_coeff_pair cf( 3 );
     cf[ 0 ] = std::make_pair( & (*ys)[ i ] , 1 );
     cf[ 1 ] = std::make_pair( & (*yd)[ j ] , 1 );
     cf[ 2 ] = std::make_pair( & (*xLP)[ j ] , -1 );

     (*pc)[ i ][ j ].set_function( new LinearFunction( std::move( cf ) ) );
     }

   // construct the (dual) objective
   v_coeff_pair docf( 2 * nvar );

   for( Index i = 0 ; i < nvar ; ++i )
    docf[ i ] = std::make_pair( & (*ys)[ i ] , s[ i ] );

   for( Index j = 0 ; j < nvar ; ++j )
    docf[ nvar + j ] = std::make_pair( & (*yd)[ j ] , d[ j ] );

   // pass the (dual) variables to the AbstractBlock
   TLPp->add_static_variable( *ys , "ys" );
   TLPp->add_static_variable( *yd , "yd" );

   // pass the (potential) constraints to the AbstractBlock
   TLPp->add_static_constraint( *pc , "pc" );

   // pass the (dual) objective to the AbstractBlock
   TLPp->set_objective( new FRealObjective( TLPp ,
			new LinearFunction( std::move( docf ) ) ) , eNoMod );

   // finally, pass it to LPBlock
   LPBlock->add_nested_Block( TLPp );

   // construct the AbstractBlock for NDO transportation- - - - - - - - - - -
   // the AbstractBlock just has a FRealObjective with a LagBFunction inside;
   // the inner Block of the LagBFunction is the transportation probem
   
   auto TNDOp = new AbstractBlock( NDOBlock );
   TNDOp->set_name( "NDO-TB_" + std::to_string( p ) );

   // construct the inner Block for the LagBFunction
   auto IBNDOp = new AbstractBlock();
   IBNDOp->set_name( "IB-NDO-TB_" + std::to_string( p ) );

   // construct the flow variables
   auto f = new boost::multi_array< ColVariable , 2 >(
				            boost::extents[ nvar ][ nvar ] );
   // construct the source constraints
   auto sc = new std::vector< FRowConstraint >( nvar );

   // construct the destination constraints
   auto dc = new std::vector< FRowConstraint >( nvar );
   
   // construct the sign constraints
   auto bc = new boost::multi_array< NNConstraint , 2 >(
				            boost::extents[ nvar ][ nvar ] );
   // initialize the source constraints
   for( Index i = 0 ; i < nvar ; ++i ) {
    // construct constraint  \sum_{ j \in J } x[ i ][ j ] == s[ i ] 

    (*sc)[ i ].set_both( s[ i ] );

    v_coeff_pair cf( nvar );
    for( Index j = 0 ; j < nvar ; ++j ) {
     cf[ j ].first = & (*f)[ i ][ j ];
     cf[ j ].second = 1;
     }

    (*sc)[ i ].set_function( new LinearFunction( std::move( cf ) ) );
    }

   // initialize the destination constraints
   for( Index j = 0 ; j < nvar ; ++j ) {
    // construct constraint \sum_{ i \in I } x[ i ][ j ] == d[ j ] 

    (*dc)[ j ].set_both( d[ j ] );

    LinearFunction::v_coeff_pair cf( nvar );
    for( Index i = 0 ; i < nvar ; ++i )
     cf[ i ] = std::make_pair( & (*f)[ i ][ j ] , 1 );

    (*dc)[ j ].set_function( new LinearFunction( std::move( cf ) ) );
    }

   // initialize the sign constraints
   for( Index i = 0 ; i < nvar ; ++i )
    for( Index j = 0 ; j < nvar ; ++j )
     (*bc)[ i ][ j ].set_variable( & (*f)[ i ][ j ] , eNoMod );

   // construct the objective
   v_coeff_pair ocf( nzc );

   auto it = ocf.begin();
   for( Index i = 0 ; i < nvar ; ++i )
    for( Index j = 0 ; j < nvar ; ++j )
     if( C[ i ][ j ] ) {
      it->first = & (*f)[ i ][ j ];
      (it++)->second = C[ i ][ j ];
      }

   assert( it == ocf.end() );

   // pass the flow variables to the inner Block
   IBNDOp->add_static_variable( *f , "f" );

   // pass the source constraints to the inner Block
   IBNDOp->add_static_constraint( *sc , "sc" );

   // pass the destination constraints to the inner Block
   IBNDOp->add_static_constraint( *dc , "dc" );

   // pass the sign constraints to the inner Block
   IBNDOp->add_static_constraint( *bc , "bc" );

   // construct the objective of the inner Block
   auto ibo = new FRealObjective( IBNDOp ,
				  new LinearFunction( std::move( ocf ) ) );
   ibo->set_sense( Objective::eMax );  // ... to be *max*imized

   // pass the objective to the inner Block
   IBNDOp->set_objective( ibo , eNoMod );

   // construct the LagBFunction, passing it the inner Block
   auto LBF = new LagBFunction( IBNDOp );

   // construct the dual pairs
   LagBFunction::v_dual_pair lp( nvar );

   // ... that use the variables in NDOBlock
   auto xNDO = NDOBlock->get_static_variable_v< ColVariable >( 0 );

   for( Index j = 0 ; j < nvar ; ++j ) {
    v_coeff_pair cfj( nvar );

    for( Index i = 0 ; i < nvar ; ++i ) {
     cfj[ i ].first = & (*f)[ i ][ j ];
     cfj[ i ].second = 1;
     }

    lp[ j ].first = & (*xNDO)[ j ];
    lp[ j ].second = new LinearFunction( std::move( cfj ) );
    }

   // pass the dual pairs to the LagBFunction
   LBF->set_dual_pairs( std::move( lp ) );

   // construct the objective to the transportation Block, passing it the
   // LagBFunction
   TNDOp->set_objective( new FRealObjective( TNDOp , LBF ) , eNoMod );

   // finally, pass the transportation Block to the NDOBlock
   NDOBlock->add_nested_Block( TNDOp );

   }  // end( for( p ) )
  }

 // final setup- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LPBlock->generate_abstract_variables();
 LPBlock->generate_abstract_constraints();
 LPBlock->generate_objective();

 NDOBlock->generate_abstract_variables();
 NDOBlock->generate_abstract_constraints();
 NDOBlock->generate_objective();

 // attach the Solver to the Block- - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // for LPBlock, "manually" attach a CPXMILPSolver and an UpdateSolver (to
 // each PolyhedralFunctionBlock in LPBlock)

 LPBlock->register_Solver( Solver::new_Solver( "CPXMILPSolver" ) );

 for( int i = 0 ; i < nf ; ++i )
  LPBlock->get_nested_Block( i )->register_Solver(
		       new UpdateSolver( NDOBlock->get_nested_Block( i ) ) );

 {
  // for NDOBlock do this by reading appropriate BlockSolverConfig from
  // files and apply() it to the NDOBlock
  ifstream BundleParFile( "BundlePar.txt" );
  if( ! BundleParFile.is_open() ) {
   cerr << "Error: cannot open file BundlePar.txt" << endl;
   return( 1 );
   }

  auto bsc = new BlockSolverConfig( BundleParFile );
  BundleParFile.close();

  // ensure the "easy components" parameter is properly set
  for( int i = 0 ; i < bsc->num_ComputeConfig() ; ++i )
   if( bsc->get_SolverName( i ) == "BundleSolver" )
    bsc->get_SolverConfig( i )->set_par( "intNoEasy" ,
					 HasEasy ? int( 0 ) : int( 1 ) );
 
  bsc->apply( NDOBlock );  // now apply the BlockSolverConfig to NDOBlock
  delete bsc;

  if( HasEasy ) {    // transportation problems are treated as "easy"
   #if( LOG_LEVEL >= 5 )
    // in the extremely verbose mode, set an event that spits out the
    // current point every k iterations; k must be in the parameter
    // file via intEverykIt (0 by default == never)

   for( auto slvr : NDOBlock->get_registered_solvers() ) {
    auto bslvr = dynamic_cast< BundleSolver * >( slvr );
    if( ! bslvr )
     continue;

    bslvr->set_event_handler( ThinComputeInterface::eEverykIteration ,
     // define the event via an appropriate Lambda
     [ & ] () {
      // print the current point
      auto l = bslvr->get_current_point();
      cout << endl << "            CP = [ ";
      for( auto el : l )
       cout << el << " ";
      cout << "]" << endl;

      return( ThinComputeInterface::eContinue );
      }  // end of Lambda
			      );
    }
   #endif
   }
  else {             // transportation problems are treated as "difficult"
   // also attach a proper Solver to each inner Block of LagBFunction
   for( Index p = nf ; p < nf + nt ; ++p ) {
    auto FRO =
         NDOBlock->get_nested_Block( p )->get_objective< FRealObjective >();
    auto LBF = static_cast< p_LBF >( FRO->get_function() );
    LBF->get_nested_Block( 0 )->register_Solver(
				    Solver::new_Solver( "CPXMILPSolver" ) );
    }

   #if( LOG_LEVEL >= 5 )
    // in the extremely verbose mode, set an event that spits out the LPs
    // in the LagBFunctions every k iterations; k must be in the parameter
    // file via intEverykIt (0 by default == never)

   for( auto slvr : NDOBlock->get_registered_solvers() ) {
    auto bslvr = dynamic_cast< BundleSolver * >( slvr );
    if( ! bslvr )
     continue;

    bslvr->set_event_handler( ThinComputeInterface::eEverykIteration ,
     // define the event via an appropriate Lambda
     [ & ] () {
      // print the current point
      auto l = bslvr->get_current_point();
      cout << endl << "            CP = [ ";
      for( auto el : l )
       cout << el << " ";
      cout << "]" << endl;

      // have the inner CPXMILPSolver spit out the LP at this iteration
      for( Index p = nf ; p < nf + nt ; ++p ) {
       auto FRO =
	NDOBlock->get_nested_Block( p )->get_objective< FRealObjective >();
       auto LBF = static_cast< p_LBF >( FRO->get_function() );
       auto slv =
	LBF->get_nested_Block( 0 )->get_registered_solvers().front();
       slv->set_par( CPXMILPSolver::strOutputFile ,
		     "TB-" + std::to_string( p - nf ) + "-" +
		     std::to_string( bslvr->n_calls() ) + "-" +
		     std::to_string( bslvr->n_iter() ) + ".lp" );
       }

      return( ThinComputeInterface::eContinue );
      }  // end of Lambda
			      );
    }
   #endif
   }
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
 // now, for n_repeat times, one inner Block is selected and:
 // - if it is a PolyhedralFunctionBlock
 //   = up to n_change rows are added
 //   = up to n_change rows are deleted
 //   = up to n_change rows are modified
 //   = up to n_change constants are modified
 // - if it is an AbstractBlock (transportation problem)
 //   = costs are modified
 //   = demands are modified
 //
 // then the two problems are re-solved
 //
 // IMPORTANT NOTE: only LPBlock is changed, because UpdateSolver takes
 //                 care of intercepting all (physical) Modification and
 //                 map_forward them to NDOBlock
 //
 // by playing with SKIP_BEAT one can re-solve the two problems after having
 // changed an arbitrary number of inner Blocks

 for( Index rep = 0 ; rep < n_repeat * ( SKIP_BEAT + 1 ) ; ) {

  p_AB LPTr = nullptr;
  p_AB NDOTr = nullptr;
  p_PFB LPBr = nullptr;

  Index bn = rep % ( nf + nt );  // which sub-Block to change

  if( bn < nf ) {
   LPBr = static_cast< p_PFB >( LPBlock->get_nested_Block( bn ) );
   LOG1( rep << "[PFB " << bn << "]: ");
   }
  else {
   LPTr = static_cast< p_AB >( LPBlock->get_nested_Block( bn ) );
   NDOTr = static_cast< p_AB >( NDOBlock->get_nested_Block( bn ) );
   auto LBF = static_cast< p_LBF >(
		  NDOTr->get_objective< FRealObjective >()->get_function() );
   NDOTr = static_cast< p_AB >( LBF->get_nested_Block( 0 ) );
   LOG1( rep << "[TB " << bn - nf << "]: ");
   }

  // add rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LPBr && ( wchg & 1 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = Index( dis( rg ) * n_change ) ) {
    LOG1( "added " << tochange << " rows - " );

    GenerateAb( tochange , nvar );

    auto cnst = LPBr->get_dynamic_constraint< FRowConstraint >( 0 );

    if( tochange == 1 )
     LPBr->get_PolyhedralFunction().add_row( std::move( A[ 0 ] ) , b[ 0 ] );
    else
     LPBr->get_PolyhedralFunction().add_rows( std::move( A ) , b );
    }


  // delete rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LPBr && ( wchg & 2 ) && ( dis( rg ) <= p_change ) ) {
   m = LPBr->get_PolyhedralFunction().get_nrows();
   if( Index tochange = min( m - 1 , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "deleted " << tochange << " rows" );

    auto cnst = LPBr->get_dynamic_constraint< FRowConstraint >( 0 );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
    
     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     LOG1( "(r) - " );
     if( tochange == 1 )
      LPBr->get_PolyhedralFunction().delete_row( strt );
     else
      LPBr->get_PolyhedralFunction().delete_rows( Range( strt , stp ) );
     }
    else {  // in the other 50% of the cases, do a sparse change
     Subset nms = GenerateSubset( m , tochange );

     LOG1( "(s) - " );
     if( tochange == 1 )
      LPBr->get_PolyhedralFunction().delete_row( nms[ 0 ] );
     else
      LPBr->get_PolyhedralFunction().delete_rows( std::move( nms ) );
     }
    }
   }

  // modify rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LPBr && ( wchg & 4 ) && ( dis( rg ) <= p_change ) ) {
   m = LPBr->get_PolyhedralFunction().get_nrows();
   if( Index tochange = std::min( m , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "modified " << tochange << " rows" );

    GenerateAb( tochange , nvar );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     LOG1( "(r) - " );
     if( tochange == 1 )
      LPBr->get_PolyhedralFunction().modify_row( strt , std::move( A[ 0 ] ) ,
						 b[ 0 ] );
     else
      LPBr->get_PolyhedralFunction().modify_rows( std::move( A ) , b ,
						  Range( strt , stp ) );
     }
    else {  // in the other 50% of the cases, do a sparse change
     Subset nms = GenerateSubset( m , tochange );

     LOG1( "(s) - " );
     if( tochange == 1 )
      LPBr->get_PolyhedralFunction().modify_row( nms[ 0 ] ,
						 std::move( A[ 0 ] ) ,
						 b[ 0 ] );
     else
      LPBr->get_PolyhedralFunction().modify_rows( std::move( A ) , b ,
						  std::move( nms ) , true );
     }
    }
   }

  // modify constants - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LPBr && ( wchg & 8 ) && ( dis( rg ) <= p_change ) ) {
   m = LPBr->get_PolyhedralFunction().get_nrows();
   if( Index tochange = std::min( m , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "modified " << tochange << " constants" );

    Generateb( tochange );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     LOG1( "(r) - " );
     if( tochange == 1 )
      LPBr->get_PolyhedralFunction().modify_constant( strt , b[ 0 ] );
     else
      LPBr->get_PolyhedralFunction().modify_constants( b ,
						       Range( strt , stp ) );
     }
    else {  // in the other 50% of the cases, do a sparse change
     Subset nms = GenerateSubset( m , tochange );

     LOG1( "(s) - " );
     if( tochange == 1 )
      LPBr->get_PolyhedralFunction().modify_constant( nms[ 0 ] , b[ 0 ] );
     else
      LPBr->get_PolyhedralFunction().modify_constants( b , std::move( nms ) ,
						       true );
     }
    }
   }

  // modify bound - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LPBr && ( wchg & 16 ) && ( dis( rg ) <= p_change ) ) {
   LOG1( "modified bound - " );

   GenerateLB();

   LPBr->get_PolyhedralFunction().modify_bound( LB );
   }

  // change costs - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LPTr && ( wchg & 32 ) && ( dis( rg ) <= p_change ) ) {
   // about 30% of the coefficients are 0: the corresponding variables were
   // not originally in the objective, but have been (hopefully) silently
   // added back to it by LagBFunction, "at the end", because all costs do
   // have a Lagrangian term. when costs are changed, there is a fair chance
   // that some of these coefficients will be changed (and made nonzero).
   // in 50% of the cases we rather eliminate (==> set the coefficient to 0)
   // a bunch of variables from the objective to test LagBFunction's
   // capability to add them back to the end

   GenerateCosts();
   #if( LOG_LEVEL >= 5 )
    printC();
   #endif

   auto lf = static_cast< p_LF >(
		  NDOTr->get_objective< FRealObjective >()->get_function() );
   auto & cf = lf->get_v_var();

   // because the order of the variables in the objective is messed up with
   // reconstruct the information about which position corresponds to what
   auto f = NDOTr->get_static_variable< ColVariable , 2 >( "f" );
   if( ! f ) {
    LOG1( "something very bad happened!" );
    exit( 1 );
    }
   std::vector< std::pair< Index , Index > > dict( cf.size() );
   for( Index h = 0 ; h < cf.size() ; ++h ) {
    bool found = false;
    for( Index i = 0 ; ( i < nvar ) && ( ! found ) ; ++i )
     for( Index j = 0 ; ( j < nvar ) && ( ! found ) ; ++j )
      if( & (*f)[ i ][ j ] == cf[ h ].first ) {
       dict[ h ].first = i;
       dict[ h ].second = j;
       found = true;
       }
    }

   if( dis( rg ) < 0.5 ) {  // in 50% of the cases, modify the costs
    // change up to 50% of the variables and *no less* than n_change
    Index tochange = std::min( Index( cf.size() ) ,
			       std::max( n_change ,
					 Index( dis( rg ) * cf.size() / 2 )
					 ) );

    LOG1( "modified " << tochange << " costs" );

     if( dis( rg ) < 0.5 ) {  // in 25% of the cases, do a ranged change
     LOG1( "(r) - " );
     Index strt = dis( rg ) * ( cf.size() - tochange );
     Index stp = strt + tochange;

     // in the dual transportation problem these are the RHS of the
     // potential constraints: send all the corresponding Modification to
     // a new channel
     auto pc = LPTr->get_static_constraint< FRowConstraint , 2 >( "pc" );

     Observer::ChnlName chnl = LPTr->open_channel();
     const auto iAM = Observer::make_par( eModBlck , chnl );
     for( Index h = strt ; h < stp ; ++h )
      (*pc)[ dict[ h ].first ][ dict[ h ].second ].set_lhs(
			      C[ dict[ h ].first ][ dict[ h ].second ] , iAM );

     LPTr->close_channel( chnl );  // then close the chanel

     // in the transportation problem inside the LagBFunction, just do it
     RealVector newc( tochange );
     for( Index h = strt ; h < stp ; ++h )
      newc[ h - strt ] = C[ dict[ h ].first ][ dict[ h ].second ];

     lf->modify_coefficients( std::move( newc ) , Range( strt , stp ) );
     }
    else {                   // in 25% of the cases, do a subset change
     LOG1( "(s) - " );
     Subset nms = GenerateSubset( cf.size() , tochange );

     // in the dual transportation problem these are the RHS of the
     // potential constraints: send all the corresponding Modification to
     // a new channel
     auto pc = LPTr->get_static_constraint< FRowConstraint , 2 >( "pc" );

     Observer::ChnlName chnl = LPTr->open_channel();
     const auto iAM = Observer::make_par( eModBlck , chnl );
     for( Index h : nms )
      (*pc)[ dict[ h ].first ][ dict[ h ].second ].set_lhs(
                             C[ dict[ h ].first ][ dict[ h ].second ] , iAM );

     LPTr->close_channel( chnl );  // then close the chanel

     // in the transportation problem inside the LagBFunction, just do it
     RealVector newc( tochange );
     for( Index h = 0 ; h < tochange ; ++h )
      newc[ h ] = C[ dict[ nms[ h ] ].first ][ dict[ nms[ h ] ].second ];

     lf->modify_coefficients( std::move( newc ) , std::move( nms ) );
     }
    }
   else {                    // in 50% of the cases, eliminate coefficients
    // change up to 25% of the variables and *no less* than 1
    Index tochange = std::min( Index( cf.size() ) ,
			       std::max( Index( 1 ) ,
					 Index( dis( rg ) * cf.size() / 4 )
					 ) );

    LOG1( "deleted " << tochange << " costs" );

    //!! if( dis( rg ) < 0.5 )
    {  // in 25% of the cases, do a ranged removal
     LOG1( "(r) - " );
     Index strt = dis( rg ) * ( cf.size() - tochange );
     Index stp = strt + tochange;

     // in the dual transportation problem these are the RHS of the
     // potential constraints: send all the corresponding Modification to
     // a new channel
     auto pc = LPTr->get_static_constraint< FRowConstraint , 2 >( "pc" );

     Observer::ChnlName chnl = LPTr->open_channel();
     const auto iAM = Observer::make_par( eModBlck , chnl );
     for( Index h = strt ; h < stp ; ++h )
      (*pc)[ dict[ h ].first ][ dict[ h ].second ].set_lhs( 0 , iAM );

     LPTr->close_channel( chnl );  // then close the chanel

     // in the transportation problem inside the LagBFunction, just do it
     lf->remove_variables( Range( strt , stp ) );
     }
    /*!!
    else {                   // in 25% of the cases, do a subset removal
     LOG1( "(s) - " );
     Subset nms = GenerateSubset( cf.size() , tochange );

     // in the dual transportation problem these are the RHS of the
     // potential constraints: send all the corresponding Modification to
     // a new channel
     auto pc = LPTr->get_static_constraint< FRowConstraint , 2 >( "pc" );

     Observer::ChnlName chnl = LPTr->open_channel();
     const auto iAM = Observer::make_par( eModBlck , chnl );
     for( Index h : nms )
      (*pc)[ dict[ h ].first ][ dict[ h ].second ].set_lhs(
                             C[ dict[ h ].first ][ dict[ h ].second ] , iAM );

     LPTr->close_channel( chnl );  // then close the chanel

     // in the transportation problem inside the LagBFunction, just do it
     lf->remove_variables( std::move( nms ) );
     }
     !!*/
    }
   }

  // change demands - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( LPTr && ( wchg & 64 ) && ( dis( rg ) <= p_change ) ) {
   LOG1( "modified demands - " );

   GenerateDemands();
   #if( LOG_LEVEL >= 5 )
    cout << endl << "d = [ ";
    for( auto el : d )
     cout << el << " ";
    cout << " ]" << endl;
   #endif

   // in the transportation problem inside the LagBFunction these are the
   // RHS of the demand constraints: send all the corresponding Modification
   // to a new channel
   auto dc = NDOTr->get_static_constraint_v< FRowConstraint >( "dc" );

   Observer::ChnlName chnl = NDOTr->open_channel();
   const auto iAM = Observer::make_par( eModBlck , chnl );
   for( Index j = 0 ; j < nvar ; ++j )
    (*dc)[ j ].set_both( d[ j ] , iAM );

   NDOTr->close_channel( chnl );  // then close the chanel

   // in the dual transportation problem these are a slice of the coefficients
   // of the objective
   auto lf = static_cast< LinearFunction * >(
		  LPTr->get_objective< FRealObjective >()->get_function() );
   lf->modify_coefficients( std::move( d ) , Range( nvar , 2 * nvar ) ); 
   }

  // add variables- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #if DYNAMIC_VARS > 0
  if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = Index( dis( rg ) * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " variables - " );

    throw( std::logic_error( "adding variables not implemented yet" ) );

    GenerateA( m , tochange );

    // add them in the LP, *copying* the data
    std::list< ColVariable > nxLPd( tochange );
    std::vector< Variable * > nxpLP( tochange );
    auto nxit = nxLPd.begin();
    for( Index i = 0 ; i < tochange ; )
     nxpLP[ i++ ] = &(*(nxit++));

    LPBlock->add_dynamic_variables(
	      *(LPBlock->get_dynamic_variable< ColVariable >( 0 )) , nxLPd );

    if( tochange == 1 )
     LPBlock->get_PolyhedralFunction().add_variable( nxpLP[ 0 ] , A[ 0 ] );
    else
     LPBlock->get_PolyhedralFunction().add_variables( std::move( nxpLP ) ,
						      MultiVector( A ) );

    // add them in the NDO, letting the data go
    std::list< ColVariable > nxNDOd( tochange );
    std::vector< Variable * > nxpNDO( tochange );
     auto nxit = nxNDOd.begin();
    for( Index i = 0 ; i < tochange ; )
     nxpNDO[ i++ ] = &(*(nxit++));

    NDOBlock->add_dynamic_variables(
	    *(NDOBlock->get_dynamic_variable< ColVariable >( 0 )) , nxNDOd );

    if( tochange == 1 )
     NDOBlock->get_PolyhedralFunction().add_variable( nxpNDO[ 0 ] , A[ 0 ] );
    else
     NDOBlock->get_PolyhedralFunction().add_variables( std::move( nxpNDO ) ,
						       std::move( A ) );

    // update ndvar
    ndvar += tochange;
    }
   }

  // remove variables - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 256 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = min( ndvar , Index( dis( rg ) * n_change ) );
   if( tochange ) {
    LOG1( "removed " << tochange << " variables" );

    throw( std::logic_error( "removing variables not implemented yet" ) );

    // in 50% of the cases do a ranged change, in the others a sparse change
    if( dis( rg ) <= 0.5 ) {
     LOG1( "(r) - " );

     Index strt = dis( rg ) * ( ndvar - tochange );
     Index stp = strt + tochange;

     // remove them from the LP
     auto xLPd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 )
      LPBlock->get_PolyhedralFunction().remove_variable( strt );
     else
      LPBlock->get_PolyhedralFunction().remove_variables( Range( strt ,
								 stp ) );

     LPBlock->remove_dynamic_variables( *xLPd , Range( strt , stp ) );

     // remove them from the NDO
     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 )
      NDOBlock->get_PolyhedralFunction().remove_variable( strt );
     else
      NDOBlock->get_PolyhedralFunction().remove_variables( Range( strt ,
								  stp ) );

     NDOBlock->remove_dynamic_variables( *xNDOd , Range( strt , stp ) );
     }
    else {
     LOG1( "(s) - " );
     Subset nms = GenerateSubset( ndvar , tochange );

     // remove them from the LP, *copying* names
     auto xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      LPBlock->get_PolyhedralFunction().remove_variable( nms[ 0 ] );
      auto vp = &(*std::next( xLPd->begin() , nms[ 0 ] ));
      LPBlock->remove_dynamic_variable( *xLPd , vp );
      }
     else {
      LPBlock->get_PolyhedralFunction().remove_variables( Subset( nms ) );
      LPBlock->remove_dynamic_variables( *xLPd , Subset( nms ) );
      }

     // remove them from the NDO, finally letting names go
     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      NDOBlock->get_PolyhedralFunction().remove_variable( nms[ 0 ] );
      auto vp = &(*std::next( xNDOd->begin() , nms[ 0 ] ));
      NDOBlock->remove_dynamic_variable( *xNDOd , vp );
      }
     else {
      NDOBlock->get_PolyhedralFunction().remove_variables( Subset( nms ) );
      NDOBlock->remove_dynamic_variables( *xNDOd , std::move( nms ) );
      }
     }

    // update ndvar
    ndvar -= tochange;
    }
   }

  #endif  // DYNAMIC_VARS > 0

  // if verbose, print out stuff- - - - - - - - - - - - - - - - - - - - - - -

  #if( LOG_LEVEL >= 3 )
   ((LPBlock->get_registered_solvers()).front())->set_par(
		                  CPXMILPSolver::strOutputFile , "LPBlock-" +
		                  std::to_string( rep ) + ".lp" );
   #if( LOG_LEVEL >= 4 )
    if( bn < nf ) {
     cout << endl << "LPBlock-PF: ";
     auto PF = & LPBr->get_PolyhedralFunction();
     printAb( PF->get_A() , PF->get_b() , PF->get_global_lower_bound() );
     }
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
  cout << GREEN( All test passed!! ) << endl;
 else
  cout << RED( Shit happened!! ) << endl;
 
 // destroy objects and vectors - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // unregister (and delete) all Solvers attached to the Blocks
 NDOBlock->unregister_Solvers();
 if( HasEasy )
  for( Index p = nf ; p < nf + nt ; ++p ) {
   auto FRO =
        NDOBlock->get_nested_Block( p )->get_objective< FRealObjective >();
   auto LBF = static_cast< LagBFunction * >( FRO->get_function() );
   LBF->get_nested_Block( 0 )->unregister_Solvers();
   }

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
