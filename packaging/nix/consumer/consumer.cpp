// A Block and a Solver of two modules made by the factories, by the names
// of their classes
#include <Block.h>
#include <Solver.h>

#include <iostream>

int main() {
 using namespace SMSpp_di_unipi_it;
 auto b = Block::new_Block( "UCBlock" );
 auto s = Solver::new_Solver( "HiGHSMILPSolver" );
 std::cout << "UCBlock " << ( b ? "ok" : "none" ) << ", HiGHSMILPSolver "
           << ( s ? "ok" : "none" ) << std::endl;
 delete s;
 delete b;
 return 0;
}
