# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ umbrella project                               #
#                                                                             #
#    The goal of this list file is to provide a quick way to configure        #
#    and build all the SMS++ projects together. It doesn't do much besides    #
#    calling the list files of the subprojects.                               #
#                                                                             #
#                              Niccolo' Iardella                              #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.19)

# ----- Policies ------------------------------------------------------------ #
# Introduced in 3.15 to handle arbitrary characters in test names
# See: https://cmake.org/cmake/help/latest/policy/CMP0091.html
if (POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW)
endif ()

# ----- Project definition and configuration -------------------------------- #
project("The SMS++ Project")

# This file can contain custom settings.
# Use it as an alternative to specify CMake variables manually with -D.
include(CMakeSettings.txt OPTIONAL)

# Define options for modules in the order of the blocks and solvers

# Blocks
option(BUILD_BinaryKnapsackBlock "Build BinaryKnapsackBlock" OFF)
option(BUILD_UCBlock "Build UCBlock" OFF)
option(BUILD_MCFBlock "Build MCFBlock" OFF)
option(BUILD_MMCFBlock "Build MMCFBlock" OFF)
option(BUILD_LukFiBlock "Build LukFiBlock" OFF)
option(BUILD_StochasticBlock "Build StochasticBlock" OFF)
option(BUILD_SDDPBlock "Build SDDPBlock" OFF)
option(BUILD_CapacitatedFacilityLocationBlock "Build CapacitatedFacilityLocationBlock" OFF)
option(BUILD_InvestmentBlock "Build InvestmentBlock" OFF)

# Solvers
option(BUILD_MILPSolver "Build MILPSolver" OFF)
option(BUILD_BundleSolver "Build BundleSolver" OFF)
option(BUILD_LagrangianDualSolver "Build LagrangianDualSolver" OFF)
option(BUILD_LEMONSolver "Build LEMONSolver" OFF)

option(BUILD_tools "Build tools" OFF)
option(BUILD_tests "Build system tests" OFF)

# Read the dependencies default location from a file written in a
# bash-style syntax since it is shared with the other hand-written Makefiles,
# so we define a function to convert these macros written as `lhs = rhs` into
# their equivalent cmake variants, i.e., `set(lhs rhs)`, avoiding comments.
macro(makefile_macros_to_cmake file)
	FILE(STRINGS ${file} paths)
	foreach (path IN LISTS paths)
		if (path) # ignore empty lines, i.e., \n
			# makes empty the comment lines, i.e., those start with #...
			string(REGEX REPLACE "^[ \t]*[#].*" "" path ${path})
			if (path) # ... and then ignore them

				string(REGEX MATCH ".*[=]" lhs "${path}") # retrieve the lhs
				string(REGEX REPLACE "=" "" lhs ${lhs}) # remove =
				# remove tab or white spaces from head
				string(STRIP "${lhs}" lhs)

				# if it's not already defined before in custom makefile-paths...
				if (NOT DEFINED ${lhs})
					# then read its rhs...
					string(REGEX MATCH "[=].*" rhs "${path}") # retrieve the rhs
					# remove comments starting with # from tails, if any
					string(REGEX REPLACE "[#].*" "" rhs ${rhs})
					string(REGEX REPLACE "=" "" rhs ${rhs}) # remove =
					string(REGEX REPLACE "\"" "" rhs ${rhs}) # remove ", if any
					# remove tab or white space from head or tail
					string(STRIP "${rhs}" rhs)

					# ... and if it is not a default path in Unix...
					if (NOT (rhs STREQUAL /usr OR
							 rhs STREQUAL /usr/local))
						# ... then set the one defined in makefile-default-paths-*
						set("${lhs}" ${rhs})
						#message(STATUS "Specify ${lhs}: " ${rhs})
					endif ()
				endif ()

				unset(lhs)
				unset(rhs)
			endif ()
		endif ()
	endforeach ()

	unset(path)
	unset(paths)
endmacro ()

# firstly, set the custom makefile-paths, if any...
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extlib/makefile-paths")
	makefile_macros_to_cmake(extlib/makefile-paths)
endif ()
# ... then set the default other ones ignoring those already previously set,
# according to the specific OS
if (UNIX)
	if (APPLE)
		makefile_macros_to_cmake(extlib/makefile-default-paths-macos)
	else ()
		makefile_macros_to_cmake(extlib/makefile-default-paths-linux)
	endif ()
