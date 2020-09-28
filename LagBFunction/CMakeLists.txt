# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND)
    add_executable(LagBFunction_test test.cpp)
    target_link_libraries(LagBFunction_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    add_test(NAME LagBFunction_test
             COMMAND LagBFunction_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
