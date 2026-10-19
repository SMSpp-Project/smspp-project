# SMS++, built from the tarball of a release of the umbrella, which carries
# the core, every module and the tools. `modules` chooses the modules, by the
# acronyms of packaging/modules.py, which are those of the vcpkg features and
# of the Debian packages: smspp.override { modules = [ "ucblock" "milp" ]; }
# builds those and the modules they need; null builds every module whose
# dependencies nixpkgs has.
{
  lib,
  stdenv,
  fetchurl,
  cmake,
  help2man,
  boost,
  eigen,
  netcdfcxx4,
  netcdf,
  hdf5,
  highs,
  scipopt-scip,
  osqp,
  proxsuite,
  qpoases,
  cadical,
  minisat,
  lemon-graph,
  libsvm,
  liblinear,
  stopt,
  modules ? null,
  withTools ? true,
}:

let
  # BEGIN of the table written by gen_recipes.py
  version = "0.7.99";
  hash = "sha256-ozhZrkymyiq51r/OZ4BOUBBR6uF4C08UjvSJ9Sl42Ro=";
  registry =
    "https://gitlab.com/api/v4/projects/smspp%2Fsmspp-project/"
    + "packages/generic/smspp-project";
  fastflowCommit = "d476f66ab924d8d122f54b4b90aee00ef979aea8";
  fastflowHash = "sha256-/KI85nL+Ev582/ppVamC1ut1gWoA0tl6k966mzUfnFM=";
  # the modules: directory, acronym, the modules it needs, its
  # required and optional externals
  table = [
    {
      dir = "SMS++";
      acr = "core";
      needs = [ ];
      req = [ "boost" "eigen" "netcdf-cxx4" ];
      opt = [ ];
    }
    {
      dir = "BinaryKnapsackBlock";
      acr = "bkb";
      needs = [ "core" ];
      req = [ ];
      opt = [ "openmp" ];
    }
    {
      dir = "MCFBlock";
      acr = "mcf";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "CapacitatedFacilityLocationBlock";
      acr = "cflb";
      needs = [ "bkb" "mcf" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "MMCFBlock";
      acr = "mmcf";
      needs = [ "bkb" "mcf" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "LukFiBlock";
      acr = "lukfi";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "UCBlock";
      acr = "ucblock";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "StochasticBlock";
      acr = "stochastic";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "TwoStageStochasticBlock";
      acr = "tssb";
      needs = [ "stochastic" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "MultiStageStochasticBlock";
      acr = "mssb";
      needs = [ "tssb" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "SDDPBlock";
      acr = "sddp";
      needs = [ "stochastic" ];
      req = [ "stopt" ];
      opt = [ "openmp" ];
    }
    {
      dir = "InvestmentBlock";
      acr = "investment";
      needs = [ "sddp" "tssb" "ucblock" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "SVMBlock";
      acr = "svm";
      needs = [ "core" ];
      req = [ ];
      opt = [ "libsvm" "liblinear" ];
    }
    {
      dir = "SingleFlowDCRBlock";
      acr = "sfdcr";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "SATBlock";
      acr = "sat";
      needs = [ "core" ];
      req = [ ];
      opt = [ "cadical" "minisat" ];
    }
    {
      dir = "SatellitesBlock";
      acr = "satellites";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "MultiKnapsackAssignBlock";
      acr = "mkab";
      needs = [ "bkb" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "MILPSolver";
      acr = "milp";
      needs = [ "core" ];
      req = [ ];
      opt = [
        "highs"
        "scip"
        "daqp"
        "qpoases"
        "proxsuite"
        "piqp"
        "osqp"
        "qpmad"
      ];
    }
    {
      dir = "BundleSolver";
      acr = "bundle";
      needs = [ "milp" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "SubGradSolver";
      acr = "subgrad";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "LagrangianDualSolver";
      acr = "lds";
      needs = [ "milp" "bnx" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "FrankWolfeSolver";
      acr = "frankwolfe";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "BranchAndXSolver";
      acr = "bnx";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "BendersDecompositionSolver";
      acr = "bds";
      needs = [ "core" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "MCFClassSolver";
      acr = "mcfclass";
      needs = [ "mcf" ];
      req = [ ];
      opt = [ ];
    }
    {
      dir = "MCFLemonSolver";
      acr = "mcflemon";
      needs = [ "mcf" ];
      req = [ "lemon" ];
      opt = [ ];
    }
    {
      dir = "ScenarioReductionSolver";
      acr = "srs";
      needs = [ "tssb" ];
      req = [ ];
      opt = [ ];
    }
  ];
  # the solvers that are never packaged
  never = [ "CPLEX" "GUROBI" "MOSEK" "PIPS" "Clarabel" "quadprog" "Torch" ];
  # the CMake names of the optional externals
  cmakeName = {
    highs = "HiGHS";
    scip = "SCIP";
    daqp = "DAQP";
    qpoases = "qpOASES";
    proxsuite = "proxsuite";
    piqp = "piqp";
    osqp = "osqp";
    qpmad = "qpmad";
    cadical = "CaDiCaL";
    minisat = "MiniSat";
    libsvm = "LIBSVM";
    liblinear = "LIBLINEAR";
  };
  # the variables that tell the CMake of the modules where an
  # external is, and the sources it is not to compile in
  rootVars = [
    "HiGHS_ROOT"
    "SCIP_ROOT"
    "DAQP_ROOT"
    "qpOASES_ROOT"
    "proxsuite_ROOT"
    "piqp_ROOT"
    "osqp_ROOT"
    "qpmad_ROOT"
    "CADICAL_ROOT"
    "MINISAT_ROOT"
    "LIBSVM_ROOT"
    "LIBLINEAR_ROOT"
    "StOpt_ROOT"
    "LEMON_ROOT"
  ];
  noSources = [ "COMBO_ROOT" "RECORD_ROOT" ];
  # END of the table written by gen_recipes.py

  # the packages of each external of packaging/modules.py, none where
  # nixpkgs has none; CPLEX, Gurobi and MOSEK are not redistributable
  externals = {
    boost = [ boost ];
    eigen = [ eigen ];
    # the core finds the C library too, and the HDF5 it is built on
    "netcdf-cxx4" = [
      netcdfcxx4
      netcdf
      hdf5
    ];
    openmp = [ ]; # the libgomp of the compiler, not asked for
    stopt = [ stopt ];
    libsvm = [ libsvm ];
    liblinear = [ liblinear ];
    cadical = [ cadical ];
    minisat = [ minisat ];
    highs = [ highs ];
    scip = [ scipopt-scip ];
    osqp = [ osqp ];
    proxsuite = [ proxsuite ];
    qpoases = [ qpoases ];
    daqp = [ ];
    piqp = [ ];
    qpmad = [ ];
    lemon = [ lemon-graph ];
  };
  has = e: externals.${e} != [ ];

  byAcr = lib.listToAttrs (map (m: lib.nameValuePair m.acr m) table);
  closure =
    acrs:
    let
      step = s: lib.unique (s ++ lib.concatMap (a: byAcr.${a}.needs) s);
      fix = s: let t = step s; in if t == s then s else fix t;
    in
    fix acrs;
  # the modules whose dependencies nixpkgs has, and those they need
  prune =
    s:
    let
      t = lib.filter (a: lib.all (n: lib.elem n s) byAcr.${a}.needs) s;
    in
    if t == s then s else prune t;
  available = prune (map (m: m.acr) (lib.filter (m: lib.all has m.req) table));
  wanted =
    if modules == null then available else closure ([ "core" ] ++ modules);
  missing = lib.filter (a: !(lib.elem a available)) wanted;
  chosen = lib.filter (m: lib.elem m.acr wanted) table;
  used = lib.concatMap (m: m.req ++ m.opt) chosen;
  deps = lib.unique (lib.concatMap (e: externals.${e}) used);
in
assert lib.assertMsg (missing == [ ])
  "smspp: no dependencies in nixpkgs for ${lib.concatStringsSep ", " missing}";
stdenv.mkDerivation {
  pname = "smspp";
  inherit version;

  # the tarball of the release has no top directory, and FastFlow, which the
  # core fetches at configure time, is given as a source of its own
  src = fetchurl {
    url = "${registry}/${version}/smspp-project-${version}.tar.gz";
    inherit hash;
  };
  fastflow = fetchurl {
    url =
      "https://github.com/fastflow/fastflow/archive/"
      + "${fastflowCommit}.tar.gz";
    hash = fastflowHash;
  };
  unpackPhase = ''
    runHook preUnpack
    mkdir source fastflow
    tar -xzf $src -C source
    tar -xzf $fastflow -C fastflow --strip-components=1
    rm -f source/CMakeSettings.txt
    cd source
    runHook postUnpack
  '';

  nativeBuildInputs = [
    cmake
    help2man
  ];
  propagatedBuildInputs = deps;

  # the modules not chosen, and the solvers that are not redistributable or
  # that nixpkgs does not have, are left out
  cmakeFlags = [
    (lib.cmakeBool "BUILD_SHARED_LIBS" true)
    (lib.cmakeBool "BUILD_tests" false)
    (lib.cmakeBool "BUILD_TESTING" false)
    (lib.cmakeBool "BUILD_experiments" false)
    (lib.cmakeBool "BUILD_tools" withTools)
    (lib.cmakeBool "USE_MPI" false)
    (lib.cmakeBool "FETCHCONTENT_FULLY_DISCONNECTED" true)
  ]
  ++ map (m: lib.cmakeBool "BUILD_${m.dir}" (lib.elem m.acr wanted)) (
    lib.filter (m: m.acr != "core") table
  )
  # the defaults of the umbrella for where the externals are, several in
  # /opt, give way to the search paths of the build, and the sources it
  # would compile in from there are not looked for
  ++ map (v: lib.cmakeFeature v "") rootVars
  ++ map (v: lib.cmakeFeature v "OFF") noSources
  ++ map (p: lib.cmakeBool "CMAKE_DISABLE_FIND_PACKAGE_${p}" true) (
    never
    ++ map (e: cmakeName.${e}) (
      lib.filter (e: !has e) (lib.attrNames cmakeName)
    )
  );

  # FastFlow is next to the sources, out of which the build runs
  preConfigure = ''
    cmakeFlagsArray+=("-DFETCHCONTENT_SOURCE_DIR_FASTFLOW=$(pwd)/../fastflow")
  '';

  doInstallCheck = withTools && lib.elem "mcf" wanted;
  installCheckPhase = ''
    runHook preInstallCheck
    $out/bin/smspp_block_solver --help > /dev/null
    $out/bin/smspp_mcfblock_solver \
      $out/share/SMS++_tools/mcfblock_solver/examples/example.dmx \
      | grep "Upper bound = 1.40000000e+01"
    runHook postInstallCheck
  '';

  passthru = {
    inherit table externals;
    modules = wanted;
  };

  meta = {
    description =
      "Framework to model and solve block-structured optimization problems";
    homepage = "https://smspp.gitlab.io/";
    license = lib.licenses.lgpl3Only;
    platforms = lib.platforms.unix;
    mainProgram = "smspp_block_solver";
  };
}
