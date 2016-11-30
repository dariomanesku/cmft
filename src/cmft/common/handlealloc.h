/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_HANDLEALLOC_H_HEADER_GUARD
#define CMFT_HANDLEALLOC_H_HEADER_GUARD

#include <stdint.h>

#ifdef max
#   undef max
#endif // max

#ifdef min
#   undef min
#endif // max

namespace cmft
{
    template <typename HandleAllocStorageTy>
    struct HandleAllocImpl : HandleAllocStorageTy
    {
        /// Expected interface:
        ///     struct HandleAllocStorageTemplate
        ///     {
        ///         typedef typename bestfit_type<MaxHandlesT>::type HandleType;
        ///         HandleType* handles();
        ///         HandleType* indices();
        ///         HandleType  max();
        ///     };
        typedef typename HandleAllocStorageTy::HandleType HandleTy;
        using HandleAllocStorageTy::handles;
        using HandleAllocStorageTy::indices;
        using HandleAllocStorageTy::max;

        HandleAllocImpl() : HandleAllocStorageTy()
        {
        }

        void init()
        {
            m_numHandles = 0;
            for (HandleTy ii = 0, end = max(); ii < end; ++ii)
            {
                handles()[ii] = ii;
            }
        }

        HandleTy alloc()
        {
            const HandleTy index = m_numHandles++;
            const HandleTy handle = handles()[index];
            indices()[handle] = index;

            return handle;
        }

        bool contains(uint32_t _handle)
        {
            HandleTy index = indices()[_handle];

            return (index < m_numHandles && handles()[index] == _handle);
        }

        void free(uint32_t _handle)
        {
            HandleTy index = indices()[_handle];

            if (index < m_numHandles && handles()[index] == _handle)
            {
                --m_numHandles;
                HandleTy temp = handles()[m_numHandles];
                handles()[m_numHandles] = _handle;
                indices()[temp] = index;
                handles()[index] = temp;
            }
        }

        HandleTy getHandleAt(uint32_t _idx)
        {
            return handles()[_idx];
        }

        HandleTy getIdxOf(uint32_t _handle)
        {
            return indices()[_handle];
        }

        void reset()
        {
            m_numHandles = 0;
        }

        HandleTy count()
        {
            return m_numHandles;
        }

    private:
        HandleTy m_numHandles;
    };

    template <uint32_t MaxHandlesT>
    struct HandleAllocStorageT
    {
        typedef uint16_t HandleType;

        HandleType* handles()
        {
            return m_handles;
        }

        HandleType* indices()
        {
            return m_indices;
        }

        HandleType max() const
        {
            return MaxHandlesT;
        }

    private:
        HandleType m_handles[MaxHandlesT];
        HandleType m_indices[MaxHandlesT];
    };

    template <uint32_t MaxHandlesT>
    struct HandleAllocT : HandleAllocImpl< HandleAllocStorageT<MaxHandlesT> >
    {
        typedef HandleAllocImpl< HandleAllocStorageT<MaxHandlesT> > Base;

        HandleAllocT()
        {
            Base::init();
        }
    };
}

#endif // CMFT_HANDLEALLOC_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

