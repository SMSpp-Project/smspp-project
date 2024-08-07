# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::MCFBlock AND
    TARGET SMS++::BinaryKnapsackBlock AND
    TARGET SMS++::BundleSolver AND
    TARGET SMS++::LagrangianDualSolver AND
    TARGET SMS++::CapacitatedFacilityLocationBlock AND
    TARGET SMS++::MILPSolver AND
    TARGET SMS++::LEMONSolver)

    add_executable(CFL_test test.cpp)
    target_link_libraries(CFL_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::LagrangianDualSolver
                          SMS++::MCFBlock
                          SMS++::MILPSolver
                          SMS++::BinaryKnapsackBlock
                          SMS++::CapacitatedFacilityLocationBlock
                          SMS++::LEMONSolver)

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch"
                          "${CMAKE_CURRENT_SOURCE_DIR}/batch-s"
                          "${CMAKE_CURRENT_SOURCE_DIR}/MCF/batch"
                          "${CMAKE_CURRENT_SOURCE_DIR}/LDVScuts/batch-s")
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
