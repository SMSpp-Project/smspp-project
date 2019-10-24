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
cmake_minimum_required(VERSION 3.8...3.15)
# ----- Project definition and configuration -------------------------------- #
project(SMS++_project)
include(CMakeSettings.txt OPTIONAL)

# ----- Subprojects --------------------------------------------------------- #
# Comment out the ones you do not need.

add_subdirectory(SMS++)
add_subdirectory(MCFClass)
add_subdirectory(MCFBlock)
add_subdirectory(SMILPBlock)
add_subdirectory(MILPSolver)
add_subdirectory(UCBlock)

# ----- Tools --------------------------------------------------------------- #
add_subdirectory(SMS++/tools)
add_subdirectory(UCBlock/tools)

# ----- System tests -------------------------------------------------------- #
add_subdirectory(MILPSolver/test_sys)
add_subdirectory(UCBlock/test_sys)
