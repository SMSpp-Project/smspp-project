/*--------------------------------------------------------------------------*/
/*----------------------------- File test.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing different formulations of some problem.
 *
 * This main loads a Block twice. Then it generate_abstract_variables() them
 * with two different Configuration, assumed to produce two different
 * formulations of the same problem. Then it attaches two identical Solver
 * to the two copies of the Block (by using the same BlockSolverConfig),
 * solve both and compare the results.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ MACROS ------------------------------------*/
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
/*----------------------------- INCLUDES -----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BlockSolverConfig.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using namespace std;

/*--------------------------------------------------------------------------*/
/*------------------------------- TYPES ------------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*----------------------------- CONSTANTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

static constexpr double INF = SMSpp_di_unipi_it::Inf< double >();

/*--------------------------------------------------------------------------*/
/*------------------------------ GLOBALS -----------------------------------*/
/*--------------------------------------------------------------------------*/

Block * Block1;
Block * Block2;

/*--------------------------------------------------------------------------*/
/*----------------------------- FUNCTIONS ----------------------------------*/
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
  auto Slvr1 = Block1->get_registered_solvers().front();

  auto start = std::chrono::system_clock::now();

  int rtrn1st = Slvr1->compute( false );
  bool hs1st = ( ( rtrn1st >= Solver::kOK ) && ( rtrn1st < Solver::kError )
		 && ( rtrn1st != Solver::kUnbounded )
		 && ( rtrn1st != Solver::kInfeasible ) )
               || ( rtrn1st == Solver::kLowPrecision );
  double fo1st = hs1st ? Slvr1->get_var_value() : -INF;

  auto end = std::chrono::system_clock::now();
  std::chrono::duration< double > elapsed = end - start;
 
  cout.setf( ios::scientific, ios::floatfield );
  cout << setprecision( 2 ) << elapsed.count() << " - " << flush;

  // solve with the 2nd Solver- - - - - - - - - - - - - - - - - - - - - - - -
  auto Slvr2 = Block2->get_registered_solvers().front();

  start = std::chrono::system_clock::now();

  int rtrn2nd = Slvr2->compute( false );
  bool hs2nd = ( ( rtrn2nd >= Solver::kOK ) && ( rtrn2nd < Solver::kError )
		 && ( rtrn2nd != Solver::kUnbounded )
		 && ( rtrn2nd != Solver::kInfeasible ) )
               || ( rtrn2nd == Solver::kLowPrecision );
  double fo2nd = hs2nd ? Slvr2->get_var_value() : -INF;

  end = std::chrono::system_clock::now();
  elapsed = end - start;

  cout.setf( ios::scientific, ios::floatfield );
  cout << setprecision( 2 ) << elapsed.count();

  if( hs1st && hs2nd && ( abs( fo1st - fo2nd ) <= 2e-7 *
			  max( double( 1 ) , max( abs( fo1st ) ,
						  abs( fo2nd ) ) ) ) ) {
   cout << " - OK(f)" << endl;
   return( true );
   }

  if( ( rtrn1st == Solver::kInfeasible ) &&
      ( rtrn2nd == Solver::kInfeasible ) ) {
   cout << " - OK(e)" << endl;
   return( true );
   }

  if( ( rtrn1st == Solver::kUnbounded ) &&
      ( rtrn2nd == Solver::kUnbounded ) ) {
   cout << " - OK(u)" << endl;
   return( true );
   }
    
  cout << " - " << setprecision( 7 );
  PrintResults( hs1st , rtrn1st , fo1st );
  cout << " - ";
  PrintResults( hs2nd , rtrn2nd , fo2nd );
  cout << endl;

  return( false );
  }
 catch( exception &e ) {
  cerr << e.what() << endl;
  exit( 1 );
  }
 catch(...) {
  cerr << "error: unknown exception thrown" << endl;
  exit( 1 );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char **argv )
{
 // read command line parameters- - - - - - - - - - - - - - - - - - - - - - -

 if( argc < 2 ) {
  cerr << "Usage: " << argv[ 0 ]
       << " block_filename [cfg_1_filename cfg_1_filename]" << endl
       << "       default configurations Config1.txt and Config1.txt" << endl;
  return( 1 );  
  }

 // load both Block out of the same netCDF file- - - - - - - - - - - - - - - - 

 Block1 = Block::deserialize( argv[ 1 ] );
 if( ! Block1 ) {
  cerr << "error: cannot load Block from " << argv[ 1 ] << endl;
  return( 1 );
  }

 Block2 = Block::deserialize( argv[ 1 ] );
 // this reasonably should not fail ...

 // load two Configuration from file- - - - - - - - - - - - - - - - - - - - -

 auto cfg1 = Configuration::deserialize( argc >= 3 ? argv[ 2 ]
					           : "Config1.txt" );
 if( ! cfg1 ) {
  cerr << "error: cannot load Configuration1" << endl;
  return( 1 );
  }

 Block1->generate_abstract_variables( cfg1 );
 delete cfg1;
 
 auto cfg2 = Configuration::deserialize( argc >= 4 ? argv[ 3 ]
					           : "Config2.txt" );
 if( ! cfg2 ) {
  cerr << "error: cannot load Configuration2" << endl;
  return( 1 );
  }

 Block2->generate_abstract_variables( cfg2 );
 delete cfg2;

 // attach two identical Solver to both Block - - - - - - - - - - - - - - - -
 // do that via a BlockSolverConfig

 auto c = Configuration::deserialize( "BSCfg.txt" );
 auto bsc = dynamic_cast< BlockSolverConfig * >( c );
 if( ! bsc ) {
  cerr << "error: BSCfg.txt does not contain a BlockSolverConfig" << endl;
  exit( 1 );
  }

 bsc->apply( Block1 );

 if( Block1->get_registered_solvers().empty() ) {
  cerr << "Error: no Solver registered to Block1" << endl;
  exit( 1 );
  }

 bsc->apply( Block2 );
 // this reasonably should not fail ...

 bsc->clear();  
  
 // solve- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 auto ok = SolveBoth();
 if( ok )
  cout << GREEN( Test passed!! ) << endl;
 else
  cout << RED( Shit happened!! ) << endl;

 // clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

 bsc->apply( Block1 );
 delete( Block1 );

 bsc->apply( Block2 );
 delete( Block2 );

 delete( bsc );

 return( ok ? 0 : 1 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File test.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
