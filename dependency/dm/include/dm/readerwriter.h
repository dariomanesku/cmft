/*
 * Copyright 2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef DM_READERWRITER_H_HEADER_GUARD
#define DM_READERWRITER_H_HEADER_GUARD

#include <stdint.h>

#include "../../3rdparty/bx/macros.h"       // BX_NO_VTABLE
#include "../../3rdparty/bx/uint32_t.h"     // bx::int64_clamp(), bx::uint32_min()
#include "../../3rdparty/bx/readerwriter.h" // bx::ReaderI, bx::SeekerI

namespace dm
{
    struct ReaderWriterTypes
    {
        enum Enum
        {
            Undefined,
            MemoryReader,
            CrtFileReader,
        };
    };

    struct BX_NO_VTABLE TypeInfo
    {
        virtual uint8_t getType() const
        {
            return 0;
        }
    };

    struct BX_NO_VTABLE ReaderSeekerI : public bx::ReaderSeekerI, public dm::TypeInfo
    {
    };

    class MemoryReader : public dm::ReaderSeekerI
    {
    public:
        MemoryReader(const void* _data, uint32_t _size)
            : m_data( (const uint8_t*)_data)
            , m_pos(0)
            , m_top(_size)
        {
        }

        virtual ~MemoryReader()
        {
        }

        virtual uint8_t getType() const
        {
            return ReaderWriterTypes::MemoryReader;
        }

        virtual int64_t seek(int64_t _offset, bx::Whence::Enum _whence) BX_OVERRIDE
        {
            switch (_whence)
            {
                case bx::Whence::Begin:
                    m_pos = _offset;
                    break;

                case bx::Whence::Current:
                    m_pos = bx::int64_clamp(m_pos + _offset, 0, m_top);
                    break;

                case bx::Whence::End:
                    m_pos = bx::int64_clamp(m_top - _offset, 0, m_top);
                    break;
            }

            return m_pos;
        }

        virtual int32_t read(void* _data, int32_t _size) BX_OVERRIDE
        {
            int64_t reminder = m_top-m_pos;
            int32_t size = bx::uint32_min(_size, int32_t(reminder > INT32_MAX ? INT32_MAX : reminder) );
            memcpy(_data, &m_data[m_pos], size);
            m_pos += size;
            return size;
        }

        const uint8_t* getDataPtr() const
        {
            return &m_data[m_pos];
        }

        int64_t getPos() const
        {
            return m_pos;
        }

        int64_t remaining() const
        {
            return m_top-m_pos;
        }

    private:
        const uint8_t* m_data;
        int64_t m_pos;
        int64_t m_top;
    };

    struct BX_NO_VTABLE FileReaderI : public dm::ReaderSeekerI
    {
        virtual int32_t open(const char* _filePath, bool _binary = true) = 0;
        virtual int32_t close() = 0;
    };

    class CrtFileReader : public dm::FileReaderI
    {
    public:
        CrtFileReader()
            : m_file(NULL)
        {
        }

        virtual ~CrtFileReader()
        {
        }

        virtual uint8_t getType() const
        {
            return ReaderWriterTypes::CrtFileReader;
        }

        virtual int32_t open(const char* _filePath, bool _binary = true) BX_OVERRIDE
        {
            strcpy(m_path, _filePath);
            m_file = fopen(_filePath, _binary?"rb":"r");
            return NULL == m_file;
        }

        virtual int32_t close() BX_OVERRIDE
        {
            fclose(m_file);
            return 0;
        }

        virtual int64_t seek(int64_t _offset = 0, bx::Whence::Enum _whence = bx::Whence::Current) BX_OVERRIDE
        {
            fseeko64(m_file, _offset, _whence);
            return ftello64(m_file);
        }

        virtual int32_t read(void* _data, int32_t _size) BX_OVERRIDE
        {
            return (int32_t)fread(_data, 1, _size, m_file);
        }

        const char* getPath() const
        {
            return m_path;
        }

    private:
        FILE* m_file;
        char m_path[4096];
    };

} // namespace dm

#endif // DM_READERWRITER_H_HEADER_GUARD
