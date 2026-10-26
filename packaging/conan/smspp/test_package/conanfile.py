# Links a program against MCFBlock and makes a Block of it by the factory,
# which works only if the library of the module has been kept at link time,
# and solves the example of the min-cost flow tool.

import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, cmake_layout


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not can_run(self):
            return
        self.run(os.path.join(self.cpp.build.bindir, "test_package"),
                 env="conanrun")
        smspp = self.dependencies["smspp"]
        if smspp.options.tools and smspp.options.mcf:
            example = os.path.join(smspp.package_folder, "share",
                                   "SMS++_tools", "mcfblock_solver",
                                   "examples", "example.dmx")
            self.run(f"smspp_mcfblock_solver {example}", env="conanrun")
