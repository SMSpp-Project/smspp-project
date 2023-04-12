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
add_subdirectory(MMCFBlock)       # Requires MCFBlock
add_subdirectory(LukFiBlock)
add_subdirectory(StochasticBlock)
add_subdirectory(SDDPBlock)       # Requires StochasticBlock, StOpt

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
