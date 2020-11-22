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

    if ( (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND
          CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9") OR
         (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND
          CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8" ) )
        target_link_libraries(BendersBFunction_test PRIVATE
                              stdc++fs)
    endif ()

#    add_test(NAME BendersBFunction_test
#             COMMAND BendersBFunction_test
#             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
