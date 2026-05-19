/*--------------------------------------------------------------------------*/
/*------------------- File CFLCSSCScenarioReductionTest.cpp --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Test CSSC scenario reduction on Two-Stage Stochastic CFL.
 *
 * Key insight: CSSCScenarioReductionSolver needs TWO CFL blocks:
 *
 *   (A) base_cfl: real CFL (nf facilities x nc customers)
 *       Used to build TwoStageStochasticBlock for solving.
 *       Wrapped in StochasticBlock so CSSC Step 1 can inject scenarios.
 *
 *   (B) sr_cfl: N×N square CFL for scenario reduction bookkeeping
 *       NCustomers = N (= nb_atoms), NFacilities = K (= nb_reduced)
 *       Required by set_Block() type check and refresh_cached_data().
 *
 * Usage:
 *   ./cfl_cssc_test -i cap41.nc4 -n 20 -k 5 -v 1
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
#include "CSSCScenarioReductionSolver.h"
#include "ColVariable.h"
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
 string solver_config  = "BSPar1.txt";
 int    full_scenarios = 0;
 int    reduced_k      = 5;
 int    verbose        = 1;
};

Config parse_args( int argc , char * argv[] ) {
 Config cfg;
 for( int i = 1 ; i < argc ; ++i ) {
  string a = argv[i];
  if(      a=="-h" )             { cout<<"Usage: "<<argv[0]<<" -i <nc4> [-f <scen>] [-c <cfg>] [-n N] [-k K] [-v V]\n"; exit(0); }
  else if( a=="-i"&&i+1<argc )    cfg.instance_path  = argv[++i];
  else if( a=="-f"&&i+1<argc )    cfg.scenario_file  = argv[++i];
  else if( a=="-c"&&i+1<argc )    cfg.solver_config  = argv[++i];
  else if( a=="-n"&&i+1<argc )    cfg.full_scenarios = stoi(argv[++i]);
  else if( a=="-k"&&i+1<argc )    cfg.reduced_k      = stoi(argv[++i]);
  else if( a=="-v"&&i+1<argc )    cfg.verbose        = stoi(argv[++i]);
  else { cerr<<"Unknown: "<<a<<"\n"; exit(1); }
 }
 if( cfg.instance_path.empty() ){ cerr<<"Error: -i required\n"; exit(1); }
 return cfg;
}

// Build TwoStageStochasticBlock from base_cfl + dss, write/read netCDF    
unique_ptr<TwoStageStochasticBlock> build_tss(
  CapacitatedFacilityLocationBlock * base_cfl ,
  DiscreteScenarioSet * dss ,
  int nf , int nc , int verbose )
{
 const string tmp = "tmp_cssc_tss.nc4";
 {
  netCDF::NcFile f(tmp, netCDF::NcFile::replace);
  auto tg = f.addGroup("TwoStageStochasticBlock");
  tg.addDim("NumberScenarios", dss->get_poolSize());
  tg.putAtt("type","TwoStageStochasticBlock");

  { auto g=tg.addGroup("FirstStageBlock"); base_cfl->serialize(g); }

  auto sg=tg.addGroup("StaticAbstractPath");
  auto dp=sg.addDim("PathDim",nf);
  auto dl=sg.addDim("PathTotalLength",nf);
  vector<unsigned int> idx(nf); for(int i=0;i<nf;i++) idx[i]=i;
  sg.addVar("PathStart"         ,netCDF::NcUint(),dp).putVar(idx.data());
  sg.addVar("PathNodeTypes"     ,netCDF::NcChar(),dl).putVar(vector<char>(nf,'V').data());
  sg.addVar("PathGroupIndices"  ,netCDF::NcUint(),dl).putVar(vector<unsigned int>(nf,0).data());
  sg.addVar("PathElementIndices",netCDF::NcUint(),dl).putVar(idx.data());
  sg.addVar("PathRangeIndices"  ,netCDF::NcUint(),dl).putVar(idx.data());

  auto sb=tg.addGroup("StochasticBlock");
  sb.putAtt("type","StochasticBlock");
  { auto g=sb.addGroup("Block"); base_cfl->serialize(g); }
  { auto g=tg.addGroup("DiscreteScenarioSet"); dss->serialize(g); }

  auto nd=sb.addDim("NumberDataMappings",1);
  char dc='D'; sb.addVar("DataType",netCDF::NcChar(),nd).putVar(&dc);
  char cc='B'; sb.addVar("Caller",  netCDF::NcChar(),nd).putVar(&cc);
  string fn="CapacitatedFacilityLocationBlock::chg_customer_demands";
  sb.addVar("FunctionName",netCDF::NcString(),nd).putVar({0},&fn);
  auto ds=sb.addDim("SetSizeDim",2);
  vector<unsigned int> ss={0,0};
  sb.addVar("SetSize",netCDF::NcUint(),ds).putVar(ss.data());
  unsigned char ord=0;
  sb.addVar("Ordered",netCDF::NcUbyte(),nd).putVar(&ord);
  auto de=sb.addDim("SetElementsDim",4);
  vector<unsigned int> se={0,(unsigned int)nc,0,(unsigned int)nc};
  sb.addVar("SetElements",netCDF::NcUint(),de).putVar(se.data());

  auto ap=sb.addGroup("AbstractPath");
  ap.addDim("PathDim",1);
  auto zd=ap.addDim("PathTotalLength",0);
  unsigned int ps=0;
  ap.addVar("PathStart"         ,netCDF::NcUint(),ap.getDim("PathDim")).putVar(&ps);
  ap.addVar("PathNodeTypes"     ,netCDF::NcChar(),zd);
  ap.addVar("PathGroupIndices"  ,netCDF::NcUint(),zd);
  ap.addVar("PathElementIndices",netCDF::NcUint(),zd);
  ap.addVar("PathRangeIndices"  ,netCDF::NcUint(),zd);
 }
 netCDF::NcFile f2(tmp,netCDF::NcFile::read);
 auto tss=make_unique<TwoStageStochasticBlock>();
 tss->deserialize(f2.getGroup("TwoStageStochasticBlock"));
 f2.close(); remove(tmp.c_str());
 return tss;
}

/*--------------------------------------------------------------------------*/
tuple<double,long long,bool> solve_tss(
  TwoStageStochasticBlock * tss , const string & cfg_file )
{
 auto * raw=Configuration::deserialize(cfg_file);
 auto * bsc=dynamic_cast<BlockSolverConfig*>(raw);
 if(!bsc){delete raw;throw runtime_error("Bad solver config");}
 bsc->apply(tss); delete bsc;
 auto &sl=tss->get_registered_solvers();
 if(sl.empty()) throw runtime_error("No solver");
 auto t0=chrono::high_resolution_clock::now();
 int st=sl.front()->compute(false);
 auto dt=chrono::duration_cast<chrono::microseconds>(
   chrono::high_resolution_clock::now()-t0).count();
 if(st==Solver::kOK||st==Solver::kLowPrecision)
  return {sl.front()->get_ub(),dt,true};
 return {0.0,dt,false};
}

