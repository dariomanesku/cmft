/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifdef DM_DYNAMIC_ARRAY
    void resize(uint32_t _max)
    {
        if (m_cleanup) // 'm_values' was allocated internally.
        {
            m_values = (Ty*)BX_REALLOC(m_reallocator, m_values, sizeFor(_max));
            m_max = _max;
        }
        else // 'm_values' was passed as a pointer.
        {
            if (_max > m_max) // Expand.
            {
                m_values = (Ty*)BX_ALLOC(m_allocator, sizeFor(_max));
            }

            m_max = _max;
        }
    }

    private: void expandIfFull()
    {
        if (m_count >= m_max)
        {
            const uint32_t newMax = m_max + m_max/2;
            resize(newMax);
        }
    } public:

    void shrink()
    {
        resize(m_count);
    }
#endif //DM_DYNAMIC_ARRAY

void add(Ty _value)
{
    #ifdef DM_DYNAMIC_ARRAY
        expandIfFull();
    #endif //DM_DYNAMIC_ARRAY

    DM_CHECK(m_count < max(), "arrayAddVal | %d, %d", m_count, max());

    m_values[m_count++] = _value;
}

void cut(uint32_t _idx)
{
    DM_CHECK(_idx < max(), "arrayCut - 1 | %d, %d", _idx, max());

    m_count = _idx;
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

const Ty* elements() const
{
    return m_values;
}

void zero()
{
    memset(m_values, 0, max()*sizeof(Ty));
}

void fillWith(Ty _value)
{
    for (uint32_t ii = max(); ii--; )
    {
        m_values[ii] = _value;
    }
}

void reset()
{
    m_count = 0;
}

#undef DM_DYNAMIC_ARRAY

/* vim: set sw=4 ts=4 expandtab: */
