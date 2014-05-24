--
-- Copyright 2014 Dario Manesku. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

-- DEPENDENCY_DIR has to be defined before including this file.
local THIRDPARTY_DIR = (DEPENDENCY_DIR .. "3rdparty/")

-- CMFT_DIR has to be defined before including this file.
local CMFT_INCLUDE_DIR  = (CMFT_DIR .. "include/")
local CMFT_SRC_DIR      = (CMFT_DIR .. "src/cmft/")

-- BX_DIR has to be defined before including this file.
local BX_INCLUDE_DIR    = (BX_DIR .. "include/")
local BX_THIRDPARTY_DIR = (BX_DIR .. "3rdparty/")

project "cmft"
    uuid("0809b9fb-eaf6-4e80-9d80-7b490e29f212")
    kind "StaticLib"

    configuration { "*gcc*" }
        links { "dl" }
        links { "pthread" }

    configuration { "vs*" }
        buildoptions
        {
            "/wd 4127" -- disable 'conditional expression is constant' for do {} while(0)
        }

    configuration { "Debug" }
        defines
        {
            "CMFT_CONFIG_DEBUG=1",
        }

    configuration { "Release" }
        defines
        {
            "CMFT_CONFIG_DEBUG=0",
        }

    configuration {}

    files
    {
        CMFT_SRC_DIR .. "**.h",
        CMFT_SRC_DIR .. "**.cpp",
        CMFT_SRC_DIR .. "**/**.h",
        CMFT_SRC_DIR .. "**/**.cpp",

        CMFT_INCLUDE_DIR .. "**.h",
    }

    includedirs
    {
        BX_INCLUDE_DIR,
        CMFT_INCLUDE_DIR,
        CMFT_SRC_DIR,
        DEPENDENCY_DIR,
        THIRDPARTY_DIR,
        BX_THIRDPARTY_DIR,
    }

-- vim: set sw=4 ts=4 expandtab:
