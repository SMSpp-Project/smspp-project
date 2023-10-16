# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::MILPSolver)

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

    add_executable(BoxSolver_test test.cpp)
    target_link_libraries(BoxSolver_test PRIVATE
                          SMS++::MILPSolver)

    # This version replicates the batch file in CMake.
    # This method was preferred because it allows to keep track
    # of the single tests via the CTest application and also
    # avoids copying all the auxiliary files.
    set(what 3)
    foreach (size 2 10 50 100 1000)
        foreach (seed RANGE 0 19)
            add_test(NAME "BoxSolver_test/[${seed},${what},${size}]"
                     COMMAND BoxSolver_test ${seed} ${what} ${size}
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endforeach ()
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
