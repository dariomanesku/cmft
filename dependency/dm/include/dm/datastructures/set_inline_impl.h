/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */
uint16_t insert(uint16_t _val)
{
    DM_CHECK(m_num < max(), "setInsert - 0 | %d, %d", m_num, max());
    DM_CHECK(_val < max(),  "setInsert - 1 | %d, %d", _val,  max());

    if (this->contains(_val))
    {
        return this->indexOf(_val);
    }

    m_values[m_num] = _val;
    uint16_t* sparse = &m_values[max()];
    sparse[_val] = m_num;

    const uint16_t index = m_num;
    ++m_num;

    return index;
}

uint16_t safeInsert(uint16_t _val)
{
    if (_val < max())
    {
        return this->insert(_val);
    }

    return UINT16_MAX;
}

bool contains(uint16_t _val)
{
    DM_CHECK(_val < max(), "setContains - 0 | %d, %d", _val, max());

    const uint16_t* sparse = &m_values[max()];
    const uint16_t index = sparse[_val];

    return (index < m_num && m_values[index] == _val);
}

uint16_t indexOf(uint16_t _val)
{
    DM_CHECK(_val < max(), "setIndexOf | %d, %d", _val, max());

    const uint16_t* sparse = &m_values[max()];
    return sparse[_val];
}

uint16_t getValueAt(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "setGetValueAt | %d, %d", _idx, max());

    return m_values[_idx];
}

void remove(uint16_t _val)
{
    DM_CHECK(_val < max(), "setRemove - 0 | %d, %d", _val, max());
    DM_CHECK(m_num < max(), "setRemove - 1 | %d, %d", m_num, max());

    if (!this->contains(_val))
    {
        return;
    }

    uint16_t* sparse = &m_values[max()];

    const uint16_t index = sparse[_val];
    const uint16_t last = m_values[--m_num];

    DM_CHECK(index < max(), "setRemove - 2 | %d, %d", index, max());
    DM_CHECK(last < max(), "setRemove - 3 | %d, %d", last, max());

    m_values[index] = last;
    sparse[last] = index;
}

void reset()
{
    m_num = 0;
}

/* vim: set sw=4 ts=4 expandtab: */
