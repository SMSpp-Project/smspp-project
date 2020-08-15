/*--------------------------------------------------------------------------*/
/*-------------------------- File test.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing PolyhedralFunctionBlock
 *
 * A "random" PolyhedralFunction is constructed inside a
 * PolyhedralFunctionBlock, then R3-Block-ed to another. The first is
 * configured to use the "linearized" representation, and has a MILPSolver
 * attached, plus an UpdateSolver that maps all the Modification to the
 * second, which is configured to use the "natural" representation and has a
 * BundleSolver attached. Two PolyhedralFunctionBlock are then repeatedly
 * randomly modified "in the same way", and re-solved several times.
 *
 * \version 0.10
 *
 * \date 09 - 08 - 2020
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

#define LOG_LEVEL 1
// 0 = only pass/fail
// 1 = result of each test
// 2 = + solver log
// 3 = + save LP file
// 4 = + print data

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) cout << x
 #define CLOG1( y , x ) if( y ) cout << x
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

#include "BundleSolver.h"

#include "CPXMILPSolver.h"

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

static void printAb( const MultiVector & tA , const RealVector & tb ,
		     double lb )
{
 PANIC( ( tA.size() == tb.size() ) || ( tA.size() + 1 == tb.size() ) );
 PANIC( tA.size() == m );
 for( auto & tai : tA )
  PANIC( tai.size() == nvar );

 cout << "n = " << nvar << ", m = " << m;
 if( lb > - INF )
  cout << ", LB = " << lb << endl;
 else
  cout << ", LB = - INF" << endl;

 for( Index i = 0 ; i < m ; ++i ) {
  cout << "A[ " << i << " ] = [ ";
  for( Index j = 0 ; j < nvar ; ++j )
   cout << tA[ i ][ j ] << " ";
   cout << "], b[ " << i << " ] = " << tb[ i ] << endl;
  }
 }

/*--------------------------------------------------------------------------*/

static void ConstructLPConstraint( Index i , FRowConstraint & ci ,
				   bool setblock = true )
{
 // construct constraint ci out of A[ i ] and b[ i ]:
 // the constraint is b[ i ] <= vLP - \sum_j Ai[ j ] * xLP[ j ] <= INF
 //
 // note: constraints are constructed dense (elements == 0, which are
 //       anyway quite unlikely, are ignored) to make things simpler
 //
 // note: variable x[ i ] is given index i + 1, variable v has index 0

 ci.set_lhs( b[ i ] );
 ci.set_rhs( INF );
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
 // change the constant == LHS of the constraint
 ci.set_lhs( b[ i ] , iAM );

 // now change the coefficients, except that of v that is always 1
 LinearFunction::Vec_FunctionValue coeffs( nvar );

 for( Index j = 0 ; j < nvar ; ++j )
  coeffs[ j ] = - A[ i ][ j ];

 auto f = static_cast< LinearFunction * >( ci.get_function() );
 f->modify_coefficients( std::move( coeffs ) , Range( 1 , nvar + 1 ) , iAM );
 }

/*--------------------------------------------------------------------------*/

