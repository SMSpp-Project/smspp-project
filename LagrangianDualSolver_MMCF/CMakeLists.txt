# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::MMCFBlock AND
    TARGET SMS++::UCBlock AND
    TARGET SMS++::BundleSolver AND
    TARGET SMS++::LagrangianDualSolver AND
    TARGET SMS++::MILPSolver AND
    TARGET SMS++::LEMONSolver)

    add_executable(LDS_MMCF_test test.cpp)
    target_link_libraries(LDS_MMCF_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver
                          SMS++::LagrangianDualSolver
                          SMS++::MMCFBlock
                          SMS++::UCBlock
                          SMS++::LEMONSolver)

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch"
                          "${CMAKE_CURRENT_SOURCE_DIR}/batch-c"
                          "${CMAKE_CURRENT_SOURCE_DIR}/batch-m")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        add_test(NAME LDS_MMCF_test/${BATCH_NAME}
                 COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/LDS_MMCF_test
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
