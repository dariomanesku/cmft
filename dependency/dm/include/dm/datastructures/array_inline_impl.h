/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

void add(Ty _value)
{
    DM_CHECK(m_count < max(), "arrayAddVal | %d, %d", m_count, max());

    m_values[m_count++] = _value;
}

void remove(uint32_t _idx)
{
    DM_CHECK(0 < m_count && m_count <= max(), "arrayRemove - 0 | %d, %d", m_count, max());
    DM_CHECK(_idx < max(), "arrayRemove - 1 | %d, %d", _idx, max());

    Ty* elem = &m_values[_idx];
    Ty* next = &m_values[_idx+1];

    memmove(elem, next, (m_count-_idx-1)*sizeof(Ty));

    m_count--;
}

Ty pop()
{
    DM_CHECK(0 < m_count, "arrayPop | %d", m_count);

    return m_values[--m_count];
}

// Uses swap instead of memmove. Order is not preserved!
void removeSwap(uint32_t _idx)
{
    DM_CHECK(0 < m_count && m_count <= max(), "arrayRemoveSwap - 0 | %d, %d", m_count, max());
    DM_CHECK(_idx < max(), "arrayRemoveSwap - 1 | %d, %d", _idx, max());

    m_values[_idx] = m_values[--m_count];
}

Ty get(uint32_t _idx) const
{
    DM_CHECK(_idx < max(), "arrayGet | %d, %d", _idx, max());

    return m_values[_idx];
}

Ty operator[](uint32_t _idx) const
{
    DM_CHECK(_idx < max(), "array[] const | %d, %d", _idx, max());

    return m_values[_idx];
}

Ty& operator[](uint32_t _idx)
{
    DM_CHECK(_idx < max(), "array[] ref | %d, %d", _idx, max());

    return m_values[_idx];
}

void zero()
{
    memset(m_values, 0, max()*sizeof(Ty));
}

void reset()
{
    m_count = 0;
}

/* vim: set sw=4 ts=4 expandtab: */
