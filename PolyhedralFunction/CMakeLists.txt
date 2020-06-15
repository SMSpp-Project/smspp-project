# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND)
    add_executable(PolyhedralFunction_test test.cpp)
    target_link_libraries(PolyhedralFunction_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    add_test(
            NAME PolyhedralFunction_test
            COMMAND PolyhedralFunction_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
