/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_OPLIST_H_HEADER_GUARD
#define DM_OPLIST_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

#include "array.h"
#include "handlealloc.h"

// Order-preserving list.
//-----

namespace dm
{
    template <typename Ty/*obj type*/, uint16_t MaxT>
    struct OpListT
    {
        #include "oplist_inline_impl.h"

        uint16_t max() const
        {
            return MaxT;
        }

    private:
        ArrayT<uint16_t, MaxT> m_handles;
        HandleAllocT<MaxT> m_handleAlloc;
        Ty m_objects[MaxT];
    };

    template <typename Ty/*obj type*/>
    struct OpList
    {
        // Uninitialized state, init() needs to be called !
        OpList()
        {
        }

        OpList(uint16_t _max)
        {
            init(_max);
        }

        OpList(uint16_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~OpList()
        {
            destroy();
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_memoryBlock = DM_ALLOC(sizeFor(_max));
            m_cleanup = true;

            void* ptr = m_memoryBlock;
            ptr = m_handles.init(_max, ptr);
            ptr = m_handleAlloc.init(_max, ptr);
            m_objects = (Ty*)ptr;
        }

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return Array<uint16_t>::sizeFor(_max)
                 + HandleAlloc::sizeFor(_max)
                 + sizeof(Ty)*_max;
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_memoryBlock = _mem;
            m_cleanup = false;

            void* ptr = m_memoryBlock;
            ptr = m_handles.init(_max, ptr);
            ptr = m_handleAlloc.init(_max, ptr);
            m_objects = (Ty*)ptr;

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        void destroy()
        {
            if (NULL != m_memoryBlock && m_cleanup)
            {
                DM_FREE(m_memoryBlock);
                m_memoryBlock = NULL;
            }
        }

        #include "oplist_inline_impl.h"

        uint16_t max() const
        {
            return m_handleAlloc.max();
        }

    private:
        Array<uint16_t> m_handles;
        HandleAlloc m_handleAlloc;
        Ty* m_objects;
        void* m_memoryBlock;
        bool m_cleanup;
    };

    template <typename Ty/*obj type*/>
    DM_INLINE OpList<Ty>* createOpList(uint16_t _max, void* _mem)
    {
        return ::new (_mem) OpList<Ty>(_max, (uint8_t*)_mem + sizeof(OpList<Ty>));
    }

    template <typename Ty/*obj type*/>
    DM_INLINE OpList<Ty>* createOpList(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(OpList<Ty>) + OpList<Ty>::sizeFor(_max));
        return createOpList<Ty>(_max, ptr);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE void destroyOpList(OpList<Ty>* _opList)
    {
        _opList->~OpList<Ty>();
        delete _opList;
    }

} // namespace dm

#endif // DM_OPLIST_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

