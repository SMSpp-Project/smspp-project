# SMS++, built from the tarball of a release of the umbrella, which carries
# the core, every module and the tools. Each module is an option, named by
# the acronym of packaging/modules.py, i.e., the vcpkg feature and the Debian
# package of the module: `-o "smspp/*:sddp=False"` leaves SDDPBlock out, and
# a module brings in the modules it needs. A consumer finds each module by
# the CMake configuration that SMS++ installs, e.g., find_package(UCBlock)
# and the target SMS++::UCBlock, which carries what the factory of the
# module needs at link time.

import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get, rm, rmdir

required_conan_version = ">=2.0"

# BEGIN of the table written by gen_recipes.py
_MODULES = [
    ('SMS++', 'core', [], ['boost', 'eigen', 'netcdf-cxx4'], [],
     'Core library of the SMS++ modelling framework'),
    ('BinaryKnapsackBlock', 'bkb', ['core'], [], ['openmp'],
     'Binary knapsack Block and its dynamic programming Solver'),
    ('MCFBlock', 'mcf', ['core'], [], [],
     'Min-cost flow Block'),
    ('CapacitatedFacilityLocationBlock', 'cflb', ['bkb', 'mcf'], [], [],
     'Capacitated facility location Block'),
    ('MMCFBlock', 'mmcf', ['bkb', 'mcf'], [], [],
     'Multicommodity min-cost flow Block'),
    ('LukFiBlock', 'lukfi', ['core'], [], [],
     'Test functions for nonsmooth optimization'),
    ('UCBlock', 'ucblock', ['core'], [], [],
     'Unit Commitment Block, its units and networks'),
    ('StochasticBlock', 'stochastic', ['core'], [], [],
     'Stochastic Block and scenario sets'),
    ('TwoStageStochasticBlock', 'tssb', ['stochastic'], [], [],
     'Two-stage stochastic Block'),
    ('MultiStageStochasticBlock', 'mssb', ['tssb'], [], [],
     'Multi-stage stochastic Block'),
    ('SDDPBlock', 'sddp', ['stochastic'], ['stopt'], ['openmp'],
     'Stochastic Dual Dynamic Programming Block'),
    ('InvestmentBlock', 'investment', ['sddp', 'tssb', 'ucblock'], [], [],
     'Investment (capacity expansion) Block'),
    ('SVMBlock', 'svm', ['core'], [], ['libsvm', 'liblinear'],
     'Support Vector Machine Block'),
    ('SingleFlowDCRBlock', 'sfdcr', ['core'], [], [],
     'Single-flow delay-constrained routing Block'),
    ('SATBlock', 'sat', ['core'], [], ['cadical', 'minisat'],
     'SAT and weighted MaxSAT Block'),
    ('SatellitesBlock', 'satellites', ['core'], [], [],
     'Satellite Constellation Design Problem Block'),
    ('MultiKnapsackAssignBlock', 'mkab', ['bkb'], [], [],
     'Multiple knapsack assignment Block'),
    ('MILPSolver', 'milp', ['core'], [],
     ['highs', 'scip', 'daqp', 'qpoases', 'proxsuite', 'piqp', 'osqp',
      'qpmad'],
     'MILP Solvers, on the open-source solvers of the channel'),
    ('BundleSolver', 'bundle', ['milp'], [], [],
     'Bundle methods for nondifferentiable optimization'),
    ('SubGradSolver', 'subgrad', ['core'], [], [],
     'Subgradient methods for nondifferentiable optimization'),
    ('LagrangianDualSolver', 'lds', ['milp', 'bnx'], [], [],
     'Lagrangian dual Solver'),
    ('FrankWolfeSolver', 'frankwolfe', ['core'], [], [],
     'Frank-Wolfe and Dantzig-Wolfe decomposition Solver'),
    ('BranchAndXSolver', 'bnx', ['core'], [], [],
     'Relaxation-agnostic Branch-and-X Solver'),
    ('BendersDecompositionSolver', 'bds', ['core'], [], [],
     'Generic Benders decomposition Solver'),
    ('MCFClassSolver', 'mcfclass', ['mcf'], [], [],
     'Min-cost flow Solvers of MCFClass'),
    ('MCFLemonSolver', 'mcflemon', ['mcf'], ['lemon'], [],
     'Min-cost flow Solvers of LEMON'),
    ('ScenarioReductionSolver', 'srs', ['tssb'], [], [],
     'Scenario reduction Solvers'),
]
_NEVER = ['CPLEX', 'GUROBI', 'MOSEK', 'PIPS', 'Clarabel', 'quadprog', 'Torch']
_NO_SOURCES = ['COMBO_ROOT', 'RECORD_ROOT']
_CMAKE_NAME = {
    'highs': 'HiGHS',
    'scip': 'SCIP',
    'daqp': 'DAQP',
    'qpoases': 'qpOASES',
    'proxsuite': 'proxsuite',
    'piqp': 'piqp',
    'osqp': 'osqp',
    'qpmad': 'qpmad',
    'cadical': 'CaDiCaL',
    'minisat': 'MiniSat',
    'libsvm': 'LIBSVM',
    'liblinear': 'LIBLINEAR',
}
_ROOT_VAR = {
    'highs': 'HiGHS_ROOT',
    'scip': 'SCIP_ROOT',
    'daqp': 'DAQP_ROOT',
    'qpoases': 'qpOASES_ROOT',
    'proxsuite': 'proxsuite_ROOT',
    'piqp': 'piqp_ROOT',
    'osqp': 'osqp_ROOT',
    'qpmad': 'qpmad_ROOT',
    'cadical': 'CADICAL_ROOT',
    'minisat': 'MINISAT_ROOT',
    'libsvm': 'LIBSVM_ROOT',
    'liblinear': 'LIBLINEAR_ROOT',
    'stopt': 'StOpt_ROOT',
    'lemon': 'LEMON_ROOT',
}
# END of the table written by gen_recipes.py

