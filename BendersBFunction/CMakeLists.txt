# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MILPSolver)

# ----- BendersBFunction_test ----------------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND)
    add_executable(BendersBFunction_test test.cpp
                   cwl-mcf/cwl-mcf.cpp cwl-mcf/MCFSimplex.C)
    target_link_libraries(BendersBFunction_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    add_test(NAME BendersBFunction_test
             COMMAND BendersBFunction_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
