/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

uint16_t alloc()
{
    DM_CHECK(m_numHandles < max(), "handleAllocAlloc | %d, %d", m_numHandles, max());

    if (m_numHandles < max())
    {
        const uint16_t index = m_numHandles++;
        const uint16_t handle = m_handles[index];
        uint16_t* sparse = &m_handles[max()];
        sparse[handle] = index;
        return handle;
    }

    return HandleAlloc::Invalid;
}

bool contains(uint16_t _handle)
{
    DM_CHECK(_handle < max(), "handleAllocContains | %d, %d", _handle, max());

    uint16_t* sparse = &m_handles[max()];
    uint16_t index = sparse[_handle];

    return (index < m_numHandles && m_handles[index] == _handle);
}

void free(uint16_t _handle)
{
    DM_CHECK(m_numHandles > 0, "handleAllocFree | %d", m_numHandles);

    uint16_t* sparse = &m_handles[max()];
    uint16_t index = sparse[_handle];

    if (index < m_numHandles && m_handles[index] == _handle)
    {
        --m_numHandles;
        uint16_t temp = m_handles[m_numHandles];
        m_handles[m_numHandles] = _handle;
        sparse[temp] = index;
        m_handles[index] = temp;
    }
}

const uint16_t* getHandles() const
{
    return m_handles;
}

uint16_t getHandleAt(uint16_t _idx) const
{
    return m_handles[_idx];
}

void reset()
{
    m_numHandles = 0;
    for (uint16_t ii = 0, end = max(); ii < end; ++ii)
    {
        m_handles[ii] = ii;
    }
}

/* vim: set sw=4 ts=4 expandtab: */
