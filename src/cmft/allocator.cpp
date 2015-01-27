/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <cmft/allocator.h>

namespace cmft
{
    bx::CrtAllocator crtAllocator;
    bx::ReallocatorI* g_allocator = &crtAllocator;
} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
