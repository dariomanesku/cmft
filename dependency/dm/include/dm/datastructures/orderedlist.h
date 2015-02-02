/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_ORDEREDLIST_H_HEADER_GUARD
#define DM_ORDEREDLIST_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

#include "list.h"

namespace dm
{
    template <typename Ty/*obj type*/, uint16_t MaxT>
    struct OrderedListT : public ListT<Ty, MaxT>
    {
        typedef typename ListT<Ty, MaxT> Base;

        #include "orderedlist_inline_impl.h"

    private:
        ArrayT<uint16_t, MaxT> m_handleArray;
    };

    template <typename Ty/*obj type*/>
    struct OrderedList : public List<Ty>
    {
        typedef typename List<Ty> Base;

        // Uninitialized state, init() needs to be called !
        OrderedList()
        {
        }

        OrderedList(uint16_t _max) : Base()
        {
            init(_max);
        }

        OrderedList(uint16_t _max, void* _mem) : Base()
        {
            init(_max, _mem);
        }

        ~OrderedList()
        {
            destroy();
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_memoryBlock = DM_ALLOC(sizeFor(_max));
            m_cleanup = true;

            void* end = Base::init(_max, m_memoryBlock);
            m_handleArray.init(_max, end);
        }

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return Base::sizeFor(_max) + Array<uint16_t>::sizeFor(_max);
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_memoryBlock = _mem;
            m_cleanup = false;

            void* end = Base::init(_max, m_memoryBlock);
            m_handleArray.init(_max, end);

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        void destroy()
        {
            if (NULL != m_memoryBlock)
            {
                m_handleArray.destroy();
                Base::destroy();
                if (m_cleanup)
                {
                    DM_FREE(m_memoryBlock);
                }
                m_memoryBlock = NULL;
            }
        }

        #include "orderedlist_inline_impl.h"

    private:
        Array<uint16_t> m_handleArray;
    };

    template <typename Ty/*obj type*/>
    DM_INLINE OrderedList<Ty>* createOrderedList(uint16_t _max, void* _mem)
    {
        return ::new (_mem) OrderedList<Ty>(_max, (uint8_t*)_mem + sizeof(OrderedList<Ty>));
    }

    template <typename Ty/*obj type*/>
    DM_INLINE OrderedList<Ty>* createOrderedList(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(OrderedList<Ty>) + OrderedList<Ty>::sizeFor(_max));
        return createOrderedList(_max, ptr);
    }

    template <typename Ty/*obj type*/>
    DM_INLINE void destroyOrderedList(OrderedList<Ty>* _list)
    {
        _list->~OrderedListT<Ty>();
        delete _list;
    }

} // namespace dm

#endif // DM_ORDEREDLIST_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
