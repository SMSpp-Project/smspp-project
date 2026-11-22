# --------------------------------------------------------------------------- #
# A program that imports the installed C++20 modules of SMS++.                #
#                                                                             #
# Given the packages of SMSPP_MODULES (e.g., "SMS++;MCFBlock") installed in   #
# SMSPP_PREFIX, it imports the module of each, whose name it reads from the   #
# installed interface, and links them all, so that nightly:modules fails      #
# when a module builds but its installed package cannot be imported.          #
# --------------------------------------------------------------------------- #

cmake_minimum_required(VERSION 3.28)
project(SMSppImport CXX)

set(_src "")
set(_libs "")
foreach (_m IN LISTS SMSPP_MODULES)
    find_package(${_m} CONFIG REQUIRED)
    list(APPEND _libs SMS++::${_m}_module)
    string(REPLACE "+" "p" _f ${_m})
    file(GLOB _cppm "${SMSPP_PREFIX}/include/SMS++/modules/${_f}.cppm")
    if (NOT _cppm)
        message(FATAL_ERROR "no installed interface of ${_m}")
    endif ()
    file(STRINGS ${_cppm} _name REGEX "^export module ")
    string(REGEX MATCH "^export module ([A-Za-z0-9_.]+)" _name "${_name}")
    string(APPEND _src "import ${CMAKE_MATCH_1};\n")
endforeach ()
string(APPEND _src "int main() { return 0; }\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/import.cpp "${_src}")

add_executable(smspp_import ${CMAKE_CURRENT_BINARY_DIR}/import.cpp)
target_compile_features(smspp_import PRIVATE cxx_std_20)
target_link_libraries(smspp_import PRIVATE ${_libs})
