/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_CHECK_H_HEADER_GUARD
#define DM_CHECK_H_HEADER_GUARD

// Config listing.
//-----

#define DM_CHECK_CONFIG_NOOP        0
#define DM_CHECK_CONFIG_PRINT       1
#define DM_CHECK_CONFIG_DEBUG_BREAK 2

// Choose desired config.
//-----

#ifndef DM_CHECK_CONFIG
    #define DM_CHECK_CONFIG DM_CHECK_CONFIG_NOOP
#endif //DM_CHECK_CONFIG

// Implementation.
//-----

#define DM_STRINGIZE(_x) DM_STRINGIZE_(_x)
#define DM_STRINGIZE_(_x) #_x
#define DM_FILE_LINE "" __FILE__ "(" DM_STRINGIZE(__LINE__) ")"

#define _DM_CHECK_NOOP(_condition, _format, ...) for (;;) { break; }
#define _DM_CHECK_PRINT(_condition, _format, ...)                                          \
    do                                                                                     \
    {                                                                                      \
        if (!(_condition))                                                                 \
        {                                                                                  \
            fprintf(stderr, "DM ERROR (" DM_FILE_LINE "): "  _format "\n", ##__VA_ARGS__); \
        }                                                                                  \
    } while(0)
#define _DM_CHECK_BREAK(_condition, _format, ...)                                          \
    do                                                                                     \
    {                                                                                      \
        if (!(_condition))                                                                 \
        {                                                                                  \
            fprintf(stderr, "DM ERROR (" DM_FILE_LINE "): "  _format "\n", ##__VA_ARGS__); \
            bx::debugBreak();                                                              \
        }                                                                                  \
    } while(0)

#if (DM_CHECK_CONFIG == DM_CHECK_CONFIG_PRINT)
    #define _DM_CHECK _DM_CHECK_PRINT
    #include <stdio.h> // fprintf()
#elif (DM_CHECK_CONFIG == DM_CHECK_CONFIG_DEBUG_BREAK)
    #define _DM_CHECK _DM_CHECK_BREAK
    #include <stdio.h> // fprintf()
    #include "../../3rdparty/bx/debug.h" // bx::debugBreak()
#else
    #define _DM_CHECK _DM_CHECK_NOOP
#endif //(DM_CHECK_CONFIG == DM_CHECK_CONFIG_PRINT)

#if !defined(DM_CHECK)
    #define DM_CHECK _DM_CHECK
#endif //!defined(DM_CHECK)

#endif // DM_CHECK_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
