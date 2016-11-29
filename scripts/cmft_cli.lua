--
-- Copyright 2014-2015 Dario Manesku. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

function cmftCliProject(_cmftDir)

    local CMFT_INCLUDE_DIR    = (_cmftDir .. "include/")
    local CMFT_SRC_DIR        = (_cmftDir .. "src/cmft/")
    local CMFT_CLI_SRC_DIR    = (_cmftDir .. "src/")
    local CMFT_RUNTIME_DIR    = (_cmftDir .. "runtime/")
    local CMFT_DEPENDENCY_DIR = (_cmftDir .. "dependency/")

    project "cmft_cli"
        uuid("52267a12-34bf-4834-8245-2bead0100dc8")
        kind "ConsoleApp"

        targetname ("cmft")

        configuration { "linux-*" }
            links
            {
                "dl",
                "pthread",
            }

        configuration { "vs*" }
            buildoptions
            {
                "/wd 4127" -- disable 'conditional expression is constant' for do {} while(0)
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
            CMFT_DEPENDENCY_DIR,
            CMFT_SRC_DIR,
            CMFT_CLI_SRC_DIR,
            CMFT_INCLUDE_DIR,
        }

end -- cmftCliProject

-- vim: set sw=4 ts=4 expandtab:
