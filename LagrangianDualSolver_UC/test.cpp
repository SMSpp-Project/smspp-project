/*--------------------------------------------------------------------------*/
/*-------------------------- File test.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing LagrangianDualSolver with UCBlock
 *
 * An UCBlock instance is loaded from netCDF file, two different Solver are
 * registered to the UCBlock, the second of which is assumed to be a
 * LagrangianDualSolver, the UCBlock is solved by the Solver and the results
 * are compared.
 *
 * Although the testerdoes not even include BundleSolver, some
 * BundleSolver-specific steps are 
 *
 * The tester has some parts for the future extension when the UCBlock is
 * repeatedly randomly modified and re-solved several times, but this is not
 * done yet.
 *
 * \version 0.20
 *
 * \date 13 - 02 - 2021
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

#define LOG_LEVEL 2
// 0 = only pass/fail
// 1 = result of each test
// 2 = + solver log
// 3 = + save LP file
// 4 = + print data

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) cout << x
 #define CLOG1( y , x ) if( y ) cout << x

 #if( LOG_LEVEL >= 2 )
  #define LOG_ON_COUT 0
  // if nonzero, the 2nd Solver (LagrangianDualSolver) log is sent on cout
  // rather than on a file
 #endif
#else
 #define LOG1( x )
 #define CLOG1( y , x )
#endif

/*--------------------------------------------------------------------------*/
// if nonzero, the 2nd Solver attched to the UCBlock is assumed to be a
// LagrangianDualSolver using the BundleSolver as the "inner" solver;
// parameters from the BlockSolverConfig are read and set so that, if
// "easy components" are used, all UnitBlock that are ThermalUnitBlock or
// HydroSystemUnitBlock are attached an appropriate Solver, whereas all
// other inner Block are treated as "easy components"

#define USE_BundleSolver 1

/*--------------------------------------------------------------------------*/
// if nonzero, the 1st Solver attched to the UCBlock is detached
// and re-attached to it at all iterations

#define DETACH_1ST 0

// if nonzero, the 2nd Solver attched to the UCBlock is detached and
// re-attached to it at all iterations

#define DETACH_2ND 0

/*--------------------------------------------------------------------------*/
// if nonzero, the two Block are not solved at every round of changes, but
// only every SKIP_BEAT + 1 rounds. this allows changes to accumulate, and
// therefore puts more pressure on the Modification handling of the Solver
// (in case this tries to do "smart" things rather than dumbly processing
// each one in turn)
//
// note that the number of rounds of changes is them multiplied by
// SKIP_BEAT + 1, so that the input parameter still dictates the number of
// Block solutions

#define SKIP_BEAT 0

/*--------------------------------------------------------------------------*/

#define USECOLORS 1
#if( USECOLORS )
 #define RED( x ) "\x1B[31m" #x "\033[0m"
 #define GREEN( x ) "\x1B[32m" #x "\033[0m"
#else
 #define RED( x ) #x
 #define GREEN( x ) #x
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <fstream>
#include <sstream>
#include <iomanip>

#include <random>

#include "BlockSolverConfig.h"

#include "PolyhedralFunctionBlock.h"

#include "UCBlock.h"

#if USE_BundleSolver
 #include "ThermalUnitBlock.h"
 #include "HydroSystemUnitBlock.h"
#endif


/*!!
#include "FRealObjective.h"

#include "FRowConstraint.h"

#include "LinearFunction.h"

#include "OneVarConstraint.h"
!!*/

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

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

const double scale = 10;
const char *const logF = "log.txt";

const FunctionValue INF = SMSpp_di_unipi_it::Inf< FunctionValue >();

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

Block * TestBlock;         // the [UC]Block that is solved

