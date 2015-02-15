/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_OBJARRAY_H_HEADER_GUARD
#define DM_OBJARRAY_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

namespace dm
{
    template <typename Ty, uint32_t MaxT>
    struct ObjArrayT
    {
        typedef typename ObjArrayT::Ty ElemTy;

        ObjArrayT()
        {
            m_count = 0;
        }

        #include "objarray_inline_impl.h"

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
    struct ObjArray
    {
        typedef typename ObjArray::Ty ElemTy;

        // Uninitialized state, init() needs to be called !
        ObjArray()
        {
        }

        ObjArray(uint32_t _max)
        {
            init(_max);
        }

        ObjArray(uint32_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~ObjArray()
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

        #include "objarray_inline_impl.h"

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
    DM_INLINE ObjArray<Ty>* createObjArray(uint32_t _max, void* _mem)
    {
        return ::new (_mem) ObjArray<Ty>(_max, (uint8_t*)_mem + sizeof(ObjArray<Ty>));
    }

    template <typename Ty>
    DM_INLINE ObjArray<Ty>* createObjArray(uint32_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(ObjArray<Ty>) + ObjArray<Ty>::sizeFor(_max));
        return createObjArray(_max, ptr);
    }

    template <typename Ty>
    DM_INLINE void destroyObjArray(ObjArray<Ty>* _objarray)
    {
        _objarray->~ObjArrayT<Ty>();
        delete _objarray;
    }

} // namespace dm

#endif // DM_OBJARRAY_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
