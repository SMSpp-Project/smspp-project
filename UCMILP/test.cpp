/**
 * @file
 * This file contains a (parametrized) test suite that solves a UC problem
 * with CPXMILPSolver and compares the results against known results.
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

#include <gtest/gtest.h>
#include <netcdf>
#include <fstream>

#include <UCBlock.h>
#include <RBlockConfig.h>
#include <BlockSolverConfig.h>

#include <HydroSystemUnitBlock.h>

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------- PARAMETERIZED TEST FIXTURE -----------------------*/
/*--------------------------------------------------------------------------*/

// Structure containing the input parameters, for convenience
struct TestParameter {
 std::string test_file;
 double ub;
} __attribute__((aligned(32)));

/*
 * Main test fixture
 *
 * A test fixture is a class used when multiple tests in a suite share code.
 * This fixture is parameterized, it will be instantiated with different input
 * parameters using the INSTANTIATE_TEST_SUITE_P() macro.
 */
class UCMILPTest :
 public ::testing::TestWithParam< TestParameter > {
 protected:

 // Constructor
 UCMILPTest() = default;

 // Destructor
 ~UCMILPTest() override = default;

 UCBlock * block{}; // Main block

 // Setup method
 void SetUp() override {
  block = new UCBlock();
  EXPECT_TRUE( block != nullptr );

  std::string filename( GetParam().test_file );
  load_nc4( filename );

  // Load block configuration
  BlockConfig * b_config = default_configure_ucblock( block );
  b_config->apply( block );
  delete b_config;

  // Load solver configuration
  std::ifstream scf;
  scf.open( "uc_solverconfig.txt", std::ifstream::in );
  ASSERT_TRUE( scf.is_open() );

  std::string name;
  scf >> eatcomments >> name;
  auto * s_config = dynamic_cast<BlockSolverConfig *> ( Configuration::new_Configuration( name ) );
  ASSERT_TRUE( s_config );
  ASSERT_NO_THROW( scf >> *s_config );
  s_config->apply( block );
  delete s_config;
 }

 void TearDown() override {
  delete block;
 }

 private:

 // Loads a nc4 file into the block
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

 // Returns a default block configuration
 static BlockConfig * default_configure_ucblock( Block * uc_block ) {
  auto b_config = new RBlockConfig;

  for( auto sb: uc_block->get_nested_Blocks() ) {
   if( !dynamic_cast<UnitBlock *>( sb ) ) {
    continue;
   }

   auto sbc = new RBlockConfig;

   // If HydroSystemUnitBlock, we configure its PolyhedralFunctionBlocks
   auto hu_block = dynamic_cast<HydroSystemUnitBlock *>( sb );

   if( hu_block != nullptr ) {
    auto num_nested_blocks_hydro = hu_block->get_number_nested_Blocks();
    for( auto ssb: hu_block->get_nested_Blocks() ) {

     auto pf_block = dynamic_cast<PolyhedralFunctionBlock *>( ssb );

     if( pf_block != nullptr ) {
      auto ssbc = new BlockConfig();
      ssbc->f_static_variables_Configuration =
       new SimpleConfiguration< int >( 1 );

      int idx = sb->get_nested_Block_index( ssb );
      sbc->add_sub_BlockConfig( ssbc, idx );
     }
    }
   }

   int idx = uc_block->get_nested_Block_index( sb );
   b_config->add_sub_BlockConfig( sbc, idx );
  }

  return b_config;
 }

 public:

 // Generates a meaningful test name for each instance
 struct PrintToStringParamName {
  template< class ParamType >
  std::string
  operator()( const testing::TestParamInfo< ParamType > & info ) const {
   auto s = static_cast<TestParameter>(info.param).test_file;
   // Test names must be non-empty, unique, and may only contain ASCII
   // alphanumeric characters or underscore.
   std::replace( s.begin(), s.end(), '/', '_' );
   std::replace( s.begin(), s.end(), '.', '_' );
   std::replace( s.begin(), s.end(), '-', '_' );
   return s;
  }
 };
};

/*--------------------------------------------------------------------------*/
/*--------------------------- PARAMETERIZED TESTS --------------------------*/
/*--------------------------------------------------------------------------*/

/*
 * Each test (case) should test a single functionality of the class/system
 * being tested. The syntax is: TEST_P(TestFixtureName, TestName)
 */

