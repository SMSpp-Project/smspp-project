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

 auto error = 1e-8;
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
                           TestParameter{ "data/InputData_TestCase1.nc", 27862915 },
                           TestParameter{ "data/nowind.nc4", 29197370 },
                           TestParameter{ "data/TestCase1RES.nc4", 2913975 }
                          ),
                          UCMILPTest::PrintToStringParamName() );

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
