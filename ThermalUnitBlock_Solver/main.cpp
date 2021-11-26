/*--------------------------------------------------------------------------*/
/*-------------------------- File main.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing ThermalUnitDPSolver
 *
 * An ThermalUnitBlock instance is loaded from netCDF file, two different
 * Solver are registered to the ThermalUnitBlock, the second of which is
 * assumed to be a ThermalUnitDPSolver, the ThermalUnitBlock is solved by
 * the Solver and the results are compared. The ThermalUnitBlock is then
 * repeatedly randomly modified and re-solved several times, the results are
 * compared. 
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
// 2 = + print optimal solutions

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) cout << x
 #define CLOG1( y , x ) if( y ) cout << x
#else
 #define LOG1( x )
 #define CLOG1( y , x )
#endif

/*--------------------------------------------------------------------------*/
// if nonzero, the 1st Solver attched to the UCBlock is detached
// and re-attached to it at all iterations

#define DETACH_1ST 0

// if nonzero, the 2nd Solver attched to the UCBlock is detached and
// re-attached to it at all iterations

#define DETACH_2ND 0

/*--------------------------------------------------------------------------*/
// if nonzero, the Block is not solved at every round of changes, but only
// every SKIP_BEAT + 1 rounds. this allows changes to accumulate, and
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

#include "ThermalUnitBlock.h"

#include "BlockSolverConfig.h"

#include "FRealObjective.h"

#include "DQuadFunction.h"

// #include "LinearFunction.h"

// #include "OneVarConstraint.h"

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
// using c_FunctionValue = Function::c_FunctionValue;
// using Vec_FunctionValue = LinearFunction::Vec_FunctionValue;

// using RHSValue = RowConstraint::RHSValue;

// using coeff_pair = LinearFunction::coeff_pair;
// using v_coeff_pair = LinearFunction::v_coeff_pair;

// using coeff_triple = DQuadFunction::coeff_triple;
// using v_coeff_triple = DQuadFunction::v_coeff_triple;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

//SMSpp_ensure_load( ThermalUnitDPSolver );

static constexpr auto INF = Inf< FunctionValue >();

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

ThermalUnitBlock * TUBlock;  // the ThermalUnitBlock

Index time_horizon;          // the length of the time horizon

std::vector< double > a;     // the quadratic cost coefficients
std::vector< double > b;     // the linear cost coefficients
std::vector< double > c;     // the fixed cost coefficients
//std::vector< double > l;     // the lower bounds on power production
std::vector< double > u;     // the upper bounds on power production

std::mt19937 rg;             // base random generator
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

static void PrintSolution( void )
{
 cout.setf( std::ios::scientific , std::ios::floatfield );
 cout << setprecision( 4 );

 auto p = TUBlock->get_active_power( 0 );
 cout << endl << "p = [ ";
 for( Index i = 0 ; ; ++p ) {
  cout << p->get_value();
  if( ++i >= time_horizon )
   break;
  else
   cout << ", ";
  }
 cout << " ]";

 auto u = TUBlock->get_commitment( 0 );
 cout << endl << "u = [ ";
 for( Index i = 0 ; ; ++u ) {
  cout << u->get_value();
  if( ++i >= time_horizon )
   break;
  else
   cout << ", ";
  }
 cout << " ]";
 }

/*--------------------------------------------------------------------------*/

static double fixed_cost( Index i )
{
 // returns the original fixed cost multiplied by a factor uniformly
 // distributed in [ -4 , 4 ]

 return( c[ i ] * ( 8 * dis( rg ) - 4 ) );
 }

/*--------------------------------------------------------------------------*/

static double quadratic_cost( Index i )
{
 // returns the original quadratic cost multiplied by a factor "uniformly
 // distributed" in [ 0.1 , 10 ]

 return( a[ i ] * pow( 10 , 2 * dis( rg ) - 1 ) );
 }

/*--------------------------------------------------------------------------*/

