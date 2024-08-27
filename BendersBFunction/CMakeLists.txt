# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(MCFBlock)
    # Solvers
    find_package(BundleSolver)
    find_package(MILPSolver)
endif ()

# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::MCFBlock)

    add_executable(BendersBFunction_test test.cpp
                                         cwl-mcf/cwl-mcf.cpp)
    target_link_libraries(BendersBFunction_test PRIVATE SMS++::MCFBlock)

    if (TARGET SMS++::BundleSolver) # includes SMS+::MILPSolver
        target_link_libraries(BendersBFunction_test PRIVATE SMS++::BundleSolver)
    elseif (TARGET SMS++::MILPSolver)
        target_link_libraries(BendersBFunction_test PRIVATE SMS++::MILPSolver)
    endif ()


    add_executable(BendersBFunction_test2 test2.cpp)
    set_source_files_properties( test2.cpp PROPERTIES COMPILE_FLAGS "-g" )

    if (TARGET SMS++::BundleSolver) # includes SMS+::MILPSolver
        target_link_libraries(BendersBFunction_test2 PRIVATE SMS++::BundleSolver)
    elseif (TARGET SMS++::MILPSolver)
        target_link_libraries(BendersBFunction_test2 PRIVATE SMS++::MILPSolver)
    endif ()

    add_test(NAME BendersBFunction_test2
             COMMAND BendersBFunction_test2 "BundlePar-cwl.txt"
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

# --------------------------------------------------------------------------- #
