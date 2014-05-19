CMFT_ROOT_DIR        = (path.getabsolute("..") .. "/")
CMFT_3RDPARTY_DIR    = (CMFT_ROOT_DIR .. "3rdparty/")
CMFT_PREMAKE_DIR     = (CMFT_ROOT_DIR .. "premake/")
CMFT_SRC_DIR         = (CMFT_ROOT_DIR .. "src/")
CMFT_RUNTIME_DIR     = (CMFT_ROOT_DIR .. "runtime/")
CMFT_TEXTURES_DIR    = (CMFT_RUNTIME_DIR .. "textures/")

CMFT_BUILD_DIR       = (CMFT_ROOT_DIR .. "_build/")
CMFT_PROJECTS_DIR    = (CMFT_ROOT_DIR .. "_projects/")
BX_DIR               = (CMFT_ROOT_DIR .. "3rdparty/bx/")

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

OSX_GCC_DIR   = addTrailingStr(os.getenv("CMFT_OSX_GCC_DIR_"), "/")
OSX_CLANG_DIR = addTrailingStr(os.getenv("CMFT_OSX_CLANG_DIR_"), "/")
LIN_GCC_DIR   = addTrailingStr(os.getenv("CMFT_LIN_GCC_DIR_"), "/")
LIN_CLANG_DIR = addTrailingStr(os.getenv("CMFT_LIN_CLANG_DIR_"), "/")
WIN_MINGW_DIR = addTrailingStr(os.getenv("CMFT_WIN_MINGW_DIR_"), "\\\\")
WIN_CLANG_DIR = addTrailingStr(os.getenv("CMFT_WIN_CLANG_DIR_"), "\\\\")

function toolchain(_buildDir, _libDir)

	newoption {
		trigger = "compiler",
		description = "Choose compiler",
		allowed = {
			{ "osx-gcc",     "OSX"                      },
			{ "linux-gcc",   "Linux (GCC compiler)"     },
--			{ "linux-clang", "Linux (Clang compiler)"   },
--			{ "win-clang",   "Windows (Clang compiler)" },
--			{ "win-mingw",   "Windows (Mingw compiler)" },
		}
	}

	-- Avoid error when invoking premake4 --help.
	if (_ACTION == nil) then return end

	location (CMFT_PROJECTS_DIR .. _ACTION)

	if _ACTION == "clean" then
		os.rmdir(BUILD_DIR)
	end

	if _ACTION == "gmake" then

		if nil == _OPTIONS["compiler"] then
			print("Compiler must be specified!")
			os.exit(1)
		end

		if "osx-gcc" == _OPTIONS["compiler"] then
			premake.gcc.cc  = OSX_GCC_DIR .. "gcc"
			premake.gcc.cxx = OSX_GCC_DIR .. "g++"
			premake.gcc.ar  = OSX_GCC_DIR .. "ar"
			location (CMFT_PROJECTS_DIR .. _ACTION .. "-osx-gcc")
		end

		if "linux-gcc" == _OPTIONS["compiler"] then
			premake.gcc.cc  = LIN_GCC_DIR .. "gcc"
			premake.gcc.cxx = LIN_GCC_DIR .. "g++"
			premake.gcc.ar  = LIN_GCC_DIR .. "ar"
			location (CMFT_PROJECTS_DIR .. _ACTION .. "-linux-gcc")
		end

--		if "linux-clang" == _OPTIONS["compiler"] then
--			premake.gcc.cc  = LIN_CLANG_DIR .. "clang"
--			premake.gcc.cxx = LIN_CLANG_DIR .. "clang++"
--			premake.gcc.ar  = LIN_CLANG_DIR .. "ar"
--			location (CMFT_PROJECTS_DIR .. _ACTION .. "-linux-clang")
--		end
--
--		if "win-mingw" == _OPTIONS["compiler"] then
--			premake.gcc.cc  = WIN_MINGW_DIR .. "x86_64-w64-mingw32-gcc"
--			premake.gcc.cxx = WIN_MINGW_DIR .. "x86_64-w64-mingw32-g++"
--			premake.gcc.ar  = WIN_MINGW_DIR .. "ar"
--			location (CMFT_PROJECTS_DIR .. _ACTION .. "-win-mingw")
--		end
--
--		if "win-clang" == _OPTIONS["compiler"] then
--			premake.gcc.cc  = WIN_CLANG_DIR .. "clang"
--			premake.gcc.cxx = WIN_CLANG_DIR .. "clang++"
--			premake.gcc.ar  = WIN_CLANG_DIR .. "ar"
--			location (CMFT_PROJECTS_DIR .. _ACTION .. "-win-clang")
--		end

	end

	flags
	{
		"StaticRuntime",
		"NoMinimalRebuild",
		"NoPCH",
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
		flags { "OptimizeSpeed", }

	configuration { "*gcc* or *mingw" }
		buildoptions
		{  "-Wall -Wextra -Wno-cast-align -Wcast-qual"
		.. " -Wdisabled-optimization -Wdiv-by-zero -Wendif-labels"
		.. " -Wformat-extra-args -Wformat-security"
		.. " -Wformat-y2k -Wimport -Winit-self -Winline -Winvalid-pch"
		.. " -Werror=missing-braces -Wno-missing-format-attribute"
		.. " -Wmissing-include-dirs -Wmultichar -Wpacked -Wpointer-arith"
		.. " -Wreturn-type -Wsequence-point -Wsign-compare -Wstrict-aliasing"
		.. " -Wstrict-aliasing=2 -Wswitch -Wno-unused-function"
		.. " -Wno-variadic-macros -Wwrite-strings -Werror=declaration-after-statement"
		.. " -Werror=implicit-function-declaration -Werror=nested-externs"
		.. " -Werror=old-style-definition -Werror=strict-prototypes"
		}

	-- VS
	configuration { "vs*" }
		flags { "EnableSSE2", }
		includedirs { BX_DIR .. "include/compat/msvc" }
		defines
		{
			"WIN32",
			"_WIN32",
			--"_HAS_EXCEPTIONS=0",
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

	configuration { "vs2008" }
		includedirs { BX_DIR .. "include/compat/msvc/pre1600" }

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
		includedirs { BX_DIR .. "include/compat/mingw" }
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
		includedirs { BX_DIR .. "include/compat/osx" }

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

--
-- Solution
--
solution "cmft"
language "C++"
configurations { "Debug", "Release" }
platforms { "x32", "x64" }

configuration { "*" }
	debugdir  (CMFT_RUNTIME_DIR)
	includedirs { CMFT_3RDPARTY_DIR }

configuration { "*gcc*" }
	links { "dl" }
	links { "pthread" }

configuration "Debug"
	defines
	{
		"CMFT_CONFIG_DEBUG=1",
	}

configuration "Release"
	defines
	{
		"CMFT_CONFIG_DEBUG=0",
	}

toolchain(CMFT_BUILD_DIR, CMFT_3RDPARTY_DIR)

project "cmft"
    uuid("52267a12-34bf-4834-8245-2bead0100dc8")
	kind "ConsoleApp"

	includedirs
	{
		BX_DIR .. "include/"
	}

	files
	{
		CMFT_SRC_DIR .. "**.c",
		CMFT_SRC_DIR .. "**.h",
		CMFT_SRC_DIR .. "**.cpp",
		CMFT_SRC_DIR .. "**.hpp",

		CMFT_SRC_DIR .. "**/**.c",
		CMFT_SRC_DIR .. "**/**.h",
		CMFT_SRC_DIR .. "**/**.cpp",
		CMFT_SRC_DIR .. "**/**.hpp",
	}

strip()
