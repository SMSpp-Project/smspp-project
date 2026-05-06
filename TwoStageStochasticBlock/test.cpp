/*--------------------------------------------------------------------------*/
/*-------------------------- File test.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing TwoStageStochasticBlock.
 *
 * A TwoStageStochasticBlock instance is loaded from a netCDF file, a
 * BlockSolverConfig is applied that registers one or two Solver to it, the
 * Block is solved and the results are compared either between the two
 * Solver (when two are registered) or against a reference objective value
 * passed on the command line.
 *
 * The CLI mirrors that of tests/LagrangianDualSolver_UC/test.cpp, except
 * that for TwoStageStochasticBlock the second Solver is always a
 * LagrangianDualSolver (there is no PrimalProximalHeur counterpart yet);
 * the third positional argument is therefore kept as a placeholder for
 * symmetry with the LDS_UC batch invocation.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli, Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- MACROS ----------------------------------*/
/*--------------------------------------------------------------------------*/

#define LOG_LEVEL 2
// -1 = no log at all, not even pass/fail
// 0 = only pass/fail
// 1 = result of each test
// 2 = + solver log

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) std::cout << x
 #define CLOG1( y , x ) if( y ) std::cout << x

 #if( LOG_LEVEL >= 2 )
  #define LOG_ON_COUT 1
 #endif
#else
 #define LOG1( x )
 #define CLOG1( y , x )
#endif

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

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "BlockSolverConfig.h"

#include "TwoStageStochasticBlock.h"

#include "UCBlock.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using FunctionValue = Function::FunctionValue;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

const char * const logF = "log.txt";

const FunctionValue INF = SMSpp_di_unipi_it::Inf< FunctionValue >();

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

Block * TestBlock;  // the TwoStageStochasticBlock that is solved

// if not-NaN, the objective value of the (1st) Solver attached to the Block
// is compared against a reference value passed on the command line

double RefObjective = std::numeric_limits< double >::quiet_NaN();

// relative tolerance used to compare the solved objective value to
// RefObjective; the EnergyCommunity.jl@stochastic reference values come
// from a slightly different numerical pipeline so this is looser than
// the solver-vs-solver tolerance below (which keeps using 1e-5).

const double RefTolerance = 1e-2;

