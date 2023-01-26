/**
 * @file
 * This file contains a (parametrized) test suite that solves a 1UC problem
 * described in a ThermalUnitBlock with both a ThermalUnitDPSolver and a
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

#include <FRealObjective.h>

#include <ThermalUnitBlock.h>
#include <CPXMILPSolver.h>
#include <ThermalUnitDPSolver.h>

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
 ThermalUnitDPSolver * tubgsolver{};
 CPXMILPSolver * milpsolver{};

 int init_t{};
 unsigned int time_horizon{};

 void SetUp() override {
  block = new ThermalUnitBlock();
  tubgsolver = new ThermalUnitDPSolver();
  milpsolver = new CPXMILPSolver();
  EXPECT_TRUE( block != nullptr );
  EXPECT_TRUE( tubgsolver != nullptr );
  EXPECT_TRUE( milpsolver != nullptr );

  std::string filename( GetParam().test_file );
  load_nc4( filename );

  time_horizon = block->get_time_horizon();

  // Generate init_t
  auto init_up_down_time = block->get_init_up_down_time();
  auto min_up_time = block->get_min_up_time();
  auto min_down_time = block->get_min_down_time();
  if( init_up_down_time > 0 ) {
   init_t = init_up_down_time >= min_up_time ?
            0 : ( int ) min_up_time - init_up_down_time;
  } else {
   init_t = -init_up_down_time >= min_down_time ?
            0 : ( int ) min_down_time + init_up_down_time;
  }

  srand48( seed );
 }

 void TearDown() override {
  block->unregister_Solvers(true);
  delete block;
 }

 // Solves the problem using the two solvers and compares the results
 void solve() {
  auto milp_status = milpsolver->compute( false );
  auto tubg_status = tubgsolver->compute( false );
  ASSERT_EQ( milp_status, tubg_status );

  auto milp_val = milpsolver->get_var_value();
  auto tubg_val = tubgsolver->get_var_value();
  auto abs_error = 1e-5 * std::max( double( 1 ),
                                    abs( std::max( milp_val, tubg_val ) ) );
  ASSERT_NEAR( milp_val, tubg_val, abs_error );
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
   return( s );
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

TEST_P ( TUB_Solver_Test, DPvsMILP ) {
 block->register_Solver( milpsolver );
 block->register_Solver( tubgsolver );
 solve();
}

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, CheckOFValue ) {

 block->register_Solver( tubgsolver );
 auto tubg_status = tubgsolver->compute( false );
 auto tubg_val = tubgsolver->get_var_value();

 // Check OF value
 tubgsolver->get_var_solution(nullptr);

 auto of = dynamic_cast<FRealObjective *>(block->get_objective());
 of->compute();
 auto of_value = of->value();

 std::cout << "DPSolver says " << tubg_val << std::endl;
 std::cout << "O.F. value is " << of_value << std::endl;
 auto abs_error = 1e-5 * std::max( double( 1 ),
                                   abs( std::max( of_value, tubg_val ) ) );
 ASSERT_NEAR( of_value, tubg_val, abs_error );
}

/*--------------------------------------------------------------------------*/

TEST_P ( TUB_Solver_Test, SetStartUpCostsSparse) {
 block->register_Solver( milpsolver );
 block->register_Solver( tubgsolver );

 while( num_repeats-- ) {

  auto values = block->get_start_up_cost();
  Block::Subset idx( 1, drand48() * 5 );
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
 block->register_Solver( milpsolver );
 block->register_Solver( tubgsolver );

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
 block->register_Solver( milpsolver );
 block->register_Solver( tubgsolver );

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

// TEST_P ( TUB_Solver_Test, SetQuadTermSparse) {
//  block->register_Solver( milpsolver );
//  block->register_Solver( tubgsolver );
//
//  while( num_repeats-- ) {
//
//   auto values = block->get_quad_term();
//   Block::Subset idx( 1, drand48() * values.size() );
//   std::vector< double > new_value( 1, values[idx[0]] * ( drand48() + 0.5 ) );
//
//   block->set_quad_term(new_value.begin(), std::move(idx));
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetQuadTermRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetAvailabilitySparse ) {
//  block->register_Solver( milpsolver );
//  block->register_Solver( tubgsolver );
//
//  while( num_repeats-- ) {
//
//   auto values = block->get_availability();
//   Block::Subset idx( 1, drand48() * values.size() );
//   std::vector< double > new_value( 1, values[idx[0]] * ( drand48() + 0.5 ) );
//
//   block->set_availability(new_value.begin(), std::move(idx));
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetAvailabilityRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetMaxPowerSparse ) {
//  block->register_Solver( milpsolver );
//  block->register_Solver( tubgsolver );
//
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetMaxPowerRanged ) {
//  while( num_repeats-- ) {
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

// TEST_P ( TUB_Solver_Test, SetInitPower ) {
//  block->register_Solver( milpsolver );
//  block->register_Solver( tubgsolver );
//
//  while( num_repeats-- ) {
//   auto old_value = block->get_initial_power();
//   std::vector< double > new_value( 1, old_value * ( drand48() + 0.5 ) );
//   Block::Subset idx( 1, 0 );
//
//   block->set_initial_power(new_value.begin(), std::move(idx));
//   solve();
//  }
// }

/*--------------------------------------------------------------------------*/

// TODO: ThermalUnitBlock::set_init_updown_time() not implemented
// TEST_P ( TUB_Solver_Test, SetInitUpDownTime ) {
//  block->register_Solver( tubgsolver );
//
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
                           TestParameters{ "data/24/S1ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S1ramp100_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S12ramp100_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S16ramp100_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp1_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp2_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp3_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp4_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp5_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp6_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp7_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp8_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp9_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp10_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp11_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp12_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp13_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp14_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp15_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp16_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp17_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp18_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp19_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp20_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp21_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp22_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp23_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp24_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp25_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp26_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp27_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp28_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp29_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp30_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp31_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp32_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp33_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp34_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp35_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp36_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp37_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp38_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp39_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp40_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp41_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp42_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp43_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp44_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp45_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp46_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp47_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp48_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp49_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp50_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp51_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp52_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp53_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp54_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp55_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp56_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp57_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp58_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp59_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp60_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp61_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp62_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp63_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp64_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp65_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp66_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp67_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp68_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp69_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp70_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp71_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp72_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp73_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp74_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp75_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp76_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp77_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp78_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp79_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp80_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp81_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp82_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp83_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp84_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp85_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp86_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp87_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp88_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp89_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp90_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp91_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp92_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp93_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp94_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp95_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp96_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp97_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp98_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp99_24.nc4", 0, 5, 5 },
                           TestParameters{ "data/24/S23ramp100_24.nc4", 0, 5, 5 }
                          ),
                          TUB_Solver_Test::PrintToStringParamName() );

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return( RUN_ALL_TESTS() );
}
