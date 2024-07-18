# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::BundleSolver AND
    TARGET SMS++::MILPSolver)

    add_executable(LagBFunction_test test.cpp)
    target_link_libraries(LagBFunction_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)
endif ()

# --------------------------------------------------------------------------- #
