/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef BX_OS_H_HEADER_GUARD
#define BX_OS_H_HEADER_GUARD

#include "bx.h"

#if BX_PLATFORM_WINDOWS
#	include <windows.h>
#elif BX_PLATFORM_NACL \
	|| BX_PLATFORM_ANDROID \
	|| BX_PLATFORM_LINUX \
	|| BX_PLATFORM_OSX \
	|| BX_PLATFORM_IOS \
	|| BX_PLATFORM_EMSCRIPTEN

#	include <sched.h> // sched_yield
#	if BX_PLATFORM_IOS || BX_PLATFORM_OSX || BX_PLATFORM_NACL
#		include <pthread.h> // mach_port_t
#	endif // BX_PLATFORM_IOS || BX_PLATFORM_OSX || BX_PLATFORM_NACL

#	if BX_PLATFORM_NACL
#		include <sys/nacl_syscalls.h> // nanosleep
#	else
#		include <time.h> // nanosleep
#		include <dlfcn.h> // dlopen, dlclose, dlsym
#	endif // BX_PLATFORM_NACL

#	if BX_PLATFORM_LINUX
#		include <unistd.h> // syscall
#		include <sys/syscall.h>
#	endif // BX_PLATFORM_LINUX

#	if BX_PLATFORM_ANDROID
#		include "debug.h" // getTid is not implemented...
#	endif // BX_PLATFORM_ANDROID
#endif // BX_PLATFORM_

namespace bx
{
	inline void sleep(uint32_t _ms)
	{
#if BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
		::Sleep(_ms);
#else
		timespec req = {(time_t)_ms/1000, (long)((_ms%1000)*1000000)};
		timespec rem = {0, 0};
		::nanosleep(&req, &rem);
#endif // BX_PLATFORM_
	}

	inline void yield()
	{
#if BX_PLATFORM_WINDOWS
		::SwitchToThread();
#elif BX_PLATFORM_XBOX360
		::Sleep(0);
#else
		::sched_yield();
#endif // BX_PLATFORM_
	}

	inline uint32_t getTid()
	{
#if BX_PLATFORM_WINDOWS
		return ::GetCurrentThreadId();
#elif BX_PLATFORM_LINUX
		return (pid_t)::syscall(SYS_gettid);
#elif BX_PLATFORM_IOS || BX_PLATFORM_OSX
		return (mach_port_t)::pthread_mach_thread_np(pthread_self() );
#elif BX_PLATFORM_NACL
		// Casting __nc_basic_thread_data*... need better way to do this.
		return *(uint32_t*)::pthread_self();
#else
//#	pragma message "not implemented."
		debugOutput("getTid is not implemented"); debugBreak();
		return 0;
#endif //
	}

	inline void* dlopen(const char* _filePath)
	{
#if BX_PLATFORM_WINDOWS
		return (void*)::LoadLibraryA(_filePath);
#elif BX_PLATFORM_NACL || BX_PLATFORM_EMSCRIPTEN
		BX_UNUSED(_filePath);
		return NULL;
#else
		return ::dlopen(_filePath, RTLD_LOCAL|RTLD_LAZY);
#endif // BX_PLATFORM_
	}

	inline void dlclose(void* _handle)
	{
#if BX_PLATFORM_WINDOWS
		::FreeLibrary( (HMODULE)_handle);
#elif BX_PLATFORM_NACL || BX_PLATFORM_EMSCRIPTEN
		BX_UNUSED(_handle);
#else
		::dlclose(_handle);
#endif // BX_PLATFORM_
	}

	inline void* dlsym(void* _handle, const char* _symbol)
	{
#if BX_PLATFORM_WINDOWS
		return (void*)::GetProcAddress( (HMODULE)_handle, _symbol);
#elif BX_PLATFORM_NACL || BX_PLATFORM_EMSCRIPTEN
		BX_UNUSED(_handle, _symbol);
		return NULL;
#else
		return ::dlsym(_handle, _symbol);
#endif // BX_PLATFORM_
	}

	inline void setenv(const char* _name, const char* _value)
	{
#if BX_PLATFORM_WINDOWS
		::SetEnvironmentVariableA(_name, _value);
#else
		::setenv(_name, _value, 1);
#endif // BX_PLATFORM_
	}

	inline void unsetenv(const char* _name)
	{
#if BX_PLATFORM_WINDOWS
		::SetEnvironmentVariableA(_name, NULL);
#else
		::unsetenv(_name);
#endif // BX_PLATFORM_
	}

} // namespace bx

#endif // BX_OS_H_HEADER_GUARD
