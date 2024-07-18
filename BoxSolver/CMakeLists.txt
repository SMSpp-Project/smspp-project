# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::MILPSolver)

    add_executable(BoxSolver_test test.cpp)
    target_link_libraries(BoxSolver_test PRIVATE
                          SMS++::MILPSolver)
endif ()

# --------------------------------------------------------------------------- #
