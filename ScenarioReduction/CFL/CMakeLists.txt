# --------------------------------------------------------------------------- #
#    CMake file for tests/ScenarioReduction/CFL                               #
# --------------------------------------------------------------------------- #
#
# Builds the CFL scenario reduction test executable
# Replaces the old test that used the sr_cfl hack
#
# --------------------------------------------------------------------------- #

cmake_minimum_required( VERSION 3.20 )

# Force linking for static libraries to avoid the linker discarding symbols
if( BUILD_SHARED_LIBS )
    if( UNIX AND NOT APPLE )
        add_link_options( "-Wl,--no-as-needed" )
    endif()
else()
    if( MSVC )
        add_link_options( "/WHOLEARCHIVE" "/FORCE:MULTIPLE" )
    elseif( APPLE )
        add_link_options( "-Wl,-all_load" )
    else()
        add_link_options( "-Wl,--whole-archive,--allow-multiple-definition" )
    endif()
endif()

# --------------------------------------------------------------------------- #
#  cfl_scenario_reduction_test                                                 #
# --------------------------------------------------------------------------- #

set( SR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. )

# --------------------------------------------------------------------------- #
#  CFLScenarioGenerator                                                        #
# --------------------------------------------------------------------------- #
add_executable( CFLScenarioGenerator
    ${CMAKE_CURRENT_SOURCE_DIR}/CFLScenarioGenerator.cpp
)
target_link_libraries( CFLScenarioGenerator PRIVATE
    SMS++::ScenarioReductionSolver
    SMS++::StochasticBlock
    SMS++::TwoStageStochasticBlock
    SMS++::CapacitatedFacilityLocationBlock
    SMS++::MILPSolver
)
target_include_directories( CFLScenarioGenerator PRIVATE
    ${CMAKE_SOURCE_DIR}/ScenarioReductionSolver/include
    ${CMAKE_SOURCE_DIR}/StochasticBlock/include
    ${CMAKE_SOURCE_DIR}/TwoStageStochasticBlock/include
    ${CMAKE_SOURCE_DIR}/CapacitatedFacilityLocationBlock/include
    ${SR_ROOT}/include
)

add_executable( cfl_scenario_reduction_test
    ${CMAKE_CURRENT_SOURCE_DIR}/CFLScenarioReductionTest.cpp
    ${SR_ROOT}/src/CSSCScenarioReductionSolver.cpp
    ${SR_ROOT}/src/ScenarioReductionSolver.cpp
)

target_link_libraries( cfl_scenario_reduction_test PRIVATE
    SMS++::StochasticBlock
    SMS++::TwoStageStochasticBlock
    SMS++::CapacitatedFacilityLocationBlock
    SMS++::MILPSolver
)

target_include_directories( cfl_scenario_reduction_test PRIVATE
    ${SR_ROOT}/include      # FIRST: our headers win over any same-named library ones
    ${CMAKE_SOURCE_DIR}/StochasticBlock/include
    ${CMAKE_SOURCE_DIR}/TwoStageStochasticBlock/include
    ${CMAKE_SOURCE_DIR}/CapacitatedFacilityLocationBlock/include
)

if( WIN32 AND BUILD_SHARED_LIBS AND NOT DEFINED ENV{CONDA_BUILD} )
    add_custom_command( TARGET cfl_scenario_reduction_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:cfl_scenario_reduction_test>
            $<TARGET_FILE_DIR:cfl_scenario_reduction_test>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs"
    )
endif()

# --------------------------------------------------------------------------- #
#  CTest integration                                                           #
# --------------------------------------------------------------------------- #

# Smoke test: run with a small instance if available
if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/cap41.nc4" AND
    EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/cap41_scenarios.nc4" )

    add_test( NAME cfl_sr_dupacova
        COMMAND cfl_scenario_reduction_test
            -i ${CMAKE_CURRENT_SOURCE_DIR}/data/cap41.nc4
            -f ${CMAKE_CURRENT_SOURCE_DIR}/data/cap41_scenarios.nc4
            -k 5
            -m dupacova
            -s ${CMAKE_CURRENT_SOURCE_DIR}/BSPar_CPLEX.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test( NAME cfl_sr_cssc
        COMMAND cfl_scenario_reduction_test
            -i ${CMAKE_CURRENT_SOURCE_DIR}/data/cap41.nc4
            -f ${CMAKE_CURRENT_SOURCE_DIR}/data/cap41_scenarios.nc4
            -k 5
            -m cssc
            -s ${CMAKE_CURRENT_SOURCE_DIR}/BSPar_CPLEX.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

endif()