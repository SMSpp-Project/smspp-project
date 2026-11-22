# HiGHS, the open-source MILP solver of MILPSolver of SMS++, which the builtin
# repository of Spack carries only inside its Python binding.

from spack_repo.builtin.build_systems.cmake import CMakePackage

from spack.package import *


class Highs(CMakePackage):
    """HiGHS: high performance software for linear optimization, i.e.,
    linear, mixed-integer and quadratic programs."""

    homepage = "https://highs.dev"
    url = "https://github.com/ERGO-Code/HiGHS/archive/refs/tags/v1.14.0.tar.gz"

    license("MIT")

    version(
        "1.14.0",
        sha256="05931e8dd8c8cac514da8297003c31a2"
        "06a0004d542b7da500810b85c87c20b9",
    )

    depends_on("c", type="build")
    depends_on("cxx", type="build")
    depends_on("cmake@3.15:", type="build")
    depends_on("zlib-api")

    def cmake_args(self):
        return [
            self.define("BUILD_SHARED_LIBS", True),
            self.define("BUILD_TESTING", False),
            self.define("BUILD_EXAMPLES", False),
            self.define("FAST_BUILD", True),
        ]
