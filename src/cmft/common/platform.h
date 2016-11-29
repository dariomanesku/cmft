/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_PLATFORM_HEADER_GUARD
#define CMFT_PLATFORM_HEADER_GUARD

/// Pre-defined C/C++ Compiler Macros: http://sourceforge.net/p/predef/wiki/Home/

//------------------------------------------------------------
// Options.
//------------------------------------------------------------

#define CMFT_COMPILER_MSVC 0
#define CMFT_COMPILER_GCC 0
#define CMFT_COMPILER_CLANG 0

#define CMFT_ARCH_32BIT 0
#define CMFT_ARCH_64BIT 0

#define CMFT_PTR_SIZE 0

#define CMFT_PLATFORM_LINUX 0
#define CMFT_PLATFORM_APPLE 0
#define CMFT_PLATFORM_WINDOWS 0

#define CMFT_PLATFORM_UNIX  0
#define CMFT_PLATFORM_POSIX 0

#define CMFT_CPP11 0
#define CMFT_CPP14 0

//------------------------------------------------------------
// Impl.
//------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#   undef CMFT_COMPILER_MSVC
#   define CMFT_COMPILER_MSVC 1
#elif defined(__GNUC__)
#   undef CMFT_COMPILER_GCC
#   define CMFT_COMPILER_GCC 1
#elif defined(__clang__)
#   undef CMFT_COMPILER_CLANG
#   define CMFT_COMPILER_CLANG 1
#endif

#if (0                      \
    || defined (__amd64__)  \
    || defined (__amd64)    \
    || defined (__x86_64__) \
    || defined (__x86_64)   \
    || defined (_M_X64)     \
    || defined (_M_AMD64)   )
#   undef CMFT_ARCH_64BIT
#   define CMFT_ARCH_64BIT 1
#   undef CMFT_PTR_SIZE
#   define CMFT_PTR_SIZE 8
#elif (0                   \
      || defined(i386)     \
      || defined(__i386)   \
      || defined(__i386__) \
      || defined(__i386)   \
      || defined(__IA32__) \
      || defined(_M_I86)   \
      || defined(_M_IX86)  \
      || defined(_X86_)    \
      || defined(__X86__)  )
#   undef CMFT_ARCH_32BIT
#   define CMFT_ARCH_32BIT 1
#   undef CMFT_PTR_SIZE
#   define CMFT_PTR_SIZE 4
#else
#   error Unsupported platform!
#endif

#if (0                        \
    || defined(__linux__)     \
    || defined(linux)         \
    || defined(__linux)       \
    || defined(__gnu_linux__) )
#   undef CMFT_PLATFORM_LINUX
#   define CMFT_PLATFORM_LINUX 1
#elif (0                    \
      || defined(__APPLE__) \
      || defined(macintosh) \
      || defined(Macintosh) )
#   undef CMFT_PLATFORM_APPLE
#   define CMFT_PLATFORM_APPLE 1
#elif (0 \
      || defined(_WIN16)      \
      || defined(_WIN32)      \
      || defined(_WIN64)      \
      || defined(__WIN32__)   \
      || defined(__TOS_WIN__) \
      || defined(__WINDOWS__) )
#   undef CMFT_PLATFORM_WINDOWS
#   define CMFT_PLATFORM_WINDOWS 1
#endif

#if defined(__unix__) || defined(__unix)
#   undef CMFT_PLATFORM_UNIX
#   define CMFT_PLATFORM_UNIX 1
#endif

#undef CMFT_PLATFORM_POSIX
#define CMFT_PLATFORM_POSIX (CMFT_PLATFORM_LINUX || CMFT_PLATFORM_APPLE || CMFT_PLATFORM_UNIX)

#undef CMFT_CPP11
#define CMFT_CPP11 (__cplusplus >= 201103L)
#undef CMFT_CPP14
#define CMFT_CPP14 (__cplusplus >= 201402L)

#endif // CMFT_PLATFORM_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

