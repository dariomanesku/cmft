/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CMFT_UTILS_H_HEADER_GUARD
#define CMFT_CMFT_UTILS_H_HEADER_GUARD

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <float.h>
#if defined(_WIN32)
#   include <malloc.h>
#   define alloca _alloca
#else /* Unix */
#   include <alloca.h>
#endif // defined(_WIN32)

#include "platform.h"

namespace cmft
{
    // Macros.
    //-----

    #define CMFT_PATH_LEN 4096

    #define RLOG2 1.4426950408889634f

    #define CMFT_MIN(_a, _b) ((_a)<(_b)?(_a):(_b))
    #define CMFT_MAX(_a, _b) ((_a)>(_b)?(_a):(_b))
    #define CMFT_CLAMP(_val, _min, _max) (CMFT_MIN(CMFT_MAX(_val, _min), _max))

    #define CMFT_UNUSED(_expr) for (;;) { (void)(true ? (void)0 : ((void)(_expr))); break; }

    #define CMFT_STRINGIZE(_x) CMFT_STRINGIZE_(_x)
    #define CMFT_STRINGIZE_(_x) #_x

    #define CMFT_CONCATENATE(_x, _y) CMFT_CONCATENATE_(_x, _y)
    #define CMFT_CONCATENATE_(_x, _y) _x ## _y

    #define _FILE_LINE_ " " __FILE__ "(" CMFT_STRINGIZE(__LINE__) ")"

    #define CMFT_MAKEFOURCC(_a, _b, _c, _d)                  \
                           ( ((uint32_t)(uint8_t)(_a))       \
                           | ((uint32_t)(uint8_t)(_b) <<  8) \
                           | ((uint32_t)(uint8_t)(_c) << 16) \
                           | ((uint32_t)(uint8_t)(_d) << 24) \
                           )

    #define CMFT_COUNTOF(arr) (sizeof(arr)/sizeof(0[arr]))

    // Pragma macros.
    //-----

    #if CMFT_COMPILER_CLANG
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH_CLANG_()     _Pragma("clang diagnostic push")
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP_CLANG_()      _Pragma("clang diagnostic pop")
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_CLANG(_x) _Pragma(CMFT_STRINGIZE(clang diagnostic ignored _x))
    #else
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH_CLANG_()
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP_CLANG_()
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_CLANG(_x)
    #endif // CMFT_COMPILER_CLANG

    #if CMFT_COMPILER_GCC
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH_GCC_()       _Pragma("GCC diagnostic push")
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP_GCC_()        _Pragma("GCC diagnostic pop")
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_GCC(_x)   _Pragma(CMFT_STRINGIZE(GCC diagnostic ignored _x))
    #else
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH_GCC_()
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP_GCC_()
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_GCC(_x)
    #endif // CMFT_COMPILER_GCC

    #if CMFT_COMPILER_MSVC
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH_MSVC_()     __pragma(warning(push))
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP_MSVC_()      __pragma(warning(pop))
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(_x) __pragma(warning(disable:_x))
    #else
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH_MSVC_()
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP_MSVC_()
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(_x)
    #endif // CMFT_COMPILER_CLANG

    #if CMFT_COMPILER_CLANG
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH              CMFT_PRAGMA_DIAGNOSTIC_PUSH_CLANG_
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP               CMFT_PRAGMA_DIAGNOSTIC_POP_CLANG_
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC CMFT_PRAGMA_DIAGNOSTIC_IGNORED_CLANG
    #elif CMFT_COMPILER_GCC
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH              CMFT_PRAGMA_DIAGNOSTIC_PUSH_GCC_
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP               CMFT_PRAGMA_DIAGNOSTIC_POP_GCC_
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC CMFT_PRAGMA_DIAGNOSTIC_IGNORED_GCC
    #elif CMFT_COMPILER_MSVC
    #   define CMFT_PRAGMA_DIAGNOSTIC_PUSH              CMFT_PRAGMA_DIAGNOSTIC_PUSH_MSVC_
    #   define CMFT_PRAGMA_DIAGNOSTIC_POP               CMFT_PRAGMA_DIAGNOSTIC_POP_MSVC_
    #   define CMFT_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC(_x)
    #endif // CMFT_COMPILER_

    // Value.
    //-----

    static inline float    utof(uint32_t _u32) { return float(int32_t(_u32));  }
    static inline uint32_t ftou(float _f)      { return uint32_t(int32_t(_f)); }

    template <typename Ty/*arithmetic type*/>
    static inline void swap(Ty _a, Ty _b)
    {
        Ty c = _a;
        _a = _b;
        _b = c;
    }

    static inline void swap(uint8_t* __restrict _a, uint8_t* __restrict _b, uint32_t _size)
    {
        uint8_t* c = (uint8_t*)alloca(_size);
        memcpy( c, _a, _size);
        memcpy(_a, _b, _size);
        memcpy(_b,  c, _size);
    }

    static inline void swap(uint8_t* __restrict _a, uint8_t* __restrict _b, uint8_t* __restrict _tmp, uint32_t _size)
    {
        uint8_t* c = _tmp;
        memcpy( c, _a, _size);
        memcpy(_a, _b, _size);
        memcpy(_b,  c, _size);
    }

