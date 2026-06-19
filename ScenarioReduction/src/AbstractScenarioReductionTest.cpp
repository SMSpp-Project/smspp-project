/*--------------------------------------------------------------------------*/
/*---------------- File AbstractScenarioReductionTest.cpp -----------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of AbstractScenarioReductionTest.
 *
 * \author Benoît Tran \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 */
/*--------------------------------------------------------------------------*/

#include "AbstractScenarioReductionTest.h"
#include "CSSCScenarioReductionSolver.h"
#include "ScenarioReductionSolver.h"

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "Configuration.h"
#include "Solver.h"

using namespace std;
using namespace SMSpp_di_unipi_it;
using namespace ScenarioReductionTesting;

/*--------------------------------------------------------------------------*/
/*----------------------- CONSTRUCTOR / DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

AbstractScenarioReductionTest::AbstractScenarioReductionTest( ) {
 config = new ComputeConfig( );

 config->set_par( std::string( "intLogVerb" )             , 0    );
 config->set_par( std::string( "dblMaxTime" )             , -1.0 );
 config->set_par( std::string( "intFullNumScenarios" )    , 0    );
 config->set_par( std::string( "intReducedNumScenarios" ) , 3    );
 config->set_par( std::string( "intUseWarmstart" )        , 0    );
 config->set_par( std::string( "intUseShuffle" )          , 0    );
 config->set_par( std::string( "intSaveResults" )         , 0    );
 config->set_par( std::string( "intLoadResults" )         , 0    );
 config->set_par( std::string( "intComputeVPI" )          , 0    );
 config->set_par( std::string( "intSkipFull" )            , 0    );

 config->set_par( std::string( "strReductionMethod" ) , std::string( "dupacova"        ) );
 config->set_par( std::string( "strInstancePath" )    , std::string( ""                ) );
 config->set_par( std::string( "strScenarioFile" )    , std::string( ""                ) );
 config->set_par( std::string( "strCacheDir" )        , std::string( "./cache/"        ) );
 config->set_par( std::string( "strSolverConfig" )    , std::string( "BSPar_HiGHS.txt" ) );
}

/*--------------------------------------------------------------------------*/

AbstractScenarioReductionTest::~AbstractScenarioReductionTest( ) {
 delete config;
 config = nullptr;
}

/*--------------------------------------------------------------------------*/
/*----------------------- CONFIG HELPERS -----------------------------------*/
/*--------------------------------------------------------------------------*/

int AbstractScenarioReductionTest::get_int_config(
  const std::string & name ) const {
 for( const auto & p : config->int_pars )
  if( p.first == name ) return p.second;
 return 0;
}

double AbstractScenarioReductionTest::get_dbl_config(
  const std::string & name ) const {
 for( const auto & p : config->dbl_pars )
  if( p.first == name ) return p.second;
 return 0.0;
}

std::string AbstractScenarioReductionTest::get_str_config(
  const std::string & name ) const {
 for( const auto & p : config->str_pars )
  if( p.first == name ) return p.second;
 return "";
}

/*--------------------------------------------------------------------------*/
/*--------------------------------- run ------------------------------------*/
/*--------------------------------------------------------------------------*/

int AbstractScenarioReductionTest::run( int argc , char * argv[] ) {
 try {
  parse_arguments( argc , argv );
  print_configuration( );

  load( );

  if( ! get_int_config( "intSkipFull" ) )
   solve_stochastic_problem( );

  solve_anticipative( );

  solve_scenario_reduction( );

  solve_stochastic_problem( );

  solve_anticipative( );

  save_solutions_cache( );

  print_results( );

  return 0;

 } catch( const exception & e ) {
  cerr << "Error: " << e.what( ) << endl;
  return 1;
 }
}

