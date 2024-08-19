# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(CapacitatedFacilityLocationBlock)
    # Solvers
    find_package(MILPSolver)
    find_package(LagrangianDualSolver)
    find_package(BundleSolver)
    find_package(LEMONSolver)
endif ()

# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::CapacitatedFacilityLocationBlock)

    add_executable(CFL_test test.cpp)
    target_link_libraries(CFL_test PRIVATE
                          SMS++::CapacitatedFacilityLocationBlock)

    if (TARGET SMS++::MILPSolver)
        target_link_libraries(CFL_test PRIVATE SMS++::MILPSolver)
    endif ()

    if (TARGET SMS++::LagrangianDualSolver)
        target_link_libraries(CFL_test PRIVATE SMS++::LagrangianDualSolver)
    endif ()

    if (TARGET SMS++::BundleSolver)
        target_link_libraries(CFL_test PRIVATE SMS++::BundleSolver)
    endif ()

    if (TARGET SMS++::LEMONSolver)
        target_link_libraries(CFL_test PRIVATE SMS++::LEMONSolver)
    endif ()

    # Add a test for each batch file
    file(GLOB BATCH_FILES #"${CMAKE_CURRENT_SOURCE_DIR}/batch" # TODO uncomment when it will be fixed
                          #"${CMAKE_CURRENT_SOURCE_DIR}/batch-s" # TODO uncomment when it will be fixed
                          "${CMAKE_CURRENT_SOURCE_DIR}/MCF/batch")
                          #"${CMAKE_CURRENT_SOURCE_DIR}/LDVScuts/batch-s") # TODO uncomment when it will be fixed
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        if (UNIX)
            add_test(NAME CFL_test/${BATCH_NAME}
                     COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/CFL_test
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        elseif (WIN32)
            add_test(NAME CFL_test/${BATCH_NAME}
                     # using Git for Windows that includes Git Bash
                     COMMAND "C:/Program Files/Git/bin/bash.exe" -c
                             "${BATCH_FILE} $<TARGET_FILE:CFL_test>"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif ()
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
