# ----- Requirements -------------------------------------------------------- #
find_package(StochasticBlock)

# ----- BendersBFunction_test ----------------------------------------------- #
if (StochasticBlock_FOUND)
    add_executable(StochasticBlock_test test.cpp)
    target_link_libraries(StochasticBlock_test PRIVATE
                          SMS++::StochasticBlock)
endif ()