# the Conan package of each external of packaging/modules.py, None where
# there is none that the modules can use: netcdf-cxx4 and stopt are recipes
# of the SMS++ remote, the osqp of Conan Center is 0.6 while MILPSolver needs
# the API of 1.0, and its minisat is 2.2.0, older than the one SATBlock
# needs; CPLEX, Gurobi and MOSEK are not redistributable
_EXTERNALS = {
    "boost": "boost/1.88.0",
    "eigen": "eigen/3.4.0",
    "netcdf-cxx4": "netcdf-cxx4/4.3.1",
    "openmp": None,
    "stopt": "stopt/7.1",
    "libsvm": "libsvm/333",
    "liblinear": None,
    "cadical": None,
    "minisat": None,
    "highs": "highs/1.15.1",
    "scip": "scip/10.0.2",
    "osqp": None,
    "proxsuite": None,
    "qpoases": "qpoases/3.2.1",
    "daqp": None,
    "piqp": None,
    "qpmad": None,
    "lemon": "coin-lemon/1.3.1",
}

_DIR = {m[1]: m[0] for m in _MODULES}
_NEEDS = {m[1]: m[2] for m in _MODULES}


class SmsppConan(ConanFile):
    name = "smspp"
    description = ("Framework to model and solve block-structured "
                   "mathematical optimization problems")
    license = "LGPL-3.0-only"
    url = "https://gitlab.com/smspp/smspp-project"
    homepage = "https://smspp.gitlab.io/"
    topics = ("optimization", "decomposition", "milp", "lagrangian",
              "bundle-methods")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "tools": [True, False],
        "with_scip": [True, False],
        **{m[1]: [True, False] for m in _MODULES if m[1] != "core"},
    }
    # the libraries are shared by default: the classes of a module register
    # themselves in the factories from static objects, which a static
    # library loses unless whoever links it takes it whole
    default_options = {
        "shared": True,
        "fPIC": True,
        "tools": True,
        "with_scip": False,
        **{m[1]: all(_EXTERNALS[e] for e in m[3])
           for m in _MODULES if m[1] != "core"},
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        # the Find module of SCIP in MILPSolver links the library of SCIP
        # alone, which only a shared library makes enough
        if self.options.with_scip:
            self.options["scip"].shared = True

    def layout(self):
        cmake_layout(self, src_folder="src")

    def _modules(self):
        """The modules chosen and those they need, the core included."""
        out, todo = set(), ["core"] + [
            m[1] for m in _MODULES
            if m[1] != "core" and self.options.get_safe(m[1])]
        while todo:
            a = todo.pop()
            if a not in out:
                out.add(a)
                todo += _NEEDS[a]
        return [m[1] for m in _MODULES if m[1] in out]

    def _externals(self):
        out = []
        for m in _MODULES:
            if m[1] in self._modules():
                for e in m[3] + m[4]:
                    if e == "scip" and not self.options.with_scip:
                        continue
                    if _EXTERNALS[e] and e not in out:
                        out.append(e)
        return out

    def requirements(self):
        # the Boost of the recipe wins over the one SoPlex pins, which the
        # SCIP of Conan Center brings in
        for e in self._externals():
            self.requires(_EXTERNALS[e], transitive_headers=True,
                          force=(e == "boost"))

    def validate(self):
        check_min_cppstd(self, 20)
        for a in self._modules():
            m = next(x for x in _MODULES if x[1] == a)
            missing = [e for e in m[3] if not _EXTERNALS[e]]
            if missing:
                raise ConanInvalidConfiguration(
                    f"{m[0]} needs {', '.join(missing)}, not in Conan")

    def source(self):
        # the tarball of a release has no top directory, and FastFlow, which
        # the core fetches at configure time, is a source of its own
        data = self.conan_data["sources"][self.version]
        get(self, **data["smspp"], strip_root=False)
        get(self, **data["fastflow"], strip_root=True,
            destination="fastflow")
        # the settings of the umbrella for its developers give way to these
        rm(self, "CMakeSettings.txt", self.source_folder)

    def generate(self):
        # the Find modules of SMS++ look for the solvers where they are
        # told, as in the other channels, and not under the defaults of the
        # umbrella, several of which are in /opt
        self.conf.define(
            "tools.cmake.cmaketoolchain:find_package_prefer_config", False)
        tc = CMakeToolchain(self)
        tc.cache_variables["BUILD_tests"] = False
        tc.cache_variables["BUILD_TESTING"] = False
        tc.cache_variables["BUILD_experiments"] = False
        tc.cache_variables["BUILD_tools"] = bool(self.options.tools)
        tc.cache_variables["USE_MPI"] = False
        tc.cache_variables["FETCHCONTENT_SOURCE_DIR_FASTFLOW"] = \
            os.path.join(self.source_folder, "fastflow").replace("\\", "/")
        tc.cache_variables["FETCHCONTENT_FULLY_DISCONNECTED"] = True
        mods = self._modules()
        for m in _MODULES:
            if m[1] != "core":
                tc.cache_variables[f"BUILD_{m[0]}"] = m[1] in mods
        exts = self._externals()
        disabled = list(_NEVER) + [n for e, n in _CMAKE_NAME.items()
                                   if e not in exts]
        for p in disabled:
            tc.cache_variables[f"CMAKE_DISABLE_FIND_PACKAGE_{p}"] = True
        for e in exts:
            if e in _ROOT_VAR:
                dep = self.dependencies[_EXTERNALS[e].split("/")[0]]
                tc.cache_variables[_ROOT_VAR[e]] = \
                    dep.package_folder.replace("\\", "/")
        for v in _NO_SOURCES:
            tc.cache_variables[v] = "OFF"
        # the Find module of LEMON looks for the name lemon everywhere before
        # the name emon of its CMake build, and would take the liblemon of
        # the system
        if "lemon" in exts:
            tc.cache_variables["LEMON_LIBRARY"] = os.path.join(
                self.dependencies["coin-lemon"].package_folder, "lib",
                "libemon.a").replace("\\", "/")
        # FindBoost, which the Find module of StOpt calls for the timer of
        # Boost, looks for its libraries in the package of Boost, since the
        # configuration that CMakeDeps writes does not tell it components
        tc.cache_variables["Boost_NO_BOOST_CMAKE"] = True
        tc.cache_variables["BOOST_ROOT"] = \
            self.dependencies["boost"].package_folder.replace("\\", "/")
        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", self.source_folder,
             os.path.join(self.package_folder, "licenses"))
        CMake(self).install()
        rmdir(self, os.path.join(self.package_folder, "share", "doc"))

    def package_info(self):
        # the CMake configuration of each module is the one SMS++ installs,
        # under lib/cmake/<module>, which find_package(<module>) finds once
        # the package is on CMAKE_PREFIX_PATH
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.builddirs = [""]
        self.cpp_info.includedirs = ["include", "include/SMS++"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.bindirs = ["bin"]
