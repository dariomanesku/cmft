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
    template <typename LinkedList>
    DM_INLINE typename LinkedList::Elem* llGetObj(LinkedList* _ll, uint16_t _handle)
    {
        DM_CHECK(_handle < _ll->max(), "llGetObj | %d, %d", _handle, _ll->max());

        return &_ll->m_elements[_handle];
    }

    template <typename LinkedList>
    DM_INLINE typename LinkedList::Elem* llLastElem(LinkedList* _ll)
    {
        return llGetObj(_ll, _ll->m_last);
    }

    template <typename LinkedList>
    DM_INLINE typename LinkedList::Elem* llFirstElem(LinkedList* _ll)
    {
        return llGetObj(_ll, llGetObj(_ll, _ll->m_last)->m_next);
    }

    template <typename LinkedList>
    DM_INLINE uint16_t llNext(LinkedList* _ll, uint16_t _handle)
    {
        DM_CHECK(_handle < _ll->max(), "llNext | %d, %d", _handle, _ll->max());

        return llGetObj(_ll, _handle)->m_next;
    }

    template <typename LinkedList>
    DM_INLINE uint16_t llPrev(LinkedList* _ll, uint16_t _handle)
    {
        DM_CHECK(_handle < _ll->max(), "llPrev | %d, %d", _handle, _ll->max());

        return llGetObj(_ll, _handle)->m_prev;
    }

    template <typename LinkedList>
    DM_INLINE bool llContains(LinkedList* _ll, const typename LinkedList::ObjTy* _obj)
    {
        return (&_ll->m_elements[0] <= _obj && _obj < &_ll->m_elements[_ll->max()]);
    }

    template <typename LinkedList>
    DM_INLINE typename LinkedList::Elem* llNext(LinkedList* _ll, const typename LinkedList::ObjTy* _obj)
    {
        typedef typename LinkedList::Elem Elem;
        DM_CHECK(llContains(_ll, _obj), "LinkedListT::next | Object not from the list.");

        const Elem* elem = static_cast<const Elem*>(_obj);
        return &_ll->m_elements[elem->m_next];
    }

    template <typename LinkedList>
    DM_INLINE typename LinkedList::Elem* llPrev(LinkedList* _ll, const typename LinkedList::ObjTy* _obj)
    {
        typedef typename LinkedList::Elem Elem;
        DM_CHECK(llContains(_ll, _obj), "LinkedListT::prev | Object not from the list.");

        const Elem* elem = static_cast<const Elem*>(_obj);
        return &_ll->m_elements[elem->m_prev];
    }

    template <typename LinkedList>
    DM_INLINE uint16_t llGetHandle(LinkedList* _ll, const typename LinkedList::ObjTy* _obj)
    {
        typedef typename LinkedList::Elem Elem;
        DM_CHECK(llContains(_ll, _obj), "llGetHandle | Object not from the list.");

        return (uint16_t)((Elem*)_obj - _ll->m_elements);
    }

#if 0 // Debug only !
    #include <stdio.h>
    #include "../../../3rdparty/bx/debug.h"

    template <typename LinkedList>
    DM_INLINE void llCheckList(LinkedList* _ll)
    {
        typedef typename LinkedList::Elem Elem;

        Elem* begin = llFirstElem(_ll);
        Elem* end = llLastElem(_ll);

        printf("L |");
        Elem* curr = begin;
        for (uint16_t ii = _ll->count()-1; ii--; )
        {
            printf("%d %d %d|", curr->m_prev, llGetHandle(_ll, curr), curr->m_next);
            curr = llNext(_ll, curr);
        }
        printf("%d %d %d|\n", curr->m_prev, llGetHandle(_ll, curr), curr->m_next);

        if (curr != end)
        {
            bx::debugBreak();
        }
    }
#else
    template <typename LinkedList>
    DM_INLINE void llCheckList(LinkedList* _ll)
    {
       BX_UNUSED(_ll);
    }
