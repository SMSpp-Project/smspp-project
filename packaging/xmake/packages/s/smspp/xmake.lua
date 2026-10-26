-- SMS++, built from the tarball of a release of the umbrella, which carries
-- the core, every module and the tools. Each module is a config, named by
-- the acronym of packaging/modules.py, i.e., the vcpkg feature and the
-- Debian package of the module: add_requires("smspp", {configs = {sddp =
-- false}}) leaves SDDPBlock out, and a module brings in the modules it needs.
package("smspp")
    set_homepage("https://smspp.gitlab.io/")
    set_description("Framework to model and solve block-structured "
                    .. "optimization problems")
    set_license("LGPL-3.0")

    -- BEGIN of the table written by gen_recipes.py
    add_urls(
        "https://gitlab.com/api/v4/projects/smspp%2Fsmspp-project/packages/"
        .. "generic/smspp-project/$(version)/smspp-project-$(version).tar.gz"
    )
    add_versions("0.7.99",
        "a33859ae4ca6ca2ab9d6bfce67804e501051eae1780b4f148ef489f52978d91a")
    -- FastFlow, which the core fetches at configure time
    add_resources("*", "fastflow",
                  "https://github.com/fastflow/fastflow/archive/"
                  .. "d476f66ab924d8d122f54b4b90aee00ef979aea8.tar.gz",
        "fca23ce672fe12fe7cdbfa6955a982d6eb75816a00d2d97a93deba9b351f9c53")

    -- the modules: directory, acronym, library, the modules it needs,
    -- its required and optional externals
    local modules = {
        {dir = "SMS++", acr = "core",
         lib = "SMS++", needs = {},
         req = {"boost", "eigen", "netcdf-cxx4"},
         opt = {},
         desc = "Core library of the SMS++ modelling framework"},
        {dir = "BinaryKnapsackBlock", acr = "bkb",
         lib = "BinaryKnapsackBlock", needs = {"core"},
         req = {},
         opt = {"openmp"},
         desc = "Binary knapsack Block and its dynamic programming Solver"},
        {dir = "MCFBlock", acr = "mcf",
         lib = "MCFBlock", needs = {"core"},
         req = {},
         opt = {},
         desc = "Min-cost flow Block"},
        {dir = "CapacitatedFacilityLocationBlock", acr = "cflb",
         lib = "CapacitatedFacilityLocationBlock", needs = {"bkb", "mcf"},
         req = {},
         opt = {},
         desc = "Capacitated facility location Block"},
        {dir = "MMCFBlock", acr = "mmcf",
         lib = "MMCFBlock", needs = {"bkb", "mcf"},
         req = {},
         opt = {},
         desc = "Multicommodity min-cost flow Block"},
        {dir = "LukFiBlock", acr = "lukfi",
         lib = "LukFiBlock", needs = {"core"},
         req = {},
         opt = {},
         desc = "Test functions for nonsmooth optimization"},
        {dir = "UCBlock", acr = "ucblock",
         lib = "UCBlock", needs = {"core"},
         req = {},
         opt = {},
         desc = "Unit Commitment Block, its units and networks"},
        {dir = "StochasticBlock", acr = "stochastic",
         lib = "StochasticBlock", needs = {"core"},
         req = {},
         opt = {},
         desc = "Stochastic Block and scenario sets"},
        {dir = "TwoStageStochasticBlock", acr = "tssb",
         lib = "TwoStageStochasticBlock", needs = {"stochastic"},
         req = {},
         opt = {},
         desc = "Two-stage stochastic Block"},
        {dir = "MultiStageStochasticBlock", acr = "mssb",
         lib = "MultiStageStochasticBlock", needs = {"tssb"},
         req = {},
         opt = {},
         desc = "Multi-stage stochastic Block"},
        {dir = "SDDPBlock", acr = "sddp",
         lib = "SDDPBlock", needs = {"stochastic"},
         req = {"stopt"},
         opt = {"openmp"},
         desc = "Stochastic Dual Dynamic Programming Block"},
        {dir = "InvestmentBlock", acr = "investment",
         lib = "InvestmentBlock", needs = {"sddp", "tssb", "ucblock"},
         req = {},
         opt = {},
         desc = "Investment (capacity expansion) Block"},
        {dir = "SVMBlock", acr = "svm",
         lib = "SVMBlock", needs = {"core"},
         req = {},
         opt = {"libsvm", "liblinear"},
         desc = "Support Vector Machine Block"},
        {dir = "SingleFlowDCRBlock", acr = "sfdcr",
         lib = "SingleFlowDCRBlock", needs = {"core"},
         req = {},
         opt = {},
         desc = "Single-flow delay-constrained routing Block"},
        {dir = "SATBlock", acr = "sat",
         lib = "SATBlock", needs = {"core"},
         req = {},
         opt = {"cadical", "minisat"},
         desc = "SAT and weighted MaxSAT Block"},
        {dir = "SatellitesBlock", acr = "satellites",
         lib = "SatellitesBlock", needs = {"core"},
         req = {},
         opt = {},
         desc = "Satellite Constellation Design Problem Block"},
        {dir = "MultiKnapsackAssignBlock", acr = "mkab",
         lib = "MultiKnapsackAssignBlock", needs = {"bkb"},
         req = {},
         opt = {},
         desc = "Multiple knapsack assignment Block"},
        {dir = "HQPBlock", acr = "hqp",
         lib = "HQPBlock", needs = {"core"},
         req = {},
         opt = {},
         desc = "SMS++ Block and Solver for hierarchical (lexicographic) quadratic programs"},
        {dir = "MILPSolver", acr = "milp",
         lib = "MILPSolver", needs = {"core"},
         req = {},
         opt = {"highs", "scip", "daqp", "qpoases",
                "proxsuite", "piqp", "osqp", "qpmad"},
         desc = "MILP Solvers, on the open-source solvers of the channel"},
        {dir = "BundleSolver", acr = "bundle",
         lib = "BundleSolver", needs = {"milp"},
         req = {},
         opt = {},
         desc = "Bundle methods for nondifferentiable optimization"},
        {dir = "SubGradSolver", acr = "subgrad",
         lib = "SubGradSolver", needs = {"core"},
         req = {},
         opt = {},
         desc = "Subgradient methods for nondifferentiable optimization"},
        {dir = "LagrangianDualSolver", acr = "lds",
         lib = "LagrangianDualSolver", needs = {"milp", "bnx"},
         req = {},
         opt = {},
         desc = "Lagrangian dual Solver"},
        {dir = "FrankWolfeSolver", acr = "frankwolfe",
         lib = "FrankWolfeSolver", needs = {"core"},
         req = {},
         opt = {},
         desc = "Frank-Wolfe and Dantzig-Wolfe decomposition Solver"},
        {dir = "BranchAndXSolver", acr = "bnx",
         lib = "BranchAndXSolver", needs = {"core"},
         req = {},
         opt = {},
         desc = "Relaxation-agnostic Branch-and-X Solver"},
        {dir = "BendersDecompositionSolver", acr = "bds",
         lib = "BendersDecompositionSolver", needs = {"core"},
         req = {},
         opt = {},
         desc = "Generic Benders decomposition Solver"},
        {dir = "MCFClassSolver", acr = "mcfclass",
         lib = "MCFClassSolver", needs = {"mcf"},
         req = {},
         opt = {},
         desc = "Min-cost flow Solvers of MCFClass"},
        {dir = "MCFLemonSolver", acr = "mcflemon",
         lib = "MCFLemonSolver", needs = {"mcf"},
         req = {"lemon"},
         opt = {},
         desc = "Min-cost flow Solvers of LEMON"},
        {dir = "ScenarioReductionSolver", acr = "srs",
         lib = "ScenarioReductionSolver", needs = {"tssb"},
         req = {},
         opt = {},
         desc = "Scenario reduction Solvers"},
    }
    -- the solvers that are never packaged, the CMake names of the
    -- optional externals, the variables that tell where an
    -- external is and the sources not to compile in
    local never = {"CPLEX", "GUROBI", "MOSEK", "PIPS",
                   "Clarabel", "quadprog", "HPIPM", "Torch"}
    local cmake_name = {
        highs = "HiGHS",
        scip = "SCIP",
        daqp = "DAQP",
        qpoases = "qpOASES",
        proxsuite = "proxsuite",
        piqp = "piqp",
        osqp = "osqp",
        qpmad = "qpmad",
        cadical = "CaDiCaL",
        minisat = "MiniSat",
        libsvm = "LIBSVM",
        liblinear = "LIBLINEAR",
    }
    local root_var = {
        highs = "HiGHS_ROOT",
        scip = "SCIP_ROOT",
        daqp = "DAQP_ROOT",
        qpoases = "qpOASES_ROOT",
        proxsuite = "proxsuite_ROOT",
        piqp = "piqp_ROOT",
        osqp = "osqp_ROOT",
        qpmad = "qpmad_ROOT",
        cadical = "CADICAL_ROOT",
        minisat = "MINISAT_ROOT",
        libsvm = "LIBSVM_ROOT",
        liblinear = "LIBLINEAR_ROOT",
        stopt = "StOpt_ROOT",
        lemon = "LEMON_ROOT",
    }
    local no_sources = {"COMBO_ROOT", "RECORD_ROOT"}
    -- END of the table written by gen_recipes.py

    -- the package of xmake-repo of each external of packaging/modules.py,
    -- none where there is none that the modules can use: netcdf-cxx4 and
    -- stopt are in this repository, the osqp of xmake-repo is 0.6 while
    -- MILPSolver needs the API of 1.0; CPLEX, Gurobi and MOSEK are not
    -- redistributable
    local externals = {
        boost = "boost", eigen = "eigen", ["netcdf-cxx4"] = "netcdf-cxx4",
        stopt = "stopt", libsvm = "libsvm", highs = "highs",
        qpoases = "qpoases", lemon = "lemon"}
    -- the oldest version that the modules can use, where it is not the
    -- oldest of xmake-repo: HiGHSMILPSolver needs the callbacks of HiGHS
    local minimum = {highs = ">=1.10.0"}

    -- the classes of a module register themselves in the factories from
    -- static objects, which a static library loses unless whoever links it
    -- takes it whole: the libraries are shared
    add_configs("shared", {description = "Build shared libraries.",
                           default = true, type = "boolean", readonly = true})
    add_configs("tools", {description = "Build the command-line tools.",
                          default = true, type = "boolean"})
    for _, m in ipairs(modules) do
        if m.acr ~= "core" then
            local ok = true
            for _, e in ipairs(m.req) do
                ok = ok and externals[e] ~= nil
            end
            add_configs(m.acr, {description = m.dir .. ": " .. m.desc,
                                default = ok, type = "boolean"})
        end
    end

    add_deps("cmake")

    -- the modules chosen and those they need, the core included
    local function chosen(package)
        local byacr, out, todo = {}, {}, {"core"}
        for _, m in ipairs(modules) do
            byacr[m.acr] = m
            if m.acr ~= "core" and package:config(m.acr) then
                table.insert(todo, m.acr)
            end
        end
        while #todo > 0 do
            local a = table.remove(todo)
            if not out[a] then
                out[a] = true
                for _, n in ipairs(byacr[a].needs) do
                    table.insert(todo, n)
                end
            end
        end
        return out
    end

    on_load(function (package)
        local on = chosen(package)
        local exts = {}
        for _, m in ipairs(modules) do
            if on[m.acr] then
                for _, e in ipairs(table.join(m.req, m.opt)) do
                    if externals[e] then
                        exts[e] = true
                    end
                end
            end
        end
        for e, _ in pairs(exts) do
            if e == "boost" then
                -- the timer of Boost, which SDDPBlock asks for with StOpt;
                -- the Boost of xmake-repo, whose libraries are those asked
                package:add("deps", "boost", {system = false,
                                              configs = {timer = on.sddp}})
            elseif minimum[e] then
                package:add("deps", externals[e] .. " " .. minimum[e])
            else
                package:add("deps", externals[e])
            end
        end
        package:data_set("externals", exts)
        package:add("includedirs", "include", "include/SMS++")
        -- the libraries of the modules, the dependent ones first
        local links = {}
        for i = #modules, 1, -1 do
            local m = modules[i]
            if on[m.acr] then
                table.insert(links, m.lib)
                if m.acr == "mcfclass" then
                    table.insert(links, "MCFClass")
                end
            end
        end
        if package:is_plat("linux") then
            -- the ELF linker keeps a library that the program does not
            -- call only out of --as-needed
            package:add("linkgroups", links, {as_needed = false})
        else
            package:add("links", links)
        end
    end)

    on_install("linux", "macosx", function (package)
        -- the settings of the umbrella for its developers give way to these
        os.tryrm("CMakeSettings.txt")
        local ff = os.dirs(path.join(package:resourcedir("fastflow"), "*"))
        local configs = {
            "-DBUILD_SHARED_LIBS=ON", "-DBUILD_tests=OFF",
            "-DBUILD_TESTING=OFF", "-DBUILD_experiments=OFF",
            "-DUSE_MPI=OFF", "-DFETCHCONTENT_FULLY_DISCONNECTED=ON",
            "-DFETCHCONTENT_SOURCE_DIR_FASTFLOW=" .. ff[1]}
        table.insert(configs, "-DBUILD_tools="
                     .. (package:config("tools") and "ON" or "OFF"))
        local on = chosen(package)
        for _, m in ipairs(modules) do
            if m.acr ~= "core" then
                table.insert(configs, "-DBUILD_" .. m.dir .. "="
                             .. (on[m.acr] and "ON" or "OFF"))
            end
        end
        -- the externals where xmake has put them, not under the defaults of
        -- the umbrella, several of which are in /opt
        local exts = package:data("externals")
        for _, p in ipairs(never) do
            table.insert(configs,
                         "-DCMAKE_DISABLE_FIND_PACKAGE_" .. p .. "=ON")
        end
        for e, n in pairs(cmake_name) do
            if not exts[e] then
                table.insert(configs,
                             "-DCMAKE_DISABLE_FIND_PACKAGE_" .. n .. "=ON")
            end
        end
        for e, v in pairs(root_var) do
            local dep = exts[e] and package:dep(externals[e])
            if dep and not dep:is_system() then
                table.insert(configs, "-D" .. v .. "=" .. dep:installdir())
            elseif dep then
                table.insert(configs, "-D" .. v .. "=/usr")
            end
        end
        for _, v in ipairs(no_sources) do
            table.insert(configs, "-D" .. v .. "=OFF")
        end
        table.insert(configs, "-DSMSPP_EXTLIB_DEFAULTS=OFF")
        -- the HDF5 under netCDF is static unless xmake is told otherwise,
        -- while FindHDF5 looks for a shared one, and fails, unless told;
        -- its targets then link ZLIB::ZLIB, which its configuration does
        -- not define, and which each project of the umbrella defines first
        -- from the zlib of xmake
        for _, dep in ipairs(package:librarydeps()) do
            if dep:name() == "hdf5" and not dep:config("shared") then
                table.insert(configs, "-DHDF5_USE_STATIC_LIBRARIES=ON")
                local zlib = path.join(package:cachedir(), "zlib.cmake")
                io.writefile(zlib, "find_package(ZLIB REQUIRED)\n")
                table.insert(configs,
                             "-DCMAKE_PROJECT_INCLUDE_BEFORE=" .. zlib)
            elseif dep:name() == "zlib" and not dep:is_system() then
                table.insert(configs, "-DZLIB_ROOT=" .. dep:installdir())
            end
        end
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <Block.h>
            void test() {
                auto b = SMSpp_di_unipi_it::Block::new_Block("MCFBlock");
                delete b;
            }
        ]]}, {configs = {languages = "c++20"}}))
    end)
