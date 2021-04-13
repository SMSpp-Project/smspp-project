# ----- Requirements -------------------------------------------------------- #
find_package(UCBlock)
find_package(MILPSolver)
find_package(DPSolver)

# ----- Test ---------------------------------------------------------------- #
if (UCBlock_FOUND AND MILPSolver_FOUND AND DPSolver_FOUND)

    add_executable(TUB_Solver_test test.cpp)
    target_link_libraries(TUB_Solver_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::UCBlock
                          SMS++::DPSolver
                          gtest)

    add_test(NAME TUB_Solver_test
             COMMAND TUB_Solver_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

endif ()

# --------------------------------------------------------------------------- #
