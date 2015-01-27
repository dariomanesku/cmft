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
    template <typename Array>
    DM_INLINE void arrayAddVal(Array* _ar, typename Array::ElemTy _value)
    {
        DM_CHECK(_ar->m_count < _ar->max(), "arrayAddVal | %d, %d", _ar->m_count, _ar->max());

        _ar->m_values[_ar->m_count++] = _value;
    }

    template <typename Array>
    DM_INLINE uint32_t arrayAddObj(Array* _ar, const typename Array::ElemTy& _obj)
    {
        typedef typename Array::ElemTy Ty;
        DM_CHECK(_ar->m_count < _ar->max(), "arrayAddObj | %d, %d", _ar->m_count, _ar->max());

        Ty* dst = &_ar->m_values[_ar->m_count++];
        dst = ::new (dst) Ty(_obj);

        return (_ar->m_count-1);
    }

    template <typename Array>
    DM_INLINE typename Array::ElemTy* arrayAddNew(Array* _ar)
    {
        DM_CHECK(_ar->m_count < _ar->max(), "arrayAddNew | %d, %d", _ar->m_count, _ar->max());

        return &_ar->m_values[_ar->m_count++];
    }

    template <typename Array>
    DM_INLINE void arrayRemove(Array* _ar, uint32_t _idx)
    {
        typedef typename Array::ElemTy Ty;
        DM_CHECK(_ar->m_count <= _ar->max(), "arrayRemove - 0 | %d, %d", _ar->m_count, _ar->max());
        DM_CHECK(_idx < _ar->max(), "arrayRemove - 1 | %d, %d", _idx, _ar->max());

        Ty* elem = &_ar->m_values[_idx];
        Ty* next = &_ar->m_values[_idx+1];

        elem->~Ty();
        memmove(elem, next, (_ar->m_count-_idx-1)*sizeof(Ty));

        _ar->m_count--;
    }

    template <typename Array>
    DM_INLINE void arrayRemoveSwap(Array* _ar, uint32_t _idx)
    {
        typedef typename Array::ElemTy Ty;
        DM_CHECK(_ar->m_count <= _ar->max(), "arrayRemoveSwap - 0 | %d, %d", _ar->m_count, _ar->max());
        DM_CHECK(_idx < _ar->max(), "arrayRemoveSwap - 1 | %d, %d", _idx, _ar->max());

        Ty* elem = &_ar->m_values[_idx];
        Ty* last = &_ar->m_values[_ar->m_count-1];

        elem->~Ty();
        elem->Ty(*last);

        _ar->m_count--;
    }

    template <typename Array>
    DM_INLINE typename Array::ElemTy arrayGetVal(Array* _ar, uint32_t _idx)
    {
        DM_CHECK(_idx < _ar->max(), "arrayGetVal | %d, %d", _idx, _ar->max());

        return _ar->m_values[_idx];
    }

    template <typename Array>
    DM_INLINE typename Array::ElemTy& arrayGetRef(Array* _ar, uint32_t _idx)
    {
        DM_CHECK(_idx < _ar->max(), "arrayGetRef | %d, %d", _idx, _ar->max());

        return const_cast<typename Array::ElemTy&>(_ar->m_values[_idx]);
    }

    template <typename Array>
    DM_INLINE void arrayZero(Array* _ar)
    {
        memset(_ar->m_values, 0, _ar->max()*sizeof(typename Array::ElemTy));
    }

    template <typename Array>
    DM_INLINE void arrayReset(Array* _ar)
    {
        _ar->m_count = 0;
    }

    template <typename Ty, uint32_t MaxT>
    struct ArrayT
    {
        typedef typename ArrayT::Ty ElemTy;

        ArrayT()
        {
            arrayReset(this);
        }

        void add(Ty _value)
        {
            arrayAddVal(this, _value);
        }

        uint32_t addObj(const Ty& _obj)
        {
            return arrayAddObj(this, _obj);
        }

        Ty* addNew()
        {
            return arrayAddNew(this);
        }

        void remove(uint32_t _idx)
        {
            arrayRemove(this, _idx);
        }

        // Uses swap instead of memmove. Order is not preserved!
        void removeSwap(uint32_t _idx)
        {
            arrayRemoveSwap(this, _idx);
        }

        Ty getVal(uint32_t _idx) const
        {
            return arrayGetVal(this, _idx);
        }

        Ty* getPtr(uint32_t _idx)
        {
            return &arrayGetRef(this, _idx);
        }

        const Ty* getPtr(uint32_t _idx) const
        {
            return &arrayGetRef(this, _idx);
        }

        Ty operator[](uint32_t _idx) const
        {
            return arrayGetVal(this, _idx);
        }

        Ty& operator[](uint32_t _idx)
        {
            return arrayGetRef(this, _idx);
        }

        void zero()
        {
            arrayZero(this);
        }

        void reset()
        {
            arrayReset(this);
        }

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
        typedef typename Array::Ty ElemTy;

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

        static inline uint32_t sizeFor(uint32_t _max)
        {
            return _max*sizeof(Ty);
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

        void add(Ty _value)
        {
            arrayAddVal(this, _value);
        }

        uint32_t addObj(const Ty& _obj)
        {
            return arrayAddObj(this, _obj);
        }

        Ty* addNew()
        {
            return arrayAddNew(this);
        }

        void remove(uint32_t _idx)
        {
            arrayRemove(this, _idx);
        }

        // Uses swap instead of memmove. Order is not preserved!
        void removeSwap(uint32_t _idx)
        {
            arrayRemoveSwap(this, _idx);
        }

        Ty getVal(uint32_t _idx) const
        {
            return arrayGetVal(this, _idx);
        }

        Ty* getPtr(uint32_t _idx)
        {
            return &arrayGetRef(this, _idx);
        }

        const Ty* getPtr(uint32_t _idx) const
        {
            return &arrayGetRef(this, _idx);
        }

        Ty operator[](uint32_t _idx) const
        {
            return arrayGetVal(this, _idx);
        }

        Ty& operator[](uint32_t _idx)
        {
            return arrayGetRef(this, _idx);
        }

        void zero()
        {
            arrayZero(this);
        }

        void reset()
        {
            arrayReset(this);
        }

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
        return createArray(_max, ptr);
    }

    template <typename Ty>
    DM_INLINE void destroyArray(Array<Ty>* _array)
    {
        _array->~ArrayT<Ty>();
        delete _array;
    }

} // namespace dm

#endif // DM_ARRAY_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