bool ProxHeur = false;     // false = LagrangianDualSolver
                           // true  = PrimalProximalHeur (currently unused;
                           //          kept for CLI symmetry with LDS_UC)

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
static void Str2Sthg( const char * const str , T & sthg ) {
 std::istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/

static inline std::ostream & def( std::ostream & os )
{
 os.setf( std::ios::scientific , std::ios::floatfield );
 os << std::setprecision( 7 );
 return( os );
 }

/*--------------------------------------------------------------------------*/

static inline std::ostream & fixd( std::ostream & os )
{
 os.setf( std::ios::fixed , std::ios::floatfield );
 os << std::setprecision( 4 );
 return( os );
 }

/*--------------------------------------------------------------------------*/

static void PrintResults( bool hs , int rtrn , double fo )
{
 if( hs ) {
  std::cout.setf( std::ios::scientific , std::ios::floatfield );
  std::cout << def << fo;
  }
 else
  if( rtrn == Solver::kInfeasible )
   std::cout << "    Unfeas";
  else
   if( rtrn == Solver::kUnbounded )
    std::cout << "      Unbounded";
   else
    std::cout << "      Error!";
 }

/*--------------------------------------------------------------------------*/

static bool SolveBoth( double * out_fo1st = nullptr ,
                       bool   * out_hs1st = nullptr
#if( LOG_LEVEL >= 1 )
                     , double * out_time1 = nullptr
#endif
                     )
{
 try {
  // solve with the 1st Solver- - - - - - - - - - - - - - - - - - - - - - - -
  #if( LOG_LEVEL >= 1 )
   auto start = std::chrono::system_clock::now();
  #endif
  Solver * Slvr1 = TestBlock->get_registered_solvers().front();
  int rtrn1st = Slvr1->compute( false );
  #if( LOG_LEVEL >= 1 )
   auto end = std::chrono::system_clock::now();
   std::chrono::duration< double > elapsed = end - start;
   auto time1 = elapsed.count();
  #endif
  bool hs1st = ( ( ( rtrn1st >= Solver::kOK ) && ( rtrn1st < Solver::kError )
                   && ( rtrn1st != Solver::kUnbounded )
                   && ( rtrn1st != Solver::kInfeasible ) )
                 || ( rtrn1st == Solver::kLowPrecision ) );

  double fo1st = hs1st ? Slvr1->get_lb() : -INF;

  if( out_fo1st ) *out_fo1st = fo1st;
  if( out_hs1st ) *out_hs1st = hs1st;
  #if( LOG_LEVEL >= 1 )
   if( out_time1 ) *out_time1 = time1;
  #endif

  if( TestBlock->get_registered_solvers().size() == 1 ) {
   #if( LOG_LEVEL >= 1 )
    std::cout << fixd << time1 << "\t";
    PrintResults( hs1st , rtrn1st , fo1st );
    std::cout << std::endl;
   #endif
   return( hs1st );
   }

  // solve with the 2nd Solver- - - - - - - - - - - - - - - - - - - - - - - -
  #if( LOG_LEVEL >= 1 )
   start = std::chrono::system_clock::now();
  #endif
  Solver * Slvr2 = TestBlock->get_registered_solvers().back();
  int rtrn2nd = Slvr2->compute( false );
  #if( LOG_LEVEL >= 1 )
   end = std::chrono::system_clock::now();
   elapsed = end - start;
   auto time2 = elapsed.count();
   std::cout << fixd << time1 << " - " << time2 << " - ";
  #endif

  bool hs2nd = ( ( ( rtrn2nd >= Solver::kOK ) && ( rtrn2nd < Solver::kError )
                   && ( rtrn2nd != Solver::kUnbounded )
                   && ( rtrn2nd != Solver::kInfeasible ) )
                 || ( rtrn2nd == Solver::kLowPrecision ) );
  double fo2nd = -INF;

  if( hs1st && hs2nd ) {
   bool OK;
   if( ProxHeur ) {
    fo2nd = Slvr2->get_ub();
    OK = ( fo1st - fo2nd <= 1e-5 );
    }
   else {
    fo2nd = Slvr2->get_lb();
    OK = ( std::abs( fo1st - fo2nd ) <=
           1e-5 * std::max( double( 1 ) , std::max( std::abs( fo1st ) ,
                                                     std::abs( fo2nd ) ) ) );
    }
   if( OK ) {
    LOG1( "OK(f)" << std::endl );
    return( true );
    }
   }

  if( ( rtrn1st == Solver::kInfeasible ) &&
      ( rtrn2nd == Solver::kInfeasible ) ) {
   LOG1( "OK(e)" << std::endl );
   return( true );
   }

  if( ( rtrn1st == Solver::kUnbounded ) &&
      ( rtrn2nd == Solver::kUnbounded ) ) {
   LOG1( "OK(u)" << std::endl );
   return( true );
   }

  #if( LOG_LEVEL >= 1 )
   std::cout << "Solver1 = ";
   PrintResults( hs1st , rtrn1st , fo1st );
   std::cout << " ~ Solver2 = ";
   PrintResults( hs2nd , rtrn2nd , fo2nd );
   std::cout << std::endl;
  #endif

  return( false );
  }
 catch( std::exception & e ) {
  std::cerr << e.what() << std::endl;
  exit( 1 );
  }
 catch( ... ) {
  std::cerr << "Error: unknown exception thrown" << std::endl;
  exit( 1 );
  }
 }

/*--------------------------------------------------------------------------*/

static bool SolveAndCheckRef( double ref , double rel_tol )
{
 try {
  #if( LOG_LEVEL >= 1 )
   auto start = std::chrono::system_clock::now();
  #endif
  Solver * Slvr1 = TestBlock->get_registered_solvers().front();
  int rtrn1st = Slvr1->compute( false );
  #if( LOG_LEVEL >= 1 )
   auto end = std::chrono::system_clock::now();
   std::chrono::duration< double > elapsed = end - start;
   auto time1 = elapsed.count();
  #endif
  bool hs1st = ( ( ( rtrn1st >= Solver::kOK ) && ( rtrn1st < Solver::kError )
                   && ( rtrn1st != Solver::kUnbounded )
                   && ( rtrn1st != Solver::kInfeasible ) )
                 || ( rtrn1st == Solver::kLowPrecision ) );

  if( ! hs1st ) {
   #if( LOG_LEVEL >= 1 )
    std::cout << "Solver returned code " << rtrn1st << std::endl;
   #endif
   return( false );
   }

  double fo1st = Slvr1->get_lb();

  double maxv = std::max( double( 1 ) ,
                          std::max( std::abs( fo1st ) , std::abs( ref ) ) );
  double diff = std::abs( fo1st - ref );
  double tol = rel_tol * maxv;

  bool OK = ( diff <= tol );

  #if( LOG_LEVEL >= 1 )
   std::cout << fixd << time1 << "\t";
   PrintResults( hs1st , rtrn1st , fo1st );
   std::cout << " ~ Ref = " << def << ref
             << " (|diff| = " << def << diff
             << ", |rel| = " << def << ( diff / maxv )
             << ( OK ? ", OK" : ", KO" ) << ")" << std::endl;
  #endif

  return( OK );
  }
 catch( std::exception & e ) {
  std::cerr << e.what() << std::endl;
  exit( 1 );
  }
 catch( ... ) {
  std::cerr << "Error: unknown exception thrown" << std::endl;
  exit( 1 );
  }
 }

/*--------------------------------------------------------------------------*/

static bool CheckRefValue( double fo , double ref , double rel_tol
#if( LOG_LEVEL >= 1 )
                         , double time1
#endif
                         )
{
 double maxv = std::max( double( 1 ) ,
                         std::max( std::abs( fo ) , std::abs( ref ) ) );
 double diff = std::abs( fo - ref );
 double tol = rel_tol * maxv;

 bool OK = ( diff <= tol );

 #if( LOG_LEVEL >= 1 )
  std::cout << fixd << time1 << "\t";
  std::cout.setf( std::ios::scientific , std::ios::floatfield );
  std::cout << def << fo;
  std::cout << " ~ Ref = " << def << ref
            << " (|diff| = " << def << diff
            << ", |rel| = " << def << ( diff / maxv )
            << ( OK ? ", OK" : ", KO" ) << ")" << std::endl;
 #endif

 return( OK );
 }

/*--------------------------------------------------------------------------*/
/// Custom terminate function to print the exception message

void smspp_terminate( void )
{
 std::cerr << "Uncaught exception in executing SMS++:\n";
 try {
  std::rethrow_exception( std::current_exception() );
  }
 catch( const std::exception & e ) {
  std::cerr << "\tException type: " << typeid( e ).name() << "\n";
  std::cerr << "\tException message: " << e.what() << "\n";
  }
 catch( ... ) {
  std::cerr << "\tUnknown exception" << std::endl;
  }
 std::abort();
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 std::set_terminate( smspp_terminate );

 // reading command line parameters - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 switch( argc ) {
  case( 5 ): Str2Sthg( argv[ 4 ] , RefObjective );
  case( 4 ): Str2Sthg( argv[ 3 ] , ProxHeur );
  case( 3 ):
  case( 2 ): break;
  default:
   std::cerr << "Usage: " << argv[ 0 ]
             << " TSSB-file [BSC-file ws ref]" << std::endl
             << "       BSC-file: BlockSolverConfig description [BSPar.txt]"
             << std::endl
             << "       ws:  0 = LagrangianDualSolver, 1 = PrimalProximalHeur"
                " [0]" << std::endl
             << "       ref: reference objective value to compare against "
                "[none]" << std::endl;
   return( 1 );
  }

 // read the Block- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 TestBlock = Block::deserialize( argv[ 1 ] );
 if( ! TestBlock ) {
  std::cout << std::endl << "Block::deserialize() failed!" << std::endl;
  exit( 1 );
  }

 if( ! dynamic_cast< TwoStageStochasticBlock * >( TestBlock ) ) {
  std::cout << std::endl
            << "Error: deserialized Block is not a TwoStageStochasticBlock"
            << std::endl;
  delete TestBlock;
  exit( 1 );
  }

 // attach the Solver(s) to the Block - - - - - - - - - - - - - - - - - - - -

 BlockSolverConfig * bsc;
 {
  auto c = Configuration::deserialize( argc >= 3 ? argv[ 2 ] : "BSPar.txt" );
  bsc = dynamic_cast< BlockSolverConfig * >( c );
  if( ! bsc ) {
   std::cerr << "Error: configuration file not a BlockSolverConfig"
             << std::endl;
   delete( c );
   delete TestBlock;
   exit( 1 );
   }

  bsc->apply( TestBlock );
  bsc->clear();

  if( TestBlock->get_registered_solvers().empty() ) {
   std::cout << std::endl
             << "no Solver registered to the Block!" << std::endl;
   delete bsc;
   delete TestBlock;
   exit( 1 );
   }
  }

 // open log-file - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 #if( LOG_LEVEL >= 2 )
  #if( LOG_ON_COUT )
   ( ( TestBlock->get_registered_solvers() ).back() )->set_log( &std::cout );
  #else
   std::ofstream LOGFile( logF , std::ofstream::out );
   if( ! LOGFile.is_open() )
    std::cerr << "Warning: cannot open log file " << logF << std::endl;
   else {
    LOGFile.setf( std::ios::scientific , std::ios::floatfield );
    LOGFile << std::setprecision( 10 );
    ( ( TestBlock->get_registered_solvers() ).back() )->set_log( &LOGFile );
    }
  #endif
 #endif

 // first solver call - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LOG1( "First call: " );

 bool AllPassed;

 if( TestBlock->get_registered_solvers().size() > 1 ) {
  double fo1st = -INF;
  bool hs1st = false;
  #if( LOG_LEVEL >= 1 )
   double time1 = 0.0;
   AllPassed = SolveBoth( &fo1st , &hs1st , &time1 );
  #else
   AllPassed = SolveBoth( &fo1st , &hs1st );
  #endif

  // optional additional check against reference objective
  if( ! std::isnan( RefObjective ) ) {
   if( hs1st ) {
    #if( LOG_LEVEL >= 1 )
     AllPassed &= CheckRefValue( fo1st , RefObjective , RefTolerance , time1 );
    #else
     double maxv = std::max( double( 1 ) ,
                             std::max( std::abs( fo1st ) ,
                                       std::abs( RefObjective ) ) );
     AllPassed &= ( std::abs( fo1st - RefObjective ) <= RefTolerance * maxv );
    #endif
    }
   else
    AllPassed = false;
   }
  }
 else {
  // single-solver case
  if( std::isnan( RefObjective ) )
   AllPassed = SolveBoth();
  else
   AllPassed = SolveAndCheckRef( RefObjective , RefTolerance );
  }

 #if( LOG_LEVEL >= 0 )
  if( ! std::isnan( RefObjective ) ||
      ( TestBlock->get_registered_solvers().size() > 1 ) ) {
   if( AllPassed )
    std::cout << GREEN( All tests passed!! ) << std::endl;
   else
    std::cout << RED( Shit happened!! ) << std::endl;
   }
 #endif

 // destroy the Block - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 delete( bsc );
 delete( TestBlock );

 return( AllPassed ? 0 : 1 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File test.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
