/*--------------------------------------------------------------------------*/
/*------------- File CFLCSSCScenarioReductionTest.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Test CSSC scenario reduction on Two-Stage Stochastic CFL.
 *
 * ### Setup
 *
 *   base_cfl: real CFL instance (nf × nc), loaded from file
 *   stoch: StochasticBlock wrapping base_cfl, with DataMapping for
 *          customer demands so set_data(xi) -> base_cfl.chg_customer_demands
 *
 *   cssc.set_Block(sr_cfl)              -> base class reads N, K, weights, tcosts
 *   cssc.set_sub_problem_block(base_cfl)-> Step 1 solves on base_cfl
 *   cssc.set_ComputeConfig(cfg)         -> MILP solver + DiscreteScenarioSet
 *   cssc.compute()                      -> builds V matrix + solves MILP
 *
 * \author Minh Duc Pham \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 */
/*--------------------------------------------------------------------------*/

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "BlockSolverConfig.h"
#include "CapacitatedFacilityLocationBlock.h"
#include "Configuration.h"
#include "CSSCScenarioReductionSolver.h"
#include "DiscreteScenarioSet.h"
#include "DataMapping.h"
#include "Solver.h"
#include "StochasticBlock.h"
#include "TwoStageStochasticBlock.h"

using namespace std;
using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ Config ------------------------------------*/
/*--------------------------------------------------------------------------*/

struct Config {
 string instance_path;
 string scenario_file;
 string solver_config = "BSPar1.txt";
 int    full_scenarios = 0;
 int    reduced_k      = 5;
 int    verbose        = 1;
 bool   skip_full      = false;
};

Config parse_args( int argc , char * argv[] ) {
 Config cfg;
 for( int i = 1 ; i < argc ; ++i ) {
  string a = argv[i];
  if( a == "-h" ) {
   cout << "Usage: " << argv[0]
        << " -i <nc4> [-f <scen>] [-c <cfg>] [-n N] [-k K] [-v V]"
           " [--skip-full]\n";
   exit(0);
  }
  else if( a=="-i" && i+1<argc ) cfg.instance_path  = argv[++i];
  else if( a=="-f" && i+1<argc ) cfg.scenario_file  = argv[++i];
  else if( a=="-c" && i+1<argc ) cfg.solver_config  = argv[++i];
  else if( a=="-n" && i+1<argc ) cfg.full_scenarios = stoi(argv[++i]);
  else if( a=="-k" && i+1<argc ) cfg.reduced_k      = stoi(argv[++i]);
  else if( a=="-v" && i+1<argc ) cfg.verbose        = stoi(argv[++i]);
  else if( a == "--skip-full" )  cfg.skip_full       = true;
  else { cerr << "Unknown: " << a << "\n"; exit(1); }
 }
 if( cfg.instance_path.empty() ) { cerr << "Error: -i required\n"; exit(1); }
 return cfg;
}

/*--------------------------------------------------------------------------*/
/*--------------------- build_tss / solve_tss ------------------------------*/
/*--------------------------------------------------------------------------*/

