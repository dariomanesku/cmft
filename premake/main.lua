--
-- Copyright 2014 Dario Manesku. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

local CMFT_DIR = (path.getabsolute("..") .. "/")
local BX_DIR = (CMFT_DIR .. "dependency/bx/")

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

-- cmft_cli project.
dofile "cmft_cli.lua"
cmftCliProject(CMFT_DIR, BX_DIR)
compat(BX_DIR)

-- cmft project.
dofile "cmft.lua"
cmftProject(CMFT_DIR, BX_DIR)
compat(BX_DIR)

strip()

-- vim: set sw=4 ts=4 expandtab:
