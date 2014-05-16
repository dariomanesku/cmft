/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CONFIG_H_HEADER_GUARD
#define CMFT_CONFIG_H_HEADER_GUARD

#include "macros.h"
#include <stdlib.h> //abort()

#ifndef CMFT_CONFIG_DEBUG
    #define CMFT_CONFIG_DEBUG 1
#endif // CMFT_CONFIG_DEBUG

#define CMFT_ENABLE_INFO_MESSAGES      1
#define CMFT_ENABLE_WARNINGS           1

#if CMFT_CONFIG_DEBUG
    #define CMFT_ENABLE_CL_CHECK           1
    #define CMFT_ENABLE_DEBUG_CHECK        1
    #define CMFT_ENABLE_FILE_ERROR_CHECK   1
    #define CMFT_ENABLE_MEMORY_ALLOC_CHECK 1
#else
    #define CMFT_ENABLE_CL_CHECK           0
    #define CMFT_ENABLE_DEBUG_CHECK        0
    #define CMFT_ENABLE_FILE_ERROR_CHECK   0
    #define CMFT_ENABLE_MEMORY_ALLOC_CHECK 0
#endif

// Cmft warning.
#ifndef CMFT_ENABLE_WARNINGS
    #define CMFT_ENABLE_WARNINGS 0
#endif

#if CMFT_ENABLE_WARNINGS
    #define WARN _WARN
#else
    #define WARN(_condition, ...) do {} while(0)
#endif

extern bool s_printWarnings;
#define _WARN(_format, ...)                                                             \
do                                                                                      \
{                                                                                       \
    if (s_printWarnings)                                                                \
    {                                                                                   \
        fprintf(stderr, "CMFT WARNING " _FILE_LINE_ ": "  _format "\n", ##__VA_ARGS__); \
    }                                                                                   \
} while(0)

// Cmft info.
#ifndef CMFT_ENABLE_INFO_MESSAGES
    #define CMFT_ENABLE_INFO_MESSAGES 0
#endif

#if CMFT_ENABLE_INFO_MESSAGES
    #define INFO _INFO
#else
    #define INFO(_condition, ...) do {} while(0)
#endif

extern bool s_printInfo;
#define _INFO(_format, ...)                                         \
do                                                                  \
{                                                                   \
    if (s_printInfo)                                                \
    {                                                               \
        fprintf(stdout, "CMFT info: " _format "\n", ##__VA_ARGS__); \
    }                                                               \
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
        abort();                                                                            \
    }                                                                                       \
} while(0)

#endif //CMFT_CONFIG_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
