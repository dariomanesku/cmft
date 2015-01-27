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

    template <typename Set>
    DM_INLINE bool setContains(Set* _set, uint16_t _val)
    {
        DM_CHECK(_val < _set->max(), "setContains - 0 | %d, %d", _val, _set->max());

        const uint16_t* sparse = &_set->m_values[_set->max()];
        const uint16_t index = sparse[_val];

        return (index < _set->m_num && _set->m_values[index] == _val);
    }

    template <typename Set>
    DM_INLINE uint16_t setIndexOf(Set* _set, uint16_t _val)
    {
        DM_CHECK(_val < _set->max(), "setIndexOf | %d, %d", _val, _set->max());

        const uint16_t* sparse = &_set->m_values[_set->max()];
        return sparse[_val];
    }

    template <typename Set>
    DM_INLINE uint16_t setInsert(Set* _set, uint16_t _val)
    {
        DM_CHECK(_set->m_num < _set->max(), "setInsert - 0 | %d, %d", _set->m_num, _set->max());
        DM_CHECK(_val < _set->max(),  "setInsert - 1 | %d, %d", _val,  _set->max());

        if (setContains(_set, _val))
        {
            return setIndexOf(_set, _val);
        }

        _set->m_values[_set->m_num] = _val;
        uint16_t* sparse = &_set->m_values[_set->max()];
        sparse[_val] = _set->m_num;

        const uint16_t index = _set->m_num;
        ++_set->m_num;

        return index;
    }

    template <typename Set>
    DM_INLINE uint16_t setSafeInsert(Set* _set, uint16_t _val)
    {
        if (_val < _set->max())
        {
            return setInsert(_set, _val);
        }

        return UINT16_MAX;
    }

    template <typename Set>
    DM_INLINE uint16_t setGetValueAt(Set* _set, uint16_t _idx)
    {
        DM_CHECK(_idx < _set->max(), "setGetValueAt | %d, %d", _idx, _set->max());

        return _set->m_values[_idx];
    }

    template <typename Set>
    DM_INLINE void setRemove(Set* _set, uint16_t _val)
    {
        DM_CHECK(_val < _set->max(), "setRemove - 0 | %d, %d", _val, _set->max());
        DM_CHECK(_set->m_num < _set->max(), "setRemove - 1 | %d, %d", _set->m_num, _set->max());

        if (!setContains(_set, _val))
        {
            return;
        }

        uint16_t* sparse = &_set->m_values[_set->max()];

        const uint16_t index = sparse[_val];
        const uint16_t last = _set->m_values[--_set->m_num];

        DM_CHECK(index < _set->max(), "setRemove - 2 | %d, %d", index, _set->max());
        DM_CHECK(last < _set->max(), "setRemove - 3 | %d, %d", last, _set->max());

        _set->m_values[index] = last;
        sparse[last] = index;
    }

    template <typename Set>
    DM_INLINE void setReset(Set* _set)
    {
        _set->m_num = 0;
    }

    template <uint16_t MaxValueT>
    struct SetT
    {
        SetT() : m_num(0)
        {
        }

        uint16_t insert(uint16_t _val)
        {
            return setInsert(this, _val);
        }

        uint16_t safeInsert(uint16_t _val)
        {
            return setSafeInsert(this, _val);
        }

        bool contains(uint16_t _val)
        {
            return setContains(this, _val);
        }

        uint16_t indexOf(uint16_t _val)
        {
            return setIndexOf(this, _val);
        }

        uint16_t getValueAt(uint16_t _at)
        {
            return setGetValueAt(this, _at);
        }

        void remove(uint16_t _val)
        {
            setRemove(this, _val);
        }

        void reset()
        {
            setReset(this);
        }

        uint16_t count() const
        {
            return m_num;
        }

        uint16_t max() const
        {
            return MaxValueT;
        }

    public:
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

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return 2*_max*sizeof(uint16_t);
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

        uint16_t insert(uint16_t _val)
        {
            return setInsert(this, _val);
        }

        uint16_t safeInsert(uint16_t _val)
        {
            return setSafeInsert(this, _val);
        }

        bool contains(uint16_t _val)
        {
            return setContains(this, _val);
        }

        uint16_t indexOf(uint16_t _val)
        {
            return setIndexOf(this, _val);
        }

        uint16_t getValueAt(uint16_t _at)
        {
            return setGetValueAt(this, _at);
        }

        void remove(uint16_t _val)
        {
            setRemove(this, _val);
        }

        void reset()
        {
            setReset(this);
        }

        uint16_t count() const
        {
            return m_num;
        }

        uint16_t max() const
        {
            return m_max;
        }

    public:
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
