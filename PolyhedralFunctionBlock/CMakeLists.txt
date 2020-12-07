# ----- Requirements -------------------------------------------------------- #
find_package(BundleSolver)
find_package(MILPSolver)

# ----- Tests --------------------------------------------------------------- #
if (BundleSolver_FOUND AND MILPSolver_FOUND)
    add_executable(PolyhedralFunctionBlock_test test.cpp)
    target_link_libraries(PolyhedralFunctionBlock_test PRIVATE
                          SMS++::BundleSolver
                          SMS++::MILPSolver)

    # Copy all the *Par.txt files and batch shell script
    # so it can run from the build directory.
    file(GLOB parFiles "*Par.txt")
    foreach (parFile ${parFiles})
        configure_file(${parFile} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
    endforeach ()
    configure_file(batch ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

    # Add to CTest suite
    add_test(NAME PolyhedralFunctionBlock_test
             COMMAND bash "${CMAKE_CURRENT_BINARY_DIR}/batch")
endif ()
