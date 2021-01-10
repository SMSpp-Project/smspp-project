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

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace std;
using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- CONSTANTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

const char *const logF = "log.bn";

/*--------------------------------------------------------------------------*/
/*--------------------------------- Main -----------------------------------*/
/*--------------------------------------------------------------------------*/

char type = 's';     // type of the input file
int main( int argc , char **argv )
{
 if( argc != 3 ) {
  cerr << "Usage: " << argc << " -- " << argv[ 0 ] << " MMCF_file_name NC4_file_name [NC4_file_name_2]" << endl;
  return( 1 );
  }

 ifstream ProbFile( argv[ 2 ] );
 if( ! ProbFile.is_open() ) {
  cerr << "Error: cannot open file " << argv[ 1 ] << endl;
  return( 1 );
  }

 Block *sblock = Block::new_Block( "MMCFBlock" );
 auto sMMCFblock = static_cast< MMCFBlock * >( sblock );
 ProbFile >> *sMMCFblock;

 char filetype = sMMCFblock->get_filetype();
 sMMCFblock->MakeMMCF( argv[ 1 ] , filetype );
 cout << *sMMCFblock;

 BlockSolverConfig * bsc = new BlockSolverConfig;
 ProbFile >> *( bsc );

 bsc->apply( sMMCFblock );
 delete bsc;

 Solver * slvr = (sMMCFblock->get_registered_solvers()).front();

 // open log-file - - - - - - - - - - -  - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 ofstream LOGFile( logF , ofstream::out );
 if( ! LOGFile.is_open() )
  cerr << "Warning: cannot open log file """ << logF << """" << endl;
 else
  slvr->set_log( &LOGFile );

 int rtrn = slvr->compute( false );

 LOGFile << std::endl << std::endl << "f* = "
		 << slvr->get_lb() << " (optimal value)" << std::endl;

 delete sMMCFblock;

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File main.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/

