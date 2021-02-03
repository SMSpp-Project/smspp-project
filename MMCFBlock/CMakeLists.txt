# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MILPSolver)
find_package(MMCFBlock)

# ----- Test ---------------------------------------------------------------- #
if (BundleSolver_FOUND AND
    MILPSolver_FOUND AND
    MMCFBlock_FOUND)

    add_subdirectory(MMCFClass)
    add_executable(MMCFBlock_test)

    target_sources(MMCFBlock_test PRIVATE
                   main.cpp)

    target_link_libraries(MMCFBlock_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver
                          SMS++::MMCFBlock
                          MMCFClass)
endif ()

# --------------------------------------------------------------------------- #
