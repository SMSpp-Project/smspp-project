# ----- Requirements -------------------------------------------------------- #
find_package(UCBlock)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (UCBlock_FOUND AND MILPSolver_FOUND)
    add_executable(ThermalUnitBlock_MILP test.cpp)
    target_link_libraries(ThermalUnitBlock_MILP PRIVATE
                          SMS++::UCBlock
                          SMS++::MILPSolver
                          gtest)

    add_test(
            NAME ThermalUnitBlock_MILP
            COMMAND ThermalUnitBlock_MILP
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