unique_ptr< TwoStageStochasticBlock > build_tss(
  CapacitatedFacilityLocationBlock * base_cfl ,
  DiscreteScenarioSet * dss ,
  int nf , int nc )
{
 const string tmp = "tmp_cssc_tss.nc4";
 {
  netCDF::NcFile f( tmp , netCDF::NcFile::replace );
  auto tg = f.addGroup( "TwoStageStochasticBlock" );
  tg.addDim( "NumberScenarios" , dss->get_poolSize() );
  tg.putAtt( "type" , "TwoStageStochasticBlock" );
  { auto g = tg.addGroup( "FirstStageBlock" ); base_cfl->serialize( g ); }

  auto sg = tg.addGroup( "StaticAbstractPath" );
  auto dp = sg.addDim( "PathDim" , nf );
  auto dl = sg.addDim( "PathTotalLength" , nf );
  vector< unsigned int > idx( nf );
  for( int i = 0 ; i < nf ; i++ ) idx[i] = i;
  sg.addVar( "PathStart"          , netCDF::NcUint() , dp ).putVar( idx.data() );
  sg.addVar( "PathNodeTypes"      , netCDF::NcChar() , dl ).putVar( vector<char>(nf,'V').data() );
  sg.addVar( "PathGroupIndices"   , netCDF::NcUint() , dl ).putVar( vector<unsigned int>(nf,0).data() );
  sg.addVar( "PathElementIndices" , netCDF::NcUint() , dl ).putVar( idx.data() );
  sg.addVar( "PathRangeIndices"   , netCDF::NcUint() , dl ).putVar( idx.data() );

  auto sb = tg.addGroup( "StochasticBlock" );
  sb.putAtt( "type" , "StochasticBlock" );
  { auto g = sb.addGroup( "Block" ); base_cfl->serialize( g ); }
  { auto g = tg.addGroup( "DiscreteScenarioSet" ); dss->serialize( g ); }

  auto nd = sb.addDim( "NumberDataMappings" , 1 );
  char dc = 'D'; sb.addVar( "DataType"     , netCDF::NcChar()   , nd ).putVar( &dc );
  char cc = 'B'; sb.addVar( "Caller"       , netCDF::NcChar()   , nd ).putVar( &cc );
  string fn = "CapacitatedFacilityLocationBlock::chg_customer_demands";
  sb.addVar( "FunctionName" , netCDF::NcString() , nd ).putVar( {0} , &fn );
  auto ds = sb.addDim( "SetSizeDim" , 2 );
  vector<unsigned int> ss = {0,0};
  sb.addVar( "SetSize" , netCDF::NcUint() , ds ).putVar( ss.data() );
  unsigned char ord = 0;
  sb.addVar( "Ordered" , netCDF::NcUbyte() , nd ).putVar( &ord );
  auto de = sb.addDim( "SetElementsDim" , 4 );
  vector<unsigned int> se = {0,(unsigned int)nc,0,(unsigned int)nc};
  sb.addVar( "SetElements" , netCDF::NcUint() , de ).putVar( se.data() );

  auto ap = sb.addGroup( "AbstractPath" );
  ap.addDim( "PathDim" , 1 );
  auto zd = ap.addDim( "PathTotalLength" , 0 );
  unsigned int ps = 0;
  ap.addVar( "PathStart"          , netCDF::NcUint() , ap.getDim("PathDim") ).putVar( &ps );
  ap.addVar( "PathNodeTypes"      , netCDF::NcChar()  , zd );
  ap.addVar( "PathGroupIndices"   , netCDF::NcUint()  , zd );
  ap.addVar( "PathElementIndices" , netCDF::NcUint()  , zd );
  ap.addVar( "PathRangeIndices"   , netCDF::NcUint()  , zd );
 }
 netCDF::NcFile f2( tmp , netCDF::NcFile::read );
 auto tss = make_unique< TwoStageStochasticBlock >();
 tss->deserialize( f2.getGroup( "TwoStageStochasticBlock" ) );
 f2.close();
 remove( tmp.c_str() );
 return tss;
}

/*--------------------------------------------------------------------------*/

tuple< double , long long , bool > solve_tss(
  TwoStageStochasticBlock * tss , const string & cfg_file )
{
 auto * raw = Configuration::deserialize( cfg_file );
 auto * bsc = dynamic_cast< BlockSolverConfig * >( raw );
 if( ! bsc ) { delete raw; throw runtime_error( "Bad solver config" ); }
 bsc->apply( tss );
 delete bsc;
 auto & sl = tss->get_registered_solvers();
 if( sl.empty() ) throw runtime_error( "No solver attached" );
 auto t0 = chrono::high_resolution_clock::now();
 int st = sl.front()->compute( false );
 auto dt = chrono::duration_cast< chrono::microseconds >(
   chrono::high_resolution_clock::now() - t0 ).count();
 if( st == Solver::kOK || st == Solver::kLowPrecision )
  return { sl.front()->get_ub() , dt , true };
 return { 0.0 , dt , false };
}

