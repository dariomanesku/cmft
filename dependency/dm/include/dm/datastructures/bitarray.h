/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_BITARRAY_H_HEADER_GUARD
#define DM_BITARRAY_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <new> // placement-new

#include "allocator.h" // DM_ALLOC()/DM_FREE()
#include "../common/common.h" // DM_INLINE
#include "../check.h" // DM_CHECK
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

    template <uint16_t MaxT>
    struct BitArrayT
    {
        BitArrayT()
        {
            reset();
        }

        #include "bitarray_inline_impl.h"

        uint16_t max() const
        {
            return MaxT;
        }

        uint16_t numSlots() const
        {
            return NumSlots;
        }

    private:
        enum { NumSlots = ((MaxT-1)>>6)+1 };
        uint64_t m_bits[NumSlots];
    };

    struct BitArray
    {
        // Uninitialized state, init() needs to be called !
        BitArray()
        {
        }

        BitArray(uint16_t _max)
        {
            init(_max);
        }

        BitArray(uint16_t _max, void* _mem)
        {
            init(_max, _mem);
        }

        ~BitArray()
        {
            destroy();
        }

        static inline uint16_t numSlotsFor(uint16_t _max)
        {
            return ((_max-1)>>6) + 1;
        }

        // Allocates memory internally.
        void init(uint16_t _max)
        {
            m_max = _max;
            m_numSlots = numSlotsFor(_max);
            m_bits = (uint64_t*)DM_ALLOC(sizeFor(_max));
            m_cleanup = true;

            reset();
        }

        static inline uint32_t sizeFor(uint16_t _max)
        {
            return numSlotsFor(_max)*sizeof(uint64_t);
        }

        // Uses externaly allocated memory.
        void* init(uint16_t _max, void* _mem)
        {
            m_max = _max;
            m_numSlots = numSlotsFor(_max);
            m_bits = (uint64_t*)_mem;
            m_cleanup = false;

            reset();

            void* end = (void*)((uint8_t*)_mem + sizeFor(_max));
            return end;
        }

        void destroy()
        {
            if (m_cleanup && NULL != m_bits)
            {
                DM_FREE(m_bits);
                m_bits = NULL;
            }
        }

        #include "bitarray_inline_impl.h"

        uint16_t max() const
        {
            return m_max;
        }

        uint16_t numSlots() const
        {
            return m_numSlots;
        }

    private:
        uint16_t m_max;
        uint16_t m_numSlots;
        uint64_t* m_bits;
        bool m_cleanup;
    };

    DM_INLINE BitArray* createBitArray(uint16_t _max, void* _mem)
    {
        return ::new (_mem) BitArray(_max, (uint8_t*)_mem + sizeof(BitArray));
    }

    DM_INLINE BitArray* createBitArray(uint16_t _max)
    {
        uint8_t* ptr = (uint8_t*)DM_ALLOC(sizeof(BitArray) + BitArray::sizeFor(_max));
        return createBitArray(_max, ptr);
    }

    DM_INLINE void destroyBitArray(BitArray* _bitArray)
    {
        _bitArray->~BitArray();
        delete _bitArray;
    }

} // namespace dm

#endif // DM_BITARRAY_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
