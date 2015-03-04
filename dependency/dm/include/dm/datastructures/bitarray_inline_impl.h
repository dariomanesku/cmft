/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

void set(uint32_t _bit)
{
    DM_CHECK(_bit < max(), "bitArraySet | %d, %d", _bit, max());

    const uint32_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    m_bits[bucket] |= bit;
}

void unset(uint32_t _bit)
{
    DM_CHECK(_bit < max(), "bitArrayUnset | %d, %d", _bit, max());

    const uint32_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    m_bits[bucket] &= ~bit;
}

void toggle(uint32_t _bit)
{
    DM_CHECK(_bit < max(), "bitArrayToggle | %d, %d", _bit, max());

    const uint32_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    m_bits[bucket] ^= bit;
}

bool isSet(uint32_t _bit)
{
    DM_CHECK(_bit < max(), "bitArrayIsSet | %d, %d", _bit, max());

    const uint32_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    return (0 != (m_bits[bucket] & bit));
}

private:
inline uint32_t setRightmostBit(uint32_t _slot)
{
    const uint64_t rightmost = m_bits[_slot]+1;
    m_bits[_slot] |= rightmost;

    const uint32_t pos = uint32_t(bx::uint64_cnttz(rightmost));
    const uint32_t idx = (_slot<<6)+pos;
    const uint32_t max = this->max();
    return idx < max ? idx : max;
}
public:

uint32_t setFirst()
{
    for (uint32_t slot = 0, end = numSlots(); slot < end; ++slot)
    {
        const bool hasUnsetBits = (m_bits[slot] != UINT64_MAX);
        if (hasUnsetBits)
        {
            return setRightmostBit(slot);
        }
    }

    return max();
}

uint32_t setAny()
{
    const uint32_t begin = m_last;
    const uint32_t count = numSlots();

    for (uint32_t slot = begin, end = numSlots(); slot < end; ++slot)
    {
        const bool hasUnsetBits = (m_bits[slot] != UINT64_MAX);
        if (hasUnsetBits)
        {
            return setRightmostBit(slot);
        }
        else
        {
            m_last = slot+1;
        }
    }

    m_last = (m_last >= count) ? 0 : m_last;

    for (uint32_t slot = 0, end = begin; slot < end; ++slot)
    {
        const bool hasUnsetBits = (m_bits[slot] != UINT64_MAX);
        if (hasUnsetBits)
        {
            return setRightmostBit(slot);
        }
        else
        {
            m_last = slot+1;
        }
    }

    return max();
}

/// Returns max() if none set.
uint32_t getFirstSetBit()
{
    for (uint32_t ii = 0, end = numSlots(); ii < end; ++ii)
    {
        const bool hasSetBits = (0 != m_bits[ii]);
        if (hasSetBits)
        {
            const uint32_t pos = uint32_t(bx::uint64_cnttz(m_bits[ii]));
            return (ii<<6)+pos;
        }
    }

    return max();
}

uint32_t getFirstUnsetBit()
{
    for (uint32_t ii = 0, end = numSlots(); ii < end; ++ii)
    {
        const bool hasUnsetBits = (m_bits[ii] != UINT64_MAX);
        if (hasUnsetBits)
        {
            const uint64_t sel = markFirstUnsetBit(m_bits[ii]);
            const uint32_t pos = uint32_t(bx::uint64_cnttz(sel));
            return (ii<<6)+pos;
        }
    }

    return max();
}

/// Returns max() if none set.
uint32_t getLastSetBit()
{
    for (uint32_t ii = numSlots(); ii--; )
    {
        const bool hasSetBits = (0 != m_bits[ii]);
        if (hasSetBits)
        {
            const bool allSet = (UINT64_MAX == m_bits[ii]);
            if (allSet)
            {
                return (ii+1)<<6;
            }
            else
            {
                const uint64_t sel = markFirstUnsetBit(m_bits[ii]);
                const uint64_t leading = bx::uint64_cntlz(sel);
                const uint32_t pos = 63-uint32_t(leading);
                return ((ii)<<6)+pos;
            }
        }
    }

    return 0;
}

uint32_t getLastUnsetBit()
{
    for (uint32_t ii = numSlots(); ii--; )
    {
        const bool hasSetBits = (0 != m_bits[ii]);
        if (hasSetBits)
        {
            const uint64_t leading = bx::uint64_cntlz(m_bits[ii]);
            const uint32_t pos = 63-uint32_t(leading);
            return (ii<<6)+pos;
        }
    }

    return max();
}

uint32_t count()
{
    uint64_t count = 0;
    for (uint32_t ii = numSlots(); ii--; )
    {
        const uint64_t curr = m_bits[ii];
        count += bx::uint64_cntbits(curr);
    }

    return uint32_t(count);
}

void reset()
{
    m_last = 0;
    memset(m_bits, 0, numSlots()*sizeof(uint64_t));
}

/* vim: set sw=4 ts=4 expandtab: */
