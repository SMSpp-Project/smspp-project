/**
 * @file
 * This file contains a (parametrized) test suite that solves a MCF problem
 * with both a MCFSolver and a MILPSolver and compares the results; the tests
 * explore all MCFBlock modifications.
 *
 * The suite uses the Google Test framework, see:
 * https://github.com/google/googletest
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Niccolo' Iardella \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Niccolo' Iardella
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINES -----------------------------------*/
/*--------------------------------------------------------------------------*/
/* If any of the following macros is defined, then the corresponding
 * :MCFClass solver is included and the corresponding version of
 * MCFSolver<> can be tested.
 *
 * - HAVE_CSCL2      for the CS2 class
 *
 * - HAVE_CPLEX      for the MCFCplex class
 *
 * - HAVE_MFSMX      for the MCFSimplex class
 *
 * - HAVE_MFZIB      for the MCFZIB class
 *
 * - HAVE_RELAX      for the RelaxIV class
 *
 * - HAVE_CPLEX      for the MCFCplex class
 *
 * - HAVE_SPTRE      for the SPTree class; note that SPTree cannot solve
 *                   most MCF instances, except those with SPT structure
 *
 * Thus, the choice of the specific :MCFClass solver can be done in the
 * makefile with a simple -DHAVE_* argument to the compiler.
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <gtest/gtest.h>
#include <iomanip>
#include <netcdf>

#ifdef HAVE_CSCL2
#include "CS2.h"
#define MCFC CS2
#endif

#ifdef HAVE_CPLEX
#include "MCFCplex.h"
#define MCFC MCFCplex
#endif

#ifdef HAVE_MFSMX
#include "MCFSimplex.h"
#define MCFC MCFSimplex
#endif

#ifdef HAVE_MFZIB
#include "MCFZIB.h"
#define MCFC MCFZIB
#endif

#ifdef HAVE_RELAX
#include "RelaxIV.h"
#define MCFC RelaxIV
#endif

#ifdef HAVE_SPTRE
#include "SPTree.h"
#define MCFC SPTree
#endif

#include "MCFBlock.h"
#include "MCFSolver.h"
#include "SCIPMILPSolver.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

#if( OPT_USE_NAMESPACES )
using namespace MCFClass_di_unipi_it;
#else
using namespace std;
#endif
using namespace SMSpp_di_unipi_it;
/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

// FIXME: Avoid these declarations
template<> const std::vector< int > MCFSolver< MCFC >::Solver_2_MCFClass_int;
template<> const std::vector< int > MCFSolver< MCFC >::Solver_2_MCFClass_dbl;

/*--------------------------------------------------------------------------*/
/*----------------------- PARAMETERIZED TEST FIXTURE -----------------------*/
/*--------------------------------------------------------------------------*/

// Structure containing the input parameters, for convenience
struct TestParameters {
 std::string test_file;
 long int seed;
 unsigned int max_changes;
 unsigned int num_repeats;
};

/*
 * Main test fixture
 *
 * A test fixture is a class used when multiple tests in a suite share code.
 * This fixture is parameterized, it will be instantiated with different input
 * parameters using the INSTANTIATE_TEST_SUITE_P() macro.
 */
