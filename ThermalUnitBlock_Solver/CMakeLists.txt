# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::UCBlock AND
    TARGET SMS++::MILPSolver)

    add_executable(TUDPS_test test.cpp)
    target_link_libraries(TUDPS_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::UCBlock)

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batches/*"
                          "${CMAKE_CURRENT_SOURCE_DIR}/cuts/*")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        add_test(NAME TUDPS_test/${BATCH_NAME}
                 COMMAND bash ${BATCH_FILE}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
