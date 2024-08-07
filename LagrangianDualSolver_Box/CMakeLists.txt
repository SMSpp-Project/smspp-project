# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::BundleSolver AND
    TARGET SMS++::LagrangianDualSolver)

    add_executable(LDS_Box_test test.cpp)
    target_link_libraries(LDS_Box_test PRIVATE
                          SMS++::LagrangianDualSolver
                          SMS++::BundleSolver)

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        if (UNIX)
            add_test(NAME LDS_Box_test/${BATCH_NAME}
                     COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/LDS_Box_test
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        elseif (WIN32)
            add_test(NAME LDS_Box_test/${BATCH_NAME}
                     # using Git for Windows that includes Git Bash
                     COMMAND "C:/Program Files/Git/bin/bash.exe" -c
                             "${BATCH_FILE} $<TARGET_FILE:LDS_Box_test>"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif ()
        # Disable the test timeout
        set_tests_properties(LDS_Box_test/${BATCH_NAME} PROPERTIES TIMEOUT 0)
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