/*--------------------------------------------------------------------------*/
int main( int argc , char * argv[] ) {

 cout<<"\nTwo-Stage Stochastic CFL - CSSC Scenario Reduction\n\n";

 Config cfg=parse_args(argc,argv);

 try {
  // 1. Load CFL instance
  if(cfg.verbose>=1) cout<<"[1/6] Loading CFL instance\n";
  auto * raw=Block::deserialize(cfg.instance_path);
  auto * base_cfl=dynamic_cast<CapacitatedFacilityLocationBlock*>(raw);
  if(!base_cfl){delete raw;throw runtime_error("Not a CFL block");}
  if(!base_cfl->get_UnSplittable()) base_cfl->chg_UnSplittable(true);
  int nf=base_cfl->get_NFacilities();
  int nc=base_cfl->get_NCustomers();
  if(cfg.verbose>=1)
   cout<<"  "<<nf<<" facilities, "<<nc<<" customers\n\n";

  // 2. Load scenarios
  if(cfg.verbose>=1) cout<<"[2/6] Loading scenarios\n";
  string sf=cfg.scenario_file;
  if(sf.empty()){
   filesystem::path p(cfg.instance_path);
   sf="../scenarios/CFL/"+p.stem().string()+"_scenarios.nc4";
  }
  netCDF::NcFile scf(sf,netCDF::NcFile::read);
  auto dss=make_unique<DiscreteScenarioSet>();
  dss->deserialize(scf); scf.close();

  int N=cfg.full_scenarios>0
      ? min(cfg.full_scenarios,(int)dss->get_nbScenarios())
      : (int)dss->get_nbScenarios();
  dss->init_representative_pool(N);
  int K=cfg.reduced_k;
  if(K>=N) throw runtime_error("K must be < N");
  if(cfg.verbose>=1)
   cout<<"  N="<<N<<", K="<<K<<", dim="<<dss->get_scenario_size()<<"\n\n";

  // 3. Solve FULL TSS
  if(cfg.verbose>=1) cout<<"[3/6] Solving full TSS ("<<N<<" scenarios)\n";
  auto tss_full=build_tss(base_cfl,dss.get(),nf,nc,cfg.verbose);
  auto [obj_full,t_full,ok_full]=solve_tss(tss_full.get(),cfg.solver_config);
  if(!ok_full) throw runtime_error("Full solve failed");
  if(cfg.verbose>=1)
   cout<<"  Objective = "<<fixed<<setprecision(2)<<obj_full
       <<"  ("<<t_full<<" us)\n\n";

  // 4. CSSC reduction
  if(cfg.verbose>=1)
   cout<<"[4/6] CSSC reduction ("<<N<<" → "<<K<<")\n";

  // 4a. sr_cfl: K×N block so refresh_cached_data gives nb_atoms=N, nb_reduced=K
  auto * sr_cfl=new CapacitatedFacilityLocationBlock();
  {
   using DV=CapacitatedFacilityLocationBlock::DVector;
   using CV=CapacitatedFacilityLocationBlock::CVector;
   using CM=CapacitatedFacilityLocationBlock::CMatrix;
   size_t D=dss->get_scenario_size();
   DV caps(N,1.0);
   CV fcosts(N,0.0);
   DV dems(N,1.0/N);
   CM tcosts(boost::extents[N][N]);
   // Transport costs = Euclidean distance between scenarios in data space
   // Used by base class Wasserstein distance computation
   for(int i=0;i<N;i++)
    for(int j=0;j<N;j++){
     double d2=0;
     for(size_t d=0;d<D;d++){
      double dd=dss->get_scenario_value(i,d)-dss->get_scenario_value(j,d);
      d2+=dd*dd;
     }
     tcosts[i][j]=sqrt(d2);
    }
   // N×N block, NMaxFacilities=K so refresh_cached_data sets nb_reduced=K
   sr_cfl->load(N,N,caps,fcosts,dems,tcosts,true,K);
  }
  if(cfg.verbose>=2)
   cout<<"  sr_cfl: "<<sr_cfl->get_NFacilities()<<"(K) x "
       <<sr_cfl->get_NCustomers()<<"(N)\n";

  // 4b. Wrap base_cfl in StochasticBlock for scenario injection
  // compute_V_matrix() does: f_Block->get_f_Block() -> StochasticBlock
  // so stoch must be the parent block of sr_cfl
  auto * stoch=new StochasticBlock();
  stoch->set_inner_block(base_cfl,false);
  using SDM=SimpleDataMapping<Block::Range,Block::Range,double,Block>;
  const SDM::F * fptr=Block::get_method<SDM::F>(
    "CapacitatedFacilityLocationBlock::chg_customer_demands");
  if(!fptr) throw runtime_error("chg_customer_demands not in factory");
  stoch->add_data_mapping(make_unique<SDM>(
    fptr,base_cfl,Block::Range(0,nc),Block::Range(0,nc)));
  // Set stoch as parent of sr_cfl so that sr_cfl->get_f_Block() = stoch
  sr_cfl->set_f_Block(stoch);

  // 4c. CSSCComputeConfig
  auto * cssc_cfg=new CSSCComputeConfig();
  auto * milp_raw=Configuration::deserialize(cfg.solver_config);
  auto * milp_bsc=dynamic_cast<BlockSolverConfig*>(milp_raw);
  if(!milp_bsc){delete milp_raw;throw runtime_error("Bad MILP config");}
  cssc_cfg->f_extra_Configuration=milp_bsc;
  cssc_cfg->f_scenario_set=dss.get();

  // 4d. Configure and run CSSC
  CSSCScenarioReductionSolver cssc;
  cssc.set_Block(sr_cfl);
  cssc.set_ComputeConfig(cssc_cfg);
  delete cssc_cfg;
  if(cfg.verbose>=2) cssc.set_log(&cout);

  if(cfg.verbose>=1)
   cout<<"  nb_atoms="<<cssc.get_nb_atoms()
       <<"  nb_reduced="<<cssc.get_nb_reduced()<<"\n"
       <<"  Step 1: "<<N*N<<" sub-problems...\n";

  auto t0c=chrono::high_resolution_clock::now();
  int cst=cssc.compute();
  long long t_cssc=chrono::duration_cast<chrono::microseconds>(
    chrono::high_resolution_clock::now()-t0c).count();

  if(cst!=Solver::kOK)
   throw runtime_error("CSSC failed (status="+to_string(cst)+")");

  // 4e. Extract selected indices
  const auto & mask=cssc.get_reduced_atoms();
  vector<int> sel;
  for(int i=0;i<N;i++) if(i<(int)mask.size()&&mask[i]) sel.push_back(i);

  if(cfg.verbose>=1){
   cout<<"  CSSC done in "<<t_cssc<<" us\n  Selected:";
   for(int r:sel) cout<<" "<<r;
   cout<<"\n\n";
  }

  // 5. Solve REDUCED TSS
  if(cfg.verbose>=1)
   cout<<"[5/6] Solving reduced TSS ("<<K<<" scenarios)\n";

  // Aggregate probabilities using CSSC cluster assignment from x[i][j]
  // After get_var_solution(), x[rep][scen]=1.0 means scen assigned to rep
  // must call get_var_solution() first to populate x variables
  cssc.get_var_solution();
  vector<double> agg(N,0.0);
  for(int i=0;i<N;i++){
   // find which representative scenario i is assigned to
   int assigned_rep = sel[0]; // fallback
   for(int r:sel){
    ColVariable * xvar = sr_cfl->get_x(r, i);
    if(xvar && xvar->get_value() > 0.5){
     assigned_rep = r;
     break;
    }
   }
   agg[assigned_rep] += 1.0/N;
  }

  size_t D=dss->get_scenario_size();
  vector<vector<double>> red_sc;
  vector<double>         red_pr;
  for(int r:sel){
   vector<double> sc(D);
   for(size_t d=0;d<D;d++) sc[d]=dss->get_scenario_value(r,d);
   red_sc.push_back(sc);
   red_pr.push_back(agg[r]);
  }
  double tot=0; for(double p:red_pr) tot+=p;
  for(double &p:red_pr) p/=tot;

  auto dss_red=make_unique<DiscreteScenarioSet>();
  dss_red->load_from_memory(red_sc,red_pr);
  dss_red->init_representative_pool(K);

  auto tss_red=build_tss(base_cfl,dss_red.get(),nf,nc,cfg.verbose);
  auto [obj_red,t_red,ok_red]=solve_tss(tss_red.get(),cfg.solver_config);
  if(!ok_red) throw runtime_error("Reduced solve failed");
  if(cfg.verbose>=1)
   cout<<"  Objective = "<<fixed<<setprecision(2)<<obj_red
       <<"  ("<<t_red<<" us)\n\n";

  // 6. Report
  double diff=abs(obj_red-obj_full);
  double gap=diff/abs(obj_full)*100.0;
  double speedup=(double)t_full/max(1LL,t_red);

  cout<<"[6/6] Results\n"
      <<"  N = "<<N<<"  K = "<<K<<"\n\n"
      <<"  Timing\n"
      <<"    CSSC reduction : "<<t_cssc<<" us\n"
      <<"    Full solve     : "<<t_full<<" us\n"
      <<"    Reduced solve  : "<<t_red<<" us  (speedup "<<fixed<<setprecision(1)<<speedup<<"x)\n\n"
      <<"  Objectives\n"
      <<"    Full    : "<<fixed<<setprecision(2)<<obj_full<<"\n"
      <<"    Reduced : "<<fixed<<setprecision(2)<<obj_red<<"\n"
      <<"    Gap     : "<<setprecision(2)<<diff<<" ("<<gap<<"%)\n\n";

  // skip explicit delete, OS reclaims memory on exit
  // (avoids double-free from complex ownership between stoch/sr_cfl/base_cfl)
  return 0;

 } catch(const exception &e){
  cerr<<"\nError: "<<e.what()<<"\n"
      <<"Tips: run CFLScenarioGenerator first, BSPar1.txt in cwd, K<N\n";
  return 1;
 }
}
/*--------------------------------------------------------------------------*/
/*------------------- End File CFLCSSCScenarioReductionTest.cpp ------------*/
/*--------------------------------------------------------------------------*/