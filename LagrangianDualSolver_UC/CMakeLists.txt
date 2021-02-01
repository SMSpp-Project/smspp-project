# ----- Requirements -------------------------------------------------------- #
find_package(LagrangianDualSolver)
find_package(BundleSolver)
find_package(UCBlock)

# ----- Test ---------------------------------------------------------------- #
if (LagrangianDualSolver_FOUND AND BundleSolver_FOUND AND UCBlock_FOUND)

    add_executable(LDS_UC_test test.cpp)
    target_link_libraries(LDS_UC_test PRIVATE
                          SMS++::UCBlock
                          SMS++::LagrangianDualSolver
                          SMS++::BundleSolver)
endif ()

# --------------------------------------------------------------------------- #