static double linear_cost( Index i )
{
 /* Randomly setting the linear cost is nontrivial, since 1UC problems have
  * an unfortunate tendency for producing "all 0" solutions with their
  * original costs. This is because a > 0, b > 0 and c > 0, so producing
  * power has a positive cost and there is no gain counter-balanging it.
  *
  * Random fixed costs can be negative (see fixed_cost()) so this provides
  * an incentive to the unit to produce, but typically one should set b < 0
  * so that also power production is convenient (least the unit is started
  * but always kept at the minimum).
  *
  * Since a > 0, the largest possible quadratic cost is a u^2. To ensure
  * that producing energy is always more convenient than not producing
  * anything (p == 0 ==> cost == 0) one must have
  *
  *   a u^2 + b u < 0    ==>   b < - a u
  *
  * The random value of b is therefore set as follows:
  *
  * - in 10% of the cases is equal to the original linear cost multiplied
  *    by a factor "uniformly distributed" in [ 0.1 , 10 ] (hence positive)
  *
  * - in all the remaining cases is - a u  multiplied by a factor
  *   "uniformly distributed" in [ 4 , 1 / 4 ] (hence negative)
  *
  * Note, however, that a could have just changed prior to the call to
  * this function, so the current value in TUBlock is used rather than
  * the stored one. */

 auto ai = TUBlock->get_quad_term( i );

 return( dis( rg ) < 0.1 ? b[ i ] * pow( 10 , 2 * dis( rg ) - 1 )
	                 : - ai * u[ i ] * pow( 2 , 4 * dis( rg ) - 2 ) );
 }

/*--------------------------------------------------------------------------*/

