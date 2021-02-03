# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MCFClass)
find_package(MILPSolver)

# ----- BendersBFunction_test ----------------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND AND MCFClass_FOUND)
    add_executable(BendersBFunction_test test.cpp
                   cwl-mcf/cwl-mcf.cpp)
    target_link_libraries(BendersBFunction_test PRIVATE
                          SMS++::BundleSolver
                          MCFClass::MCFClass
                          SMS++::MILPSolver)

    if ( (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND
          CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9") OR
         (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND
          CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9" ) )
        target_link_libraries(BendersBFunction_test PRIVATE
                              stdc++fs)
    endif ()

    add_test(NAME BendersBFunction_test
             COMMAND BendersBFunction_test ./simple
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(BendersBFunction_test2 test2.cpp)

    target_link_libraries(BendersBFunction_test2 PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    add_test(NAME BendersBFunction_test2
             COMMAND BendersBFunction_test2
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

endif ()

# --------------------------------------------------------------------------- #
