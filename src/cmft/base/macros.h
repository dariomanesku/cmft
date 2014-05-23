/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_MACROS_H_HEADER_GUARD
#define CMFT_MACROS_H_HEADER_GUARD

//TODO: use <bx/macros.h>

#define STRINGIZE(_x) STRINGIZE_(_x)
#define STRINGIZE_(_x) #_x

#define CONCATENATE(_x, _y) CONCATENATE_(_x, _y)
#define CONCATENATE_(_x, _y) _x ## _y

#if CMFT_CONFIG_DEBUG
    #define _FILE_LINE_ " " __FILE__ "(" STRINGIZE(__LINE__) ")"
#else
    #define _FILE_LINE_
#endif

#if ((__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
#   define CMFT_UNUSED __attribute__((__unused__))
#else
#   define CMFT_UNUSED
#endif

#define CMFT_MAKEFOURCC(_a, _b, _c, _d)                  \
                       ( ((uint32_t)(uint8_t)(_a))       \
                       | ((uint32_t)(uint8_t)(_b) <<  8) \
                       | ((uint32_t)(uint8_t)(_c) << 16) \
                       | ((uint32_t)(uint8_t)(_d) << 24) \
                       )

#define CMFT_COUNTOF(arr) (sizeof(arr)/sizeof(0[arr]))

#endif //CMFT_MACROS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
