--
-- Copyright 2014 Dario Manesku. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

function cmftCliProject(_cmftDir, _bxDir)

    local CMFT_INCLUDE_DIR  = (_cmftDir .. "include/")
    local CMFT_SRC_DIR      = (_cmftDir .. "src/cmft/")
    local CMFT_CLI_SRC_DIR  = (_cmftDir .. "src/")
    local CMFT_RUNTIME_DIR  = (_cmftDir .. "runtime/")

    local BX_INCLUDE_DIR    = (_bxDir .. "include/")
    local BX_THIRDPARTY_DIR = (_bxDir .. "3rdparty/")

    project "cmft_cli"
        uuid("52267a12-34bf-4834-8245-2bead0100dc8")
        kind "ConsoleApp"

        targetname ("cmft")

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

        debugdir (CMFT_RUNTIME_DIR)

        files
        {
            CMFT_SRC_DIR .. "**.h",
            CMFT_SRC_DIR .. "**.cpp",
            CMFT_SRC_DIR .. "**/**.h",
            CMFT_SRC_DIR .. "**/**.cpp",

            CMFT_CLI_SRC_DIR .. "**.h",
            CMFT_CLI_SRC_DIR .. "**.cpp",

            CMFT_INCLUDE_DIR .. "**.h",
        }

        includedirs
        {
            BX_INCLUDE_DIR,
            CMFT_SRC_DIR,
            CMFT_CLI_SRC_DIR,
            CMFT_INCLUDE_DIR,
            BX_THIRDPARTY_DIR,
        }

end -- cmftCliProject

-- vim: set sw=4 ts=4 expandtab:
