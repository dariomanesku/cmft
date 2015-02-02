/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_LIST_H_HEADER_GUARD
#define DM_LIST_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

#include "handlealloc.h"

namespace dm
{
    template <typename Ty/*obj type*/, uint16_t MaxT>
    struct ListT
    {
        typedef typename ListT<Ty,MaxT> This;

        ListT()
        {
        }

        #include "list_inline_impl.h"

        uint16_t count() const
        {
            return m_handles.count();
        }

        uint16_t max() const
        {
            return MaxT;
        }

    private:
        HandleAllocT<MaxT> m_handles;
        Ty m_elements[MaxT];
    };

    template <typename Ty/*obj type*/>
    struct List
    {
        typedef typename List<Ty> This;

        // Uninitialized state, init() needs to be called !
        List()
        {
        }

        List(uint16_t _max)
        {
            init(_max);
        }

        List(uint16_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~List()
        {
            destroy();
        }

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return HandleAlloc::sizeFor(_max) + _max*sizeof(Ty);
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_memoryBlock = DM_ALLOC(sizeFor(_max));
            m_cleanup = true;

            void* ptr = m_handles.init(_max, m_memoryBlock);
            m_elements = (Ty*)ptr;
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_memoryBlock = _mem;
            m_cleanup = false;

            void* ptr = m_handles.init(_max, m_memoryBlock);
            m_elements = (Ty*)ptr;

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        void destroy()
        {
            if (NULL != m_memoryBlock)
            {
                m_handles.destroy();
                if (m_cleanup)
                {
                    DM_FREE(m_memoryBlock);
                }
                m_memoryBlock = NULL;
            }
        }

        #include "list_inline_impl.h"

        uint16_t count() const
        {
            return m_handles.count();
        }

        uint16_t max() const
        {
            return m_handles.max();
        }

    private:
        HandleAlloc m_handles;
        Ty* m_elements;
        void* m_memoryBlock;
        bool m_cleanup;
    };

    template <typename Ty/*obj type*/>
    DM_INLINE List<Ty>* createList(uint16_t _max, void* _mem)
    {
        return ::new (_mem) List<Ty>(_max, (uint8_t*)_mem + sizeof(List<Ty>));
    }

    template <typename Ty/*obj type*/>
    DM_INLINE List<Ty>* createList(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(List<Ty>) + List<Ty>::sizeFor(_max));
        return createList(_max, ptr);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE void destroyList(List<Ty>* _list)
    {
        _list->~ListT<Ty>();
        delete _list;
    }

} // namespace dm

#endif // DM_LIST_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
