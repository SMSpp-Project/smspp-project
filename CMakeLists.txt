project(SMS++ DESCRIPTION "SMS++")

cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_VERBOSE_MAKEFILE OFF)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIRS})

find_package(netCDF REQUIRED)
include_directories(${netCDF_INCLUDE_DIR})
link_directories(${netCDF_LIB_DIR})

#message(STATUS "Boost  headers found in ${Boost_INCLUDE_DIRS}")
#message(STATUS "Eigen3 headers found in ${EIGEN3_INCLUDE_DIRS}")
#message(STATUS "netCDF headers found in ${netCDF_INCLUDE_DIR}")
#message(STATUS "netCDF library found in ${netCDF_LIB_DIR}")

# TODO: This needs to be system-dependant
set(CPLEX_DIR "/Applications/CPLEX_Studio128/cplex")
include_directories("${CPLEX_DIR}/include/ilcplex")
link_directories("${CPLEX_DIR}/lib/x86-64_osx/static_pic")

# Options for choosing the MCF Solver
#add_compile_definitions(HAVE_MFSMX)
#add_compile_definitions(HAVE_CPLEX)
add_compile_definitions(HAVE_RELAX)

include_directories(SMS++/include)
include_directories(MCFClass/MCFClass)
include_directories(MCFClass/MCFClone)
include_directories(MCFClass/MCFSimplex)
include_directories(MCFClass/RelaxIV)
include_directories(MCFClass/SPTree)
include_directories(MCFClass/OPTUtils)
include_directories(MCFBlock)
include_directories(SMILPBlock)
include_directories(MILPSolver/include)
include_directories(UCBlock/include)

add_subdirectory(SMS++)
add_subdirectory(MCFClass)
add_subdirectory(MCFBlock)
add_subdirectory(SMILPBlock)
add_subdirectory(MILPSolver)
add_subdirectory(UCBlock)
