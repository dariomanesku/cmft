/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

/*
 * Copyright 2010-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

// Copyright 2006 Mike Acton <macton@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE

#ifndef CMFT_OS_H_HEADER_GUARD
#define CMFT_OS_H_HEADER_GUARD

#include "platform.h"
#include <stdint.h>

#if CMFT_PLATFORM_WINDOWS
#   include <windows.h>
#elif CMFT_PLATFORM_LINUX || CMFT_PLATFORM_APPLE
#   include <sched.h> // sched_yield
#   if CMFT_PLATFORM_APPLE
#       include <pthread.h> // mach_port_t
#   endif // CMFT_PLATFORM_*
#
#   include <dlfcn.h> // dlopen, dlclose, dlsym
#
#   if CMFT_PLATFORM_LINUX
#       include <unistd.h> // syscall
#       include <sys/syscall.h>
#   endif // CMFT_PLATFORM_LINUX
#endif // CMFT_PLATFORM_

namespace cmft
{
    inline void* dlopen(const char* _filePath)
    {
    #if CMFT_PLATFORM_WINDOWS
        return (void*)::LoadLibraryA(_filePath);
    #else
        return ::dlopen(_filePath, RTLD_LOCAL|RTLD_LAZY);
    #endif // CMFT_PLATFORM_
    }

    inline void dlclose(void* _handle)
    {
    #if CMFT_PLATFORM_WINDOWS
        ::FreeLibrary( (HMODULE)_handle);
    #else
        ::dlclose(_handle);
    #endif // CMFT_PLATFORM_
    }

    inline void* dlsym(void* _handle, const char* _symbol)
    {
    #if CMFT_PLATFORM_WINDOWS
        return (void*)::GetProcAddress( (HMODULE)_handle, _symbol);
    #else
        return ::dlsym(_handle, _symbol);
    #endif // CMFT_PLATFORM_
    }
}

#endif // CMFT_OS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

