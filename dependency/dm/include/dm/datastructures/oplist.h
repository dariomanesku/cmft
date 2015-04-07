/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_OPLIST_H_HEADER_GUARD
#define DM_OPLIST_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new>      // placement-new

#include "../common/common.h" // DM_INLINE
#include "../check.h"         // DM_CHECK

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
            m_memoryBlock = NULL;
        }

        OpList(uint16_t _max, bx::ReallocatorI* _reallocator)
        {
            init(_max, _reallocator);
        }

        OpList(uint16_t _max, void* _mem, bx::AllocatorI* _allocator)
        {
            init(_max, _mem, _allocator);
        }

        ~OpList()
        {
            destroy();
        }

        // Allocates memory internally.
        void init(uint16_t _max, bx::ReallocatorI* _reallocator)
        {
            m_memoryBlock = BX_ALLOC(_reallocator, sizeFor(_max));
            m_reallocator = _reallocator;
            m_cleanup = true;

            void* ptr = m_memoryBlock;
            ptr = m_handles.init(_max, ptr);
            ptr = m_handleAlloc.init(_max, ptr);
            m_objects = (Ty*)ptr;
        }

        enum
        {
            SizePerElement = sizeof(Ty)
                           + Array<uint16_t>::SizePerElement
                           + HandleAlloc::SizePerElement
                           ,
        };

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return _max*SizePerElement;
        }

        // Uses externally allocated memory.
        void* init(uint16_t _max, void* _mem, bx::AllocatorI* _allocator = NULL)
        {
            m_memoryBlock = _mem;
            m_allocator = _allocator;
            m_cleanup = false;

            void* ptr = m_memoryBlock;
            ptr = m_handles.init(_max, ptr);
            ptr = m_handleAlloc.init(_max, ptr);
            m_objects = (Ty*)ptr;

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
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

        #include "oplist_inline_impl.h"

        uint16_t max() const
        {
            return m_handleAlloc.max();
        }

        bx::AllocatorI* allocator()
        {
            return m_allocator;
        }

    private:
        Array<uint16_t> m_handles;
        HandleAlloc m_handleAlloc;
        Ty* m_objects;
        void* m_memoryBlock;
        union
        {
            bx::AllocatorI*   m_allocator;
            bx::ReallocatorI* m_reallocator;
        };
        bool m_cleanup;
    };

    template <typename Ty/*obj type*/>
    DM_INLINE OpList<Ty>* createOpList(uint16_t _max, void* _mem, bx::AllocatorI* _allocator)
    {
        return ::new (_mem) OpList<Ty>(_max, (uint8_t*)_mem + sizeof(OpList<Ty>), _allocator);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE OpList<Ty>* createOpList(uint16_t _max, bx::AllocatorI* _allocator)
    {
        uint8_t* ptr = (uint8_t*)BX_ALLOC(_allocator, sizeof(OpList<Ty>) + OpList<Ty>::sizeFor(_max));
        return createOpList<Ty>(_max, ptr, _allocator);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE void destroyOpList(OpList<Ty>* _opList)
    {
        _opList->~OpList<Ty>();
        BX_FREE(_opList->allocator(), _opList);
    }

} // namespace dm

#endif // DM_OPLIST_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

