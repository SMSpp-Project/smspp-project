"""The modules of SMS++ and their tools, as the packages see them.

This is the one table out of which gen_recipes.py writes the recipes of the
channels in this directory (Conan, Spack, Nix, AUR, MSYS2, xmake): a new
module of the umbrella is one row of MODULES, a new tool one row of TOOLS,
and then `./gen_recipes.py` rewrites every recipe. The acronym of a module
is the one of the other channels too: the vcpkg feature smspp[<acr>], the
Debian package libsmspp-<acr>, the conda package libsmspp-<acr>.

An external dependency is named here by an abstract name, which every recipe
maps to the package of its own channel; a module whose `required` externals
a channel does not have is left out of that channel, while an `optional`
one is used where the channel has it, the CMake of the module finding it or
doing without.
"""

# (directory, acronym, modules it needs, required externals, optional
#  externals, description); the core SMS++ is the acronym "core"
MODULES = [
    ("SMS++", "core", [], ["boost", "eigen", "netcdf-cxx4"], [],
     "Core library of the SMS++ modelling framework"),
    ("BinaryKnapsackBlock", "bkb", ["core"], [], ["openmp"],
     "Binary knapsack Block and its dynamic programming Solver"),
    ("MCFBlock", "mcf", ["core"], [], [],
     "Min-cost flow Block"),
    ("CapacitatedFacilityLocationBlock", "cflb", ["bkb", "mcf"], [], [],
     "Capacitated facility location Block"),
    ("MMCFBlock", "mmcf", ["bkb", "mcf"], [], [],
     "Multicommodity min-cost flow Block"),
    ("LukFiBlock", "lukfi", ["core"], [], [],
     "Test functions for nonsmooth optimization"),
    ("UCBlock", "ucblock", ["core"], [], [],
     "Unit Commitment Block, its units and networks"),
    ("StochasticBlock", "stochastic", ["core"], [], [],
     "Stochastic Block and scenario sets"),
    ("TwoStageStochasticBlock", "tssb", ["stochastic"], [], [],
     "Two-stage stochastic Block"),
    ("MultiStageStochasticBlock", "mssb", ["tssb"], [], [],
     "Multi-stage stochastic Block"),
    ("SDDPBlock", "sddp", ["stochastic"], ["stopt"], ["openmp"],
     "Stochastic Dual Dynamic Programming Block"),
    ("InvestmentBlock", "investment", ["sddp", "tssb", "ucblock"], [], [],
     "Investment (capacity expansion) Block"),
    ("SVMBlock", "svm", ["core"], [], ["libsvm", "liblinear"],
     "Support Vector Machine Block"),
    ("SingleFlowDCRBlock", "sfdcr", ["core"], [], [],
     "Single-flow delay-constrained routing Block"),
    ("SATBlock", "sat", ["core"], [], ["cadical", "minisat"],
     "SAT and weighted MaxSAT Block"),
    ("SatellitesBlock", "satellites", ["core"], [], [],
     "Satellite Constellation Design Problem Block"),
    ("MultiKnapsackAssignBlock", "mkab", ["bkb"], [], [],
     "Multiple knapsack assignment Block"),
    ("MILPSolver", "milp", ["core"], [],
     ["highs", "scip", "daqp", "qpoases", "proxsuite", "piqp", "osqp",
      "qpmad"],
     "MILP Solvers, on the open-source solvers of the channel"),
    ("BundleSolver", "bundle", ["milp"], [], [],
     "Bundle methods for nondifferentiable optimization"),
    ("SubGradSolver", "subgrad", ["core"], [], [],
     "Subgradient methods for nondifferentiable optimization"),
    ("LagrangianDualSolver", "lds", ["milp", "bnx"], [], [],
     "Lagrangian dual Solver"),
    ("FrankWolfeSolver", "frankwolfe", ["core"], [], [],
     "Frank-Wolfe and Dantzig-Wolfe decomposition Solver"),
    ("BranchAndXSolver", "bnx", ["core"], [], [],
     "Relaxation-agnostic Branch-and-X Solver"),
    ("BendersDecompositionSolver", "bds", ["core"], [], [],
     "Generic Benders decomposition Solver"),
    ("MCFClassSolver", "mcfclass", ["mcf"], [], [],
     "Min-cost flow Solvers of MCFClass"),
    ("MCFLemonSolver", "mcflemon", ["mcf"], ["lemon"], [],
     "Min-cost flow Solvers of LEMON"),
    ("ScenarioReductionSolver", "srs", ["tssb"], [], [],
     "Scenario reduction Solvers"),
]

