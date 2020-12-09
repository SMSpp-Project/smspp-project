# ----- Requirements -------------------------------------------------------- #
find_package(SMILPBlock)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (SMILPBlock_FOUND AND MILPSolver_FOUND)
    add_executable(CPXMILPSolver_test test.cpp)
    target_link_libraries(CPXMILPSolver_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::SMILPBlock
                          gtest)

    add_test(NAME CPXMILPSolver_test
             COMMAND CPXMILPSolver_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
