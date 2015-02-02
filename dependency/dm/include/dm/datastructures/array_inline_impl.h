/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

void add(Ty _value)
{
    DM_CHECK(m_count < max(), "arrayAddVal | %d, %d", m_count, max());

    m_values[m_count++] = _value;
}

uint32_t addObj(const Ty& _obj)
{
    DM_CHECK(m_count < max(), "arrayAddObj | %d, %d", m_count, max());

    Ty* dst = &m_values[m_count++];
    dst = ::new (dst) Ty(_obj);

    return (m_count-1);
}

Ty* addNew()
{
    DM_CHECK(m_count < max(), "arrayAddNew | %d, %d", m_count, max());

    return &m_values[m_count++];
}

void remove(uint32_t _idx)
{
    DM_CHECK(m_count <= max(), "arrayRemove - 0 | %d, %d", m_count, max());
    DM_CHECK(_idx < max(), "arrayRemove - 1 | %d, %d", _idx, max());

    Ty* elem = &m_values[_idx];
    Ty* next = &m_values[_idx+1];

    elem->~Ty();
    memmove(elem, next, (m_count-_idx-1)*sizeof(Ty));

    m_count--;
}

// Uses swap instead of memmove. Order is not preserved!
void removeSwap(uint32_t _idx)
{
    DM_CHECK(m_count <= max(), "arrayRemoveSwap - 0 | %d, %d", m_count, max());
    DM_CHECK(_idx < max(), "arrayRemoveSwap - 1 | %d, %d", _idx, max());

    Ty* elem = &m_values[_idx];
    Ty* last = &m_values[m_count-1];

    elem->~Ty();
    elem->Ty(*last);

    m_count--;
}

Ty getVal(uint32_t _idx) const
{
    DM_CHECK(_idx < max(), "arrayGetVal | %d, %d", _idx, max());

    return m_values[_idx];
}

Ty operator[](uint32_t _idx) const
{
    DM_CHECK(_idx < max(), "arrayGetVal | %d, %d", _idx, max());

    return m_values[_idx];
}

Ty* getPtr(uint32_t _idx)
{
    DM_CHECK(_idx < max(), "arrayGetRef | %d, %d", _idx, max());

    return &m_values[_idx];
}

const Ty* getPtr(uint32_t _idx) const
{
    DM_CHECK(_idx < max(), "arrayGetRef | %d, %d", _idx, max());

    return &m_values[_idx];
}

Ty& operator[](uint32_t _idx)
{
    DM_CHECK(_idx < max(), "arrayGetRef | %d, %d", _idx, max());

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
