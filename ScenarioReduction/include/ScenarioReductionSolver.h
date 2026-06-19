/*--------------------------------------------------------------------------*/
/*------------------- File ScenarioReductionSolver.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Fully generic heuristic scenario reduction
 * solver: the four heuristic methods Baseline, Dupacova (forward selection),
 * BestFit and FirstFit (local search), selecting K representative scenarios by
 * minimising the (weighted) Wasserstein distance to the full set.
 *
 * Unlike the library's CFLScenarioReductionSolver, which reads N, K, weights
 * and the distance matrix from a CapacitatedFacilityLocationBlock (so callers
 * must build a *synthetic* CFLB just to carry the scenario distances), this
 * solver reads the scenario vectors DIRECTLY from the DiscreteScenarioSet held
 * by the ScenarioReductionBlock, exactly like GenericCSSCScenarioReductionSolver.
 * It therefore works unchanged for CFL, UC, or any other problem; no
 * problem-specific Block is needed.
 *
 * ### How to use
 * @code
 *   auto srs = std::make_unique< ScenarioReductionSolver >();
 *   srs->set_nb_reduced( K );
 *   srs->set_algorithm( ScenarioReductionSolver::Algorithm::Dupacova );
 *   srs->set_Block( srb );          // ScenarioReductionBlock (with DSS)
 *   srs->compute();
 *   srs->get_var_solution();        // writes ScenarioReductionBlockSolution
 * @endcode
 *
 * The algorithms themselves are ports of the routines in
 * CFLScenarioReductionSolver; only the data-loading layer differs
 *
 * \author Minh Duc Pham \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 */
/*--------------------------------------------------------------------------*/

#ifndef __ScenarioReductionSolver
#define __ScenarioReductionSolver

/*--------------------------------------------------------------------------*/

#include "DiscreteScenarioSet.h"
#include "Objective.h"
#include "ScenarioReductionBlock.h"
#include "Solver.h"

#include <random>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/

namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS ScenarioReductionSolver ---------------------*/
/*--------------------------------------------------------------------------*/

class ScenarioReductionSolver : public Solver {

public:

/*--------------------------------------------------------------------------*/

 using Index   = unsigned int;
 using OFValue = RealObjective::OFValue;

 /** Available distribution-driven scenario reduction algorithms. */
 enum class Algorithm {
  Baseline = 0 ,  ///< select the K highest-weight scenarios
  Dupacova = 1 ,  ///< Dupacova forward selection (default)
  BestFit  = 2 ,  ///< local search, best-improvement swap
  FirstFit = 3    ///< local search, first-improvement swap
  };

/*--------------------------------------------------------------------------*/

 ScenarioReductionSolver()           = default;
 ~ScenarioReductionSolver() override = default;

/*--------------------------------------------------------------------------*/

 /** Attach to a ScenarioReductionBlock.  Reads the scenario vectors from its
  * DiscreteScenarioSet and builds the pairwise (Euclidean) distance matrix and
  * the uniform weights.  Call set_nb_reduced() before compute(). */
 void set_Block( Block * block ) override;

 /** Run the selected heuristic.  Returns kOK on success. */
 int compute( bool changedvars = false ) override;

 /** Write the solution (representatives + assignments + weights) into the
  * ScenarioReductionBlock. */
 void get_var_solution( Configuration * solc = nullptr ) override;

 bool has_var_solution() override { return ! ind_red.empty(); }

 OFValue get_var_value() override { return f_solution_value; }

/*--------------------------------------------------------------------------*/

 /** Number of representative scenarios K (0 < K <= N). */
 void set_nb_reduced( int K ) { f_K = K; }

 /** Select the algorithm (enum or 0..3 int, matching CFLScenarioReductionSolver
  * Baseline=0, Dupacova=1, BestFit=2, FirstFit=3). */
 void set_algorithm( Algorithm a ) { f_algo = a; }
 void set_algorithm( int a ) { f_algo = static_cast< Algorithm >( a ); }

 /** FirstFit only: shuffle the representative order each pass. */
 void set_shuffle( bool s ) { f_shuffle = s; }

 /** Seed for the local-search random initialisation / shuffling. */
 void set_seed( unsigned int s ) { f_rng.seed( s ); }

/*--------------------------------------------------------------------------*/

private:

 int   f_K   = 0;          ///< requested number of representatives
 Index nb_atoms   = 0;     ///< total scenarios N
 Index nb_reduced = 0;     ///< = f_K, validated in compute()
 Algorithm f_algo = Algorithm::Dupacova;
 bool  f_shuffle  = false;
 std::mt19937 f_rng{ 12345u };  ///< deterministic by default (reproducible)

 /** Pairwise (weighted-able) distance matrix between scenario vectors. */
 std::vector< std::vector< double > > f_dist;
 /** Scenario weights (uniform 1/N). */
 std::vector< double > f_weights;
 /** Relative pool index [0..N-1] -> absolute DSS scenario index. */
 std::vector< Index > f_pool_map;

 std::vector< Index > ind_red;            ///< selected representatives (relative)
 std::vector< Index > indices_to_choose;  ///< non-representative indices
 std::vector< bool >  reduced_atoms;      ///< mask of representatives
 double f_solution_value = 0.0;           ///< Wasserstein distance achieved

 /*------ ported algorithms (operate only on f_dist / f_weights) ----------*/
 int    compute_dupacova();
 int    compute_baseline();
 int    compute_local_search();
 double init_local_search();

 std::tuple< int , int > pick_dupacova(
  const std::vector< double > & min_cost );

 std::tuple< Index , Index , double > pick_ij_bestfit( double curr_d );
 std::pair< Index , double > bestfit_selection(
  const std::vector< Index > & curr_indices );

 std::tuple< Index , Index , double > pick_ij_firstfit( double curr_d );
 std::pair< Index , double > firstfit_selection(
  const std::vector< Index > & curr_indices , double curr_d );

 void swap_indices( Index i , Index j );
 void update_reduced_atoms();

 SMSpp_insert_in_factory_h;

 };  // end( class ScenarioReductionSolver )

}  // end( namespace SMSpp_di_unipi_it )

#endif  /* ScenarioReductionSolver.h included */

/*--------------------------------------------------------------------------*/
/*----------------- End File ScenarioReductionSolver.h --------------------*/
/*--------------------------------------------------------------------------*/
