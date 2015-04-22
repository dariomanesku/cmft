/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_MISC_H_HEADER_GUARD
#define DM_MISC_H_HEADER_GUARD

#include <stdint.h>
#include <stdlib.h> // _fullpath
#include <ctype.h>  // toupper()
#include <math.h>   // logf()
#include <stdio.h>  // FILE, fopen()
#include <float.h>  // FLT_EPSILON
#include <malloc.h> // alloca()

#include "common/common.h" // DM_INLINE()
#include "check.h"         // DM_CHECK()

#include "../../3rdparty/bx/os.h"       // bx::pwd()
#include "../../3rdparty/bx/string.h"   // bx::strlcat()
#include "../../3rdparty/bx/uint32_t.h" // bx::uint32_cntlz(), bx::uint64_cntlz()

namespace dm
{
    // Compile time.
    //-----

    /// Usage: Log<2, 512>::Value
    template <uint8_t Base, uint32_t N>
    struct Log
    {
        enum { Value = 1 + Log<Base, N/Base>::Value };
    };
    template <uint8_t Base> struct Log<Base, 1> { enum { Value = 0 }; };
    template <uint8_t Base> struct Log<Base, 0> { enum { Value = 0 }; };

    // Value.
    //-----

    #define DM_MIN(_a, _b) ((_a)<(_b)?(_a):(_b))
    #define DM_MAX(_a, _b) ((_a)>(_b)?(_a):(_b))
    #define DM_CLAMP(_val, _min, _max) (DM_MIN(DM_MAX(_val, _min), _max))

    template <typename Ty/*arithmetic type*/>
    DM_INLINE Ty min(Ty _a, Ty _b)
    {
        return  _a < _b ? _a : _b;
    }

    template <typename Ty/*arithmetic type*/>
    DM_INLINE Ty max(Ty _a, Ty _b)
    {
        return  _a > _b ? _a : _b;
    }

    /// Assumes _min < _max.
    template <typename Ty/*arithmetic type*/>
    DM_INLINE Ty clamp(Ty _val, Ty _min, Ty _max)
    {
        return ( _val > _max ? _max
               : _val < _min ? _min
               : _val
               );
    }

    template <typename Ty/*arithmetic type*/>
    DM_INLINE void swap(Ty _a, Ty _b)
    {
        Ty c = _a;
        _a = _b;
        _b = c;
    }

    DM_INLINE void swap(uint8_t* __restrict _a, uint8_t* __restrict _b, uint32_t _size)
    {
        uint8_t* c = (uint8_t*)alloca(_size);
        memcpy( c, _a, _size);
        memcpy(_a, _b, _size);
        memcpy(_b,  c, _size);
    }

    DM_INLINE void swap(uint8_t* __restrict _a, uint8_t* __restrict _b, uint8_t* __restrict _tmp, uint32_t _size)
    {
        uint8_t* c = _tmp;
        memcpy( c, _a, _size);
        memcpy(_a, _b, _size);
        memcpy(_b,  c, _size);
    }

    DM_INLINE void toggle(bool& _flag)
    {
        _flag = !_flag;
    }

    // Integer.
    //-----

    #define DM_KILOBYTES(_KB)     (_KB##ul<<10ul)
    #define DM_MEGABYTES(_MB)     (_MB##ul<<20ul)
    #define DM_GIGABYTES(_GB)     (_GB##ul<<30ul)
    #define DM_GIGABYTES_ULL(_GB) (_GB##ull<<30ull)

    DM_INLINE uint32_t log2floor(uint32_t _u32)
    {
        return (31 - bx::uint32_cntlz(_u32));
    }

    DM_INLINE uint64_t log2floor(uint64_t _u64)
    {
        return (63 - bx::uint64_cntlz(_u64));
    }

    DM_INLINE uint32_t log2ceil(uint32_t _u32)
    {
        const uint32_t floor = log2floor(_u32);
        const uint32_t mask = (UINT32_C(1)<<floor)-1;
        return floor + (0 != (_u32&mask));
    }

    DM_INLINE uint64_t log2ceil(uint64_t _u64)
    {
        const uint64_t floor = log2floor(_u64);
        const uint64_t mask = (UINT64_C(1)<<floor)-1;
        return floor + (0 != (_u64&mask));
    }

