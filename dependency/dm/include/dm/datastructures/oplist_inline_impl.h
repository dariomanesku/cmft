/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

Ty* addNew()
{
    const uint16_t handle = m_handleAlloc.alloc();
    m_handles.add(handle);

    Ty* obj = &m_objects[handle];
    obj = ::new (obj) Ty();

    return obj;
}

uint16_t add(const Ty& _obj)
{
    const uint16_t handle = m_handleAlloc.alloc();
    m_handles.add(handle);

    Ty* obj = &m_objects[handle];
    obj = ::new (obj) Ty(_obj);

    return obj;
}

void remove(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "oplistRemove | %d, %d", _idx, max());

    const uint16_t handle = m_handles[_idx];
    m_handles.remove(_idx);

    m_handleAlloc.free(handle);
    m_objects[handle].~Ty();
}

void removeAll()
{
    for (uint16_t ii = count(); ii--; )
    {
        m_objects[ii].~Ty();
    }
    m_handleAlloc.reset();
    m_handles.reset();
}

Ty* get(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "oplistGet | %d, %d", _idx, max());

    const uint16_t handle = m_handles[_idx];
    return &m_objects[handle];
}

const Ty* get(uint16_t _idx) const
{
    DM_CHECK(_idx < max(), "oplistGet const | %d, %d", _idx, max());

    const uint16_t handle = m_handles[_idx];
    return &m_objects[handle];
}

Ty& operator[](uint16_t _idx)
{
    DM_CHECK(_idx < max(), "oplist[] | %d, %d", _idx, max());

    const uint16_t handle = m_handles[_idx];
    return m_objects[handle];
}

const Ty& operator[](uint16_t _idx) const
{
    DM_CHECK(_idx < max(), "oplist[] const | %d, %d", _idx, max());

    const uint16_t handle = m_handles[_idx];
    return m_objects[handle];
}

uint16_t getHandleAt(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "oplistGetHandleAt | %d, %d", _idx, max());

    return m_handles[_idx];
}

uint16_t getHandleOf(const Ty* _obj) const
{
    DM_CHECK(&m_objects[0] <= _obj && _obj < &m_objects[max()], "opListGetHandleOf | Object not from the list.");

    return uint16_t(_obj - m_objects);
}

Ty* getFromHandle(uint16_t _handle)
{
    DM_CHECK(_handle < max(), "oplistGetFromHandle | %d, %d", _handle, max());

    return &m_objects[_handle];
}

uint16_t count()
{
    return m_handleAlloc.count();
}

/* vim: set sw=4 ts=4 expandtab: */