# (acronym of the command package, tool directories under tools/, modules it
#  needs, description); the package is smspp-<acronym>, the commands are
#  smspp_<tool directory>
TOOLS = [
    ("ucblock", ["ucblock_solver"], ["ucblock", "milp"],
     "Unit Commitment solver"),
    ("tssb", ["tssb_solver"], ["ucblock", "tssb", "milp"],
     "two-stage stochastic solver"),
    ("mssb", ["mssb_solver"], ["ucblock", "mssb", "milp"],
     "multi-stage stochastic solver"),
    ("sddp", ["sddp_solver"], ["ucblock", "sddp", "milp"],
     "SDDP solver"),
    ("investment", ["investmentblock_solver"], ["investment", "mssb", "milp"],
     "capacity expansion solver"),
    ("svm", ["svm_solver"], ["svm", "milp"],
     "support vector machine trainer"),
    ("mcf", ["mcfblock_solver"], ["mcf", "mcfclass"],
     "min-cost flow solver"),
    ("bkb", ["bkblock_solver"], ["bkb"],
     "binary knapsack solver"),
    ("cflb", ["cflblock_solver"], ["cflb", "milp"],
     "capacitated facility location solver"),
    ("mmcf", ["mmcfblock_solver"], ["mmcf", "milp"],
     "multicommodity min-cost flow solver"),
    ("sfdcr", ["sfdcrblock_solver"], ["sfdcr", "milp"],
     "delay-constrained routing solver"),
    ("tools", ["block_solver", "chgcfg"], ["core"],
     "generic solver and configuration editor"),
]

# the name by which the CMake of the modules finds an optional external, so
# that a recipe can turn off the search of one its channel does not carry
# (or carries in a form that cannot be used)
CMAKE_NAME = {
    "highs": "HiGHS", "scip": "SCIP", "daqp": "DAQP", "qpoases": "qpOASES",
    "proxsuite": "proxsuite", "piqp": "piqp", "osqp": "osqp",
    "qpmad": "qpmad", "cadical": "CaDiCaL", "minisat": "MiniSat",
    "libsvm": "LIBSVM", "liblinear": "LIBLINEAR",
}

# the variable that tells the CMake of the modules where an external is: the
# umbrella reads defaults for them from extlib/, several under /opt, which a
# recipe overrides with the prefix of its channel, so that nothing outside
# the channel can be picked up on the machine that builds the package
ROOT_VAR = {
    "highs": "HiGHS_ROOT", "scip": "SCIP_ROOT", "daqp": "DAQP_ROOT",
    "qpoases": "qpOASES_ROOT", "proxsuite": "proxsuite_ROOT",
    "piqp": "piqp_ROOT", "osqp": "osqp_ROOT", "qpmad": "qpmad_ROOT",
    "cadical": "CADICAL_ROOT", "minisat": "MINISAT_ROOT",
    "libsvm": "LIBSVM_ROOT", "liblinear": "LIBLINEAR_ROOT",
    "stopt": "StOpt_ROOT", "lemon": "LEMON_ROOT",
}

# the solvers that are never packaged: CPLEX, Gurobi and MOSEK are not
# redistributable, PIPS-IPM++ and the C interface of Clarabel are in no
# package, quadprog is under the GPL and Torch is not wanted in a package
NEVER = ["CPLEX", "GUROBI", "MOSEK", "PIPS", "Clarabel", "quadprog", "Torch"]

# the sources that BinaryKnapsackBlock compiles in when it finds them, under
# /opt by default: COMBO is for non-commercial use only, and neither is in a
# package, so every recipe turns their search off
NO_SOURCES = ["COMBO_ROOT", "RECORD_ROOT"]

# the executables that the install of a module carries besides its library,
# which go to the command package of the module where a channel splits
HELPERS = [
    ("bkb", "bkbench", "bkb"),
    ("cflb", "cfl2nc4", "cflb"),
    ("mcf", "dmx2nc4", "mcf"),
    ("ucblock", "nc4generator", "ucblock"),
]

# FastFlow, which the core fetches at configure time and every recipe gives
# it instead, at the commit the vcpkg port, the PPA and the formula pin
FASTFLOW = {
    "commit": "d476f66ab924d8d122f54b4b90aee00ef979aea8",
    "url": "https://github.com/fastflow/fastflow/archive/"
           "d476f66ab924d8d122f54b4b90aee00ef979aea8.tar.gz",
    "sha256": "fca23ce672fe12fe7cdbfa6955a982d6"
              "eb75816a00d2d97a93deba9b351f9c53",
}

# StOpt, which SDDPBlock needs and which most channels do not carry, so that
# the recipes of this directory carry it as the Homebrew tap does
STOPT = {
    "version": "7.1",
    "url": "https://gitlab.com/stochastic-control/StOpt/-/archive/v7.1/"
           "StOpt-v7.1.tar.gz",
    "sha256": "8bee8f2aab097c91a791729cde34f50b"
              "062208a1ab1af4d62ee41dba24a9e5e7",
}

# netCDF-C++, which the core needs and which Conan Center and xmake-repo do
# not carry
NETCDF_CXX4 = {
    "version": "4.3.1",
    "url": "https://github.com/Unidata/netcdf-cxx4/archive/refs/tags/"
           "v4.3.1.tar.gz",
    "sha256": "e3fe3d2ec06c1c2772555bf1208d220a"
              "ab5fee186d04bd265219b0bc7a978edc",
}

HOMEPAGE = "https://smspp.gitlab.io/"
LICENSE = "LGPL-3.0-only"
SUMMARY = "Framework to model and solve block-structured optimization problems"
MAINTAINER = "Donato Meoli <donato.meoli.95@gmail.com>"


def closure(acrs):
    """The modules in acrs and all those they need, in table order."""
    needs = {m[1]: m[2] for m in MODULES}
    out, todo = set(), list(acrs)
    while todo:
        a = todo.pop()
        if a not in out:
            out.add(a)
            todo += needs[a]
    return [m[1] for m in MODULES if m[1] in out]
