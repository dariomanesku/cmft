/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_HANDLEALLOC_H_HEADER_GUARD
#define DM_HANDLEALLOC_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK

namespace dm
{
    // Adapted from: https://github.com/bkaradzic/bx/blob/master/include/bx/handlealloc.h

    template <typename HandleAlloc>
    DM_INLINE void handleAllocReset(HandleAlloc* _ha)
    {
        _ha->m_numHandles = 0;
        for (uint16_t ii = 0, end = _ha->max(); ii < end; ++ii)
        {
            _ha->m_handles[ii] = ii;
        }
    }

    template <typename HandleAlloc>
    DM_INLINE uint16_t handleAllocAlloc(HandleAlloc* _ha)
    {
        if (_ha->m_numHandles < _ha->max())
        {
            uint16_t index = _ha->m_numHandles;
            ++_ha->m_numHandles;

            uint16_t handle = _ha->m_handles[index];
            uint16_t* sparse = &_ha->m_handles[_ha->max()];
            sparse[handle] = index;
            return handle;
        }

        return HandleAlloc::Invalid;
    }

    template <typename HandleAlloc>
    DM_INLINE bool handleAllocContains(HandleAlloc* _ha, uint16_t _handle)
    {
        DM_CHECK(_handle < _ha->max(), "handleAllocContains | %d, %d", _handle, _ha->max());

        uint16_t* sparse = &_ha->m_handles[_ha->max()];
        uint16_t index = sparse[_handle];

        return (index < _ha->m_numHandles && _ha->m_handles[index] == _handle);
    }

    template <typename HandleAlloc>
    DM_INLINE void handleAllocFree(HandleAlloc* _ha, uint16_t _handle)
    {
        DM_CHECK(_ha->m_numHandles > 0, "handleAllocFree | %d", _ha->m_numHandles);

        uint16_t* sparse = &_ha->m_handles[_ha->max()];
        uint16_t index = sparse[_handle];
        --_ha->m_numHandles;
        uint16_t temp = _ha->m_handles[_ha->m_numHandles];
        _ha->m_handles[_ha->m_numHandles] = _handle;
        sparse[temp] = index;
        _ha->m_handles[index] = temp;
    }

    template <uint16_t MaxHandlesT>
    struct HandleAllocT
    {
        enum
        {
            Invalid = 0xffff,
        };

        HandleAllocT()
        {
            reset();
        }

        void reset()
        {
            return handleAllocReset(this);
        }

        uint16_t alloc()
        {
            return handleAllocAlloc(this);
        }

        bool contains(uint16_t _handle)
        {
            return handleAllocContains(this, _handle);
        }

        void free(uint16_t _handle)
        {
            handleAllocFree(this, _handle);
        }

        const uint16_t* getHandles() const
        {
            return m_handles;
        }

        uint16_t getHandleAt(uint16_t _idx) const
        {
            return m_handles[_idx];
        }

        uint16_t count() const
        {
            return m_numHandles;
        }

        uint16_t max() const
        {
            return MaxHandlesT;
        }

    public:
        uint16_t m_handles[MaxHandlesT*2];
        uint16_t m_numHandles;
    };

    struct HandleAlloc
    {
        enum
        {
            Invalid = 0xffff,
        };

        // Uninitialized state, init() needs to be called !
        HandleAlloc()
        {
        }

        HandleAlloc(uint16_t _max)
        {
            init(_max);
        }

        HandleAlloc(uint16_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~HandleAlloc()
        {
            destroy();
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_maxHandles = _max;
            m_handles = (uint16_t*)DM_ALLOC(sizeFor(_max));
            m_cleanup = true;

            reset();
        }

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return 2*_max*sizeof(uint16_t);
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_maxHandles = _max;
            m_handles = (uint16_t*)_mem;
            m_cleanup = false;

            reset();

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        void destroy()
        {
            if (m_cleanup && NULL != m_handles)
            {
                DM_FREE(m_handles);
                m_handles = NULL;
            }
        }

        void reset()
        {
            return handleAllocReset(this);
        }

        uint16_t alloc()
        {
            return handleAllocAlloc(this);
        }

        bool contains(uint16_t _handle)
        {
            return handleAllocContains(this, _handle);
        }

        void free(uint16_t _handle)
        {
            handleAllocFree(this, _handle);
        }

        const uint16_t* getHandles() const
        {
            return m_handles;
        }

        uint16_t getHandleAt(uint16_t _idx) const
        {
            return m_handles[_idx];
        }

        uint16_t count() const
        {
            return m_numHandles;
        }

        uint16_t max() const
        {
            return m_maxHandles;
        }

    public:
        uint16_t m_numHandles;
        uint16_t m_maxHandles;
        uint16_t* m_handles;
        bool m_cleanup;
    };

    DM_INLINE HandleAlloc* createHandleAlloc(uint16_t _max, void* _mem)
    {
        return ::new (_mem) HandleAlloc(_max, (uint8_t*)_mem + sizeof(HandleAlloc));
    }

    DM_INLINE HandleAlloc* createHandleAlloc(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(HandleAlloc) + HandleAlloc::sizeFor(_max));
        return createHandleAlloc(_max, ptr);
    }

    DM_INLINE void destroyHandleAlloc(HandleAlloc* _handleAlloc)
    {
        _handleAlloc->~HandleAlloc();
        delete _handleAlloc;
    }

} // namespace dm

#endif // DM_HANDLEALLOC_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
