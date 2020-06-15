# ----- Requirements -------------------------------------------------------- #
find_package(MCFBlock)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (MCFBlock_FOUND AND MILPSolver_FOUND)
    # Googletest
    add_executable(MCF_MILP_test test.cpp)
    target_link_libraries(MCF_MILP_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock
                          gtest)

    add_test(
            NAME MCF_MILP_test
            COMMAND MCF_MILP_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Old test
    add_executable(MCF_MILP_test_old test_old.cpp)
    target_link_libraries(MCF_MILP_test_old PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock)
endif ()
