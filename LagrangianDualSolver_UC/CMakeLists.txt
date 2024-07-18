# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::UCBlock AND
    TARGET SMS++::BundleSolver AND
    TARGET SMS++::LagrangianDualSolver)

    add_executable(LDS_UC_test test.cpp)
    target_link_libraries(LDS_UC_test PRIVATE
                          SMS++::UCBlock
                          SMS++::LagrangianDualSolver
                          SMS++::BundleSolver)
endif ()

# --------------------------------------------------------------------------- #