static bool SolveBoth( void ) 
{
 try {
  // solve with the 1st Solver- - - - - - - - - - - - - - - - - - - - - - - -
  Solver * Slvr1 = TUBlock->get_registered_solvers().front();
  #if DETACH_1ST
   TUBlock->unregister_Solver( Slvr1 );
   TUBlock->register_Solver( Slvr1 , true );  // push it to the front
  #endif
  int rtrn1st = Slvr1->compute( false );
  bool hs1st = ( ( rtrn1st >= Solver::kOK ) && ( rtrn1st < Solver::kError ) )
               || ( rtrn1st == Solver::kLowPrecision );
  double fo1st = Slvr1->get_var_value();
  #if( LOG_LEVEL >= 1 )
  if( hs1st ) {
   if( ! Slvr1->has_var_solution() ) {
    cerr << "Error: Solver1 has not found any solution" << endl;
    exit( 1 );
    }
   Slvr1->get_var_solution();
   PrintSolution();
   }
  #endif

  // solve with the 2nd Solver- - - - - - - - - - - - - - - - - - - - - - - -
  Solver * Slvr2 = TUBlock->get_registered_solvers().back();
  #if DETACH_2ND
   TUBlock->unregister_Solver( Slvr2 );
   TUBlock->register_Solver( Slvr2 );  // push it to the back
  #endif
  int rtrn2nd = Slvr2->compute( false );

  bool hs2nd = ( ( rtrn2nd >= Solver::kOK ) && ( rtrn2nd < Solver::kError ) )
                 || ( rtrn2nd == Solver::kLowPrecision );
  double fo2nd = hs2nd ? Slvr2->get_var_value() : -INF;
  #if( LOG_LEVEL >= 1 )
  if( hs2st ) {
   if( ! Slvr2->has_var_solution() ) {
    cerr << "Error: Solver2 has not found any solution" << endl;
    exit( 1 );
    }
   Slvr2->get_var_solution();
   PrintSolution();
   }
  #endif

  if( hs1st && hs2nd && ( abs( fo1st - fo2nd ) <= 2e-7 *
			  max( double( 1 ) , max( abs( fo1st ) ,
						  abs( fo2nd ) ) ) ) ) {
   LOG1( "OK(f)" << endl );
   return( true );
   }

  if( ( rtrn1st == Solver::kInfeasible ) &&
      ( rtrn2nd == Solver::kInfeasible ) ) {
    LOG1( "OK(e)" << endl );
    return( true );
    }

  if( ( rtrn1st == Solver::kUnbounded ) &&
      ( rtrn2nd == Solver::kUnbounded ) ) {
   LOG1( "OK(u)" << endl );
   return( true );
   }

  #if( LOG_LEVEL >= 1 )
   cout << "S1 = ";
   PrintResults( hs1st , rtrn1st , fo1st );

   cout << " ~ S2 = ";
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

 long int seed = 0;
 Index wchg = 135;
 double p_change = 0.6;
 Index n_change = 10;
 Index n_repeat = 100;

 switch( argc ) {
  case( 6 ): Str2Sthg( argv[ 5 ] , p_change );
  case( 5 ): Str2Sthg( argv[ 4 ] , n_change );
  case( 4 ): Str2Sthg( argv[ 3 ] , n_repeat );
  case( 3 ): Str2Sthg( argv[ 2 ] , wchg );
  case( 2 ): Str2Sthg( argv[ 1 ] , seed );
             break;
  default: cerr << "Usage: " << argv[ 0 ] <<
	   " seed [wchg nvar #rounds #chng %chng]"
 		<< endl <<
           "       wchg: what to change, coded bit-wise [135]"
		<< endl <<
           "             0 = fixed costs, 1 = linear costs "
		<< endl <<
           "             2 = quadratic costs "
		<< endl <<
 	   "             +128 = also change abstract representation"
	        << endl <<
           "       #rounds: how many iterations [100]"
	        << endl <<
           "       #chng: number changes [10]"
	        << endl <<
           "       %chng: probability of changing [0.6]"
	        << endl;
	   return( 1 );
  }

 rg.seed( seed );  // seed the pseudo-random number generator

 // read the Block- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  auto b = Block::deserialize( argv[ 1 ] );
  if( ! b ) {
   cout << endl << "Block::deserialize() failed!" << endl;
   exit( 1 );
   }
  TUBlock = dynamic_cast< ThermalUnitBlock * >( b );
  if( ! TUBlock ) {
   cout << endl << "The deserialized Block is not a ThermalUnitBlock" << endl;
   exit( 1 );
   }
  }

 TUBlock->generate_abstract_variables();
 
 // save some original data of the ThermalUnitBlock - - - - - - - - - - - - -

 time_horizon = TUBlock->get_time_horizon();
 a.resize( time_horizon );
 b.resize( time_horizon );
 c.resize( time_horizon );
 // l.resize( time_horizon );
 u.resize( time_horizon );
 for( Index i = 0 ; i < time_horizon ; ++i ) {
  a[ i ] = TUBlock->get_quad_term( i );
  b[ i ] = TUBlock->get_linear_term( i );
  c[ i ] = TUBlock->get_const_term( i );
  // l[ i ] = TUBlock->get_operational_min_power( i );
  u[ i ] = TUBlock->get_operational_max_power( i );
  }
 
 // attach the Solver(s) to the ThermalUnitBlock- - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // do this by reading an appropriate BlockSolverConfig from file and
 // apply() it to the BoxBlock; note that the BlockSolverConfig is
 // clear()-ed and kept to do the cleanup at the end

 BlockSolverConfig * bsc;
 {
  auto c = Configuration::deserialize( "BSCfg.txt" );
  bsc = dynamic_cast< BlockSolverConfig * >( c );
  if( ! bsc ) {
   cerr << "Error: configuration file not a BlockSolverConfig" << endl;
   delete c;
   exit( 1 );
   }

  bsc->apply( TUBlock );
  bsc->clear();

  if( TUBlock->get_registered_solvers().size() < 2 ) {
   cout << endl << "too few Solver registered to the Block" << endl;
   exit( 1 );
   }
  }

 // first solver call - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LOG1( "First call: " );

 bool AllPassed = SolveBoth();
 
 // main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // now, for n_repeat times:
 // - up to n_change constant terms are changed
 // - up to n_change linear terms are changed
 // - up to n_change quadratic terms are changed
 //
 // then the two Solver are called to re-solve the BoxBlock

 for( Index rep = 0 ; rep < n_repeat * ( SKIP_BEAT + 1 ) ; ) {
  LOG1( rep << ": ");

  DQuadFunction * of;
  {
   auto obj = TUBlock->get_objective();
   assert( obj );
   auto fro = dynamic_cast< FRealObjective * >( obj );
   assert( fro );
   of = dynamic_cast< DQuadFunction * >( fro->get_function() );
   assert( of );
   }

  // change fixed costs - - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( ( wchg & 1 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = min( time_horizon , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "changed " << tochange << " fixed costs" );

    std::vector< double >newcsts( tochange );

    // in 50% of the cases do a ranged change, in the others a sparse change
    if( dis( rg ) <= 0.5 ) {
     Index strt = dis( rg ) * ( time_horizon - tochange );
     Index stp = strt + tochange;

     for( Index i = 0 ; i < tochange ; ++i )
      newcsts[ i ] = fixed_cost( strt + i );

     if( ( wchg & 128 ) && ( dis( rg ) < 0.5 ) ) {
      // change via abstract representation
      // note that while this is a range of fixed costs, but the
      // corresponding variables are "scattered around" the objective
      // and therefore it becomes a Subset
      LOG1( "(r,a) - " );

      Subset nms( tochange );
      for( Index i = 0 ; i < tochange ; ++i )
       nms[ i ] = of->is_active( TUBlock->get_commitment( 0 ) + ( strt + i ) );
     
      of->modify_linear_coefficients( std::move( newcsts ) ,
				      std::move( nms ) , false );
      }
     else {  // change via call to set_* method
      LOG1( "(r) - " );
      TUBlock->set_const_term( newcsts.begin() , Range( strt , stp ) );
      }
     }
    else {
     Subset nms( GenerateRand( time_horizon , tochange ) );

     for( Index i = 0 ; i < tochange ; ++i )
      newcsts[ i ] = fixed_cost( nms[ i ] );

     if( ( wchg & 128 ) && ( dis( rg ) < 0.5 ) ) {
      // change via abstract representation
      LOG1( "(s,a) - " );

      for( Index i = 0 ; i < tochange ; ++i )
       nms[ i ] = of->is_active( TUBlock->get_commitment( 0 ) + nms[ i ] );
     
      of->modify_linear_coefficients( std::move( newcsts ) ,
				      std::move( nms ) , false );
      }
     else {  // change via call to set_* method
      LOG1( "(s) - " );
      TUBlock->set_const_term( newcsts.begin() , std::move( nms ) , true );
      }
     }
    }

  // change quadratic coefficients- - - - - - - - - - - - - - - - - - - - - -
  if( ( wchg & 2 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = min( time_horizon , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "changed " << tochange << " quadratic coeffs" );

    std::vector< double > newcsts( tochange );

    // in 50% of the cases do a ranged change, in the others a sparse change
    if( dis( rg ) <= 0.5 ) {
     Index strt = dis( rg ) * ( time_horizon - tochange );
     Index stp = strt + tochange;

     for( Index i = 0 ; i < tochange ; ++i )
      newcsts[ i ] = quadratic_cost( strt + i );

     if( ( wchg & 128 ) && ( dis( rg ) < 0.5 ) ) {
      // change via abstract representation
      // note that while this is a range of fixed costs, but the
      // corresponding variables are "scattered around" the objective
      // and therefore it becomes a Subset
      LOG1( "(r,a) - " );

      std::vector< double > lincsts( tochange );
      for( Index i = 0 ; i < tochange ; ++i )
       lincsts[ i ] = TUBlock->get_linear_term( strt + i );

      Subset nms( tochange );
      for( Index i = 0 ; i < tochange ; ++i )
       nms[ i ] = of->is_active( TUBlock->get_active_power( 0 )
				 + ( strt + i ) );
     
      of->modify_terms( newcsts.begin() , lincsts.begin() ,
			std::move( nms ) , false );
      }
     else {  // change via call to set_* method
      LOG1( "(r) - " );
      TUBlock->set_quad_term( newcsts.begin() , Range( strt , stp ) );
      }
     }
    else {
     Subset nms( GenerateRand( time_horizon , tochange ) );

     for( Index i = 0 ; i < tochange ; ++i )
      newcsts[ i ] = linear_cost( nms[ i ] );

     if( ( wchg & 128 ) && ( dis( rg ) < 0.5 ) ) {
      // change via abstract representation
      LOG1( "(s,a) - " );

      std::vector< double > lincsts( tochange );
      for( Index i = 0 ; i < tochange ; ++i )
       lincsts[ i ] = TUBlock->get_linear_term( nms[ i ] );

      for( Index i = 0 ; i < tochange ; ++i )
       nms[ i ] = of->is_active( TUBlock->get_active_power( 0 ) + nms[ i ] );

      of->modify_terms( newcsts.begin() , lincsts.begin() ,
			std::move( nms ) , false );
      }
     else {  // change via call to set_* method
      LOG1( "(s) - " );
      TUBlock->set_quad_term( newcsts.begin() , std::move( nms ) , false );
      }
     }
    }

  // change linear coefficients - - - - - - - - - - - - - - - - - - - - - - -
  if( ( wchg & 4 ) && ( dis( rg ) <= p_change ) )
   if( Index tochange = min( time_horizon , Index( dis( rg ) * n_change ) ) ) {
    LOG1( "changed " << tochange << " linear coeffs" );

    std::vector< double > newcsts( tochange );

    // in 50% of the cases do a ranged change, in the others a sparse change
    if( dis( rg ) <= 0.5 ) {
     Index strt = dis( rg ) * ( time_horizon - tochange );
     Index stp = strt + tochange;

     for( Index i = 0 ; i < tochange ; ++i )
      newcsts[ i ] = linear_cost( strt + i );

     if( ( wchg & 128 ) && ( dis( rg ) < 0.5 ) ) {
      // change via abstract representation
      // note that while this is a range of fixed costs, but the
      // corresponding variables are "scattered around" the objective
      // and therefore it becomes a Subset
      LOG1( "(r,a) - " );

      Subset nms( tochange );
      for( Index i = 0 ; i < tochange ; ++i )
       nms[ i ] = of->is_active( TUBlock->get_active_power( 0 )
				 + ( strt + i ) );
     
      of->modify_linear_coefficients( std::move( newcsts ) ,
				      std::move( nms ) , true );
      }
     else {  // change via call to set_* method
      LOG1( "(r) - " );
      TUBlock->set_linear_term( newcsts.begin() , Range( strt , stp ) );
      }
     }
    else {
     Subset nms( GenerateRand( time_horizon , tochange ) );

     for( Index i = 0 ; i < tochange ; ++i )
      newcsts[ i ] = linear_cost( nms[ i ] );

     if( ( wchg & 128 ) && ( dis( rg ) < 0.5 ) ) {
      // change via abstract representation
      LOG1( "(s,a) - " );

      Subset nms( tochange );
      for( Index i = 0 ; i < tochange ; ++i )
       nms[ i ] = of->is_active( TUBlock->get_active_power( 0 ) + nms[ i ] );
     
      of->modify_linear_coefficients( std::move( newcsts ) ,
				      std::move( nms ) , true );
      }
     else {  // change via call to set_* method
      LOG1( "(s) - " );
      TUBlock->set_linear_term( newcsts.begin() , std::move( nms ) , false );
      }
     }
    }

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

 // apply() the clear()-ed BlockSolverConfig to cleanup Solver
 bsc->apply( TUBlock );

 // then delete the BlockSolverConfig
 delete bsc;

 // delete the Block
 delete TUBlock;

 // terminate - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( AllPassed ? 0 : 1 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*------------------------ End File main.cpp -------------------------------*/
/*--------------------------------------------------------------------------*/
