# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::BundleSolver AND
    TARGET SMS++::LagrangianDualSolver)

    add_executable(LDS_Box_test test.cpp)
    target_link_libraries(LDS_Box_test PRIVATE
                          SMS++::LagrangianDualSolver
                          SMS++::BundleSolver)
endif ()

# --------------------------------------------------------------------------- #
