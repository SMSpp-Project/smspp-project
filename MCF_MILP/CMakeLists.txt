# ----- Tests --------------------------------------------------------------- #
if (TARGET MCFBlock AND
    TARGET MILPSolver)

    # MCFBlock_SOLVER should be read from MCFBlock configuration
    if (MCFBlock_SOLVER STREQUAL "relax")
        add_compile_definitions(HAVE_RELAX)
    elseif (MCFBlock_SOLVER STREQUAL "cplex")
        add_compile_definitions(HAVE_CPLEX)
    elseif (MCFBlock_SOLVER STREQUAL "mfsmx")
        add_compile_definitions(HAVE_MFSMX)
    elseif (MCFBlock_SOLVER STREQUAL "sptree")
        add_compile_definitions(HAVE_SPTRE)
    endif ()

    # Googletest
    add_executable(MCF_MILP_test test.cpp)
    target_link_libraries(MCF_MILP_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock
                          gtest)

    add_test(NAME MCF_MILP_test
             COMMAND MCF_MILP_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Old test
#    add_executable(MCF_MILP_test_old test_old.cpp)
#    target_link_libraries(MCF_MILP_test_old PRIVATE
#                          SMS++::MILPSolver
#                          SMS++::MCFBlock)
endif ()

# --------------------------------------------------------------------------- #
