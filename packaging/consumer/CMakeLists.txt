cmake_minimum_required(VERSION 3.21)
project(consumer LANGUAGES CXX)

find_package(UCBlock REQUIRED)
find_package(MILPSolver REQUIRED)
add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE SMS++::UCBlock SMS++::MILPSolver)
