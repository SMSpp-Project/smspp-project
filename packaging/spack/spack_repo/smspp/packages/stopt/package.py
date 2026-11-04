# The StOpt library of stochastic optimization, which SDDPBlock of SMS++
# needs and the builtin repository of Spack does not carry.

from spack_repo.builtin.build_systems.cmake import CMakePackage

from spack.package import *


class Stopt(CMakePackage):
    """StOpt: library for stochastic optimization by dynamic programming,
    regression methods and SDDP."""

    homepage = "https://gitlab.com/stochastic-control/StOpt"
    url = ("https://gitlab.com/stochastic-control/StOpt/-/archive/v7.1/"
           "StOpt-v7.1.tar.gz")

    license("LGPL-3.0-only")

    # BEGIN of the table written by gen_recipes.py
    version(
        '7.1',
        sha256='8bee8f2aab097c91a791729cde34f50b'
        '062208a1ab1af4d62ee41dba24a9e5e7',
    )
    # END of the table written by gen_recipes.py

    depends_on("c", type="build")
    depends_on("cxx", type="build")
    depends_on("cmake@3.16:", type="build")
    depends_on("boost+random+serialization+system")
    depends_on("eigen@3.4:")
    depends_on("zlib-api")
    depends_on("bzip2")

    def cmake_args(self):
        # the geners serialization library travels inside StOpt and is built
        # with it; the SDDP and DP CUTS switches build tests, not the library
        return [
            self.define("BUILD_SHARED_LIBS", True),
            self.define("BUILD_PYTHON", False),
            self.define("BUILD_TEST", False),
            self.define("BUILD_SDDP", False),
            self.define("BUILD_DPCUTS", False),
            self.define("BUILD_MPI", False),
            self.define("BUILD_SYSTEM_INSTALL", True),
        ]
