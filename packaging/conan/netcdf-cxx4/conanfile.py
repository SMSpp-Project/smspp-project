# netCDF-C++, the C++ interface of netCDF that the core of SMS++ needs, which
# Conan Center does not carry. Its own CMake looks for netCDF-C and HDF5 by
# paths and variables that the packages of Conan do not set, so that the
# library, a handful of sources over netCDF-C, is built by the CMakeLists.txt
# of this recipe.

import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get

required_conan_version = ">=2.0"


class NetcdfCxx4Conan(ConanFile):
    name = "netcdf-cxx4"
    description = "C++ interface of the netCDF library"
    license = "BSD-3-Clause"
    url = "https://gitlab.com/smspp/smspp-project"
    homepage = "https://github.com/Unidata/netcdf-cxx4"
    topics = ("netcdf", "scientific-data")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    exports_sources = "CMakeLists.txt"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires("netcdf/4.8.1", transitive_headers=True)

    def source(self):
        get(self, **self.conan_data["sources"][self.version],
            strip_root=True)

    def generate(self):
        CMakeToolchain(self).generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder=os.path.join(
            self.source_folder, os.pardir))
        cmake.build()

    def package(self):
        copy(self, "COPYRIGHT", self.source_folder,
             os.path.join(self.package_folder, "licenses"))
        CMake(self).install()

    def package_info(self):
        # the names by which the core of SMS++ finds it
        self.cpp_info.set_property("cmake_file_name", "netCDFCxx")
        self.cpp_info.set_property("cmake_target_name", "netCDF::netCDFCxx")
        self.cpp_info.libs = ["netcdf-cxx4"]
