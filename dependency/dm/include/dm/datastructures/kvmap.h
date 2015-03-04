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
    template <typename Ty/*arithmetic type*/, uint16_t MaxKeyT>
    struct KeyValueMapT
    {
        KeyValueMapT()
        {
        }

        #include "kvmap_inline_impl.h"

        uint16_t count()
        {
            return m_set.count();
        }

        uint16_t max()
        {
            return MaxKeyT;
        }

    private:
        SetT<MaxKeyT> m_set;
        Ty m_values[MaxKeyT];
    };

    template <typename Ty/*arithmetic type*/>
    struct KeyValueMap
    {
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

        #include "kvmap_inline_impl.h"

        uint16_t count()
        {
            return m_set.count();
        }

        uint16_t max()
        {
            return m_max;
        }

    private:
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
        return createKeyValueMap<Ty>(_max, ptr);
    }

    template <typename Ty/*arithmetic type*/>
    DM_INLINE void destroyKeyValueMap(KeyValueMap<Ty>* _kvMap)
    {
        _kvMap->~KeyValueMap<Ty>();
        delete _kvMap;
    }

} // namespace dm

#endif // DM_KVMAP_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
