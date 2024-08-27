# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(SMS++)
    # Solvers
    find_package(MILPSolver)
endif ()

# ----- Tests --------------------------------------------------------------- #
add_executable(quad_test quad-test.cpp)
target_link_libraries(quad_test PRIVATE SMS++::SMS++)
target_link_libraries(quad_test PRIVATE SMS++::MILPSolver)

# --------------------------------------------------------------------------- #
