# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::BundleSolver AND
    TARGET SMS++::LukFiBlock)

    add_executable(LukFi_test main.cpp)
    target_link_libraries(LukFi_test PRIVATE
                          SMS++::LukFiBlock
                          SMS++::BundleSolver)
endif ()

# --------------------------------------------------------------------------- #
