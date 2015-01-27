/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_KVMAP_H_HEADER_GUARD
#define DM_KVMAP_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

#include "set.h"

namespace dm
{
    template <typename Kvmap>
    DM_INLINE void kvmapInsert(Kvmap* _kvm, uint16_t _key, typename Kvmap::ValTy _value)
    {
        DM_CHECK(_key < _kvm->max(), "kvmapInsert - 0 | %d, %d", _key, _kvm->max());

        if (_key < _kvm->max())
        {
            const uint16_t index = _kvm->m_set.insert(_key);

            DM_CHECK(index < _kvm->max(), "kvmapInsert - 1 | %d, %d", _key, _kvm->max());
            _kvm->m_values[index] = _value;
        }
    }

    template <typename Kvmap>
    DM_INLINE bool kvmapContains(Kvmap* _kvm, uint16_t _key)
    {
        DM_CHECK(_key < _kvm->max(), "kvmapContains | %d, %d", _key, _kvm->max());

        return _kvm->m_set.contains(_key);
    }

    template <typename Kvmap>
    DM_INLINE typename Kvmap::ValTy kvmapValueOf(Kvmap* _kvm, uint16_t _key)
    {
        DM_CHECK(_kvm->m_set.indexOf(_key) < _kvm->max(), "kvmapValueOf | %d, %d, %d", _key, _kvm->m_set.indexOf(_key), _kvm->max());

        return _kvm->m_values[_kvm->m_set.indexOf(_key)];
    }

    template <typename Kvmap>
    DM_INLINE uint16_t kvmapGetKeyAt(Kvmap* _kvm, uint16_t _idx)
    {
        DM_CHECK(_idx < _kvm->max(), "kvmapGetKeyAt | %d, %d", _idx, _kvm->max());

        return _kvm->m_set.getValueAt(_idx);
    }

    template <typename Kvmap>
    DM_INLINE typename Kvmap::ValTy kvmapGetValueAt(Kvmap* _kvm, uint16_t _idx)
    {
        DM_CHECK(_idx < _kvm->max(), "kvmapGetKeyAt | %d, %d", _idx, _kvm->max());

        return _kvm->m_set.getValueAt(_idx);
    }

    template <typename Kvmap>
    DM_INLINE void kvmapRemove(Kvmap* _kvm, uint16_t _key)
    {
        DM_CHECK(_key < _kvm->max(), "kvmapRemove | %d, %d", _key, _kvm->max());

        _kvm->m_set.remove(_key);
    }

    template <typename Kvmap>
    DM_INLINE void kvmapReset(Kvmap* _kvm)
    {
        _kvm->m_set.reset();
    }

    template <typename Ty/*arithmetic type*/, uint16_t MaxKeyT>
    struct KeyValueMapT
    {
        typedef typename KeyValueMapT::Ty ValTy;

        void insert(uint16_t _key, Ty _value)
        {
            kvmapInsert(this, _key, _value);
        }

        bool contains(uint16_t _key)
        {
            return kvmapContains(this, _key);
        }

        Ty valueOf(uint16_t _key)
        {
            return kvmapValueOf(this, _key);
        }

        uint16_t getKeyAt(uint16_t _idx)
        {
            return kvmapGetKeyAt(this, _idx);
        }

        Ty getValueAt(uint16_t _idx)
        {
            return kvmapGetValueAt(this, _idx);
        }

        void remove(uint16_t _key)
        {
            kvmapRemove(this, _key);
        }

        void reset()
        {
            kvmapReset(this);
        }

        uint16_t count()
        {
            return m_set.count();
        }

        uint16_t max()
        {
            return MaxKeyT;
        }

    public:
        SetT<MaxKeyT> m_set;
        Ty m_values[MaxKeyT];
    };

    template <typename Ty/*arithmetic type*/>
    struct KeyValueMap
    {
        typedef typename KeyValueMap::Ty ValTy;

        // Uninitialized state, init() needs to be called !
        KeyValueMap()
        {
        }

        KeyValueMap(uint16_t _max)
        {
            init(_max);
        }

        KeyValueMap(uint16_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~KeyValueMap()
        {
            destroy();
        }

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return Set::sizeFor(_max) + _max*sizeof(Ty);
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_max = _max;
            m_memoryBlock = DM_ALLOC(sizeFor(_max));
            m_cleanup = true;

            void* ptr = m_set.init(_max, m_memoryBlock);
            m_values = (Ty*)ptr;
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_max = _max;
            m_memoryBlock = _mem;
            m_cleanup = false;

            void* ptr = m_set.init(_max, m_memoryBlock);
            m_values = (Ty*)ptr;

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        void destroy()
        {
            if (NULL != m_memoryBlock)
            {
                m_set.destroy();
                if (m_cleanup)
                {
                    DM_FREE(m_memoryBlock);
                }
                m_memoryBlock = NULL;
            }
        }

        void insert(uint16_t _key, Ty _value)
        {
            kvmapInsert(this, _key, _value);
        }

        bool contains(uint16_t _key)
        {
            return kvmapContains(this, _key);
        }

        Ty valueOf(uint16_t _key)
        {
            return kvmapValueOf(this, _key);
        }

        uint16_t getKeyAt(uint16_t _idx)
        {
            return kvmapGetKeyAt(this, _idx);
        }

        Ty getValueAt(uint16_t _idx)
        {
            return kvmapGetValueAt(this, _idx);
        }

        void remove(uint16_t _key)
        {
            kvmapRemove(this, _key);
        }

        void reset()
        {
            kvmapReset(this);
        }

        uint16_t count()
        {
            return m_set.count();
        }

        uint16_t max()
        {
            return m_max;
        }

    public:
        Set m_set;
        Ty* m_values;
        uint16_t m_max;
        bool m_cleanup;
        void* m_memoryBlock;
    };

    template <typename Ty/*arithmetic type*/>
    DM_INLINE KeyValueMap<Ty>* createKeyValueMap(uint16_t _max, void* _mem)
    {
        return ::new (_mem) KeyValueMap<Ty>(_max, (uint8_t*)_mem + sizeof(KeyValueMap<Ty>));
    }

    template <typename Ty/*arithmetic type*/>
    DM_INLINE KeyValueMap<Ty>* createKeyValueMap(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(KeyValueMap<Ty>) + KeyValueMap<Ty>::sizeFor(_max));
        return createKeyValueMap(_max, ptr);
    }

    template <typename Ty/*arithmetic type*/>
    DM_INLINE void destroyKeyValueMap(KeyValueMap<Ty>* _kvMap)
    {
        _kvMap->~KeyValueMapT<Ty>();
        delete _kvMap;
    }

} // namespace dm

#endif // DM_KVMAP_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
