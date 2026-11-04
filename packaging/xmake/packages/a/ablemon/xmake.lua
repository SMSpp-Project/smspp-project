-- abLEMON, the fork of LEMON that MCFLemonSolver of SMS++ needs, which
-- xmake-repo does not carry; its LP and MIP backends are left out.
package("ablemon")
    set_homepage("https://github.com/dmeoli/abLEMON")
    set_description("Maintained fork of LEMON, a C++ library of graphs")
    set_license("BSL-1.0")

    add_urls("https://github.com/dmeoli/abLEMON/archive/refs/tags/"
             .. "$(version).tar.gz")
    -- BEGIN of the table written by gen_recipes.py
    add_versions("1.7.0",
        "6ca9da3d115557c2dc914f58f3dbcdb2186808ed98c92a306160b1dc7b4d50ee")
    -- END of the table written by gen_recipes.py

    add_deps("cmake")
    add_links("emon")

    on_install("linux", "macosx", function (package)
        local configs = {"-DLEMON_VERSION=" .. package:version_str()}
        for _, o in ipairs({"TESTS", "DEMOS", "TOOLS", "DOC"}) do
            table.insert(configs, "-DLEMON_BUILD_" .. o .. "=OFF")
        end
        for _, o in ipairs({"GLPK", "ILOG", "COIN", "SOPLEX"}) do
            table.insert(configs, "-DLEMON_ENABLE_" .. o .. "=OFF")
        end
        table.insert(configs, "-DBUILD_SHARED_LIBS="
                     .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <lemon/k_best_extreme_flows.h>
            #include <lemon/smart_graph.h>
            void test() {
                lemon::SmartDigraph g;
                lemon::KBestExtremeFlows<lemon::SmartDigraph> k(g);
            }
        ]]}, {configs = {languages = "c++11"}}))
    end)
