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
#                                                                             #
#                                 Donato Meoli                                #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.12)

# ----- Project definition and configuration -------------------------------- #
project("The SMS++ Project")

# This file can contain custom settings.
# Use it as an alternative to specify CMake variables manually with -D.
include(CMakeSettings.txt OPTIONAL)

# Read the dependencies default location from a file written in a
# bash-style syntax since it is shared with the other hand-written Makefiles,
# so we define a function to convert these macros written as `lhs = rhs` into
# their equivalent cmake variants, i.e., `set(lhs rhs)`, avoiding comments.
macro(makefile_macros_to_cmake file)
	FILE(STRINGS ${file} paths)
	while (paths)
		list(POP_FRONT paths path)
		if (path) # ignore empty lines, i.e., \n
			# makes empty the comment lines, i.e. those start with #...
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
						# ... then set the one defined in makefile-default-paths
						set("${lhs}" ${rhs})
						message(STATUS "Specify ${lhs}: " ${rhs})
					endif ()
				endif ()

				unset(lhs)
				unset(rhs)
			endif ()
		endif ()
	endwhile ()

	unset(path)
	unset(paths)
endmacro()

# firstly, set the custom makefile-paths, if any...
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extlib/makefile-paths")
	makefile_macros_to_cmake(extlib/makefile-paths)
endif ()
# ... then set the default other ones ignoring those already previously set
makefile_macros_to_cmake(extlib/makefile-default-paths)

# Retrieve the system architecture
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	if (UNIX)
		set(ARCH x86-64)
	else () # Windows
		set(ARCH x64)
	endif ()
else ()
	set(ARCH x86)
endif ()

# Some easier way to alleviate the cmake call and pick the directories from a ROOT
if (COIN_ROOT_DIR)
	set(Clp_ROOT ${COIN_ROOT_DIR})
	set(Osi_ROOT ${COIN_ROOT_DIR})
	set(CoinUtils_ROOT ${COIN_ROOT_DIR})
endif ()

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
add_subdirectory(SDDPBlock)       # Requires StochasticBlock
add_subdirectory(CapacitatedFacilityLocationBlock)
                                  # Requires MCFBlock, BinaryKnapsackBlock
add_subdirectory(InvestmentBlock) # Requires SDDPBlock, UCBlock

# Solvers
add_subdirectory(MILPSolver)
add_subdirectory(BundleSolver)    # Requires MILPSolver
add_subdirectory(LagrangianDualSolver)
#add_subdirectory(SubGradSolver)

# ----- Tools --------------------------------------------------------------- #
add_subdirectory(tools)

# ----- System tests -------------------------------------------------------- #
add_subdirectory(tests)

# --------------------------------------------------------------------------- #
