/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

/*
 * Adapted from: https://github.com/bkaradzic/bx/include/bx/timer.h
 * Copyright 2010-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */
#ifndef CMFT_OS_H_HEADER_GUARD
#define CMFT_OS_H_HEADER_GUARD

#include "platform.h"
#include <stdint.h>

#if CMFT_PLATFORM_WINDOWS
#   include <windows.h>
#else
#   include <sys/time.h> // gettimeofday
#endif // CMFT_PLATFORM_

namespace cmft
{
    inline int64_t getHPCounter()
    {
        #if CMFT_PLATFORM_WINDOWS
            LARGE_INTEGER li;
            // Performance counter value may unexpectedly leap forward
            // http://support.microsoft.com/kb/274323
            QueryPerformanceCounter(&li);
            int64_t i64 = li.QuadPart;
        #else
            struct timeval now;
            gettimeofday(&now, 0);
            int64_t i64 = now.tv_sec*INT64_C(1000000) + now.tv_usec;
        #endif // CMFT_PLATFORM_

        return i64;
    }

    inline int64_t getHPFrequency()
    {
        #if CMFT_PLATFORM_WINDOWS
            LARGE_INTEGER li;
            QueryPerformanceFrequency(&li);
            return li.QuadPart;
        #else
            return INT64_C(1000000);
        #endif // CMFT_PLATFORM_
    }
}

#endif // CMFT_OS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

