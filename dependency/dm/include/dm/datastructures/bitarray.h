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

    template <typename BitArrayTy>
    DM_INLINE void bitArraySet(BitArrayTy* _ba, uint16_t _bit)
    {
        DM_CHECK(_bit < _ba->m_max, "bitArraySet | %d, %d", _bit, _ba->m_max);

        const uint16_t bucket = _bit>>6;
        const uint64_t bit    = UINT64_C(1)<<(_bit&63);
        _ba->m_bits[bucket] |= bit;
    }

    template <typename BitArrayTy>
    DM_INLINE void bitArrayUnset(BitArrayTy* _ba, uint16_t _bit)
    {
        DM_CHECK(_bit < _ba->max(), "bitArrayUnset | %d, %d", _bit, _ba->max());

        const uint16_t bucket = _bit>>6;
        const uint64_t bit    = UINT64_C(1)<<(_bit&63);
        _ba->m_bits[bucket] &= ~bit;
    }

    template <typename BitArrayTy>
    DM_INLINE void bitArrayToggle(BitArrayTy* _ba, uint16_t _bit)
    {
        DM_CHECK(_bit < _ba->max(), "bitArrayToggle | %d, %d", _bit, _ba->max());

        const uint16_t bucket = _bit>>6;
        const uint64_t bit    = UINT64_C(1)<<(_bit&63);
        _ba->m_bits[bucket] ^= bit;
    }

    template <typename BitArrayTy>
    DM_INLINE bool bitArrayIsSet(BitArrayTy* _ba, uint16_t _bit)
    {
        DM_CHECK(_bit < _ba->max(), "bitArrayIsSet | %d, %d", _bit, _ba->max());

        const uint16_t bucket = _bit>>6;
        const uint64_t bit    = UINT64_C(1)<<(_bit&63);
        return (0 != (_ba->m_bits[bucket] & bit));
    }

    template <typename BitArrayTy>
    DM_INLINE uint16_t bitArraySetFirst(BitArrayTy* _ba)
    {
        for (uint16_t ii = 0, end = _ba->numSlots(); ii < end; ++ii)
        {
            const bool hasUnsetBits = (_ba->m_bits[ii] != UINT64_MAX);
            if (hasUnsetBits)
            {
                // Set the rightmost bit.
                const uint64_t rightmost = _ba->m_bits[ii]+1;
                _ba->m_bits[ii] |= rightmost;

                const uint16_t pos = uint16_t(bx::uint64_cnttz(rightmost));
                const uint16_t idx = (ii<<6)+pos;
                const uint16_t max = _ba->max();
                return idx < max ? idx : max;
            }
        }

        return _ba->max();
    }

    template <typename BitArrayTy>
    DM_INLINE uint16_t bitArrayGetFirstSetBit(BitArrayTy* _ba)
    {
        for (uint16_t ii = 0, end = _ba->numSlots(); ii < end; ++ii)
        {
            const bool hasSetBits = (0 != _ba->m_bits[ii]);
            if (hasSetBits)
            {
                const uint16_t pos = uint16_t(bx::uint64_cnttz(_ba->m_bits[ii]));
                return (ii<<6)+pos;
            }
        }

        return _ba->max();
    }

    template <typename BitArrayTy>
    DM_INLINE uint16_t bitArrayGetFirstUnsetBit(BitArrayTy* _ba)
    {
        for (uint16_t ii = 0, end = _ba->numSlots(); ii < end; ++ii)
        {
            const bool hasUnsetBits = (_ba->m_bits[ii] != UINT64_MAX);
            if (hasUnsetBits)
            {
                const uint64_t sel = markFirstUnsetBit(_ba->m_bits[ii]);
                const uint16_t pos = uint16_t(bx::uint64_cnttz(sel));
                return (ii<<6)+pos;
            }
        }

        return _ba->max();
    }

    template <typename BitArrayTy>
    DM_INLINE uint16_t bitArrayGetLastSetBit(BitArrayTy* _ba)
    {
        for (uint16_t ii = _ba->numSlots(); ii--; )
        {
            const bool hasSetBits = (0 != _ba->m_bits[ii]);
            if (hasSetBits)
            {
                const bool allSet = (UINT64_MAX == _ba->m_bits[ii]);
                if (allSet)
                {
                    return (ii+1)<<6;
                }
                else
                {
                    const uint64_t sel = markFirstUnsetBit(_ba->m_bits[ii]);
                    const uint64_t leading = bx::uint64_cntlz(sel);
                    const uint16_t pos = 63-uint16_t(leading);
                    return ((ii)<<6)+pos;
                }
            }
        }

        return 0;
    }

    template <typename BitArrayTy>
    DM_INLINE uint16_t bitArrayGetLastUnsetBit(BitArrayTy* _ba)
    {
        for (uint16_t ii = _ba->numSlots(); ii--; )
        {
            const bool hasSetBits = (0 != _ba->m_bits[ii]);
            if (hasSetBits)
            {
                const uint64_t leading = bx::uint64_cntlz(_ba->m_bits[ii]);
                const uint16_t pos = 63-uint16_t(leading);
                return (ii<<6)+pos;
            }
        }

        return _ba->max();
    }

    template <typename BitArrayTy>
    DM_INLINE uint16_t bitArrayCount(BitArrayTy* _ba)
    {
        uint64_t count = 0;
        for (uint16_t ii = _ba->numSlots(); ii--; )
        {
            const uint64_t curr = _ba->m_bits[ii];
            count += bx::uint64_cntbits(curr);
        }

        return uint16_t(count);
    }

    template <typename BitArrayTy>
    DM_INLINE void bitArrayReset(BitArrayTy* _ba)
    {
        memset(_ba->m_bits, 0, sizeof(_ba->m_bits));
    }

    template <uint16_t MaxT>
    struct BitArrayT
    {
        BitArrayT()
        {
            reset();
        }

        void set(uint16_t _bit)
        {
            bitArraySet(this, _bit);
        };

        void unset(uint16_t _bit)
        {
            bitArrayUnset(this, _bit);
        };

        void toggle(uint16_t _bit)
        {
            bitArrayToggle(this, _bit);
        };

        bool isSet(uint16_t _bit)
        {
            return bitArrayIsSet(this, _bit);
        }

        uint16_t setFirst()
        {
            return bitArraySetFirst(this);
        }

        /// Returns max() if none set.
        uint16_t getFirstSetBit()
        {
            return bitArrayGetFirstSetBit(this);
        }

        uint16_t getFirstUnsetBit()
        {
            return bitArrayGetFirstUnsetBit(this);
        }

        /// Returns max() if none set.
        uint16_t getLastSetBit()
        {
            return bitArrayGetLastSetBit(this);
        }

        uint16_t getLastUnsetBit()
        {
            return bitArrayGetLastUnsetBit(this);
        }

        uint16_t count()
        {
            return bitArrayCount(this);
        }

        void reset()
        {
            bitArrayReset(this);
        }

        uint16_t max() const
        {
            return MaxT;
        }

        uint16_t numSlots() const
        {
            return NumSlots;
        }

    public:
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

        void set(uint16_t _bit)
        {
            bitArraySet(this, _bit);
        };

        void unset(uint16_t _bit)
        {
            bitArrayUnset(this, _bit);
        };

        void toggle(uint16_t _bit)
        {
            bitArrayToggle(this, _bit);
        };

        bool isSet(uint16_t _bit)
        {
            return bitArrayIsSet(this, _bit);
        }

        uint16_t setFirst()
        {
            return bitArraySetFirst(this);
        }

        /// Returns max() if none set.
        uint16_t getFirstSetBit()
        {
            return bitArrayGetFirstSetBit(this);
        }

        uint16_t getFirstUnsetBit()
        {
            return bitArrayGetFirstUnsetBit(this);
        }

        /// Returns max() if none set.
        uint16_t getLastSetBit()
        {
            return bitArrayGetLastSetBit(this);
        }

        uint16_t getLastUnsetBit()
        {
            return bitArrayGetLastUnsetBit(this);
        }

        uint16_t count()
        {
            return bitArrayCount(this);
        }

        uint16_t max() const
        {
            return m_max;
        }

        uint16_t numSlots() const
        {
            return m_numSlots;
        }

        void reset()
        {
            bitArrayReset(this);
        }

    public:
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
