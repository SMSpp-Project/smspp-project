# The library of netCDF-C++ out of its sources, for the recipe of Conan.
cmake_minimum_required(VERSION 3.15)
project(netcdf-cxx4 LANGUAGES CXX)

find_package(netCDF REQUIRED CONFIG)
file(GLOB sources src/cxx4/nc*.cpp)
add_library(netcdf-cxx4 ${sources})
target_include_directories(netcdf-cxx4 PUBLIC src/cxx4)
target_link_libraries(netcdf-cxx4 PUBLIC netCDF::netcdf)
set_target_properties(netcdf-cxx4 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(GNUInstallDirs)
install(TARGETS netcdf-cxx4)
file(GLOB headers src/cxx4/nc*.h)
install(FILES ${headers} src/cxx4/netcdf
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
