# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ umbrella project                               #
#                                                                             #
#    The goal of this list file is to provide a quick way to configure        #
#    and build all the SMS++ projects together. It doesn't do much besides    #
#    calling the list files of the subprojects.                               #
#                                                                             #
#    If Boost, Eigen3 and netCDF are installed properly, this should          #
#    run just fine. Otherwise, edit CMakeCustom.txt to set the proper paths.  #
#    You can use CMakeCustom.txt also to add your own settings.               #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.8)
# ----- Project definition and configuration -------------------------------- #
project(SMS++_project)
include(CMakeCustom.txt OPTIONAL)

# We provide this in case subprojects need to know
set(SMS++_CONFIG_ALL ON)

# ----- Subprojects --------------------------------------------------------- #
# Comment out the ones you do not need.

add_subdirectory(SMS++)
add_subdirectory(MCFClass)
add_subdirectory(MCFBlock)
add_subdirectory(SMILPBlock)
add_subdirectory(MILPSolver)
add_subdirectory(UCBlock)
