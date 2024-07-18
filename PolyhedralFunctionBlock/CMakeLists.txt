# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::BundleSolver AND
    TARGET SMS++::MILPSolver)

    add_executable(PolyhedralFunctionBlock_test test.cpp)
    target_link_libraries(PolyhedralFunctionBlock_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)
endif ()

# --------------------------------------------------------------------------- #
