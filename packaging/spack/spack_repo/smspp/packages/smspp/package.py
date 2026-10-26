# SMS++, built from the tarball of a release of the umbrella, which carries
# the core, every module and the tools. Each module is a variant, named by
# the acronym of packaging/modules.py, i.e., the vcpkg feature and the
# Debian package of the module: `spack install smspp ~sddp ~investment`
# leaves those out, and a module turns on the modules it needs.

import os

from spack_repo.builtin.build_systems.cmake import CMakePackage

from spack.package import *

# BEGIN of the table written by gen_recipes.py
_RELEASE = '0.7.99'
_SHA256 = 'a33859ae4ca6ca2ab9d6bfce67804e501051eae1780b4f148ef489f52978d91a'
_URL = (
    'https://gitlab.com/api/v4/projects/smspp%2Fsmspp-project/packages/'
    'generic/smspp-project/0.7.99/smspp-project-0.7.99.tar.gz'
)
_FASTFLOW_URL = (
    'https://github.com/fastflow/fastflow/archive/'
    'd476f66ab924d8d122f54b4b90aee00ef979aea8.tar.gz'
)
_FASTFLOW_SHA256 = (
    'fca23ce672fe12fe7cdbfa6955a982d6'
    'eb75816a00d2d97a93deba9b351f9c53'
)
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
    ('HQPBlock', 'hqp', ['core'], [], [],
     'SMS++ Block and Solver for hierarchical (lexicographic) quadratic programs'),
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
_NEVER = ['PIPS', 'Clarabel', 'quadprog', 'HPIPM', 'Torch']
_LOCAL = {
    'cplex': ('CPLEX', 'CPLEX_ROOT'),
    'gurobi': ('GUROBI', 'GUROBI_ROOT'),
    'mosek': ('MOSEK', 'MOSEK_ROOT'),
}
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

# the Spack package of each external of packaging/modules.py, None where
# Spack has none that the modules can use: SCIP is there only as a manual
# download, the OSQP is 0.6, while MILPSolver needs the API of 1.0, and the
# libsvm installs neither its header nor the link of its library
_EXTERNALS = {
    "boost": "boost",
    "eigen": "eigen",
    "netcdf-cxx4": "netcdf-cxx4",
    "openmp": None,
    "stopt": "stopt",
    "libsvm": None,
    "liblinear": None,
    "cadical": None,
    "minisat": None,
    "highs": "highs",
    "scip": None,
    "osqp": None,
    "proxsuite": None,
    "qpoases": None,
    "daqp": None,
    "piqp": None,
    "qpmad": None,
    "lemon": "lemon",
}


