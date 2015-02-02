/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

enum { Invalid = UINT16_MAX };

void fillWith(const Ty* _obj)
{
    Ty* elem = m_elements;
    for (uint16_t ii = count(); ii--; )
    {
        dst = ::new (&elem[ii]) Ty(*_obj);
    }
}

uint16_t add(const Ty& _obj)
{
    const uint16_t idx = m_handles.alloc();

    Ty* dst = &m_elements[idx];
    dst = ::new (dst) Ty(_obj);
    return idx;
}

Ty* addNew()
{
    const uint16_t idx = m_handles.alloc();
    DM_CHECK(idx < max(), "listAddNew | %d, %d", idx, max());

    Ty* dst = &m_elements[idx];
    dst = ::new (dst) Ty();
    return dst;
}

bool contains(uint16_t _handle)
{
    return m_handles.contains(_handle);
}

uint16_t getHandleOf(const Ty* _obj) const
{
    DM_CHECK(&m_elements[0] <= _obj && _obj < &m_elements[max()], "listGetHandleOf | Object not from the list.");

    return uint16_t(_obj - m_elements);
}

uint16_t getHandleAt(uint16_t _idx) const
{
    return m_handles.getHandleAt(_idx);
}

Ty* getObjFromHandle(uint16_t _handle)
{
    DM_CHECK(_handle < max(), "listGetObjFromHandle | %d, %d", _handle, max());

    return const_cast<Ty*>(&m_elements[_handle]);
}

private:
Ty* getObjAtImpl(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "listGetObjAt | %d, %d", _idx, max());

    const uint16_t handle = m_handles.getHandleAt(_idx);
    return this->getObjFromHandle(handle);
}
public:

Ty* getObjAt(uint16_t _idx)
{
    This* list = const_cast<This*>(this);
    return list->getObjAtImpl(_idx);
}

const Ty* getObjAt(uint16_t _idx) const
{
    This* list = const_cast<This*>(this);
    return list->getObjAtImpl(_idx);
}

Ty& operator[](uint16_t _idx)
{
    This* list = const_cast<This*>(this);
    return *list->getObjAtImpl(_idx);
}

const Ty& operator[](uint16_t _idx) const
{
    This* list = const_cast<This*>(this);
    return *list->getObjAtImpl(_idx);
}

void remove(uint16_t _handle)
{
    DM_CHECK(_handle < max(), "listRemove | %d, %d", _handle, max());

    m_elements[_handle].~Ty();

    m_handles.free(_handle);
}

void removeAt(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "listRemoveAt | %d, %d", _idx, max());

    const uint16_t handle = m_handles.getHandleAt(_idx);
    this->remove(handle);
}

void removeAll()
{
    for (uint16_t ii = count(); ii--; )
    {
        this->removeAt(0);
    }
}

/* vim: set sw=4 ts=4 expandtab: */