    DM_INLINE uint32_t log2(uint32_t _u32)
    {
        return log2floor(_u32);
    }

    DM_INLINE uint64_t log2(uint64_t _u64)
    {
        return log2floor(_u64);
    }

    /// Usage:
    ///   270 -> 256 /* For a non-power-of-two input, returns expected value. */
    ///   256 -> 256 /* For a power-of-two input, returns the same value. */
    ///     0 ->   1 /* For 0 input, returns invalid value! */
    DM_INLINE uint32_t prevPowTwo(uint32_t _u32)
    {
        const uint32_t floor = log2floor(_u32);
        return (UINT32_C(1)<<floor);
    }

    /// Usage:
    ///   270 -> 512 /* For a non-power-of-two input, returns expected value. */
    ///   256 -> 256 /* For a power-of-two input, returns the same value. */
    ///     0 ->   1 /* For 0 input, returns 1. */
    DM_INLINE uint32_t nextPowTwo(uint32_t _u32)
    {
        const uint32_t ceil = log2ceil(_u32);
        return (UINT32_C(1)<<ceil);
    }

    /// Example: for input 12780 (12.492KB) returns 12.
    DM_INLINE uint64_t asKBInt(uint64_t _dataSize)
    {
        return _dataSize>>10;
    }

    /// Example: for input 12780 (12.492KB) returns 492.
    DM_INLINE uint64_t asKBDec(uint64_t _dataSize)
    {
        const uint64_t kb = asKBInt(_dataSize);
        return _dataSize-(kb<<10);
    }

    /// Example: for input 13450000 (12.846MB) returns 12.
    DM_INLINE uint64_t asMBInt(uint64_t _dataSize)
    {
        return _dataSize>>20;
    }

    /// Example: for input 13450000 (12.846MB) returns 826.
    DM_INLINE uint64_t asMBDec(uint64_t _dataSize)
    {
        const uint64_t mb = asMBInt(_dataSize);
        return (_dataSize-(mb<<20))>>10;
    }

    /// Used for formatted print. Example: printf("Size: %u.%uMB", U_UMB(size));
    #define U_UKB(_size) asKBInt(_size), dm::asKBDec(_size)
    #define U_UMB(_size) asMBInt(_size), dm::asMBDec(_size)

    #define DM_BOOL(_val) (0 != (_val))

    DM_INLINE bool toBool(int32_t _v)
    {
        return (0 != _v);
    }

    DM_INLINE bool inside(int32_t _px, int32_t _py, int32_t _minx, int32_t _miny, int32_t _width, int32_t _height)
    {
        return (_px > _minx)
            && (_py > _miny)
            && (_px < (_minx+_width))
            && (_py < (_miny+_height));
    }

    // Align.
    //-----

    DM_INLINE uint32_t align(uint32_t _val, uint32_t _alignPwrTwo)
    {
        const uint32_t mask = _alignPwrTwo-UINT32_C(1);
        return (_val+mask)&(~mask);
    }

    DM_INLINE uint32_t alignf(float _val, uint32_t _align)
    {
        return uint32_t(_val/float(int32_t(_align)))*_align;
    }

    DM_INLINE void* alignPtrNext(void* _ptr, size_t _alignPwrTwo)
    {
        union { void* p; size_t addr; } ptr;
        ptr.p = _ptr;
        const size_t mask = _alignPwrTwo-1;
        ptr.addr = (ptr.addr+mask)&(~mask);
        return ptr.p;
    }

    DM_INLINE void* alignPtrPrev(void* _ptr, size_t _alignPwrTwo)
    {
        union { void* p; size_t addr; } ptr;
        ptr.p = _ptr;
        const size_t mask = _alignPwrTwo-1;
        ptr.addr = ptr.addr&(~mask);
        return ptr.p;
    }

    DM_INLINE size_t alignSizeNext(size_t _size, size_t _alignPwrTwo)
    {
        const size_t mask = (_alignPwrTwo-1);
        return (_size+mask)&(~mask);
    }

    DM_INLINE size_t alignSizePrev(size_t _size, size_t _alignPwrTwo)
    {
        const size_t mask = (_alignPwrTwo-1);
        return _size&(~mask);
    }