class MCFMILPTest :
 public ::testing::TestWithParam< TestParameters > {
 protected:
 MCFMILPTest() {
  seed = GetParam().seed;
  num_repeats = GetParam().num_repeats;
  max_changes = GetParam().max_changes;
 }

 ~MCFMILPTest() override = default;

 long int seed;
 unsigned int max_changes;
 unsigned int num_repeats;

 MCFBlock * mcfb{};
 MCFClass::Index m{};
 MCFClass::Index n{};

 MCFClass::CNumber c_max{};
 MCFClass::CNumber c_min{};
 MCFClass::FNumber u_avg{};
 MCFClass::FNumber u_min{};
 bool nz_deficits = false;

 void SetUp() override {
  mcfb = dynamic_cast<MCFBlock *>( Block::new_Block( "MCFBlock" ));
  EXPECT_TRUE( mcfb != nullptr );

  std::string filename( GetParam().test_file );
  load_nc4( filename );

  mcfb->generate_abstract_constraints();
  mcfb->generate_objective();
  compute_costs_deficits();

  auto * mcfsolver = new MCFSolver< MCFC >();
  auto * milpsolver = new SCIPMILPSolver();

  mcfb->register_Solver( milpsolver );
  mcfsolver->set_par( Solver::dblAbsAcc, u_avg * 1e-8 );
  mcfb->register_Solver( mcfsolver );

  srand48( seed );
 }

 void TearDown() override {
  delete mcfb;
 }

 // Solves the problem using the two solvers and compares the results
 void solve() {
  Solver * milpsolver = mcfb->get_registered_solvers().front();
  Solver * mcfsolver = mcfb->get_registered_solvers().back();

  auto t1 = milpsolver->compute_async( false );
  auto t2 = mcfsolver->compute_async( false );

  t1.wait();
  t2.wait();

  auto milp_status = t1.get();
  auto mcf_status = t2.get();

  EXPECT_EQ( milp_status, mcf_status );

  auto milp_ub = milpsolver->get_ub();
  auto mcf_ub = mcfsolver->get_ub();

  if( milp_ub == std::numeric_limits< double >::infinity() ) {
   EXPECT_EQ( milp_ub, mcf_ub );
  } else {
   auto abs_error = 1e-9 * max( double( 1 ), abs( max( milp_ub, mcf_ub ) ) );
   EXPECT_NEAR( milp_ub, mcf_ub, abs_error );
  }
 }

 // Returns a random number between 0.5 and 2, with 50% prob. of being < 1
 static inline double rndfctr() {
  double fctr = drand48() - 0.5;
  return ( fctr < 0 ? -fctr : fctr * 4 );
 }

 public:

 // Generates a meaningful test name for each instance
 struct PrintToStringParamName {
  template< class ParamType >
  std::string
  operator()( const testing::TestParamInfo< ParamType > & info ) const {
   auto s = static_cast<TestParameters>(info.param).test_file;
   // Test names must be non-empty, unique, and may only contain ASCII
   // alphanumeric characters or underscore.
   std::replace( s.begin(), s.end(), '/', '_' );
   std::replace( s.begin(), s.end(), '.', '_' );
   std::replace( s.begin(), s.end(), '-', '_' );
   return s;
  }
 };

 private:

 // Loads a nc4 file
 void load_nc4( std::string & filename ) {
  netCDF::NcFile f( filename, netCDF::NcFile::read );
  ASSERT_FALSE( f.isNull() );

  netCDF::NcGroupAtt gtype = f.getAtt( "SMS++_file_type" );
  ASSERT_FALSE( gtype.isNull() );

  int type = 0;
  gtype.getValues( &type );
  ASSERT_EQ( type, eBlockFile );

  netCDF::NcGroup bg = f.getGroup( "Block_0" );
  ASSERT_FALSE( bg.isNull() );

  mcfb->deserialize( bg );
 }

 // Computes costs and deficits from the MCFBlock
 void compute_costs_deficits() {
  m = mcfb->get_NArcs();
  n = mcfb->get_NNodes();

  if( max_changes > m ) {
   max_changes = m;
  }

  c_max = -OPTtypes_di_unipi_it::Inf< MCFClass::CNumber >();
  c_min = -c_max;
  u_avg = 0;
  u_min = OPTtypes_di_unipi_it::Inf< MCFClass::FNumber >();

  for( MCFClass::Index i = 0; i < m; i++ ) {
   MCFClass::cCNumber ci = mcfb->get_C( i );
   if( ci < c_min ) c_min = ci;
   if( ci > c_max ) c_max = ci;
   MCFClass::cFNumber ui = mcfb->get_U( i );
   u_avg += ui;
   if( ui < u_min ) u_min = ui;
  }

  u_avg /= m;
  nz_deficits = false;

  for( MCFClass::Index i = 0; i < n; i++ ) {
   if( mcfb->get_B( i ) > 0 ) {
    nz_deficits = true;
    break;
   }
  }
 }
};

