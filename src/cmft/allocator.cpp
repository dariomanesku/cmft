/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <bx/allocator.h>
#include <cmft/allocator.h>

namespace cmft
{
    dm::CrtAllocator crtAllocator;

    dm::AllocatorI* g_allocator      = &crtAllocator;
    dm::AllocatorI* g_stackAllocator = &crtAllocator;
} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