/*--------------------------------------------------------------------------*/
/*------------------------ CONFIGURATION METHODS --------------------------*/
/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::parse_arguments( int argc , char * argv[] )
{
 for( int i = 1 ; i < argc ; ++i ) {
  string arg = argv[ i ];

  if( arg == "-h" || arg == "--help" ) {
   print_help( argv[ 0 ] );
   exit( 0 );
  }
  else if( arg == "-i" || arg == "--instance" ) {
   if( i + 1 < argc )
    config->set_par( std::string("strInstancePath") , std::string(argv[ ++i ]) );
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-f" || arg == "--scenario-file" ) {
   if( i + 1 < argc )
    config->set_par( std::string("strScenarioFile") , std::string(argv[ ++i ]) );
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-v" || arg == "--verbose" ) {
   if( i + 1 < argc ) {
    try {
     int verbose = stoi( argv[ ++i ] );
     if( verbose < 0 || verbose > 2 ) {
      cerr << "Verbose level must be 0, 1, or 2" << endl; exit( 1 );
     }
     config->set_par( std::string("intLogVerb") , verbose );
    } catch( const exception & e ) {
     cerr << "Invalid verbose level: " << argv[ i ] << endl; exit( 1 );
    }
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-t" || arg == "--time" ) {
   if( i + 1 < argc ) {
    try {
     double time_limit = stod( argv[ ++i ] );
     if( time_limit <= 0 ) {
      cerr << "Time limit must be positive" << endl; exit( 1 );
     }
     config->set_par( std::string("dblMaxTime") , time_limit );
    } catch( const exception & e ) {
     cerr << "Invalid time limit: " << argv[ i ] << endl; exit( 1 );
    }
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-n" || arg == "--full-scenarios" ) {
   if( i + 1 < argc ) {
    try {
     int full_num = stoi( argv[ ++i ] );
     if( full_num <= 0 ) {
      cerr << "Number of full scenarios must be positive" << endl; exit( 1 );
     }
     config->set_par( std::string("intFullNumScenarios") , full_num );
    } catch( const exception & e ) {
     cerr << "Invalid number of full scenarios: " << argv[ i ] << endl; exit( 1 );
    }
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-r" || arg == "--reduced" ) {
   if( i + 1 < argc ) {
    try {
     int reduced_num = stoi( argv[ ++i ] );
     if( reduced_num <= 0 ) {
      cerr << "Number of reduced scenarios must be positive" << endl; exit( 1 );
     }
     config->set_par( std::string("intReducedNumScenarios") , reduced_num );
    } catch( const exception & e ) {
     cerr << "Invalid number of reduced scenarios: " << argv[ i ] << endl; exit( 1 );
    }
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-m" || arg == "--method" ) {
   if( i + 1 < argc ) {
    string method = argv[ ++i ];
    if( method != "baseline" && method != "dupacova" && method != "bestfit" &&
        method != "firstfit" && method != "milp"     && method != "cssc" ) {
     cerr << "Unknown reduction method: " << method << endl;
     cerr << "Valid methods: baseline, dupacova, bestfit, firstfit, milp, cssc" << endl;
     exit( 1 );
    }
    config->set_par( std::string("strReductionMethod") , std::move(method) );
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-w" || arg == "--warmstart" ) {
   if( i + 1 < argc ) {
    string val = argv[ ++i ];
    config->set_par( std::string("intUseWarmstart") ,
                     (val == "1" || val == "true") ? 1 : 0 );
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-c" || arg == "--solver-config" ) {
   if( i + 1 < argc )
    config->set_par( std::string("strSolverConfig") , std::string(argv[ ++i ]) );
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-S" || arg == "--shuffle" ) {
   if( i + 1 < argc ) {
    string val = argv[ ++i ];
    config->set_par( std::string("intUseShuffle") ,
                     (val == "1" || val == "true") ? 1 : 0 );
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-d" || arg == "--cache-dir" ) {
   if( i + 1 < argc ) {
    string cache_dir = argv[ ++i ];
    if( ! cache_dir.empty( ) && cache_dir.back( ) != '/' ) cache_dir += '/';
    config->set_par( std::string("strCacheDir") , std::move(cache_dir) );
   }
   else { cerr << "Missing value after " << arg << endl; exit( 1 ); }
  }
  else if( arg == "-s" || arg == "--save-results" )
   config->set_par( std::string("intSaveResults") , 1 );
  else if( arg == "-L" || arg == "--load-results" )
   config->set_par( std::string("intLoadResults") , 1 );
  else if( arg == "--skip-full" )
   config->set_par( std::string("intSkipFull") , 1 );
  else if( arg == "-p" || arg == "--vpi" )
   config->set_par( std::string("intComputeVPI") , 1 );
 }

 if( get_str_config( "strInstancePath" ).empty( ) ) {
  cerr << "Error: Instance path is required (-i or --instance)" << endl;
  print_help( argv[ 0 ] );
  exit( 1 );
 }
}

/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::print_help( const char * program_name ) {
 cout << "Usage: " << program_name << " [options]" << endl;
 cout << "\nGeneral Options:" << endl;
 cout << "  -i, --instance <path>      Path to problem instance file (required)" << endl;
 cout << "  -f, --scenario-file <path> Load pre-generated scenarios from file" << endl;
 cout << "  -v, --verbose <level>      Set verbosity level (0=silent, 1=normal, 2=detailed)" << endl;
 cout << "  -t, --time <seconds>       Set solver time limit in seconds" << endl;
 cout << "  -c, --solver-config <file> Solver config file (default: BSPar_HiGHS.txt)" << endl;
 cout << "  -h, --help                 Show this help message" << endl;
 cout << "\nScenario Options:" << endl;
 cout << "  -n, --full-scenarios <N>   Set number of full scenarios to use" << endl;
 cout << "                             (Default: use all from loaded file)" << endl;
 cout << "  -r, --reduced <number>     Set number of reduced scenarios (default: 3)" << endl;
 cout << "\nReduction Method Options:" << endl;
 cout << "  -m, --method <name>        Scenario reduction method:" << endl;
 cout << "                             baseline, dupacova (default), bestfit, firstfit, milp, cssc" << endl;
 cout << "  -w, --warmstart <0|1>      Use warm start for local search (0=false, 1=true)" << endl;
 cout << "  -S, --shuffle <0|1>        Enable shuffling for FirstFit (0=false, 1=true)" << endl;
 cout << "\nResults Cache Options:" << endl;
 cout << "  -s, --save-results         Save solution results to cache files" << endl;
 cout << "  -L, --load-results         Load pre-computed results from cache" << endl;
 cout << "  -d, --cache-dir <path>     Specify cache directory (default: ./cache/)" << endl;
 cout << "\nAnalysis Options:" << endl;
 cout << "  --skip-full                Skip solving the full N-scenario problem" << endl;
 cout << "  -p, --vpi                  Compute Value of Perfect Information" << endl;
}

/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::print_configuration( ) {
 if( get_int_config( "intLogVerb" ) >= 1 ) {
  cout << "\nTest Configuration" << endl;
  cout << "  Instance.........: " << get_str_config( "strInstancePath" ) << endl;
  if( ! get_str_config( "strScenarioFile" ).empty( ) )
   cout << "  Scenario file...: " << get_str_config( "strScenarioFile" ) << endl;
  if( get_int_config( "intFullNumScenarios" ) > 0 )
   cout << "  Full scenarios..: " << get_int_config( "intFullNumScenarios" ) << endl;
  cout << "  Reduced scenarios: " << get_int_config( "intReducedNumScenarios" ) << endl;
  cout << "  Reduction method.: " << get_str_config( "strReductionMethod" ) << endl;
  cout << "  Verbose level....: " << get_int_config( "intLogVerb" ) << endl;
  if( get_dbl_config( "dblMaxTime" ) > 0 )
   cout << "  Time limit......: " << get_dbl_config( "dblMaxTime" ) << " seconds" << endl;
  if( get_int_config( "intSaveResults" ) || get_int_config( "intLoadResults" ) ) {
   cout << "  Cache directory.: " << get_str_config( "strCacheDir" ) << endl;
   if( get_int_config( "intSaveResults" ) ) cout << "  Saving results.: enabled" << endl;
   if( get_int_config( "intLoadResults" ) ) cout << "  Loading results from cache: enabled" << endl;
  }
  if( get_int_config( "intComputeVPI" ) ) cout << "  Compute VPI....: enabled" << endl;
  if( get_int_config( "intSkipFull" )   ) cout << "  Skip full solve: enabled" << endl;
  cout << "=== Scenario Reduction Test  ===" << endl;
 }
}

/*--------------------------------------------------------------------------*/
/*---------------------------- load ----------------------------------------*/
/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::load( ) {
 if( get_int_config( "intLogVerb" ) >= 1 )
  cout << "\nLoading base instance" << endl;

 load_problem_instance( get_str_config( "strInstancePath" ) );

 if( ! base_block )
  throw runtime_error(
   "Failed to load base block from " + get_str_config( "strInstancePath" ) );
 if( ! stochastic_block )
  throw runtime_error(
   "Failed to create stochastic block for " + get_str_config( "strInstancePath" ) );

 if( get_int_config( "intLogVerb" ) >= 1 )
  cout << "\nLoading scenarios" << endl;

 string scenario_file;
 if( ! get_str_config( "strScenarioFile" ).empty( ) ) {
  scenario_file = get_str_config( "strScenarioFile" );
  if( get_int_config( "intLogVerb" ) >= 2 )
   cout << "  Using user-specified scenario file: " << scenario_file << endl;
 }
 else {
  scenario_file = get_scenario_file( get_str_config( "strInstancePath" ) );
  if( get_int_config( "intLogVerb" ) >= 2 )
   cout << "  Using scenario file: " << scenario_file << endl;
 }

 try {
  netCDF::NcFile file( scenario_file , netCDF::NcFile::read );
  scenario_set = std::make_unique< DiscreteScenarioSet >( );
  scenario_set->deserialize( file );
  file.close( );

  size_t total = scenario_set->get_nbScenarios( );
  dimension_scenario = scenario_set->get_scenarioSize( );

  if( get_int_config( "intFullNumScenarios" ) > 0 ) {
   if( static_cast< size_t >( get_int_config( "intFullNumScenarios" ) ) <= total ) {
    scenario_set->init_representative_pool(
     get_int_config( "intFullNumScenarios" ) );
   }
   else {
    if( get_int_config( "intLogVerb" ) >= 1 )
     cout << "  Warning: Requested " << get_int_config( "intFullNumScenarios" )
          << " scenarios but file only contains " << total
          << "; using all" << endl;
    config->set_par( std::string("intFullNumScenarios") , static_cast< int >(total) );
    scenario_set->init_representative_pool(
     get_int_config( "intFullNumScenarios" ) );
   }
  }
  else {
   config->set_par( std::string("intFullNumScenarios") , static_cast< int >(total) );
   scenario_set->init_representative_pool(
    get_int_config( "intFullNumScenarios" ) );
  }

 } catch( const netCDF::exceptions::NcException & e ) {
  throw runtime_error(
   "Failed to load scenarios from: " + scenario_file + "\n\n"
   "Scenarios must be pre-generated before running tests.\n"
   "  1. Generate scenarios using the problem-specific generator\n"
   "  2. Place scenarios in: " + get_scenarios_directory( ) + "\n"
   "  3. Use naming convention: <instance_name>_scenarios.nc4\n"
   "  4. Or specify a custom scenario file with -f option\n\n"
   "NetCDF error: " + e.what( ) );
 } catch( const std::exception & e ) {
  throw runtime_error(
   "Failed to load scenarios from: " + scenario_file + "\n\n"
   "Scenarios must be pre-generated. Use the problem-specific generator\n"
   "or specify a valid scenario file with the -f option.\n\n"
   "Error: " + e.what( ) );
 }

 if( get_int_config( "intLogVerb" ) >= 1 ) {
  cout << "  Loaded instance" << endl;
  cout << "    Scenario dimension: " << get_scenario_dimension( ) << endl;
 }

 if( get_int_config( "intReducedNumScenarios" ) >
     get_int_config( "intFullNumScenarios" ) )
  throw runtime_error(
   "Error: Number of reduced scenarios (" +
   to_string( get_int_config( "intReducedNumScenarios" ) ) +
   ") cannot exceed total scenarios (" +
   to_string( get_int_config( "intFullNumScenarios" ) ) + ")" );
}

/*--------------------------------------------------------------------------*/
/*------------------- solve_scenario_reduction ----------------------------*/
/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::solve_scenario_reduction( ) {
 const int    K      = get_int_config( "intReducedNumScenarios" );
 const string method = get_str_config( "strReductionMethod" );

 if( get_int_config( "intLogVerb" ) >= 1 )
  cout << "\nPerforming scenario reduction:\n"
       << "  Reducing from " << get_int_config( "intFullNumScenarios" )
       << " to " << K << " scenarios using " << method << " method..." << endl;

 // ---- Try to load cached reduction solution ---------------------------
 if( get_int_config( "intLoadResults" ) ) {
  try {
   string cache = generate_reduction_cache_filename( );
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Attempting to load reduction solution from cache: "
         << cache << endl;
   reduction_metrics = load_reduction_solution( cache );
   if( ! reduction_metrics.selected_indices.empty( ) ) {
    if( get_int_config( "intLogVerb" ) >= 1 ) {
     cout << "  Loaded reduction solution metadata from cache" << endl;
     cout << "  Selected " << reduction_metrics.selected_indices.size( )
          << " scenarios" << endl;
     cout << "  Wasserstein distance: "
          << reduction_metrics.wasserstein_distance << endl;
     cout << "  Note: Will need to recompute reduction for exact indices" << endl;
    }
   }
  } catch( const exception & e ) {
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Could not load reduction solution: " << e.what( ) << endl;
   if( get_int_config( "intLogVerb" ) >= 1 )
    cout << "  Computing scenario reduction..." << endl;
  }
 }

 auto reduction_start = chrono::high_resolution_clock::now( );

 // ---- Build ScenarioReductionBlock via subclass -----------------------
 auto srb = create_srb( K , method );

 // ---- Attach solver and run -------------------------------------------
 if( method == "cssc" ) {
  auto * bsc = dynamic_cast< BlockSolverConfig * >(
   Configuration::deserialize( get_str_config( "strSolverConfig" ) ) );
  if( ! bsc )
   throw runtime_error(
    "solve_scenario_reduction: failed to load solver config for CSSC" );
  run_cssc( srb.get() , bsc , K );  // virtual, subclass may override for UC
 }
 else {
  // Generic distribution-driven heuristics: reads the DiscreteScenarioSet
  // directly from the ScenarioReductionBlock (no synthetic CapacitatedFacility
  // LocationBlock needed), works identically for CFL, UC, or any problem.
  auto solver = std::make_unique< ScenarioReductionSolver >( );
  solver->set_nb_reduced( K );

  int algo = 1;  // dupacova default
  if(      method == "baseline" ) algo = 0;
  else if( method == "dupacova" ) algo = 1;
  else if( method == "bestfit"  ) algo = 2;
  else if( method == "firstfit" ) algo = 3;
  solver->set_algorithm( algo );

  solver->set_Block( srb.get( ) );
  solver->compute( );
  solver->get_var_solution( );
 }

 auto reduction_end = chrono::high_resolution_clock::now( );
 reduction_metrics.reduction_time_ms =
  chrono::duration_cast< chrono::microseconds >(
   reduction_end - reduction_start ).count( );

 // ---- Read solution from SRB and rebuild scenario_set -----------------
 const auto & sol = srb->get_solution( );
 if( ! sol.is_set( ) )
  throw runtime_error(
   "solve_scenario_reduction: solver did not write solution into "
   "ScenarioReductionBlock" );

 const auto & sel = sol.selected_indices;
 const auto & wts = sol.weights;
 const int    Ks  = static_cast< int >( sel.size( ) );

 reduction_metrics.selected_indices.clear( );
 reduction_metrics.probabilities.clear( );
 for( int p = 0 ; p < Ks ; ++p ) {
  reduction_metrics.selected_indices.push_back( static_cast< int >( sel[p] ) );
  reduction_metrics.probabilities.push_back( wts[p] );
 }
 reduction_metrics.ell = scenario_set->get_ell( );

 // Rebuild scenario_set with only the K selected scenarios + weights
 const size_t sc_dim = scenario_set->get_scenarioSize( );
 std::vector< std::vector< double > > red_sc;
 std::vector< double >                red_wt;
 red_sc.reserve( Ks );
 red_wt.reserve( Ks );
 for( int p = 0 ; p < Ks ; ++p ) {
  std::vector< double > sv( sc_dim );
  for( DiscreteScenarioSet::ScenarioSize d = 0 ; d < sc_dim ; ++d )
   sv[d] = scenario_set->get_scenario_value( sel[p] , d );
  red_sc.push_back( sv );
  double w = wts[p];
  red_wt.push_back( w > 0.0 ? w : 1.0 / Ks );
 }
 double wsum = 0.0;
 for( double w : red_wt ) wsum += w;
 if( wsum > 0.0 ) for( double & w : red_wt ) w /= wsum;

 scenario_set = std::make_unique< DiscreteScenarioSet >( );
 scenario_set->load_from_memory( red_sc , red_wt );
 scenario_set->init_representative_pool(
  static_cast< DiscreteScenarioSet::ScenarioIndex >( Ks ) );

 if( get_int_config( "intLogVerb" ) >= 1 ) {
  cout << "  Scenario reduction completed in "
       << reduction_metrics.reduction_time_ms << " us" << endl;
  cout << "  Selected " << Ks << " scenarios" << endl;
 }

 // Save to cache 
 if( get_int_config( "intSaveResults" ) ) {
  try {
   save_reduction_solution( generate_reduction_cache_filename( ) ,
                             reduction_metrics );
   if( get_int_config( "intLogVerb" ) >= 1 )
    cout << "  Saved reduction solution to cache" << endl;
  } catch( const exception & e ) {
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Failed to save reduction solution: " << e.what( ) << endl;
  }
 }
}

/*--------------------------------------------------------------------------*/
/*------------------- solve_stochastic_problem ----------------------------*/
/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::solve_stochastic_problem( ) {
 if( get_int_config( "intLogVerb" ) >= 1 )
  cout << "\nSolving stochastic problem" << endl;

 size_t current_scenarios = scenario_set->get_poolSize( );
 bool   is_full = ( current_scenarios ==
  static_cast< size_t >( get_int_config( "intFullNumScenarios" ) ) );

 // ---- Try to load cached results --------------------------------------
 if( get_int_config( "intLoadResults" ) ) {
  try {
   string cache_file = generate_cache_filename( is_full , false );
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Attempting to load cache: " << cache_file << endl;
   if( is_full ) full_result    = load_solution_cache( cache_file );
   else          reduced_result = load_solution_cache( cache_file );
   if( get_int_config( "intLogVerb" ) >= 1 )
    cout << "  Loaded pre-computed results from cache: " << cache_file << endl;
   return;
  } catch( const exception & e ) {
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Cache load failed: " << e.what( ) << endl;
   if( get_int_config( "intLogVerb" ) >= 1 )
    cout << "  Could not load cached results, computing..." << endl;
  }
 }

 if( get_int_config( "intLogVerb" ) >= 1 )
  cout << "  Solving stochastic problem with " << current_scenarios
       << " scenarios:" << endl;

 //  Build TSSB via subclass 
 const string tmp = is_full ? "/tmp/abs_tssb_full.nc4"
                             : "/tmp/abs_tssb_red.nc4";
 auto tss_block = build_tssb_for_current_pool( tmp );

 SolutionResult res = compute_extensive_form( tss_block.get( ) );
 if( is_full ) full_result    = res;
 else          reduced_result = res;
}

/*--------------------------------------------------------------------------*/
/*------------------------- solve_anticipative ----------------------------*/
/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::solve_anticipative( ) {
 if( ! get_int_config( "intComputeVPI" ) ) return;

 if( get_int_config( "intLogVerb" ) >= 1 )
  cout << "\nComputing anticipative solutions:" << endl;

 if( get_int_config( "intLoadResults" ) ) {
  bool loaded_full = false , loaded_reduced = false;
  if( get_int_config( "intLogVerb" ) >= 2 )
   cout << "[DEBUG] Checking for cached anticipative solutions..." << endl;

  try {
   string f = generate_cache_filename( true , true );
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Trying to load: " << f << endl;
   anticipative_full = load_solution_cache( f );
   loaded_full = true;
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Successfully loaded anticipative full from cache" << endl;
  } catch( const exception & e ) {
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Anticipative full not in cache: " << e.what( ) << endl;
  }

  try {
   string f = generate_cache_filename( false , true );
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Trying to load: " << f << endl;
   anticipative_reduced = load_solution_cache( f );
   loaded_reduced = true;
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Successfully loaded anticipative reduced from cache" << endl;
  } catch( const exception & e ) {
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Anticipative reduced not in cache: " << e.what( ) << endl;
  }

  if( (loaded_full || loaded_reduced) && get_int_config( "intLogVerb" ) >= 1 )
   cout << "  Loaded pre-computed anticipative solutions from cache "
        << "(full: " << (loaded_full ? "yes" : "no")
        << ", reduced: " << (loaded_reduced ? "yes" : "no") << ")" << endl;
  else if( get_int_config( "intLogVerb" ) >= 1 )
   cout << "  Could not load cached anticipative solutions, computing..." << endl;
 }

 if( ! scenario_set->is_pool_initialized( ) )
  throw runtime_error( "solve_anticipative: pool not initialized" );

 size_t current_pool_size = scenario_set->get_poolSize( );
 bool is_full_pool    = (current_pool_size ==
  static_cast< size_t >( get_int_config( "intFullNumScenarios" ) ));
 bool is_reduced_pool = (current_pool_size ==
  static_cast< size_t >( get_int_config( "intReducedNumScenarios" ) ));

 scenario_set->reset_pool( );

 if( is_full_pool ) {
  if( ! anticipative_full.solved ) {
   if( get_int_config( "intLogVerb" ) >= 1 )
    cout << "  Computing anticipative solution for full scenario set ("
         << current_pool_size << " scenarios)..." << endl;
   anticipative_full = solve_anticipative_solution( );
  }
 }
 else if( is_reduced_pool ) {
  if( ! anticipative_reduced.solved ) {
   if( get_int_config( "intLogVerb" ) >= 1 )
    cout << "  Computing anticipative solution for reduced scenario set ("
         << current_pool_size << " scenarios)..." << endl;
   anticipative_reduced = solve_anticipative_solution( );
  }
 }
 else {
  if( get_int_config( "intLogVerb" ) >= 1 )
   cout << "  Warning: Current pool size (" << current_pool_size
        << ") doesn't match full (" << get_int_config( "intFullNumScenarios" )
        << ") or reduced (" << get_int_config( "intReducedNumScenarios" )
        << ") - skipping anticipative computation" << endl;
  return;
 }

 if( get_int_config( "intLogVerb" ) >= 1 ) {
  if( is_full_pool && anticipative_full.solved )
   cout << "  Full anticipative objective: " << fixed << setprecision( 2 )
        << anticipative_full.objective << endl;
  else if( is_reduced_pool && anticipative_reduced.solved )
   cout << "  Reduced anticipative objective: " << fixed << setprecision( 2 )
        << anticipative_reduced.objective << endl;
 }
}

/*--------------------------------------------------------------------------*/
/*--------------------------- print_results --------------------------------*/
/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::print_results( ) {
 if( get_int_config( "intLogVerb" ) < 1 ) return;

 cout << "\n========== RESULTS ==========" << endl;
 cout << "Instance..........: " << get_str_config( "strInstancePath" ) << endl;
 cout << "Scenario dimension: " << get_scenario_dimension( ) << endl;

 cout << "\nScenarios" << endl;
 cout << "  Full............: " << get_int_config( "intFullNumScenarios" ) << endl;
 cout << "  Reduced.........: " << get_int_config( "intReducedNumScenarios" ) << endl;
 cout << "  Reduction method: " << get_str_config( "strReductionMethod" ) << endl;

 cout << "\nTiming" << endl;
 cout << "  Reduction time.: " << reduction_metrics.reduction_time_ms << " us" << endl;
 if( full_result.solved )
  cout << "  Full solve time: " << full_result.time_ms << " us" << endl;
 if( reduced_result.solved )
  cout << "  Reduced solve t: " << reduced_result.time_ms << " us" << endl;

 cout << "\nObjectives" << endl;
 if( full_result.solved )
  cout << "  Full...: " << fixed << setprecision( 2 ) << full_result.objective << endl;
 else
  cout << "  Full...: NOT SOLVED" << endl;
 if( reduced_result.solved )
  cout << "  Reduced: " << fixed << setprecision( 2 ) << reduced_result.objective << endl;
 else
  cout << "  Reduced: NOT SOLVED" << endl;

 if( full_result.solved && reduced_result.solved ) {
  double diff = abs( reduced_result.objective - full_result.objective );
  double rel  = diff / abs( full_result.objective ) * 100.0;
  cout << "  Gap (absolute): " << fixed << setprecision( 4 ) << diff
       << "  (" << setprecision( 6 ) << rel << "%)" << endl;
 }
 else if( reduced_result.solved )
  cout << "  Gap (absolute): n/a (full not solved - omit --skip-full to "
          "compute the gap)" << endl;

 if( get_int_config( "intComputeVPI" ) &&
     (anticipative_full.solved || anticipative_reduced.solved) ) {
  cout << "\nValue of Perfect Information" << endl;
  if( anticipative_full.solved && full_result.solved ) {
   double vpi = full_result.objective - anticipative_full.objective;
   cout << "  Full scenarios" << endl;
   cout << "    Stochastic......: " << fixed << setprecision( 2 )
        << full_result.objective << endl;
   cout << "    Anticipative....: " << fixed << setprecision( 2 )
        << anticipative_full.objective << endl;
   cout << "    Value Perf. Info: " << fixed << setprecision( 2 ) << vpi;
   if( vpi < 0 ) cout << " (WARNING: negative VPI indicates potential issue)";
   cout << endl;
  }
  if( anticipative_reduced.solved && reduced_result.solved ) {
   double vpi = reduced_result.objective - anticipative_reduced.objective;
   cout << "  Reduced scenarios" << endl;
   cout << "    Stochastic..: " << fixed << setprecision( 2 )
        << reduced_result.objective << endl;
   cout << "    Anticipative: " << fixed << setprecision( 2 )
        << anticipative_reduced.objective << endl;
   cout << "    VPI.........: " << fixed << setprecision( 2 ) << vpi;
   if( vpi < 0 ) cout << " (WARNING: negative VPI indicates potential issue)";
   cout << endl;
  }
 }

 if( get_int_config( "intLogVerb" ) >= 2 ) {
  cout << "\nSelected scenarios and weights:" << endl;
  auto pool_weights = scenario_set->get_pool_weights( );
  cout << "  Indices: ";
  for( int idx : reduction_metrics.selected_indices )
   cout << setw( 8 ) << idx;
  cout << endl;
  cout << "  Weights: ";
  for( double weight : pool_weights )
   cout << setw( 8 ) << fixed << setprecision( 4 ) << weight;
  cout << endl;
 }

 cout << "\n=============================" << endl;
}

/*--------------------------------------------------------------------------*/
/*------------------------- HELPER METHODS ---------------------------------*/
/*--------------------------------------------------------------------------*/

Block * AbstractScenarioReductionTest::get_base_block( ) { return base_block; }

void AbstractScenarioReductionTest::apply_scenario_to_block(
  const std::vector< double > & scenario ) {
 if( ! stochastic_block )
  throw runtime_error( "StochasticBlock not initialized - ensure "
   "TwoStageStochasticBlock is created first" );

 const auto & data_mappings = stochastic_block->get_data_mappings( );
 Block * original_caller    = stochastic_block->get_inner_block( );

 for( auto & mapping : data_mappings ) {
  mapping->set_caller( base_block );
  mapping->set_data( scenario.begin( ) , eNoBlck , eNoBlck );
 }
 for( auto & mapping : data_mappings )
  mapping->set_caller( original_caller );
}

SolutionResult AbstractScenarioReductionTest::compute_extensive_form(
  TwoStageStochasticBlock * tss_block ) {
 SolutionResult result;
 try {
  auto * bsc = dynamic_cast< BlockSolverConfig * >(
   Configuration::deserialize( get_str_config( "strSolverConfig" ) ) );
  if( ! bsc ) throw runtime_error( "Failed to load solver configuration" );
  bsc->apply( tss_block );

  if( ! tss_block->get_registered_solvers( ).empty( ) ) {
   auto solver = tss_block->get_registered_solvers( ).front( );
   if( get_dbl_config( "dblMaxTime" ) > 0 )
    solver->set_par( Solver::dblMaxTime , get_dbl_config( "dblMaxTime" ) );

   auto solve_start = chrono::high_resolution_clock::now( );
   int  solve_result = solver->compute( false );
   auto solve_end   = chrono::high_resolution_clock::now( );
   result.time_ms = chrono::duration_cast< chrono::microseconds >(
    solve_end - solve_start ).count( );

   if( solve_result == Solver::kOK ) {
    result.objective = solver->get_ub( );
    result.solved    = true;
    if( get_int_config( "intLogVerb" ) >= 2 )
     cout << "    Objective (expected): " << fixed << setprecision( 2 )
          << result.objective << "\n    Time: " << result.time_ms << " us" << endl;
   }
   else if( get_int_config( "intLogVerb" ) >= 1 ) {
    cout << "    Failed to solve (status: " << solve_result;
    if(      solve_result == Solver::kInfeasible   ) cout << " - Problem is infeasible";
    else if( solve_result == Solver::kUnbounded     ) cout << " - Problem is unbounded";
    else if( solve_result == Solver::kError         ) cout << " - Solver error";
    else if( solve_result == Solver::kStopTime      ) cout << " - Time limit reached";
    else if( solve_result == Solver::kStopIter      ) cout << " - Iteration limit reached";
    else if( solve_result == Solver::kLowPrecision  ) cout << " - Low precision solution";
    cout << ")" << endl;
   }
  }
  delete bsc;
 } catch( const exception & e ) {
  if( get_int_config( "intLogVerb" ) >= 1 )
   cout << "    Error: " << e.what( ) << endl;
 }
 return result;
}

SolutionResult AbstractScenarioReductionTest::solve_anticipative_solution( ) {
 SolutionResult result;
 auto selected_indices = scenario_set->get_selected_scenarios( );
 auto pool_weights     = scenario_set->get_pool_weights( );
 size_t scenario_size  = scenario_set->get_scenarioSize( );

 for( size_t i = 0 ; i < selected_indices.size( ) ; ++i ) {
  vector< double > scenario( scenario_size );
  for( size_t j = 0 ; j < scenario_size ; ++j )
   scenario[j] = scenario_set->get_scenario_value( selected_indices[i] , j );
  apply_scenario_to_block( scenario );
  auto [ obj , solved ] = solve( get_base_block( ) );
  if( solved )
   result.scenario_objectives.push_back( obj );
  else { result.solved = false; return result; }
 }

 if( ! result.scenario_objectives.empty( ) ) {
  double ws = 0.0;
  for( size_t i = 0 ; i < result.scenario_objectives.size( ) ; ++i )
   ws += result.scenario_objectives[i] * pool_weights[i];
  result.objective = ws;
  result.solved    = true;
 }
 return result;
}

pair< double , bool > AbstractScenarioReductionTest::solve( Block * block ) {
 try {
  auto cfg = Configuration::deserialize( get_str_config( "strSolverConfig" ) );
  auto * bsc = dynamic_cast< BlockSolverConfig * >(cfg);
  if( ! bsc ) { delete cfg; return { 0.0 , false }; }
  bsc->apply( block );
  if( ! block->get_registered_solvers( ).empty( ) ) {
   auto solver = block->get_registered_solvers( ).front( );
   if( get_dbl_config( "dblMaxTime" ) > 0 )
    solver->set_par( Solver::dblMaxTime , get_dbl_config( "dblMaxTime" ) );
   int result = solver->compute( false );
   if( result == Solver::kOK ) {
    double obj = solver->get_ub( );
    delete cfg;
    return { obj , true };
   }
  }
  delete cfg;
 } catch( const exception & ) {}
 return { 0.0 , false };
}

/*--------------------------------------------------------------------------*/
/*------------------------- I/O METHODS -----------------------------------*/
/*--------------------------------------------------------------------------*/

vector< vector< double > >
AbstractScenarioReductionTest::load_scenarios_from_file(
  const string & filename ) {
 vector< vector< double > > scenarios;
 try {
  netCDF::NcFile file( filename , netCDF::NcFile::read );
  auto scenDim = file.getDim( "NumberScenarios" );
  auto sizeDim = file.getDim( "ScenarioSize" );
  if( scenDim.isNull( ) || sizeDim.isNull( ) )
   throw runtime_error( "File doesn't contain valid DiscreteScenarioSet dimensions" );
  size_t ns = scenDim.getSize( ) , ss = sizeDim.getSize( );
  auto scenVar = file.getVar( "Scenarios" );
  if( scenVar.isNull( ) )
   throw runtime_error( "File doesn't contain Scenarios variable" );
  scenarios.resize( ns );
  for( size_t s = 0 ; s < ns ; ++s ) {
   scenarios[s].resize( ss );
   scenVar.getVar( {s,0} , {1,ss} , scenarios[s].data( ) );
  }
  file.close( );
 } catch( const netCDF::exceptions::NcException & e ) {
  throw runtime_error( "NetCDF error reading scenario file: " + string(e.what()) );
 }
 return scenarios;
}

string AbstractScenarioReductionTest::get_scenario_file(
  const string & instance_path ) const {
 filesystem::path p( instance_path );
 return get_scenarios_directory( ) + p.stem( ).string( ) + "_scenarios.nc4";
}

string AbstractScenarioReductionTest::extract_instance_name(
  const string & instance_path ) const {
 filesystem::path p( instance_path );
 string filename = p.stem( ).string( );
 if( instance_path.find( "Yang" ) != string::npos ) {
  filesystem::path parent = p.parent_path( );
  if( parent.filename( ).string( ).find( "-" ) != string::npos ) {
   string result = "Yang" + filename;
   if( get_int_config( "intLogVerb" ) >= 2 )
    cout << "[DEBUG] Extracted instance name from Yang path: " << result
         << " (from: " << instance_path << ")" << endl;
   return result;
  }
 }
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "[DEBUG] Extracted instance name: " << filename
       << " (from: " << instance_path << ")" << endl;
 return filename;
}

string AbstractScenarioReductionTest::generate_cache_filename(
  bool is_full , bool is_anticipative ) const {
 string inst   = extract_instance_name( get_str_config( "strInstancePath" ) );
 int    n      = get_int_config( "intFullNumScenarios" );
 int    m      = get_int_config( "intReducedNumScenarios" );
 string method = get_str_config( "strReductionMethod" );
 stringstream ss;
 ss << get_str_config( "strCacheDir" ) << inst;
 if( is_full )  ss << "_" << n;
 else           ss << "_" << n << "_" << m << "_" << method;
 if( is_anticipative ) ss << "_anticipative";
 ss << ".nc4";
 string result = ss.str( );
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "[DEBUG] Generated cache filename: " << result
       << " (full=" << is_full << ", anticipative=" << is_anticipative << ")" << endl;
 return result;
}

bool AbstractScenarioReductionTest::update_SR_config(
  const string & method , bool warmstart , bool shuffle ) {
 try {
  ifstream in( "BSConfig_SR.txt" );
  if( ! in.is_open( ) ) return false;
  stringstream buffer;
  string line;
  while( getline( in , line ) ) {
   if( line.find( "intAlgorithm" ) != string::npos ) {
    if(      method == "baseline" ) line = "intAlgorithm                            0  # Baseline";
    else if( method == "dupacova" ) line = "intAlgorithm                            1  # Dupacova";
    else if( method == "bestfit"  ) line = "intAlgorithm                            2  # BestFit";
    else if( method == "firstfit" ) line = "intAlgorithm                            3  # FirstFit";
   }
   else if( line.find( "intWarmStart"  ) != string::npos )
    line = "intWarmStart = " + to_string( warmstart ? 1 : 0 );
   else if( line.find( "boolShuffleFF" ) != string::npos )
    line = "boolShuffleFF = " + to_string( shuffle ? 1 : 0 );
   buffer << line << '\n';
  }
  in.close( );
  ofstream out( "BSConfig_SR.txt" );
  out << buffer.str( );
  return true;
 } catch( ... ) { return false; }
}

void AbstractScenarioReductionTest::save_solution_cache(
  const string & filename , const SolutionResult & result ) {
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "    Saving solution result to cache: " << filename << endl;
 try {
  filesystem::path fp( filename );
  if( fp.has_parent_path( ) ) filesystem::create_directories( fp.parent_path( ) );
  netCDF::NcFile file( filename , netCDF::NcFile::replace );
  file.putAtt( "objective" , netCDF::NcDouble( ) , result.objective );
  file.putAtt( "solved"    , netCDF::NcInt( )    , result.solved ? 1 : 0 );
  file.putAtt( "time_ms"   , netCDF::NcInt64( )  , result.time_ms );
  if( ! result.scenario_objectives.empty( ) ) {
   auto dim = file.addDim( "NumScenarios" , result.scenario_objectives.size( ) );
   file.addVar( "ScenarioObjectives" , netCDF::NcDouble( ) , dim )
       .putVar( result.scenario_objectives.data( ) );
  }
  file.close( );
  if( get_int_config( "intLogVerb" ) >= 2 )
   cout << "      Saved solution with objective=" << result.objective
        << ", time=" << result.time_ms << "ms" << endl;
 } catch( const exception & e ) {
  cerr << "Warning: Failed to save solution cache: " << e.what( ) << endl;
 }
}

void AbstractScenarioReductionTest::save_solutions_cache( ) {
 if( ! get_int_config( "intSaveResults" ) ) return;
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "[DEBUG] Saving cache files..." << endl;
 if( full_result.solved ) {
  string f = generate_cache_filename( true , false );
  save_solution_cache( f , full_result );
  if( get_int_config( "intLogVerb" ) >= 2 ) cout << "[DEBUG] Saved full extensive: " << f << endl;
 }
 if( reduced_result.solved ) {
  string f = generate_cache_filename( false , false );
  save_solution_cache( f , reduced_result );
  if( get_int_config( "intLogVerb" ) >= 2 ) cout << "[DEBUG] Saved reduced extensive: " << f << endl;
 }
 if( anticipative_full.solved ) {
  string f = generate_cache_filename( true , true );
  save_solution_cache( f , anticipative_full );
  if( get_int_config( "intLogVerb" ) >= 2 ) cout << "[DEBUG] Saved anticipative full: " << f << endl;
 }
 if( anticipative_reduced.solved ) {
  string f = generate_cache_filename( false , true );
  save_solution_cache( f , anticipative_reduced );
  if( get_int_config( "intLogVerb" ) >= 2 ) cout << "[DEBUG] Saved anticipative reduced: " << f << endl;
 }
 if( get_int_config( "intLogVerb" ) >= 1 )
  cout << "  Saved results to cache" << endl;
}

SolutionResult AbstractScenarioReductionTest::load_solution_cache(
  const string & filename ) {
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "    Loading solution result from cache: " << filename << endl;
 if( ! filesystem::exists( filename ) )
  throw runtime_error( "Cache file not found: " + filename );
 SolutionResult result;
 netCDF::NcFile file( filename , netCDF::NcFile::read );
 file.getAtt( "objective" ).getValues( & result.objective );
 int si; file.getAtt( "solved" ).getValues( &si ); result.solved = (si != 0);
 file.getAtt( "time_ms"   ).getValues( & result.time_ms );
 try {
  auto var = file.getVar( "ScenarioObjectives" );
  if( ! var.isNull( ) ) {
   size_t n = file.getDim( "NumScenarios" ).getSize( );
   result.scenario_objectives.resize( n );
   var.getVar( result.scenario_objectives.data( ) );
  }
 } catch( ... ) {}
 file.close( );
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "      Loaded solution with objective=" << result.objective
       << ", time=" << result.time_ms << "ms" << endl;
 return result;
}

string AbstractScenarioReductionTest::generate_reduction_cache_filename( ) const {
 string inst   = extract_instance_name( get_str_config( "strInstancePath" ) );
 int    n      = get_int_config( "intFullNumScenarios" );
 int    m      = get_int_config( "intReducedNumScenarios" );
 string method = get_str_config( "strReductionMethod" );
 stringstream ss;
 ss << get_str_config( "strCacheDir" ) << inst
    << "_" << n << "_" << m << "_" << method << "_reduction.nc4";
 string result = ss.str( );
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "[DEBUG] Generated reduction cache filename: " << result << endl;
 return result;
}

void AbstractScenarioReductionTest::save_reduction_solution(
  const string & filename , const ScenarioReductionMetrics & metrics ) {
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "[DEBUG] Saving reduction solution to cache: " << filename << endl;
 try {
  filesystem::path fp( filename );
  if( fp.has_parent_path( ) ) filesystem::create_directories( fp.parent_path( ) );
  netCDF::NcFile file( filename , netCDF::NcFile::replace );
  file.putAtt( "reduction_time_ms"    , netCDF::NcInt64( )  , metrics.reduction_time_ms );
  file.putAtt( "ell"                  , netCDF::NcDouble( ) , metrics.ell );
  file.putAtt( "wasserstein_distance" , netCDF::NcDouble( ) , metrics.wasserstein_distance );
  file.putAtt( "wasserstein_ell_power", netCDF::NcDouble( ) , metrics.wasserstein_ell_power );
  if( ! metrics.selected_indices.empty( ) ) {
   auto dim = file.addDim( "NumSelected" , metrics.selected_indices.size( ) );
   file.addVar( "SelectedIndices" , netCDF::NcInt( )    , dim )
       .putVar( metrics.selected_indices.data( ) );
   if( ! metrics.probabilities.empty( ) )
    file.addVar( "Probabilities"  , netCDF::NcDouble( ) , dim )
        .putVar( metrics.probabilities.data( ) );
  }
  file.close( );
  if( get_int_config( "intLogVerb" ) >= 2 )
   cout << "[DEBUG] Saved reduction solution: "
        << metrics.selected_indices.size( ) << " scenarios selected, "
        << "Wasserstein distance=" << metrics.wasserstein_distance
        << ", time=" << metrics.reduction_time_ms << "ms" << endl;
 } catch( const exception & e ) {
  cerr << "Warning: Failed to save reduction solution cache: " << e.what( ) << endl;
 }
}

ScenarioReductionMetrics AbstractScenarioReductionTest::load_reduction_solution(
  const string & filename ) {
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "[DEBUG] Loading reduction solution from cache: " << filename << endl;
 if( ! filesystem::exists( filename ) )
  throw runtime_error( "Reduction cache file not found: " + filename );
 ScenarioReductionMetrics metrics;
 netCDF::NcFile file( filename , netCDF::NcFile::read );
 file.getAtt( "reduction_time_ms"    ).getValues( & metrics.reduction_time_ms );
 file.getAtt( "ell"                  ).getValues( & metrics.ell );
 file.getAtt( "wasserstein_distance" ).getValues( & metrics.wasserstein_distance );
 file.getAtt( "wasserstein_ell_power").getValues( & metrics.wasserstein_ell_power );
 try {
  auto vi = file.getVar( "SelectedIndices" );
  if( ! vi.isNull( ) ) {
   size_t n = file.getDim( "NumSelected" ).getSize( );
   metrics.selected_indices.resize( n );
   vi.getVar( metrics.selected_indices.data( ) );
   auto vp = file.getVar( "Probabilities" );
   if( ! vp.isNull( ) ) {
    metrics.probabilities.resize( n );
    vp.getVar( metrics.probabilities.data( ) );
   }
  }
 } catch( ... ) {}
 file.close( );
 if( get_int_config( "intLogVerb" ) >= 2 )
  cout << "[DEBUG] Loaded reduction solution: "
       << metrics.selected_indices.size( ) << " scenarios, "
       << "Wasserstein distance=" << metrics.wasserstein_distance
       << ", time=" << metrics.reduction_time_ms << "ms" << endl;
 return metrics;
}

/*--------------------------------------------------------------------------*/
/*------------------------------ run_cssc ---------------------------------*/
/*--------------------------------------------------------------------------*/

void AbstractScenarioReductionTest::run_cssc(
  ScenarioReductionBlock * /*srb*/ , BlockSolverConfig * bsc , int /*K*/ )
{
 // No problem-agnostic default: CSSC needs a problem-specific VarExtractor
 // (the here-and-now variables) and data injection
 delete bsc;  // caller transferred ownership; release it on this error path
 throw std::logic_error(
  "AbstractScenarioReductionTest::run_cssc: no generic default - override "
  "run_cssc() in your subclass (a problem-specific VarExtractor is required)." );
}

/*--------------------------------------------------------------------------*/
/*---------- End File AbstractScenarioReductionTest.cpp -------------------*/
/*--------------------------------------------------------------------------*/