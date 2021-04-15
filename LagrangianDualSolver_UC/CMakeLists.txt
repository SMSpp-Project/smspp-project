# ----- Requirements -------------------------------------------------------- #
find_package(LagrangianDualSolver)
find_package(BundleSolver)
find_package(UCBlock)
find_package(DPSolver)

# Introduced in 3.19 to handle arbitrary characters in test names
# See: https://cmake.org/cmake/help/latest/policy/CMP0110.html
if (POLICY CMP0110)
    cmake_policy(SET CMP0110 NEW)
endif()

# ----- Test ---------------------------------------------------------------- #
if (LagrangianDualSolver_FOUND AND
    BundleSolver_FOUND AND
    UCBlock_FOUND AND
    DPSolver_FOUND)

    add_executable(LDS_UC_test test.cpp)
    target_link_libraries(LDS_UC_test PRIVATE
                          SMS++::UCBlock
                          SMS++::LagrangianDualSolver
                          SMS++::BundleSolver
                          SMS++::DPSolver)

    # This version replicates the batch file in CMake.
    # This method was preferred because it allows to keep track
    # of the single tests via the CTest application and also
    # avoids copying all the auxiliary files.
    foreach (size 10 20 50 75 100 200)
        foreach (seed RANGE 1 5)
            add_test(NAME "LDS_UC_test/[${size},${seed}]"
                     COMMAND LDS_UC_test ../../UCBlock/netCDF_files/UC_Data/T-Ramp/${size}_0_${seed}_w.nc4
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endforeach ()
    endforeach ()
endif ()

# --------------------------------------------------------------------------- #
