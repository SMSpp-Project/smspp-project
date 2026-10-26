// A Block of the module made by the factory, by the name of its class
#include <Block.h>

#include <iostream>
#include <memory>

int main() {
 std::unique_ptr< SMSpp_di_unipi_it::Block > b(
  SMSpp_di_unipi_it::Block::new_Block( "MCFBlock" ) );
 std::cout << "MCFBlock from the factory: " << ( b ? "ok" : "none" )
           << std::endl;
 return b ? 0 : 1;
}
