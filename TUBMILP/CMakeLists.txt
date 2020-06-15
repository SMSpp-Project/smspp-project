# ----- Requirements -------------------------------------------------------- #
find_package(UCBlock)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (UCBlock_FOUND AND MILPSolver_FOUND)
    add_executable(TUBMILP_test test.cpp)
    target_link_libraries(TUBMILP_test PRIVATE
                          SMS++::UCBlock
                          SMS++::MILPSolver
                          gtest)

    add_test(
            NAME TUBMILP_test
            COMMAND TUBMILP_test
            WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}/../../UCBlock/netCDF_files)
endif ()
