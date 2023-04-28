# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ umbrella project                               #
#                                                                             #
#    The goal of this list file is to provide a quick way to configure        #
#    and build all the SMS++ projects together. It doesn't do much besides    #
#    calling the list files of the subprojects.                               #
#                                                                             #
#                              Niccolo' Iardella                              #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.11)
# ----- Project definition and configuration -------------------------------- #
project("The SMS++ Project")

# This file can contain custom settings.
# Use it as an alternative to specify CMake variables manually with -D.
include(CMakeSettings.txt OPTIONAL)

# Some easier way to alleviate the cmake call and pick the directories from a ROOT
if( COIN_ROOT_DIR )
	set(Clp_ROOT ${COIN_ROOT_DIR})
	set(Osi_ROOT ${COIN_ROOT_DIR})
	set(CoinUtils_ROOT ${COIN_ROOT_DIR})
endif()

if ( StOpt_ROOT )
	set( StOpt_geners_INCLUDE_DIR ${StOpt_ROOT}/StOpt/geners-1.11.0/ )
	set( StOpt_INCLUDE_DIR ${StOpt_ROOT}/StOpt )
	set( StOpt_LIBRARY ${StOpt_ROOT}/BUILD/lib/libStOpt.a )
	set( StOpt_geners_LIBRARY ${StOpt_ROOT}/BUILD/lib/libgeners.a )	
endif()
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ADD_CXX_FLAGS}")

# ----- Dependencies -------------------------------------------------------- #
# Comment out the ones you do not need.

# MCFClass library, required by MCFBlock.
# Requires CPLEX.
add_subdirectory(MCFClass)

# NDOSolver/FiOracle library, required by BundleSolver.
# Requires CoinUtils, Osi and CPLEX.
add_subdirectory(BundleSolver/NdoFiOracle)

# ----- Subprojects --------------------------------------------------------- #
# Comment out the ones you do not need.
# Some modules have requirements, so they should be loaded after those.

# The core library, required by all other SMS++ modules.
# Requires Boost (headers only), netCDF-C++ and Eigen3.
add_subdirectory(SMS++)

# Blocks
add_subdirectory(BinaryKnapsackBlock)
add_subdirectory(UCBlock)
add_subdirectory(MCFBlock)        # Requires MCFClass
add_subdirectory(MMCFBlock)       # Requires MCFBlock, BinaryKnapsackBlock
add_subdirectory(LukFiBlock)
add_subdirectory(StochasticBlock)
add_subdirectory(SDDPBlock)       # Requires StochasticBlock, StOpt
add_subdirectory(CapacitatedFacilityLocationBlock)
                                  # Requires MCFBlock, BinaryKnapsackBlock

# Solvers
add_subdirectory(MILPSolver)      # Requires CPLEX and/or SCIP
add_subdirectory(BundleSolver)    # Requires MILPSolver
add_subdirectory(LagrangianDualSolver)
#add_subdirectory(SubGradSolver)

# ----- Tools --------------------------------------------------------------- #
add_subdirectory(tools)

# ----- System tests -------------------------------------------------------- #
add_subdirectory(tests)

# --------------------------------------------------------------------------- #