static bool SolveBoth( void ) 
{
 try {
  // solve the LPBlock- - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Solver * slvrLP = (LPBlock->get_registered_solvers()).front();
  #if DETACH_LP
   Solver * slvrU = (LPBlock->get_registered_solvers()).back();
   LPBlock->unregister_Solver( slvrU );
   LPBlock->unregister_Solver( slvrLP );
   LPBlock->register_Solver( slvrLP );
   LPBlock->register_Solver( slvrU );
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

 long int seed = 0;
 Index wchg = 159;
 double dens = 3;
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
           "       wchg: what to change, coded bit-wise [159]"
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
           "             7 (+128) = do \"abstract\" changes"
	        << endl <<
           "       nvar: number of variables [10]"
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
 // construct the matrix m x nvar matrix A and the m-vector b
 
 GenerateAb( m , nvar );
 GenerateLB();

 cout.setf( ios::scientific, ios::floatfield );
 cout << setprecision( 10 );

 #if( LOG_LEVEL >= 4 )
  printAb( A , b , LB );
 #endif

 // construction and loading of the objects - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
 // construct the "linearized" representation - - - - - - - - - - - - - - - -
 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  LPBlock = new PolyhedralFunctionBlock();

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

  // pass all the data of the PolyhedralFunction
  LPBlock->get_PolyhedralFunction().set_PolyhedralFunction( std::move( A ) ,
							    std::move( b ) ,
							    LB );
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

  NDOBlock = dynamic_cast< PolyhedralFunctionBlock * >(
						  LPBlock->get_R3_Block() );
  assert( NDOBlock );  // excess of caution (we know it is)

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

  // set the worst-case "conditional" lower bound
  NDOBlock->set_valid_lower_bound( lb , true );

  // generate the abstract representation
  SimpleConfiguration<int> cfg( 0 );  // 0 = natural representation
  NDOBlock->generate_abstract_variables( &cfg );
  NDOBlock->generate_abstract_constraints();
  NDOBlock->generate_objective();
  }

 // attach the Solver to the Block- - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // for LPBlock, "manually" attach a CPXMILPSolver and an UpdateSolver

 LPBlock->register_Solver( Solver::new_Solver( "CPXMILPSolver" ) );
 LPBlock->register_Solver( new UpdateSolver( NDOBlock ) );

 {
  // for NDOBlock do this by reading appropriate BlockSolverConfig from
  // files and use set_SolverConfig()
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
 //
 // IMPORTANT NOTE: only LPBlock is changed, because UpdateSolver takes
 //                 care of intercepting all (physical) Modification and
 //                 map_forward them to NDOBlock

 for( Index rep = 0 ; rep < n_repeat ; ++rep ) {

  LOG1( rep << ": ");

  // add rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 1 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = Index( dis( rg ) * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " rows" );

    GenerateAb( tochange , nvar );

    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

    if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
     // in 50% of the cases do an "abstract" change
     LOG1( "(a)" );

     vLP = LPBlock->get_static_variable< ColVariable >( 0 );
     xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
     #if DYNAMIC_VARS > 0
      xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     #endif

     std::list< FRowConstraint > nc( tochange );
     auto ncit = nc.begin();
     for( Index i = 0 ; i < tochange ; )
      ConstructLPConstraint( i++ , *(ncit++) );

     LPBlock->add_dynamic_constraints( *cnst , nc );
     }
    else  // directly change the PolyhedralFunction in LPBlock
     if( tochange == 1 )
      LPBlock->get_PolyhedralFunction().add_row( std::move( A[ 0 ] ) , b[ 0 ] );
     else
      LPBlock->get_PolyhedralFunction().add_rows( std::move( A ) , b );

    LOG1( " - " );

    // update m
    m += tochange;

    // sanity checks
    PANIC( m == LPBlock->get_PolyhedralFunction().get_A().size() );
    PANIC( m == NDOBlock->get_PolyhedralFunction().get_A().size() );
    PANIC( m == cnst->size() );
    }
   }

  // delete rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 2 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = min( m - 1 , Index( dis( rg ) * n_change ) );
   if( tochange ) {
    LOG1( "deleted " << tochange << " rows" );

    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
    
     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(r,a) - " );
      if( tochange == 1 )
       LPBlock->remove_dynamic_constraint( *cnst , std::next( cnst->begin() ,
							      strt ) );
      else
       LPBlock->remove_dynamic_constraints( *cnst , Range( strt , stp ) );
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(r) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().delete_row( strt );
      else
       LPBlock->get_PolyhedralFunction().delete_rows( Range( strt , stp ) );
      }
     }
    else {  // in the other 50% of the cases, do a sparse change
     Subset nms = GenerateRand( m , tochange );

     // remove them from the LP
     if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(s,a) - " );
      if( tochange == 1 )
       LPBlock->remove_dynamic_constraint( *cnst , std::next( cnst->begin() ,
							      nms[ 0 ] ) );
      else
       LPBlock->remove_dynamic_constraints( *cnst , std::move( nms ) , true );
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(s) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().delete_row( nms[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().delete_rows( std::move( nms ) );
      }
     }

    // update m
    m -= tochange;

    // sanity checks
    PANIC( m == LPBlock->get_PolyhedralFunction().get_A().size() );
    PANIC( m == NDOBlock->get_PolyhedralFunction().get_A().size() );
    PANIC( m == cnst->size() );
    }
   }

  // modify rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 4 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = std::min( m , Index( dis( rg ) * n_change ) );
   if( tochange ) {
    LOG1( "modified " << tochange << " rows" );

    GenerateAb( tochange , nvar );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(r,a) - " );

      // modify them in the LP
      vLP = LPBlock->get_static_variable< ColVariable >( 0 );
      xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
      #if DYNAMIC_VARS > 0
       xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
      #endif
      auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

      // send all the Modification to the same channel
      Observer::ChnlName chnl = LPBlock->open_channel();

      auto cit = std::next( cnst->begin() , strt );
      for( Index i = 0 ; i < tochange ; ++i )
       ChangeLPConstraint( i , *(cit++) ,
			   Observer::make_par( eModBlck , chnl ) );

      LPBlock->close_channel( chnl );
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(r) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().modify_row( strt ,
						     std::move( A[ 0 ] ) ,						             b[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().modify_rows( std::move( A ) , b ,
						      Range( strt , stp ) );
      }
     }
    else {  // in the other 50% of the cases, do a sparse change
     Subset nms = GenerateRand( m , tochange );

     if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(s,a) - " );

      vLP = LPBlock->get_static_variable< ColVariable >( 0 );
      xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
      #if DYNAMIC_VARS > 0
       xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
      #endif
      auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

      // send all the Modification to the same channel
      Observer::ChnlName chnl = LPBlock->open_channel();

      Index prev = 0;
      auto cit = cnst->begin();
      for( Index i = 0 ; i < tochange ; ++i ) {
       cit = std::next( cit , nms[ i ] - prev );
       prev = nms[ i ];
       ChangeLPConstraint( i , *cit , Observer::make_par( eModBlck , chnl ) );
       }

      LPBlock->close_channel( chnl );
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(s) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().modify_row( nms[ 0 ] ,
						     std::move( A[ 0 ] ) ,
						     b[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().modify_rows( std::move( A ) , b ,
						      Subset( nms ) , true );
      }
     }
    }
   }

  // modify constants - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 8 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = std::min( m , Index( dis( rg ) * n_change ) );
   if( tochange ) {
    LOG1( "modified " << tochange << " constants" );

    Generateb( tochange );

    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     Index strt = dis( rg ) * ( m - tochange );
     Index stp = strt + tochange;

     if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(r,a) - " );

      auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

      // send all the Modification to the same channel
      Observer::ChnlName chnl = LPBlock->open_channel();

      auto cit = std::next( cnst->begin() , strt );
      for( Index i = 0 ; i < tochange ; )
       (*(cit++)).set_lhs( b[ i++ ] , Observer::make_par( eModBlck , chnl ) );

      LPBlock->close_channel( chnl );
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
     }
    else {  // in the other 50% of the cases, do a sparse change
     Subset nms = GenerateRand( m , tochange );

     if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
      // in 50% of the cases do an "abstract" change
      LOG1( "(s,a) - " );

      auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

      // send all the Modification to the same channel
      Observer::ChnlName chnl = LPBlock->open_channel();

      Index prev = 0;
      auto cit = cnst->begin();
      for( Index i = 0 ; i < tochange ; ) {
       cit = std::next( cit , nms[ i ] - prev );
       prev = nms[ i ];
       (*cit).set_lhs( b[ i++ ] , Observer::make_par( eModBlck , chnl ) );
       }

      LPBlock->close_channel( chnl );
      }
     else {  // directly change the PolyhedralFunction
      LOG1( "(s) - " );
      if( tochange == 1 )
       LPBlock->get_PolyhedralFunction().modify_constant( nms[ 0 ] , b[ 0 ] );
      else
       LPBlock->get_PolyhedralFunction().modify_constants( b , Subset( nms ) ,
							   true );
      }
     }
    }
   }

  // modify bound - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 16 ) && ( dis( rg ) <= p_change ) ) {
   LOG1( "modified bound" );

   GenerateLB();

   if( ( wchg & 128 ) && ( dis( rg ) <= p_change ) ) {
    // in 50% of the cases do an "abstract" change
    LOG1( "(a)" );

    LPBlock->get_static_constraint< BoxConstraint >( 0 )->set_lhs( LB );
    }
   else  // directly change the PolyhedralFunction
    LPBlock->get_PolyhedralFunction().modify_bound( LB );

   LOG1( " - " );

   /*!!??
   // if no globally valid lower bound, set a "conditional" one
   if( LB <= - INF )
    NDOBlock->set_valid_lower_bound( lb , true );
   */
   }

 // add variables- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #if DYNAMIC_VARS > 0
  if( ( wchg & 32 ) && ( dis( rg ) <= p_change ) ) {
   Index tochange = Index( dis( rg ) * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " variables - " );

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
     Subset nms = GenerateRand( ndvar , tochange );

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
   #if( LOG_LEVEL >= 4 )
    cout << endl << "LPBlock-PF: ";
    auto PF = & LPBlock->get_PolyhedralFunction();
    printAb( PF->get_A() , PF->get_b() , PF->get_global_lower_bound() );
    cout << "NDOBlock-PF: ";
    PF = & NDOBlock->get_PolyhedralFunction();
    printAb( PF->get_A() , PF->get_b() , PF->get_global_lower_bound() );
   #endif
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

 NDOBlock->set_SolverConfig();  // reset all Solver attached to NDOBlock
 delete NDOBlock;
 LPBlock->set_SolverConfig();   // reset all Solver attached to LPBlock
 delete LPBlock;

 // terminate - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( 0 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*------------------------ End File test.cpp -------------------------------*/
/*--------------------------------------------------------------------------*/
