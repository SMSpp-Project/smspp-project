# ----- Requirements -------------------------------------------------------- #
find_package(MCFBlock)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (MCFBlock_FOUND AND MILPSolver_FOUND)

    # MCF_SOLVER should be read from MCFBlock configuration
    if (MCF_SOLVER STREQUAL "relax")
        message(STATUS "Using RelaxIV solver")
        add_compile_definitions(HAVE_RELAX)
    elseif (MCF_SOLVER STREQUAL "cplex")
        message(STATUS "Using CPLEX solver")
        add_compile_definitions(HAVE_CPLEX)
    elseif (MCF_SOLVER STREQUAL "mfsmx")
        message(STATUS "Using Simplex solver")
        add_compile_definitions(HAVE_MFSMX)
    elseif (MCF_SOLVER STREQUAL "sptree")
        message(STATUS "Using Shortest Path Tree solver")
        add_compile_definitions(HAVE_SPTRE)
    endif ()

    # Googletest
    add_executable(MCFMILP_test test.cpp)
    target_link_libraries(MCFMILP_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock
                          gtest)

    add_test(NAME MCFMILP_test
             COMMAND MCFMILP_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Old test
    add_executable(MCFMILP_test_old test_old.cpp)
    target_link_libraries(MCFMILP_test_old PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock)
endif ()
