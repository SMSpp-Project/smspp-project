# MOSEK, which SMS++ can use but which is not redistributable: Spack takes
# it only as an external, e.g., in packages.yaml
#
#   packages:
#     mosek:
#       buildable: false
#       externals:
#       - spec: mosek@11.0
#         prefix: /opt/mosek/11.0

from spack_repo.builtin.build_systems.generic import Package

from spack.package import *


class Mosek(Package):
    """MOSEK: solver of linear, quadratic and conic programs. It is not
    redistributable, hence this package only stands for a local
    installation declared as an external."""

    homepage = "https://www.mosek.com"
    has_code = False

    license("LicenseRef-MOSEK")

    version("11.0")
    version("10.2")

    def install(self, spec, prefix):
        raise InstallError(
            "MOSEK is not redistributable: declare a local installation as "
            "the external mosek in packages.yaml (prefix: the directory of "
            "its version, e.g., /opt/mosek/11.0)")
