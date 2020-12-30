find_package(BundleSolver)
add_executable(LukFiBlock_test main.cpp)
target_link_libraries(LukFiBlock_test PRIVATE
                      SMS++::LukFiBlock
                      SMS++::BundleSolver)
