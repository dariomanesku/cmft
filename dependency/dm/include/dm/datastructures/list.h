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
    template <typename List>
    DM_INLINE void listFillWith(List* _li, const typename List::ElemTy* _obj)
    {
        typedef typename List::ElemTy Ty;

        Ty* elem = _li->m_elements;
        for (uint16_t ii = _li->count(); ii--; )
        {
            dst = ::new (&elem[ii]) Ty(*_obj);
        }
    }

    template <typename List>
    DM_INLINE uint16_t listAdd(List* _li, const typename List::ElemTy* _obj)
    {
        typedef typename List::ElemTy Ty;

        const uint16_t idx = _li->m_handles.alloc();
        DM_CHECK(idx < _li->max(), "listAdd | %d, %d", idx, _li->max());

        Ty* dst = &_li->m_elements[idx];
        dst = ::new (dst) Ty(*_obj);
        return idx;
    }

    template <typename List>
    DM_INLINE typename List::ElemTy* listAddNew(List* _li)
    {
        typedef typename List::ElemTy Ty;

        const uint16_t idx = _li->m_handles.alloc();
        DM_CHECK(idx < _li->max(), "listAddNew | %d, %d", idx, _li->max());

        Ty* dst = &_li->m_elements[idx];
        dst = ::new (dst) Ty();
        return dst;
    }

    template <typename List>
    DM_INLINE bool listContains(List* _li, uint16_t _handle)
    {
        return _li->m_handles.contains(_handle);
    }

    template <typename List>
    DM_INLINE uint16_t listGetHandleOf(List* _li, const typename List::ElemTy* _obj)
    {
        DM_CHECK(&_li->m_elements[0] <= _obj && _obj < &_li->m_elements[_li->max()], "listGetHandleOf | Object not from the list.");

        return uint16_t(_obj - _li->m_elements);
    }

    template <typename List>
    DM_INLINE typename List::ElemTy* listGetObjFromHandle(List* _li, uint16_t _handle)
    {
        typedef typename List::ElemTy Ty;
        DM_CHECK(_handle < _li->max(), "listGetObjFromHandle | %d, %d", _handle, _li->max());

        return const_cast<Ty*>(&_li->m_elements[_handle]);
    }

    template <typename List>
    DM_INLINE typename List::ElemTy* listGetObjAt(List* _li, uint16_t _idx)
    {
        typedef typename List::ElemTy Ty;
        DM_CHECK(_idx < _li->max(), "listGetObjAt | %d, %d", _idx, _li->max());

        const uint16_t handle = _li->m_handles.getHandleAt(_idx);
        return const_cast<Ty*>(listGetObjFromHandle(_li, handle));
    }

    template <typename List>
    DM_INLINE void listRemove(List* _li, uint16_t _handle)
    {
        typedef typename List::ElemTy Ty;
        DM_CHECK(_handle < _li->max(), "listRemove | %d, %d", _handle, _li->max());

        _li->m_elements[_handle].~Ty();

        _li->m_handles.free(_handle);
    }

    template <typename List>
    DM_INLINE void listRemoveAt(List* _li, uint16_t _idx)
    {
        DM_CHECK(_idx < _li->max(), "listRemoveAt | %d, %d", _idx, _li->max());

        const uint16_t handle = _li->m_handles.getHandleAt(_idx);
        listRemove(_li, handle);
    }

    template <typename List>
    DM_INLINE void listRemoveAll(List* _li)
    {
        for (uint16_t ii = _li->count(); ii--; )
        {
            listRemoveAt(_li, 0);
        }
    }

    template <typename Ty/*obj type*/, uint16_t MaxT>
    struct ListT
    {
        typedef typename ListT::Ty ElemTy;

        enum { Invalid = UINT16_MAX };

        void fillWith(const Ty* _obj)
        {
            listFillWith(this, _obj);
        }

        uint16_t add(const Ty& _obj)
        {
            return listAdd(this, &_obj);
        }

        Ty* addNew()
        {
            return listAddNew(this);
        }

        bool contains(uint16_t _handle)
        {
            return listContains(this, _handle);
        }

        uint16_t getHandleOf(const Ty* _obj) const
        {
            return listGetHandleOf(this, _obj);
        }

        uint16_t getHandleAt(uint16_t _idx) const
        {
            return m_handles.getHandleAt(_idx);
        }

        Ty* getObjFromHandle(uint16_t _handle)
        {
            return listGetObjFromHandle(this, _handle);
        }

        Ty* getObjAt(uint16_t _idx)
        {
            return listGetObjAt(this, _idx);
        }

        const Ty* getObjAt(uint16_t _idx) const
        {
            return listGetObjAt(this, _idx);
        }

        Ty& operator[](uint16_t _idx)
        {
            return *listGetObjAt(this, _idx);
        }

        const Ty& operator[](uint16_t _idx) const
        {
            return *listGetObjAt(this, _idx);
        }

        void remove(uint16_t _handle)
        {
            listRemove(this, _handle);
        }

        void removeAt(uint16_t _idx)
        {
            listRemoveAt(this, _idx);
        }

        void removeAll()
        {
            listRemoveAll(this);
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
        HandleAllocT<MaxT> m_handles;
        Ty m_elements[MaxT];
    };

    template <typename Ty/*obj type*/>
    struct List
    {
        typedef typename List::Ty ElemTy;

        enum { Invalid = UINT16_MAX };

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

        void fillWith(const Ty* _obj)
        {
            listFillWith(this, _obj);
        }

        uint16_t add(const Ty& _obj)
        {
            return listAdd(this, &_obj);
        }

        Ty* addNew()
        {
            return listAddNew(this);
        }

        bool contains(uint16_t _handle)
        {
            return listContains(this, _handle);
        }

        uint16_t getHandleOf(const Ty* _obj) const
        {
            return listGetHandleOf(this, _obj);
        }

        uint16_t getHandleAt(uint16_t _idx) const
        {
            return m_handles.getHandleAt(_idx);
        }

        Ty* getObjFromHandle(uint16_t _handle)
        {
            return listGetObjFromHandle(this, _handle);
        }

        Ty* getObjAt(uint16_t _idx)
        {
            return listGetObjAt(this, _idx);
        }

        const Ty* getObjAt(uint16_t _idx) const
        {
            return listGetObjAt(this, _idx);
        }

        Ty& operator[](uint16_t _idx)
        {
            return *listGetObjAt(this, _idx);
        }

        const Ty& operator[](uint16_t _idx) const
        {
            return *listGetObjAt(this, _idx);
        }

        void remove(uint16_t _handle)
        {
            listRemove(this, _handle);
        }

        void removeAt(uint16_t _idx)
        {
            listRemoveAt(this, _idx);
        }

        void removeAll()
        {
            listRemoveAll(this);
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
