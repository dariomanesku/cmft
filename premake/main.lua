--
-- Copyright 2014 Dario Manesku. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

CMFT_DIR = (path.getabsolute("..") .. "/")
BX_DIR = (CMFT_DIR .. "dependency/3rdparty/bx/")
DEPENDENCY_DIR = (CMFT_DIR .. "dependency/")

local CMFT_BUILD_DIR    = (CMFT_DIR .. "_build/")
local CMFT_PROJECTS_DIR = (CMFT_DIR .. "_projects/")

--
-- Solution.
--
solution "cmft"
language "C++"
configurations { "Debug", "Release" }
platforms { "x32", "x64" }

-- Toolchain.
dofile "toolchain.lua"
cmft_toolchain(CMFT_BUILD_DIR, CMFT_PROJECTS_DIR)
compat(BX_DIR)

-- cmft_cli project
dofile "cmft_cli.lua"

-- cmft project
dofile "cmft.lua"

strip()

-- vim: set sw=4 ts=4 expandtab:
