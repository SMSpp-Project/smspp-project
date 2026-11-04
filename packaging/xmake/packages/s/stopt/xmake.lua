-- StOpt, the library of stochastic optimization that SDDPBlock of SMS++
-- needs, which xmake-repo does not carry; the geners serialization library
-- travels inside it and is built with it.
package("stopt")
    set_homepage("https://gitlab.com/stochastic-control/StOpt")
    set_description("Library for stochastic optimization by dynamic "
                    .. "programming")
    set_license("LGPL-3.0")

    add_urls("https://gitlab.com/stochastic-control/StOpt/-/archive/"
             .. "v$(version)/StOpt-v$(version).tar.gz")
    -- BEGIN of the table written by gen_recipes.py
    add_versions("7.1",
        "8bee8f2aab097c91a791729cde34f50b062208a1ab1af4d62ee41dba24a9e5e7")
    -- END of the table written by gen_recipes.py

    add_deps("cmake")
    add_deps("boost", {configs = {random = true, serialization = true,
                                  system = true, timer = true}})
    add_deps("eigen", "zlib", "bzip2")
    add_links("StOpt", "geners")

    on_install("linux", "macosx", function (package)
        -- the SDDP and DP CUTS switches build tests, not the library
        local configs = {
            "-DBUILD_PYTHON=OFF", "-DBUILD_TEST=OFF", "-DBUILD_SDDP=OFF",
            "-DBUILD_DPCUTS=OFF", "-DBUILD_MPI=OFF",
            "-DBUILD_SYSTEM_INSTALL=ON"}
        table.insert(configs, "-DBUILD_SHARED_LIBS="
                     .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <StOpt/sddp/SDDPACut.h>
            void test() { StOpt::SDDPACut cut; }
        ]]}, {configs = {languages = "c++17"}}))
    end)
