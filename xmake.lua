add_rules("mode.debug", "mode.release")
add_requires("vcpkg::p-ranav-csv2", "vcpkg::nlohmann-json", "vcpkg::fmt", "vcpkg::abseil")
add_requires("toml++ 3.4.0")
add_requires("vcpkg::ftxui")
set_languages("c++23")
set_toolchains("llvm")
set_optimize("fastest")

target("epi_sim")
    set_kind("binary")
    set_default(false)
    add_files("src/*.cpp")
    add_packages("vcpkg::p-ranav-csv2", "vcpkg::nlohmann-json", "vcpkg::fmt", "vcpkg::abseil", "toml++")
    add_packages("vcpkg::ftxui")

target("test")
    set_kind("binary")
    set_default(false)
    add_files("test/test_main.cpp", "test/test_pop_serialize.cpp", "test/test_parameters.cpp",
        "test/test_disease_modeling.cpp", "test/test_vaccination.cpp", "test/test_traits.cpp",
        "test/test_series.cpp", "test/test_setup.cpp", "test/test_plot.cpp", "test/test_runsim.cpp",
        "test/test_templates.cpp")
    add_files("src/*.cpp|epi_sim.cpp")
    add_packages("vcpkg::p-ranav-csv2", "vcpkg::nlohmann-json", "vcpkg::fmt", "vcpkg::abseil", "toml++")
    add_packages("vcpkg::ftxui")


target("this")
    set_kind("binary")
    set_default(false)
    add_packages("vcpkg::ftxui")
    add_packages("vcpkg::fmt")
    add_files("scratch/tui/tui_example.cpp", "scratch/tui/stubs.cpp")

-- target("randstuff")
--     set_kind("binary")
--     set_default(false)
--     add_files("scratch/benchmark_rand.cpp")
--     add_files("src/helpers.cpp")
--     add_packages("vcpkg::abseil", "vcpkg::fmt")

-- target("dates")
--     set_kind("binary")
--     set_default(false)
--     add_files("scratch/date-tests.cpp")
--     add_packages("vcpkg::abseil")

-- target("ring_experiment")
--     set_kind("binary")
--     set_default(false)
--     add_files("scratch/ring_experiment.cpp", "src/*.cpp|epi_sim.cpp")
--     add_packages("vcpkg::p-ranav-csv2", "vcpkg::nlohmann-json", "vcpkg::fmt", "vcpkg::abseil", "toml++")



--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--
