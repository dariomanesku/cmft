/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_LINKEDLIST_H_HEADER_GUARD
#define DM_LINKEDLIST_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE / BX_UNUSED
#include "../check.h" // DM_CHECK

namespace dm
{
    template <typename Ty/*obj type*/, uint16_t MaxT>
    struct LinkedListT
    {
        typedef LinkedListT<Ty, MaxT> This;

        LinkedListT()
        {
            m_elements[0].m_prev = 0;
            m_elements[0].m_next = 0;
            m_last = 0;
        }

        #include "linkedlist_inline_impl.h"

        uint16_t count() const
        {
            return m_handles.count();
        }

        uint16_t max() const
        {
            return MaxT;
        }

    private:
        uint16_t m_last;
        HandleAllocT<MaxT> m_handles;
        Elem m_elements[MaxT];
    };

    template <typename Ty/*obj type*/>
    struct LinkedList
    {
        typedef LinkedList<Ty> This;

        // Uninitialized state, init() needs to be called !
        LinkedList()
        {
        }

        LinkedList(uint16_t _max)
        {
            init(_max);
        }

        LinkedList(uint16_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~LinkedList()
        {
            destroy();
        }

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return _max*SizePerElement;
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_memoryBlock = DM_ALLOC(sizeFor(_max));
            m_cleanup = true;

            void* ptr = m_handles.init(_max, m_memoryBlock);
            m_elements = (Elem*)ptr;

            m_elements[0].m_prev = 0;
            m_elements[0].m_next = 0;
            m_last = 0;
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_memoryBlock = _mem;
            m_cleanup = false;

            void* ptr = m_handles.init(_max, m_memoryBlock);
            m_elements = (Elem*)ptr;

            m_elements[0].m_prev = 0;
            m_elements[0].m_next = 0;
            m_last = 0;

            uint8_t* end = (uint8_t*)_mem + sizeFor(_max);
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

        #include "linkedlist_inline_impl.h"

        enum
        {
            SizePerElement = sizeof(Elem) + HandleAlloc::SizePerElement,
        };

        uint16_t count() const
        {
            return m_handles.count();
        }

        uint16_t max() const
        {
            return m_handles.max();
        }

    private:
        uint16_t m_last;
        HandleAlloc m_handles;
        void* m_memoryBlock;
        bool m_cleanup;
        Elem* m_elements;
    };

    template <typename Ty/*obj type*/>
    DM_INLINE LinkedList<Ty>* createLinkedList(uint16_t _max, void* _mem)
    {
        return ::new (_mem) LinkedList<Ty>(_max, (uint8_t*)_mem + sizeof(LinkedList<Ty>));
    }

    template <typename Ty/*obj type*/>
    DM_INLINE LinkedList<Ty>* createLinkedList(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(LinkedList<Ty>) + LinkedList<Ty>::sizeFor(_max));
        return createLinkedList<Ty>(_max, ptr);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE void destroyLinkedList(LinkedList<Ty>* _ll)
    {
        _ll->~LinkedList<Ty>();
        delete _ll;
    }

} // namespace dm

#endif // DM_LINKEDLIST_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