    // Math.
    //-----

    static inline float log2f(float _val)
    {
        return logf(_val)*RLOG2;
    }

    static inline bool equals(float _a, float _b, float _epsilon = FLT_EPSILON)
    {
        return fabsf(_a - _b) < _epsilon;
    }

    // Align.
    //-----

    static inline  uint32_t align(uint32_t _val, uint32_t _alignPwrTwo)
    {
        const uint32_t mask = _alignPwrTwo-UINT32_C(1);
        return (_val+mask)&(~mask);
    }

    static inline  uint32_t alignf(float _val, uint32_t _align)
    {
        return uint32_t(_val/float(int32_t(_align)))*_align;
    }

    // File.
    //-----

    static inline long int fsize(FILE* _file)
    {
        long int pos = ftell(_file);
        fseek(_file, 0L, SEEK_END);
        long int size = ftell(_file);
        fseek(_file, pos, SEEK_SET);
        return size;
    }

    // String.
    //-----

    /// Case insensitive string compare.
    inline int32_t stricmp(const char* _a, const char* _b)
    {
    #if defined(_MSC_VER)
        return ::_stricmp(_a, _b);
    #else
        return ::strcasecmp(_a, _b);
    #endif // BX_COMPILER_
    }

    static inline void strscpy(char* _dst, const char* _src, size_t _dstSize)
    {
        _dst[0] = '\0';
        if (NULL != _src)
        {
            strncat(_dst, _src, _dstSize-1);
        }
    }

    template <uint32_t DstSize>
    static inline void stracpy(char (&_dst)[DstSize], const char* _src)
    {
        strscpy(_dst, _src, DstSize);
    }

    /// Notice: do NOT use return value of this function for memory deallocation!
    static inline char* trim(char* _str)
    {
        char* beg = _str;
        char* end = _str + strlen(_str)-1;

        // Point to the first non-whitespace character.
        while (isspace(*beg)) { ++beg; }

        // If end is reached (_str contained all spaces), return.
        if ('\0' == *beg)
        {
            return beg;
        }

        // Point to the last non-whitespace character.
        while (isspace(*end)) { --end; }

        // Add string terminator after non-whitespace character.
        end[1] = '\0';

        return beg;
    }

    /// Find substring in string. Case insensitive.
    inline const char* stristr(const char* _str, const char* _find)
    {
        const char* ptr = _str;

        for (size_t len = strlen(_str), searchLen = strlen(_find)
            ; len >= searchLen
            ; ++ptr, --len)
        {
            // Find start of the string.
            while (tolower(*ptr) != tolower(*_find) )
            {
                ++ptr;
                --len;

                // Search pattern lenght can't be longer than the string.
                if (searchLen > len)
                {
                    return NULL;
                }
            }

            // Set pointers.
            const char* string = ptr;
            const char* search = _find;

            // Start comparing.
            while (tolower(*string++) == tolower(*search++) )
            {
                // If end of the 'search' string is reached, all characters match.
                if ('\0' == *search)
                {
                    return ptr;
                }
            }
        }

        return NULL;
    }

    /// Find substring in string. Case insensitive. Limit search to _max.
    inline const char* stristr(const char* _str, const char* _find, size_t _max)
    {
        const char* ptr = _str;

        size_t       stringLen = strnlen(_str, _max);
        const size_t findLen   = strlen(_find);

        for (; stringLen >= findLen; ++ptr, --stringLen)
        {
            // Find start of the string.
            while (tolower(*ptr) != tolower(*_find) )
            {
                ++ptr;
                --stringLen;

                // Search pattern lenght can't be longer than the string.
                if (findLen > stringLen)
                {
                    return NULL;
                }
            }

            // Set pointers.
            const char* string = ptr;
            const char* search = _find;

            // Start comparing.
            while (tolower(*string++) == tolower(*search++) )
            {
                // If end of the 'search' string is reached, all characters match.
                if ('\0' == *search)
                {
                    return ptr;
                }
            }
        }

        return NULL;
    }

    /// Appends src to string dst of size siz (unlike strncat, siz is the
    /// full size of dst, not space left).  At most siz-1 characters
    /// will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
    /// Returns strlen(src) + MIN(siz, strlen(initial dst)).
    /// If retval >= siz, truncation occurred.
    inline size_t strlcat(char* _dst, const char* _src, size_t _siz)
    {
        char* dd = _dst;
        const char *s = _src;
        size_t nn = _siz;
        size_t dlen;

        /* Find the end of dst and adjust bytes left but don't go past end */
        while (nn-- != 0 && *dd != '\0')
        {
            dd++;
        }

        dlen = dd - _dst;
        nn = _siz - dlen;

        if (nn == 0)
        {
            return(dlen + strlen(s));
        }

        while (*s != '\0')
        {
            if (nn != 1)
            {
                *dd++ = *s;
                nn--;
            }
            s++;
        }
        *dd = '\0';

        return(dlen + (s - _src)); /* count does not include NUL */
    }

