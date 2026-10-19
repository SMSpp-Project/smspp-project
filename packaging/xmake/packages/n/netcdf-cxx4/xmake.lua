-- netCDF-C++, the C++ interface of netCDF that the core of SMS++ needs,
-- which xmake-repo does not carry: a handful of sources over netCDF-C, built
-- here by xmake, since its own CMake looks for netCDF-C and HDF5 by paths
-- that the packages of xmake do not set.
package("netcdf-cxx4")
    set_homepage("https://github.com/Unidata/netcdf-cxx4")
    set_description("C++ interface of the netCDF library")
    set_license("BSD-3-Clause")

    add_urls("https://github.com/Unidata/netcdf-cxx4/archive/refs/tags/"
             .. "v$(version).tar.gz")
    -- BEGIN of the table written by gen_recipes.py
    add_versions("4.3.1",
        "e3fe3d2ec06c1c2772555bf1208d220aab5fee186d04bd265219b0bc7a978edc")
    -- END of the table written by gen_recipes.py

    add_deps("netcdf-c")
    add_links("netcdf-cxx4")

    on_install("linux", "macosx", "windows", "mingw", function (package)
        io.writefile("xmake.lua", [[
            add_rules("mode.release", "mode.debug")
            add_requires("netcdf-c")
            target("netcdf-cxx4")
                set_kind("$(kind)")
                add_files("cxx4/nc*.cpp")
                add_headerfiles("cxx4/nc*.h", "cxx4/netcdf")
                add_includedirs("cxx4")
                add_packages("netcdf-c")
                if is_plat("windows") and is_kind("shared") then
                    add_rules("utils.symbols.export_all",
                              {export_classes = true})
                end
        ]])
        import("package.tools.xmake").install(package)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <netcdf>
            void test() { netCDF::NcFile f; }
        ]]}, {configs = {languages = "c++11"}}))
    end)
