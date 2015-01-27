/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_ALLOCATOR_H_HEADER_GUARD
#define CMFT_ALLOCATOR_H_HEADER_GUARD

#include <stdlib.h>
#include <bx/allocator.h>

#ifdef CMFT_USE_CRT_ALLOC_FUNCTIONS
    #undef CMFT_ALLOC
    #undef CMFT_REALLOC
    #undef CMFT_FREE
    #define CMFT_ALLOC(_size)         ::malloc(_size)
    #define CMFT_REALLOC(_ptr, _size) ::realloc(_ptr, _size)
    #define CMFT_FREE(_ptr)           ::free(_ptr)
#endif

#if    defined(CMFT_ALLOC) &&  defined(CMFT_REALLOC) &&  defined(CMFT_FREE)
#elif !defined(CMFT_ALLOC) && !defined(CMFT_REALLOC) && !defined(CMFT_FREE)
#else
#error "Either define all: alloc, realloc and free functions, or none of them!"
#endif

#if !defined(CMFT_ALLOC)
    #define CMFT_ALLOC(_size)         BX_ALLOC(cmft::g_allocator, _size)
    #define CMFT_REALLOC(_ptr, _size) BX_REALLOC(cmft::g_allocator, _ptr, _size)
    #define CMFT_FREE(_ptr)           BX_FREE(cmft::g_allocator, _ptr)
#endif //!defined(CMFT_ALLOC)

namespace cmft
{
    extern bx::ReallocatorI* g_allocator;

    static inline void setAllocator(bx::ReallocatorI* _allocator)
    {
        g_allocator = _allocator;
    }
};

#endif //CMFT_ALLOCATOR_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
