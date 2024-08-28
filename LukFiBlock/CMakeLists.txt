# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(LukFiBlock)
    # Solvers
    find_package(BundleSolver)
    find_package(MILPSolver)
endif ()

# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::LukFiBlock)

    add_executable(LukFi_test main.cpp)
    target_link_libraries(LukFi_test PRIVATE SMS++::LukFiBlock)

    if (TARGET SMS++::BundleSolver) # includes SMS+::MILPSolver
        target_link_libraries(LukFi_test PRIVATE SMS++::BundleSolver)
    elseif (TARGET SMS++::MILPSolver)
        target_link_libraries(LukFi_test PRIVATE SMS++::MILPSolver)
    endif ()
endif ()

# --------------------------------------------------------------------------- #
