/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

Ty* addNew()
{
    DM_CHECK(m_count < max(), "objarrayAddNew | %d, %d", m_count, max());

    return &m_values[m_count++];
}

uint32_t addObj(const Ty& _obj)
{
    DM_CHECK(m_count < max(), "objarrayAddObj | %d, %d", m_count, max());

    Ty* dst = &m_values[m_count++];
    dst = ::new (dst) Ty(_obj);

    return (m_count-1);
}

void remove(uint32_t _idx)
{
    DM_CHECK(0 < m_count && m_count <= max(), "objarrayRemove - 0 | %d, %d", m_count, max());
    DM_CHECK(_idx < max(), "objarrayRemove - 1 | %d, %d", _idx, max());

    Ty* elem = &m_values[_idx];
    Ty* next = &m_values[_idx+1];

    elem->~Ty();
    memmove(elem, next, (m_count-_idx-1)*sizeof(Ty));

    m_count--;
}

// Uses swap instead of memmove. Order is not preserved!
void removeSwap(uint32_t _idx)
{
    DM_CHECK(0 < m_count && m_count <= max(), "objarrayRemoveSwap - 0 | %d, %d", m_count, max());
    DM_CHECK(_idx < max(), "objarrayRemoveSwap - 1 | %d, %d", _idx, max());

    Ty* elem = &m_values[_idx];
    elem->~Ty();

    if (_idx != --m_count)
    {
        Ty* last = &m_values[m_count];
        elem->Ty(*last);
    }
}

void removeAll()
{
    for (uint32_t ii = m_count; ii--; )
    {
        Ty* obj = &m_values[_idx];
        obj->~Ty();
        BX_UNUSED(obj);
    }
    m_count = 0;
}

Ty* get(uint32_t _idx)
{
    DM_CHECK(_idx < max(), "objarrayGet | %d, %d", _idx, max());

    return &m_values[_idx];
}

const Ty* get(uint32_t _idx) const
{
    DM_CHECK(_idx < max(), "objarrayGet | %d, %d", _idx, max());

    return &m_values[_idx];
}

Ty& operator[](uint32_t _idx)
{
    DM_CHECK(_idx < max(), "objarray[] | %d, %d", _idx, max());

    return m_values[_idx];
}

const Ty& operator[](uint32_t _idx) const
{
    DM_CHECK(_idx < max(), "objarray[] const | %d, %d", _idx, max());

    return m_values[_idx];
}

void reset()
{
    m_count = 0;
}

/* vim: set sw=4 ts=4 expandtab: */
