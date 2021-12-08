# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::UCBlock AND
    TARGET SMS++::MILPSolver)

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
