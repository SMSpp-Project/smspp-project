/*--------------------------------------------------------------------------*/
/*------------ File CSSCScenarioReductionSolver.h ------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Fully generic CSSC scenario reduction solver.
 *
 * Implements the Cost-Space Scenario Clustering (CSSC) algorithm of
 * Keutchayan et al. (2023) with NO dependency on any specific Block type.
 * It works with any TwoStageStochasticBlock whose inner block can be solved
 * by a registered MILPSolver.
 *
 * ### How to use
 *
 * @code
 *   auto cssc = std::make_unique<CSSCScenarioReductionSolver>();
 *   cssc->set_milp_config( bsc );            // solver config
 *   cssc->set_nb_reduced( K );               // number of representatives
 *   cssc->set_var_extractor( []( Block * inner ) {
 *       // problem-specific: collect here-and-now ColVariable pointers
 *       auto * my_block = dynamic_cast< MyBlock * >( inner );
 *       std::vector< ColVariable * > vars;
 *       // ...
 *       return vars;
 *   } );
 *   cssc->set_Block( srb );                  // ScenarioReductionBlock
 *   cssc->compute();
 *   cssc->get_var_solution();
 * @endcode
 *
 * The ScenarioReductionBlock requires:
 *   - a TwoStageStochasticBlock (set via set_stochastic_block)
 *   - a StochasticBlock applicator (set via set_scenario_applicator)
 *   - a DiscreteScenarioSet (set via set_scenario_generator)
 *
 * \author Minh Duc Pham \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 */
/*--------------------------------------------------------------------------*/

#ifndef __CSSCScenarioReductionSolver
#define __CSSCScenarioReductionSolver

/*--------------------------------------------------------------------------*/

#include "BlockSolverConfig.h"
#include "ColVariable.h"
#include "DiscreteScenarioSet.h"
#include "ScenarioReductionBlock.h"
#include "Solver.h"
#include "StochasticBlock.h"
#include "TwoStageStochasticBlock.h"

#include <functional>
#include <memory>
#include <vector>

/*--------------------------------------------------------------------------*/

namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*-------------- CLASS CSSCScenarioReductionSolver ------------------*/
/*--------------------------------------------------------------------------*/

class CSSCScenarioReductionSolver : public Solver {

public:

/*--------------------------------------------------------------------------*/

 using Index    = unsigned int;
 using OFValue  = RealObjective::OFValue;

 /** Callable: receives the inner Block (after generate_abstract_variables)
  * and returns pointers to all here-and-now (first-stage) ColVariables
  * Examples:
  *   - CFL:  pointers to y[0]…y[nf-1] of CapacitatedFacilityLocationBlock
  *   - UC:   pointers to u[0]…u[T-1] of each ThermalUnitBlock */
 using VarExtractor = std::function< std::vector< ColVariable * >( Block * ) >;

/*--------------------------------------------------------------------------*/

 CSSCScenarioReductionSolver()  = default;
 ~CSSCScenarioReductionSolver() override;

/*--------------------------------------------------------------------------*/

 /** Attach to a ScenarioReductionBlock.
  * Requires: TwoStageStochasticBlock, StochasticBlock applicator, and
  * DiscreteScenarioSet.  No CapacitatedFacilityLocationBlock needed
  * Call set_nb_reduced() and set_milp_config() before this. */
 void set_Block( Block * block ) override;

 /** Run CSSC: Step 1 (V matrix) + Step 2 (MILP partitioning). */
 int compute( bool changedvars = false ) override;

 /** Write the solution (representatives + assignments + weights) into the
  * ScenarioReductionBlock. */
 void get_var_solution( Configuration * solc = nullptr ) override;

 bool has_var_solution() override { return ! ind_red.empty(); }

 /** Returns the Wasserstein distance of the selected scenario set. */
 OFValue get_var_value() override { return f_solution_value; }

/*--------------------------------------------------------------------------*/

 /** Transfer ownership of the BlockSolverConfig used for both Step 1
  * sub-problem solves and the Step 2 partitioning MILP. */
 void set_milp_config( BlockSolverConfig * cfg ) {
  delete f_milp_config;
  f_milp_config = cfg;
  }

 /** Register the problem-specific variable extractor (must be called before
  * compute()).  The lambda receives the inner Block (fully initialised via
  * generate_abstract_variables) and must return ColVariable* pointers to all
  * here-and-now variables. */
 void set_var_extractor( VarExtractor extractor ) {
  f_var_extractor = std::move( extractor );
  }

