--
-- Copyright 2014 Dario Manesku. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

function addTrailingStr(_path, _str)
    if _path == nil then
        return ""
    end
    if string.sub(_path, -1) ~= _str then
        return _path .. _str
    else
        return _path
    end
end

local OSX_GCC_DIR   = addTrailingStr(os.getenv("CMFT_OSX_GCC_DIR_"),   "/")
local OSX_CLANG_DIR = addTrailingStr(os.getenv("CMFT_OSX_CLANG_DIR_"), "/")
local LIN_GCC_DIR   = addTrailingStr(os.getenv("CMFT_LIN_GCC_DIR_"),   "/")
local LIN_CLANG_DIR = addTrailingStr(os.getenv("CMFT_LIN_CLANG_DIR_"), "/")
local WIN_MINGW_DIR = addTrailingStr(os.getenv("CMFT_WIN_MINGW_DIR_"), "\\\\")
local WIN_CLANG_DIR = addTrailingStr(os.getenv("CMFT_WIN_CLANG_DIR_"), "\\\\")

function cmft_toolchain(_buildDir, _projectsDir)

    configuration {}

    newoption
    {
        trigger = "compiler",
        description = "Choose compiler",
        allowed =
        {
            { "osx-gcc",     "OSX"                      },
            { "linux-gcc",   "Linux (GCC compiler)"     },
--          { "linux-clang", "Linux (Clang compiler)"   },
--          { "win-clang",   "Windows (Clang compiler)" },
--          { "win-mingw",   "Windows (Mingw compiler)" },
        }
    }

    -- Avoid error when invoking premake4 --help.
    if (_ACTION == nil) then return end

    location (_projectsDir .. _ACTION)

    if _ACTION == "gmake" then

        if nil == _OPTIONS["compiler"] then
            print("Compiler must be specified!")
            os.exit(1)
        end

        if "osx-gcc" == _OPTIONS["compiler"] then
            premake.gcc.cc  = OSX_GCC_DIR .. "gcc"
            premake.gcc.cxx = OSX_GCC_DIR .. "g++"
            premake.gcc.ar  = OSX_GCC_DIR .. "ar"
            location (_projectsDir .. _ACTION .. "-osx-gcc")
        end

        if "linux-gcc" == _OPTIONS["compiler"] then
            premake.gcc.cc  = LIN_GCC_DIR .. "gcc"
            premake.gcc.cxx = LIN_GCC_DIR .. "g++"
            premake.gcc.ar  = LIN_GCC_DIR .. "ar"
            location (_projectsDir .. _ACTION .. "-linux-gcc")
        end

