# ----- Test ---------------------------------------------------------------- #
if (TARGET SMS++::BinaryKnapsackBlock AND
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

    add_executable(BinaryKnapsackBlock_test test.cpp)
    target_link_libraries(BinaryKnapsackBlock_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::BinaryKnapsackBlock)
endif ()

# --------------------------------------------------------------------------- #
