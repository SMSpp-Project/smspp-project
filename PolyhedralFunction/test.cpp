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
 * \version 0.20
 *
 * \date 03 - 10 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <fstream>
#include <sstream>
#include <iomanip>

#include "AbstractBlock.h"

#include "BundleSolver.h"

#include "CPXMILPSolver.h"

#include "PolyhedralFunction.h"

// #include "FakeSolver.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- MACROS ----------------------------------*/
/*--------------------------------------------------------------------------*/

#define LOG_LEVEL 1
// 0 = only pass/fail
// 1 = result of each test
// 2 = print data + solver log

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

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define DYNAMIC_VARS 0
// if 1, half of the variables are dynamic

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

static void ConstructLPConstraint( c_Index i , FRowConstraint & ci ,
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

static void printAb( const PolyhedralFunction::MultiVector & tA ,
		     const std::vector < FunctionValue > & tb )
{
 PANIC( tA.size() == tb.size() );
 PANIC( tA.size() == m );
 for( auto & tai : tA )
  PANIC( tai.size() == nvar );

 cout << "n = " << nvar << ", m = " << m << endl;
 for( Index i = 0 ; i < m ; ++i ) {
  cout << "A[ " << i << " ] = [ ";
  for( Index j = 0 ; j < nvar ; ++j )
   cout << A[ i ][ j ] << " ";
   cout << "], b[ " << i << " ] = " << b[ i ] << endl;
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
       <= 2e-8 * max( double( 1 ) , abs( max( foLP , foNDO ) ) ) ) {
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
   cout << " ~ LPBlock = ";
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
    cout << slvrNDO->get_ub() << endl;
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
 Index n_repeat = 0;

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

 #if( LOG_LEVEL >= 2 )
  printAb( A , b );
  if( LB > - INF )
   cout << "LB = " << LB << endl;
  else
   cout << "LB = - INF" << endl;
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
  for( auto & xi : *xLP )
   xi.set_Block( LPBlock );
  #if DYNAMIC_VARS > 0
   xLPd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xLPd )
    xi.set_Block( LPBlock );
  #endif

  vLP = new ColVariable;
  vLP->set_Block( LPBlock );

  // construct the m dynamic Constraint
  auto ALP = new std::list< FRowConstraint >( m );
  auto ALPit = ALP->begin();
  for( Index i = 0 ; i < m ; )
   ConstructLPConstraint( i++ , *(ALPit++) );

  // construct the static lower bound Constraint
  auto LBc = new BoxConstraint( LPBlock , vLP , LB );

  // construct the Objective
  auto objLP = new FRealObjective();
  objLP->set_function( new LinearFunction( { std::make_pair( vLP , 1 ) } ) );

  // now set the Variable, Constraint and Objective in the AbstractBlock
  LPBlock->add_static_variable( *vLP );
  #if DYNAMIC_VARS > 0
   LPBlock->add_dynamic_variable( *xLPd );
  #endif
  LPBlock->add_static_variable( *xLP );
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
  #if DYNAMIC_VARS > 0
   auto xNDOd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xNDOd )
    xi.set_Block( LPBlock );
  #endif
  PolyhedralFunction::VarVector vars( nvar );
  auto vit = vars.begin();
  for( auto & xi : *xNDO ) {
   *(vit++) = & xi;
   xi.set_Block( NDOBlock );
   }

  if( LB > - INF )     // a LB is defined
   b.push_back( LB );  // grow b by one to hold it

  // construct the Objective
  auto objNDO = new FRealObjective();
  objNDO->set_function( new PolyhedralFunction( std::move( vars ) ,
						std::move( A ) ,
						std::move( b ) ) );

  // now set the Variable and Objective in the AbstractBlock
  NDOBlock->add_static_variable( *xNDO );
  #if DYNAMIC_VARS > 0
   NDOBlock->add_dynamic_variable( *xNDOd );
  #endif
  NDOBlock->set_objective( objNDO );

  // set lower bound, be it "hard" or "conditional"
  if( LB > - INF )
   NDOBlock->set_valid_lower_bound( LB );
  else
   NDOBlock->set_valid_lower_bound( lb , true );
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
  else
   ((NDOBlock->get_registered_solvers()).front())->set_log( &LOGFile );

  ((LPBlock->get_registered_solvers()).front())->set_par(
	                      CPXMILPSolver::strOutputFile , "LPBlock.lp" );
 #endif

 // first solver call - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 cout << "First call: ";
 cout.setf( ios::scientific, ios::floatfield );
 cout << setprecision( 6 );

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

  LOG1( "Changes: ");

  // add rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 1 ) && ( drand48() <= p_change ) ) {
   Index tochange = Index( drand48() * n_change );
   if( tochange ) {

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
    PANIC( m == PF->get_b().size() );
    PANIC( m == cnst->size() );
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
     LOG1( "(r) - " );
    
     Index strt = drand48() * ( m - tochange );
     Index stp = strt + tochange;

     // remove them from the LP
     auto cit = std::next( cnst->begin() , strt );
     if( tochange == 1 )
      LPBlock->remove_dynamic_constraint( *cnst , cit );
     else {
      std::vector< std::list< FRowConstraint >::iterator > itrs( tochange );
      for( Index i = 0 ; i < tochange ; )
       itrs[ i++ ] = cit++;

      LPBlock->remove_dynamic_constraints( *cnst , itrs );
      }
 
     // remove them from the NDO
     if( tochange == 1 )
      PF->delete_row( strt );
     else
      PF->delete_rows( Range( strt , stp ) );
     }
    else {
     LOG1( "(s) - " );
     Subset nms = GenerateRand( tochange , m );

     // remove them from the LP
     if( tochange == 1 )
      LPBlock->remove_dynamic_constraint( *cnst , std::next( cnst->begin() ,
							     nms[ 0 ] ) );
     else {
      std::vector< std::list< FRowConstraint >::iterator > itrs( tochange );
      Index prev = 0;
      auto cit = cnst->begin();
      for( Index i = 0 ; i < tochange ; ) {
       itrs[ i ] = cit = std::next( cit , nms[ i ] - prev );
       prev = nms[ i++ ];
       }

      LPBlock->remove_dynamic_constraints( *cnst , itrs );
      }
    
     // remove them from the NDO
     if( tochange == 1 )
      PF->delete_row( nms[ 0 ] );
     else
      PF->delete_rows( std::move( nms ) );
     }

    // update m
    m -= tochange;

    // sanity checks
    PANIC( m == PF->get_A().size() );
    PANIC( m == PF->get_b().size() );
    PANIC( m == cnst->size() );
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
     LOG1( "(r) - " );

     Index strt = drand48() * ( m - tochange );
     Index stp = strt + tochange;

     // modify them in the LP
     vLP = LPBlock->get_static_variable< ColVariable >( 0 );
     xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
     #if DYNAMIC_VARS > 0
      xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     #endif
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     auto cit = std::next( cnst->begin() , strt );
     for( Index i = 0 ; i < tochange ; )
      ConstructLPConstraint( i++ , *(cit++) );

     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_row( strt , std::move( A[ 0 ] ) , b[ 0 ] );
     else
      PF->modify_rows( std::move( A ) , b , Range( strt , stp ) );
     }
    else {
     LOG1( "(s) - " );
     Subset nms = GenerateRand( tochange , m );

     // modify them in the LP
     vLP = LPBlock->get_static_variable< ColVariable >( 0 );
     xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
     #if DYNAMIC_VARS > 0
      xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     #endif
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     Index prev = 0;
     auto cit = cnst->begin();
     for( Index i = 0 ; i < tochange ; ) {
      cit = std::next( cit , nms[ i ] - prev );
      prev = nms[ i ];
      ConstructLPConstraint( i++ , *cit );
      }

     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_row( nms[ 0 ] , std::move( A[ 0 ] ) , b[ 0 ] );
     else
      PF->modify_rows( std::move( A ) , b , std::move( nms ) , true );
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
     LOG1( "(r) - " );

     Index strt = drand48() * ( m - tochange );
     Index stp = strt + tochange;

     // change them in the LP
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     auto cit = std::next( cnst->begin() , strt );
     for( Index i = 0 ; i < tochange ; )
      (*(cit++)).set_lhs( b[ i++ ] );

     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_constant( strt , b[ 0 ] );
     else
      PF->modify_constants( b , Range( strt , stp ) );
     }
    else {
     LOG1( "(s) - " );
     Subset nms = GenerateRand( tochange , m );

     // change them in the LP
     auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

     Index prev = 0;
     auto cit = cnst->begin();
     for( Index i = 0 ; i < tochange ; ) {
      cit = std::next( cit , nms[ i ] - prev );
      prev = nms[ i ];
      (*cit).set_lhs( b[ i++ ] );
      }

     // modify them in the NDO
     if( tochange == 1 )
      PF->modify_constant( nms[ 0 ] , b[ 0 ] );
     else
      PF->modify_constants( b , std::move( nms ) , true );
     }
    }
   }

  // modify bound - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 16 ) && ( drand48() <= p_change ) ) {
   LOG1( "modified bound - " );

   GenerateLB();

   // change it in the LP
   auto cnst = LPBlock->get_static_constraint< BoxConstraint >( 0 );
   cnst->set_lhs( LB );

   // modify it in the NDO
   auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
   PANIC( PF );

   PF->modify_bound( LB );

   // set lower bound, be it "hard" or "conditional"
   if( LB > - INF )
    NDOBlock->set_valid_lower_bound( LB );
   else
    NDOBlock->set_valid_lower_bound( lb , true );
   }

 // add variables- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #if DYNAMIC_VARS > 0
  if( ( wchg & 32 ) && ( drand48() <= p_change ) ) {
   Index tochange = Index( drand48() * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " variables - " );

    GenerateA( m , tochange );

    // add them in the LP
    std::list< ColVariable > nxLPd( tochange );
    std::vector< Variable * > nxp( tochange );
    auto nxit = nxLPd.begin();
    for( Index i = 0 ; i < tochange ; )
     nxp[ i++ ] = &(*(nxit++));

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
      for( Index j = 0 ; j < ncp ; ++j ) {
       ncp[ i ].first = nxp[ i ];
       ncp[ i ].second = - A[ i ][ j ];
       }
      fi->add_variables( std::move( ncp ) );
      }

    // add them in the NDO
    std::list< ColVariable > nxNDOd( tochange );
    auto nxit = nxNDOd.begin();
    for( Index i = 0 ; i < tochange ; )
     nxp[ i++ ] = &(*(nxit++));

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
     xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
     auto cnst_it =
             LPBlock->get_dynamic_constraint< FRowConstraint >( 0 )->begin();
     if( tochange == 1 ) {
      for( Index i = 0 ; i < m ; ++i ) {
       auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
       PANIC( fi );
       fi->remove_variable( strt + 1 );
       }

      auto vp = &(*std::next( *xLPd() , strt ));
      LPBlock->remove_dynamic_variable( *xLPd , vp );
      }
     else {
      for( Index i = 0 ; i < m ; ++i ) {
       auto fi = dynamic_cast< LinearFunction * >(
					       (cnst_it++)->get_function() );
       PANIC( fi );
       fi->remove_variables( Range( strt + 1 , stp + 1 ) ) , true );
       }

      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = std::next( xLPd->begin() , strt );
      for( Index i = 0 ; i < tochange ; ) {
       itrs[ i++ ] = vit++;
      LPBlock->remove_dynamic_variables( *xLPd , itrs );
      }

     // remove them from the NDO
     auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
     PANIC( PF );

     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      PF->remove_variable( strt );

      auto vp = &(*std::next( xNDOd->begin() , strt ));
      LPBlock->remove_dynamic_variable( *xLPd , vp );
      }
     else {
      PF->remove_variables( Range( strt , stp ) );

      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = std::next( xNDOd->begin() , strt );
      for( Index i = 0 ; i < tochange ; )
       itrs[ i++ ] = vit++;

      LPBlock->remove_dynamic_variables( *xNDOd , itrs );
      }
     }
    else {
     LOG1( "(s) - " );
     Subset nms = GenerateRand( tochange , ndvar );

     // remove them from the LP
     xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
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

      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = xLPd->begin();
      for( Index i = 0 ; i < tochange ; ) {
       itrs[ i ] = vit = std::next( vit , nms[ i ] - prev );
       prev = nms[ i++ ];
       }
      LPBlock->remove_dynamic_variables( *xLPd , itrs );
      }

     // remove them from the NDO
     auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
     PANIC( PF );

     auto xNDOd = NDOBlock->get_dynamic_variable< ColVariable >( 0 );
     if( tochange == 1 ) {
      PF->remove_variable( nms[ 0 ] );

      auto vp = &(*std::next( xNDOd->begin() , nms[ 0 ] ));
      LPBlock->remove_dynamic_variable( *xLPd , vp );
      }
     else {
      std::vector< std::list< ColVariable >::iterator > itrs( tochange );
      Index prev = 0;
      auto vit = xNDOd->begin();
      for( Index i = 0 ; i < tochange ; ) {
       itrs[ i ] = vit = std::next( vit , nms[ i ] - prev );
       prev = nms[ i++ ];
       }

      PF->remove_variables( std::move( nms ) );
      LPBlock->remove_dynamic_variables( *xNDOd , itrs );
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

  #if( LOG_LEVEL >= 2 )
   ((LPBlock->get_registered_solvers()).front())->set_par(
		                  CPXMILPSolver::strOutputFile , "LPBlock-" +
		                  std::to_string( rep ) + ".lp" );
   auto PF = dynamic_cast< PolyhedralFunction * >(
	       NDOBlock->get_objective< FRealObjective >()->get_function() );
   PANIC( PF );
   printAb( PF->get_A() , PF->get_b() );
  #endif

  // finally, re-solve the problems- - - - - - - - - - - - - - - - - - - - -

  AllPassed &= SolveBoth();

  }  // end( main loop )- - - - - - - - - - - - - - - - - - - - - - - - - - -
     // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( AllPassed )
  cout << "All test passed!!" << endl;
 else
  cout << "Shit happened!!" << endl;
 
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
