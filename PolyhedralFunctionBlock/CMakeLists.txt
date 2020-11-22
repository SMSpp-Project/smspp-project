# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND)
    add_executable(PolyhedralFunctionBlock_test test.cpp)
    target_link_libraries(PolyhedralFunctionBlock_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

#    add_test(NAME PolyhedralFunctionBlock_test
#             COMMAND PolyhedralFunctionBlock_test
#             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
