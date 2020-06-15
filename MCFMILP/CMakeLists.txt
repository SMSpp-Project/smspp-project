# ----- Requirements -------------------------------------------------------- #
find_package(MCFBlock)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (MCFBlock_FOUND AND MILPSolver_FOUND)
    # Googletest
    add_executable(MCFMILP_test test.cpp)
    target_link_libraries(MCFMILP_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock
                          gtest)

    add_test(
            NAME MCFMILP_test
            COMMAND MCFMILP_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Old test
    add_executable(MCFMILP_test_old test_old.cpp)
    target_link_libraries(MCFMILP_test_old PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock)
endif ()
