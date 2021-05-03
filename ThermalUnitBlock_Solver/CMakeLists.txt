# ----- Test ---------------------------------------------------------------- #
if (TARGET UCBlock AND
    TARGET MILPSolver)

    add_executable(TUB_Solver_test test.cpp)
    target_link_libraries(TUB_Solver_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::UCBlock
                          gtest)

    add_test(NAME TUB_Solver_test
             COMMAND TUB_Solver_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

endif ()

# --------------------------------------------------------------------------- #
