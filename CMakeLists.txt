# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ system tests                                   #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.11...3.15)
project(SMS++_tests VERSION 0.1.0 LANGUAGES CXX)

# ----- Settings ------------------------------------------------------------ #
include(CTest)
add_compile_definitions(HAVE_RELAX)

# Since we are using the block factory, objects from statically linked libraries
# may wrongly appear unused, and by default the linker does not include them,
# so we have to force the linking.

if (NOT BUILD_SHARED_LIBS)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        add_link_options("-Wl,-all_load")
    else ()
        add_link_options("-Wl,--whole-archive,--allow-multiple-definition")
    endif ()
endif ()

# ----- Requirements -------------------------------------------------------- #
# TODO: Proper requirement management
find_package(SMILPBlock QUIET)
find_package(MILPSolver QUIET)
find_package(MCFBlock QUIET)
find_package(UCBlock QUIET)
find_package(BundleSolver QUIET)

# ----- Fetch GoogleTest ---------------------------------------------------- #
include(FetchContent)
fetchcontent_declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.8.1
)

fetchcontent_getproperties(googletest)
if (NOT googletest_POPULATED)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_MACOSX_RPATH 1)
    endif ()
    fetchcontent_populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()

# ----- CPXMILPSolver_test -------------------------------------------------- #
if (SMILPBlock_FOUND AND MILPSolver_FOUND)
    add_executable(CPXMILPSolver_test tests_CPXMILPSolver.cpp)
    target_link_libraries(CPXMILPSolver_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::SMILPBlock
                          gtest)

    add_test(
            NAME CPXMILPSolver_test
            COMMAND CPXMILPSolver_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif ()

# ----- MCFMILPSolvers_test ------------------------------------------------- #
if (MCFBlock_FOUND AND MILPSolver_FOUND)
    add_executable(MCFMILPSolvers_test tests_MCFMILPSolvers.cpp)
    target_link_libraries(MCFMILPSolvers_test PRIVATE
                          SMS++::MILPSolver
                          SMS++::MCFBlock
                          gtest)

    add_test(
            NAME MCFMILPSolvers_test
            COMMAND MCFMILPSolvers_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif ()

# ----- ThermalUnitBlock_test ----------------------------------------------- #
if (UCBlock_FOUND AND MILPSolver_FOUND)
    add_executable(ThermalUnitBlock_test tests_ThermalUnitBlock.cpp)
    target_link_libraries(ThermalUnitBlock_test PRIVATE
                          SMS++::UCBlock
                          SMS++::MILPSolver
                          gtest)

    add_test(
            NAME ThermalUnitBlock_test
            COMMAND ThermalUnitBlock_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif ()

# ----- PolyhedralFunction_test --------------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND)

    add_executable(PolyhedralFunction_test PolyhedralFunction/test.cpp)
    target_link_libraries(PolyhedralFunction_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    add_test(
            NAME PolyhedralFunction_test
            COMMAND PolyhedralFunction_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PolyhedralFunction
    )
endif ()

# ----- PolyhedralFunctionBlock_test ---------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND)
    add_executable(PolyhedralFunctionBlock_test PolyhedralFunctionBlock/test.cpp)
    target_link_libraries(PolyhedralFunctionBlock_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    add_test(
            NAME PolyhedralFunctionBlock_test
            COMMAND PolyhedralFunctionBlock_test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/PolyhedralFunctionBlock
    )
endif ()

# ----- OLD TESTS ----------------------------------------------------------- #
if (SMILPBlock_FOUND AND MILPSolver_FOUND)
add_executable(MILPSolver_test milpsolver_test.cpp)
target_link_libraries(MILPSolver_test PRIVATE
                      SMS++::MILPSolver
                      SMS++::SMILPBlock)
endif ()

if (MCFBlock_FOUND AND MILPSolver_FOUND)
add_executable(MCFSolver_test mcfsolver_test.cpp)
target_link_libraries(MCFSolver_test PRIVATE
                      SMS++::MILPSolver
                      SMS++::MCFBlock)
endif ()
