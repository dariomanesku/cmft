/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "stb_image.h"

namespace stb
{
    BX_PRAGMA_DIAGNOSTIC_PUSH();
    BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow");
    BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wcast-qual");
    #define STB_IMAGE_STATIC
    #define STB_IMAGE_IMPLEMENTATION
    #include <stb/stb_image.h>
    BX_PRAGMA_DIAGNOSTIC_POP();

} // namespace stb

/* vim: set sw=4 ts=4 expandtab: */
