/*--------------------------------------------------------------------------*/
/*---------------- File AbstractScenarioReductionTest.h --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Abstract base class for scenario reduction tests.
 * Provides the common framework for testing scenario reduction algorithms
 * on different problem types.
 *
 * ## Architecture
 *
 * This class uses the new ScenarioReductionBlock-based architecture.
 * Subclasses must implement two pure virtual methods:
 *
 *  - create_srb(): builds and returns a fully configured
 *    ScenarioReductionBlock (with synthetic CFLB, and optionally TSSB +
 *    applicator for CSSC).
 *
 *  - build_tssb_for_current_pool(): builds a TwoStageStochasticBlock from
 *    the base problem instance and the current active pool of scenario_set.
 *    Called by solve_stochastic_problem() for both the full and reduced
 *    solves.
 * 
 * \author Benoît Tran \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Benoît Tran
 */
/*--------------------------------------------------------------------------*/

#ifndef __AbstractScenarioReductionTest
#define __AbstractScenarioReductionTest

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ScenarioReductionCommon.h"

#include <chrono>
#include <iostream>
#include <memory>

#include "BlockSolverConfig.h"
#include "DiscreteScenarioSet.h"
#include "ScenarioReductionBlock.h"
#include "StochasticBlock.h"
#include "ThinComputeInterface.h"
#include "TwoStageStochasticBlock.h"

/*--------------------------------------------------------------------------*/
/*------------------------------ NAMESPACE ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ScenarioReductionTesting {

 using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------- CLASS AbstractScenarioReductionTest --------------------*/
