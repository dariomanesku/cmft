/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

void set(uint16_t _bit)
{
    DM_CHECK(_bit < m_max, "bitArraySet | %d, %d", _bit, m_max);

    const uint16_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    m_bits[bucket] |= bit;
}

void unset(uint16_t _bit)
{
    DM_CHECK(_bit < max(), "bitArrayUnset | %d, %d", _bit, max());

    const uint16_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    m_bits[bucket] &= ~bit;
}

void toggle(uint16_t _bit)
{
    DM_CHECK(_bit < max(), "bitArrayToggle | %d, %d", _bit, max());

    const uint16_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    m_bits[bucket] ^= bit;
}

bool isSet(uint16_t _bit)
{
    DM_CHECK(_bit < max(), "bitArrayIsSet | %d, %d", _bit, max());

    const uint16_t bucket = _bit>>6;
    const uint64_t bit    = UINT64_C(1)<<(_bit&63);
    return (0 != (m_bits[bucket] & bit));
}

uint16_t setFirst()
{
    for (uint16_t ii = 0, end = numSlots(); ii < end; ++ii)
    {
        const bool hasUnsetBits = (m_bits[ii] != UINT64_MAX);
        if (hasUnsetBits)
        {
            // Set the rightmost bit.
            const uint64_t rightmost = m_bits[ii]+1;
            m_bits[ii] |= rightmost;

            const uint16_t pos = uint16_t(bx::uint64_cnttz(rightmost));
            const uint16_t idx = (ii<<6)+pos;
            const uint16_t max = this->max();
            return idx < max ? idx : max;
        }
    }

    return max();
}

/// Returns max() if none set.
uint16_t getFirstSetBit()
{
    for (uint16_t ii = 0, end = numSlots(); ii < end; ++ii)
    {
        const bool hasSetBits = (0 != m_bits[ii]);
        if (hasSetBits)
        {
            const uint16_t pos = uint16_t(bx::uint64_cnttz(m_bits[ii]));
            return (ii<<6)+pos;
        }
    }

    return max();
}

uint16_t getFirstUnsetBit()
{
    for (uint16_t ii = 0, end = numSlots(); ii < end; ++ii)
    {
        const bool hasUnsetBits = (m_bits[ii] != UINT64_MAX);
        if (hasUnsetBits)
        {
            const uint64_t sel = markFirstUnsetBit(m_bits[ii]);
            const uint16_t pos = uint16_t(bx::uint64_cnttz(sel));
            return (ii<<6)+pos;
        }
    }

    return max();
}

/// Returns max() if none set.
uint16_t getLastSetBit()
{
    for (uint16_t ii = numSlots(); ii--; )
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
                const uint16_t pos = 63-uint16_t(leading);
                return ((ii)<<6)+pos;
            }
        }
    }

    return 0;
}

uint16_t getLastUnsetBit()
{
    for (uint16_t ii = numSlots(); ii--; )
    {
        const bool hasSetBits = (0 != m_bits[ii]);
        if (hasSetBits)
        {
            const uint64_t leading = bx::uint64_cntlz(m_bits[ii]);
            const uint16_t pos = 63-uint16_t(leading);
            return (ii<<6)+pos;
        }
    }

    return max();
}

uint16_t count()
{
    uint64_t count = 0;
    for (uint16_t ii = numSlots(); ii--; )
    {
        const uint64_t curr = m_bits[ii];
        count += bx::uint64_cntbits(curr);
    }

    return uint16_t(count);
}

void reset()
{
    memset(m_bits, 0, sizeof(m_bits));
}

/* vim: set sw=4 ts=4 expandtab: */
