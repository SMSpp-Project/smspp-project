/*--------------------------------------------------------------------------*/
/*-------------------------- File test.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing PolyhedralFunctionBlock
 *
 * A "random" PolyhedralFunction is constructed inside a
 * PolyhedralFunctionBlock, then copied to another. A BundleSolver is attached
 * to a PolyhedralFunctionBlock with "natural" representation, a MILPSolver
 * is attached to the other PolyhedralFunctionBlock with "linearized"
 * representation; the two PolyhedralFunctionBlock are solved and the results
 * are compared. The two PolyhedralFunctionBlock are then repeatedly randomly
 * modified "in the same way", and re-solved several times.
 *
 * \version 0.20
 *
 * \date 09 - 02 - 2020
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
// 3 = + print data + save LP file

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) cout << x
 #define CLOG1( y , x ) if( y ) cout << x
#else
 #define LOG1( x )
 #define CLOG1( y , x )
#endif

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

#include "BundleSolver.h"

#include "CPXMILPSolver.h"

#include "PolyhedralFunctionBlock.h"

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

using RealVector = PolyhedralFunction::RealVector;
using MultiVector = PolyhedralFunction::MultiVector;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

const double scale = 10;
const char *const logF = "log.bn";

const FunctionValue INF = SMSpp_di_unipi_it::Inf< FunctionValue >();

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

PolyhedralFunctionBlock * LPBlock;   // the "linearized" representaion

PolyhedralFunctionBlock * NDOBlock;  // the "natural" representation

double lb = - 1000;        // a tentative LB to detect unbounded instances

FunctionValue LB;          // the "true" LB in the PolyhedralFunction (if any)

Index nvar = 10;           // number of variables
#if DYNAMIC_VARS > 0
 Index nsvar;              // number of static variables
 Index ndvar;              // number of dynamic variables
#else
 #define nsvar nvar        // all variables are static
#endif

Index m;                   // number of rows

PolyhedralFunction::MultiVector A;

std::vector < FunctionValue > b;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template<class T>
static inline void Str2Sthg( const char* const str , T &sthg )
{
 istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/

static inline double rndfctr( void )
{
 // return a random number between 0.5 and 2, with 50% probability of being
 // < 1
 double fctr = drand48() - 0.5;
 return( fctr < 0 ? - fctr : fctr * 4 );
 }

/*--------------------------------------------------------------------------*/

static void GenerateA( Index nr , Index nc )
{
 A.resize( nr );

 for( auto & Ai : A ) {
  Ai.resize( nc );
  for( auto & aij : Ai )
   aij = scale * ( 2 * drand48() - 1 );
  }
 }

/*--------------------------------------------------------------------------*/

static void Generateb( Index nr )
{
 b.resize( nr );

 for( auto & bj : b )
  bj = scale * nvar * ( 2 * drand48() - 1 ) / 4;
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

 if( drand48() <= 0.333 ) {  // "tight" LB
  LB = - drand48() * 5 * scale * nvar / 4;
  return;
  }

 if( drand48() <= 0.333 ) {  // "loose" LB
  LB = - drand48() * 5 * scale * nvar;
  return;
  }

 LB = - INF;
 }

/*--------------------------------------------------------------------------*/

static Subset && GenerateRand( Index m , Index k )
{
 // generate a sorted random k-vector of unique integers in 0 ... m - 1

 Subset rnd( m );
 std::iota( rnd.begin() , rnd.end() , 1 );

 for( Index i = 0 ; i < k ; i++ )
  swap( rnd[ i ] , rnd[ i + drand48() * ( m - i ) ] );

 rnd.resize( k );
 sort( rnd.begin() , rnd.end() );

 return( std::move( rnd ) );
 }

/*--------------------------------------------------------------------------*/