    DM_INLINE void alignPtrAndSize(void*& _alignedPtr, size_t& _alignedSize, void* _ptr, size_t _size, size_t _alignPwrTwo)
    {
        _alignedPtr = alignPtrNext(_ptr, _alignPwrTwo);
        const size_t diff = (uint8_t*)_alignedPtr - (uint8_t*)_ptr;
        const size_t totalSize = _size + diff;
        _alignedSize = alignSizePrev(totalSize, _alignPwrTwo);
    }

    // Float.
    //-----

    DM_INLINE float utof(uint32_t _u32)
    {
        DM_CHECK((_u32&0x80000000) == 0, "Unsigned value is too big!");
        return float(int32_t(_u32));
    }

    DM_INLINE uint32_t ftou(float _f)
    {
        return uint32_t(int32_t(_f));
    }

    DM_INLINE bool equals(float _a, float _b, float _epsilon = FLT_EPSILON)
    {
        return fabsf(_a - _b) < _epsilon;
    }

    /// Example: for input 5.34f returns 5.0f.
    DM_INLINE float integerPart(float _val)
    {
        return float(int32_t(_val));
    }

    /// Example: for input 5.34f returns 0.34f.
    DM_INLINE float decimalPart(float _val)
    {
        return _val - integerPart(_val);
    }

    DM_INLINE bool isSet(float _flag)
    {
        return (0.0f != _flag);
    }

    DM_INLINE float signf(float _val)
    {
        return (_val > 0.0f) ? 1.0f : -1.0f;
    }

    DM_INLINE float squaref(float _x)
    {
        return _x*_x;
    }

    DM_INLINE float fminf(float _a, float _b)
    {
        return _a < _b ? _a : _b;
    }

    DM_INLINE float fmaxf(float _a, float _b)
    {
        return _a > _b ? _a : _b;
    }

    #if defined(_MSC_VER)
    DM_INLINE float log2f(float _val)
    {
        static const float invLog2 = 1.4426950408889634f;
        return logf(_val)*invLog2;
    }
    #else
    DM_INLINE float log2f(float _val)
    {
        return ::log2f(_val);
    }
    #endif //defined(_MSC_VER)

    // String.
    //----

    DM_INLINE void strscpy(char* _dst, const char* _src, size_t _dstSize)
    {
        _dst[0] = '\0';
        if (NULL != _src)
        {
            strncat(_dst, _src, _dstSize-1);
        }
    }
    #define strscpya(_charArray, _src) strscpy(_charArray, _src, sizeof(_charArray))

    DM_INLINE void strtolower(char* _out, char* _in)
    {
        while (*_in)
        {
            *_out++ = (char)tolower(*_in++);
        }
        *_out = '\0';
    }

    DM_INLINE void strtoupper(char* _out, char* _in)
    {
        while (*_in)
        {
            *_out++ = (char)toupper(*_in++);
        }
        *_out = '\0';
    }

    DM_INLINE void strtolower(char* _str)
    {
        for( ; *_str; ++_str)
        {
            *_str = (char)tolower(*_str);
        }
    }

    DM_INLINE void strtoupper(char* _str)
    {
        for( ; *_str; ++_str)
        {
            *_str = (char)toupper(*_str);
        }
    }

    /// Notice: do NOT use return value of this function for memory deallocation!
    DM_INLINE char* trim(char* _str)
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

    // File system.
    //-----

    #define DM_PATH_LEN 4096

    #if BX_PLATFORM_WINDOWS
    #   define DM_DIRSLASH "\\"
    #else
    #   define DM_DIRSLASH "/"
    #endif

    DM_INLINE void realpath(char _abs[DM_PATH_LEN], const char _rel[DM_PATH_LEN])
    {
        #if BX_PLATFORM_WINDOWS
            _fullpath(_abs, _rel, DM_PATH_LEN);
        #elif BX_PLATFORM_LINUX
            bx::snprintf(_abs, DM_PATH_LEN, "%s", _rel); //::realpath is not thread-safe on Linux.
        #else // OSX
            char* path = ::realpath(_rel, _abs);
            BX_UNUSED(path);
        #endif // BX_PLATFORM_WINDOWS
    }

