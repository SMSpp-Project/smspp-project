# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(SMS++)
    # Solvers
    find_package(MILPSolver)
endif ()

# ----- Tests --------------------------------------------------------------- #
add_executable(BoxSolver_test test.cpp)
target_link_libraries(BoxSolver_test PRIVATE SMS++::SMS++)

if (TARGET SMS++::MILPSolver)
    target_link_libraries(BoxSolver_test PRIVATE SMS++::MILPSolver)
endif ()

if (WIN32 AND BUILD_SHARED_LIBS AND NOT DEFINED ENV{CONDA_BUILD})
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET BoxSolver_test POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       $<TARGET_RUNTIME_DLLS:BoxSolver_test>
                       $<TARGET_FILE_DIR:BoxSolver_test>
                       COMMAND_EXPAND_LISTS
                       COMMENT "Copying runtime DLLs next to BoxSolver_test.exe")
endif()

# Add a test for each batch file
file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
foreach (BATCH_FILE ${BATCH_FILES})
    # Remove the source directory prefix to get the batch name
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
    if (UNIX)
        add_test(NAME BoxSolver_test/${BATCH_NAME}
                 COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/BoxSolver_test
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    elseif (WIN32)
        add_test(NAME BoxSolver_test/${BATCH_NAME}
                 # using Git for Windows that includes Git Bash
                 COMMAND "C:/Program Files/Git/bin/bash.exe"
                         "${BATCH_FILE}" "$<TARGET_FILE:BoxSolver_test>"
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()
endforeach ()

# --------------------------------------------------------------------------- #
