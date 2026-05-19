/*--------------------------------------------------------------------------*/
/*--------------------- File CFLTwoStageStochasticTest.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Solves the full Two-Stage Stochastic Capacitated Facility Location problem
 * using all N scenarios, without any scenario reduction.
 *
 * Usage:
 *   ./cfl_full_tss -i cap41.nc4 -n 50 -v 1
 *   ./cfl_full_tss -i cap41.nc4 -f cap41_scenarios.nc4 -n 50 -c BSPar1_CSSC.txt -v 1
 *
 * \author Minh Duc Pham
 *         Dipartimento di Informatica,
 *         Universita' di Pisa
 *
 * \copyright &copy; by Minh Duc Pham
 */
/*--------------------------------------------------------------------------*/

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "BlockSolverConfig.h"
#include "CapacitatedFacilityLocationBlock.h"
#include "Configuration.h"
#include "DiscreteScenarioSet.h"
#include "Solver.h"
#include "StochasticBlock.h"
#include "TwoStageStochasticBlock.h"

using namespace std;
using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
struct Config {
 string instance_path;
 string scenario_file;
 string solver_config  = "BSPar_HiGHS.txt";
 int    num_scenarios  = 0;   // 0 = use all from file
 int    verbose        = 1;
};

/*--------------------------------------------------------------------------*/

void print_help( const char * prog ) {
 cout << "Usage: " << prog << " [options]\n\n"
      << "  -i <path>   CFL instance (.nc4)          [required]\n"
      << "  -f <path>   Scenario file (.nc4)          [auto from instance name]\n"
      << "  -c <path>   Solver config file            [default: BSPar_HiGHS.txt]\n"
      << "  -n <N>      Number of scenarios to use   [default: all]\n"
      << "  -v <level>  Verbosity 0-2                 [default: 1]\n"
      << "  -h          Help\n";
}

/*--------------------------------------------------------------------------*/

Config parse_args( int argc , char * argv[] ) {
 Config cfg;
 for( int i = 1 ; i < argc ; ++i ) {
  string a = argv[ i ];
  if(      a == "-h" )              { print_help( argv[0] ); exit(0); }
  else if( a == "-i" && i+1 < argc ) cfg.instance_path = argv[++i];
  else if( a == "-f" && i+1 < argc ) cfg.scenario_file = argv[++i];
  else if( a == "-c" && i+1 < argc ) cfg.solver_config = argv[++i];
  else if( a == "-n" && i+1 < argc ) cfg.num_scenarios = stoi(argv[++i]);
  else if( a == "-v" && i+1 < argc ) cfg.verbose       = stoi(argv[++i]);
  else { cerr << "Unknown option: " << a << "\n"; exit(1); }
 }
 if( cfg.instance_path.empty() ) {
  cerr << "Error: -i is required\n";
  print_help( argv[0] ); exit(1);
 }
 return cfg;
}

// Build TwoStageStochasticBlock from base_cfl + dss, write/read netCDF 


unique_ptr<TwoStageStochasticBlock> build_tss(
  CapacitatedFacilityLocationBlock * base_cfl ,
  DiscreteScenarioSet * dss ,
  int nf , int nc )
{
 const string tmp = "tmp_full_tss.nc4";
 {
  netCDF::NcFile f( tmp , netCDF::NcFile::replace );
  auto tg = f.addGroup( "TwoStageStochasticBlock" );
  tg.addDim( "NumberScenarios" , dss->get_poolSize() );
  tg.putAtt( "type" , "TwoStageStochasticBlock" );

  { auto g = tg.addGroup( "FirstStageBlock" ); base_cfl->serialize( g ); }

  auto sg = tg.addGroup( "StaticAbstractPath" );
  auto dp = sg.addDim( "PathDim" , nf );
  auto dl = sg.addDim( "PathTotalLength" , nf );
  vector<unsigned int> idx( nf );
  for( int i = 0 ; i < nf ; i++ ) idx[ i ] = i;
  sg.addVar( "PathStart"          , netCDF::NcUint() , dp ).putVar( idx.data() );
  sg.addVar( "PathNodeTypes"      , netCDF::NcChar() , dl )
    .putVar( vector<char>( nf , 'V' ).data() );
  sg.addVar( "PathGroupIndices"   , netCDF::NcUint() , dl )
    .putVar( vector<unsigned int>( nf , 0 ).data() );
  sg.addVar( "PathElementIndices" , netCDF::NcUint() , dl ).putVar( idx.data() );
  sg.addVar( "PathRangeIndices"   , netCDF::NcUint() , dl ).putVar( idx.data() );

  auto sb = tg.addGroup( "StochasticBlock" );
  sb.putAtt( "type" , "StochasticBlock" );
  { auto g = sb.addGroup( "Block" ); base_cfl->serialize( g ); }
  { auto g = tg.addGroup( "DiscreteScenarioSet" ); dss->serialize( g ); }

  auto nd = sb.addDim( "NumberDataMappings" , 1 );
  char dc = 'D'; sb.addVar( "DataType" , netCDF::NcChar() , nd ).putVar( &dc );
  char cc = 'B'; sb.addVar( "Caller"   , netCDF::NcChar() , nd ).putVar( &cc );
  string fn = "CapacitatedFacilityLocationBlock::chg_customer_demands";
  sb.addVar( "FunctionName" , netCDF::NcString() , nd ).putVar( { 0 } , &fn );
  auto ds = sb.addDim( "SetSizeDim" , 2 );
  vector<unsigned int> ss = { 0 , 0 };
  sb.addVar( "SetSize" , netCDF::NcUint() , ds ).putVar( ss.data() );
  unsigned char ord = 0;
  sb.addVar( "Ordered" , netCDF::NcUbyte() , nd ).putVar( &ord );
  auto de = sb.addDim( "SetElementsDim" , 4 );
  vector<unsigned int> se = { 0 , (unsigned int)nc , 0 , (unsigned int)nc };
  sb.addVar( "SetElements" , netCDF::NcUint() , de ).putVar( se.data() );

  auto ap = sb.addGroup( "AbstractPath" );
  ap.addDim( "PathDim" , 1 );
  auto zd = ap.addDim( "PathTotalLength" , 0 );
  unsigned int ps = 0;
  ap.addVar( "PathStart"          , netCDF::NcUint() ,
             ap.getDim( "PathDim" ) ).putVar( &ps );
  ap.addVar( "PathNodeTypes"      , netCDF::NcChar()  , zd );
  ap.addVar( "PathGroupIndices"   , netCDF::NcUint()  , zd );
  ap.addVar( "PathElementIndices" , netCDF::NcUint()  , zd );
  ap.addVar( "PathRangeIndices"   , netCDF::NcUint()  , zd );
 }

 netCDF::NcFile f2( tmp , netCDF::NcFile::read );
 auto tss = make_unique<TwoStageStochasticBlock>();
 tss->deserialize( f2.getGroup( "TwoStageStochasticBlock" ) );
 f2.close();
 remove( tmp.c_str() );
 return tss;
}