/*--------------------------------------------------------------------------*/
/*--------------------------------- main -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] ) {

 cout << "\nTwo-Stage Stochastic CFL — CSSC Scenario Reduction\n\n";

 Config cfg = parse_args( argc , argv );

 try {

  // ------------------------------------------------------------------
  // 1. Load base_cfl (real CFL instance)
  // ------------------------------------------------------------------
  if( cfg.verbose >= 1 ) cout << "[1/6] Loading CFL instance\n";

  auto * raw = Block::deserialize( cfg.instance_path );
  auto * base_cfl = dynamic_cast< CapacitatedFacilityLocationBlock * >( raw );
  if( ! base_cfl ) { delete raw; throw runtime_error( "Not a CFL block" ); }
  if( ! base_cfl->get_UnSplittable() ) base_cfl->chg_UnSplittable( true );

  const int nf = base_cfl->get_NFacilities();
  const int nc = base_cfl->get_NCustomers();

  if( cfg.verbose >= 1 )
   cout << "  " << nf << " facilities, " << nc << " customers\n\n";

  // ------------------------------------------------------------------
  // 2. Load scenarios
  // ------------------------------------------------------------------
  if( cfg.verbose >= 1 ) cout << "[2/6] Loading scenarios\n";

  string sf = cfg.scenario_file;
  if( sf.empty() ) {
   filesystem::path p( cfg.instance_path );
   sf = "../scenarios/CFL/" + p.stem().string() + "_scenarios.nc4";
  }
  netCDF::NcFile scf( sf , netCDF::NcFile::read );
  auto dss = make_unique< DiscreteScenarioSet >();
  dss->deserialize( scf );
  scf.close();

  const int N = cfg.full_scenarios > 0
    ? min( cfg.full_scenarios , (int)dss->get_nbScenarios() )
    : (int)dss->get_nbScenarios();
  dss->init_representative_pool( N );

  const int K = cfg.reduced_k;
  if( K >= N ) throw runtime_error( "K must be < N" );

  if( cfg.verbose >= 1 )
   cout << "  N=" << N << ", K=" << K
        << ", dim=" << dss->get_scenario_size() << "\n\n";

  // ------------------------------------------------------------------
  // 3. Solve FULL TSS (optional)
  // ------------------------------------------------------------------
  double    obj_full = 0.0;
  long long t_full   = 0;

  if( ! cfg.skip_full ) {
   if( cfg.verbose >= 1 )
    cout << "[3/6] Solving full TSS (" << N << " scenarios)\n";
   auto tss_full = build_tss( base_cfl , dss.get() , nf , nc );
   auto [_obj,_t,ok] = solve_tss( tss_full.get() , cfg.solver_config );
   if( ! ok ) throw runtime_error( "Full solve failed" );
   obj_full = _obj;  t_full = _t;
   if( cfg.verbose >= 1 )
    cout << "  Objective = " << fixed << setprecision(2) << obj_full
         << "  (" << t_full << " us)\n\n";
  }

  // ------------------------------------------------------------------
  // 4. CSSC reduction
  // ------------------------------------------------------------------
  if( cfg.verbose >= 1 )
   cout << "[4/6] CSSC reduction (" << N << " -> " << K << ")\n";

  // 4a. Build sr_cfl (N×N synthetic block)
  //
  //   Role: satisfies ScenarioReductionSolver::set_Block() which reads:
  //     nb_atoms   = NCustomers     = N
  //     nb_reduced = NMaxFacilities = K
  //     weights    = demands        = 1/N
  //     f_transportation_costs      = Euclidean distances (for Wasserstein)
  //
  //   This block is NEVER solved. All Step 1 sub-problems run on base_cfl.
  auto * sr_cfl = new CapacitatedFacilityLocationBlock();
  {
   using DV = CapacitatedFacilityLocationBlock::DVector;
   using CV = CapacitatedFacilityLocationBlock::CVector;
   using CM = CapacitatedFacilityLocationBlock::CMatrix;

   const size_t D = dss->get_scenario_size();

   DV caps( N , 1.0 );
   CV fcosts( N , 0.0 );
   DV dems( N , 1.0 / N );

   CM tcosts( boost::extents[N][N] );
   for( int i = 0 ; i < N ; ++i )
    for( int j = 0 ; j < N ; ++j ) {
     double d2 = 0.0;
     for( size_t d = 0 ; d < D ; ++d ) {
      const double dd = dss->get_scenario_value(i,d) - dss->get_scenario_value(j,d);
      d2 += dd * dd;
     }
     tcosts[i][j] = sqrt(d2);
    }

   sr_cfl->load( N , N , caps , fcosts , dems , tcosts , true , K );
  }

  // 4b. Wrap base_cfl in StochasticBlock for scenario injection.
  //
  //   compute_V_matrix() does: f_sub_block->get_f_Block() -> StochasticBlock
  //   So stoch must be the parent of base_cfl.
  //   DataMapping: stoch->set_data(Xi) -> base_cfl->chg_customer_demands(Xi)
  auto * stoch = new StochasticBlock();
  stoch->set_inner_block( base_cfl , false );

  using SDM = SimpleDataMapping< Block::Range , Block::Range , double , Block >;
  const SDM::F * fptr = Block::get_method< SDM::F >(
    "CapacitatedFacilityLocationBlock::chg_customer_demands" );
  if( ! fptr )
   throw runtime_error( "chg_customer_demands not found in Block factory" );
  stoch->add_data_mapping( make_unique< SDM >(
    fptr , base_cfl ,
    Block::Range( 0 , nc ) ,
    Block::Range( 0 , nc ) ) );

  // Make stoch the parent of base_cfl
  base_cfl->set_f_Block( stoch );

  // 4c. Build CSSCComputeConfig
  //   f_extra_Configuration = LP relaxation config for Step 1 sub-problems
  //   f_milp_config         = MIP config for Step 2 partitioning MILP
  auto * cssc_cfg = new CSSCComputeConfig();
  {
   // Step 1: LP relaxation (intRelaxIntVars=1) — fast LP solves
   auto * lp_raw = Configuration::deserialize( "BSPar_CPLEX_LP.txt" );
   auto * lp_bsc = dynamic_cast< BlockSolverConfig * >( lp_raw );
   if( ! lp_bsc ) {
    // Fallback: use same config as Step 2 if LP config not found
    delete lp_raw;
    lp_raw = Configuration::deserialize( cfg.solver_config );
    lp_bsc = dynamic_cast< BlockSolverConfig * >( lp_raw );
    if( ! lp_bsc ) { delete lp_raw; throw runtime_error( "Bad LP config" ); }
   }
   cssc_cfg->f_extra_Configuration = lp_bsc;
  }
  {
   // Step 2: full MIP config
   auto * milp_raw = Configuration::deserialize( cfg.solver_config );
   auto * milp_bsc = dynamic_cast< BlockSolverConfig * >( milp_raw );
   if( ! milp_bsc ) { delete milp_raw; throw runtime_error( "Bad MILP config" ); }
   cssc_cfg->f_milp_config  = milp_bsc;
  }
  cssc_cfg->f_scenario_set = dss.get();

  // 4d. Configure and run CSSC
  //
  //   set_Block(sr_cfl)               -> nb_atoms=N, nb_reduced=K
  //   set_sub_problem_block(base_cfl) -> Step 1 attaches solver here
  //   set_ComputeConfig(cssc_cfg)     -> MILP config + scenario set
  //   compute()                       -> Step 1 (V matrix) + Step 2 (MILP)
  CSSCScenarioReductionSolver cssc;
  cssc.set_Block( sr_cfl );
  cssc.set_sub_problem_block( base_cfl );
  cssc.set_ComputeConfig( cssc_cfg );
  delete cssc_cfg;

  if( cfg.verbose >= 2 ) cssc.set_log( &cout );

  if( cfg.verbose >= 1 )
   cout << "  nb_atoms="   << cssc.get_nb_atoms()
        << "  nb_reduced=" << cssc.get_nb_reduced()
        << "\n  Step 1: " << N*N << " sub-problem solves...\n";

  auto t0c = chrono::high_resolution_clock::now();
  const int cst = cssc.compute();
  const long long t_cssc = chrono::duration_cast< chrono::microseconds >(
    chrono::high_resolution_clock::now() - t0c ).count();

  if( cst != Solver::kOK )
   throw runtime_error( "CSSC failed (status=" + to_string(cst) + ")" );

  // 4e. Extract representatives from ind_red
  const auto & ind = cssc.get_ind_red();
  vector< int > sel( ind.begin() , ind.end() );

  if( cfg.verbose >= 1 ) {
   cout << "  CSSC done in " << t_cssc << " us\n  Selected:";
   for( int r : sel ) cout << " " << r;
   cout << "\n\n";
  }

  // ------------------------------------------------------------------
  // 5. Build reduced scenario set and solve reduced TSS
  //
  //   Use cluster assignment from Step 2 MILP (f_scenario_assignment),
  //   which is consistent with the CSSC cost-space objective.
  // ------------------------------------------------------------------
  if( cfg.verbose >= 1 )
   cout << "[5/6] Solving reduced TSS (" << K << " scenarios)\n";

  const size_t D = dss->get_scenario_size();
  vector< double > agg( N , 0.0 );
  {
   const double w = 1.0 / N;
   const auto & assignment = cssc.f_scenario_assignment;
   for( int i = 0 ; i < N ; ++i )
    agg[ assignment[i] ] += w;
  }

  vector< vector< double > > red_sc;
  vector< double >           red_pr;
  red_sc.reserve( K );  red_pr.reserve( K );
  for( int r : sel ) {
   vector< double > sc( D );
   for( size_t d = 0 ; d < D ; ++d ) sc[d] = dss->get_scenario_value( r , d );
   red_sc.push_back( sc );
   red_pr.push_back( agg[r] );
  }
  double tot = 0.0;
  for( double p : red_pr ) tot += p;
  for( double & p : red_pr ) p /= tot;

  auto dss_red = make_unique< DiscreteScenarioSet >();
  dss_red->load_from_memory( red_sc , red_pr );
  dss_red->init_representative_pool( K );

  auto tss_red = build_tss( base_cfl , dss_red.get() , nf , nc );
  auto [obj_red,t_red,ok_red] = solve_tss( tss_red.get() , cfg.solver_config );
  if( ! ok_red ) throw runtime_error( "Reduced solve failed" );

  if( cfg.verbose >= 1 )
   cout << "  Objective = " << fixed << setprecision(2) << obj_red
        << "  (" << t_red << " us)\n\n";

  // ------------------------------------------------------------------
  // 6. Report
  // ------------------------------------------------------------------
  cout << "[6/6] Results\n"
       << "  N = " << N << "  K = " << K << "\n\n"
       << "  Timing\n"
       << "    CSSC reduction : " << t_cssc << " us\n";
  if( ! cfg.skip_full )
   cout << "    Full solve     : " << t_full << " us\n";
  cout << "    Reduced solve  : " << t_red << " us";
  if( ! cfg.skip_full )
   cout << "  (speedup " << fixed << setprecision(1)
        << (double)t_full / max(1LL,t_red) << "x)";
  cout << "\n\n  Objectives\n";
  if( ! cfg.skip_full ) {
   const double diff = abs( obj_red - obj_full );
   const double gap  = diff / abs( obj_full ) * 100.0;
   cout << "    Full    : " << fixed << setprecision(2) << obj_full << "\n"
        << "    Reduced : " << fixed << setprecision(2) << obj_red  << "\n"
        << "    Gap     : " << setprecision(2) << diff
        << " (" << gap << "%)\n\n";
  } else {
   cout << "    Reduced : " << fixed << setprecision(2) << obj_red << "\n\n";
  }

  // Cleanup
  // Order matters: clear base_cfl's parent pointer before deleting stoch
  // to avoid dangling pointer in stoch's destructor.
  base_cfl->set_f_Block( nullptr );
  sr_cfl->set_f_Block( nullptr );
  delete stoch;
  delete sr_cfl;
  delete base_cfl;

  return 0;

 } catch( const exception & e ) {
  cerr << "\nError: " << e.what() << "\n"
       << "Tips: run CFLScenarioGenerator first; BSPar1.txt in cwd; K < N\n";
  return 1;
 }
}

/*--------------------------------------------------------------------------*/
/*----------- End File CFLCSSCScenarioReductionTest.cpp --------------------*/
/*--------------------------------------------------------------------------*/