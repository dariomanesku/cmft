/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <cmft/allocator.h>

namespace cmft
{
    CrtAllocator      g_crtAllocator;
    CrtStackAllocator g_crtStackAllocator;

    AllocatorI*      g_allocator      = &g_crtAllocator;
    StackAllocatorI* g_stackAllocator = &g_crtStackAllocator;

    void setAllocator(AllocatorI* _allocator)
    {
        g_allocator = (NULL != _allocator) ? _allocator : &g_crtAllocator;
    }

    void setStackAllocator(StackAllocatorI* _stackAllocator)
    {
        g_stackAllocator = (NULL != _stackAllocator) ? _stackAllocator : &g_crtStackAllocator;
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
