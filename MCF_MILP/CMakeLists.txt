# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(MCFBlock)
    # Solvers
    find_package(MILPSolver)
    find_package(LEMONSolver)
endif ()

# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::MCFBlock)

    add_executable(MCF_MILP_test test.cpp)
    target_link_libraries(MCF_MILP_test PRIVATE SMS++::MCFBlock)

    if (TARGET SMS++::MILPSolver)
        target_link_libraries(MCF_MILP_test PRIVATE SMS++::MILPSolver)
    endif ()

    if (TARGET SMS++::LEMONSolver)
        target_link_libraries(MCF_MILP_test PRIVATE SMS++::LEMONSolver)
    endif ()

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        if (UNIX)
            add_test(NAME MCF_MILP_test/${BATCH_NAME}
                     COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/MCF_MILP_test
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        elseif (WIN32)
            add_test(NAME MCF_MILP_test/${BATCH_NAME}
                     # using Git for Windows that includes Git Bash
                     COMMAND "C:/Program Files/Git/bin/bash.exe"
                             "${BATCH_FILE}" "$<TARGET_FILE:MCF_MILP_test>"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif ()
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
