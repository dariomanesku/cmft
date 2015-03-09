/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_SET_H_HEADER_GUARD
#define DM_SET_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

namespace dm
{
    // Based on: http://research.swtch.com/sparse

    template <uint16_t MaxValueT>
    struct SetT
    {
        SetT()
        {
            m_num = 0;
        }

        #include "set_inline_impl.h"

        uint16_t count() const
        {
            return m_num;
        }

        uint16_t max() const
        {
            return MaxValueT;
        }

    private:
        uint16_t m_num;
        uint16_t m_values[MaxValueT*2];
    };

    struct Set
    {
        // Uninitialized state, init() needs to be called !
        Set()
        {
        }

        Set(uint16_t _max)
        {
            init(_max);
        }

        Set(uint16_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~Set()
        {
            destroy();
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_num = 0;
            m_max = _max;
            m_values = (uint16_t*)DM_ALLOC(sizeFor(_max));
            m_cleanup = true;
        }

        enum
        {
            SizePerElement = 2*sizeof(uint16_t),
        };

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return _max*SizePerElement;
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_num = 0;
            m_max = _max;
            m_values = (uint16_t*)_mem;
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

        #include "set_inline_impl.h"

        uint16_t count() const
        {
            return m_num;
        }

        uint16_t max() const
        {
            return m_max;
        }

    private:
        uint16_t m_max;
        uint16_t m_num;
        uint16_t* m_values;
        bool m_cleanup;
    };

    DM_INLINE Set* createSet(uint16_t _max, void* _mem)
    {
        return ::new (_mem) Set(_max, (uint8_t*)_mem + sizeof(Set));
    }

    DM_INLINE Set* createSet(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(Set) + Set::sizeFor(_max));
        return createSet(_max, ptr);
    }

    DM_INLINE void destroySet(Set* _set)
    {
        _set->~Set();
        delete _set;
    }

} // namespace dm

#endif // DM_SET_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
