/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_HANDLEALLOC_H_HEADER_GUARD
#define DM_HANDLEALLOC_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "../common/common.h" // DM_INLINE
#include "../check.h"         // DM_CHECK

namespace dm
{
    // Adapted from: https://github.com/bkaradzic/bx/blob/master/include/bx/handlealloc.h

    template <uint16_t MaxHandlesT>
    struct HandleAllocT
    {
        HandleAllocT()
        {
            reset();
        }

        #include "handlealloc_inline_impl.h"

        uint16_t count() const
        {
            return m_numHandles;
        }

        uint16_t max() const
        {
            return MaxHandlesT;
        }

    private:
        uint16_t m_handles[MaxHandlesT*2];
        uint16_t m_numHandles;
    };

    struct HandleAlloc
    {
        // Uninitialized state, init() needs to be called !
        HandleAlloc()
        {
            m_handles = NULL;
        }

        HandleAlloc(uint16_t _max, bx::ReallocatorI* _reallocator)
        {
            init(_max, _reallocator);
        }

        HandleAlloc(uint16_t _max, void* _mem, bx::AllocatorI* _allocator)
        {
            init(_max, _mem, _allocator);
        }

        ~HandleAlloc()
        {
            destroy();
        }

        // Allocates memory internally.
        void init(uint16_t _max, bx::ReallocatorI* _reallocator)
        {
            m_maxHandles = _max;
            m_handles = (uint16_t*)BX_ALLOC(_reallocator, sizeFor(_max));
            m_reallocator = _reallocator;
            m_cleanup = true;

            reset();
        }

        enum
        {
            SizePerElement = 2*sizeof(uint16_t),
        };

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return _max*SizePerElement;
        }

        // Uses externally allocated memory.
        void* init(uint16_t _max, void* _mem, bx::AllocatorI* _allocator = NULL)
        {
            m_maxHandles = _max;
            m_handles = (uint16_t*)_mem;
            m_allocator = _allocator;
            m_cleanup = false;

            reset();

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        bool isInitialized() const
        {
            return (NULL != m_handles);
        }

        void destroy()
        {
            if (m_cleanup && NULL != m_handles)
            {
                BX_FREE(m_reallocator, m_handles);
                m_handles = NULL;
            }

            m_numHandles = 0;
        }

        #include "handlealloc_inline_impl.h"

        uint16_t count() const
        {
            return m_numHandles;
        }

        uint16_t max() const
        {
            return m_maxHandles;
        }

        bx::AllocatorI* allocator()
        {
            return m_allocator;
        }

    private:
        uint16_t m_numHandles;
        uint16_t m_maxHandles;
        uint16_t* m_handles;
        union
        {
            bx::AllocatorI*   m_allocator;
            bx::ReallocatorI* m_reallocator;
        };
        bool m_cleanup;
    };

    DM_INLINE HandleAlloc* createHandleAlloc(uint16_t _max, void* _mem, bx::AllocatorI* _allocator)
    {
        return ::new (_mem) HandleAlloc(_max, (uint8_t*)_mem + sizeof(HandleAlloc), _allocator);
    }

    DM_INLINE HandleAlloc* createHandleAlloc(uint16_t _max, bx::AllocatorI* _allocator)
    {
        uint8_t* ptr = (uint8_t*)BX_ALLOC(_allocator, sizeof(HandleAlloc) + HandleAlloc::sizeFor(_max));
        return createHandleAlloc(_max, ptr, _allocator);
    }

    DM_INLINE void destroyHandleAlloc(HandleAlloc* _handleAlloc)
    {
        _handleAlloc->~HandleAlloc();
        BX_FREE(_handleAlloc->allocator(), _handleAlloc);
    }

} // namespace dm

#endif // DM_HANDLEALLOC_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