#endif //1

    template <typename LinkedList>
    DM_INLINE typename LinkedList::Elem* llGetObjAt(LinkedList* _ll, uint16_t _idx)
    {
        DM_CHECK(_idx < _ll->max(), "llGetObj | %d, %d", _idx, _ll->max());

        const uint16_t handle = ll->m_handles.getHandleAt(_idx);
        return &ll->m_elements[handle];
    }

    template <typename LinkedList>
    DM_INLINE typename LinkedList::Elem* llInsertAfter(LinkedList* _ll, const typename LinkedList::ObjTy* _obj)
    {
        typedef typename LinkedList::Elem Elem;

        const uint16_t idx = _ll->m_handles.alloc();

        Elem* elem = &_ll->m_elements[idx];
        elem = ::new (elem) Elem();

        Elem* prev = (Elem*)_obj;
        const uint16_t prevHandle = llGetHandle(_ll, prev);
        Elem* next = llGetObj(_ll, prev->m_next);

        elem->m_prev = prevHandle;
        elem->m_next = prev->m_next;

        prev->m_next = idx;
        next->m_prev = idx;

        if (_ll->m_last == prevHandle)
        {
            _ll->m_last = idx;
        }

        llCheckList(_ll);

        return elem;
    }

    template <typename LinkedList>
    DM_INLINE void llRemove(LinkedList* _ll, uint16_t _handle)
    {
        typedef typename LinkedList::Elem Elem;

        DM_CHECK(_handle < _ll->max(), "llRemove | %d, %d", _handle, _ll->max());

        Elem* elem = _ll->getObj(_handle);
        Elem* prev = _ll->getObj(elem->m_prev);
        Elem* next = _ll->getObj(elem->m_next);

        prev->m_next = elem->m_next;
        next->m_prev = elem->m_prev;

        _ll->m_handles.free(_handle);

        if (_handle == _ll->m_last)
        {
            _ll->m_last = elem->m_prev;
        }

        llCheckList(_ll);
    }

    template <typename Ty/*obj type*/, uint16_t MaxT>
    struct LinkedListT
    {
        typedef typename LinkedListT::Ty ObjTy;

        struct Node
        {
            uint16_t m_prev;
            uint16_t m_next;
        };

        struct Elem : Node,Ty
        {
        };

        LinkedListT()
        {
            m_elements[0].m_prev = 0;
            m_elements[0].m_next = 0;
            m_last = 0;
        }

        Elem* insertAfter(const Ty* _obj)
        {
            return llInsertAfter(this, _obj);
        }

        Elem* addNew()
        {
            return insertAfter(m_last);
        }

        Elem* insertAfter(uint16_t _handle)
        {
            DM_CHECK(_handle < max(), "LinkedListT::insertAfter | %d, %d", _handle, max());

            return insertAfter(getObj(_handle));
        }

        Elem* next(const Ty* _obj)
        {
            return llNext(this, _obj);
        }

        Elem* prev(const Ty* _obj)
        {
            return llPrev(this, _obj);
        }

        uint16_t next(uint16_t _handle)
        {
            return llNext(this, _handle);
        }

        uint16_t prev(uint16_t _handle)
        {
            return llPrev(this, _handle);
        }

        Elem* lastElem()
        {
            return llLastElem(this);
        }

        Elem* firstElem()
        {
            return llFirstElem(this);
        }

        uint16_t lastHandle()
        {
            return m_last;
        }

        uint16_t firstHandle()
        {
            return getObj(m_last)->m_next;
        }

        uint16_t getHandle(const Ty* _obj)
        {
            return llGetHandle(this, _obj);
        }

        Elem* getObj(uint16_t _handle)
        {
            return llGetObj(this, _handle);
        }

        Elem* getObjAt(uint16_t _idx)
        {
            return llGetObjAt(_idx);
        }

        Elem* operator[](uint16_t _idx)
        {
            return llGetObjAt(_idx);
        }

        const Elem* operator[](uint16_t _idx) const
        {
            return llGetObjAt(_idx);
        }

        void remove(uint16_t _handle)
        {
            return llRemove(this, _handle);
        }

        void removeAll()
        {
            for (uint16_t ii = m_handles.count(); ii--; )
            {
                Elem* elem = getObj(m_handles.getHandleAt(ii));
                elem->~Elem();
                BX_UNUSED(elem);
            }

            reset();
        }

        void reset()
        {
            m_handles.reset();
            m_elements[0].m_next = 0;
            m_last = 0;
        }

        bool contains(uint16_t _handle)
        {
            return m_handles.contains(_handle);
        }

        bool contains(const Ty* _obj)
        {
            return llContains(this, _obj);
        }

        uint16_t count() const
        {
            return m_handles.count();
        }

        uint16_t max() const
        {
            return MaxT;
        }

    public:
        uint16_t m_last;
        HandleAllocT<MaxT> m_handles;
        Elem m_elements[MaxT];
    };

    template <typename Ty/*obj type*/>
    struct LinkedList
    {
        typedef typename LinkedList::Ty ObjTy;

        struct Node
        {
            uint16_t m_prev;
            uint16_t m_next;
        };

        struct Elem : Node,Ty
        {
        };

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
            return HandleAlloc::sizeFor(_max) + _max*sizeof(Elem);
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

        Elem* insertAfter(const Ty* _obj)
        {
            return llInsertAfter(this, _obj);
        }

        Elem* addNew()
        {
            return insertAfter(m_last);
        }

        Elem* insertAfter(uint16_t _handle)
        {
            DM_CHECK(_handle < max(), "LinkedList::insertAfter | %d, %d", _handle, max());

            return insertAfter(getObj(_handle));
        }

        Elem* next(const Ty* _obj)
        {
            return llNext(this, _obj);
        }

        Elem* prev(const Ty* _obj)
        {
            return llPrev(this, _obj);
        }

        uint16_t next(uint16_t _handle)
        {
            return llNext(this, _handle);
        }

        uint16_t prev(uint16_t _handle)
        {
            return llPrev(this, _handle);
        }

        Elem* lastElem()
        {
            return llLastElem(this);
        }

        Elem* firstElem()
        {
            return llFirstElem(this);
        }

        uint16_t lastHandle()
        {
            return m_last;
        }

        uint16_t firstHandle()
        {
            return getObj(m_last)->m_next;
        }

        uint16_t getHandle(const Ty* _obj)
        {
            return llGetHandle(this, _obj);
        }

        Elem* getObj(uint16_t _handle)
        {
            return llGetObj(this, _handle);
        }

        Elem* getObjAt(uint16_t _idx)
        {
            return llGetObjAt(_idx);
        }

        Elem* operator[](uint16_t _idx)
        {
            return llGetObjAt(_idx);
        }

        const Elem* operator[](uint16_t _idx) const
        {
            return llGetObjAt(_idx);
        }

        void checkList()
        {
            #if 0
                Elem* begin = firstElem();
                Elem* end = lastElem();

                //printf("L ");
                Elem* curr = begin;
                for (uint16_t ii = count()-1; ii--; )
                {
                    //printf("%d %d %d|", curr->m_prev, getHandle(curr), curr->m_next);
                    curr = next(curr);
                }
                //printf("%d %d %d|", curr->m_prev, getHandle(curr), curr->m_next);
                //printf("\n");

                if (curr != end)
                {
                    bx::debugBreak();
                }
            #endif
        }

        void remove(uint16_t _handle)
        {
            return llRemove(this, _handle);
        }

        void removeAll()
        {
            for (uint16_t ii = m_handles.count(); ii--; )
            {
                Elem* elem = getObj(m_handles.getHandleAt(ii));
                elem->~Elem();
                BX_UNUSED(elem);
            }

            reset();
        }

        void reset()
        {
            m_handles.reset();
            m_elements[0].m_prev = 0;
            m_elements[0].m_next = 0;
            m_last = 0;
        }

        bool contains(uint16_t _handle)
        {
            return m_handles.contains(_handle);
        }

        bool contains(const Ty* _obj)
        {
            return llContains(this, _obj);
        }

        uint16_t count() const
        {
            return m_handles.count();
        }

        uint16_t max() const
        {
            return m_handles.max();
        }

    public:
        uint16_t m_last;
        HandleAlloc m_handles;
        void* m_memoryBlock;
        bool m_cleanup;
        Elem* m_elements;
    };

} // namespace dm

#endif // DM_LINKEDLIST_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