class Smspp(CMakePackage):
    """SMS++: a framework to model and solve block-structured mathematical
    optimization problems, by MILP reformulations, Lagrangian and Benders
    decompositions, bundle methods and dynamic programming."""

    homepage = "https://smspp.gitlab.io/"
    git = "https://gitlab.com/smspp/smspp-project.git"

    license("LGPL-3.0-only")

    # the tarball of a release has no top directory, and Spack moves what it
    # holds into the source directory as it is
    version(_RELEASE, sha256=_SHA256, url=_URL)

    # FastFlow, which the core fetches at configure time
    resource(
        name="fastflow",
        url=_FASTFLOW_URL,
        sha256=_FASTFLOW_SHA256,
        destination="",
        placement="fastflow",
    )

    for _dir, _acr, _needs, _req, _opt, _desc in _MODULES:
        if _acr == "core":
            continue
        _ok = all(_EXTERNALS[_e] for _e in _req)
        variant(_acr, default=_ok, description=f"{_dir}: {_desc}")
        for _n in _needs:
            if _n != "core":
                requires(f"+{_n}", when=f"+{_acr}")
        for _e in _req + _opt:
            if _EXTERNALS[_e]:
                depends_on(_EXTERNALS[_e], when=f"+{_acr}")
        if not _ok:
            conflicts(f"+{_acr}", msg=f"{_dir} needs what Spack lacks")
    variant("tools", default=True, description="The command-line tools")
    # the timer of Boost, which SDDPBlock asks for along with StOpt
    depends_on("boost+timer", when="+sddp")

    # CPLEX, Gurobi and MOSEK are never in a package: a variant builds the
    # backend of MILPSolver against the installation of the solver, which
    # Spack knows as an external (cplex and gurobi of the builtin
    # repository, mosek of this one), see the description of each
    for _e, (_name, _root) in _LOCAL.items():
        variant(_e, default=False,
                description=f"The {_name} backend of MILPSolver, on a local "
                f"installation declared as the external {_e}")
        depends_on(_e, when=f"+{_e}")
        requires("+milp", when=f"+{_e}")

    depends_on("c", type="build")
    depends_on("cxx", type="build")
    depends_on("cmake@3.21:", type="build")
    depends_on("help2man", type="build", when="+tools")
    depends_on("boost")
    depends_on("eigen")
    depends_on("netcdf-cxx4")

    def patch(self):
        # the settings of the umbrella for its developers give way to these
        if os.path.exists("CMakeSettings.txt"):
            os.remove("CMakeSettings.txt")

    def cmake_args(self):
        spec = self.spec
        args = [
            self.define("BUILD_SHARED_LIBS", True),
            self.define("BUILD_tests", False),
            self.define("BUILD_TESTING", False),
            self.define("BUILD_experiments", False),
            self.define_from_variant("BUILD_tools", "tools"),
            self.define("USE_MPI", False),
            self.define(
                "FETCHCONTENT_SOURCE_DIR_FASTFLOW",
                join_path(self.stage.source_path, "fastflow"),
            ),
            self.define("FETCHCONTENT_FULLY_DISCONNECTED", True),
        ]
        for d, acr, _, _, _, _ in _MODULES:
            if acr != "core":
                args.append(self.define(f"BUILD_{d}", f"+{acr}" in spec))
        disabled = list(_NEVER) + [
            n for e, n in _CMAKE_NAME.items() if not _EXTERNALS[e]
        ] + [n for e, (n, _) in _LOCAL.items() if f"+{e}" not in spec]
        for p in disabled:
            args.append(self.define(f"CMAKE_DISABLE_FIND_PACKAGE_{p}", True))
        # the externals where Spack has them, and not under the defaults of
        # the umbrella, several of which are in /opt
        for e, v in _ROOT_VAR.items():
            if _EXTERNALS.get(e) and _EXTERNALS[e] in spec:
                args.append(self.define(v, spec[_EXTERNALS[e]].prefix))
        for v in _NO_SOURCES:
            args.append(self.define(v, "OFF"))
        args.append(self.define("SMSPP_EXTLIB_DEFAULTS", False))
        args += self._local_args()
        # the Find module of LEMON looks for the name lemon everywhere before
        # the name emon of its CMake build, and would take the liblemon of
        # the system
        if "+mcflemon" in spec:
            args.append(self.define(
                "LEMON_LIBRARY", join_path(spec["lemon"].prefix.lib,
                                           "libemon.a")))
        return args

    def _local_args(self):
        """The installations of the solvers that are not redistributable."""
        spec, args = self.spec, []
        for e, (name, root) in _LOCAL.items():
            if f"+{e}" not in spec:
                continue
            prefix = spec[e].prefix
            if e == "gurobi":
                # the Find module of Gurobi looks in <root>/linux64, which
                # is what the prefix of the gurobi package holds; the link
                # stays in the prefix of smspp, since the library of Gurobi
                # has no SONAME and the libraries record the path to it
                prefix = join_path(self.prefix, "share", "smspp",
                                   "gurobi-root")
                mkdirp(prefix)
                link = join_path(prefix, "linux64")
                if not os.path.exists(link):
                    symlink(spec[e].prefix, link)
            args.append(self.define(root, prefix))
        return args

    @run_after("cmake")
    def check_local_solvers(self):
        """MILPSolver leaves out a backend whose solver it does not find,
        which a solver asked for must not be."""
        rules = ""
        for root, _, files in os.walk(join_path(self.build_directory,
                                                "MILPSolver")):
            for f in files:
                if f in ("build.make", "build.ninja"):
                    with open(join_path(root, f)) as fh:
                        rules += fh.read()
        ninja = join_path(self.build_directory, "build.ninja")
        if os.path.exists(ninja):
            with open(ninja) as fh:
                rules += fh.read()
        sources = {"cplex": "CPXMILPSolver", "gurobi": "GRBMILPSolver",
                   "mosek": "MOSEKMILPSolver"}
        for e, (name, _) in _LOCAL.items():
            if f"+{e}" in self.spec and f"{sources[e]}.cpp" not in rules:
                raise InstallError(f"MILPSolver did not find {name} in the "
                                   f"external {e}")

    def test_mcfblock_solver(self):
        """the min-cost flow tool solves the example installed with it"""
        if "+tools" not in self.spec or "+mcf" not in self.spec:
            raise SkipTest("needs +tools and +mcf")
        example = join_path(self.prefix.share, "SMS++_tools",
                            "mcfblock_solver", "examples", "example.dmx")
        mcf = which(join_path(self.prefix.bin, "smspp_mcfblock_solver"))
        out = mcf(example, output=str.split, error=str.split)
        assert "Upper bound = 1.40000000e+01" in out

    def test_ucblock_solver(self):
        """the Unit Commitment tool solves the example installed with it"""
        if "+tools" not in self.spec or "+ucblock" not in self.spec:
            raise SkipTest("needs +tools and +ucblock")
        example = join_path(self.prefix.share, "SMS++_tools",
                            "ucblock_solver", "examples", "Bus_Test.nc4")
        uc = which(join_path(self.prefix.bin, "smspp_ucblock_solver"))
        out = uc(example, output=str.split, error=str.split)
        assert "Upper bound = 4.14035578e+05" in out
