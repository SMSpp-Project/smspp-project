# ----- Test ---------------------------------------------------------------- #
if (TARGET MMCFBlock AND
    TARGET UCBlock AND
    TARGET BundleSolver AND
    TARGET LagrangianDualSolver AND
    TARGET MILPSolver)

    add_executable(LDS_MMCF_test test.cpp)
    target_link_libraries(LDS_MMCF_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver
                          SMS++::LagrangianDualSolver
                          SMS++::MMCFBlock
                          SMS++::UCBlock)
endif ()

# --------------------------------------------------------------------------- #
