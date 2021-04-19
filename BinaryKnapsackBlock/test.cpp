/*--------------------------------------------------------------------------*/
/*--------------------------- File test.cpp --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing BinaryKnapsackBlock, comparing the results of two 
 * different Solvers attached to it */

/*--------------------------------------------------------------------------*/
/*------------------------------ MACROS ------------------------------------*/
/*--------------------------------------------------------------------------*/

 
#define STEP 3  // after modifications solve again at each multiple of step

#define LOG_LEVEL 0
// 0 = only pass/fail
// 1 = list of modifications


/*--------------------------------------------------------------------------*/
/*----------------------------- INCLUDES -----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BinaryKnapsackBlock.h"

#include "BlockSolverConfig.h"

#include <random>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using namespace std;

/*--------------------------------------------------------------------------*/
/*------------------------------- TYPES ------------------------------------*/
/*--------------------------------------------------------------------------*/

using Index = Block::Index;
using c_Index = Block::c_Index;

using Range = Block::Range;
using c_Range = Block::c_Range;

using Subset = Block::Subset;
using c_Subset = Block::c_Subset;

/*--------------------------------------------------------------------------*/
/*----------------------------- CONSTANTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ GLOBALS -----------------------------------*/
/*--------------------------------------------------------------------------*/

BinaryKnapsackBlock * BKB;          // The Binary Knapsack Block

Solver * Solver1;                   // Solver1

Solver * Solver2;                   // Solver2
    
std::mt19937 rg;                    // random generator

std::uniform_real_distribution<> dis( 0.0 , 1.0 );

int N;                              // number of items

int rangeW = 100;                   // range values of weights

double rangeP = 100;                // range values of profits

/*--------------------------------------------------------------------------*/
/*----------------------------- FUNCTIONS ----------------------------------*/
/*--------------------------------------------------------------------------*/

template<class T>
static void Str2Sthg( const char* const str , T &sthg ){
 istringstream( str ) >> sthg;
}


/*--------------------------------------------------------------------------*/
// Generate a random Range of size m < N

Range generateRange( int m ){
 Range rng;
 rng.first = dis( rg ) * ( N - m );
 rng.second = rng.first + m;
 return rng;
}

/*--------------------------------------------------------------------------*/
// Generate a random Subset of size m < N

Subset generateSubset( int m ){
 Subset nms;
 
 Subset idx( N );            
 iota( idx.begin() , idx.end() , 0 );
 
 sample( idx.begin() , idx.end() , back_inserter( nms ) , m , rg );
 
return move( nms );
}

/*--------------------------------------------------------------------------*/

bool SolveBoth(){ 

  #if( LOG_LEVEL > 0 )
   cout << endl;
  #endif

 // Solve with both Solvers - - - - - - - - - - - - - - - - - - - - - - - -

 auto status1 = Solver1->compute();

 auto status2 = Solver2->compute();

 // check status- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  

 if( status1 == Solver::kInfeasible && status2 == Solver::kInfeasible )
  return( true );

 if( status1 == Solver::kInfeasible || status2 == Solver::kInfeasible )
  return( false );

 // get optimal values- - - - - - - - - - - - - - - - - - - - - - - - - - -

 double Value1 = Solver1->get_var_value();

 double Value2 = Solver2->get_var_value();  

 // get solutions - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Solver1->get_var_solution();
 
 double checksol = 0;
 for( int i = 0 ; i < N ; i++ )
  checksol += BKB->get_x( i ) * BKB->get_Profit( i );
 
 if( abs( checksol - Value1 ) > 1e-06 )
  cerr << "Error computing solution Solver1\n";  

 Solver2->get_var_solution();

 checksol = 0;
 for( int i = 0 ; i < N ; i++ )
  checksol += BKB->get_x( i ) * BKB->get_Profit( i );
 
 if( abs( checksol - Value2 ) > 1e-06 )
  cerr << "Error computing solution Solver2\n";  

 // compare optimal values- - - - - - - - - - - - - - - - - - - - - - - - - 
 
 double gap = ( Value2 - Value1 ) / max( abs( Value1 ) , 1.0 ) ;

 if( abs( gap ) < 2e-06 )
  return( true );
 
 return( false );     
} 


