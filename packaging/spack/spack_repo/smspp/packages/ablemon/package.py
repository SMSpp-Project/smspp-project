# abLEMON, the fork of LEMON that MCFLemonSolver of SMS++ needs, which the
# builtin repository of Spack does not carry.

from spack_repo.builtin.build_systems.cmake import CMakePackage

from spack.package import *


class Ablemon(CMakePackage):
    """abLEMON: a maintained fork of LEMON, a C++ library of graphs and of
    network optimization."""

    homepage = "https://github.com/dmeoli/abLEMON"
    url = ("https://github.com/dmeoli/abLEMON/archive/refs/tags/"
           "1.7.0.tar.gz")

    license("BSL-1.0 AND BSD-3-Clause")

    # BEGIN of the table written by gen_recipes.py
    version(
        '1.7.0',
        sha256='6ca9da3d115557c2dc914f58f3dbcdb2'
        '186808ed98c92a306160b1dc7b4d50ee',
    )
    # END of the table written by gen_recipes.py

    depends_on("c", type="build")
    depends_on("cxx", type="build")
    depends_on("cmake@3.19:", type="build")

    @property
    def libs(self):
        # the library is libemon, which the Find module of LEMON takes for
        # lemon if it is told
        return find_libraries("libemon", self.prefix, shared=True,
                              recursive=True)

    def cmake_args(self):
        # the LP and MIP backends of LEMON are left out
        return [self.define("BUILD_SHARED_LIBS", True),
                self.define("LEMON_VERSION", str(self.version))] + \
            [self.define(f"LEMON_BUILD_{o}", False)
             for o in ("TESTS", "DEMOS", "TOOLS", "DOC")] + \
            [self.define(f"LEMON_ENABLE_{o}", False)
             for o in ("GLPK", "ILOG", "COIN", "SOPLEX")]
