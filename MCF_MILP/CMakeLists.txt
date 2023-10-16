# ----- Tests --------------------------------------------------------------- #
if (TARGET SMS++::MCFBlock AND
    TARGET SMS++::MILPSolver)

    # Since we are using the block factory, objects from linked libraries
    # may wrongly appear unused, and by default the linker does not include them,
    # so we have to force the linking.
    if (BUILD_SHARED_LIBS)
        if (UNIX AND (NOT APPLE))
            add_link_options("-Wl,--no-as-needed")
        endif ()
    else ()
        if (MSVC)
            add_link_options("/WHOLEARCHIVE")
            add_link_options("/FORCE:MULTIPLE")
        else () # Unix
            if (APPLE)
                add_link_options("-Wl,-all_load")
            else ()
                add_link_options("-Wl,--whole-archive,--allow-multiple-definition")
            endif ()
        endif ()
    endif ()

    add_executable(MCF_MILP_test test.cpp)
    target_link_libraries(MCF_MILP_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock)

    add_test(NAME MCF_MILP_test
             COMMAND MCF_MILP_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

# --------------------------------------------------------------------------- #
