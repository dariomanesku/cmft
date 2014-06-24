--
-- Copyright 2010-2013 Branimir Karadzic. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

function flagsremove(name)
	-- bx's toolchain.lua disables exceptions everywhere
	-- this function can remove some of those flags when
	-- needed.
	local container, err = premake.getobject("solution")
	for _, block in pairs(container["blocks"]) do
		local tbl = block["flags"]
		for index, value in pairs(tbl) do
			if value == name then
				table.remove(tbl, index)
			end
		end
	end
end

project "UnitTest++"
	uuid "ab932f5c-2409-11e3-b000-887628d43830"
	kind "StaticLib"

	flagsremove("NoExceptions")

	files {
		"../3rdparty/UnitTest++/src/*.cpp",
		"../3rdparty/UnitTest++/src/*.h",
	}

	configuration { "linux or osx or android-*" }
		files {
			"../3rdparty/UnitTest++/src/Posix/**.cpp",
			"../3rdparty/UnitTest++/src/Posix/**.h",
		}

	configuration { "mingw or vs*" }
		files {
			"../3rdparty/UnitTest++/src/Win32/**.cpp",
			"../3rdparty/UnitTest++/src/Win32/**.h",
		}

	configuration {}
