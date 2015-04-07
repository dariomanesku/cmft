/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_ARRAY_H_HEADER_GUARD
#define DM_ARRAY_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new>      // placement-new

#include "../common/common.h" // DM_INLINE
#include "../check.h"         // DM_CHECK

namespace dm
{
    template <typename Ty, uint32_t MaxT>
    struct ArrayT
    {
        ArrayT()
        {
            m_count = 0;
        }

        #include "array_inline_impl.h"

        uint32_t count() const
        {
            return m_count;
        }

        uint32_t max() const
        {
            return MaxT;
        }

    private:
        uint32_t m_count;
        Ty m_values[MaxT];
    };

    template <typename Ty>
    struct Array
    {
        // Uninitialized state, init() needs to be called !
        Array()
        {
            m_values = NULL;
        }

        Array(uint32_t _max, bx::ReallocatorI* _reallocator)
        {
            init(_max, _reallocator);
        }

        Array(uint32_t _max, void* _mem, bx::AllocatorI* _allocator)
        {
            init(_max, _mem, _allocator);
        }

        ~Array()
        {
            destroy();
        }

        enum
        {
            SizePerElement = sizeof(Ty),
        };

        static inline uint32_t sizeFor(uint32_t _max)
        {
            return _max*SizePerElement;
        }

        // Allocates memory internally.
        void init(uint32_t _max, bx::ReallocatorI* _reallocator)
        {
            m_count = 0;
            m_max = _max;
            m_values = (Ty*)BX_ALLOC(_reallocator, sizeFor(_max));
            m_reallocator = _reallocator;
            m_cleanup = true;
        }

        // Uses externally allocated memory.
        void* init(uint32_t _max, void* _mem, bx::AllocatorI* _allocator = NULL)
        {
            m_count = 0;
            m_max = _max;
            m_values = (Ty*)_mem;
            m_allocator = _allocator;
            m_cleanup = false;

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        bool isInitialized() const
        {
            return (NULL != m_values);
        }

        void reinit(uint32_t _max, bx::ReallocatorI* _reallocator)
        {
            if (isInitialized())
            {
                destroy();
            }

            init(_max, _reallocator);
        }

        void destroy()
        {
            if (m_cleanup && NULL != m_values)
            {
                BX_FREE(m_reallocator, m_values);
                m_values = NULL;
            }

            m_count = 0;
        }

        #define DM_DYNAMIC_ARRAY
        #include "array_inline_impl.h"

        uint32_t count() const
        {
            return m_count;
        }

        uint32_t max() const
        {
            return m_max;
        }

        bx::AllocatorI* allocator()
        {
            return m_allocator;
        }

    private:
        uint32_t m_count;
        uint32_t m_max;
        Ty* m_values;
        union
        {
            bx::AllocatorI*   m_allocator;
            bx::ReallocatorI* m_reallocator;
        };
        bool m_cleanup;
    };

    template <typename Ty>
    DM_INLINE Array<Ty>* createArray(uint32_t _max, void* _mem, bx::AllocatorI* _allocator)
    {
        return ::new (_mem) Array<Ty>(_max, (uint8_t*)_mem + sizeof(Array<Ty>), _allocator);
    }

    template <typename Ty>
    DM_INLINE Array<Ty>* createArray(uint32_t _max, bx::AllocatorI* _allocator)
    {
        uint8_t* ptr = (uint8_t*)BX_ALLOC(_allocator, sizeof(Array<Ty>) + Array<Ty>::sizeFor(_max));
        return createArray<Ty>(_max, ptr, _allocator);
    }

    template <typename Ty>
    DM_INLINE void destroyArray(Array<Ty>* _array)
    {
        _array->~Array<Ty>();
        BX_FREE(_array->allocator(), _array);
    }

} // namespace dm

#endif // DM_ARRAY_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
