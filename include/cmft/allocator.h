/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_ALLOCATOR_H_HEADER_GUARD
#define CMFT_ALLOCATOR_H_HEADER_GUARD

#include <stdlib.h>
#include <bx/allocator.h>

namespace cmft
{
    extern bx::AllocatorI* g_allocator;

    static inline void setAllocator(bx::AllocatorI* _allocator)
    {
        g_allocator = _allocator;
    }
};

#endif //CMFT_ALLOCATOR_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
