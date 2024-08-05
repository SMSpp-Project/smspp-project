# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::BundleSolver AND
    TARGET SMS++::MILPSolver)

    add_executable(PolyhedralFunction_test test.cpp)
    target_link_libraries(PolyhedralFunction_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    if (TARGET SMS++::SubGradSolver)
        target_link_libraries(PolyhedralFunction_test PRIVATE
                              SMS++::SubGradSolver)
    endif ()

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        add_test(NAME PolyhedralFunction_test/${BATCH_NAME}
                 COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/PolyhedralFunction_test
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