    /// Find substring in string. Limit search to _size.
    inline const char* strnstr(const char* _str, const char* _find, size_t _size)
    {
        char first = *_find;
        if ('\0' == first)
        {
            return _str;
        }

        const char* cmp = _find + 1;
        size_t len = strlen(cmp);
        do
        {
            for (char match = *_str++; match != first && 0 < _size; match = *_str++, --_size)
            {
                if ('\0' == match)
                {
                    return NULL;
                }
            }

            if (0 == _size)
            {
                return NULL;
            }

        } while (0 != strncmp(_str, cmp, len) );

        return --_str;
    }

    /// Find end of line. Retuns pointer to new line terminator.
    inline const char* streol(const char* _str)
    {
        for (; '\0' != *_str; _str += strnlen(_str, 1024) )
        {
            const char* eol = strnstr(_str, "\r\n", 1024);
            if (NULL != eol)
            {
                return eol;
            }

            eol = strnstr(_str, "\n", 1024);
            if (NULL != eol)
            {
                return eol;
            }
        }

        return _str;
    }

    /// Find new line. Returns pointer after new line terminator.
    inline const char* strnl(const char* _str)
    {
        for (; '\0' != *_str; _str += strnlen(_str, 1024) )
        {
            const char* eol = strnstr(_str, "\r\n", 1024);
            if (NULL != eol)
            {
                return eol + 2;
            }

            eol = strnstr(_str, "\n", 1024);
            if (NULL != eol)
            {
                return eol + 1;
            }
        }

        return _str;
    }

    static inline void strtolower(char* _out, char* _in)
    {
        while (*_in) { *_out++ = (char)tolower(*_in++); }
        *_out = '\0';
    }

    static inline void strtoupper(char* _out, char* _in)
    {
        while (*_in) { *_out++ = (char)toupper(*_in++); }
        *_out = '\0';
    }

    static inline void strtolower(char* _str)
    {
        for (; *_str; ++_str)
        {
            *_str = (char)tolower(*_str);
        }
    }

    static inline void strtoupper(char* _str)
    {
        for (; *_str; ++_str)
        {
            *_str = (char)toupper(*_str);
        }
    }

    static inline int32_t vsnprintf(char* _str, size_t _count, const char* _format, va_list _argList)
    {
        #if CMFT_COMPILER_MSVC
            int32_t len = ::vsnprintf_s(_str, _count, size_t(-1), _format, _argList);
            return -1 == len ? ::_vscprintf(_format, _argList) : len;
        #else
            return ::vsnprintf(_str, _count, _format, _argList);
        #endif // CMFT_COMPILER_MSVC
    }

    static inline int32_t snprintf(char* _str, size_t _count, const char* _format, ...)
    {
        va_list argList;
        va_start(argList, _format);
        int32_t len = vsnprintf(_str, _count, _format, argList);
        va_end(argList);
        return len;
    }

    // Path.
    //-----

    /// Gets file name without extension from file path. Examples:
    ///     /tmp/foo.c -> foo
    ///     C:\\tmp\\foo.c -> foo
    static inline bool basename(char* _out, size_t _outSize, const char* _filePath)
    {
        const char* begin;
        const char* end;

        const char* ptr;
        begin = NULL != (ptr = strrchr(_filePath, '\\')) ? ++ptr
              : NULL != (ptr = strrchr(_filePath, '/' )) ? ++ptr
              : _filePath
              ;

        end = NULL != (ptr = strrchr(_filePath, '.')) ? ptr : strrchr(_filePath, '\0');

        if (NULL != begin && NULL != end)
        {
            const size_t size = CMFT_MIN(size_t(end-begin)+1, _outSize);
            cmft::strscpy(_out, begin, size);
            return true;
        }

        return false;
    }

    // Endianess.
    //-----

    inline uint16_t endianSwap(uint16_t _in)
    {
        return (_in>>8) | (_in<<8);
    }

    inline uint32_t endianSwap(uint32_t _in)
    {
        return (_in>>24) | (_in<<24)
             | ((_in&0x00ff0000)>>8) | ((_in&0x0000ff00)<<8)
             ;
    }

    inline uint64_t endianSwap(uint64_t _in)
    {
        return (_in>>56) | (_in<<56)
             | ((_in&UINT64_C(0x00ff000000000000))>>40) | ((_in&UINT64_C(0x000000000000ff00))<<40)
             | ((_in&UINT64_C(0x0000ff0000000000))>>24) | ((_in&UINT64_C(0x0000000000ff0000))<<24)
             | ((_in&UINT64_C(0x000000ff00000000))>>8 ) | ((_in&UINT64_C(0x00000000ff000000))<<8 )
             ;
    }

    // Scope.
    //-----

    struct ScopeFclose
    {
        ScopeFclose(FILE* _fp) { m_fp = _fp; }
        ~ScopeFclose()         { if (NULL != m_fp) { fclose(m_fp); } }

    private:
        FILE* m_fp;
    };
}

#endif // CMFT_CMFT_UTILS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
