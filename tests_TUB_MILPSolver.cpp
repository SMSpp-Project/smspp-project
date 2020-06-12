#include <gtest/gtest.h>
#include <UCBlock.h>
#include <ThermalUnitBlock.h>
#include <CPXMILPSolver.h>
// #include <BusNetworkBlock.h>

using namespace SMSpp_di_unipi_it;


struct TestParameters {
 std::string test_file;
};

/*--------------------------------------------------------------------------*/
/*------------------------- PARAMETRIZED FIXTURE ---------------------------*/
/*--------------------------------------------------------------------------*/

class TUBMILPSolverTest :
 public ::testing::TestWithParam< TestParameters > {

 protected:

 ThermalUnitBlock * block{};

 TUBMILPSolverTest() = default;

 ~TUBMILPSolverTest() override = default;

 void SetUp() override {
  std::string filename( GetParam().test_file );
  netCDF::NcFile f( filename, netCDF::NcFile::read );
  ASSERT_FALSE( f.isNull() );

  netCDF::NcGroupAtt gtype = f.getAtt( "SMS++_file_type" );
  ASSERT_FALSE( gtype.isNull() );

  int type;
  gtype.getValues( &type );
  ASSERT_EQ( type, eBlockFile );

  netCDF::NcGroup bg = f.getGroup( "Block_0" );
  ASSERT_FALSE( bg.isNull() );

  block = dynamic_cast<ThermalUnitBlock *>(Block::new_Block( "ThermalUnitBlock" ));
  block->deserialize( bg );
 }

 void TearDown() override {
  remove( "output.nc4" );
 }

 static std::string exec( const char * cmd ) {
  std::array< char, 128 > buffer{};
  std::string result;
  std::unique_ptr< FILE, decltype( &pclose ) > pipe( popen( cmd, "r" ), pclose );
  while( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr ) {
   result += buffer.data();
  }
  return result;
 }

 public:
 // Prints the test name
 struct PrintToStringParamName {
  template< class ParamType >
  std::string operator()( const testing::TestParamInfo< ParamType > & info ) const {
   auto s = static_cast<TestParameters>(info.param).test_file;
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
/*------------------------ PARAMETRIZED TEST CASES -------------------------*/
/*--------------------------------------------------------------------------*/
TEST_P( TUBMILPSolverTest, Serialize ) {
 netCDF::NcFile f1( "output.nc4", netCDF::NcFile::replace );
 f1.putAtt( "SMS++_file_type", netCDF::NcInt(), eBlockFile );
 auto bg1 = f1.addGroup( "Block_0" );
 block->serialize( bg1 );
 f1.close();

 std::string cmd1 = "ncdump -n test " + GetParam().test_file + " | sort";
 std::string cmd2 = "ncdump -n test output.nc4 | sort";

 auto res1 = exec( cmd1.c_str() );
 auto res2 = exec( cmd2.c_str() );
 ASSERT_EQ( res1.compare( res2 ), 0 );
}

TEST_P( TUBMILPSolverTest, GenerateAbsRepresentation ) {
 int tmp = 15;
 SimpleConfiguration< int > myconfig( tmp );

 ASSERT_NO_THROW( {
                   block->generate_abstract_variables( &myconfig );
                   block->generate_abstract_constraints( nullptr );
                   block->generate_objective( nullptr );
                  } );
}


/*--------------------------------------------------------------------------*/

TEST_P( TUBMILPSolverTest, SimpleSolve ) {
 auto milpsolver = new CPXMILPSolver();
 block->register_Solver( milpsolver );

 int tmp = 15;
 SimpleConfiguration< int > myconfig( tmp );

 ASSERT_NO_THROW( {
                   block->generate_abstract_variables( &myconfig );
                   block->generate_abstract_constraints( nullptr );
                   block->generate_objective( nullptr );
                  } );

 auto solver = block->get_registered_solvers().front();
 int status = solver->compute();
 ASSERT_EQ( status, Solver::kOK );
 // auto ub = solver->get_ub();
}

/*--------------------------------------------------------------------------*/
/*------------------------- TEST CASE INSTANCES ----------------------------*/
/*--------------------------------------------------------------------------*/

INSTANTIATE_TEST_SUITE_P( TUBMILPSolverTests,
                          TUBMILPSolverTest,
                          ::testing::Values(
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp1_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp2_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp3_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp4_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp5_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp6_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp7_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp8_24.nc4" },
                           TestParameters{ "../UCBlock/netCDF_files/1UC_Data/24/S1ramp9_24.nc4" }
                          ),
                          TUBMILPSolverTest::PrintToStringParamName() );

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return RUN_ALL_TESTS();
}
