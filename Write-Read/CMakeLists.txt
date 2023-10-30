# ----- Tests --------------------------------------------------------------- #

if (TARGET SMS++::MILPSolver)
    add_executable(writeandread_test test.cpp)
    set_source_files_properties(test.cpp PROPERTIES COMPILE_FLAGS "-g" )

    target_link_libraries(writeandread_test PRIVATE
                          SMS++::MILPSolver)

    set_target_properties(writeandread_test PROPERTIES COMPILE_FLAGS "-O3 -std=c++17 -DNDEBUG")
endif ()

# --------------------------------------------------------------------------- #
