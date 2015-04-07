/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_LINKEDLIST_H_HEADER_GUARD
#define DM_LINKEDLIST_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new>      // placement-new

#include "../common/common.h" // DM_INLINE / BX_UNUSED
#include "../check.h"         // DM_CHECK

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
            m_memoryBlock = NULL;
        }

        LinkedList(uint16_t _max, bx::ReallocatorI* _reallocator)
        {
            init(_max, _reallocator);
        }

        LinkedList(uint16_t _max, void* _mem, bx::AllocatorI* _allocator)
        {
            init(_max, _mem, _allocator);
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
        void init(uint16_t _max, bx::ReallocatorI* _reallocator)
        {
            m_memoryBlock = BX_ALLOC(_reallocator, sizeFor(_max));
            m_reallocator = _reallocator;
            m_cleanup = true;

            void* ptr = m_handles.init(_max, m_memoryBlock);
            m_elements = (Elem*)ptr;

            m_elements[0].m_prev = 0;
            m_elements[0].m_next = 0;
            m_last = 0;
        }

        // Uses externally allocated memory.
        void* init(uint16_t _max, void* _mem, bx::AllocatorI* _allocator = NULL)
        {
            m_memoryBlock = _mem;
            m_allocator = _allocator;
            m_cleanup = false;

            void* ptr = m_handles.init(_max, m_memoryBlock);
            m_elements = (Elem*)ptr;

            m_elements[0].m_prev = 0;
            m_elements[0].m_next = 0;
            m_last = 0;

            uint8_t* end = (uint8_t*)_mem + sizeFor(_max);
            return end;
        }

        bool isInitialized() const
        {
            return (NULL != m_memoryBlock);
        }

        void destroy()
        {
            if (NULL != m_memoryBlock)
            {
                m_handles.destroy();
                if (m_cleanup)
                {
                    BX_FREE(m_reallocator, m_memoryBlock);
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

        bx::AllocatorI* allocator()
        {
            return m_allocator;
        }

    private:
        uint16_t m_last;
        HandleAlloc m_handles;
        void* m_memoryBlock;
        union
        {
            bx::AllocatorI*   m_allocator;
            bx::ReallocatorI* m_reallocator;
        };
        bool m_cleanup;
        Elem* m_elements;
    };

    template <typename Ty/*obj type*/>
    DM_INLINE LinkedList<Ty>* createLinkedList(uint16_t _max, void* _mem, bx::AllocatorI* _allocator)
    {
        return ::new (_mem) LinkedList<Ty>(_max, (uint8_t*)_mem + sizeof(LinkedList<Ty>), _allocator);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE LinkedList<Ty>* createLinkedList(uint16_t _max, bx::AllocatorI* _allocator)
    {
        uint8_t* ptr = (uint8_t*)BX_ALLOC(_allocator, sizeof(LinkedList<Ty>) + LinkedList<Ty>::sizeFor(_max));
        return createLinkedList<Ty>(_max, ptr, _allocator);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE void destroyLinkedList(LinkedList<Ty>* _ll)
    {
        _ll->~LinkedList<Ty>();
        BX_FREE(_ll->allocator(), _ll);
    }

} // namespace dm

#endif // DM_LINKEDLIST_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