 /** Optional: register a problem-specific data injector.
  *
  * The default data injection path (StochasticBlock::set_data with
  * eNoBlck flags) is sufficient for some block types (e.g. UC) where
  * set_data triggers an NBModification that causes the solver to reload.
  * For other block types (e.g. CFL) the DataMapping does not send a solver
  * notification, so the solver reads stale LP data.
  *
  * When registered, the injector is called AFTER set_data() for every
  * scenario injection and is responsible for sending the solver-visible
  * modification. */
 using DataInjector = std::function< void( Block * ,
                                           const std::vector< double > & ) >;
 void set_data_injector( DataInjector injector ) {
  f_data_injector = std::move( injector );
  }

 /** Set the number of representative scenarios K (must be called before
  * compute(); must satisfy 0 < K <= N). */
 void set_nb_reduced( int K ) { f_K = K; }

 /** Wall-clock time limit (seconds) for the Step-2 partitioning MILP*/
 void set_milp_time_limit( double seconds ) { f_milp_time_limit = seconds; }

 /** Control how variable fixing/unfixing is communicated to solvers.
  *
  * When true (default, CFL-compatible): is_fixed() is called with eModBlck
  * so the registered solver receives an explicit modification and updates
  * variable bounds immediately.
  *
  * When false (UC-compatible): is_fixed() is called with eNoBlck (no
  * modification sent up the block hierarchy).  The solver learns about the
  * fixed state through the NBModification triggered by the next set_data()
  * call, which causes load_problem() to rescan all variable states.
  * Use this mode when the inner block type rejects VariableMod */
 void set_fix_with_modification( bool fwm ) { f_fix_with_mod = fwm; }

 using BlockFactory =
  std::function< std::unique_ptr< Block >( const std::vector< double > & ) >;
 void set_block_factory( BlockFactory factory ) {
  f_block_factory = std::move( factory );
  }

/*--------------------------------------------------------------------------*/

private:

 TwoStageStochasticBlock * f_tssb             = nullptr;
 StochasticBlock         * f_stoch_applicator = nullptr;
 BlockSolverConfig       * f_milp_config      = nullptr;
 VarExtractor              f_var_extractor;
 DataInjector              f_data_injector;   ///< optional: forces solver notification after set_data
 BlockFactory              f_block_factory;   ///< optional: per-scenario fresh inner block (UC)
 bool                      f_fix_with_mod = true;  ///< if false, is_fixed() uses eNoBlck (UC mode)
 double                    f_milp_time_limit = 120.0;  ///< Step-2 MILP wall-clock limit (s); <=0 disables

 int f_N = 0;  ///< total scenarios in pool
 int f_K = 0;  ///< target number of representatives

 /** Pool index mapping: relative [0..f_N-1] → absolute DSS scenario index. */
 std::vector< Index > f_pool_map;

 /** Scenario weights (uniform 1/N). */
 std::vector< double > f_weights;

 /** Pairwise Euclidean distances between scenario feature vectors. */
 std::vector< std::vector< double > > f_dist;

 /** Solution state (set by solve_cssc_milp, read by get_var_solution). */
 std::vector< bool >  reduced_atoms;
 std::vector< Index > ind_red;           ///< relative indices of representatives
 std::vector< Index > f_representatives; ///< same as ind_red (for get_var_solution)
 std::vector< Index > f_assignments;     ///< assignment[i] = relative rep index
 std::vector< Index > indices_to_choose; ///< non-representative indices

 double f_solution_value = 0.0;  ///< Wasserstein distance

 std::vector< std::vector< double > > f_V_matrix;

 /** Step 1: build the NxN cost-space matrix V (V[i][j] = cost of scenario i's
  * optimal first-stage applied to scenario j).  One routine, two interchange-
  * able fill strategies selected internally: a fresh inner block per cell when
  * a block factory is registered (set_block_factory), else one reused inner
  * block with incremental injection (DataInjector). */
 std::vector< std::vector< double > > compute_generic_V_matrix();

 /** Step 2: solve the MILP partitioning problem (fully generic, uses only
  * SMS++ AbstractBlock / ColVariable / LinearFunction / FRowConstraint). */
 void solve_cssc_milp( const std::vector< std::vector< double > > & V );

 void update_reduced_atoms() {
  std::fill( reduced_atoms.begin() , reduced_atoms.end() , false );
  for( auto r : ind_red ) reduced_atoms[ r ] = true;
  }

 SMSpp_insert_in_factory_h;

 };  // end( class CSSCScenarioReductionSolver )

}  // end( namespace SMSpp_di_unipi_it )

#endif  /* CSSCScenarioReductionSolver.h included */

/*--------------------------------------------------------------------------*/
/*------- End File CSSCScenarioReductionSolver.h -------------------*/
/*--------------------------------------------------------------------------*/