--      if "linux-clang" == _OPTIONS["compiler"] then
--          premake.gcc.cc  = LIN_CLANG_DIR .. "clang"
--          premake.gcc.cxx = LIN_CLANG_DIR .. "clang++"
--          premake.gcc.ar  = LIN_CLANG_DIR .. "ar"
--          location (_projectsDir .. _ACTION .. "-linux-clang")
--      end
--
--      if "win-mingw" == _OPTIONS["compiler"] then
--          premake.gcc.cc  = WIN_MINGW_DIR .. "x86_64-w64-mingw32-gcc"
--          premake.gcc.cxx = WIN_MINGW_DIR .. "x86_64-w64-mingw32-g++"
--          premake.gcc.ar  = WIN_MINGW_DIR .. "ar"
--          location (_projectsDir .. _ACTION .. "-win-mingw")
--      end
--
--      if "win-clang" == _OPTIONS["compiler"] then
--          premake.gcc.cc  = WIN_CLANG_DIR .. "clang"
--          premake.gcc.cxx = WIN_CLANG_DIR .. "clang++"
--          premake.gcc.ar  = WIN_CLANG_DIR .. "ar"
--          location (_projectsDir .. _ACTION .. "-win-clang")
--      end

    end

    flags
    {
        "StaticRuntime",
        "NoNativeWChar",
        "NoRTTI",
        "NoExceptions",
        "NoEditAndContinue",
        "ExtraWarnings",
    }

    defines
    {
        "__STDC_LIMIT_MACROS",
        "__STDC_FORMAT_MACROS",
        "__STDC_CONSTANT_MACROS",
    }

    configuration { "Debug" }
        targetsuffix "Debug"
        flags {"Symbols", }

    configuration { "Release" }
        targetsuffix "Release"
        flags { "OptimizeSpeed", "NoPCH" }

    configuration { "*gcc* or *mingw" }
        --TODO
        buildoptions
        {
           "-Wall"
        .. " -Wextra"
        .. " -Wno-cast-align"
        .. " -Wno-unused-function"
        .. " -Wno-variadic-macros"
        .. " -Wno-missing-format-attribute"
        .. " -Wcast-qual"
        .. " -Wdisabled-optimization"
        .. " -Wdiv-by-zero"
        .. " -Wendif-labels"
        .. " -Wformat-extra-args"
        .. " -Wformat-security"
        .. " -Wformat-y2k"
        .. " -Wimport"
        .. " -Winit-self"
        .. " -Winline"
        .. " -Winvalid-pch"
        .. " -Werror=missing-braces"
        .. " -Wmissing-include-dirs"
        .. " -Wmultichar"
        .. " -Wpacked"
        .. " -Wpointer-arith"
        .. " -Wreturn-type"
        .. " -Wsequence-point"
        .. " -Wsign-compare"
        .. " -Wstrict-aliasing"
        .. " -Wstrict-aliasing=2"
        .. " -Wswitch"
        .. " -Wwrite-strings"
        .. " -Werror=declaration-after-statement"
        .. " -Werror=implicit-function-declaration"
        .. " -Werror=nested-externs"
        .. " -Werror=old-style-definition"
        .. " -Werror=strict-prototypes"
        }

    -- VS
    configuration { "vs*" }
        flags { "EnableSSE2", }
        defines
        {
            "WIN32",
            "_WIN32",
            "_HAS_EXCEPTIONS=0",
            "_HAS_ITERATOR_DEBUGGING=0",
            "_SCL_SECURE=0",
            "_SECURE_SCL=0",
            "_SCL_SECURE_NO_WARNINGS",
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_SECURE_NO_DEPRECATE",
        }
        linkoptions
        {
            "/ignore:4221", -- LNK4221: This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
        }

    configuration { "vs*", "Debug" }
        buildoptions { "/Oy-" }

    configuration { "vs*", "Release" }
        buildoptions
        {
            "/Oy", -- Suppresses creation of frame pointers on the call stack.
            "/Ob2", -- The Inline Function Expansion.
            "/Ox",  -- Full optimization.
            "/Oi",  -- Enable intrinsics.
            "/Ot",  -- Favor fast code.
        }

    configuration { "x32", "vs*" }
        targetdir (_buildDir .. "win32_" .. _ACTION .. "/bin")
        objdir    (_buildDir .. "win32_" .. _ACTION .. "/obj")

    configuration { "x64", "vs*" }
        defines { "_WIN64" }
        targetdir (_buildDir .. "win64_" .. _ACTION .. "/bin")
        objdir    (_buildDir .. "win64_" .. _ACTION .. "/obj")

    -- MinGW
    configuration { "*mingw*" }
        defines { "WIN32" }
        buildoptions
        {
            "-std=c++11",
            "-U__STRICT_ANSI__",
            "-Wunused-value",
            "-fdata-sections",
            "-ffunction-sections",
            "-msse2",

            "-shared-libstdc++",
        }
        linkoptions { "-Wl,--gc-sections", }

    configuration { "x32", "*mingw" }
        targetdir (_buildDir .. "win32_mingw" .. "/bin")
        objdir    (_buildDir .. "win32_mingw" .. "/obj")
        buildoptions { "-m32" }

    configuration { "x64", "*mingw" }
        targetdir (_buildDir .. "win64_mingw" .. "/bin")
        objdir    (_buildDir .. "win64_mingw" .. "/obj")
        buildoptions { "-m64" }

    -- Linux
    configuration { "linux-*" }
        buildoptions
        {
            "-std=c++11",
            "-U__STRICT_ANSI__",
            "-Wunused-value",
            "-msse2",
        }
        links
        {
            "rt",
        }
        linkoptions
        {
            "-Wl,--gc-sections",
        }

    -- Linux GCC
    configuration { "linux-gcc and not linux-clang" }
        buildoptions
        {
            "-mfpmath=sse", -- force SSE to get 32-bit and 64-bit builds deterministic.
        }

    configuration { "linux-gcc", "x32" }
        targetdir (_buildDir .. "linux32_gcc" .. "/bin")
        objdir    (_buildDir .. "linux32_gcc" .. "/obj")
        buildoptions { "-m32", }

    configuration { "linux-gcc", "x64" }
        targetdir (_buildDir .. "linux64_gcc" .. "/bin")
        objdir    (_buildDir .. "linux64_gcc" .. "/obj")
        buildoptions { "-m64 -Ofast", }

    -- Linux Clang
    configuration { "linux-clang" }
        defines { "__STRICT_ANSI__" }
        buildoptions { "--analyze", }

    configuration { "linux-clang", "x32" }
        targetdir (_buildDir .. "linux32_clang" .. "/bin")
        objdir    (_buildDir .. "linux32_clang" .. "/obj")
        buildoptions
        {
            "-m32",
        }

    configuration { "linux-clang", "x64" }
        targetdir (_buildDir .. "linux64_clang" .. "/bin")
        objdir    (_buildDir .. "linux64_clang" .. "/obj")
        buildoptions
        {
            "-m64",
        }

    -- Windows Clang
    configuration { "win-clang", "x32" }
        targetdir (_buildDir .. "win32_clang" .. "/bin")
        objdir    (_buildDir .. "win32_clang" .. "/obj")
        buildoptions
        {
            "-m32",
            "-std=c++11",
        }

    configuration { "win-clang", "x64" }
        targetdir (_buildDir .. "win64_clang" .. "/bin")
        objdir    (_buildDir .. "win64_clang" .. "/obj")
        buildoptions
        {
            "-m64",
            "-std=c++11",
        }

    -- OSX
    configuration { "osx*" }
        buildoptions
        {
            "-U__STRICT_ANSI__",
            "-Wfatal-errors",
            "-Wunused-value",
            "-Wno-string-plus-int",
            "-msse2",
        }

    configuration { "osx*", "x32" }
        targetdir (_buildDir .. "osx32_gcc" .. "/bin")
        objdir    (_buildDir .. "osx32_gcc" .. "/obj")
        buildoptions
        {
            "-m32",
            "-std=c++11",
        }

    configuration { "osx*", "x64" }
        targetdir (_buildDir .. "osx64_gcc" .. "/bin")
        objdir    (_buildDir .. "osx64_gcc" .. "/obj")
        buildoptions
        {
            "-m64",
            "-std=c++11",
        }

    configuration { "xcode*", "x32" }
        targetdir (_buildDir .. "osx32_" .. _ACTION .. "/bin")
        objdir    (_buildDir .. "osx32_" .. _ACTION .. "/obj")
        buildoptions
        {
            "-m32",
            "-std=c++11",
        }

    configuration { "xcode*", "x64" }
        targetdir (_buildDir .. "osx64_" .. _ACTION .. "/bin")
        objdir    (_buildDir .. "osx64_" .. _ACTION .. "/obj")
        buildoptions
        {
            "-m64",
            "-std=c++11",
        }

    configuration {} -- reset configuration
end

function compat(_bxDir)
    -- VS
    configuration { "vs*" }
        includedirs { _bxDir .. "include/compat/msvc" }

    configuration { "vs2008" }
        includedirs { _bxDir .. "include/compat/msvc/pre1600" }

    -- MinGW
    configuration { "*mingw*" }
        includedirs { _bxDir .. "include/compat/mingw" }

    -- OSX
    configuration { "osx* or xcode*" }
        includedirs { _bxDir .. "include/compat/osx" }

    configuration {} -- reset configuration
end

function strip()
    configuration { "linux-*", "Release" }
        postbuildcommands
        {
            "@echo Stripping symbols.",
            "@strip -s \"$(TARGET)\""
        }

    configuration { "*mingw", "Release" }
        postbuildcommands
        {
            "@echo Stripping symbols.",
            "@$(MINGW)/bin/strip -s \"$(TARGET)\""
        }

    configuration {} -- reset configuration
end

-- vim: set sw=4 ts=4 expandtab:
