# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::BinaryKnapsackBlock AND
    TARGET SMS++::MILPSolver)

    add_executable(BinaryKnapsackBlock_test test.cpp)
    target_link_libraries(BinaryKnapsackBlock_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::BinaryKnapsackBlock)

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        add_test(NAME BinaryKnapsackBlock_test/${BATCH_NAME}
                 COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/BinaryKnapsackBlock_test
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
