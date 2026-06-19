/*--------------------------------------------------------------------------*/
/*------------------ File ScenarioReductionSolver.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of ScenarioReductionSolver, generic heuristic
 * scenario reduction (Baseline / Dupacova / BestFit / FirstFit).
 *
 * The algorithms are ports of the problem-agnostic routines in
 * CFLScenarioReductionSolver; the only difference is that the distance matrix
 * and weights are built here directly from the DiscreteScenarioSet rather than
 * read from a CapacitatedFacilityLocationBlock.
 *
 * \author Benoît Tran \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Minh Duc Pham \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Benoît Tran
 */
/*--------------------------------------------------------------------------*/

#include "ScenarioReductionSolver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unordered_map>

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/

SMSpp_insert_in_factory_cpp_1( ScenarioReductionSolver );

/*--------------------------------------------------------------------------*/
/*---------------------------- set_Block -----------------------------------*/
/*--------------------------------------------------------------------------*/

void ScenarioReductionSolver::set_Block( Block * block )
{
 Solver::set_Block( block );
 if( ! block )
  return;

 auto * srb = dynamic_cast< ScenarioReductionBlock * >( block );
 if( ! srb )
  throw std::invalid_argument(
   "ScenarioReductionSolver::set_Block: block must be a "
   "ScenarioReductionBlock" );

 auto * dss = dynamic_cast< DiscreteScenarioSet * >(
  srb->get_scenario_generator() );
 if( ! dss )
  throw std::invalid_argument(
   "ScenarioReductionSolver::set_Block: ScenarioGenerator is not a "
   "DiscreteScenarioSet" );

 nb_atoms = static_cast< Index >( dss->get_poolSize() );
 const auto sc_dim = dss->get_scenarioSize();

 // pool_map: relative [0..N-1] -> absolute DSS scenario index
 const auto pool = dss->get_selected_scenarios();
 f_pool_map.assign( pool.begin() , pool.end() );
 if( f_pool_map.size() != nb_atoms ) {       // fall back to identity
  f_pool_map.resize( nb_atoms );
  std::iota( f_pool_map.begin() , f_pool_map.end() , 0 );
  }

 // uniform weights 1/N
 f_weights.assign( nb_atoms , nb_atoms ? 1.0 / nb_atoms : 0.0 );

 // pairwise Euclidean distance matrix (Wasserstein ground metric)
 f_dist.assign( nb_atoms , std::vector< double >( nb_atoms , 0.0 ) );
 for( Index i = 0 ; i < nb_atoms ; ++i )
  for( Index j = i + 1 ; j < nb_atoms ; ++j ) {
   double d2 = 0.0;
   for( std::size_t d = 0 ; d < sc_dim ; ++d ) {
    const double dd = dss->get_scenario_value( f_pool_map[ i ] , d )
                    - dss->get_scenario_value( f_pool_map[ j ] , d );
    d2 += dd * dd;
    }
   const double dist = std::sqrt( d2 );
   f_dist[ i ][ j ] = dist;
   f_dist[ j ][ i ] = dist;
   }

 reduced_atoms.assign( nb_atoms , false );
 ind_red.clear();
 indices_to_choose.clear();
 f_solution_value = 0.0;
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- compute -------------------------------------*/
/*--------------------------------------------------------------------------*/

int ScenarioReductionSolver::compute( bool /*changedvars*/ )
{
 std::lock_guard< std::recursive_mutex > lock( f_mutex );

 if( ! get_Block() )
  return kError;

 if( f_K <= 0 || static_cast< Index >( f_K ) > nb_atoms )
  throw std::logic_error(
   "ScenarioReductionSolver::compute: call set_nb_reduced(K) with "
   "0 < K <= N before compute()" );

 mod_clear();

 nb_reduced = static_cast< Index >( f_K );
 indices_to_choose.clear();
 ind_red.clear();
 std::fill( reduced_atoms.begin() , reduced_atoms.end() , false );

 switch( f_algo ) {
  case Algorithm::Baseline: return compute_baseline();
  case Algorithm::Dupacova: return compute_dupacova();
  case Algorithm::BestFit:  [[fallthrough]];
  case Algorithm::FirstFit: return compute_local_search();
  default:                  return kError;
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------- get_var_solution -------------------------------*/
/*--------------------------------------------------------------------------*/

void ScenarioReductionSolver::get_var_solution( Configuration * /*solc*/ )
{
 std::lock_guard< std::recursive_mutex > lock( f_mutex );

 auto * srb = dynamic_cast< ScenarioReductionBlock * >( get_Block() );
 if( ! srb )
  throw std::logic_error(
   "ScenarioReductionSolver::get_var_solution: no ScenarioReductionBlock" );
 if( ind_red.empty() )
  throw std::logic_error(
   "ScenarioReductionSolver::get_var_solution: no solution available" );

 ScenarioReductionBlockSolution sol;
 sol.selected_indices.reserve( ind_red.size() );
 for( auto r : ind_red )
  sol.selected_indices.push_back( f_pool_map[ r ] );

 sol.assignments.resize( nb_atoms );
 sol.weights.assign( ind_red.size() , 0.0 );

 for( Index i = 0 ; i < nb_atoms ; ++i ) {
  // nearest representative (relative index) by distance
  Index   best_rel  = ind_red[ 0 ];
  double  best_cost = f_dist[ i ][ best_rel ];
  for( std::size_t r = 1 ; r < ind_red.size() ; ++r ) {
   const double c = f_dist[ i ][ ind_red[ r ] ];
   if( c < best_cost ) { best_cost = c; best_rel = ind_red[ r ]; }
   }
  sol.assignments[ i ] = f_pool_map[ best_rel ];

  // aggregate weight onto that representative
  const Index rep_abs = f_pool_map[ best_rel ];
  auto it = std::find( sol.selected_indices.begin() ,
                       sol.selected_indices.end() , rep_abs );
  if( it != sol.selected_indices.end() )
   sol.weights[ std::distance( sol.selected_indices.begin() , it ) ]
    += f_weights[ i ];
  }

 srb->set_solution( sol );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- ALGORITHM IMPLEMENTATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

int ScenarioReductionSolver::compute_dupacova()
{
 const int m = static_cast< int >( nb_reduced );
 const int n = static_cast< int >( nb_atoms );

 indices_to_choose.resize( n );
 std::iota( indices_to_choose.begin() , indices_to_choose.end() , 0 );

 std::vector< double > minimum_d( n ,
  std::numeric_limits< double >::infinity() );

 for( int k = 0 ; k < m ; ++k ) {
  auto [ j_best , j_tmp ] = pick_dupacova( minimum_d );
  for( int i = 0 ; i < n ; ++i )
   minimum_d[ i ] = std::min( minimum_d[ i ] ,
    f_dist[ i ][ j_best ] );
  reduced_atoms[ j_best ] = true;
  ind_red.push_back( static_cast< Index >( j_best ) );
  indices_to_choose.erase( indices_to_choose.begin() + j_tmp );
  }

 f_solution_value = std::inner_product(
  minimum_d.begin() , minimum_d.end() , f_weights.begin() , 0.0 );
 return kOK;
 }

/*--------------------------------------------------------------------------*/

int ScenarioReductionSolver::compute_baseline()
{
 std::vector< std::pair< double , Index > > wp;
 wp.reserve( nb_atoms );
 for( Index i = 0 ; i < nb_atoms ; ++i )
  wp.emplace_back( f_weights[ i ] , i );

 std::sort( wp.begin() , wp.end() ,
  []( const auto & a , const auto & b ) { return a.first > b.first; } );

 std::fill( reduced_atoms.begin() , reduced_atoms.end() , false );
 ind_red.clear();
 for( Index i = 0 ; i < nb_reduced && i < nb_atoms ; ++i ) {
  reduced_atoms[ wp[ i ].second ] = true;
  ind_red.push_back( wp[ i ].second );
  }

 std::vector< double > min_d( nb_atoms ,
  std::numeric_limits< double >::infinity() );
 for( Index i = 0 ; i < nb_atoms ; ++i )
  for( Index j = 0 ; j < nb_atoms ; ++j )
   if( reduced_atoms[ j ] )
    min_d[ i ] = std::min( min_d[ i ] , f_dist[ i ][ j ] );

 f_solution_value = std::inner_product(
  min_d.begin() , min_d.end() , f_weights.begin() , 0.0 );
 return kOK;
 }

/*--------------------------------------------------------------------------*/

int ScenarioReductionSolver::compute_local_search()
{
 double curr_d = init_local_search();

 bool improvement = true;
 while( improvement ) {
  int i , j;
  double trial_d;
  if( f_algo == Algorithm::BestFit ) {
   auto [ a , b , c ] = pick_ij_bestfit( curr_d );
   i = static_cast< int >( a ); j = static_cast< int >( b ); trial_d = c;
   }
  else {  // FirstFit
   auto [ a , b , c ] = pick_ij_firstfit( curr_d );
   i = static_cast< int >( a ); j = static_cast< int >( b ); trial_d = c;
   }

  if( i >= 0 && j >= 0 && trial_d < curr_d ) {
   curr_d = trial_d;
   swap_indices( static_cast< Index >( i ) , static_cast< Index >( j ) );
   }
  else
   improvement = false;
  }

 update_reduced_atoms();
 f_solution_value = curr_d;
 return kOK;
 }

/*--------------------------------------------------------------------------*/

double ScenarioReductionSolver::init_local_search()
{
 const int n = static_cast< int >( nb_atoms );
 const int m = static_cast< int >( nb_reduced );

 // random initial selection (deterministic via the seeded RNG)
 std::vector< Index > shuffled( n );
 std::iota( shuffled.begin() , shuffled.end() , 0 );
 std::shuffle( shuffled.begin() , shuffled.end() , f_rng );
 ind_red.assign( shuffled.begin() , shuffled.begin() + m );
 std::sort( ind_red.begin() , ind_red.end() );

 indices_to_choose.clear();
 for( int i = 0 ; i < n ; ++i )
  if( std::find( ind_red.begin() , ind_red.end() ,
                 static_cast< Index >( i ) ) == ind_red.end() )
   indices_to_choose.push_back( static_cast< Index >( i ) );

 update_reduced_atoms();

 std::vector< double > min_d( n ,
  std::numeric_limits< double >::infinity() );
 for( int i = 0 ; i < n ; ++i )
  for( auto j : ind_red )
   min_d[ i ] = std::min( min_d[ i ] , f_dist[ i ][ j ] );

 return std::inner_product(
  min_d.begin() , min_d.end() , f_weights.begin() , 0.0 );
 }

/*--------------------------------------------------------------------------*/

std::tuple< int , int > ScenarioReductionSolver::pick_dupacova(
 const std::vector< double > & min_cost )
{
 const int n = static_cast< int >( nb_atoms );
 std::vector< double > inner_min( n );
 std::vector< double > distances( indices_to_choose.size() );

 for( std::size_t idx = 0 ; idx < indices_to_choose.size() ; ++idx ) {
  const auto j = indices_to_choose[ idx ];
  for( int i = 0 ; i < n ; ++i )
   inner_min[ i ] = std::min( min_cost[ i ] , f_dist[ i ][ j ] );
  distances[ idx ] = std::inner_product(
   inner_min.begin() , inner_min.end() , f_weights.begin() , 0.0 );
  }

 auto min_it = std::min_element( distances.begin() , distances.end() );
 const int j_tmp = static_cast< int >(
  std::distance( distances.begin() , min_it ) );
 return { static_cast< int >( indices_to_choose[ j_tmp ] ) , j_tmp };
 }

/*--------------------------------------------------------------------------*/

std::tuple< ScenarioReductionSolver::Index ,
            ScenarioReductionSolver::Index , double >
ScenarioReductionSolver::pick_ij_bestfit( double /*curr_d*/ )
{
 std::vector< double > distances( ind_red.size() ,
  std::numeric_limits< double >::infinity() );
 std::unordered_map< Index , Index > j_map;

 for( std::size_t i = 0 ; i < ind_red.size() ; ++i ) {
  const Index atom = ind_red[ i ];
  ind_red.erase( ind_red.begin() + i );
  auto [ best_j , dist ] = bestfit_selection( ind_red );
  j_map[ atom ] = best_j;
  distances[ i ] = dist;
  ind_red.insert( ind_red.begin() + i , atom );
  }

 auto min_it = std::min_element( distances.begin() , distances.end() );
 if( min_it == distances.end() )
  return { static_cast< Index >( -1 ) , static_cast< Index >( -1 ) ,
           std::numeric_limits< double >::infinity() };

 const std::size_t best_idx = std::distance( distances.begin() , min_it );
 const Index best_i = ind_red[ best_idx ];
 const Index best_j = j_map[ best_i ];
 return { best_i , best_j , distances[ best_idx ] };
 }

/*--------------------------------------------------------------------------*/

std::pair< ScenarioReductionSolver::Index , double >
ScenarioReductionSolver::bestfit_selection(
 const std::vector< Index > & curr_indices )
{
 const int n = static_cast< int >( nb_atoms );
 std::vector< double > min_on_red( n ,
  std::numeric_limits< double >::infinity() );
 for( int i = 0 ; i < n ; ++i )
  for( auto j : curr_indices )
   min_on_red[ i ] = std::min( min_on_red[ i ] , f_dist[ i ][ j ] );

 Index  best_j    = static_cast< Index >( -1 );
 double best_dist = std::numeric_limits< double >::infinity();
 for( auto j : indices_to_choose ) {
  double obj = 0.0;
  for( int i = 0 ; i < n ; ++i )
   obj += std::min( min_on_red[ i ] , f_dist[ i ][ j ] ) * f_weights[ i ];
  if( obj < best_dist ) { best_dist = obj; best_j = j; }
  }
 return { best_j , best_dist };
 }

/*--------------------------------------------------------------------------*/

std::tuple< ScenarioReductionSolver::Index ,
            ScenarioReductionSolver::Index , double >
ScenarioReductionSolver::pick_ij_firstfit( double curr_d )
{
 if( f_shuffle )
  std::shuffle( ind_red.begin() , ind_red.end() , f_rng );

 for( auto i : ind_red ) {
  std::vector< Index > temp;
  temp.reserve( ind_red.size() - 1 );
  for( auto idx : ind_red )
   if( idx != i ) temp.push_back( idx );
  auto [ j , dist ] = firstfit_selection( temp , curr_d );
  if( j != static_cast< Index >( -1 ) ) {
   if( f_shuffle ) std::sort( ind_red.begin() , ind_red.end() );
   return { i , j , dist };
   }
  }

 if( f_shuffle )
  std::sort( ind_red.begin() , ind_red.end() );
 return { static_cast< Index >( -1 ) , static_cast< Index >( -1 ) ,
          std::numeric_limits< double >::infinity() };
 }

/*--------------------------------------------------------------------------*/

std::pair< ScenarioReductionSolver::Index , double >
ScenarioReductionSolver::firstfit_selection(
 const std::vector< Index > & curr_indices , double curr_d )
{
 const int n = static_cast< int >( nb_atoms );
 std::vector< double > min_on_red( n ,
  std::numeric_limits< double >::infinity() );
 for( int i = 0 ; i < n ; ++i )
  for( auto j : curr_indices )
   min_on_red[ i ] = std::min( min_on_red[ i ] , f_dist[ i ][ j ] );

 for( auto j : indices_to_choose ) {
  double trial_d = 0.0;
  for( int i = 0 ; i < n ; ++i )
   trial_d += std::min( min_on_red[ i ] , f_dist[ i ][ j ] ) * f_weights[ i ];
  if( trial_d < curr_d )
   return { j , trial_d };
  }
 return { static_cast< Index >( -1 ) ,
          std::numeric_limits< double >::infinity() };
 }

/*--------------------------------------------------------------------------*/

void ScenarioReductionSolver::swap_indices( Index i , Index j )
{
 auto it_i = std::find( ind_red.begin() , ind_red.end() , i );
 if( it_i != ind_red.end() ) ind_red.erase( it_i );
 auto it_j = std::find( indices_to_choose.begin() ,
                        indices_to_choose.end() , j );
 if( it_j != indices_to_choose.end() ) indices_to_choose.erase( it_j );
 ind_red.push_back( j );
 indices_to_choose.push_back( i );
 std::sort( ind_red.begin() , ind_red.end() );
 std::sort( indices_to_choose.begin() , indices_to_choose.end() );
 }

/*--------------------------------------------------------------------------*/

void ScenarioReductionSolver::update_reduced_atoms()
{
 std::fill( reduced_atoms.begin() , reduced_atoms.end() , false );
 for( auto i : ind_red )
  if( i < static_cast< Index >( reduced_atoms.size() ) )
   reduced_atoms[ i ] = true;
 }

/*--------------------------------------------------------------------------*/
/*------------- End File ScenarioReductionSolver.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
