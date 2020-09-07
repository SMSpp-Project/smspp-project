/*--------------------------------------------------------------------------*/
/*-------------------- File tests_BendersBFunction.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing BendersBFunction
 *
 * \version 0.10
 *
 * \date 28 - 11 - 2019
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Rafael Durbano Lobato
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "BlockSolverConfig.h"
#include "BundleSolver.h"
#include "CPXMILPSolver.h"
#include "CWLAbstractBlockBuilder.h"

#include "cwl-mcf/cwl-mcf.h"

#include <iostream>
#include <filesystem>

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;
using namespace SMSpp_di_unipi_it::tests;

/*--------------------------------------------------------------------------*/
/*-------------------------- AUXILIARY TYPES -------------------------------*/
/*--------------------------------------------------------------------------*/

enum SolverType { MILPSolver , BundleSolver };

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

BlockSolverConfig * build_config( const std::string & config_file_path ) {

 std::ifstream config_file( config_file_path );
 if( ! config_file.is_open() )
  throw std::invalid_argument( "Error: cannot open file " + config_file_path );

 auto bsc = new RBlockSolverConfig;
 config_file >> ( * bsc );
 config_file.close();
 return bsc;
}

/*--------------------------------------------------------------------------*/

double solve_with_BundleSolver( std::string file_name ,
                                bool continuous_relaxation ) {
 auto inner_block_solver = new CPXMILPSolver();
 auto block = build_CWL_block_with_Benders_decomposition
   ( file_name , continuous_relaxation , inner_block_solver );

 auto block_solver_config = build_config( "BundlePar-cwl.txt" );
 block_solver_config->apply( block );

 auto solver = block->get_registered_solvers().front();
 auto status = solver->compute();
 if( status != ThinComputeInterface::kOK )
  std::cout << "Problem not solved for instance " << file_name << std::endl;
 auto solution_value = solver->get_var_value();
 std::cout << solution_value << std::endl;

 block_solver_config->reset_Solver( block );
 delete block_solver_config;
 delete block;
 return solution_value;
}

/*--------------------------------------------------------------------------*/

double solve_with_MILPSolver( std::string file_name ,
                              bool continuous_relaxation ) {
 auto block = build_CWL_block( file_name , continuous_relaxation );
 auto solver = new CPXMILPSolver();
 block->register_Solver( solver );
 auto status = solver->compute();
 if( status != ThinComputeInterface::kOK )
  std::cout << "Problem not solved for instance " << file_name << std::endl;
 auto solution_value = solver->get_var_value();
 delete block;
 return solution_value;
}

/*--------------------------------------------------------------------------*/

void compare( std::string data_dir_path ,
              SolverType solver_type = SolverType::BundleSolver ,
              double epsilon = 1.0e-6 ) {

 const bool continuous_relaxation = true;

 for( const auto & file :
       std::filesystem::directory_iterator( data_dir_path ) ) {
  auto file_name = file.path();

  double solution_value = 0;

  if( solver_type == SolverType::MILPSolver )
   solution_value = solve_with_MILPSolver( file_name , continuous_relaxation );
  else if( solver_type == SolverType::BundleSolver )
   solution_value = solve_with_BundleSolver( file_name , continuous_relaxation );
  else {
   std::cerr << "Unknown Solver type: " << solver_type << std::endl;
   exit( 1 );
  }

  auto cwl_mcf_value = cwl_mcf( file_name );
  auto diff = std::abs( solution_value - cwl_mcf_value );
  auto max_diff = std::max( epsilon , epsilon *
                            std::min( abs( solution_value ),
                                      abs( cwl_mcf_value ) ) );
  if( diff > max_diff )
   std::cout << "Solution value difference for instance " <<
    file_name << ": "  << diff << std::endl;
 }
}

/*--------------------------------------------------------------------------*/

int main( int argc, char ** argv ) {

 if( argc < 2 ) {
  std::cerr << "The path to the directory containing the instance files " <<
   "must be provided as argument." << std::endl;
  std::cerr << "Usage: " << argv[ 0 ] << " PATH" << std::endl;
  return 1;
 }

 std::string path = argv[ 1 ];

 compare( path , SolverType::MILPSolver );

 return 0;
}

/*--------------------------------------------------------------------------*/
/*------------------ End File tests_BendersBFunction.cpp -------------------*/
/*--------------------------------------------------------------------------*/
