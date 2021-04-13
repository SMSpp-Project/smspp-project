/**
 * @file
 * This file contains a (parametrized) test suite that solves a 1UC problem
 * described in a ThermalUnitBlock with both a ThermalUnitBlockDPSolver and a
 * MILPSolver and compares the results;
 * the tests explore all ThermalUnitBlock modifications.
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
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <gtest/gtest.h>
#include <iomanip>
#include <netcdf>

#include <LinearFunction.h>
#include <FRealObjective.h>

#include <ThermalUnitBlock.h>
#include <CPXMILPSolver.h>
#include <legacy_dpsolver/LegacyDPSolver.h>

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

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
class TUB_Solver_Test :
 public ::testing::TestWithParam< TestParameters > {
 protected:
 TUB_Solver_Test() : seed( GetParam().seed ),
                     num_repeats( GetParam().num_repeats ),
                     max_changes( GetParam().max_changes ) {}

 ~TUB_Solver_Test() override = default;

 long int seed;
 unsigned int max_changes;
 unsigned int num_repeats;

 ThermalUnitBlock * block{};

 Solver * dpsolver{};
 Solver * milpsolver{};

 int init_t{};
 int time_horizon{};

 void SetUp() override {
  block = new ThermalUnitBlock();
  EXPECT_TRUE( block != nullptr );

  std::string filename( GetParam().test_file );
  load_nc4( filename );

  dpsolver = new LegacyDPSolver();
  milpsolver = new CPXMILPSolver();

  block->register_Solver( milpsolver );
  block->register_Solver( dpsolver );

  // Generate init_t
  if( block->get_init_up_down_time() > 0 ) {
   init_t = block->get_init_up_down_time() >= block->get_min_up_time() ? 0 : block->get_min_up_time() - block->get_init_up_down_time();
  } else {
   init_t = -block->get_init_up_down_time() >= block->get_min_down_time() ? 0 : block->get_min_down_time() + block->get_init_up_down_time();
  }

  time_horizon = block->get_time_horizon();

  srand48( seed );
 }

 void TearDown() override {
  block->unregister_Solvers(true);
  delete block;
 }

 // Solves the problem using the two solvers and compares the results
 void solve() {
  Solver * s1 = block->get_registered_solvers().front();
  Solver * s2 = block->get_registered_solvers().back();

  auto milp_status = s1->compute( false );
  auto dp_status = s2->compute( false );
  //
  // auto t1 = s1->compute_async( false );
  // auto t2 = s2->compute_async( false );
  //
  // t1.wait();
  // t2.wait();

  // auto milp_status = t1.get();
  // auto dp_status = t2.get();

  EXPECT_EQ( milp_status, dp_status );

  auto milp_ub = s1->get_ub();
  auto dp_ub = s2->get_ub();

  if( milp_ub == std::numeric_limits< double >::infinity() ) {
   EXPECT_EQ( milp_ub, dp_ub );
  } else {
   auto abs_error = 1e-5 * max( double( 1 ), abs( max( milp_ub, dp_ub ) ) );
   EXPECT_NEAR( milp_ub, dp_ub, abs_error );
  }
 }

 // Returns a random number between 0.5 and 2, with 50% prob. of being < 1
 // static inline double rndfctr() {
 //  double fctr = drand48() - 0.5;
 //  return ( fctr < 0 ? -fctr : fctr * 4 );
 // }

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

  block->deserialize( bg );
 }
};

/*--------------------------------------------------------------------------*/
/*--------------------------- PARAMETERIZED TESTS --------------------------*/
/*--------------------------------------------------------------------------*/

/*
 * Each test (case) should test a single functionality of the class/system
 * being tested. The syntax is: TEST_P(TestFixtureName, TestName)
 */

TEST_P ( TUB_Solver_Test, SimpleSolve ) {
 solve();
}

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetStartUpCostsSparse) {

 while( num_repeats-- ) {

  auto values = block->get_start_up_cost();
  Block::Subset idx( 1, drand48() * values.size() );
  std::vector< double > new_value( 1, values[idx[0]] * ( drand48() + 0.5 ) );

  block->set_startup_costs(new_value.begin(), std::move(idx));
  solve();
 }
}

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetStartUpCostsRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetConstTermSparse) {

 while( num_repeats-- ) {

  auto values = block->get_const_term();
  Block::Subset idx( 1, drand48() * values.size() );
  std::vector< double > new_value( 1, values[idx[0]] * ( drand48() + 0.5 ) );

  block->set_const_term(new_value.begin(), std::move(idx));
  solve();
 }
}

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetConstTermRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetLinearTermSparse) {

 while( num_repeats-- ) {

  auto values = block->get_linear_term();
  Block::Subset idx( 1, drand48() * values.size() );
  std::vector< double > new_value( 1, values[idx[0]] * ( drand48() + 0.5 ) );

  block->set_linear_term(new_value.begin(), std::move(idx));
  solve();
 }
}

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetLinearTermRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetQuadTermSparse) {

 while( num_repeats-- ) {

  auto values = block->get_quad_term();
  Block::Subset idx( 1, drand48() * values.size() );
  std::vector< double > new_value( 1, values[idx[0]] * ( drand48() + 0.5 ) );

  block->set_quad_term(new_value.begin(), std::move(idx));
  solve();
 }
}

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetQuadTermRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetAvailabilitySparse ) {
 while( num_repeats-- ) {

  auto values = block->get_availability();
  Block::Subset idx( 1, drand48() * values.size() );
  std::vector< double > new_value( 1, values[idx[0]] * ( drand48() + 0.5 ) );

  block->set_availability(new_value.begin(), std::move(idx));
  solve();
 }
}

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetAvailabilityRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetMaxPowerSparse ) {
 while( num_repeats-- ) {
  solve();
 }
}

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetMaxPowerRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetInitPower ) {
 while( num_repeats-- ) {
  auto old_value = block->get_initial_power();
  std::vector< double > new_value( 1, old_value * ( drand48() + 0.5 ) );
  Block::Subset idx( 1, 0 );

  block->set_initial_power(new_value.begin(), std::move(idx));
  solve();
 }
}

/*--------------------------------------------------------------------------*/

// TODO: ThermalUnitBlock::set_init_updown_time() not implemented
// TEST_P ( TUB_Solver_Test, SetInitUpDownTime ) {
//  while( num_repeats-- ) {
//   int old_value = ( int ) block->get_init_up_down_time();
//   std::vector< int > new_value( 1 );
//   Block::Subset idx( 1, 0 );
//
//   double fctr = drand48() - 0.5;
//   if( fctr < 0 ) {
//    new_value[ 0 ] = old_value - 1;
//   } else {
//    new_value[ 0 ] = old_value + 1;
//   }
//
//   block->set_init_updown_time( new_value.begin(), std::move( idx ) );
//   solve();
//  }
// }

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

INSTANTIATE_TEST_SUITE_P( TUB_Solver_Tests,
                          TUB_Solver_Test,
                          ::testing::Values(  // file, seed, changes, repeats
                           TestParameters{ "data/S1ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S1ramp100_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S12ramp100_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S16ramp100_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/S23ramp100_24.nc4", 0, 5, 5 }
                          ),
                          TUB_Solver_Test::PrintToStringParamName() );

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