/*--------------------------------------------------------------------------*/

 class AbstractScenarioReductionTest {

 /*--------------------------------------------------------------------------*/
 /*-------------------------- PUBLIC METHODS --------------------------------*/
 /*--------------------------------------------------------------------------*/
 public:

  AbstractScenarioReductionTest();
  virtual ~AbstractScenarioReductionTest();

  /** Main entry point. Fixed workflow:
   *  1. parse_arguments + print_configuration
   *  2. load (calls load_problem_instance)
   *  3. solve_stochastic_problem  (full N scenarios)
   *  4. solve_anticipative        (optional VPI, full)
   *  5. solve_scenario_reduction  (uses ScenarioReductionBlock)
   *  6. solve_stochastic_problem  (reduced K scenarios)
   *  7. solve_anticipative        (optional VPI, reduced)
   *  8. save_solutions_cache      (optional)
   *  9. print_results             (optional)
   */
  int run( int argc , char * argv[] );

 /*--------------------------------------------------------------------------*/
 /*------------------------- PROTECTED METHODS ------------------------------*/
 /*--------------------------------------------------------------------------*/
 protected:

  // ===================== Pure Virtual: problem-specific ===================

  /** Load the base problem instance from @p path.
   *  Must set base_block and create stochastic_block. */
  virtual void load_problem_instance( const std::string & path ) = 0;

  /** Build and return a fully configured ScenarioReductionBlock for the
   *  current scenario_set pool.
   *
   *  The returned SRB must have at minimum:
   *   - scenario_generator set to scenario_set.get()
   *   - sub_problem_block set to a synthetic NxN CFLB encoding N, K,
   *     uniform weights, and pairwise distances
   *
   *  For CSSC additionally:
   *   - stochastic_block set to a TwoStageStochasticBlock
   *   - scenario_applicator set to its StochasticBlock child
   *
   *  Ownership of the SRB is transferred to the caller (AbstractSRT).
   *  The subclass may keep owning pointers to any objects it placed
   *  inside the SRB; the SRB itself does not own them.
   *
   *  @param K     Number of representatives requested.
   *  @param method  Reduction method string (e.g. "cssc", "dupacova").
   */
  virtual std::unique_ptr< ScenarioReductionBlock >
  create_srb( int K , const std::string & method ) = 0;

  /** Build a TwoStageStochasticBlock using the base instance and the
   *  currently active pool of scenario_set (whatever size it is).
   *  Called by solve_stochastic_problem() for both full and reduced solves.
   *
   *  @param tmp  Path for the temporary netCDF file (deleted after load).
   */
  virtual std::unique_ptr< TwoStageStochasticBlock >
  build_tssb_for_current_pool( const std::string & tmp ) = 0;

  /** Problem type string (e.g. "CFL", "UC"). Used in print/cache names. */
  virtual std::string get_problem_type() const = 0;

  /** Run the CSSC solver on @p srb using @p bsc as the MILP config.
   *  The default implementation uses CSSCScenarioReductionSolver.
   *  Subclasses may override to use a problem-specific CSSC implementation.
   *  @p bsc ownership is transferred (solver will delete it). */
  virtual void run_cssc( ScenarioReductionBlock * srb ,
                         BlockSolverConfig      * bsc ,
                         int                      K );

  // ===================== Virtual with default implementation ==============

  virtual std::string get_scenarios_directory() const {
   return "../scenarios/" + get_problem_type() + "/";
  }

  virtual std::string get_scenario_file(
   const std::string & instance_path ) const;

  virtual void parse_arguments( int argc , char * argv[] );
  virtual void print_help( const char * program_name );

  // ===================== Common protected methods =========================

  virtual size_t get_scenario_dimension() const { return dimension_scenario; }

  Block * get_base_block();

  void apply_scenario_to_block( const std::vector< double > & scenario );

  void print_configuration();
  void load();

  /** Perform scenario reduction using ScenarioReductionBlock.
   *
   *  Calls create_srb() to get a configured SRB, attaches the appropriate
   *  solver (CFLScenarioReductionSolver or CSSCScenarioReductionSolver),
   *  runs compute() + get_var_solution(), reads the solution from the SRB,
   *  and updates scenario_set to reflect the K selected representatives.
   */
  void solve_scenario_reduction();

  /** Solve the extensive form with the current pool of scenario_set.
   *  Calls build_tssb_for_current_pool() to obtain the TSSB. */
  void solve_stochastic_problem();

  void solve_anticipative();
  void save_solutions_cache();
  void print_results();

  // ===================== I/O helpers =====================================

  std::vector< std::vector< double > > load_scenarios_from_file(
   const std::string & filename );

  std::string generate_cache_filename(
   bool is_full , bool is_anticipative ) const;

  std::string extract_instance_name(
   const std::string & instance_path ) const;

  void save_solution_cache(
   const std::string & filename ,
   const SolutionResult & result );

  SolutionResult load_solution_cache( const std::string & filename );

  void save_reduction_solution(
   const std::string & filename ,
   const ScenarioReductionMetrics & metrics );

  ScenarioReductionMetrics load_reduction_solution(
   const std::string & filename );

  std::string generate_reduction_cache_filename() const;

  std::pair< double , bool > solve( Block * block );

  SolutionResult compute_extensive_form(
   TwoStageStochasticBlock * tss_block );

  SolutionResult solve_anticipative_solution();

  bool update_SR_config(
   const std::string & method , bool warmstart , bool shuffle );

  // ===================== Protected data members ==========================

  ComputeConfig * config = nullptr;

  int         get_int_config( const std::string & name ) const;
  double      get_dbl_config( const std::string & name ) const;
  std::string get_str_config( const std::string & name ) const;

  std::unique_ptr< DiscreteScenarioSet > scenario_set;

  SolutionResult full_result;
  SolutionResult reduced_result;
  SolutionResult anticipative_full;
  SolutionResult anticipative_reduced;
  ScenarioReductionMetrics reduction_metrics;

  Block * base_block = nullptr;
  std::unique_ptr< StochasticBlock > stochastic_block;

  size_t dimension_scenario = 0;

 };  // class AbstractScenarioReductionTest

}  // namespace ScenarioReductionTesting

#endif /* __AbstractScenarioReductionTest */

/*--------------------------------------------------------------------------*/
/*------------------ End File AbstractScenarioReductionTest.h --------------*/
/*--------------------------------------------------------------------------*/