std::mt19937 rg;           // base random generator
std::uniform_real_distribution<> dis( 0.0 , 1.0 );

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template<class T>
static void Str2Sthg( const char* const str , T &sthg )
{
 istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/

#if USE_BundleSolver

static void Configure_HSUB( HydroSystemUnitBlock * hsub )
{
 // ensure that the PolyhedralFunctionBlock in the HydroSystemUnitBlock is
 // Configured to use the "linearised" representation of the Objective

 for( auto sb : hsub->get_nested_Blocks() )
  if( auto pfb = dynamic_cast< PolyhedralFunctionBlock * >( sb ) ) {
   auto sci = new SimpleConfiguration< int >;
   sci->f_value = 1;
   auto bc = new BlockConfig;
   bc->f_static_variables_Configuration = sci;
   pfb->set_BlockConfig( bc );
   }
 }

#endif

/*--------------------------------------------------------------------------*/

static double rndfctr( void )
{
 // return a random number between 0.5 and 2, with 50% probability of being
 // < 1
 double fctr = dis( rg ) - 0.5;
 return( fctr < 0 ? - fctr : fctr * 4 );
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

static void PrintResults( bool hs , int rtrn , double fo )
{
 if( hs )
  cout << fo;
 else
  if( rtrn == Solver::kInfeasible )
   cout << "    Unfeas";
  else
   if( rtrn == Solver::kUnbounded )
    cout << "      Unbounded";
   else
    cout << "      Error!";
 }

/*--------------------------------------------------------------------------*/

static bool SolveBoth( void ) 
{
 try {
  // solve with the 1st Solver- - - - - - - - - - - - - - - - - - - - - - - -
  #if( LOG_LEVEL >= 1 )
   std::clock_t c_start = std::clock();
  #endif
  Solver * Slvr1 = TestBlock->get_registered_solvers().front();
  #if DETACH_1ST
   TestBlock->unregister_Solver( Slvr1 );
   TestBlock->register_Solver( Slvr1 , true );  // push it to the front
  #endif
  int rtrn1st = Slvr1->compute( false );
  bool hs1st = ( ( rtrn1st >= Solver::kOK ) && ( rtrn1st < Solver::kError )
		 && ( rtrn1st != Solver::kUnbounded )
		 && ( rtrn1st != Solver::kInfeasible ) )
               || ( rtrn1st == Solver::kLowPrecision );
  double fo1st = hs1st ? Slvr1->get_var_value() : -INF;
  #if( LOG_LEVEL >= 1 )
   double time1 = double( std::clock() - c_start ) / double( CLOCKS_PER_SEC );
  #endif

  if( TestBlock->get_registered_solvers().size() == 1 ) {
   #if( LOG_LEVEL >= 1 )
    cout << "Solver1 (" << time1 << ") = ";
    PrintResults( hs1st , rtrn1st , fo1st );
    cout << endl;
   #endif
   return( true );
   }

  // solve with the 2nd Solver- - - - - - - - - - - - - - - - - - - - - - - -
  #if( LOG_LEVEL >= 1 )
   c_start = std::clock();
   cout.setf( ios::scientific, ios::floatfield );
   cout << setprecision( 2 ) << flush;
  #endif
  Solver * Slvr2 = TestBlock->get_registered_solvers().back();
  #if DETACH_2ND
   TestBlock->unregister_Solver( Slvr2 );
   TestBlock->register_Solver( Slvr2 );  // push it to the back
  #endif
  int rtrn2nd = Slvr2->compute( false );

  bool hs2nd = ( ( rtrn2nd >= Solver::kOK ) && ( rtrn2nd < Solver::kError )
		 && ( rtrn2nd != Solver::kUnbounded )
		 && ( rtrn2nd != Solver::kInfeasible ) )
               || ( rtrn2nd == Solver::kLowPrecision );
  double fo2nd = hs2nd ? Slvr2->get_var_value() : -INF;
  #if( LOG_LEVEL >= 1 )
   double time2 = double( std::clock() - c_start ) / double( CLOCKS_PER_SEC );
  #endif

  if( hs1st && hs2nd && ( abs( fo1st - fo2nd ) <= 2e-7 *
			  max( double( 1 ) , max( abs( fo1st ) ,
						  abs( fo2nd ) ) ) ) ) {
   LOG1( time1 << " - " << time2 << " - OK(f)" << endl );
   return( true );
   }

  if( ( rtrn1st == Solver::kInfeasible ) &&
      ( rtrn2nd == Solver::kInfeasible ) ) {
   LOG1( time1 << " - " << time2 << " - OK(e)" << endl );
   return( true );
   }

  if( ( rtrn1st == Solver::kUnbounded ) &&
      ( rtrn2nd == Solver::kUnbounded ) ) {
   LOG1( time1 << " - " << time2 << " - OK(u)" << endl );
   return( true );
   }

  #if( LOG_LEVEL >= 1 )
   cout << "Solver1 (" << time1 << ") = ";
   cout << setprecision( 7 );
   PrintResults( hs1st , rtrn1st , fo1st );

   cout << " ~ Solver2 (" << setprecision( 2 ) << time2 << ") = ";
   cout << setprecision( 7 );
   PrintResults( hs2nd , rtrn2nd , fo2nd );
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

 /*!!
 long int seed = 0;
 Index wchg = 127;
 double dens = 4;  
 double p_change = 0.5;
 Index n_change = 10;
 Index n_repeat = 40;
 !!*/

 switch( argc ) {
  /*!!
  case( 8 ): Str2Sthg( argv[ 7 ] , p_change );
  case( 7 ): Str2Sthg( argv[ 6 ] , n_change );
  case( 6 ): Str2Sthg( argv[ 5 ] , n_repeat );
  case( 3 ): Str2Sthg( argv[ 2 ] , wchg );
  case( 2 ): Str2Sthg( argv[ 1 ] , seed );
             break;
	     !!*/
  case( 3 ): break;
  case( 2 ): break;
  default: cerr << "Usage: " << argv[ 0 ] << "UC-file [BSC-file]"
		<< endl <<
	   "       BSC-file: BlockSolverConfig description [BSPar.txt]"
	        << endl;
    /*!!
	   " UC file [BSC file seed wchg #rounds #chng %chng]"
 		<< endl <<
	   "       seed: random seed generator [0]"
 		<< endl <<
           "       wchg: what to change, coded bit-wise [127]"
		<< endl <<
           "             0 = ..., 1 = ...s "
		<< endl <<
           "             2 = ..., 3 = ..."
	        << endl <<
           "       #rounds: how many iterations [40]"
	        << endl <<
           "       #chng: number changes [10]"
	        << endl <<
           "       %chng: probability of changing [0.5]"
		!!*/
	   return( 1 );
  }

 // read the Block- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 TestBlock = Block::deserialize( argv[ 1 ] );
 if( ! TestBlock ) {
  cout << endl << "Block::deserialize() failed!" << endl;
  exit( 1 );
  }

 // attach the Solver(s) to the Block - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // do this by reading an appropriate BlockSolverConfig from file and
 // apply() it to the TestBlock; note that the BlockSolverConfig is
 // clear()-ed and kept to do the cleanup at the end

 BlockSolverConfig * bsc;
 {
  auto c = Configuration::deserialize( argc >= 3 ? argv[ 2 ] : "BSPar.txt" );
  bsc = dynamic_cast< BlockSolverConfig * >( c );
  if( ! bsc ) {
   cerr << "Error: configuration file not a BlockSolverConfig" << endl;
   delete c;
   exit( 1 );
   }

  #if USE_BundleSolver
   auto nbsc = bsc->num_ComputeConfig();
   if( ! nbsc ) {
    cerr << "Error: no ComputeConfig in the BlockSolverConfig" << endl;
    delete c;
    exit( 1 );
    }

   // if there are (at least) two Solver get the ComputeConfig of the 2nd,
   // otherwise that of the 1st (and only)
   auto cc = bsc->get_SolverConfig( nbsc > 1 ? 1 : 0 );
   if( ! cc ) {
    cerr << "Error: empty ComputeConfig in the BlockSolverConfig" << endl;
    delete c;
    exit( 1 );
    }

   // find if the ComputeConfig contains "intDoEasy", if so read it,
   // otherwise assume it is true (default)
   bool DoEasy = true;
   auto it = std::find_if( cc->int_pars.begin() , cc->int_pars.end() ,
			   []( auto & pair ) {
			    return( pair.first == "intDoEasy" );
			    } );
   if( it != cc->int_pars.end() )
    DoEasy = ( it->second & 1 ) > 0;

   // if easy components are used
   if( DoEasy ) {
    // define the vector of components to be excluded from being "easy",
    // i.e., all ThermalUnitBlock and possibly the HydroSystemUnitBlock
    std::vector< int > NoEasy;

    // load the BlockSolverConfig for ThermalUnitBlock
    auto ct = Configuration::deserialize( "TUBSCfg.txt" );
    auto tbsc = dynamic_cast< BlockSolverConfig * >( ct );
    if( ! tbsc ) {
     cerr << "Error: TUBSCfg.txt does not contain a BlockSolverConfig" << endl;
     delete c;
     delete ct;
     exit( 1 );
     }

    // load the BlockSolverConfig for HydroSystemUnitBlock; note that
    // this can be "empty", in which case the HydroSystemUnitBlock will
    // be treated as "easy"
    auto ch = Configuration::deserialize( "HSUBSCfg.txt" );
    auto hbsc = dynamic_cast< BlockSolverConfig * >( ch );
    if( ! hbsc ) {
     cerr << "Error: HSUBSCfg.txt does not contain a BlockSolverConfig" << endl;
     delete c;
     delete ct;
     delete ch;
     exit( 1 );
     }

    if( ! hbsc->num_ComputeConfig() ) {
     delete hbsc;
     hbsc = nullptr;
     }

    auto sb = TestBlock->get_nested_Blocks();
    for( int i = 0 ; i < sb.size() ; ++i ) {
     // deal with ThermalUnitBlock
     if( auto tub = dynamic_cast< ThermalUnitBlock * >( sb[ i ] ) ) {
      NoEasy.push_back( i );
      tbsc->apply( tub );
      continue;
      }

     // deal with HydroSystemUnitBlock
     if( auto hub = dynamic_cast< HydroSystemUnitBlock * >( sb[ i ] ) ) {
      // surely Configure it to use the "linearised" representation
      Configure_HSUB( hub );
      // if not considered an easy component, also BlockSolverConfig-ure it 
      if( hbsc ) {
       NoEasy.push_back( i );
       hbsc->apply( hub );
       }
      continue;
      }

     // all the rest will be treated as an "easy component"
     }

    // now add the vintNoEasy parameter to the ComputeConfig
    // we are assuming it's not there already: if it is, the new copy is
    // seen after the old one and therefore overrides it
    cc->vint_pars.push_back( std::make_pair( "vintNoEasy" ,
					     std::move( NoEasy ) ) );
    // cleanup
    delete hbsc;
    delete tbsc;

    }  // end( if( DoEasy ) )
   else
    // Configure all HydroSystemUnitBlock to use the "linearised" representation
    for(  auto sb : TestBlock->get_nested_Blocks() )
     if( auto hub = dynamic_cast< HydroSystemUnitBlock * >( sb ) )
      Configure_HSUB( hub );
  #endif

  bsc->apply( TestBlock );
  bsc->clear();

  if( TestBlock->get_registered_solvers().empty() ) {
   cout << endl << "no Solver registered to the Block!" << endl;
   exit( 1 );
   }
  }

 // open log-file - - - - - - - - - - -  - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 #if( LOG_LEVEL >= 2 )
  #if( LOG_ON_COUT )
   ((TestBlock->get_registered_solvers()).back())->set_log( &cout );
  #else
   ofstream LOGFile( logF , ofstream::out );
   if( ! LOGFile.is_open() )
    cerr << "Warning: cannot open log file """ << logF << """" << endl;
   else {
    LOGFile.setf( ios::scientific, ios::floatfield );
    LOGFile << setprecision( 10 );
    ((TestBlock->get_registered_solvers()).back())->set_log( &LOGFile );
    }
  #endif
 #endif

 // first solver call - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LOG1( "First call: " );

 bool AllPassed = SolveBoth();
 
 // main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // now, for n_repeat times:
 // - up to n_change ... are ...
 // - up to n_change ... are ...
 // - up to n_change ... are ...
 // - up to n_change ... are ...
 //
 // then the TestBlock is re-solved with both Solver

 /*!!
 for( Index rep = 0 ; rep < n_repeat * ( SKIP_BEAT + 1 ) ; ) {
  LOG1( rep << ": ");

  // do stuff 1 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 1 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = Index( dis( rg ) * n_change ) ) {
    LOG1( "... " << tochange << " ... - " );

    }

  // do stuff 2 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 2 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = min( m - 1 , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "... " << tochange << " ..." );

    
    if( dis( rg ) <= 0.5 ) {  // in 50% of the cases do a ranged change
     LOG1( "(r) - " );

     }
    else {  // in the other 50% of the cases, do a sparse change
     LOG1( "(s) - " );
     Subset nms( GenerateRand( m , tochange ) );

     }

    }

  // ...


  // if verbose, print out stuff- - - - - - - - - - - - - - - - - - - - - - -

  #if( LOG_LEVEL >= 3 )
   ((LPBlock->get_registered_solvers()).front())->set_par(
		                     MILPSolver::strOutputFile , "LPBlock-" +
		                     std::to_string( rep ) + ".lp" );
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
     !!*/

 if( AllPassed )
  cout << GREEN( All tests passed!! ) << endl;
 else
  cout << RED( Shit happened!! ) << endl;
 
 // destroy the Block - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // apply() the clear()-ed BlockSolverConfig to cleanup Solver
 bsc->apply( TestBlock );

 // then delete the BlockSolverConfig
 delete bsc;

 #if USE_BundleSolver
  // since some Solver have been attached "by hand" to some sub-Block,
  // unregister "by hand" any remaning Solver attached to them
  for( auto sb : TestBlock->get_nested_Blocks() )
   sb->unregister_Solvers();
 #endif

 // finally the AbstractBlock can be deleted
 delete TestBlock;

 // terminate - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( AllPassed ? 0 : 1 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*------------------------ End File test.cpp -------------------------------*/
/*--------------------------------------------------------------------------*/
