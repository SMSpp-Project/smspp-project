# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::BinaryKnapsackBlock AND
    TARGET SMS++::MILPSolver)

    add_executable(test test.cpp)
    target_link_libraries(test PRIVATE
                          SMS++::MILPSolver
                          SMS++::BinaryKnapsackBlock)
endif ()

# --------------------------------------------------------------------------- #