/*--------------------------------------------------------------------------*/
/*--------------------------- PARAMETERIZED TESTS --------------------------*/
/*--------------------------------------------------------------------------*/

/*
 * Each test (case) should test a single functionality of the class/system
 * being tested. The syntax is: TEST_P(TestFixtureName, TestName)
 */

TEST_P ( MCFMILPTest, SimpleSolve ) {
 solve();
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeOneCostAbstract ) {

 while( num_repeats-- ) {
  auto newcst = c_min + MCFBlock::CNumber( drand48() * ( c_max - c_min ) );
  auto arc = MCFBlock::Index( drand48() * ( m - 1 ) );

  auto * obj = dynamic_cast<FRealObjective *>( mcfb->get_objective() );
  auto * lf = dynamic_cast<LinearFunction *>( obj->get_function() );
  LinearFunction::v_coeff nc = { newcst };
  lf->modify_coefficient( arc, nc.front() );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeOneCostPhysical ) {

 while( num_repeats-- ) {
  auto newcst = c_min + MCFBlock::CNumber( drand48() * ( c_max - c_min ) );
  auto arc = MCFBlock::Index( drand48() * ( m - 1 ) );
  mcfb->chg_cost( newcst, arc );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCostsRangedAbstract ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_CNumber newcsts( tochange );
  for( MCFBlock::Index i = 0; i < tochange; i++ ) {
   newcsts[ i ] = c_min + MCFBlock::CNumber( drand48() * ( c_max - c_min ) );
  }

  MCFBlock::Index strt = drand48() * ( m - tochange );
  MCFBlock::Index stp = strt + tochange;

  auto * obj = dynamic_cast<FRealObjective *>( mcfb->get_objective() );
  auto * lf = dynamic_cast<LinearFunction *>( obj->get_function() );

  lf->modify_coefficients( std::move( newcsts ), Function::Range( strt, stp ) );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCostsRangedPhysical ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_CNumber newcsts( tochange );
  for( MCFBlock::Index i = 0; i < tochange; i++ ) {
   newcsts[ i ] = c_min + MCFBlock::CNumber( drand48() * ( c_max - c_min ) );
  }

  MCFBlock::Index strt = drand48() * ( m - tochange );
  MCFBlock::Index stp = strt + tochange;

  mcfb->chg_costs( newcsts.begin(), Block::Range( strt, stp ) );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCostsSparseAbstract ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_CNumber newcsts( tochange );
  for( MCFBlock::Index i = 0; i < tochange; i++ ) {
   newcsts[ i ] = c_min + MCFBlock::CNumber( drand48() * ( c_max - c_min ) );
  }

  Block::Subset nms( m );
  std::iota( nms.begin(), nms.end(), 0 );

  for( Block::Index i = 0; i < tochange; i++ )
   swap( nms[ i ], nms[ i + drand48() * ( m - i ) ] );

  auto end = nms.begin() + tochange;
  sort( nms.begin(), end );
  nms.resize( tochange );

  auto * obj = dynamic_cast<FRealObjective *>( mcfb->get_objective() );
  auto * lf = dynamic_cast<LinearFunction *>( obj->get_function() );

  lf->modify_coefficients( std::move( newcsts ),
                           std::move( nms ),
                           true );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCostsSparsePhysical ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_CNumber newcsts( tochange );
  for( MCFBlock::Index i = 0; i < tochange; i++ ) {
   newcsts[ i ] = c_min + MCFBlock::CNumber( drand48() * ( c_max - c_min ) );
  }

  Block::Subset nms( m );
  std::iota( nms.begin(), nms.end(), 0 );

  for( Block::Index i = 0; i < tochange; i++ )
   swap( nms[ i ], nms[ i + drand48() * ( m - i ) ] );

  auto end = nms.begin() + tochange;
  sort( nms.begin(), end );
  nms.resize( tochange );

  mcfb->chg_costs( newcsts.begin(), std::move( nms ), true );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeOneCapacityAbstract ) {

 while( num_repeats-- ) {
  auto arc = MCFBlock::Index( drand48() * ( m - 1 ) );
  MCFBlock::CNumber newcap = mcfb->get_U( arc ) * rndfctr();

  mcfb->i2p_ub( arc )->set_rhs( newcap );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeOneCapacityPhysical ) {

 while( num_repeats-- ) {
  auto arc = MCFBlock::Index( drand48() * ( m - 1 ) );
  MCFBlock::CNumber newcap = mcfb->get_U( arc ) * rndfctr();

  mcfb->chg_ucap( newcap, arc );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCapacitiesRangedAbstract ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_FNumber newcaps( tochange );

  MCFBlock::Index strt = drand48() * ( m - tochange );
  MCFBlock::Index stp = strt + tochange;
  for( MCFBlock::Index i = 0; i < tochange; ++i )
   newcaps[ i ] = mcfb->get_U( i + strt ) * rndfctr();

  for( MCFBlock::Index i = 0; i < tochange; ++i )
   mcfb->i2p_ub( i + strt )->set_rhs( newcaps[ i ] );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCapacitiesRangedPhysical ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_FNumber newcaps( tochange );

  MCFBlock::Index strt = drand48() * ( m - tochange );
  MCFBlock::Index stp = strt + tochange;
  for( MCFBlock::Index i = 0; i < tochange; ++i )
   newcaps[ i ] = mcfb->get_U( i + strt ) * rndfctr();

  mcfb->chg_ucaps( newcaps.begin(), Block::Range( strt, stp ) );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCapacitiesSparseAbstract ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_FNumber newcaps( tochange );

  Block::Subset nms( m );
  std::iota( nms.begin(), nms.end(), 0 );

  for( Block::Index i = 0; i < tochange; i++ ) {
   swap( nms[ i ], nms[ i + drand48() * ( m - i ) ] );
   newcaps[ i ] = mcfb->get_U( nms[ i ] ) * rndfctr();
  }

  auto end = nms.begin() + tochange;
  sort( nms.begin(), end );
  nms.resize( tochange );

  for( MCFBlock::Index i = 0; i < tochange; ++i )
   mcfb->i2p_ub( nms[ i ] )->set_rhs( newcaps[ i ] );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeCapacitiesSparsePhysical ) {

 while( num_repeats-- ) {
  MCFBlock::Index tochange = max( double( 1 ), drand48() * max_changes );
  MCFBlock::Vec_FNumber newcaps( tochange );

  Block::Subset nms( m );
  std::iota( nms.begin(), nms.end(), 0 );

  for( Block::Index i = 0; i < tochange; i++ ) {
   swap( nms[ i ], nms[ i + drand48() * ( m - i ) ] );
   newcaps[ i ] = mcfb->get_U( nms[ i ] ) * rndfctr();
  }

  auto end = nms.begin() + tochange;
  sort( nms.begin(), end );
  nms.resize( tochange );

  mcfb->chg_ucaps( newcaps.begin(), std::move( nms ), true );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeDeficitsAbstract ) {

 while( num_repeats-- ) {
  MCFClass::Index posn = 0;
  MCFClass::Index negn = 0;
  MCFClass::FNumber posd = NAN;
  MCFClass::FNumber negd = NAN;

  if( nz_deficits ) {
   MCFBlock::Vec_FNumber dfcts( n );

   do
    posn = MCFClass::Index( drand48() * n ); // select node with positive
   while( mcfb->get_B( posn ) <= 0 );         // deficit (one must exist)
   posd = mcfb->get_B( posn );

   do
    negn = MCFClass::Index( drand48() * n ); // select node with negative
   while( mcfb->get_B( negn ) >= 0 );         // deficit (one must exist)
   negd = mcfb->get_B( negn );
  } else {
   posn = MCFClass::Index( drand48() * n );  // just select at random
   negn = MCFClass::Index( drand48() * n );
   posd = negd = 0;
  }

  MCFClass::FNumber Dlt = std::ceil( u_avg * 2 * drand48() );
  if( drand48() <= 0.5 ) {  // in 50% of cases up, in 50% of cases down
   posd += Dlt;
   negd -= Dlt;
  } else {
   Dlt = min( Dlt, max( max( posd, -negd ) / 2, double( 1 ) ) );
   posd -= Dlt;
   negd += Dlt;
  }

  mcfb->i2p_e( posn )->set_both( posd );
  mcfb->i2p_e( negn )->set_both( negd );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, ChangeDeficitsPhysical ) {

 while( num_repeats-- ) {
  MCFClass::Index posn = 0;
  MCFClass::Index negn = 0;
  MCFClass::FNumber posd = NAN;
  MCFClass::FNumber negd = NAN;

  if( nz_deficits ) {
   MCFBlock::Vec_FNumber dfcts( n );

   do
    posn = MCFClass::Index( drand48() * n ); // select node with positive
   while( mcfb->get_B( posn ) <= 0 );         // deficit (one must exist)
   posd = mcfb->get_B( posn );

   do
    negn = MCFClass::Index( drand48() * n ); // select node with negative
   while( mcfb->get_B( negn ) >= 0 );         // deficit (one must exist)
   negd = mcfb->get_B( negn );
  } else {
   posn = MCFClass::Index( drand48() * n );  // just select at random
   negn = MCFClass::Index( drand48() * n );
   posd = negd = 0;
  }

  MCFClass::FNumber Dlt = std::ceil( u_avg * 2 * drand48() );
  if( drand48() <= 0.5 ) {  // in 50% of cases up, in 50% of cases down
   posd += Dlt;
   negd -= Dlt;
  } else {
   Dlt = min( Dlt, max( max( posd, -negd ) / 2, double( 1 ) ) );
   posd -= Dlt;
   negd += Dlt;
  }

  mcfb->chg_dfct( posd, posn );
  mcfb->chg_dfct( negd, negn );

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, CloseOpenArcsAbstract ) {

 while( num_repeats-- ) {
  MCFBlock::Subset nms( max_changes );
  MCFBlock::Index changed = 0;

  // Arcs to change
  for( MCFBlock::Index i = mcfb->get_NStaticArcs();
       i < mcfb->get_NArcs(); ++i ) {

   if( mcfb->is_deleted( i ) )
    continue;
   if( mcfb->is_closed( i ) )
    continue;
   if( drand48() <= 0.5 )
    continue;

   nms[ changed++ ] = i;
   if( changed >= max_changes )
    break;
  }

  // Close
  if( changed ) {
   nms.resize( changed );
   for( auto i : nms ) {
    auto * x = mcfb->i2p_x( i );
    x->set_value( 0 );
    x->is_fixed( true );
   }
  }

  solve();

  // Re-open
  if( changed ) {
   for( auto i : nms )
    mcfb->i2p_x( i )->is_fixed( false );
  }

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, CloseOpenArcsPhysical ) {

 while( num_repeats-- ) {
  MCFBlock::Subset nms1( max_changes );
  MCFBlock::Subset nms2( max_changes );
  MCFBlock::Index changed = 0;

  // Arcs to change
  for( MCFBlock::Index i = mcfb->get_NStaticArcs();
       i < mcfb->get_NArcs(); ++i ) {

   if( mcfb->is_deleted( i ) )
    continue;
   if( mcfb->is_closed( i ) )
    continue;
   if( drand48() <= 0.5 )
    continue;

   nms1[ changed ] = i;
   nms2[ changed ] = i;
   changed++;
   if( changed >= max_changes )
    break;
  }

  // Close
  if( changed ) {
   nms1.resize( changed );
   mcfb->close_arcs( std::move( nms1 ) );
  }

  solve();

  // Re-open
  if( changed ) {
   nms2.resize( changed );
   mcfb->open_arcs( std::move( nms2 ) );
  }

  solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, DeleteArcs ) {

 while( num_repeats-- ) {
  MCFBlock::Index changed = 0;

  if( drand48() < 0.5 ) {
   // delete somewhere in the middle

   for( MCFBlock::Index i = mcfb->get_NStaticArcs();
        i < mcfb->get_NArcs(); ++i ) {
    if( mcfb->is_deleted( i ) )
     continue;
    if( drand48() <= 0.75 )
     continue;

    ASSERT_NO_THROW( mcfb->remove_arc( i ) );
    ++changed;
    if( changed >= max_changes )
     break;
   }

  } else {
   for( MCFBlock::Index i = mcfb->get_NArcs();
        --i >= mcfb->get_NStaticArcs(); ) {
    if( mcfb->is_deleted( i ) )
     continue;
    if( drand48() <= 0.13 )
     break;

    ASSERT_NO_THROW( mcfb->remove_arc( i ) );
    ++changed;
    if( changed >= max_changes )
     break;
   }
  }
   solve();
 }
}

/*--------------------------------------------------------------------------*/

TEST_P ( MCFMILPTest, AddNewArcs ) {

 while( num_repeats-- ) {
  MCFBlock::Index changed = 0;
  MCFBlock::Index afterend = 0;
  while( changed < max_changes ) {
   if( drand48() <= 0.13 )
    break;

   ++changed;

   MCFBlock::Index sn = 0, en = 0;
   do {
    sn = drand48() * mcfb->get_NNodes() + 1;
    en = drand48() * mcfb->get_NNodes() + 1;
   } while( sn == en );

   // random cost in [ - c_max , c_max ]
   auto cst = c_max * ( 1 - 2 * drand48() );

   // random capacity <= 0.75 u_avg
   auto cap = 1.5 * ( u_avg - u_min ) * drand48() + u_min;

   auto arc = mcfb->add_arc( sn, en, cst, cap );

   if( arc >= m )
    ++afterend;

   if( mcfb->get_NArcs() >= mcfb->get_MaxNArcs() )
    break;
  }

  solve();
 }
}

/*--------------------------------------------------------------------------*/
/*------------------------- TEST SUITE INSTANCES ---------------------------*/
/*--------------------------------------------------------------------------*/

/*
 * With INSTANTIATE_TEST_SUITE_P(), multiple test suites, each with different
 * input parameters are generated. The syntax is:
 *
 * INSTANTIATE_TEST_SUITE_P(InstantiationName,
 *                          TestFixtureName,
 *                          testing::Values("meeny", "miny", "moe"));
 */

INSTANTIATE_TEST_SUITE_P( MCFMILPTests,
                          MCFMILPTest,
                          ::testing::Values(  // file, seed, changes, repeats
                           // TestParameters{ "data/N3-0-0-0-0.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-0-0-0-1.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-0-0-0-5.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-0-0-1-0.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-0-0-5-0.nc4", 0, 10, 10 },
                           TestParameters{ "data/N3-0-1-0-0.nc4", 1, 1, 1 }
                           // TestParameters{ "data/N3-0-5-0-0.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-1-0-0-0.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-1-1-0-0.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-1-1-1-1.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-5-0-0-0.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-5-5-0-0.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-5-5-1-1.nc4", 0, 10, 10 },
                           // TestParameters{ "data/N3-5-5-2-2.nc4", 0, 10, 10 }
                          ),
                          MCFMILPTest::PrintToStringParamName() );

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
