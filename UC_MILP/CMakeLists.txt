# ----- Requirements -------------------------------------------------------- #
find_package(UCBlock)
find_package(MILPSolver)

# ----- Test ---------------------------------------------------------------- #
if (UCBlock_FOUND AND MILPSolver_FOUND)

    add_executable(UC_MILP_test test.cpp)
    target_link_libraries(UC_MILP_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::UCBlock
                          gtest)

    add_test(NAME UC_MILP_test
             COMMAND UC_MILP_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

endif ()

# --------------------------------------------------------------------------- #