TEST_P ( UCMILPTest, Solve ) {
 // Solve
 Solver * solver = block->get_registered_solvers().front();
 int status = solver->compute();
 ASSERT_EQ( status, Solver::kOK );

 auto error = 1e-4;
 const auto ub = GetParam().ub;
 EXPECT_NEAR( ub, solver->get_ub(), error );
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

INSTANTIATE_TEST_SUITE_P( UCMILPTests,
                          UCMILPTest,
                          ::testing::Values(  // file, ub
                           TestParameter{ "data/bus/InputData_TestCase1.nc", 29197370 },
                           TestParameter{ "data/bus/nowind.nc4", 2913975.2703765822 },
                           TestParameter{ "data/bus/TestCase1RES.nc4", 27862915 },
                           TestParameter{ "data/bus/20090907_noHydro_none.nc4", 16496061.862973599 },
                           TestParameter{ "data/bus/20090907_noHydro_prim.nc4", 16515189.748169417 },
                           TestParameter{ "data/bus/20090907_noHydro_PplusS.nc4", 16524705.871391438 },
                           TestParameter{ "data/bus/20091005_noHydro_none.nc4", 16813204.66119789 },
                           TestParameter{ "data/bus/20091005_noHydro_prim.nc4", 16833330.135148738 },
                           TestParameter{ "data/bus/20091005_noHydro_PplusS.nc4", 16839922.353689693 },
                           TestParameter{ "data/bus/20100311_noHydro_none.nc4", 28100596.369223103 },
                           TestParameter{ "data/bus/20100311_noHydro_prim.nc4", 28279997.217679787 },
                           TestParameter{ "data/bus/20100311_noHydro_PplusS.nc4", 28387334.970608912 },
                           TestParameter{ "data/bus/20100323_noHydro_none.nc4", 22086483.002185054 },
                           TestParameter{ "data/bus/20100323_noHydro_prim.nc4", 22194613.607022248 },
                           TestParameter{ "data/bus/20100323_noHydro_PplusS.nc4", 22260871.451456014 },
                           TestParameter{ "data/bus/20100623_noHydro_none.nc4", 16650367.054579698 },
                           TestParameter{ "data/bus/20100623_noHydro_prim.nc4", 16667146.884921743 },
                           TestParameter{ "data/bus/20100623_noHydro_PplusS.nc4", 16676595.032141121 },
                           TestParameter{ "data/bus/20090907_pHydro_1_none.nc4", 16411264.127826694 },
                           TestParameter{ "data/bus/20090907_pHydro_2_none.nc4", 16411264.127826694 },
                           TestParameter{ "data/bus/20090907_pHydro_3_none.nc4", 16175748.923443241 },
                           TestParameter{ "data/bus/20090907_pHydro_1A_none.nc4", 16463300.568516795 },
                           TestParameter{ "data/bus/20090907_pHydro_2block_withP_none.nc4", 16441192.467556689 },
                           TestParameter{ "data/bus/Input_simplified.nc4", 67462862.591071039 },
                           TestParameter{ "data/bus/InputData_Sysflex.nc4", 52450805.457220048 },
                           TestParameter{ "data/bus/InputData_Sysflex_V1.nc4", 138317912.23035586 },
                           TestParameter{ "data/bus/InputData_Sysflex_V2.nc4", 76083387.511754155 },
                           TestParameter{ "data/bus/InputData_Sysflex_V3.nc4",  77789285.5},
                           TestParameter{ "data/bus/InputData_Sysflex_V4.nc4", 140717780.21067753 },
                           TestParameter{ "data/bus/PublicFrance1Day.nc4", 33252650.6 },
                           TestParameter{ "data/PublicFrance1DayWith5Pr-4Sc-2In.nc4", 33252650.6 },
                           TestParameter{ "data/HVDC/PublicCountries1Day.nc4", 3478981001.314 },
                           TestParameter{ "data/HVDC/PublicCountries1DayWithPr.nc4",  3478984002.981585},
                           TestParameter{ "data/HVDC/PublicCountries1DayWith4Pr.nc4", 3502239238.8164968 },
                           TestParameter{ "data/HVDC/PublicCountries1DayWithPr-Sc.nc4", 3479149264.2781839 },
                           TestParameter{ "data/HVDC/PublicCountries1DayWithPr-Sc-In.nc4", 3479256187.856719 },
                           TestParameter{ "data/HVDC/PublicCountries1DayWith4Pr-6Sc.nc4", 3581497827.0875959 },
                           TestParameter{ "data/HVDC/PublicCountries1DayWith4Pr-6Sc-3In.nc4", 3549723954.2646852 },
                           TestParameter{ "data/HVDC/test1.nc4", 373764825.4381 },
                           TestParameter{ "data/HVDC/test2.nc4", 419791455.0002 },
                           TestParameter{ "data/HVDC/test3.nc4", 664474090.5224 },
                           TestParameter{ "data/HVDC/test4.nc4", -3179819037.1161385},
                           TestParameter{ "data/HVDC/test5.nc4", -3180068243.1455112 },
                           TestParameter{ "data/HVDC/test6.nc4", -3168655601.564 },
                           TestParameter{ "data/HVDC/test7.nc4", -3166626247.773 },
                           TestParameter{ "data/HVDC/test8.nc4",  -1128391550.157 }
                          // TestParameter{ "data/HVDC/test_jdd0.nc4", -3183551850 },
                          // TestParameter{ "data/HVDC/test_jdd1.nc4", -3170270040 },
                          // TestParameter{ "data/HVDC/test_jdd2.nc4",  -1612786730}
                          ),
                          UCMILPTest::PrintToStringParamName() );

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
