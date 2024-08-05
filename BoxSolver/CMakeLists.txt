# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::MILPSolver)

    add_executable(BoxSolver_test test.cpp)
    target_link_libraries(BoxSolver_test PRIVATE
                          SMS++::MILPSolver)

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        add_test(NAME BoxSolver_test/${BATCH_NAME}
                 COMMAND bash ${BATCH_FILE}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
