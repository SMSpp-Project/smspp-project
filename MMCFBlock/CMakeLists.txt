# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::MMCFBlock AND
    TARGET SMS++::BundleSolver AND
    TARGET SMS++::MILPSolver AND
    TARGET MMCFClass)

    add_subdirectory(MMCFClass)
    add_executable(MMCF_test)

    target_sources(MMCF_test PRIVATE main.cpp)

    target_link_libraries(MMCF_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver
                          SMS++::MMCFBlock
                          MMCFClass)

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        add_test(NAME MMCF_test/${BATCH_NAME}
                 COMMAND bash ${BATCH_FILE}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