/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] ) {

 cout << "\nTwo-Stage Stochastic CFL - Full Extensive Form\n\n";

 Config cfg = parse_args( argc , argv );

 try {
  //1. Load CFL instance
  if( cfg.verbose >= 1 ) cout << "[1/3] Loading CFL instance\n";

  auto * raw = Block::deserialize( cfg.instance_path );
  auto * base_cfl =
    dynamic_cast<CapacitatedFacilityLocationBlock*>( raw );
  if( !base_cfl ) { delete raw; throw runtime_error( "Not a CFL block" ); }
  if( !base_cfl->get_UnSplittable() ) base_cfl->chg_UnSplittable( true );

  int nf = base_cfl->get_NFacilities();
  int nc = base_cfl->get_NCustomers();

  if( cfg.verbose >= 1 )
   cout << "  " << nf << " facilities, " << nc << " customers\n\n";

  //2. Load scenarios
  if( cfg.verbose >= 1 ) cout << "[2/3] Loading scenarios\n";

  string sf = cfg.scenario_file;
  if( sf.empty() ) {
   filesystem::path p( cfg.instance_path );
   sf = "../scenarios/CFL/" + p.stem().string() + "_scenarios.nc4";
  }

  if( cfg.verbose >= 2 )
   cout << "  Scenario file: " << sf << "\n";

  netCDF::NcFile scf( sf , netCDF::NcFile::read );
  auto dss = make_unique<DiscreteScenarioSet>();
  dss->deserialize( scf );
  scf.close();

  int N = cfg.num_scenarios > 0
        ? min( cfg.num_scenarios , (int)dss->get_nbScenarios() )
        : (int)dss->get_nbScenarios();
  dss->init_representative_pool( N );

  if( cfg.verbose >= 1 )
   cout << "  N = " << N
        << "  dim = " << dss->get_scenario_size() << "\n\n";

  //3. Build and solve Full TSS
  if( cfg.verbose >= 1 )
   cout << "[3/3] Solving Full TSS (" << N << " scenarios)\n";

  auto tss = build_tss( base_cfl , dss.get() , nf , nc );

  auto * raw_cfg = Configuration::deserialize( cfg.solver_config );
  auto * bsc = dynamic_cast<BlockSolverConfig*>( raw_cfg );
  if( !bsc ) { delete raw_cfg; throw runtime_error( "Bad solver config" ); }
  bsc->apply( tss.get() );
  delete bsc;

  auto & sl = tss->get_registered_solvers();
  if( sl.empty() ) throw runtime_error( "No solver registered" );

  auto t0 = chrono::high_resolution_clock::now();
  int  st = sl.front()->compute( false );
  long long dt = chrono::duration_cast<chrono::milliseconds>(
    chrono::high_resolution_clock::now() - t0 ).count();

  if( st != Solver::kOK && st != Solver::kLowPrecision )
   throw runtime_error( "Solver failed (status=" + to_string(st) + ")" );

  double obj = sl.front()->get_ub();

  // Report
  cout << "\nResults\n"
       << "  Instance  : "
       << filesystem::path( cfg.instance_path ).stem().string() << "\n"
       << "  Scenarios : " << N << "\n"
       << "  Solver    : " << cfg.solver_config << "\n"
       << "  Objective : " << fixed << setprecision(2) << obj << "\n"
       << "  Time      : " << dt << " ms\n\n";

  return 0;

 } catch( const exception & e ) {
  cerr << "\nError: " << e.what() << "\n"
       << "Tips: run CFLScenarioGenerator first, K < N\n";
  return 1;
 }
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File CFLTwoStageStochasticTest.cpp --------------*/
/*--------------------------------------------------------------------------*/