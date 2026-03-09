# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    # Blocks
    find_package(LukFiBlock)
    # Solvers
    find_package(BundleSolver)
    find_package(MILPSolver)
endif ()

# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::LukFiBlock)

    add_executable(LukFi_test main.cpp)
    target_link_libraries(LukFi_test PRIVATE SMS++::LukFiBlock)

    if (TARGET SMS++::BundleSolver)
        target_link_libraries(LukFi_test PRIVATE SMS++::BundleSolver)
    endif ()

    if (TARGET SMS++::MILPSolver)
        target_link_libraries(LukFi_test PRIVATE SMS++::MILPSolver)
    endif ()

    if (WIN32 AND BUILD_SHARED_LIBS)
        # copy runtime DLLs next to the exe after build
        add_custom_command(TARGET LukFi_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:LukFi_test>
                $<TARGET_FILE_DIR:LukFi_test>
                COMMAND_EXPAND_LISTS
                COMMENT "Copying runtime DLLs next to LukFi_test.exe"
        )
    endif()
endif ()

# --------------------------------------------------------------------------- #
