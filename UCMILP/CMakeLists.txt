# ----- Requirements -------------------------------------------------------- #
find_package(UCBlock)
find_package(MILPSolver)

# ----- Test ---------------------------------------------------------------- #
if (UCBlock_FOUND AND MILPSolver_FOUND)

    add_executable(UCMILP_test test.cpp)
    target_link_libraries(UCMILP_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::UCBlock
                          gtest)

    add_test(NAME UCMILP_test
             COMMAND UCMILP_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

endif ()

# --------------------------------------------------------------------------- #
