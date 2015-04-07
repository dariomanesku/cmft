/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_BITARRAY_H_HEADER_GUARD
#define DM_BITARRAY_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new>      // placement-new

#include "../common/common.h"              // DM_INLINE
#include "../check.h"                      // DM_CHECK
#include "../../../3rdparty/bx/uint32_t.h" // bx::uint64_cnttz(), bx::uint64_cntlz(), bx::uint64_cntbits()

namespace dm
{
    DM_INLINE uint64_t markFirstUnsetBit(uint64_t _v)
    {
        const uint64_t val = _v;      // ..010111
        const uint64_t neg = ~val;    // ..101000
        const uint64_t add = val+1;   // ..011000
        const uint64_t bit = neg&add; // ..001000

        return bit;
    }

    template <uint32_t MaxT>
    struct BitArrayT
    {
        BitArrayT()
        {
            reset();
        }

        #include "bitarray_inline_impl.h"

        uint32_t max() const
        {
            return MaxT;
        }

        uint32_t numSlots() const
        {
            return NumSlots;
        }

    private:
        enum { NumSlots = ((MaxT-1)>>6)+1 };
        uint32_t m_last;
        uint64_t m_bits[NumSlots];
    };

    struct BitArray
    {
        // Uninitialized state, init() needs to be called !
        BitArray()
        {
            m_bits = NULL;
        }

        BitArray(uint32_t _max, bx::ReallocatorI* _reallocator)
        {
            init(_max, _reallocator);
        }

        BitArray(uint32_t _max, void* _mem, bx::AllocatorI* _allocator)
        {
            init(_max, _mem, _allocator);
        }

        ~BitArray()
        {
            destroy();
        }

        static inline uint32_t numSlotsFor(uint32_t _max)
        {
            return ((_max-1)>>6) + 1;
        }

        // Allocates memory internally.
        void init(uint32_t _max, bx::ReallocatorI* _reallocator)
        {
            m_max = _max;
            m_numSlots = numSlotsFor(_max);
            m_bits = (uint64_t*)BX_ALLOC(_reallocator, sizeFor(_max));
            m_reallocator = _reallocator;
            m_cleanup = true;

            reset();
        }

        static inline uint32_t sizeFor(uint32_t _max)
        {
            return numSlotsFor(_max)*sizeof(uint64_t);
        }

        // Uses externally allocated memory.
        void* init(uint32_t _max, void* _mem, bx::AllocatorI* _allocator = NULL)
        {
            m_max = _max;
            m_numSlots = numSlotsFor(_max);
            m_bits = (uint64_t*)_mem;
            m_allocator = _allocator;
            m_cleanup = false;

            reset();

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        bool isInitialized() const
        {
            return (NULL != m_bits);
        }

        void destroy()
        {
            if (m_cleanup && NULL != m_bits)
            {
                BX_FREE(m_reallocator, m_bits);
                m_bits = NULL;
            }
        }

        #include "bitarray_inline_impl.h"

        uint32_t max() const
        {
            return m_max;
        }

        uint32_t numSlots() const
        {
            return m_numSlots;
        }

        bx::AllocatorI* allocator()
        {
            return m_allocator;
        }

    private:
        uint32_t m_last;
        uint32_t m_max;
        uint32_t m_numSlots;
        uint64_t* m_bits;
        union
        {
            bx::AllocatorI*   m_allocator;
            bx::ReallocatorI* m_reallocator;
        };
        bool m_cleanup;
    };

    DM_INLINE BitArray* createBitArray(uint32_t _max, void* _mem, bx::AllocatorI* _allocator)
    {
        return ::new (_mem) BitArray(_max, (uint8_t*)_mem + sizeof(BitArray), _allocator);
    }

    DM_INLINE BitArray* createBitArray(uint32_t _max, bx::AllocatorI* _allocator)
    {
        uint8_t* ptr = (uint8_t*)BX_ALLOC(_allocator, sizeof(BitArray) + BitArray::sizeFor(_max));
        return createBitArray(_max, ptr, _allocator);
    }

    DM_INLINE void destroyBitArray(BitArray* _bitArray)
    {
        _bitArray->~BitArray();
        BX_FREE(_bitArray->allocator(), _bitArray);
    }

} // namespace dm

#endif // DM_BITARRAY_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
