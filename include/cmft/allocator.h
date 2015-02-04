/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_ALLOCATOR_H_HEADER_GUARD
#define CMFT_ALLOCATOR_H_HEADER_GUARD

#include <stdlib.h>
#include <bx/allocator.h>

#if !defined(CMFT_STACK_PUSH)
    #define CMFT_STACK_PUSH()
    #define CMFT_STACK_POP()
#endif

namespace cmft
{
    extern bx::ReallocatorI* g_allocator;
    extern bx::ReallocatorI* g_stackAllocator;

    static inline void setAllocator(bx::ReallocatorI* _allocator)
    {
        g_allocator = _allocator;
    }

    static inline void setStackAllocator(bx::ReallocatorI* _allocator)
    {
        g_stackAllocator = _allocator;
    }
};

#endif //CMFT_ALLOCATOR_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
