# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ umbrella project                               #
#                                                                             #
#    The goal of this list file is to provide a quick way to configure        #
#    and build all the SMS++ projects together. It doesn't do much besides    #
#    calling the list files of the subprojects.                               #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.11)
# ----- Project definition and configuration -------------------------------- #
project(SMS++_project)
include(CMakeSettings.txt OPTIONAL)

# ----- Dependencies -------------------------------------------------------- #
add_subdirectory(MCFClass)                 # Required by MCFBlock
add_subdirectory(BundleSolver/NdoFiOracle) # Required by BundleSolver

# ----- Subprojects --------------------------------------------------------- #
# Comment out the ones you do not need.
# Some modules have requirements, so they should be loaded after those.

# Core library (required by all the others)
add_subdirectory(SMS++)

# Blocks
add_subdirectory(UCBlock)
add_subdirectory(MCFBlock)
add_subdirectory(MMCFBlock)    # Requires MCFBlock
add_subdirectory(LukFiBlock)
add_subdirectory(StochasticBlock)
add_subdirectory(SDDPBlock)    # Requires StochasticBlock

# Solvers
add_subdirectory(MILPSolver)
add_subdirectory(BundleSolver) # Requires MILPSolver
add_subdirectory(DPSolver)     # Requires UCBlock
add_subdirectory(LagrangianDualSolver)

# ----- Tools --------------------------------------------------------------- #
add_subdirectory(tools)

# ----- System tests -------------------------------------------------------- #
add_subdirectory(tests)

# --------------------------------------------------------------------------- #
