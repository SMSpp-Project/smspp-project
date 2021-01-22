/*--------------------------------------------------------------------------*/
/*----------------------------- File main.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Small main() for testing MMCFBlock using MILPSolver.
 * It just creates one and loads it from a stream;
 * little more than a compilation check.
 *
 * \version 0.10
 *
 * \date 30 - 12 - 2020
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Enrico Gorgone \n
 *         Dipartimento di Matematica ed Informatica \n
 *         Universita' di Cagliari \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>

#include "MMCFBlock.h"
#include "MILPSolver.h"
#include "BlockSolverConfig.h"

#include "MMCFCple.h"
#include "Graph.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace std;
using namespace SMSpp_di_unipi_it;
using namespace MMCFClass_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- FUNCTIONS ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< typename T>
static void read_T( istream & iStrm , T & t )
{
 iStrm >> eatcomments;

 int c = iStrm.peek();

 switch( c ) {
  case 'I' :
  case 'i' : t = SMSpp_di_unipi_it::Inf<T>();
             break;
  case '-' : iStrm.get();
             read_T( iStrm , t );
             t = - t;
             return;
  case 'M' :
  case 'm' : t = -SMSpp_di_unipi_it::Inf<T>();
             break;
  default :  iStrm >> t;
             return;
  }

 do { c = iStrm.get(); c = iStrm.peek();
  } while( ( c != iStrm.widen( ' ' ) ) &&
	   ( c != iStrm.widen( '\n' ) ) &&
	   ( c != iStrm.widen( '\t' ) ) );

 }

/*--------------------------------------------------------------------------*/

static inline int read_int( istream & iStrm )
{
 int d;
 read_T( iStrm , d );
 return( d );
 }

/*--------------------------------------------------------------------------*/

static inline double read_dbl( istream & iStrm )
{
 double d;
 read_T( iStrm , d );
 return( d );
 }

/*--------------------------------------------------------------------------*/

static inline string read_string( istream & iStrm )
{
 iStrm >> eatcomments;
 string s;
 int c = iStrm.peek();
 iStrm >> s;
 return( s );
 }

/*--------------------------------------------------------------------------*/

static inline char read_char( istream & iStrm )
{
 char d;
 read_T( iStrm , d );
 return( d );
 }

/*--------------------------------------------------------------------------*/

