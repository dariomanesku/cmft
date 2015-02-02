/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

void insert(uint16_t _key, Ty _value)
{
    DM_CHECK(_key < max(), "kvmapInsert - 0 | %d, %d", _key, max());

    if (_key < max())
    {
        const uint16_t index = m_set.insert(_key);

        DM_CHECK(index < max(), "kvmapInsert - 1 | %d, %d", _key, max());
        m_values[index] = _value;
    }
}

bool contains(uint16_t _key)
{
    DM_CHECK(_key < max(), "kvmapContains | %d, %d", _key, max());

    return m_set.contains(_key);
}

Ty valueOf(uint16_t _key)
{
    DM_CHECK(m_set.indexOf(_key) < max(), "kvmapValueOf | %d, %d, %d", _key, m_set.indexOf(_key), max());

    return m_values[m_set.indexOf(_key)];
}

uint16_t getKeyAt(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "kvmapGetKeyAt | %d, %d", _idx, max());

    return m_set.getValueAt(_idx);
}

Ty getValueAt(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "kvmapGetValueAt | %d, %d", _idx, max());

    return this->valueOf(this->getKeyAt(_idx));
}

void remove(uint16_t _key)
{
    DM_CHECK(_key < max(), "kvmapRemove | %d, %d", _key, max());

    remove(_key);
}

void reset()
{
    m_set.reset();
}

/* vim: set sw=4 ts=4 expandtab: */