elseif (WIN32)
	makefile_macros_to_cmake(extlib/makefile-default-paths-win)
endif ()

# Retrieve the system architecture
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	if (UNIX)
		set(ARCH x86-64)
	elseif (WIN32)
		set(ARCH x64)
	endif ()
else ()
	set(ARCH x86)
endif ()

if (MSVC)
	set(MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

# ----- Handle dependencies ------------------------------------------------- #
# Resolve dependencies before adding subdirectories

# Blocks
if (BUILD_MMCFBlock)
	set(BUILD_MCFBlock ON) # Requires MCFBlock
	set(BUILD_BinaryKnapsackBlock ON) # Requires BinaryKnapsackBlock
endif ()

if (BUILD_SDDPBlock)
	set(BUILD_StochasticBlock ON) # Requires StochasticBlock
endif ()

if (BUILD_CapacitatedFacilityLocationBlock)
	set(BUILD_MCFBlock ON) # Requires MCFBlock
	set(BUILD_BinaryKnapsackBlock ON) # Requires BinaryKnapsackBlock
endif ()

if (BUILD_InvestmentBlock)
	set(BUILD_SDDPBlock ON) # Requires SDDPBlock
	set(BUILD_UCBlock ON) # Requires UCBlock
endif ()

# Solvers
if (BUILD_BundleSolver)
	set(BUILD_MILPSolver ON) # Requires MILPSolver
endif ()

if (BUILD_LEMONSolver)
	set(BUILD_MCFBlock ON) # Requires MCFBlock
endif ()

# ----- Initialize submodules ------------------------------------------------ #
macro(init_submodule name)
	execute_process(
			COMMAND git submodule update --init --recursive ${name}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
endmacro()

# ----- Subprojects --------------------------------------------------------- #
# The core library is required by all other SMS++ modules, if any
# Requires Boost (headers only), netCDF-C++ and Eigen3.
set(SMSPP_REQUIRED OFF)

# Blocks
if (BUILD_BinaryKnapsackBlock)
	init_submodule(BinaryKnapsackBlock)
	add_subdirectory(BinaryKnapsackBlock)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_UCBlock)
	init_submodule(UCBlock)
	add_subdirectory(UCBlock)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_MCFBlock)
	init_submodule(MCFBlock)
	add_subdirectory(MCFBlock) # Requires MCFClass
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_MMCFBlock)
	init_submodule(MMCFBlock)
	add_subdirectory(MMCFBlock)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_LukFiBlock)
	init_submodule(LukFiBlock)
	add_subdirectory(LukFiBlock)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_StochasticBlock)
	init_submodule(StochasticBlock)
	add_subdirectory(StochasticBlock)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_SDDPBlock)
	init_submodule(SDDPBlock)
	add_subdirectory(SDDPBlock)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_CapacitatedFacilityLocationBlock)
	init_submodule(CapacitatedFacilityLocationBlock)
	add_subdirectory(CapacitatedFacilityLocationBlock)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_InvestmentBlock)
	init_submodule(InvestmentBlock)
	add_subdirectory(InvestmentBlock)
	set(SMSPP_REQUIRED ON)
endif ()

# Solvers
if (BUILD_MILPSolver)
	init_submodule(MILPSolver)
	add_subdirectory(MILPSolver)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_BundleSolver)
	init_submodule(BundleSolver)
	add_subdirectory(BundleSolver)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_LagrangianDualSolver)
	init_submodule(LagrangianDualSolver)
	add_subdirectory(LagrangianDualSolver)
	set(SMSPP_REQUIRED ON)
endif ()

if (BUILD_LEMONSolver)
	init_submodule(LEMONSolver)
	add_subdirectory(LEMONSolver)
	set(SMSPP_REQUIRED ON)
endif ()

# ----- Tools --------------------------------------------------------------- #
if (BUILD_tools)
	init_submodule(tools)
	add_subdirectory(tools)
	set(SMSPP_REQUIRED ON)
endif ()

# ----- System tests -------------------------------------------------------- #
if (BUILD_tests)
	init_submodule(tests)
	add_subdirectory(tests)
	set(SMSPP_REQUIRED ON)
endif ()

# ----- SMS++ --------------------------------------------------------------- #
if (SMSPP_REQUIRED)
	init_submodule(SMS++)
	add_subdirectory(SMS++)
endif ()

# --------------------------------------------------------------------------- #
