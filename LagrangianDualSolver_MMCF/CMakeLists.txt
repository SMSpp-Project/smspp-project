# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MILPSolver)
find_package(LagrangianDualSolver)

find_package(MMCFBlock)
find_package(UCBlock)

# ----- Test ---------------------------------------------------------------- #
if (BundleSolver_FOUND AND
    MILPSolver_FOUND AND
    LagrangianDualSolver_FOUND AND
    MMCFBlock_FOUND AND
    UCBlock_FOUND)

    add_executable(LDS_MMCF_test test.cpp)
    target_link_libraries(LDS_MMCF_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver
                          SMS++::LagrangianDualSolver
                          SMS++::MMCFBlock
                          SMS++::UCBlock)
endif ()

# --------------------------------------------------------------------------- #
