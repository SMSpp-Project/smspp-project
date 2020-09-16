# ----- Requirements -------------------------------------------------------- #
find_package(SMILPBlock)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (SMILPBlock_FOUND AND MILPSolver_FOUND)
    # Googletest
    add_executable(CPXMILPSolver_test test.cpp)
    target_link_libraries(CPXMILPSolver_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::SMILPBlock
                          gtest)

    add_test(NAME CPXMILPSolver_test
             COMMAND CPXMILPSolver_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Old test
    add_executable(CPXMILPSolver_test_old test_old.cpp)
    target_link_libraries(CPXMILPSolver_test_old PRIVATE
                          SMS++::MILPSolver
                          SMS++::SMILPBlock)
endif ()
