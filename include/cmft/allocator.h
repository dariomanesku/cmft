/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_ALLOCATOR_H_HEADER_GUARD
#define CMFT_ALLOCATOR_H_HEADER_GUARD

#include <stdlib.h>
#include <bx/allocator.h>

#if    defined(CMFT_STACK_PUSH) &&  defined(CMFT_STACK_POP)
#elif !defined(CMFT_STACK_PUSH) && !defined(CMFT_STACK_POP)
#else
#error "Either define both stack push and pop or none of them!"
#endif

#if !defined(CMFT_STACK_PUSH)
    #define CMFT_STACK_PUSH()
    #define CMFT_STACK_POP()
#endif

namespace cmft
{
    extern bx::AllocatorI* g_allocator;
    extern bx::AllocatorI* g_stackAllocator;

    static inline void setAllocator(bx::AllocatorI* _allocator)
    {
        g_allocator = _allocator;
    }

    static inline void setStackAllocator(bx::AllocatorI* _allocator)
    {
        g_stackAllocator = _allocator;
    }
};

#endif //CMFT_ALLOCATOR_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
