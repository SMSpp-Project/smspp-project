# =============================================================================
# tests/ScenarioReduction/CFL/CMakeLists.txt
# Builds all CFL scenario reduction executables:
#   - CFLScenarioGenerator
#   - cfl_scenario_reduction_test
#   - cfl_cssc_test
#   - cfl_full_tss
# =============================================================================

if (NOT hasParent)
    find_package(CapacitatedFacilityLocationBlock REQUIRED)
    find_package(StochasticBlock REQUIRED)
    find_package(TwoStageStochasticBlock REQUIRED)
    find_package(MILPSolver REQUIRED)
endif ()

if (TARGET SMS++::StochasticBlock AND TARGET SMS++::TwoStageStochasticBlock)

    # ---- CFL Scenario Generator ------------------------------------------- #
    add_executable(CFLScenarioGenerator
        ${CMAKE_CURRENT_SOURCE_DIR}/CFLScenarioGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CFLScenarioReductionTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/AbstractScenarioReductionTest.cpp)
    target_include_directories(CFLScenarioGenerator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(CFLScenarioGenerator PRIVATE
        SMS++::CapacitatedFacilityLocationBlock
        SMS++::StochasticBlock
        SMS++::TwoStageStochasticBlock)
    if (TARGET SMS++::MILPSolver)
        target_link_libraries(CFLScenarioGenerator PRIVATE SMS++::MILPSolver)
    endif ()
    # ---- End CFL Scenario Generator --------------------------------------- #

    # ---- CFL Scenario Reduction Test -------------------------------------- #
    add_executable(cfl_scenario_reduction_test
        ${CMAKE_CURRENT_SOURCE_DIR}/cfl_scenario_reduction_main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CFLScenarioReductionTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/AbstractScenarioReductionTest.cpp)
    target_include_directories(cfl_scenario_reduction_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(cfl_scenario_reduction_test PRIVATE
        SMS++::CapacitatedFacilityLocationBlock
        SMS++::StochasticBlock
        SMS++::TwoStageStochasticBlock)
    if (TARGET SMS++::MILPSolver)
        target_link_libraries(cfl_scenario_reduction_test PRIVATE SMS++::MILPSolver)
    endif ()
    # ---- End CFL Scenario Reduction Test ---------------------------------- #

    # ---- CFL CSSC Test ---------------------------------------------------- #
    add_executable(cfl_cssc_test
        ${CMAKE_CURRENT_SOURCE_DIR}/CFLCSSCScenarioReductionTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CFLScenarioReductionTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/AbstractScenarioReductionTest.cpp)
    target_include_directories(cfl_cssc_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(cfl_cssc_test PRIVATE
        SMS++::CapacitatedFacilityLocationBlock
        SMS++::StochasticBlock
        SMS++::TwoStageStochasticBlock
        SMS++::MILPSolver)
    # ---- End CFL CSSC Test ------------------------------------------------ #

    # ---- Full TSS Solver -------------------------------------------------- #
    add_executable(cfl_full_tss
        ${CMAKE_CURRENT_SOURCE_DIR}/CFLTwoStageStochasticTest.cpp)
    target_include_directories(cfl_full_tss PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(cfl_full_tss PRIVATE
        SMS++::CapacitatedFacilityLocationBlock
        SMS++::StochasticBlock
        SMS++::TwoStageStochasticBlock
        SMS++::MILPSolver)
    # ---- End Full TSS Solver ---------------------------------------------- #

endif ()