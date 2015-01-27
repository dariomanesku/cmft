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
    template <typename OrderedList>
    DM_INLINE typename OrderedList::ElemTy* olAddNew(OrderedList* _ol)
    {
        typedef typename OrderedList::ElemTy Ty;
        OrderedList::Base* base = (OrderedList::Base*)_ol;

        Ty* obj = base->addNew();
        const uint16_t handle = base->getHandleOf(obj);
        _ol->m_handleArray.add(handle);
        return obj;
    }

    template <typename OrderedList>
    DM_INLINE typename OrderedList::ElemTy* olGetObjFromIdx(OrderedList* _ol, uint16_t _idx)
    {
        OrderedList::Base* base = (OrderedList::Base*)_ol;
        DM_CHECK(_idx < _ol->max(), "olGetObjFromIdx | %d, %d", _idx, _ol->max());

        const uint16_t handle = _ol->m_handleArray.getVal(_idx);
        return base->getObjFromHandle(handle);
    }

    template <typename OrderedList>
    DM_INLINE uint16_t olGetIdxFromHandle(OrderedList* _ol, uint16_t _handle)
    {
        OrderedList::Base* base = (OrderedList::Base*)_ol;

        for (uint16_t ii = 0, end = base->count(); ii < end; ++ii)
        {
            if (_ol->m_handleArray.getVal(ii) == _handle)
            {
                return ii;
            }
        }

        return OrderedList::Base::Invalid;
    }

    template <typename OrderedList>
    DM_INLINE uint16_t olIdxFromHandle(OrderedList* _ol, uint16_t _handle)
    {
        OrderedList::Base* base = (OrderedList::Base*)_ol;
        DM_CHECK(_idx < _ol->max(), "olGetObjFromIdx | %d, %d", _idx, _ol->max());

        const uint16_t handle = _ol->m_handleArray.getVal(_idx);
        return base->getObjFromHandle(handle);
    }

    template <typename OrderedList>
    DM_INLINE uint16_t olGetHandleFromIdx(OrderedList* _ol, uint16_t _idx)
    {
        DM_CHECK(_idx < _ol->max(), "olGetHandleFromIdx | %d, %d", _idx, _ol->max());

        return _ol->m_handleArray.getVal(_idx);
    }

    template <typename OrderedList>
    DM_INLINE void olRemove(OrderedList* _ol, uint16_t _idx)
    {
        OrderedList::Base* base = (OrderedList::Base*)_ol;
        DM_CHECK(_idx < _ol->max(), "olRemove | %d, %d", _idx, _ol->max());

        const uint16_t handle = _ol->m_handleArray.getVal(_idx);
        base->remove(handle);
        _ol->m_handleArray.remove(_idx);
    }

    template <typename OrderedList>
    DM_INLINE void olRemove(OrderedList* _ol, typename OrderedList::ElemTy* _obj)
    {
        OrderedList::Base* base = (OrderedList::Base*)_ol;

        const uint16_t handle = base->getHandleOf(_obj);
        const uint16_t idx = olGetIdxFromHandle(_ol, handle);
        if (idx != Base::Invalid)
        {
            remove(idx);
        }
    }

    template <typename OrderedList>
    DM_INLINE void olRemoveAll(OrderedList* _ol)
    {
        OrderedList::Base* base = (OrderedList::Base*)_ol;

        base->removeAll();
        _ol->m_handleArray.reset();
    }

    template <typename Ty/*obj type*/, uint16_t MaxT>
    struct OrderedListT : public ListT<Ty, MaxT>
    {
        typedef typename OrderedListT::Ty ElemTy;
        typedef typename ListT<Ty, MaxT> Base;

        Ty* addNew()
        {
            return olAddNew(this);
        }

        Ty* getObjFromIdx(uint16_t _idx)
        {
            return olGetObjFromIdx(this, _idx);
        }

        // Avoid using this, it's O(n).
        uint16_t getIdxFromHandle(uint16_t _handle)
        {
            return olGetIdxFromHandle(this, _handle);
        }

        uint16_t getHandleFromIdx(uint16_t _idx)
        {
            return olGetHandleFromIdx(this, _idx);
        }

        void remove(uint16_t _idx)
        {
            olRemove(this, _idx);
        }

        // Avoid using this, it's O(n).
        void removeObj(Ty* _obj)
        {
            olRemove(this, _obj);
        }

        void removeAll()
        {
            return olRemoveAll(_idx);
        }

    public:
        ArrayT<uint16_t, MaxT> m_handleArray;
    };

    template <typename Ty/*obj type*/>
    struct OrderedList : public List<Ty>
    {
        typedef typename OrderedList::Ty Ty;
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
            m_max = _max;
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
            m_max = _max;
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

        Ty* addNew()
        {
            return olAddNew(this);
        }

        Ty* getObjFromIdx(uint16_t _idx)
        {
            return olGetObjFromIdx(this, _idx);
        }

        // Avoid using this, it's O(n).
        uint16_t getIdxFromHandle(uint16_t _handle)
        {
            return olGetIdxFromHandle(this, _handle);
        }

        uint16_t getHandleFromIdx(uint16_t _idx)
        {
            return olGetHandleFromIdx(this, _idx);
        }

        void remove(uint16_t _idx)
        {
            olRemove(this, _idx);
        }

        // Avoid using this, it's O(n).
        void removeObj(Ty* _obj)
        {
            olRemove(this, _obj);
        }

        void removeAll()
        {
            return olRemoveAll(_idx);
        }

    public:
        uint16_t m_max;
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
