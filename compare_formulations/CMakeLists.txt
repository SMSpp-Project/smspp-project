# ----- Requirements -------------------------------------------------------- #
# If it's not being called by the umbrella, we need to
# look for the system-installed libraries.
if (NOT hasParent)
    find_package(SMS++)
endif ()

# ----- Tests --------------------------------------------------------------- #
#[[if (TARGET SMS++::SMS++)

    add_executable(compare_formulations test.cpp)
    target_link_libraries(compare_formulations PRIVATE SMS++::SMS++)

    add_test(NAME compare_formulations
             COMMAND compare_formulations
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()]]

# --------------------------------------------------------------------------- #
