/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

Ty* addNew()
{
    Ty* obj = Base::addNew();
    const uint16_t handle = this->getHandleOf(obj);
    m_handleArray.add(handle);
    return obj;
}

Ty* getObjFromIdx(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "olGetObjFromIdx | %d, %d", _idx, max());

    const uint16_t handle = m_handleArray.getVal(_idx);
    return this->getObjFromHandle(handle);
}

// Avoid using this, it's O(n).
uint16_t getIdxFromHandle(uint16_t _handle)
{
    for (uint16_t ii = 0, end = this->count(); ii < end; ++ii)
    {
        if (m_handleArray.getVal(ii) == _handle)
        {
            return ii;
        }
    }

    return Invalid;
}

uint16_t getHandleFromIdx(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "olGetHandleFromIdx | %d, %d", _idx, max());

    return m_handleArray.getVal(_idx);
}

void remove(uint16_t _idx)
{
    DM_CHECK(_idx < max(), "olRemove | %d, %d", _idx, max());

    const uint16_t handle = m_handleArray.getVal(_idx);
    Base::remove(handle);
    m_handleArray.remove(_idx);
}

// Avoid using this, it's O(n).
void removeObj(Ty* _obj)
{
    const uint16_t handle = Base::getHandleOf(_obj);
    const uint16_t idx = this->getIdxFromHandle(handle);
    if (idx != Invalid)
    {
        remove(idx);
    }
}

void removeAll()
{
    Base::removeAll();
    m_handleArray.reset();
}

/* vim: set sw=4 ts=4 expandtab: */