static void printAb( const PolyhedralFunction::MultiVector & tA ,
		     const std::vector < FunctionValue > & tb )
{
 PANIC( ( tA.size() == tb.size() ) || ( tA.size() + 1 == tb.size() ) );
 PANIC( tA.size() == m );
 for( auto & tai : tA )
  PANIC( tai.size() == nvar );

 cout << "n = " << nvar << ", m = " << m << endl;
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
  int rtrnLP = slvrLP->compute( false );

  // solve the NODBlock - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Solver * slvrNDO = (NDOBlock->get_registered_solvers()).front();
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
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char **argv )
{
 // reading command line parameters - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 long int seed = 1;
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
  #if DYNAMIC_VARS > 0
	   " seed [wchg nvar dens #rounds #chng %chng]"
  #else
	   " seed [wchg nvar dens #rounds rchng]"
  #endif
 		<< endl <<
           "       wchg: what to change, coded bit-wise "
		<< endl <<
           "             0 = add rows, 1 = delete rows "
		<< endl <<
           "             2 = modify rows, 3 = modify constants"
		<< endl <<
           "             4 = change global lower/upper bound"
  #if DYNAMIC_VARS > 0  
		<< endl <<
           "             5 = add variables rows, 6 = delete variables"
  #endif
		<< endl <<
           "             7 (+128) = do ""abstract"" changes"
	        << endl <<
           "       nvar: number of variables [10]"
	        << endl <<
           "       dens: rows / variables [4]"
	        << endl <<
           "       #rounds: how many iterations [80]"
	        << endl <<
           "       #chng: number changes [10]"
	        << endl <<
           "       %chng: probability of changing [50%]"
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

 srand48( seed );  // seed the pseudo-random number generator

 // constructing the data of the problem- - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // construct the matrix m x nvar matrix A and the m-vector b
 
 GenerateAb( m , nvar );
 GenerateLB();
 if( LB > - INF )     // a LB is defined
  b.push_back( LB );  // grow b by one to hold it

 cout.setf( ios::scientific, ios::floatfield );
 cout << setprecision( 10 );

 #if( LOG_LEVEL >= 3 )
  printAb( A , b );
  if( LB > - INF )
   cout << "LB = " << LB << endl;
  else
   cout << "LB = - INF" << endl;
 #endif

 // construction and loading of the objects - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
 // construct the "linearized" representation - - - - - - - - - - - - - - - -
 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  LPBlock = new PolyhedralFunctionBlock();

  // pass it the data of the PolyhedralFunction, *copying it* because it'll
  // be needed again later
  LPBlock->get_PolyhedralFunction().set_PolyhedralFunction( MultiVector( A ) ,
							    RealVector( b ) );
  // construct the Variable
  auto xLP = new std::vector< ColVariable >( nsvar );
  PolyhedralFunction::VarVector vars( nvar );
  auto vit = vars.begin();
  for( auto & xi : *xLP )
   *(vit++) = & xi;
  #if DYNAMIC_VARS > 0
   auto xLPd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xLPd )
    *(vit++) = & xi;
  #endif

  // now set the Variable, Constraint and Objective in the AbstractBlock
  LPBlock->add_static_variable( *xLP );
  #if DYNAMIC_VARS > 0
   LPBlock->add_dynamic_variable( *xLPd );
  #endif

  // then pass them to the PolyhedralFunction
  LPBlock->get_PolyhedralFunction().set_variables( std::move( vars ) );

  // generate the abstract representation
  SimpleConfiguration<int> cfg( 1 );  // 1 = linearized representation
  LPBlock->generate_abstract_variables( &cfg );
  LPBlock->generate_abstract_constraints();
  LPBlock->generate_objective();
  }

 // construct the "natural" representation- - - - - - - - - - - - - - - - - -
 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  NDOBlock = new PolyhedralFunctionBlock();

  // pass it the data of the PolyhedralFunction, letting it go
  NDOBlock->get_PolyhedralFunction().set_PolyhedralFunction( std::move( A ) ,
							     std::move( b ) );
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

  // now set the Variable, Constraint and Objective in the AbstractBlock
  NDOBlock->add_static_variable( *xNDO );
  #if DYNAMIC_VARS > 0
   NDOBlock->add_dynamic_variable( *xNDOd );
  #endif

  // then pass them to the PolyhedralFunction
  NDOBlock->get_PolyhedralFunction().set_variables( std::move( vars ) );

  // if no globally valid lower bound, set a "conditional" one
  if( LB <= - INF )
   NDOBlock->set_valid_lower_bound( lb , true );

  // generate the abstract representation
  SimpleConfiguration<int> cfg( 0 );  // 0 = natural representation
  NDOBlock->generate_abstract_variables( &cfg );
  NDOBlock->generate_abstract_constraints();
  NDOBlock->generate_objective();
  }

 // attach the Solver to the Block- - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // do this by reading appropriate BlockSolverConfig from files and use
 // set_SolverConfig()

 LPBlock->register_Solver( Solver::new_Solver( "CPXMILPSolver" ) );

 {
  ifstream BundleParFile( "BundlePar.txt" );
  if( ! BundleParFile.is_open() ) {
   cerr << "Error: cannot open file BundlePar.txt" << endl;
   return( 1 );
   }

  BlockSolverConfig * bsc = new BlockSolverConfig;
  BundleParFile >> *( bsc );
  BundleParFile.close();

  NDOBlock->set_SolverConfig( bsc );
  delete bsc;
  }

 // open log-file - - - - - - - - - - -  - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 #if( LOG_LEVEL >= 2 )
  ofstream LOGFile( logF , ofstream::out );
  if( ! LOGFile.is_open() )
   cerr << "Warning: cannot open log file """ << logF << """" << endl;
  else {
   LOGFile.setf( ios::scientific, ios::floatfield );
   LOGFile << setprecision( 10 );
   ((NDOBlock->get_registered_solvers()).front())->set_log( &LOGFile );
   }

  #if( LOG_LEVEL >= 3 )
   ((LPBlock->get_registered_solvers()).front())->set_par(
	                      CPXMILPSolver::strOutputFile , "LPBlock.lp" );
  #endif
 #endif

 // first solver call - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 #if( LOG_LEVEL >= 1 )
  cout << "First call: ";
 #endif

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

 for( Index rep = 0 ; rep < n_repeat ; ++rep ) {

  LOG1( rep << ": ");

  // add rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 1 ) && ( drand48() <= p_change ) ) {
   Index tochange = Index( drand48() * n_change );
   if( tochange ) {

    LOG1( "added " << tochange << " rows" );

    GenerateAb( tochange , nvar );

    // add them to the LP, *copying* them
    if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
     // in 50% of the cases do an "abstract" change
     LOG1( "(a)" );
     assert( false );  // not ready yet
     }
    else  // directly change the PolyhedralFunction
     if( tochange == 1 )
      LPBlock->get_PolyhedralFunction().add_row( RealVector( A[ 0 ] ) ,
						 b[ 0 ] );
     else
      LPBlock->get_PolyhedralFunction().add_rows( MultiVector( A ) , b );

    // add them to the NDO, letting them go
    if( tochange == 1 )
     NDOBlock->get_PolyhedralFunction().add_row( std::move( A[ 0 ] ) ,
						  b[ 0 ] );
    else
     NDOBlock->get_PolyhedralFunction().add_rows( std::move( A ) , b );

    LOG1( " - " );

    // update m
    m += tochange;

    // sanity checks
    PANIC( m == LPBlock->get_PolyhedralFunction().get_A().size() );
    PANIC( m == NDOBlock->get_PolyhedralFunction().get_A().size() );
    }
   }

  // delete rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 2 ) && ( drand48() <= p_change ) ) {
   Index tochange = min( m - 1 , Index( drand48() * n_change ) );
   auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );
   if( tochange ) {
    LOG1( "deleted " << tochange << " rows" );

    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );
    
    // in 50% of the cases do a ranged change, in the others a sparse change
    if( drand48() <= 0.5 ) {
    
     Index strt = drand48() * ( m - tochange );
     Index stp = strt + tochange;

     // remove them from the LP
     if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(r,a) - " );
      assert( false );  // not ready yet
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(r) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().delete_row( strt );
      else
       LPBlock->get_PolyhedralFunction().delete_rows( Range( strt , stp ) );
      }

     // remove them from the NDO
     if( tochange == 1 )
      NDOBlock->get_PolyhedralFunction().delete_row( strt );
     else
      NDOBlock->get_PolyhedralFunction().delete_rows( Range( strt , stp ) );
     }
    else {
     Subset nms = GenerateRand( tochange , m );

     // remove them from the LP
     if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(s,a) - " );
      assert( false );  // not ready yet
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(s) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().delete_row( nms[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().delete_rows( std::move( nms ) );
      }

     // remove them from the NDO
     if( tochange == 1 )
      NDOBlock->get_PolyhedralFunction().delete_row( nms[ 0 ] );
     else
      NDOBlock->get_PolyhedralFunction().delete_rows( std::move( nms ) );
     }

    // update m
    m -= tochange;

    // sanity checks
    PANIC( m == LPBlock->get_PolyhedralFunction().get_A().size() );
    PANIC( m == NDOBlock->get_PolyhedralFunction().get_A().size() );
    }
   }

  // modify rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 4 ) && ( drand48() <= p_change ) ) {
   Index tochange = Index( drand48() * n_change );
   if( tochange ) {
    LOG1( "modified " << tochange << " rows" );

    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );

    GenerateAb( tochange , nvar );

    // in 50% of the cases do a ranged change, in the others a sparse change
    if( drand48() <= 0.5 ) {
     Index strt = drand48() * ( m - tochange );
     Index stp = strt + tochange;

     // modify them in the LP, *copying* them
     if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(r,a) - " );
      assert( false );  // not ready yet
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(r) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().modify_row( strt ,
						     RealVector( A[ 0 ] ) ,
						     b[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().modify_rows( MultiVector( A ) , b ,
						      Range( strt , stp ) );
      }

     // modify them in the NDO, letting them go
     if( tochange == 1 )
      NDOBlock->get_PolyhedralFunction().modify_row( strt ,
						     std::move( A[ 0 ] ) ,
						     b[ 0 ] );
     else
      NDOBlock->get_PolyhedralFunction().modify_rows( std::move( A ) , b ,
						      Range( strt , stp ) );
     }
    else {
     Subset nms = GenerateRand( tochange , m );

     // modify them in the LP, *copying* them
     if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(s,a) - " );
      assert( false );  // not ready yet
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(s) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().modify_row( nms[ 0 ] ,
						     RealVector( A[ 0 ] ) ,
						     b[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().modify_rows( MultiVector( A ) , b ,
						      Subset( nms ) , true );
      }

     // modify them in the NDO, letting them go
     if( tochange == 1 )
      NDOBlock->get_PolyhedralFunction().modify_row( nms[ 0 ] ,
						     std::move( A[ 0 ] ) ,
						     b[ 0 ] );
     else
      NDOBlock->get_PolyhedralFunction().modify_rows( std::move( A ) , b ,
						      std::move( nms ) ,
						      true );
     }
    }
   }

  // modify constants - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 8 ) && ( drand48() <= p_change ) ) {
   Index tochange = Index( drand48() * n_change );
   if( tochange ) {
    LOG1( "modified " << tochange << " constants" );

    Generateb( tochange );

    auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
    PANIC( PF );
     
    // in 50% of the cases do a ranged change, in the others a sparse change
    if( drand48() <= 0.5 ) {
     Index strt = drand48() * ( m - tochange );
     Index stp = strt + tochange;

     // change them in the LP
     if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(r,a) - " );
      assert( false );  // not ready yet
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(r) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().modify_constant( strt , b[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().modify_constants( b ,
							   Range( strt ,
								  stp ) );
      }

     // modify them in the NDO
     if( tochange == 1 )
      NDOBlock->get_PolyhedralFunction().modify_constant( strt , b[ 0 ] );
     else
      NDOBlock->get_PolyhedralFunction().modify_constants( b ,
							   Range( strt ,
								  stp ) );
     }
    else {
     Subset nms = GenerateRand( tochange , m );

     // change them in the LP, *copying* them
     if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(s,a) - " );
      assert( false );  // not ready yet
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(s) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().modify_constant( nms[ 0 ] , b[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().modify_constants( b , Subset( nms ) ,
							   true );
      }

     // modify them in the NDO, letting them go
     if( tochange == 1 )
      NDOBlock->get_PolyhedralFunction().modify_constant( nms[ 0 ] , b[ 0 ] );
     else
      NDOBlock->get_PolyhedralFunction().modify_constants( b ,
							   std::move( nms ) ,
							   true );
     }
    }
   }

  // modify bound - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 16 ) && ( drand48() <= p_change ) ) {
   LOG1( "modified bound - " );

   GenerateLB();

   // change it in the LP
   if( ( wchg & 128 ) && ( drand48() <= p_change ) ) {
    // in 50% of the cases do an "abstract" change
    LOG1( "(a)" );
    assert( false );  // not ready yet
    }
   else  // directly change the PolyhedralFunction
    LPBlock->get_PolyhedralFunction().modify_bound( LB );

   LOG1( " - " );

   // change it in the NDO
   NDOBlock->get_PolyhedralFunction().modify_bound( LB );

   // if no globally valid lower bound, set a "conditional" one
   if( LB <= - INF )
    NDOBlock->set_valid_lower_bound( lb , true );
   }

 // add variables- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #if DYNAMIC_VARS > 0
  if( ( wchg & 32 ) && ( drand48() <= p_change ) ) {
   Index tochange = Index( drand48() * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " variables - " );

    GenerateA( m , tochange );

    // add them in the LP, *copying* the data
    std::list< ColVariable > nxLPd( tochange );
    std::vector< Variable * > nxpLP( tochange );
    auto nxit = nxLPd.begin();
    for( Index i = 0 ; i < tochange ; )
     nxpLP[ i++ ] = &(*(nxit++));

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

    if( tochange == 1 )
     NDOBlock->get_PolyhedralFunction().add_variable( nxpNDO[ 0 ] , A[ 0 ] );
    else
     NDOBlock->get_PolyhedralFunction().add_variables( std::move( nxpNDO ) ,
						       std::move( A ) );

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

  if( ( wchg & 64 ) && ( drand48() <= p_change ) ) {
   Index tochange = min( ndvar , Index( drand48() * n_change ) );
   if( tochange ) {
    LOG1( "removed " << tochange << " variables" );

    // in 50% of the cases do a ranged change, in the others a sparse change
    if( drand48() <= 0.5 ) {
     LOG1( "(r) - " );

     Index strt = drand48() * ( ndvar - tochange );
     Index stp = strt + tochange;

     // remove them from the LP
     auto xLPd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      LPBlock->get_PolyhedralFunction().remove_variable( strt );

      auto vp = &(*std::next( xNDOd->begin() , strt ));
      LPBlock->remove_dynamic_variable( *xLPd , vp );
      }
     else {
      LPBlock->get_PolyhedralFunction().remove_variables( Range( strt ,
								 stp ) );

      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = std::next( xLPd->begin() , strt );
      for( Index i = 0 ; i < tochange ; )
       itrs[ i++ ] = vit++;

      LPBlock->remove_dynamic_variables( *xLPd , itrs );
      }

     // remove them from the NDO
     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      NDOBlock->get_PolyhedralFunction().remove_variable( strt );

      auto vp = &(*std::next( xNDOd->begin() , strt ));
      NDOBlock->remove_dynamic_variable( *xNDOd , vp );
      }
     else {
      NDOBlock->get_PolyhedralFunction().remove_variables( Range( strt ,
								  stp ) );

      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = std::next( xNDOd->begin() , strt );
      for( Index i = 0 ; i < tochange ; )
       itrs[ i++ ] = vit++;

      NDOBlock->remove_dynamic_variables( *xNDOd , itrs );
      }
     }
    else {
     LOG1( "(s) - " );
     Subset nms = GenerateRand( tochange , ndvar );

     // remove them from the LP, *copying* names
     auto xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      LPBlock->get_PolyhedralFunction().remove_variable( nms[ 0 ] );

      auto vp = &(*std::next( xLPd->begin() , nms[ 0 ] ));
      LPBlock->remove_dynamic_variable( *xLPd , vp );
      }
     else {
      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = xLPd->begin();
      for( Index i = 0 ; i < tochange ; ) {
       itrs[ i ] = vit = std::next( vit , nms[ i ] - prev );
       prev = nms[ i++ ];
       }

      LPBlock->get_PolyhedralFunction().remove_variables( std::move( nms ) );
      LPBlock->remove_dynamic_variables( *xLPd , itrs );
      }

     // remove them from the NDO, letting names go
     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      NDOBlock->get_PolyhedralFunction().remove_variable( nms[ 0 ] );

      auto vp = &(*std::next( xNDOd->begin() , nms[ 0 ] ));
      NDOBlock->remove_dynamic_variable( *xNDOd , vp );
      }
     else {
      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = xNDOd->begin();
      for( Index i = 0 ; i < tochange ; ) {
       itrs[ i ] = vit = std::next( vit , nms[ i ] - prev );
       prev = nms[ i++ ];
       }

      NDOBlock->get_PolyhedralFunction().remove_variables( std::move( nms ) );
      NDOBlock->remove_dynamic_variables( *xNDOd , itrs );
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

  #endif  // DYNAMIC_VARS > 0

  // if verbose, print out stuff- - - - - - - - - - - - - - - - - - - - - - -

  #if( LOG_LEVEL >= 3 )
   ((LPBlock->get_registered_solvers()).front())->set_par(
		                  CPXMILPSolver::strOutputFile , "LPBlock-" +
		                  std::to_string( rep ) + ".lp" );
   auto PF = & NDOBlock->get_PolyhedralFunction();
   printAb( PF->get_A() , PF->get_b() );
  #endif

  // finally, re-solve the problems- - - - - - - - - - - - - - - - - - - - -

  AllPassed &= SolveBoth();

  }  // end( main loop )- - - - - - - - - - - - - - - - - - - - - - - - - - -
     // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( AllPassed )
  cout << GREEN( All test passed!! ) << endl;
 else
  cout << RED( Shit happened!! ) << endl;
 
 // destroy objects and vectors - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 delete NDOBlock;
 delete LPBlock;

 // terminate - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( 0 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*------------------------ End File test.cpp -------------------------------*/
/*--------------------------------------------------------------------------*/
