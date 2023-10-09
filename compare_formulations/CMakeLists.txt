# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::UCBlock AND
    TARGET SMS++::MILPSolver)

    add_executable(TUB_Solver_test test.cpp)
    target_link_libraries(TUB_Solver_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::UCBlock)

    add_executable(TUDPS_test main.cpp)
    target_link_libraries(TUDPS_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::UCBlock)

    add_test(NAME TUB_Solver_test
             COMMAND TUB_Solver_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

# --------------------------------------------------------------------------- #
