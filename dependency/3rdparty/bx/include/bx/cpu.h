/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef BX_CPU_H_HEADER_GUARD
#define BX_CPU_H_HEADER_GUARD

#include "bx.h"

#if BX_COMPILER_MSVC
#	if BX_PLATFORM_XBOX360
#		include <ppcintrinsics.h>
#		include <xtl.h>
#	else
#		include <math.h> // math.h is included because VS bitches:
						 // warning C4985: 'ceil': attributes not present on previous declaration.
						 // must be included before intrin.h.
#		include <intrin.h>
#		include <windows.h>
#	endif // !BX_PLATFORM_XBOX360
extern "C" void _ReadBarrier();
extern "C" void _WriteBarrier();
extern "C" void _ReadWriteBarrier();
#	pragma intrinsic(_ReadBarrier)
#	pragma intrinsic(_WriteBarrier)
#	pragma intrinsic(_ReadWriteBarrier)
#	pragma intrinsic(_InterlockedIncrement)
#	pragma intrinsic(_InterlockedDecrement)
#endif // BX_COMPILER_MSVC

namespace bx
{
	inline void readBarrier()
	{
#if BX_COMPILER_MSVC
		_ReadBarrier();
#elif BX_COMPILER_GCC || BX_COMPILER_CLANG
		asm volatile("":::"memory");
#endif // BX_COMPILER
	}

	inline void writeBarrier()
	{
#if BX_COMPILER_MSVC
		_WriteBarrier();
#elif BX_COMPILER_GCC || BX_COMPILER_CLANG
		asm volatile("":::"memory");
#endif // BX_COMPILER
	}

	inline void readWriteBarrier()
	{
#if BX_COMPILER_MSVC
		_ReadWriteBarrier();
#elif BX_COMPILER_GCC || BX_COMPILER_CLANG
		asm volatile("":::"memory");
#endif // BX_COMPILER
	}

	inline void memoryBarrier()
	{
#if BX_PLATFORM_XBOX360
		__lwsync();
#elif BX_COMPILER_MSVC
		_mm_mfence();
#else
		__sync_synchronize();
//		asm volatile("mfence":::"memory");
#endif // BX_COMPILER
	}

	inline int32_t atomicIncr(volatile void* _var)
	{
#if BX_COMPILER_MSVC
		return _InterlockedIncrement( (volatile LONG*)(_var) );
#elif BX_COMPILER_GCC || BX_COMPILER_CLANG
		return __sync_fetch_and_add( (volatile int32_t*)_var, 1);
#endif // BX_COMPILER
	}

	inline int32_t atomicDecr(volatile void* _var)
	{
#if BX_COMPILER_MSVC
		return _InterlockedDecrement( (volatile LONG*)(_var) );
#elif BX_COMPILER_GCC || BX_COMPILER_CLANG
		return __sync_fetch_and_sub( (volatile int32_t*)_var, 1);
#endif // BX_COMPILER
	}

	inline void* atomicExchangePtr(void** _target, void* _ptr)
	{
#if BX_COMPILER_MSVC
		return InterlockedExchangePointer(_target, _ptr);
#elif BX_COMPILER_GCC || BX_COMPILER_CLANG
		return __sync_lock_test_and_set(_target, _ptr);
#endif // BX_COMPILER
	}

} // namespace bx

#endif // BX_CPU_H_HEADER_GUARD
