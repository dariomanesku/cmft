/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CONFIG_H_HEADER_GUARD
#define CMFT_CONFIG_H_HEADER_GUARD

#include <stdlib.h> //abort()
#include <stdio.h>  //stderr
#include <cmft/print.h> // PrintFunc

// Config.
//-----

// This flags can also be specified in the build tool by defining CMFT_CUSTOM_CONFIG.
#ifndef CMFT_CUSTOM_CONFIG
    // Output messages.
    #define CMFT_ENABLE_INFO_MESSAGES   1
    #define CMFT_ENABLE_WARNINGS        1
    #define CMFT_ENABLE_PROGRESS_REPORT 0

    // Flush output.
    #define CMFT_ALWAYS_FLUSH_OUTPUT 0

    // Checks.
    #define CMFT_ENABLE_CL_CHECK           0
    #define CMFT_ENABLE_DEBUG_CHECK        0
    #define CMFT_ENABLE_FILE_ERROR_CHECK   0
    #define CMFT_ENABLE_MEMORY_ALLOC_CHECK 0
#endif // CMFT_CUSTOM_CONFIG

// When CMFT_TEST_BUILD is 0, 'src/cmft_cli/cmft_cli.h' gets executed.
// When CMFT_TEST_BUILD is 1, 'src/tests/test.h'        gets executed.
#define CMFT_TEST_BUILD 0

// Implementation.
//-----

// Flush output.
#ifndef CMFT_ALWAYS_FLUSH_OUTPUT
    #define CMFT_ALWAYS_FLUSH_OUTPUT 0
#endif //CMFT_ALWAYS_FLUSH_OUTPUT

#if CMFT_ALWAYS_FLUSH_OUTPUT
    #define CMFT_FLUSH_OUTPUT() do { fflush(stdout); fflush(stderr); } while(0)
#else
    #define CMFT_FLUSH_OUTPUT() do {} while(0)
#endif

// Cmft progress.
#ifndef CMFT_ENABLE_PROGRESS_REPORT
    #define CMFT_ENABLE_PROGRESS_REPORT 0
#endif

#if CMFT_ENABLE_PROGRESS_REPORT
    #define CMFT_PROGRESS(_format, ...)      \
    do                                       \
    {                                        \
        printf(_format "\n", ##__VA_ARGS__); \
        CMFT_FLUSH_OUTPUT();                 \
    } while(0)
#else
    #define CMFT_PROGRESS(...) do {} while(0)
#endif

// Cmft info.
#ifndef CMFT_ENABLE_INFO_MESSAGES
    #define CMFT_ENABLE_INFO_MESSAGES 0
#endif

#if CMFT_ENABLE_INFO_MESSAGES
    #define INFO _INFO
#else
    #define INFO(...) do {} while(0)
#endif

namespace cmft { extern PrintFunc printfInfo; }
#define _INFO(_format, ...)                                          \
do                                                                   \
{                                                                    \
    if (NULL != cmft::printfInfo)                                    \
    {                                                                \
        cmft::printfInfo("CMFT info: " _format "\n", ##__VA_ARGS__); \
    }                                                                \
} while(0)

// Cmft warning.
#ifndef CMFT_ENABLE_WARNINGS
    #define CMFT_ENABLE_WARNINGS 0
#endif

#if CMFT_ENABLE_WARNINGS
    #define WARN _WARN
#else
    #define WARN(...) do {} while(0)
#endif

namespace cmft { extern PrintFunc printfWarning; }
#define _WARN(_format, ...)                                                 \
do                                                                          \
{                                                                           \
    if (NULL != cmft::printfWarning)                                        \
    {                                                                       \
        cmft::printfWarning("CMFT WARNING: "  _format "\n", ##__VA_ARGS__); \
    }                                                                       \
} while(0)

// File error check.
#ifndef CMFT_ENABLE_FILE_ERROR_CHECK
    #define CMFT_ENABLE_FILE_ERROR_CHECK 0
#endif

#if CMFT_ENABLE_FILE_ERROR_CHECK
    #define FERROR_CHECK _FERROR_CHECK
#else
    #define FERROR_CHECK(_fp) do {} while(0)
#endif

#define _FERROR_CHECK(_fp)                                         \
do                                                                 \
{                                                                  \
    if (ferror(_fp))                                               \
    {                                                              \
        fprintf(stderr, "CMFT FILE I/O ERROR " _FILE_LINE_ ".\n"); \
        CMFT_FLUSH_OUTPUT();                                       \
    }                                                              \
} while(0)

// Memory alloc check.
#ifndef CMFT_ENABLE_MEMORY_ALLOC_CHECK
    #define CMFT_ENABLE_MEMORY_ALLOC_CHECK 0
#endif

#if CMFT_ENABLE_MEMORY_ALLOC_CHECK
    #define MALLOC_CHECK _MALLOC_CHECK
#else
    #define MALLOC_CHECK(_fp) do {} while(0)
#endif

#define _MALLOC_CHECK(_ptr)                                            \
do                                                                     \
{                                                                      \
    if (NULL == _ptr)                                                  \
    {                                                                  \
        fprintf(stderr, "CMFT MEMORY ALLOC ERROR " _FILE_LINE_ ".\n"); \
        CMFT_FLUSH_OUTPUT();                                           \
    }                                                                  \
} while(0)

// Debug check.
#ifndef CMFT_ENABLE_DEBUG_CHECK
    #define CMFT_ENABLE_DEBUG_CHECK 0
#endif

#if CMFT_ENABLE_DEBUG_CHECK
    #define DEBUG_CHECK _DEBUG_CHECK
#else
    #define DEBUG_CHECK(_condition, ...) do {} while(0)
#endif

#define _DEBUG_CHECK(_condition, _format, ...)                                              \
do                                                                                          \
{                                                                                           \
    if (!(_condition))                                                                      \
    {                                                                                       \
        fprintf(stderr, "CMFT DEBUG CHECK " _FILE_LINE_ ": "  _format "\n", ##__VA_ARGS__); \
        CMFT_FLUSH_OUTPUT();                                                                \
        abort();                                                                            \
    }                                                                                       \
} while(0)

// CL check.
#ifndef CMFT_ENABLE_CL_CHECK
    #define CMFT_ENABLE_CL_CHECK 0
#endif

#if CMFT_ENABLE_CL_CHECK
    #define CL_CHECK            _CL_CHECK
    #define CL_CHECK_ERR        _CL_CHECK_ERR
#else
    #define CL_CHECK(_expr)     _expr
    #define CL_CHECK_ERR(_expr) BX_UNUSED(_expr)
#endif

#define _CL_CHECK(_expr)                                                                 \
    do                                                                                   \
    {                                                                                    \
        cl_int err = _expr;                                                              \
        if (CL_SUCCESS != err)                                                           \
        {                                                                                \
            fprintf(stderr, "CMFT OpenCL Error: '%s' returned %d!\n", #_expr, (int)err); \
            CMFT_FLUSH_OUTPUT();                                                         \
            abort();                                                                     \
        }                                                                                \
    } while (0)

#define _CL_CHECK_ERR(_err)                                     \
    if (CL_SUCCESS != _err)                                     \
    {                                                           \
        fprintf(stderr, "CMFT OpenCL Error: %d!\n", (int)_err); \
        CMFT_FLUSH_OUTPUT();                                    \
        abort();                                                \
    }

#endif //CMFT_CONFIG_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
