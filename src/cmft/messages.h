/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_MESSAGES_H_HEADER_GUARD
#define CMFT_MESSAGES_H_HEADER_GUARD

#include "base/macros.h"

extern bool g_printWarnings;
#define _WARN(_format, ...)                                                             \
do                                                                                      \
{                                                                                       \
    if (g_printWarnings)                                                                \
    {                                                                                   \
        fprintf(stderr, "CMFT WARNING" _FILE_LINE_ ": "  _format "\n", ##__VA_ARGS__);  \
    }                                                                                   \
} while(0)

extern bool g_printInfo;
#define _INFO(_format, ...)                                         \
do                                                                  \
{                                                                   \
    if (g_printInfo)                                                \
    {                                                               \
        fprintf(stdout, "CMFT info: " _format "\n", ##__VA_ARGS__); \
    }                                                               \
} while(0)

#endif //CMFT_MESSAGES_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
