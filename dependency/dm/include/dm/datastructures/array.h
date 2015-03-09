/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_ARRAY_H_HEADER_GUARD
#define DM_ARRAY_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

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

    public:
        uint32_t m_count;
        Ty m_values[MaxT];
    };

    template <typename Ty>
    struct Array
    {
        // Uninitialized state, init() needs to be called !
        Array()
        {
        }

        Array(uint32_t _max)
        {
            init(_max);
        }

        Array(uint32_t _max, void* _mem)
        {
            init(_max, _mem);
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
        void init(uint16_t _max)
        {
            m_count = 0;
            m_max = _max;
            m_values = (Ty*)DM_ALLOC(sizeFor(_max));
            m_cleanup = true;
        }

        // Uses externaly allocated memory.
        void* init(uint32_t _max, void* _mem = NULL)
        {
            m_count = 0;
            m_max = _max;
            m_values = (Ty*)_mem;
            m_cleanup = false;

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        void destroy()
        {
            if (m_cleanup && NULL != m_values)
            {
                DM_FREE(m_values);
                m_values = NULL;
            }
        }

        #include "array_inline_impl.h"

        uint32_t count() const
        {
            return m_count;
        }

        uint32_t max() const
        {
            return m_max;
        }

    public:
        uint32_t m_count;
        uint32_t m_max;
        Ty* m_values;
        bool m_cleanup;
    };

    template <typename Ty>
    DM_INLINE Array<Ty>* createArray(uint32_t _max, void* _mem)
    {
        return ::new (_mem) Array<Ty>(_max, (uint8_t*)_mem + sizeof(Array<Ty>));
    }

    template <typename Ty>
    DM_INLINE Array<Ty>* createArray(uint32_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(Array<Ty>) + Array<Ty>::sizeFor(_max));
        return createArray<Ty>(_max, ptr);
    }

    template <typename Ty>
    DM_INLINE void destroyArray(Array<Ty>* _array)
    {
        _array->~Array<Ty>();
        delete _array;
    }

} // namespace dm

#endif // DM_ARRAY_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