    DM_INLINE void homeDir(char _path[DM_PATH_LEN])
    {
        #if BX_PLATFORM_WINDOWS
            strscpy(_path, getenv("USERPROFILE"), DM_PATH_LEN);
        #else // OSX and Linux.
            strscpy(_path, getenv("HOME"), DM_PATH_LEN);
        #endif
    }

    DM_INLINE void desktopDir(char _path[DM_PATH_LEN])
    {
        #if BX_PLATFORM_WINDOWS
            strscpy(_path, getenv("USERPROFILE"), DM_PATH_LEN);
            bx::strlcat(_path, DM_DIRSLASH"Desktop", DM_PATH_LEN);
        #else // OSX and Linux.
            strscpy(_path, getenv("HOME"), DM_PATH_LEN);
            bx::strlcat(_path, DM_DIRSLASH"Desktop", DM_PATH_LEN);
        #endif
    }

    DM_INLINE void rootDir(char _path[DM_PATH_LEN])
    {
        #if BX_PLATFORM_WINDOWS
            char currentDir[DM_PATH_LEN];
            bx::pwd(currentDir, DM_PATH_LEN);
            strscpy(_path, currentDir, 4);
        #else // OSX and Linux.
            _path[0] = '/';
            _path[1] = '\0';
        #endif
    }

    /// '/foo/bar/ ' -> '/foo/bar'. Modifies input string.
    DM_INLINE char* trimDirPath(char _path[DM_PATH_LEN])
    {
        size_t end = strlen(_path);
        while (--end)
        {
            if (_path[end] != ' '
            &&  _path[end] != '/'
            &&  _path[end] != '\\'
            )
            {
                _path[end+1] = '\0';
                break;
            }
        }
        return _path;
    }

    DM_INLINE uint32_t windowsDrives()
    {
    #if BX_PLATFORM_WINDOWS
        return GetLogicalDrives();
    #else
        return 0;
    #endif // BX_PLATFORM_WINDOWS
    }

    /// Gets file name without extension from file path. Examples:
    ///     /tmp/foo.c -> foo
    ///     C:\\tmp\\foo.c -> foo
    DM_INLINE bool basename(char* _out, size_t _outSize, const char* _filePath)
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
            const size_t size = dm::min(size_t(end-begin)+1, _outSize);
            dm::strscpy(_out, begin, size);
            return true;
        }

        return false;
    }

    DM_INLINE const char* fileExtension(const char* _filePath)
    {
        const char* dot = strrchr(_filePath, '.');
        const char* ext = (NULL != dot) ? ++dot : _filePath;
        return ext;
    }

    DM_INLINE long int fileExists(const char* _file)
    {
        FILE* file = fopen(_file, "rb");
        if (NULL == file)
        {
            return false;
        }
        else
        {
            fclose(file);
            return true;
        }
    }

    DM_INLINE long int fsize(FILE* _file)
    {
        long int pos = ftell(_file);
        fseek(_file, 0L, SEEK_END);
        long int size = ftell(_file);
        fseek(_file, pos, SEEK_SET);
        return size;
    }

    // Inherit.
    //-----

    struct NoCopyNoAssign
    {
    protected:
        NoCopyNoAssign() { }
        ~NoCopyNoAssign() { }
    private:
        NoCopyNoAssign(const NoCopyNoAssign&);
        const NoCopyNoAssign& operator=(const NoCopyNoAssign&);
    };


    // Scope.
    //-----

    struct ScopeFclose : NoCopyNoAssign
    {
        ScopeFclose(FILE* _fp)
            : m_fp(_fp)
        {
        }

        ~ScopeFclose()
        {
            if (m_fp)
            {
                fclose(m_fp);
            }
        }

    private:
        FILE* m_fp;
    };

    template <typename Ty>
    struct ScopeUnload : NoCopyNoAssign
    {
        ScopeUnload(Ty& _ptr)
            : m_ptr(_ptr)
        {
        }

        ~ScopeUnload()
        {
            m_ptr.unload();
        }

    private:
        Ty& m_ptr;
    };

} // namespace dm

#endif // DM_MISC_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