int main( int argc , char **argv ){ 

 // reading command line parameters - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 long int seed = 123123;                // seed
 Index wchg = 63;                       // what to change, coded bit-wise
 N = 100;                               // number of items
 int n_repeat = 100;                    // number of repetitions
 double delta = 0.001;                  // capacity parameter
 double nW = 0.1;
 double nP = 0.1;

 switch( argc ) {
  case( 8 ): Str2Sthg( argv[ 7 ] , nP );
  case( 7 ): Str2Sthg( argv[ 6 ] , nW );
  case( 6 ): Str2Sthg( argv[ 5 ] , delta );
  case( 5 ): Str2Sthg( argv[ 4 ] , n_repeat );
  case( 4 ): Str2Sthg( argv[ 3 ] , N );
  case( 3 ): Str2Sthg( argv[ 2 ] , wchg );
  case( 2 ): Str2Sthg( argv[ 1 ] , seed );
             break;
  default: cerr << "Usage: " << argv[ 0 ] <<
     " seed [wchg N n_repeat nW nP]"
    << endl <<
           "       wchg: what to change, coded bit-wise [63]"
    << endl <<
           "             0 = change sense, 1 = change capacity "
    << endl <<
           "             2 = change profits, 3 = change weights"
    << endl <<
           "             4 = fix x , 5 = unfix x"
    << endl <<

           "       N: number of variables [100]"
          << endl <<
           "       n_repeat: number of repetitions [100]"
          << endl <<
           "       delta: Capacity parameter [0.001]"
          << endl <<
           "       nW: percentage of negative weights [0.1]"
          << endl <<
           "       nP: percentage of negative profits [0.1]"
          << endl; 
     return( 1 );
  }

 // check - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 
 if( delta < 0 || delta > 1 ){
  cerr << "error: delta must be in [ 0 , 1 ]" << endl;
  exit( 1 ); 
 }

 if( nW < 0 || nW > 1 ){
  cerr << "error: nW must be in [ 0 , 1 ]" << endl;
  exit( 1 );
 }

 if( nP < 0 || nP > 1 ){
  cerr << "error: nP must be in [ 0 , 1 ]" << endl;
  exit( 1 );
 }

 int minW =  - int( nW * rangeW );
 int maxW = minW + rangeW;

 double minP = - nP * rangeP;
 double maxP = minP + rangeP;

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // Seed the pseudo-random number generator     
 rg.seed( seed );

 // print - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 #if( LOG_LEVEL > 0 )
 cout << "Seed " << seed << endl;
 cout << "N " << N << endl;
 cout << "n_repeat " << n_repeat << endl;
 cout << "delta " << delta << endl;
 cout << "minW " << minW << "\tmaxW " << maxW << endl;
 cout << "minP " << minP << "\tmaxP " << maxP << endl;

 cout << "\nModifications: " << endl;
 if( wchg & 1 )                   
  cout << " - Objective Sense\n";
 if( wchg & 2 )
  cout << " - Capacity\n"; 
 if( wchg & 4 )                   
  cout << " - Profits\n";
 if( wchg & 8 )
  cout << " - Weights\n"; 
 if( wchg & 16 )                   
  cout << " - Fix x\n";
 if( wchg & 32 )
  cout << " - Unfix x\n";
 cout << endl;  
 #endif
 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // Create the BinaryKnapsackBlock- - - - - - - - - - - - - - - - - - - - - -

 BKB = new BinaryKnapsackBlock();            

 // generate instance - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

 // generate weights from a uniform int distribution
 uniform_int_distribution<> dist_W( minW , maxW );

 // generate profits from a uniform real distribution
 uniform_real_distribution<> dist_P( minP , maxP );

 vector< double > W( N );             // vector of weights
 vector< double > P( N );             // vector of profits
 double C;                            // Capacity of the Knapsack

 int totWp = 0;                       // total sum of the positive weights
 int totWn = 0;                       // total sum of the negative weights
  
 for( int i = 0 ; i < N ; i++ ){
  W[ i ] = dist_W( rg );      
  P[ i ] = dist_P( rg );   

 if( W[ i ] > 0 )                     // update totWn and totWp
  totWp += W[ i ];      
 else
  totWn += W[ i ];      

 }
 
 // generate the Capacity from a uniform real distribution
 uniform_real_distribution<> dist_C( totWn , 
                                     totWn + delta * ( totWp - totWn ) );

 C = dist_C( rg );

 // load the Binary Knapsack instance- - - - - - - - - - - - - - - - - - - -
 
 BKB->load( N , C , move( W ) , move( P ) ); 

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // Attach Solvers to the BinaryKnapsackBlock - - - - - - - - - - - - - - - - 

 // Read a configuration file
 ifstream config_file( "BinaryKnapsackPar.txt" );

 if( ! config_file.is_open() ){
  cerr << "Error: cannot open BlockSolverConfig file" << endl;
  return( 1 );  
 }

 auto bsc = new BlockSolverConfig;
 config_file >> ( * bsc );
 config_file.close();

 bsc->apply( BKB );

 delete bsc;

 // get Solvers - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Solver1 = BKB->get_registered_solvers().front();
 
 Solver2 = BKB->get_registered_solvers().back();

 // get Objective and Constraint- - - - - - - - - - - - - - - - - - - - - - -

 auto obj = BKB->get_objective< FRealObjective >(); 
 
 auto cnst = BKB->get_static_constraint< FRowConstraint >( 0 );

 // get the linear functions 

 auto lfobj = dynamic_cast< LinearFunction * >( obj->get_function() );
 if( ! lfobj ){
  cerr << "Error: cannot get the objective linear function" << endl;
  exit( 1 ); 
 }

 auto lfcnst = dynamic_cast< LinearFunction * >( cnst->get_function() );
 if( ! lfcnst ){
  cerr << "Error: cannot get the constraint linear function" << endl;
  exit( 1 ); 
 }

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     
 bool AllPassed = true;
 
 // modifications- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( int i = 0 ; i <= ( n_repeat - 1 ) * STEP ; i++ ){

  // Change the sense of the objective - - - - - - - - - - - - - - - - - - -
   
  if( wchg & 1 &&  dis( rg ) < 0.3 ){                  
  
  #if( LOG_LEVEL > 0 )
   cout << "0 ";
  #endif  

   if( dis( rg ) < 0.5 )
    BKB->set_objective_sense( 1 - BKB->get_objective_sense() );   // PR
   else
    obj->set_sense( 1 - BKB->get_objective_sense() );             // AR  
  
  }
  // Change the Capacity of the Knapsack - - - - - - - - - - - - - - - - - -
   
  if( wchg & 2 && dis( rg ) < 0.3 ){

  #if( LOG_LEVEL > 0 )
   cout << "1 ";
  #endif   
   
   C = dist_C( rg );
   
   if( dis( rg ) < 0.5 )
    BKB->chg_capacity( C );     // PR
   else
    cnst->set_rhs( C );         // AR    
  
  }                   
  
  // Change Profits (range or subset)- - - - - - - - - - - - - - - - - - - -
   
  if( wchg & 4 && dis( rg ) < 0.3 ){

  #if( LOG_LEVEL > 0 )
   cout << "2 ";
  #endif   

   int m = int( dis( rg ) * ( N / 5 ) );     // number of items to modify

   vector< double > nP( m );                 // generate new profits
   for( auto & p : nP )  
    p = dist_P( rg ); 

   if( dis( rg ) < 0.5 ){                    // ranged modification
    
    Range rng = generateRange( m );
    
    if( dis( rg ) < 0.5 ) 
     BKB->chg_profits( nP.begin() , rng );              // PR
    else
     lfobj->modify_coefficients( move( nP ) , rng );    // AR  
   
   }
   else{                                     // or subset modification
    Subset nms = generateSubset( m ); 
    if( dis( rg ) < 0.5 )
     BKB->chg_profits( nP.begin() , move( nms ) );              // PR
    else
     lfobj->modify_coefficients( move( nP ) , move( nms ) );    // AR
   }

  }                   

  // Change Weights (range or subset)- - - - - - - - - - - - - - - - - - - -
   
  if( wchg & 8 && dis( rg ) < 0.3 ){

  #if( LOG_LEVEL > 0 )
   cout << "3 ";
  #endif  
    
   int m = int( dis( rg ) * ( N / 5 ) );     // number of items to modify

   vector< double > nW( m );                 // generate new profits
   for( auto & w : nW )  
    w = dist_W( rg );

   if( dis( rg ) < 0.5 ){                    // ranged modification
    
    Range rng = generateRange( m );
    
    if( dis( rg ) < 0.5 )
     BKB->chg_weights( nW.begin() , rng );              // PR
    else
     lfcnst->modify_coefficients( move( nW ) , rng );   // AR

   }
   else{                                     // or subset modification
    
    Subset nms = generateSubset( m ); 
    
    if( dis( rg ) < 0.5 )
     BKB->chg_weights( nW.begin() , move( nms ) );              // PR
    else
    lfcnst->modify_coefficients( move( nW ) , move( nms ) );    // AR

   }

  }      
   
  // Fix x (range or subset)- - - - - - - - - - - - - - - - - - - - - - - - 
   
  if( wchg & 16 && dis( rg ) < 0.3 ){

  #if( LOG_LEVEL > 0 )
   cout << "4 ";
  #endif  
    
   int m = int( dis( rg ) * ( N / 5 ) );     // number of items to modify

   vector< bool > nX( m );
   for( int i = 0 ; i < m ; i++ )            // generate new x values
    nX[ i ] = ( dis( rg ) < 0.5 ) ? false : true;

   if( dis( rg ) < 0.5 ){                    // ranged modification
    Range rng = generateRange( m );
    BKB->fix_x( nX , rng ); 
   }
   else{                                     // or subset modification
    Subset nms = generateSubset( m ); 
    BKB->fix_x( nX , move( nms ) ); 
   }

  }       

  // Unfix x (range or subset)- - - - - - - - - - - - - - - - - - - - - - - 
   
  if( wchg & 32 && dis( rg ) < 0.3 ){

  #if( LOG_LEVEL > 0 )
   cout << "5 ";
  #endif   
    
   int m = int( dis( rg ) * ( N / 5 ) );     // number of items to modify

   if( dis( rg ) < 0.5 ){                    // ranged modification
    Range rng = generateRange( m );
    BKB->unfix_x( rng ); 
   }
   else{                                     // or subset modification
    Subset nms = generateSubset( m ); 
    BKB->unfix_x( move( nms ) ); 
   }

  }  
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 
  if( i % STEP == 0 )
   AllPassed &= SolveBoth();
 
 } 

if( AllPassed )
 cout << "All test passed" << endl;
else
 cout << "Error" << endl;    

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

 BKB->unregister_Solvers();

 delete BKB;

 return( AllPassed ? 0 : 1 );

}

/*--------------------------------------------------------------------------*/
/*------------------------ End File test.cpp -------------------------------*/
/*--------------------------------------------------------------------------*/








