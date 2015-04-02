/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_ALLOCATOR_H_HEADER_GUARD
#define DM_ALLOCATOR_H_HEADER_GUARD

#include <stdint.h> //size_t
#include "../common/common.h" // DM_INLINE

// Allocator impl.
//-----

#if !defined(DM_ALLOCATOR_IMPL)
    namespace dm
    {
        struct Allocator
        {
            static DM_INLINE void* alloc(size_t _size)
            {
                return ::malloc(_size);
            }

            static DM_INLINE void* realloc(void* _ptr, size_t _size)
            {
                return ::realloc(_ptr, _size);
            }

            static DM_INLINE void free(void* _ptr)
            {
                return ::free(_ptr);
            }
        };
    } // namespace dm
    #define DM_ALLOCATOR_IMPL dm::Allocator
#endif //!defined(DM_ALLOCATOR_IMPL)


// Allocator interface.
//-----

#if    defined(DM_ALLOC) &&  defined(DM_REALLOC) &&  defined(DM_FREE)
#elif !defined(DM_ALLOC) && !defined(DM_REALLOC) && !defined(DM_FREE)
#else
#error "Either define alloc(), realloc() and free() or none of them!"
#endif

#if !defined(DM_ALLOC)
    #define DM_ALLOC(_size)         DM_ALLOCATOR_IMPL::alloc(_size)
    #define DM_REALLOC(_ptr, _size) DM_ALLOCATOR_IMPL::realloc(_ptr, _size)
    #define DM_FREE(_ptr)           DM_ALLOCATOR_IMPL::free(_ptr)
#endif //!defined(DM_ALLOC)


#endif // DM_ALLOCATOR_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
