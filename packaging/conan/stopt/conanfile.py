# StOpt, the library of stochastic optimization that SDDPBlock of SMS++
# needs, which Conan Center does not carry; the geners serialization library
# travels inside it and is built with it.

import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get

required_conan_version = ">=2.0"


class StoptConan(ConanFile):
    name = "stopt"
    description = ("Library for stochastic optimization by dynamic "
                   "programming, regression methods and SDDP")
    license = "LGPL-3.0-only"
    url = "https://gitlab.com/smspp/smspp-project"
    homepage = "https://gitlab.com/stochastic-control/StOpt"
    topics = ("stochastic-optimization", "dynamic-programming", "sddp")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires("boost/1.88.0", transitive_headers=True)
        self.requires("eigen/3.4.0", transitive_headers=True)
        self.requires("zlib/[>=1.2.11 <2]")
        self.requires("bzip2/1.0.8")

    def source(self):
        get(self, **self.conan_data["sources"][self.version],
            strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        # the SDDP and DP CUTS switches build tests, not the library
        tc.cache_variables["BUILD_PYTHON"] = False
        tc.cache_variables["BUILD_TEST"] = False
        tc.cache_variables["BUILD_SDDP"] = False
        tc.cache_variables["BUILD_DPCUTS"] = False
        tc.cache_variables["BUILD_MPI"] = False
        tc.cache_variables["BUILD_SYSTEM_INSTALL"] = True
        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE*", self.source_folder,
             os.path.join(self.package_folder, "licenses"))
        CMake(self).install()

    def package_info(self):
        self.cpp_info.libs = ["StOpt", "geners"]
        if self.settings.os in ("Linux", "FreeBSD"):
            self.cpp_info.system_libs = ["m", "pthread"]