template<class T>
static inline void str2val( const char* const str , T &sthg )
{
 istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CONSTANTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

const char *const logF = "out.txt";

#define USECOLORS 1
#if( USECOLORS )
 #define RED( x ) "\x1B[31m" #x "\033[0m"
 #define GREEN( x ) "\x1B[32m" #x "\033[0m"
#else
 #define RED( x ) #x
 #define GREEN( x ) #x
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------- Main -----------------------------------*/
/*--------------------------------------------------------------------------*/

char type = 's';     // type of the input file
int main( int argc , char **argv )
{
 if( argc != 4 ) {
  cerr << "Usage: " << argc << " -- " << argv[ 0 ] << " MMCF_file_name NC4_file_name [NC4_file_name_2]" << endl;
  return( 1 );
  }

 ifstream ProbFile( argv[ 1 ] );
 if( ! ProbFile.is_open() ) {
  cerr << "Error: cannot open file " << argv[ 1 ] << endl;
  return( 1 );
  }

 Block *sblock = Block::new_Block( "MMCFBlock" );
 auto sMMCFblock = static_cast< MMCFBlock * >( sblock );
 ProbFile >> *sMMCFblock;

 BlockSolverConfig * bsc = new BlockSolverConfig;
 ProbFile >> *( bsc );

 char filetype;
 str2val( argv[ 3 ] , filetype );

 sMMCFblock->Load( argv[ 2 ] , filetype );
 sMMCFblock->PreProcess();
 sMMCFblock->MakeMMCF();
 // cout << *sMMCFblock;

 bsc->apply( sMMCFblock );
 delete bsc;

 Solver * slvr = (sMMCFblock->get_registered_solvers()).front();

 // open log-file - - - - - - - - - - -  - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 int rtrn = slvr->compute( false );
 double lb_value = slvr->get_lb();
 double ub_value = slvr->get_ub();

 ofstream LOGFile( logF , ofstream::out );
 if( ! LOGFile.is_open() )
  cerr << "Warning: cannot open log file """ << logF << """" << endl;
 else
  slvr->set_log( &LOGFile );

 LOGFile << std::endl << std::endl << "f* = "
		 << lb_value << " (optimal value)" << std::endl;

 delete sMMCFblock;


 // set the Log of NDData  - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 ofstream LOGCPX( "log.cpx" , ofstream::out );
 if( ! LOGCPX.is_open() )
  cerr << "Warning: cannot open log file log.cpx" << endl;

 // open parameters file - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 int BBlvl = read_int( ProbFile );      // level of verbosity of MIP problem
 double epsilon = read_dbl( ProbFile ); // relative tolerance
 int threads1 = read_dbl( ProbFile );   // the number of threads

 // read and modify the problem - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Graph *Gh = new Graph( argv[ 2 ] , filetype );
 Gh->PreProcess();

 // allocate the solver - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 MMCFCplex *mmcf = new MMCFCplex( Gh , &ProbFile );
 ProbFile.close();

 // set tolerance  - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 mmcf->SetCplexParam( CPX_PARAM_EPOPT , epsilon );
 mmcf->SetCplexParam( CPX_PARAM_EPGAP , epsilon);

 // pass the number of threads - - - - - - - - - - - - - - - - - - - - - - -

 mmcf->SetCplexParam( CPX_PARAM_THREADS , threads1 );

 // pass Log- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 mmcf->SetMMCFLog( &LOGCPX , BBlvl );

 // free some memory- - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 delete( Gh );

 // set the timers on - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 mmcf->SetMMCFTime();

 // solve the problem - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 MMCFClass::MMCFStatus Status = mmcf->SolveMMCF();

 // get the results - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 double tu , ts;
 mmcf->TimeMMCF( tu , ts );       // get the running time

 MMCFClass::FONumber OV1 = mmcf->GetPVal();      // get the primal value
 MMCFClass::FONumber OV2 = mmcf->GetDVal();      // get the dual value

 // clean up- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 delete mmcf ;

 // output the results- - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LOGFile << "Time:" << tu + ts  << "\t"  ;

 switch( Status ) {
  case( MMCFClass::kOK ) :
   LOGFile.precision( 8 );
   LOGFile << "Status: OK, Value: ( " << OV1 << " , "<< OV2 << " ) " << endl;
   if( abs( lb_value - OV1 ) <= 1e-6 )
	cout << GREEN( Test passed!! ) << endl;
   else
	cout << RED( Shit happened!! ) << endl;
   break;
  case( MMCFClass::kStopped ) :
   LOGFile << "Status: Stopped: ( " << OV1 << " , "<< OV2 << " ) " << endl;
   break;
  case( MMCFClass::kUnfeasible ) :
   LOGFile << "Status: Unfeas." << endl;
   if( ub_value >= SMSpp_di_unipi_it::Inf<double>() )
	cout << GREEN( Test passed!! ) << endl;
   else
	cout << RED( Shit happened!! ) << endl;
   break;
  case( MMCFClass::kUnbounded ) :
   LOGFile << "Status: Unbound." << endl;
   if( lb_value <= -SMSpp_di_unipi_it::Inf<double>() )
 	cout << GREEN( Test passed!! ) << endl;
   else
 	cout << RED( Shit happened!! ) << endl;
   break;
  default :
   LOGFile << "Status: Error" << endl;
   cout << RED( Shit happened!! ) << endl;
   }

 LOGCPX.close();

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File main.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/

