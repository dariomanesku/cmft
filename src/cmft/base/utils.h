/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_UTILS_H_HEADER_GUARD
#define CMFT_UTILS_H_HEADER_GUARD

#include <stdio.h>
#include <stdint.h>
#include <string.h> //memcpy
#include <ctype.h>  //tolower
#include <math.h>   //log2
#include <malloc.h> //alloca

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifdef max
    #undef max
#endif

#ifdef min
    #undef min
#endif

namespace cmft
{
#if defined(_MSC_VER)
    static inline float fminf(float _a, float _b)
    {
        return _a < _b ? _a : _b;
    }

    static inline float fmaxf(float _a, float _b)
    {
        return _a > _b ? _a : _b;
    }

    // TODO: use c++11 log2 from <cmath.h>
    static inline float log2f(float _val)
    {
        return logf(_val)/logf(2.0f);
    }
#endif //defined(_MSC_VER)

    template <typename Ty>
    static inline Ty max(Ty _a, Ty _b)
    {
        return (_a > _b) ? _a : _b;
    }

    template <typename Ty>
    static inline Ty min(Ty _a, Ty _b)
    {
        return (_a > _b) ? _b : _a;
    }

    /// Assumes _min < _max.
    template <typename Ty>
    static inline Ty clamp(Ty _val, Ty _min, Ty _max)
    {
        return ( _val > _max ? _max
               : _val < _min ? _min
               : _val
               );
    }

    static inline void cmft_swap(uint8_t* _a, uint8_t* _b, uint32_t _size)
    {
        uint8_t* c = (uint8_t*)alloca(_size);
        memcpy( c, _a, _size);
        memcpy(_a, _b, _size);
        memcpy(_b,  c, _size);
    }

    static inline uint32_t alignf(float _val, uint32_t _align)
    {
        return uint32_t(_val/float(int32_t(_align)))*_align;
    }

    static inline uint32_t align(uint32_t _val, uint32_t _alignPow2)
    {
        const uint32_t mask = _alignPow2-uint32_t(1);
        return (_val+mask)&(~mask);
    }

    static inline long int fsize(FILE* _file)
    {
        long int pos = ftell(_file);
        fseek(_file, 0L, SEEK_END);
        long int size = ftell(_file);
        fseek(_file, pos, SEEK_SET);
        return size;
    }

    static inline void strtolower(char* _out, char* _in)
    {
        for( ; *_in; ++_in)
        {
            *_out++ = (char)tolower(*_in);
        }
        *_out = '\0';
    }

    static inline void strtolower(char* _str)
    {
        for( ; *_str; ++_str)
        {
            *_str = (char)tolower(*_str);
        }
        *_str = '\0';
    }

    static inline void strtoupper(char* _out, char* _in)
    {
        for( ; *_in; ++_in)
        {
            *_out++ = (char)toupper(*_in);
        }
        *_out = '\0';
    }

    static inline void strtoupper(char* _str)
    {
        for( ; *_str; ++_str)
        {
            *_str = (char)toupper(*_str);
        }
        *_str = '\0';
    }

    static inline void cmft_strscpy(char* _dst, const char* _src, size_t _dstSize)
    {
        if (NULL != _src)
        {
            _dst[0] = '\0';
            strncat(_dst, _src, _dstSize-1);
        }
    }

    /// Gets file name without extension from file path. Examples:
    ///     /tmp/foo.c -> foo
    ///     C:\\tmp\\foo.c -> foo
    static inline bool getFileName(char* _out, size_t _outSize, const char* _filePath)
    {
       const char *begin;
       const char *end;

       const char *ptr;
       begin = NULL != (ptr = strrchr(_filePath, '\\')) ? ++ptr
             : NULL != (ptr = strrchr(_filePath, '/' )) ? ++ptr
             : _filePath
             ;

       end = NULL != (ptr = strrchr(_filePath, '.')) ? ptr : strrchr(_filePath, '\0');

       if (NULL != begin && NULL != end)
       {
           const size_t size = min(size_t(end-begin)+1, _outSize);
           cmft_strscpy(_out, begin, size);
           return true;
       }

       return false;
    }

    struct NoCopyNoAssign
    {
    protected:
        NoCopyNoAssign() { }
        ~NoCopyNoAssign() { }
    private:
        NoCopyNoAssign(const NoCopyNoAssign&);
        const NoCopyNoAssign& operator=(const NoCopyNoAssign&);
    };

    struct ScopeFclose : NoCopyNoAssign
    {
        ScopeFclose(FILE* _fp) : m_fp(_fp) { }

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

    #ifndef CMFT_FREE
        #define CMFT_FREE(_ptr) ::free(_ptr)
    #endif

    struct ScopeFree : NoCopyNoAssign
    {
        ScopeFree(void* _ptr) : m_ptr(_ptr) { }

        ~ScopeFree()
        {
            if (m_ptr)
            {
                CMFT_FREE(m_ptr);
            }
        }

    private:
        void* m_ptr;
    };

    template <typename Ty>
    struct ScopeUnload : NoCopyNoAssign
    {
        ScopeUnload(Ty& _ptr) : m_ptr(_ptr) { }

        ~ScopeUnload()
        {
            m_ptr.unload();
        }

    private:
        Ty& m_ptr;
    };

} // namespace cmft

#endif //CMFT_UTILS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
