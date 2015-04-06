/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_PRINTCALLBACK_H_HEADER_GUARD
#define CMFT_PRINTCALLBACK_H_HEADER_GUARD

#include "macros.h"

namespace cmft
{

    extern bool g_printWarnings;
    #define _WARN(_format, ...)                                                         \
    do                                                                                  \
    {                                                                                   \
        if (g_printWarnings)                                                            \
        {                                                                               \
            printWarning("CMFT WARNING: "  _format "\n", ##__VA_ARGS__); \
        }                                                                               \
    } while(0)

    extern bool g_printInfo;
    #define _INFO(_format, ...)                                   \
    do                                                            \
    {                                                             \
        if (g_printInfo)                                          \
        {                                                         \
            printInfo("CMFT info: " _format "\n", ##__VA_ARGS__); \
        }                                                         \
    } while(0)


    void printInfo(const char* _format, ...);
    void printWarning(const char* _format, ...);

} // namespace cmft

#endif //CMFT_PRINTCALLBACK_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
