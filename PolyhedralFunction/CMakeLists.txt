# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::BundleSolver AND
    TARGET SMS++::MILPSolver)

    add_executable(PolyhedralFunction_test test.cpp)
    target_link_libraries(PolyhedralFunction_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    if (TARGET SMS++::SubGradSolver)
        target_link_libraries(PolyhedralFunction_test PRIVATE
                              SMS++::SubGradSolver)
    endif ()
endif ()

# --------------------------------------------------------------------------- #
