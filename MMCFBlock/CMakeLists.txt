# ----- Settings ------------------------------------------------------------ #
# An option is an ON/OFF user-settable cache variable
option(RUN_mnetgen "Whether ${modName} will run mnetgen or not." OFF)

# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(MMCFBlock)
    find_package(MMCFClass)
    # Solvers
    find_package(BundleSolver)
    find_package(MILPSolver)
endif ()

# ----- Subdirectories ------------------------------------------------------ #
add_subdirectory(gen/Mnetgen)

# ----- Custom commands and targets ----------------------------------------- #

if (RUN_mnetgen)
    # Custom command to run gen/batch with the mnetgen exe if the .generated file does not exist
    if (UNIX)
        add_custom_command(
                OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/data/Mnetgen/.generated
                COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/gen/genbatch ${CMAKE_CURRENT_BINARY_DIR}/gen/Mnetgen/mnetgen
                COMMAND echo > ${CMAKE_CURRENT_SOURCE_DIR}/data/Mnetgen/.generated
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gen
                COMMENT "Running mnetgen"
        )
    elseif (WIN32)
        add_custom_command(
                OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/data/Mnetgen/.generated
                # using Git for Windows that includes Git Bash
                COMMAND "C:/Program Files/Git/bin/bash.exe"
                "${CMAKE_CURRENT_SOURCE_DIR}/gen/genbatch" "$<TARGET_FILE:mnetgen>"
                COMMAND echo > ${CMAKE_CURRENT_SOURCE_DIR}/data/Mnetgen/.generated
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gen
                COMMENT "Running mnetgen"
        )
    endif ()

    # Create custom targets to run the nc4 generation
    add_custom_target(run_mnetgen ALL
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/Mnetgen/.generated
    )
endif ()

# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::MMCFBlock AND
    TARGET MMCFClass)

    add_subdirectory(MMCFClass)

    add_executable(MMCF_test main.cpp)
    target_link_libraries(MMCF_test PRIVATE
                          SMS++::MMCFBlock
                          MMCFClass)

    if (TARGET SMS++::BundleSolver)
        target_link_libraries(MMCF_test PRIVATE SMS++::BundleSolver)
    endif ()

    if (TARGET SMS++::MILPSolver)
        target_link_libraries(MMCF_test PRIVATE SMS++::MILPSolver)
    endif ()

    # Add a test for each batch file
    file(GLOB BATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/batch")
    foreach (BATCH_FILE ${BATCH_FILES})
        # Remove the source directory prefix to get the batch name
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BATCH_NAME ${BATCH_FILE})
        if (UNIX)
            add_test(NAME MMCF_test/${BATCH_NAME}
                     COMMAND bash ${BATCH_FILE} ${CMAKE_CURRENT_BINARY_DIR}/MMCF_test
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        elseif (WIN32)
            add_test(NAME MMCF_test/${BATCH_NAME}
                     # using Git for Windows that includes Git Bash
                     COMMAND "C:/Program Files/Git/bin/bash.exe"
                             "${BATCH_FILE}" "$<TARGET_FILE:MMCF_test>"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif ()
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
