/*
 * Copyright 2014-2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <cmft/image.h>
#include <cmft/allocator.h>

#include "common/config.h"
#include "common/utils.h"
#include "common/halffloat.h"
#include "common/stb_image.h"

#include "cubemaputils.h"

#include <string.h>

namespace cmft
{
    // Read/write.
    //-----

    struct RwError
    {
        enum Enum
        {
            None,
            Open,
            Read,
            Write,
            Eof,
        };
    };

    struct RwType
    {
        enum Enum
        {
            Memory,
            FilePath,
        };
    };

    struct Rw
    {
        uint8_t m_error;
        uint8_t m_type;
        union
        {
            struct
            {
                const char* m_path;
                FILE* m_file;
            };

            struct
            {
                void* m_mem;
                size_t m_size;
                size_t m_offset;
            };
        };
    };

    struct Whence
    {
        enum Enum
        {
            Begin,
            Current,
            End,
        };
    };

    typedef int64_t (*RwSeekFn)(Rw* _rw, int64_t _offset, Whence::Enum _whence);
    typedef size_t  (*RwReadFn)(Rw* _rw, void* _data, size_t _size);

    void     rwInit(Rw* _rw, const char* _path);
    void     rwInit(Rw* _rw, FILE* _file);
    void     rwInit(Rw* _rw, void* _mem, size_t _size);
    bool     rwFileOpen(Rw* _rw, const char* _mode = "rb");
    bool     rwFileOpened(const Rw* _rw);
    void     rwFileClose(Rw* _rw);
    uint8_t  rwGetError(Rw* _rw);
    void     rwClearError(Rw* _rw);
    RwSeekFn rwSeekFnFor(const Rw* _rw);
    RwReadFn rwReadFnFor(const Rw* _rw);

    struct RwScopeFileClose
    {
        RwScopeFileClose(Rw* _rw, bool _condition = true)
        {
            m_rw = _rw;
            m_condition = _condition;
        }

        ~RwScopeFileClose()
        {
            if (m_condition)
            {
                rwFileClose(m_rw);
            }
        }
    private:
        Rw* m_rw;
        bool m_condition;
    };

    void rwInit(Rw* _rw, void* _mem, size_t _size)
    {
        _rw->m_error = RwError::None;
        _rw->m_type  = RwType::Memory;
        _rw->m_mem   = _mem;
        _rw->m_size  = _size;
    }

    void rwInit(Rw* _rw, FILE* _file)
    {
        _rw->m_error = RwError::None;
        _rw->m_type  = RwType::FilePath;
        _rw->m_path  = NULL;
        _rw->m_file  = _file;
    }

    void rwInit(Rw* _rw, const char* _path)
    {
        _rw->m_error = RwError::None;
        _rw->m_type  = RwType::FilePath;
        _rw->m_path  = _path;
        _rw->m_file  = NULL;
    }

    bool rwFileOpen(Rw* _rw, const char* _mode)
    {
        if (RwType::FilePath == _rw->m_type
        &&  NULL == _rw->m_file)
        {
            FILE* file = fopen(_rw->m_path, _mode);
            if (NULL != file)
            {
                _rw->m_error = 0;
                _rw->m_file  = file;

                return true;
            }
            else
            {
                _rw->m_error = RwError::Open; // Error opening file.

                return false;
            }
        }

        return false;
    }

    bool rwFileOpened(const Rw* _rw)
    {
        return (NULL != _rw->m_file);
    }

    void rwFileClose(Rw* _rw)
    {
        if (NULL != _rw->m_file)
        {
            int result = fclose(_rw->m_file);
            if (0 == result)
            {
                _rw->m_file = NULL;
            }
            else
            {
                _rw->m_error = RwError::Eof; // Error closing file.
            }
        }
    }

    uint8_t rwGetError(Rw* _rw)
    {
        return _rw->m_error;
    }

    void rwClearError(Rw* _rw)
    {
        _rw->m_error = 0;
    }

    #if CMFT_COMPILER_MSVC
    #   define fseeko64 _fseeki64
    #   define ftello64 _ftelli64
    #elif CMFT_PLATFORM_APPLE
    #   define fseeko64 fseeko
    #   define ftello64 ftello
    #endif // CMFT_

    int64_t rwSeekFile(Rw* _rw, int64_t _offset = 0, Whence::Enum _whence = Whence::Current)
    {
        fseeko64(_rw->m_file, _offset, _whence);
        return ftello64(_rw->m_file);
    }

    int64_t rwSeekMem(Rw* _rw, int64_t _offset = 0, Whence::Enum _whence = Whence::Current)
    {
        int64_t offset;
        if (Whence::Begin == _whence)
        {
            offset = _offset;
        }
        else if (Whence::Current == _whence)
        {
            offset = int64_t(_rw->m_offset) + _offset;
        }
        else /*.if (Whence::End == _whence).*/
        {
            offset = int64_t(_rw->m_size) - _offset;
        }
        offset = CMFT_CLAMP(offset, 0, int64_t(_rw->m_size));

        _rw->m_offset = offset;

        return offset;
    }

    RwSeekFn rwSeekFnFor(const Rw* _rw)
    {
        if (RwType::Memory == _rw->m_type)
        {
            return rwSeekMem;
        }
        else
        {
            return rwSeekFile;
        }
    }

    size_t rwReadFile(Rw* _src, void* _data, size_t _size)
    {
        const size_t size = fread(_data, 1, _size, _src->m_file);
        if (size != _size)
        {
            if (0 != feof(_src->m_file))
            {
                _src->m_error = RwError::Eof;
            }
            else if (0 != ferror(_src->m_file))
            {
                _src->m_error = RwError::Read;
            }
        }

        return size;
    }

    size_t rwReadMem(Rw* _rw, void* _data, size_t _size)
    {
        const size_t remainder  = _rw->m_size - _rw->m_offset;
        const size_t sizeToRead = _size < remainder ? _size : remainder;
        if (_size != sizeToRead)
        {
            _rw->m_error = RwError::Read; // Size truncated.
        }
        _rw->m_offset += sizeToRead;

        memcpy(_data, _rw->m_mem, sizeToRead);

        return sizeToRead;
    }

    RwReadFn rwReadFnFor(const Rw* _rw)
    {
        if (RwType::Memory == _rw->m_type)
        {
            return rwReadMem;
        }
        else
        {
            return rwReadFile;
        }
    }

    // Texture format string.
    //-----

    static const char* s_textureFormatStr[TextureFormat::Count] =
    {
        "BGR8",    //BGR8
        "RGB8",    //RGB8
        "RGB16",   //RGB16
        "RGB16F",  //RGB16F
        "RGB32F",  //RGB32F
        "RGBE",    //RGBE
        "BGRA8",   //BGRA8
        "RGBA8",   //RGBA8
        "RGBA16",  //RGBA16
        "RGBA16F", //RGBA16F
        "RGBA32F", //RGBA32F
        "RGBM",    //RGBM
    };

    const char* getTextureFormatStr(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(_format < TextureFormat::Count, "Reading array out of bounds!");
        return s_textureFormatStr[uint8_t(_format)];
    }

    // Image file type extension.
    //-----

    static const char* s_imageFileTypeExtension[ImageFileType::Count] =
    {
        ".dds", //DDS
        ".ktx", //KTX
        ".tga", //TGA
        ".hdr", //HDR
    };

    const char* getFilenameExtensionStr(ImageFileType::Enum _ft)
    {
        DEBUG_CHECK(_ft < ImageFileType::Count, "Reading array out of bounds!");
        return s_imageFileTypeExtension[uint8_t(_ft)];
    }

    // Image file type name.
    //-----

    static const char* s_imageFileTypeName[ImageFileType::Count] =
    {
        "DDS", //DDS
        "KTX", //KTX
        "TGA", //TGA
        "HDR", //HDR
    };

    const char* getFileTypeStr(ImageFileType::Enum _ft)
    {
        DEBUG_CHECK(_ft < ImageFileType::Count, "Reading array out of bounds!");
        return s_imageFileTypeName[uint8_t(_ft)];
    }

    // Image output type name.
    //-----

    static const char* s_outputTypeStr[OutputType::Count] =
    {
        "LatLong",
        "Cubemap",
        "HCross",
        "VCross",
        "HStrip",
        "VStrip",
        "FaceList",
        "Octant",
    };

    const char* getOutputTypeStr(OutputType::Enum _outputType)
    {
        DEBUG_CHECK(_outputType < OutputType::Count, "Reading array out of bounds!");
        return s_outputTypeStr[uint8_t(_outputType)];
    }

    // Cubemap faceId names.
    //-----

    static const char* s_cubemapFaceIdStr[6] =
    {
        "posx",
        "negx",
        "posy",
        "negy",
        "posz",
        "negz",
    };

    const char* getCubemapFaceIdStr(uint8_t _face)
    {
        DEBUG_CHECK(_face < 6, "Reading array out of bounds!");
        return s_cubemapFaceIdStr[_face];
    }


    // Valid output types.
    //-----

    static const OutputType::Enum s_ddsValidOutputTypes[] =
    {
        OutputType::LatLong,
        OutputType::Cubemap,
        OutputType::HCross,
        OutputType::VCross,
        OutputType::HStrip,
        OutputType::VStrip,
        OutputType::FaceList,
        OutputType::Octant,
        OutputType::Null,
    };

    static const OutputType::Enum s_ktxValidOutputTypes[] =
    {
        OutputType::LatLong,
        OutputType::Cubemap,
        OutputType::HCross,
        OutputType::VCross,
        OutputType::HStrip,
        OutputType::VStrip,
        OutputType::FaceList,
        OutputType::Octant,
        OutputType::Null,
    };

    static const OutputType::Enum s_tgaValidOutputTypes[] =
    {
        OutputType::LatLong,
        OutputType::HCross,
        OutputType::VCross,
        OutputType::HStrip,
        OutputType::VStrip,
        OutputType::FaceList,
        OutputType::Octant,
        OutputType::Null,
    };

    static const OutputType::Enum s_hdrValidOutputTypes[] =
    {
        OutputType::LatLong,
        OutputType::HCross,
        OutputType::VCross,
        OutputType::HStrip,
        OutputType::VStrip,
        OutputType::FaceList,
        OutputType::Octant,
        OutputType::Null,
    };

    const OutputType::Enum* getValidOutputTypes(ImageFileType::Enum _fileType)
    {
        if (ImageFileType::DDS == _fileType)
        {
            return s_ddsValidOutputTypes;
        }
        else if (ImageFileType::KTX == _fileType)
        {
            return s_ktxValidOutputTypes;
        }
        else if (ImageFileType::TGA == _fileType)
        {
            return s_tgaValidOutputTypes;
        }
        else if (ImageFileType::HDR == _fileType)
        {
            return s_hdrValidOutputTypes;
        }
        else
        {
            return NULL;
        }
    }

    void getValidOutputTypesStr(char* _str, ImageFileType::Enum _fileType)
    {
        const OutputType::Enum* validOutputTypes = getValidOutputTypes(_fileType);
        if (NULL == validOutputTypes)
        {
            _str[0] = '\0';
            return;
        }

        uint8_t ii = 0;
        OutputType::Enum curr;
        if (OutputType::Null != (curr = validOutputTypes[ii++]))
        {
            strcpy(_str, getOutputTypeStr(curr));
        }
        while (OutputType::Null != (curr = validOutputTypes[ii++]))
        {
            strcat(_str, " ");
            strcat(_str, getOutputTypeStr(curr));
        }
    }

    static bool contains(OutputType::Enum _format, const OutputType::Enum* _formatList)
    {
        OutputType::Enum curr;
        uint8_t ii = 0;
        while (OutputType::Null != (curr = _formatList[ii++]))
        {
            if (curr == _format)
            {
                return true;
            }
        }

        return false;
    };

    bool checkValidOutputType(ImageFileType::Enum _fileType, OutputType::Enum _outputType)
    {
        if (ImageFileType::DDS == _fileType)
        {
            return contains(_outputType, s_ddsValidOutputTypes);
        }
        else if (ImageFileType::KTX == _fileType)
        {
            return contains(_outputType, s_ktxValidOutputTypes);
        }
        else if (ImageFileType::TGA == _fileType)
        {
            return contains(_outputType, s_tgaValidOutputTypes);
        }
        else if (ImageFileType::HDR == _fileType)
        {
            return contains(_outputType, s_hdrValidOutputTypes);
        }

        return false;
    }

    // Valid texture formats.
    //-----

    static const TextureFormat::Enum s_ddsValidFormats[] =
    {
        TextureFormat::BGR8,
        TextureFormat::BGRA8,
        TextureFormat::RGBA16,
        TextureFormat::RGBA16F,
        TextureFormat::RGBA32F,
        TextureFormat::RGBM,
        TextureFormat::Null,
    };

    static const TextureFormat::Enum s_ktxValidFormats[] =
    {
        TextureFormat::RGB8,
        TextureFormat::RGB16,
        TextureFormat::RGB16F,
        TextureFormat::RGB32F,
        TextureFormat::RGBA8,
        TextureFormat::RGBA16,
        TextureFormat::RGBA16F,
        TextureFormat::RGBA32F,
        TextureFormat::Null,
    };

    static const TextureFormat::Enum s_tgaValidFormats[] =
    {
        TextureFormat::BGR8,
        TextureFormat::BGRA8,
        TextureFormat::RGBM,
        TextureFormat::Null,
    };

    static const TextureFormat::Enum s_hdrValidFormats[] =
    {
        TextureFormat::RGBE,
        TextureFormat::Null,
    };

    const TextureFormat::Enum* getValidTextureFormats(ImageFileType::Enum _fileType)
    {
        if (ImageFileType::DDS == _fileType)
        {
            return s_ddsValidFormats;
        }
        else if (ImageFileType::KTX == _fileType)
        {
            return s_ktxValidFormats;
        }
        else if (ImageFileType::TGA == _fileType)
        {
            return s_tgaValidFormats;
        }
        else if (ImageFileType::HDR == _fileType)
        {
            return s_hdrValidFormats;
        }
        else
        {
            return NULL;
        }
    }

    void getValidTextureFormatsStr(char* _str, ImageFileType::Enum _fileType)
    {
        const TextureFormat::Enum* validFormatsList = getValidTextureFormats(_fileType);
        if (NULL == validFormatsList)
        {
            _str[0] = '\0';
            return;
        }

        uint8_t ii = 0;
        TextureFormat::Enum curr;
        if (TextureFormat::Null != (curr = validFormatsList[ii++]))
        {
            strcpy(_str, getTextureFormatStr(curr));
        }
        while (TextureFormat::Null != (curr = validFormatsList[ii++]))
        {
            strcat(_str, " ");
            strcat(_str, getTextureFormatStr(curr));
        }
    }

    static bool contains(TextureFormat::Enum _format, const TextureFormat::Enum* _formatList)
    {
        TextureFormat::Enum curr;
        uint8_t ii = 0;
        while (TextureFormat::Null != (curr = _formatList[ii++]))
        {
            if (curr == _format)
            {
                return true;
            }
        }

        return false;
    };

    bool checkValidTextureFormat(ImageFileType::Enum _fileType, TextureFormat::Enum _textureFormat)
    {
        if (ImageFileType::DDS == _fileType)
        {
            return contains(_textureFormat, s_ddsValidFormats);
        }
        else if (ImageFileType::KTX == _fileType)
        {
            return contains(_textureFormat, s_ktxValidFormats);
        }
        else if (ImageFileType::TGA == _fileType)
        {
            return contains(_textureFormat, s_tgaValidFormats);
        }
        else if (ImageFileType::HDR == _fileType)
        {
            return contains(_textureFormat, s_hdrValidFormats);
        }

        return false;
    }

    // Image data info.
    //-----

    struct PixelDataType
    {
        enum Enum
        {
            UINT8,
            UINT16,
            UINT32,
            HALF_FLOAT,
            FLOAT,

            Count,
        };
    };

    static const ImageDataInfo s_imageDataInfo[TextureFormat::Count] =
    {
        {  3, 3, 0, PixelDataType::UINT8       }, //BGR8
        {  3, 3, 0, PixelDataType::UINT8       }, //RGB8
        {  6, 3, 0, PixelDataType::UINT16      }, //RGB16
        {  6, 3, 0, PixelDataType::HALF_FLOAT  }, //RGB16F
        { 12, 3, 0, PixelDataType::FLOAT       }, //RGB32F
        {  4, 4, 0, PixelDataType::UINT8       }, //RGBE
        {  4, 4, 1, PixelDataType::UINT8       }, //BGRA8
        {  4, 4, 1, PixelDataType::UINT8       }, //RGBA8
        {  8, 4, 1, PixelDataType::UINT16      }, //RGBA16
        {  8, 4, 1, PixelDataType::HALF_FLOAT  }, //RGBA16F
        { 16, 4, 1, PixelDataType::FLOAT       }, //RGBA32F
    };

    const ImageDataInfo& getImageDataInfo(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(_format < TextureFormat::Count, "Reading array out of bounds!");
        return s_imageDataInfo[_format];
    }

    uint8_t getNaturalAlignment(TextureFormat::Enum _format)
    {
        const uint8_t bytesPerPixel = getImageDataInfo(_format).m_bytesPerPixel;
        const uint8_t numChannels   = getImageDataInfo(_format).m_numChanels;

        return bytesPerPixel/numChannels;
    }

    // HDR format.
    //-----

    #define HDR_VALID_PROGRAMTYPE 0x01
    #define HDR_VALID_GAMMA       0x02
    #define HDR_VALID_EXPOSURE    0x04
    #define HDR_MAGIC             CMFT_MAKEFOURCC('#','?','R','A')
    #define HDR_MAGIC_FULL        "#?RADIANCE"
    #define HDR_MAGIC_LEN         10

    struct HdrHeader
    {
        int m_valid;
        float m_gamma;
        float m_exposure;
    };

    // TGA format.
    //-----

    #define TGA_HEADER_SIZE 18
    #define TGA_ID          { 'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0' }
    #define TGA_ID_LEN      18
    #define TGA_FOOTER_SIZE 26

    #define TGA_IT_NOIMAGE     0x0
    #define TGA_IT_COLORMAPPED 0x1
    #define TGA_IT_RGB         0x2
    #define TGA_IT_BW          0x3
    #define TGA_IT_RLE         0x8

    #define TGA_DESC_HORIZONTAL 0x10
    #define TGA_DESC_VERTICAL   0x20

    struct TgaHeader
    {
        uint8_t m_idLength;
        uint8_t m_colorMapType;
        uint8_t m_imageType;
        int16_t m_colorMapOrigin;
        int16_t m_colorMapLength;
        uint8_t m_colorMapDepth;
        int16_t m_xOrigin;
        int16_t m_yOrigin;
        uint16_t m_width;
        uint16_t m_height;
        uint8_t m_bitsPerPixel;
        uint8_t m_imageDescriptor;
    };

    struct TgaFooter
    {
        uint32_t m_extensionOffset;
        uint32_t m_developerOffset;
        uint8_t m_signature[18];
    };

    // DDS format.
    //-----

    #define DDS_MAGIC             CMFT_MAKEFOURCC('D', 'D', 'S', ' ')
    #define DDS_HEADER_SIZE       124
    #define DDS_IMAGE_DATA_OFFSET (DDS_HEADER_SIZE + 4)
    #define DDS_PIXELFORMAT_SIZE  32
    #define DDS_DX10_HEADER_SIZE  20

    #define DDS_DXT1 CMFT_MAKEFOURCC('D', 'X', 'T', '1')
    #define DDS_DXT2 CMFT_MAKEFOURCC('D', 'X', 'T', '2')
    #define DDS_DXT3 CMFT_MAKEFOURCC('D', 'X', 'T', '3')
    #define DDS_DXT4 CMFT_MAKEFOURCC('D', 'X', 'T', '4')
    #define DDS_DXT5 CMFT_MAKEFOURCC('D', 'X', 'T', '5')
    #define DDS_ATI1 CMFT_MAKEFOURCC('A', 'T', 'I', '1')
    #define DDS_BC4U CMFT_MAKEFOURCC('B', 'C', '4', 'U')
    #define DDS_ATI2 CMFT_MAKEFOURCC('A', 'T', 'I', '2')
    #define DDS_BC5U CMFT_MAKEFOURCC('B', 'C', '5', 'U')

    #define DDS_DX10 CMFT_MAKEFOURCC('D', 'X', '1', '0')

    #define D3DFMT_R8G8B8        20
    #define D3DFMT_A8R8G8B8      21
    #define D3DFMT_X8R8G8B8      22
    #define D3DFMT_A8B8G8R8      32
    #define D3DFMT_X8B8G8R8      33
    #define D3DFMT_A16B16G16R16  36
    #define D3DFMT_A16B16G16R16F 113
    #define D3DFMT_A32B32G32R32F 116

    #define DDSD_CAPS                   0x00000001
    #define DDSD_HEIGHT                 0x00000002
    #define DDSD_WIDTH                  0x00000004
    #define DDSD_PITCH                  0x00000008
    #define DDSD_PIXELFORMAT            0x00001000
    #define DDSD_MIPMAPCOUNT            0x00020000
    #define DDSD_LINEARSIZE             0x00080000
    #define DDSD_DEPTH                  0x00800000

    #define DDPF_ALPHAPIXELS            0x00000001
    #define DDPF_ALPHA                  0x00000002
    #define DDPF_FOURCC                 0x00000004
    #define DDPF_INDEXED                0x00000020
    #define DDPF_RGB                    0x00000040
    #define DDPF_YUV                    0x00000200
    #define DDPF_LUMINANCE              0x00020000
    #define DDPF_RGBA                   (DDPF_RGB|DDPF_ALPHAPIXELS)
    #define DDPF_LUMINANCEA             (DDPF_LUMINANCE|DDPF_ALPHAPIXELS)
    #define DDS_PF_BC_24                0x00100000
    #define DDS_PF_BC_32                0x00200000
    #define DDS_PF_BC_48                0x00400000

    #define DDSCAPS_COMPLEX             0x00000008
    #define DDSCAPS_TEXTURE             0x00001000
    #define DDSCAPS_MIPMAP              0x00400000

    #define DDSCAPS2_CUBEMAP            0x00000200
    #define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
    #define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
    #define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
    #define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
    #define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
    #define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000

    #define DDS_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX|DDSCAPS2_CUBEMAP_NEGATIVEX \
                                 | DDSCAPS2_CUBEMAP_POSITIVEY|DDSCAPS2_CUBEMAP_NEGATIVEY \
                                 | DDSCAPS2_CUBEMAP_POSITIVEZ|DDSCAPS2_CUBEMAP_NEGATIVEZ )

    #define DDSCAPS2_VOLUME             0x00200000

    #define DXGI_FORMAT_UNKNOWN             0
    #define DXGI_FORMAT_R32G32B32A32_FLOAT  2
    #define DXGI_FORMAT_R16G16B16A16_FLOAT  10
    #define DXGI_FORMAT_R16G16B16A16_UINT   12
    #define DXGI_FORMAT_R8G8B8A8_UNORM      28
    #define DXGI_FORMAT_R8G8B8A8_UINT       30
    #define DXGI_FORMAT_B8G8R8A8_UNORM      87
    #define DXGI_FORMAT_B8G8R8X8_UNORM      88
    #define DXGI_FORMAT_B8G8R8A8_TYPELESS   90

    #define DDS_DIMENSION_TEXTURE1D 2
    #define DDS_DIMENSION_TEXTURE2D 3
    #define DDS_DIMENSION_TEXTURE3D 4

    #define DDS_ALPHA_MODE_UNKNOWN        0x0
    #define DDS_ALPHA_MODE_STRAIGHT       0x1
    #define DDS_ALPHA_MODE_PREMULTIPLIED  0x2
    #define DDS_ALPHA_MODE_OPAQUE         0x3
    #define DDS_ALPHA_MODE_CUSTOM         0x4

    #define D3D10_RESOURCE_MISC_GENERATE_MIPS      0x1L
    #define D3D10_RESOURCE_MISC_SHARED             0x2L
    #define D3D10_RESOURCE_MISC_TEXTURECUBE        0x4L
    #define D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX  0x10L
    #define D3D10_RESOURCE_MISC_GDI_COMPATIBLE     0x20L

    struct DdsPixelFormat
    {
        uint32_t m_size;
        uint32_t m_flags;
        uint32_t m_fourcc;
        uint32_t m_rgbBitCount;
        uint32_t m_rBitMask;
        uint32_t m_gBitMask;
        uint32_t m_bBitMask;
        uint32_t m_aBitMask;
    };

    struct DdsHeader
    {
        uint32_t m_size;
        uint32_t m_flags;
        uint32_t m_height;
        uint32_t m_width;
        uint32_t m_pitchOrLinearSize;
        uint32_t m_depth;
        uint32_t m_mipMapCount;
        uint32_t m_reserved1[11];
        DdsPixelFormat m_pixelFormat;
        uint32_t m_caps;
        uint32_t m_caps2;
        uint32_t m_caps3;
        uint32_t m_caps4;
        uint32_t m_reserved2;
    };

    struct DdsHeaderDxt10
    {
        uint32_t m_dxgiFormat;
        uint32_t m_resourceDimension;
        uint32_t m_miscFlags;
        uint32_t m_arraySize;
        uint32_t m_miscFlags2;
    };

    static const DdsPixelFormat s_ddsPixelFormat[] =
    {
        { sizeof(DdsPixelFormat), DDPF_RGB,    0,                     24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }, //BGR8
        { sizeof(DdsPixelFormat), DDPF_RGBA,   0,                     32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //BGRA8
        { sizeof(DdsPixelFormat), DDPF_FOURCC, DDS_DX10,              64, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //RGBA16
        { sizeof(DdsPixelFormat), DDPF_FOURCC, D3DFMT_A16B16G16R16F,  64, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //RGBA16F
        { sizeof(DdsPixelFormat), DDPF_FOURCC, D3DFMT_A32B32G32R32F, 128, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, //RGBA32F
    };

    static inline const DdsPixelFormat& getDdsPixelFormat(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(checkValidTextureFormat(ImageFileType::DDS, _format), "Not a valid DDS texture format!");
        if      (TextureFormat::BGR8    == _format) { return s_ddsPixelFormat[0];  }
        else if (TextureFormat::BGRA8   == _format) { return s_ddsPixelFormat[1];  }
        else if (TextureFormat::RGBA16  == _format) { return s_ddsPixelFormat[2];  }
        else if (TextureFormat::RGBA16F == _format) { return s_ddsPixelFormat[3];  }
        else/*(TextureFormat::RGBA32F == _format)*/ { return s_ddsPixelFormat[4];  }
    }

    static inline uint8_t getDdsDxgiFormat(TextureFormat::Enum _format)
    {
        if      (TextureFormat::RGBA16  == _format) { return DXGI_FORMAT_R16G16B16A16_UINT;  }
        else if (TextureFormat::RGBA16F == _format) { return DXGI_FORMAT_R16G16B16A16_FLOAT; }
        else if (TextureFormat::RGBA32F == _format) { return DXGI_FORMAT_R32G32B32A32_FLOAT; }
        else { return DXGI_FORMAT_UNKNOWN; }
    }

    static const struct TranslateDdsPfBitCount
    {
        uint32_t m_bitCount;
        uint32_t m_flag;
    } s_translateDdsPfBitCount[] =
    {
        { 24,  DDS_PF_BC_24 },
        { 32,  DDS_PF_BC_32 },
        { 48,  DDS_PF_BC_48 },
    };

    static const struct TranslateDdsFormat
    {
        uint32_t m_format;
        TextureFormat::Enum m_textureFormat;

    } s_translateDdsFormat[] =
    {
        { D3DFMT_R8G8B8,          TextureFormat::BGR8    },
        { D3DFMT_A8R8G8B8,        TextureFormat::BGRA8   },
        { D3DFMT_A16B16G16R16,    TextureFormat::RGBA16  },
        { D3DFMT_A16B16G16R16F,   TextureFormat::RGBA16F },
        { D3DFMT_A32B32G32R32F,   TextureFormat::RGBA32F },
        { DDS_PF_BC_24|DDPF_RGB,  TextureFormat::BGR8    },
        { DDS_PF_BC_32|DDPF_RGBA, TextureFormat::BGRA8   },
        { DDS_PF_BC_48|DDPF_RGB,  TextureFormat::RGB16   },
    };

    static const struct TranslateDdsDxgiFormat
    {
        uint8_t m_dxgiFormat;
        TextureFormat::Enum m_textureFormat;

    } s_translateDdsDxgiFormat[] =
    {
        { DXGI_FORMAT_R16G16B16A16_UINT,  TextureFormat::RGBA16  },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, TextureFormat::RGBA16F },
        { DXGI_FORMAT_R32G32B32A32_FLOAT, TextureFormat::RGBA32F },
    };

    // KTX format.
    //-----

    #define KTX_MAGIC             { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
    #define KTX_MAGIC_SHORT       0x58544BAB
    #define KTX_MAGIC_LEN         12
    #define KTX_ENDIAN_REF        0x04030201
    #define KTX_ENDIAN_REF_REV    0x01020304
    #define KTX_HEADER_SIZE       52
    #define KTX_UNPACK_ALIGNMENT  4

    // GL data type.
    #define GL_BYTE             0x1400
    #define GL_UNSIGNED_BYTE    0x1401
    #define GL_SHORT            0x1402
    #define GL_UNSIGNED_SHORT   0x1403
    #define GL_INT              0x1404
    #define GL_UNSIGNED_INT     0x1405
    #define GL_FLOAT            0x1406
    #define GL_HALF_FLOAT       0x140B
    #define GL_FIXED            0x140C

    // GL pixel format.
    #define GL_RGB              0x1907
    #define GL_RGBA             0x1908

    #define GL_RGB8             0x8051
    #define GL_RGBA8            0x8058
  
    #define GL_RGBA32F          0x8814
    #define GL_RGB32F           0x8815
    #define GL_RGBA16F          0x881A
    #define GL_RGB16F           0x881B
    #define GL_RGBA32UI         0x8D70
    #define GL_RGB32UI          0x8D71
    #define GL_RGBA16UI         0x8D76
    #define GL_RGB16UI          0x8D77
    #define GL_RGBA8UI          0x8D7C
    #define GL_RGB8UI           0x8D7D
    #define GL_RGBA32I          0x8D82
    #define GL_RGB32I           0x8D83
    #define GL_RGBA16I          0x8D88
    #define GL_RGB16I           0x8D89
    #define GL_RGBA8I           0x8D8E
    #define GL_RGB8I            0x8D8F

    struct KtxHeader
    {
        uint32_t m_endianness;
        uint32_t m_glType;
        uint32_t m_glTypeSize;
        uint32_t m_glFormat;
        uint32_t m_glInternalFormat;
        uint32_t m_glBaseInternalFormat;
        uint32_t m_pixelWidth;
        uint32_t m_pixelHeight;
        uint32_t m_pixelDepth;
        uint32_t m_numArrayElements;
        uint32_t m_numFaces;
        uint32_t m_numMips;
        uint32_t m_bytesKeyValue;
    };

    struct GlSizedInternalFormat
    {
        uint32_t m_glInternalFormat;
        uint32_t m_glFormat;
    };

    static const GlSizedInternalFormat s_glSizedInternalFormats[TextureFormat::Count] =
    {
        { 0, 0 }, //BGR8
        { GL_RGB8UI,   GL_RGB  }, //RGB8
        { GL_RGB16UI,  GL_RGB  }, //RGB16
        { GL_RGB16F,   GL_RGB  }, //RGB16F
        { GL_RGB32F,   GL_RGB  }, //RGB32F
        { 0, 0 }, //RGBE
        { 0, 0 }, //BGRA8
        { GL_RGBA8UI,  GL_RGBA }, //RGBA8
        { GL_RGBA16UI, GL_RGBA }, //RGBA16
        { GL_RGBA16F,  GL_RGBA }, //RGBA16F
        { GL_RGBA32F,  GL_RGBA }, //RGBA32F
    };

    static const GlSizedInternalFormat& getGlSizedInternalFormat(TextureFormat::Enum _format)
    {
        DEBUG_CHECK(_format < TextureFormat::Count, "Reading array out of bounds!");
        return s_glSizedInternalFormats[_format];
    }

    static const uint32_t s_pixelDataTypeToGlType[PixelDataType::Count] =
    {
        GL_UNSIGNED_BYTE,  // UINT8
        GL_UNSIGNED_SHORT, // UINT16
        GL_UNSIGNED_INT,   // UINT32
        GL_HALF_FLOAT,     // HALF_FLOAT
        GL_FLOAT,          // FLOAT
    };

    static uint32_t pixelDataTypeToGlType(PixelDataType::Enum _pdt)
    {
        DEBUG_CHECK(_pdt < PixelDataType::Count, "Reading array out of bounds!");
        return s_pixelDataTypeToGlType[_pdt];
    }

    static const struct TranslateKtxFormat
    {
        uint32_t m_glInternalFormat;
        TextureFormat::Enum m_textureFormat;

    } s_translateKtxFormat[] =
    {
        { GL_RGB,      TextureFormat::RGB8    },
        { GL_RGB8,     TextureFormat::RGB8    },
        { GL_RGB8UI,   TextureFormat::RGB8    },
        { GL_RGB16UI,  TextureFormat::RGB16   },
        { GL_RGB16F,   TextureFormat::RGB16F  },
        { GL_RGB32F,   TextureFormat::RGB32F  },
        { GL_RGBA,     TextureFormat::RGBA8   },
        { GL_RGBA8,    TextureFormat::RGBA8   },
        { GL_RGBA8UI,  TextureFormat::RGBA8   },
        { GL_RGBA16UI, TextureFormat::RGBA16  },
        { GL_RGBA16F,  TextureFormat::RGBA16F },
        { GL_RGBA32F,  TextureFormat::RGBA32F },
    };

    // Image -> format headers/footers.
    //-----

    // Notice: creates proper dds headers only for uncompressed formats.
    void ddsHeaderFromImage(DdsHeader& _ddsHeader, DdsHeaderDxt10* _ddsHeaderDxt10, const Image& _image)
    {
        const DdsPixelFormat& ddsPixelFormat = getDdsPixelFormat(_image.m_format);

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const bool hasMipMaps = _image.m_numMips > 1;
        const bool hasMultipleFaces = _image.m_numFaces > 0;
        const bool isCubemap = _image.m_numFaces == 6;

        if (DDS_DX10 == ddsPixelFormat.m_fourcc)
        {
            if (NULL != _ddsHeaderDxt10)
            {
                _ddsHeaderDxt10->m_dxgiFormat = getDdsDxgiFormat(_image.m_format);
                DEBUG_CHECK(0 != _ddsHeaderDxt10->m_dxgiFormat, "Dxt10 format should not be 0.");
                _ddsHeaderDxt10->m_resourceDimension = DDS_DIMENSION_TEXTURE2D;
                _ddsHeaderDxt10->m_miscFlags = isCubemap ? D3D10_RESOURCE_MISC_TEXTURECUBE : 0;
                _ddsHeaderDxt10->m_arraySize = 1;
                _ddsHeaderDxt10->m_miscFlags2 = 0;
            }
            else
            {
                WARN("Dds header dxt10 is required but it is NULL.");
            }
        }

        memset(&_ddsHeader, 0, sizeof(DdsHeader));
        _ddsHeader.m_size = DDS_HEADER_SIZE;
        _ddsHeader.m_flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
                           | (hasMipMaps ? DDSD_MIPMAPCOUNT : 0)
                           | DDSD_PITCH
                           ;
        _ddsHeader.m_height = _image.m_height;
        _ddsHeader.m_width = _image.m_width;
        _ddsHeader.m_pitchOrLinearSize = _image.m_width * bytesPerPixel;
        _ddsHeader.m_mipMapCount = _image.m_numMips;
        memcpy(&_ddsHeader.m_pixelFormat, &ddsPixelFormat, sizeof(DdsPixelFormat));
        _ddsHeader.m_caps = DDSCAPS_TEXTURE
                          | (hasMipMaps ? DDSCAPS_MIPMAP : 0)
                          | ((hasMipMaps | hasMultipleFaces) ? DDSCAPS_COMPLEX : 0)
                          ;
        _ddsHeader.m_caps2 = (isCubemap ? DDSCAPS2_CUBEMAP | DDS_CUBEMAP_ALLFACES : 0);
    }

    void printDdsHeader(const DdsHeader& _ddsHeader)
    {
        printf("ddsHeader.m_size                      = %u\n"
               "ddsHeader.m_flags                     = %u\n"
               "ddsHeader.m_height                    = %u\n"
               "ddsHeader.m_width                     = %u\n"
               "ddsHeader.m_pitchOrLinearSize         = %u\n"
               "ddsHeader.m_depth                     = %u\n"
               "ddsHeader.m_mipMapCount               = %u\n"
               "ddsHeader.m_reserved1[0]              = %u\n"
               "ddsHeader.m_pixelFormat.m_size        = %u\n"
               "ddsHeader.m_pixelFormat.m_flags       = %u\n"
               "ddsHeader.m_pixelFormat.m_fourcc      = %u\n"
               "ddsHeader.m_pixelFormat.m_rgbBitCount = %u\n"
               "ddsHeader.m_pixelFormat.m_rBitMask    = %u\n"
               "ddsHeader.m_pixelFormat.m_gBitMask    = %u\n"
               "ddsHeader.m_pixelFormat.m_bBitMask    = %u\n"
               "ddsHeader.m_pixelFormat.m_aBitMask    = %u\n"
               "ddsHeader.m_caps                      = %u\n"
               "ddsHeader.m_caps2                     = %u\n"
               "ddsHeader.m_caps3                     = %u\n"
               "ddsHeader.m_caps4                     = %u\n"
               "ddsHeader.m_reserved2                 = %u\n"
               , _ddsHeader.m_size
               , _ddsHeader.m_flags
               , _ddsHeader.m_height
               , _ddsHeader.m_width
               , _ddsHeader.m_pitchOrLinearSize
               , _ddsHeader.m_depth
               , _ddsHeader.m_mipMapCount
               , _ddsHeader.m_reserved1[0]
               , _ddsHeader.m_pixelFormat.m_size
               , _ddsHeader.m_pixelFormat.m_flags
               , _ddsHeader.m_pixelFormat.m_fourcc
               , _ddsHeader.m_pixelFormat.m_rgbBitCount
               , _ddsHeader.m_pixelFormat.m_rBitMask
               , _ddsHeader.m_pixelFormat.m_gBitMask
               , _ddsHeader.m_pixelFormat.m_bBitMask
               , _ddsHeader.m_pixelFormat.m_aBitMask
               , _ddsHeader.m_caps
               , _ddsHeader.m_caps2
               , _ddsHeader.m_caps3
               , _ddsHeader.m_caps4
               , _ddsHeader.m_reserved2
               );
    }

    void printDdsHeaderDxt10(const DdsHeaderDxt10& _ddsHeaderDxt10)
    {
        printf("ddsHeaderDxt10.m_dxgiFormat        = %u\n"
               "ddsHeaderDxt10.m_resourceDimension = %u\n"
               "ddsHeaderDxt10.m_miscFlags         = %u\n"
               "ddsHeaderDxt10.m_arraySize         = %u\n"
               "ddsHeaderDxt10.m_miscFlags2        = %u\n"
               , _ddsHeaderDxt10.m_dxgiFormat
               , _ddsHeaderDxt10.m_resourceDimension
               , _ddsHeaderDxt10.m_miscFlags
               , _ddsHeaderDxt10.m_arraySize
               , _ddsHeaderDxt10.m_miscFlags2
               );
    }

    void ktxHeaderFromImage(KtxHeader& _ktxHeader, const Image& _image)
    {
        const ImageDataInfo& imageDataInfo = getImageDataInfo(_image.m_format);

        _ktxHeader.m_endianness = KTX_ENDIAN_REF;
        _ktxHeader.m_glType = pixelDataTypeToGlType((PixelDataType::Enum)imageDataInfo.m_pixelType);
        _ktxHeader.m_glTypeSize = (imageDataInfo.m_bytesPerPixel/imageDataInfo.m_numChanels);
        _ktxHeader.m_glFormat = getGlSizedInternalFormat(_image.m_format).m_glFormat;
        _ktxHeader.m_glInternalFormat = getGlSizedInternalFormat(_image.m_format).m_glInternalFormat;
        _ktxHeader.m_glBaseInternalFormat = _ktxHeader.m_glFormat;
        _ktxHeader.m_pixelWidth = _image.m_width;
        _ktxHeader.m_pixelHeight = _image.m_height;
        _ktxHeader.m_pixelDepth = 0;
        _ktxHeader.m_numArrayElements = 0;
        _ktxHeader.m_numFaces = _image.m_numFaces;
        _ktxHeader.m_numMips = _image.m_numMips;
        _ktxHeader.m_bytesKeyValue = 0;

        DEBUG_CHECK(_ktxHeader.m_glTypeSize == 1
                 || _ktxHeader.m_glTypeSize == 2
                 || _ktxHeader.m_glTypeSize == 4
                  , "Invalid ktx header glTypeSize."
                  );
        DEBUG_CHECK(0 != _image.m_numMips, "Mips count cannot be 0.");
    }

    void printKtxHeader(const KtxHeader& _ktxHeader)
    {
        printf("ktxHeader.m_endianness       = %u\n"
               "ktxHeader.m_glType           = %u\n"
               "ktxHeader.m_glTypeSize       = %u\n"
               "ktxHeader.m_glFormat         = %u\n"
               "ktxHeader.m_glInternalFormat = %u\n"
               "ktxHeader.m_glBaseInternal   = %u\n"
               "ktxHeader.m_pixelWidth       = %u\n"
               "ktxHeader.m_pixelHeight      = %u\n"
               "ktxHeader.m_pixelDepth       = %u\n"
               "ktxHeader.m_numArrayElements = %u\n"
               "ktxHeader.m_numFaces         = %u\n"
               "ktxHeader.m_numMips          = %u\n"
               "ktxHeader.m_bytesKeyValue    = %u\n"
               , _ktxHeader.m_endianness
               , _ktxHeader.m_glType
               , _ktxHeader.m_glTypeSize
               , _ktxHeader.m_glFormat
               , _ktxHeader.m_glInternalFormat
               , _ktxHeader.m_glBaseInternalFormat
               , _ktxHeader.m_pixelWidth
               , _ktxHeader.m_pixelHeight
               , _ktxHeader.m_pixelDepth
               , _ktxHeader.m_numArrayElements
               , _ktxHeader.m_numFaces
               , _ktxHeader.m_numMips
               , _ktxHeader.m_bytesKeyValue
               );
    }

    void tgaHeaderFromImage(TgaHeader& _tgaHeader, const Image& _image, uint8_t _mip)
    {
        memset(&_tgaHeader, 0, sizeof(TgaHeader));
        _tgaHeader.m_idLength = 0;
        _tgaHeader.m_colorMapType = 0;
        _tgaHeader.m_imageType = TGA_IT_RGB;
        _tgaHeader.m_xOrigin = 0;
        _tgaHeader.m_yOrigin = 0;
        _tgaHeader.m_width  = uint16_t(CMFT_MAX(1, _image.m_width  >> _mip));
        _tgaHeader.m_height = uint16_t(CMFT_MAX(1, _image.m_height >> _mip));
        _tgaHeader.m_bitsPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel*8;
        _tgaHeader.m_imageDescriptor = (getImageDataInfo(_image.m_format).m_hasAlpha ? 0x8 : 0x0);
    }

    void printTgaHeader(const TgaHeader& _tgaHeader)
    {
        printf("tgaHeader.m_idLength        = %u\n"
               "tgaHeader.m_colorMapType    = %u\n"
               "tgaHeader.m_imageType       = %u\n"
               "tgaHeader.m_colorMapOrigin  = %d\n"
               "tgaHeader.m_colorMapLength  = %d\n"
               "tgaHeader.m_colorMapDepth   = %u\n"
               "tgaHeader.m_xOrigin         = %d\n"
               "tgaHeader.m_yOrigin         = %d\n"
               "tgaHeader.m_width           = %u\n"
               "tgaHeader.m_height          = %u\n"
               "tgaHeader.m_bitsPerPixel    = %u\n"
               "tgaHeader.m_imageDescriptor = %u\n"
               , _tgaHeader.m_idLength
               , _tgaHeader.m_colorMapType
               , _tgaHeader.m_imageType
               , _tgaHeader.m_colorMapOrigin
               , _tgaHeader.m_colorMapLength
               , _tgaHeader.m_colorMapDepth
               , _tgaHeader.m_xOrigin
               , _tgaHeader.m_yOrigin
               , _tgaHeader.m_width
               , _tgaHeader.m_height
               , _tgaHeader.m_bitsPerPixel
               , _tgaHeader.m_imageDescriptor
               );
    }

    void hdrHeaderFromImage(HdrHeader& _hdrHeader, const Image& _image)
    {
        CMFT_UNUSED(_image);

        memset(&_hdrHeader, 0, sizeof(HdrHeader));
        _hdrHeader.m_valid = HDR_VALID_GAMMA | HDR_VALID_EXPOSURE;
        _hdrHeader.m_gamma = 1.0f;
        _hdrHeader.m_exposure = 1.0f;
    }

    void printHdrHeader(const HdrHeader& _hdrHeader)
    {
        printf("hdrHeader.m_valid    = %d\n"
               "hdrHeader.m_gamma    = %f\n"
               "hdrHeader.m_exposure = %f\n"
               , _hdrHeader.m_valid
               , _hdrHeader.m_gamma
               , _hdrHeader.m_exposure
               );
    }

    // Image.
    //-----

    void imageCreate(Image& _image, uint32_t _width, uint32_t _height, uint32_t _rgba, uint8_t _numMips, uint8_t _numFaces, TextureFormat::Enum _format, AllocatorI* _allocator)
    {
        const uint8_t numFaces = _numFaces > 0 ? _numFaces : 1;
        const uint8_t numMips  = _numMips  > 0 ? _numMips  : 1;

        // Alloc data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        uint32_t dstDataSize = 0;
        for (uint8_t mip = 0; mip < numMips; ++mip)
        {
            const uint32_t mipWidth  = CMFT_MAX(UINT32_C(1), _width  >> mip);
            const uint32_t mipHeight = CMFT_MAX(UINT32_C(1), _height >> mip);
            dstDataSize += mipWidth * mipHeight * bytesPerPixel;
        }
        dstDataSize *= numFaces;
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get color in rgba32f format.
        float color[4];
        toRgba32f(color, TextureFormat::RGBA8, &_rgba);

        // Fill data with specified color.
        float* dstPtr = (float*)dstData;
        const float* end = (float*)((uint8_t*)dstData + dstDataSize);
        for (;dstPtr < end; dstPtr+=4)
        {
            dstPtr[0] = color[0];
            dstPtr[1] = color[1];
            dstPtr[2] = color[2];
            dstPtr[3] = color[3];
        }

        // Fill image structure.
        Image result;
        result.m_data = dstData;
        result.m_width = _width;
        result.m_height = _height;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = numMips;
        result.m_numFaces = numFaces;

        // Convert result to source format.
        if (TextureFormat::RGBA8 == _format)
        {
            imageMove(_image, result, _allocator);
        }
        else
        {
            imageConvert(_image, _format, result, _allocator);
            imageUnload(result, _allocator);
        }
    }

    void imageUnload(Image& _image, AllocatorI* _allocator)
    {
        if (_image.m_data)
        {
            CMFT_FREE(_allocator, _image.m_data);
            _image.m_data = NULL;
        }
    }

    void imageMove(Image& _dst, Image& _src, AllocatorI* _allocator)
    {
        imageUnload(_dst, _allocator);
        _dst.m_data     = _src.m_data;
        _dst.m_width    = _src.m_width;
        _dst.m_height   = _src.m_height;
        _dst.m_dataSize = _src.m_dataSize;
        _dst.m_format   = _src.m_format;
        _dst.m_numMips  = _src.m_numMips;
        _dst.m_numFaces = _src.m_numFaces;

        _src.m_data     = NULL;
    }

    void imageCopy(Image& _dst, const Image& _src, AllocatorI* _allocator)
    {
        imageUnload(_dst, _allocator);

        _dst.m_data = CMFT_ALLOC(_allocator, _src.m_dataSize);
        MALLOC_CHECK(_dst.m_data);
        memcpy(_dst.m_data, _src.m_data, _src.m_dataSize);
        _dst.m_width    = _src.m_width;
        _dst.m_height   = _src.m_height;
        _dst.m_dataSize = _src.m_dataSize;
        _dst.m_format   = _src.m_format;
        _dst.m_numMips  = _src.m_numMips;
        _dst.m_numFaces = _src.m_numFaces;
    }

    uint32_t imageGetNumPixels(const Image& _image)
    {
        DEBUG_CHECK(0 != _image.m_numMips,  "Mips count cannot be 0.");
        DEBUG_CHECK(0 != _image.m_numFaces, "Face count cannot be 0.");

        uint32_t count = 0;
        for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
        {
            const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
            const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
            count += width * height;
        }
        count *= _image.m_numFaces;

        return count;
    }

    void imageGetMipOffsets(uint32_t _offsets[CUBE_FACE_NUM][MAX_MIP_NUM], const Image& _image)
    {
        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;

        uint32_t offset = 0;
        for (uint8_t face = 0; face < _image.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
            {
                _offsets[face][mip] = offset;

                const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                offset += width * height * bytesPerPixel;
            }
        }
    }

    void imageGetFaceOffsets(uint32_t _faceOffsets[CUBE_FACE_NUM], const Image& _image)
    {
        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;

        uint32_t offset = 0;
        for (uint8_t face = 0; face < _image.m_numFaces; ++face)
        {
            _faceOffsets[face] = offset;

            for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
            {
                const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                offset += width * height * bytesPerPixel;
            }
        }
    }

    // To rgba32f.
    //-----

    inline void bgr8ToRgba32f(float* _rgba32f, const uint8_t* _bgr8)
    {
        _rgba32f[0] = float(_bgr8[2]) * (1.0f/255.0f);
        _rgba32f[1] = float(_bgr8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_bgr8[0]) * (1.0f/255.0f);
        _rgba32f[3] = 1.0f;
    }

    inline void bgra8ToRgba32f(float* _rgba32f, const uint8_t* _bgra8)
    {
        _rgba32f[0] = float(_bgra8[2]) * (1.0f/255.0f);
        _rgba32f[1] = float(_bgra8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_bgra8[0]) * (1.0f/255.0f);
        _rgba32f[3] = float(_bgra8[3]) * (1.0f/255.0f);
    }

    inline void rgb8ToRgba32f(float* _rgba32f, const uint8_t* _rgb8)
    {
        _rgba32f[0] = float(_rgb8[0]) * (1.0f/255.0f);
        _rgba32f[1] = float(_rgb8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_rgb8[2]) * (1.0f/255.0f);
        _rgba32f[3] = 1.0f;
    }

    inline void rgba8ToRgba32f(float* _rgba32f, const uint8_t* _rgba8)
    {
        _rgba32f[0] = float(_rgba8[0]) * (1.0f/255.0f);
        _rgba32f[1] = float(_rgba8[1]) * (1.0f/255.0f);
        _rgba32f[2] = float(_rgba8[2]) * (1.0f/255.0f);
        _rgba32f[3] = float(_rgba8[3]) * (1.0f/255.0f);
    }

    inline void rgb16ToRgba32f(float* _rgba32f, const uint16_t* _rgb16)
    {
        _rgba32f[0] = float(_rgb16[0]) * (1.0f/65535.0f);
        _rgba32f[1] = float(_rgb16[1]) * (1.0f/65535.0f);
        _rgba32f[2] = float(_rgb16[2]) * (1.0f/65535.0f);
        _rgba32f[3] = 1.0f;
    }

    inline void rgba16ToRgba32f(float* _rgba32f, const uint16_t* _rgba16)
    {
        _rgba32f[0] = float(_rgba16[0]) * (1.0f/65535.0f);
        _rgba32f[1] = float(_rgba16[1]) * (1.0f/65535.0f);
        _rgba32f[2] = float(_rgba16[2]) * (1.0f/65535.0f);
        _rgba32f[3] = float(_rgba16[3]) * (1.0f/65535.0f);
    }

    inline void rgb16fToRgba32f(float* _rgba32f, const uint16_t* _rgb16f)
    {
        _rgba32f[0] = cmft::halfToFloat(_rgb16f[0]);
        _rgba32f[1] = cmft::halfToFloat(_rgb16f[1]);
        _rgba32f[2] = cmft::halfToFloat(_rgb16f[2]);
        _rgba32f[3] = 1.0f;
    }

    inline void rgba16fToRgba32f(float* _rgba32f, const uint16_t* _rgba16f)
    {
        _rgba32f[0] = cmft::halfToFloat(_rgba16f[0]);
        _rgba32f[1] = cmft::halfToFloat(_rgba16f[1]);
        _rgba32f[2] = cmft::halfToFloat(_rgba16f[2]);
        _rgba32f[3] = cmft::halfToFloat(_rgba16f[3]);
    }

    inline void rgb32fToRgba32f(float* _rgba32f, const float* _rgb32f)
    {
        _rgba32f[0] = _rgb32f[0];
        _rgba32f[1] = _rgb32f[1];
        _rgba32f[2] = _rgb32f[2];
        _rgba32f[3] = 1.0f;
    }

    inline void rgba32fToRgba32f(float* _dst, const float* _src)
    {
        memcpy(_dst, _src, 4*sizeof(float));
    }

    inline void rgbeToRgba32f(float* _rgba32f, const uint8_t* _rgbe)
    {
        if (_rgbe[3])
        {
            const float exp = ldexp(1.0f, _rgbe[3] - (128+8));
            _rgba32f[0] = float(_rgbe[0]) * exp;
            _rgba32f[1] = float(_rgbe[1]) * exp;
            _rgba32f[2] = float(_rgbe[2]) * exp;
            _rgba32f[3] = 1.0f;
        }
        else
        {
            _rgba32f[0] = 0.0f;
            _rgba32f[1] = 0.0f;
            _rgba32f[2] = 0.0f;
            _rgba32f[3] = 1.0f;
        }
    }

    void toRgba32f(float _rgba32f[4], TextureFormat::Enum _srcFormat, const void* _src)
    {
        switch(_srcFormat)
        {
        case TextureFormat::BGR8:     bgr8ToRgba32f(_rgba32f,    (const  uint8_t*)_src); break;
        case TextureFormat::RGB8:     rgb8ToRgba32f(_rgba32f,    (const  uint8_t*)_src); break;
        case TextureFormat::RGB16:    rgb16ToRgba32f(_rgba32f,   (const uint16_t*)_src); break;
        case TextureFormat::RGB16F:   rgb16fToRgba32f(_rgba32f,  (const uint16_t*)_src); break;
        case TextureFormat::RGB32F:   rgb32fToRgba32f(_rgba32f,  (const    float*)_src); break;
        case TextureFormat::RGBE:     rgbeToRgba32f(_rgba32f,    (const  uint8_t*)_src); break;
        case TextureFormat::BGRA8:    bgra8ToRgba32f(_rgba32f,   (const  uint8_t*)_src); break;
        case TextureFormat::RGBA8:    rgba8ToRgba32f(_rgba32f,   (const  uint8_t*)_src); break;
        case TextureFormat::RGBA16:   rgba16ToRgba32f(_rgba32f,  (const uint16_t*)_src); break;
        case TextureFormat::RGBA16F:  rgba16fToRgba32f(_rgba32f, (const uint16_t*)_src); break;
        case TextureFormat::RGBA32F:  rgba32fToRgba32f(_rgba32f, (const    float*)_src); break;
        default: DEBUG_CHECK(false, "Unknown image format.");
        };
    }

    void imageToRgba32f(Image& _dst, const Image& _src, AllocatorI* _allocator)
    {
        // Alloc dst data.
        const uint32_t pixelCount = imageGetNumPixels(_src);
        const uint8_t dstBytesPerPixel = getImageDataInfo(TextureFormat::RGBA32F).m_bytesPerPixel;
        const uint32_t dataSize = pixelCount*dstBytesPerPixel;
        void* data = CMFT_ALLOC(_allocator, dataSize);
        MALLOC_CHECK(data);

        // Get total number of channels.
        const uint8_t numChannelsPerPixel = getImageDataInfo(TextureFormat::RGBA32F).m_numChanels;
        const uint32_t totalNumChannels = pixelCount*numChannelsPerPixel;

        // Convert each channel.
        float* dst = (float*)data;
        const float* end = (float*)dst + totalNumChannels;
        switch(_src.m_format)
        {
        case TextureFormat::BGR8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    bgr8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb16ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16F:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb16fToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB32F:
            {
                const float* src = (const float*)_src.m_data;

                for (;dst < end; dst+=4, src+=3)
                {
                    rgb32fToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBE:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgbeToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::BGRA8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    bgra8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA8:
            {
                const uint8_t* src = (const uint8_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgba8ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgba16ToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16F:
            {
                const uint16_t* src = (const uint16_t*)_src.m_data;

                for (;dst < end; dst+=4, src+=4)
                {
                    rgba16fToRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA32F:
            {
                // Copy data.
                memcpy(data, _src.m_data, dataSize);
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "Unknown image format.");
            }
        break;
        };

        // Fill image structure.
        Image result;
        result.m_data = data;
        result.m_width = _src.m_width;
        result.m_height = _src.m_height;
        result.m_dataSize = dataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = _src.m_numFaces;

        // Fill image structure.
        imageMove(_dst, result, _allocator);
    }

    void imageToRgba32f(Image& _image, AllocatorI* _allocator)
    {
        Image tmp;
        imageToRgba32f(tmp, _image, _allocator);
        imageMove(_image, tmp, _allocator);
    }

    // From Rgba32f.
    //-----

    inline void bgr8FromRgba32f(uint8_t* _bgr8, const float* _rgba32f)
    {
        _bgr8[2] = uint8_t(CMFT_CLAMP(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _bgr8[1] = uint8_t(CMFT_CLAMP(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _bgr8[0] = uint8_t(CMFT_CLAMP(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
    }

    inline void bgra8FromRgba32f(uint8_t* _bgra8, const float* _rgba32f)
    {
        _bgra8[2] = uint8_t(CMFT_CLAMP(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _bgra8[1] = uint8_t(CMFT_CLAMP(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _bgra8[0] = uint8_t(CMFT_CLAMP(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
        _bgra8[3] = uint8_t(CMFT_CLAMP(_rgba32f[3], 0.0f, 1.0f) * 255.0f);
    }

    inline void rgb8FromRgba32f(uint8_t* _rgb8, const float* _rgba32f)
    {
        _rgb8[0] = uint8_t(CMFT_CLAMP(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _rgb8[1] = uint8_t(CMFT_CLAMP(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _rgb8[2] = uint8_t(CMFT_CLAMP(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
    }

    inline void rgba8FromRgba32f(uint8_t* _rgba8, const float* _rgba32f)
    {
        _rgba8[0] = uint8_t(CMFT_CLAMP(_rgba32f[0], 0.0f, 1.0f) * 255.0f);
        _rgba8[1] = uint8_t(CMFT_CLAMP(_rgba32f[1], 0.0f, 1.0f) * 255.0f);
        _rgba8[2] = uint8_t(CMFT_CLAMP(_rgba32f[2], 0.0f, 1.0f) * 255.0f);
        _rgba8[3] = uint8_t(CMFT_CLAMP(_rgba32f[3], 0.0f, 1.0f) * 255.0f);
    }

    inline void rgb16FromRgba32f(uint16_t* _rgb16, const float* _rgba32f)
    {
        _rgb16[0] = uint16_t(CMFT_CLAMP(_rgba32f[0], 0.0f, 1.0f) * 65535.0f);
        _rgb16[1] = uint16_t(CMFT_CLAMP(_rgba32f[1], 0.0f, 1.0f) * 65535.0f);
        _rgb16[2] = uint16_t(CMFT_CLAMP(_rgba32f[2], 0.0f, 1.0f) * 65535.0f);
    }

    inline void rgba16FromRgba32f(uint16_t* _rgba16, const float* _rgba32f)
    {
        _rgba16[0] = uint16_t(CMFT_CLAMP(_rgba32f[0], 0.0f, 1.0f) * 65535.0f);
        _rgba16[1] = uint16_t(CMFT_CLAMP(_rgba32f[1], 0.0f, 1.0f) * 65535.0f);
        _rgba16[2] = uint16_t(CMFT_CLAMP(_rgba32f[2], 0.0f, 1.0f) * 65535.0f);
        _rgba16[3] = uint16_t(CMFT_CLAMP(_rgba32f[3], 0.0f, 1.0f) * 65535.0f);
    }

    inline void rgb16fFromRgba32f(uint16_t* _rgb16f, const float* _rgba32f)
    {
        _rgb16f[0] = cmft::halfFromFloat(_rgba32f[0]);
        _rgb16f[1] = cmft::halfFromFloat(_rgba32f[1]);
        _rgb16f[2] = cmft::halfFromFloat(_rgba32f[2]);
    }

    inline void rgba16fFromRgba32f(uint16_t* _rgba16f, const float* _rgba32f)
    {
        _rgba16f[0] = cmft::halfFromFloat(_rgba32f[0]);
        _rgba16f[1] = cmft::halfFromFloat(_rgba32f[1]);
        _rgba16f[2] = cmft::halfFromFloat(_rgba32f[2]);
        _rgba16f[3] = cmft::halfFromFloat(_rgba32f[3]);
    }

    inline void rgb32fFromRgba32f(float* _rgb32f, const float* _rgba32f)
    {
        _rgb32f[0] = _rgba32f[0];
        _rgb32f[1] = _rgba32f[1];
        _rgb32f[2] = _rgba32f[2];
    }

    inline void rgba32fFromRgba32f(float* _dst, const float* _src)
    {
        memcpy(_dst, _src, 4*sizeof(float));
    }

    inline void rgbeFromRgba32f(uint8_t* _rgbe, const float* _rgba32f)
    {
        const float maxVal = CMFT_MAX(CMFT_MAX(_rgba32f[0], _rgba32f[1]), _rgba32f[2]);
        const float exp = ceilf(cmft::log2f(maxVal));
        const float toRgb8 = 255.0f * 1.0f/ldexp(1.0f, int(exp)); //ldexp -> exp2 (c++11 - <cmath.h>)
        _rgbe[0] = uint8_t(_rgba32f[0] * toRgb8);
        _rgbe[1] = uint8_t(_rgba32f[1] * toRgb8);
        _rgbe[2] = uint8_t(_rgba32f[2] * toRgb8);
        _rgbe[3] = uint8_t(exp+128.0f);
    }

    void fromRgba32f(void* _out, TextureFormat::Enum _format, const float _rgba32f[4])
    {
        switch(_format)
        {
        case TextureFormat::BGR8:     bgr8FromRgba32f((uint8_t*)_out,     _rgba32f); break;
        case TextureFormat::RGB8:     rgb8FromRgba32f((uint8_t*)_out,     _rgba32f); break;
        case TextureFormat::RGB16:    rgb16FromRgba32f((uint16_t*)_out,   _rgba32f); break;
        case TextureFormat::RGB16F:   rgb16fFromRgba32f((uint16_t*)_out,  _rgba32f); break;
        case TextureFormat::RGB32F:   rgb32fFromRgba32f((float*)_out,     _rgba32f); break;
        case TextureFormat::RGBE:     rgbeFromRgba32f((uint8_t*)_out,     _rgba32f); break;
        case TextureFormat::BGRA8:    bgra8FromRgba32f((uint8_t*)_out,    _rgba32f); break;
        case TextureFormat::RGBA8:    rgba8FromRgba32f((uint8_t*)_out,    _rgba32f); break;
        case TextureFormat::RGBA16:   rgba16FromRgba32f((uint16_t*)_out,  _rgba32f); break;
        case TextureFormat::RGBA16F:  rgba16fFromRgba32f((uint16_t*)_out, _rgba32f); break;
        case TextureFormat::RGBA32F:  rgba32fFromRgba32f((float*)_out,    _rgba32f); break;
        default: DEBUG_CHECK(false, "Unknown image format.");
        };
    }

    void imageFromRgba32f(Image& _dst, TextureFormat::Enum _dstFormat, const Image& _src, AllocatorI* _allocator)
    {
        DEBUG_CHECK(TextureFormat::RGBA32F == _src.m_format, "Source image is not in RGBA32F format!");

        // Alloc dst data.
        const uint32_t pixelCount = imageGetNumPixels(_src);
        const uint8_t dstBytesPerPixel = getImageDataInfo(_dstFormat).m_bytesPerPixel;
        const uint32_t dstDataSize = pixelCount*dstBytesPerPixel;
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get total number of channels.
        const uint8_t srcNumChannels = getImageDataInfo(_src.m_format).m_numChanels;
        const uint32_t totalNumChannels = pixelCount*srcNumChannels;

        // Convert data.
        const float* src = (const float*)_src.m_data;
        const float* end = (const float*)_src.m_data + totalNumChannels;
        switch(_dstFormat)
        {
        case TextureFormat::BGR8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    bgr8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb16FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB16F:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb16fFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGB32F:
            {
                float* dst = (float*)dstData;

                for (;src < end; src+=4, dst+=3)
                {
                    rgb32fFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBE:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgbeFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::BGRA8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    bgra8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA8:
            {
                uint8_t* dst = (uint8_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba8FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba16FromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA16F:
            {
                uint16_t* dst = (uint16_t*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba16fFromRgba32f(dst, src);
                }
            }
        break;

        case TextureFormat::RGBA32F:
            {
                float* dst = (float*)dstData;

                for (;src < end; src+=4, dst+=4)
                {
                    rgba32fFromRgba32f(dst, src);
                }
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "Unknown image format.");
            }
        break;
        };

        // Fill image structure.
        Image result;
        result.m_data = dstData;
        result.m_width = _src.m_width;
        result.m_height = _src.m_height;
        result.m_dataSize = dstDataSize;
        result.m_format = _dstFormat;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = _src.m_numFaces;

        // Output.
        imageMove(_dst, result, _allocator);
    }

    void imageFromRgba32f(Image& _image, TextureFormat::Enum _textureFormat, AllocatorI* _allocator)
    {
        Image tmp;
        imageFromRgba32f(tmp, _textureFormat, _image, _allocator);
        imageMove(_image, tmp, _allocator);
    }

    void imageConvert(Image& _dst, TextureFormat::Enum _dstFormat, const Image& _src, AllocatorI* _allocator)
    {
        // Image _src to rgba32f.
        ImageSoftRef imageRgba32f;
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageRef(imageRgba32f, _src);
        }
        else
        {
            imageToRgba32f(imageRgba32f, _src, _allocator);
        }

        // Image rgba32f to _dst.
        if (TextureFormat::RGBA32F == _dstFormat)
        {
            imageUnload(_dst, _allocator);
            if (imageRgba32f.m_isRef)
            {
                imageCopy(_dst, imageRgba32f, _allocator);
            }
            else
            {
                imageMove(_dst, imageRgba32f, _allocator);
            }

        }
        else
        {
            imageUnload(_dst, _allocator);
            imageFromRgba32f(_dst, _dstFormat, imageRgba32f, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);
    }

    void imageConvert(Image& _image, TextureFormat::Enum _format, AllocatorI* _allocator)
    {
        if (_format != _image.m_format)
        {
            Image tmp;
            imageConvert(tmp, _format, _image, _allocator);
            imageMove(_image, tmp, _allocator);
        }
    }

    void imageGetPixel(void* _out, TextureFormat::Enum _format, uint32_t _x, uint32_t _y, uint8_t _face, uint8_t _mip, const Image& _image)
    {
        // Input check.
        DEBUG_CHECK(_x < _image.m_width,       "Invalid input parameters. X coord bigger than image width.");
        DEBUG_CHECK(_y < _image.m_height,      "Invalid input parameters. Y coord bigger than image height.");
        DEBUG_CHECK(_face < _image.m_numFaces, "Invalid input parameters. Requesting pixel from non-existing face.");
        DEBUG_CHECK(_mip < _image.m_numMips,   "Invalid input parameters. Requesting pixel from non-existing mip level.");

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const uint32_t pitch = _image.m_width * bytesPerPixel;

        // Get face and mip offset.
        uint32_t offset = 0;
        for (uint8_t face = 0; face < _face; ++face)
        {
            for (uint8_t mip = 0, end = _mip+1; mip < end; ++mip)
            {
                const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                offset += width * height * bytesPerPixel;
            }
        }

        const void* src = (const void*)((const uint8_t*)_image.m_data + offset + _y*pitch + _x*bytesPerPixel);

        // Output.
        if (_image.m_format == _format)
        {
            // Image is already in requested format, just copy data.
            memcpy(_out, src, bytesPerPixel);
        }
        else if (_image.m_format == TextureFormat::RGBA32F)
        {
            // Image is in rgba32f format. Transform to output format.
            fromRgba32f(_out, _format, (const float*)src);
        }
        else
        {
            // Image is in some other format.
            // Transform to rgba32f and then back to requested output format.
            float buf[4];
            toRgba32f(buf, _image.m_format, src);
            fromRgba32f(_out, _format, buf);
        }
    }

    void imageCubemapGetPixel(void* _out, TextureFormat::Enum _format, float _dir[3], uint8_t _mip, const Image& _image)
    {
        float uu;
        float vv;
        uint8_t face;
        vecToTexelCoord(uu, vv, face, _dir);

        const float sizeMinusOne = float(int32_t(_image.m_width-1));
        const int32_t xx = int32_t(uu*sizeMinusOne);
        const int32_t yy = int32_t(vv*sizeMinusOne);

        imageGetPixel(_out, _format, xx, yy, face, _mip, _image);
    }

    // Notice: this is the most trivial image resampling implementation. Use this only for testing/debugging purposes!
    void imageResize(Image& _dst, uint32_t _width, uint32_t _height, const Image& _src, AllocatorI* _allocator)
    {
        // Operation is done in rgba32f format.
        ImageSoftRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);

        // Alloc dst data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        uint32_t dstDataSize = 0;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        for (uint8_t face = 0; face < imageRgba32f.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
            {
                dstOffsets[face][mip] = dstDataSize;
                const uint32_t dstMipWidth  = CMFT_MAX(UINT32_C(1), _width  >> mip);
                const uint32_t dstMipHeight = CMFT_MAX(UINT32_C(1), _height >> mip);
                dstDataSize += dstMipWidth * dstMipHeight * bytesPerPixel;
            }
        }
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, imageRgba32f);

        // Resample.
        for (uint8_t face = 0; face < imageRgba32f.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
            {
                const uint8_t  srcMip       = CMFT_MIN(mip, uint8_t(_src.m_numMips-1));
                const uint32_t srcMipWidth  = CMFT_MAX(UINT32_C(1), imageRgba32f.m_width  >> srcMip);
                const uint32_t srcMipHeight = CMFT_MAX(UINT32_C(1), imageRgba32f.m_height >> srcMip);
                const uint32_t srcMipPitch  = srcMipWidth * bytesPerPixel;
                const uint8_t* srcMipData   = (const uint8_t*)imageRgba32f.m_data + srcOffsets[face][srcMip];

                const uint32_t dstMipWidth  = CMFT_MAX(UINT32_C(1), _width  >> mip);
                const uint32_t dstMipHeight = CMFT_MAX(UINT32_C(1), _height >> mip);
                const uint32_t dstMipPitch  = dstMipWidth * bytesPerPixel;

                const float    dstToSrcRatioXf = cmft::utof(srcMipWidth) /cmft::utof(dstMipWidth);
                const float    dstToSrcRatioYf = cmft::utof(srcMipHeight)/cmft::utof(dstMipHeight);
                const uint32_t dstToSrcRatioX  = cmft::ftou(dstToSrcRatioXf);
                const uint32_t dstToSrcRatioY  = cmft::ftou(dstToSrcRatioYf);

                uint8_t* dstMipData = (uint8_t*)dstData + dstOffsets[face][mip];

                for (int32_t yDst = 0; yDst < int32_t(dstMipHeight); ++yDst)
                {
                    uint8_t* dstFaceRow = (uint8_t*)dstMipData + yDst*dstMipPitch;

                    for (int32_t xDst = 0; xDst < int32_t(dstMipWidth); ++xDst)
                    {
                        float* dstFaceColumn = (float*)((uint8_t*)dstFaceRow + xDst*bytesPerPixel);

                        float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                        uint32_t weight = 0;

                        uint32_t       ySrc    = cmft::ftou(float(yDst)*dstToSrcRatioYf);
                        uint32_t const ySrcEnd = ySrc + CMFT_MAX(1, dstToSrcRatioY);
                        for (; ySrc < ySrcEnd; ++ySrc)
                        {
                            const uint8_t* srcRowData = (const uint8_t*)srcMipData + ySrc*srcMipPitch;

                            uint32_t       xSrc    = cmft::ftou(float(xDst)*dstToSrcRatioXf);
                            uint32_t const xSrcEnd = xSrc + CMFT_MAX(1, dstToSrcRatioX);
                            for (; xSrc < xSrcEnd; ++xSrc)
                            {
                                const float* srcColumnData = (const float*)((const uint8_t*)srcRowData + xSrc*bytesPerPixel);
                                color[0] += srcColumnData[0];
                                color[1] += srcColumnData[1];
                                color[2] += srcColumnData[2];
                                color[3] += srcColumnData[3];
                                weight++;
                            }
                        }

                        const float invWeight = 1.0f/cmft::utof(CMFT_MAX(weight, UINT32_C(1)));
                        dstFaceColumn[0] = color[0] * invWeight;
                        dstFaceColumn[1] = color[1] * invWeight;
                        dstFaceColumn[2] = color[2] * invWeight;
                        dstFaceColumn[3] = color[3] * invWeight;
                    }
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = _width;
        result.m_height = _height;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = imageRgba32f.m_numMips;
        result.m_numFaces = imageRgba32f.m_numFaces;
        result.m_data = dstData;

        // Convert result to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result, _allocator);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result, _allocator);
            imageUnload(result, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);
    }

    void imageResize(Image& _image, uint32_t _width, uint32_t _height, AllocatorI* _allocator)
    {
        Image tmp;
        imageResize(tmp, _width, _height, _image, _allocator);
        imageMove(_image, tmp, _allocator);
    }

    static inline void faceSizeToWH(uint32_t& _width, uint32_t& _height, uint32_t _faceSize, const cmft::Image& _image)
    {
        if (cmft::imageIsLatLong(_image))
        {
            _width  = _faceSize*4;
            _height = _faceSize*2;
        }
        else if (imageIsCubeCross(_image, true))
        {
            const float aspect = float(int32_t(_image.m_width))/float(int32_t(_image.m_height));
            const bool isVertical = cmft::equals(aspect, 3.0f/4.0f, 0.0001f);
            if (isVertical)
            {
                _width  = _faceSize*3;
                _height = _faceSize*4;
            }
            else // isHorizontal
            {
                _width  = _faceSize*4;
                _height = _faceSize*3;
            }
        }
        else if (imageIsHStrip(_image))
        {
            _width  = _faceSize*6;
            _height = _faceSize;
        }
        else if (imageIsVStrip(_image))
        {
            _width  = _faceSize;
            _height = _faceSize*6;
        }
        else // Cubemap.
        {
            _width  = _faceSize;
            _height = _faceSize;
        }
    }

    void imageResize(Image& _dst, uint32_t _faceSize, const Image& _src, AllocatorI* _allocator)
    {
        uint32_t width, height;
        faceSizeToWH(width, height, _faceSize, _src);
        imageResize(_dst, width, height, _src, _allocator);
    }

    void imageResize(Image& _image, uint32_t _faceSize, AllocatorI* _allocator)
    {
        uint32_t width, height;
        faceSizeToWH(width, height, _faceSize, _image);
        imageResize(_image, width, height, _allocator);
    }

    uint32_t imageGetCubemapFaceSize(const Image& _image)
    {
        if (cmft::imageIsLatLong(_image))
        {
            return _image.m_width>>2;
        }
        else if (imageIsHStrip(_image))
        {
            return _image.m_height;
        }
        else if (imageIsCubeCross(_image, true))
        {
            const float aspect = float(int32_t(_image.m_width))/float(int32_t(_image.m_height));
            const bool isVertical = cmft::equals(aspect, 3.0f/4.0f, 0.0001f);
            if (isVertical)
            {
                return _image.m_height>>2;
            }
            else // isHorizontal.
            {
                return _image.m_width>>2;
            }
        }
        else // Image is vstrip or cubemap.
        {
            return _image.m_width;
        }
    }

    void imageTransformUseMacroInstead(Image* _image, ...)
    {
        va_list argList;
        va_start(argList, _image);
        imageTransformArg(*_image, argList);
        va_end(argList);
    }

    void imageTransformArg(Image& _image, va_list _argList)
    {
        uint32_t op = va_arg(_argList, uint32_t);
        if (UINT32_MAX != op)
        {
            const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;

            uint32_t offsets[CUBE_FACE_NUM][MAX_MIP_NUM];
            imageGetMipOffsets(offsets, _image);

            uint8_t* tmp = (uint8_t*)alloca(bytesPerPixel * _image.m_width);

            for (uint8_t ii = 0; op != UINT32_MAX; ++ii, op = va_arg(_argList, uint32_t))
            {
                const uint16_t imageOp = (op&IMAGE_OP_MASK);
                const uint8_t imageFace = (op&IMAGE_FACE_MASK)>>IMAGE_FACE_SHIFT;

                if (imageOp&IMAGE_OP_ROT_90)
                {
                    if (_image.m_width == _image.m_height)
                    {
                        for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
                        {
                            const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                            const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                            const uint32_t pitch  = width * bytesPerPixel;

                            uint8_t* facePtr = (uint8_t*)_image.m_data + offsets[imageFace][mip];
                            for (uint32_t yy = 0, yyEnd = height-1; yy < height; ++yy, --yyEnd)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + yy*pitch;
                                uint8_t* columnPtr = (uint8_t*)facePtr + yyEnd*bytesPerPixel;
                                for (uint32_t xx = 0, xxEnd = width-1; xx < width; ++xx, --xxEnd)
                                {
                                    if (xx < yyEnd)
                                    {
                                        uint8_t* aa = (uint8_t*)rowPtr    + xx*bytesPerPixel;
                                        uint8_t* bb = (uint8_t*)columnPtr + xxEnd*pitch;
                                        cmft::swap(aa, bb, tmp, bytesPerPixel);
                                    }
                                }
                            }

                            // Flip X.
                            for (uint32_t yy = 0, yyEnd = height-1; yy < yyEnd; ++yy, --yyEnd)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + pitch*yy;
                                uint8_t* rowPtrEnd = (uint8_t*)facePtr + pitch*yyEnd;
                                cmft::swap(rowPtr, rowPtrEnd, tmp, pitch);
                            }
                        }
                    }
                    else
                    {
                        WARN("Because image data transformation is done in place, "
                             "rotation operations work only when image width is equal to image height.");
                    }
                }

                if (imageOp&IMAGE_OP_ROT_180)
                {
                    if (_image.m_width == _image.m_height)
                    {
                        for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
                        {
                            const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                            const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                            const uint32_t pitch  = width * bytesPerPixel;

                            uint8_t* facePtr = (uint8_t*)_image.m_data + offsets[imageFace][mip];
                            uint32_t yy = 0, yyEnd = height-1;
                            for (; yy < yyEnd; ++yy, --yyEnd)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + pitch*yy;
                                uint8_t* rowPtrEnd = (uint8_t*)facePtr + pitch*yyEnd;
                                for (uint32_t xx = 0, xxEnd = width-1; xx < width; ++xx, --xxEnd)
                                {
                                    uint8_t* aa = (uint8_t*)rowPtr    + bytesPerPixel*xx;
                                    uint8_t* bb = (uint8_t*)rowPtrEnd + bytesPerPixel*xxEnd;
                                    cmft::swap(aa, bb, tmp, bytesPerPixel);
                                }
                            }

                            // Handle middle line as special case.
                            if (yy == yyEnd)
                            {
                                uint8_t* rowPtr = (uint8_t*)facePtr + pitch*yy;
                                for (uint32_t xx = 0, xxEnd = width-1; xx < xxEnd; ++xx, --xxEnd)
                                {
                                    uint8_t* aa = (uint8_t*)rowPtr + bytesPerPixel*xx;
                                    uint8_t* bb = (uint8_t*)rowPtr + bytesPerPixel*xxEnd;
                                    cmft::swap(aa, bb, tmp, bytesPerPixel);
                                }
                            }
                        }
                    }
                    else
                    {
                        WARN("Because image data transformation is done in place, "
                             "rotation operations work only when image width is equal to image height."
                             );
                    }
                }

                if (imageOp&IMAGE_OP_ROT_270)
                {
                    if (_image.m_width == _image.m_height)
                    {
                        for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
                        {
                            const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                            const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                            const uint32_t pitch  = width * bytesPerPixel;

                            uint8_t* facePtr = (uint8_t*)_image.m_data + offsets[imageFace][mip];
                            for (uint32_t yy = 0; yy < height; ++yy)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + yy*pitch;
                                uint8_t* columnPtr = (uint8_t*)facePtr + yy*bytesPerPixel;
                                for (uint32_t xx = 0; xx < width; ++xx)
                                {
                                    if (xx > yy)
                                    {
                                        uint8_t* aa = (uint8_t*)rowPtr    + xx*bytesPerPixel;
                                        uint8_t* bb = (uint8_t*)columnPtr + xx*pitch;
                                        cmft::swap(aa, bb, tmp, bytesPerPixel);
                                    }
                                }
                            }

                            // Flip X.
                            for (uint32_t yy = 0, yyEnd = height-1; yy < yyEnd; ++yy, --yyEnd)
                            {
                                uint8_t* rowPtr    = (uint8_t*)facePtr + pitch*yy;
                                uint8_t* rowPtrEnd = (uint8_t*)facePtr + pitch*yyEnd;
                                cmft::swap(rowPtr, rowPtrEnd, tmp, pitch);
                            }
                        }
                    }
                    else
                    {
                        WARN("Because image data transformation is done in place, "
                             "rotation operations work only when image width is equal to image height."
                             );
                    }
                }

                if (imageOp&IMAGE_OP_FLIP_X)
                {
                    for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
                    {
                        const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                        const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                        const uint32_t pitch  = width * bytesPerPixel;

                        uint8_t* facePtr = (uint8_t*)_image.m_data + offsets[imageFace][mip];
                        for (uint32_t yy = 0, yyEnd = height-1; yy < yyEnd; ++yy, --yyEnd)
                        {
                            uint8_t* rowPtr    = (uint8_t*)facePtr + pitch*yy;
                            uint8_t* rowPtrEnd = (uint8_t*)facePtr + pitch*yyEnd;
                            cmft::swap(rowPtr, rowPtrEnd, tmp, pitch);
                        }
                    }
                }

                if (imageOp&IMAGE_OP_FLIP_Y)
                {
                    for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
                    {
                        const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                        const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                        const uint32_t pitch  = width * bytesPerPixel;

                        uint8_t* facePtr = (uint8_t*)_image.m_data + offsets[imageFace][mip];
                        for (uint32_t yy = 0; yy < height; ++yy)
                        {
                            uint8_t* rowPtr = (uint8_t*)facePtr + pitch*yy;
                            for (uint32_t xx = 0, xxEnd = width-1; xx < xxEnd; ++xx, --xxEnd)
                            {
                                uint8_t* columnPtr    = (uint8_t*)rowPtr + bytesPerPixel*xx;
                                uint8_t* columnPtrEnd = (uint8_t*)rowPtr + bytesPerPixel*xxEnd;
                                cmft::swap(columnPtr, columnPtrEnd, tmp, bytesPerPixel);
                            }
                        }
                    }
                }
            }
        }
    }

    void imageGenerateMipMapChain(Image& _image, uint8_t _numMips, AllocatorI* _allocator)
    {
        // Processing is done in rgba32f format.
        ImageHardRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image, _allocator);

        // Calculate dataSize and offsets for the entire mip map chain.
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        uint32_t dstDataSize = 0;
        uint8_t mipCount = 0;
        const uint8_t maxMipNum = CMFT_MIN(_numMips, uint8_t(MAX_MIP_NUM));
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        for (uint8_t face = 0; face < imageRgba32f.m_numFaces; ++face)
        {
            uint32_t width = 0;
            uint32_t height = 0;

            for (mipCount = 0; (mipCount < maxMipNum) && (width != 1) && (height != 1); ++mipCount)
            {
                dstOffsets[face][mipCount] = dstDataSize;
                width  = CMFT_MAX(UINT32_C(1), imageRgba32f.m_width  >> mipCount);
                height = CMFT_MAX(UINT32_C(1), imageRgba32f.m_height >> mipCount);

                dstDataSize += width * height * bytesPerPixel;
            }
        }

        // Alloc data.
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, imageRgba32f);

        // Generate mip chain.
        for (uint8_t face = 0; face < imageRgba32f.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < mipCount; ++mip)
            {
                const uint32_t width  = CMFT_MAX(UINT32_C(1), imageRgba32f.m_width  >> mip);
                const uint32_t height = CMFT_MAX(UINT32_C(1), imageRgba32f.m_height >> mip);
                const uint32_t pitch = width * bytesPerPixel;

                uint8_t* dstMipData       = (uint8_t*)dstData                   + dstOffsets[face][mip];
                const uint8_t* srcMipData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[face][mip];

                // If mip is present, copy data.
                if (mip < imageRgba32f.m_numMips)
                {
                    for (uint32_t yy = 0; yy < height; ++yy)
                    {
                        uint8_t* dst       = (uint8_t*)dstMipData       + yy*pitch;
                        const uint8_t* src = (const uint8_t*)srcMipData + yy*pitch;
                        memcpy(dst, src, pitch);
                    }
                }
                // Else generate it.
                else
                {
                    const uint8_t parentMip = mip - 1;
                    const uint32_t parentWidth = CMFT_MAX(UINT32_C(1), imageRgba32f.m_width >> parentMip);
                    const uint32_t parentPitch = parentWidth * bytesPerPixel;
                    const uint8_t* parentMipData = (const uint8_t*)dstData + dstOffsets[face][parentMip];

                    for (uint32_t yy = 0; yy < height; ++yy)
                    {
                        uint8_t* dstRowData = (uint8_t*)dstMipData + pitch*yy;
                        for (uint32_t xx = 0; xx < width; ++xx)
                        {
                            float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                            float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                            for (uint32_t yParent = yy*2, yEnd = yParent+2; yParent < yEnd; ++yParent)
                            {
                                const uint8_t* parentRowData = (const uint8_t*)parentMipData + parentPitch*yParent;
                                for (uint32_t xParent = xx*2, xEnd = xParent+2; xParent < xEnd; ++xParent)
                                {
                                    const float* parentColumnData = (const float*)((const uint8_t*)parentRowData + xParent*bytesPerPixel);
                                    color[0] += parentColumnData[0];
                                    color[1] += parentColumnData[1];
                                    color[2] += parentColumnData[2];
                                    color[3] += parentColumnData[3];
                                }
                            }

                            dstColumnData[0] = color[0] * 0.25f;
                            dstColumnData[1] = color[1] * 0.25f;
                            dstColumnData[2] = color[2] * 0.25f;
                            dstColumnData[3] = color[3] * 0.25f;
                        }
                    }
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = imageRgba32f.m_width;
        result.m_height = imageRgba32f.m_height;
        result.m_dataSize = dstDataSize;
        result.m_format = imageRgba32f.m_format;
        result.m_numMips = mipCount;
        result.m_numFaces = imageRgba32f.m_numFaces;
        result.m_data = dstData;

        // Convert result to source format.
        if (TextureFormat::RGBA32F == (TextureFormat::Enum)_image.m_format)
        {
            imageMove(_image, result, _allocator);
        }
        else
        {
            imageConvert(_image, (TextureFormat::Enum)_image.m_format, result, _allocator);
            imageUnload(result, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);
    }

    // From: http://chilliant.blogspot.pt/2012/08/srgb-approximations-for-hlsl.html
    float ToSRGBApprox(float v)
    {
        float S1 = sqrtf( v );
        float S2 = sqrtf( S1 );
        float S3 = sqrtf( S2 );
        float sRGB = 0.585122381f * S1 + 0.783140355f * S2 - 0.368262736f * S3;
        return sRGB;
    }

    void imageEncodeRGBM(Image& _image, AllocatorI* _allocator)
    {
        // Operation is done in rgba32f format.
        ImageHardRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image, _allocator);

        // Iterate through image channels and apply gamma function.
        float* channel = (float*)imageRgba32f.m_data;
        const float* end = (const float*)((const uint8_t*)imageRgba32f.m_data + imageRgba32f.m_dataSize);

        float rgbm[4];
        for (; channel < end; channel += 4)
        {
            // convert to gamma space before encoding
            channel[0] = ToSRGBApprox(channel[0]);
            channel[1] = ToSRGBApprox(channel[1]);
            channel[2] = ToSRGBApprox(channel[2]);
            channel[3] = ToSRGBApprox(channel[3]);

            memcpy( rgbm, channel, 4*sizeof(float) );

            rgbm[0] /= 6.0f;
            rgbm[1] /= 6.0f;
            rgbm[2] /= 6.0f;

            float m = fsaturate(fmaxf(fmaxf(rgbm[0], rgbm[1]), fmaxf(rgbm[2], 1e-6f)));
            m = ceil(rgbm[3] * 255.0f) / 255.0f;
            rgbm[0] /= m;
            rgbm[1] /= m;
            rgbm[2] /= m;
            rgbm[3] = m;

            channel[0] = rgbm[0];
            channel[1] = rgbm[1];
            channel[2] = rgbm[2];
            channel[3] = rgbm[3];
        }

        // Convert to BGRA8 format as final. Overrides any format the user asks
        if (imageRgba32f.isCopy())
        {
            imageConvert(_image, TextureFormat::BGRA8, imageRgba32f, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);
    }

    void imageApplyGamma(Image& _image, float _gammaPow, AllocatorI* _allocator)
    {
        // Do nothing if _gammaPow is ~= 1.0f.
        if (cmft::equals(_gammaPow, 1.0, 0.0001f))
        {
            return;
        }

        // Operation is done in rgba32f format.
        ImageHardRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image, _allocator);

        // Iterate through image channels and apply gamma function.
        float* channel = (float*)imageRgba32f.m_data;
        const float* end = (const float*)((const uint8_t*)imageRgba32f.m_data + imageRgba32f.m_dataSize);

        for (;channel < end; channel+=4)
        {
            channel[0] = powf(channel[0], _gammaPow);
            channel[1] = powf(channel[1], _gammaPow);
            channel[2] = powf(channel[2], _gammaPow);
            //channel[3] = leave alpha channel as is.
        }

        // If image was converted, convert back to original format.
        if (imageRgba32f.isCopy())
        {
            imageConvert(_image, (TextureFormat::Enum)_image.m_format, imageRgba32f, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);
    }

    void imageClamp(Image& _dst, const Image& _src, AllocatorI* _allocator)
    {
        // Get a copy in rgba32f format.
        Image imageRgba32f;
        imageConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);

        // Iterate through image channels and clamp to [0.0-1.0] range.
        float* channel = (float*)imageRgba32f.m_data;
        const float* end = (const float*)((const uint8_t*)imageRgba32f.m_data + imageRgba32f.m_dataSize);

        for (;channel < end; channel+=4)
        {
            channel[0] = CMFT_CLAMP(channel[0], 0.0f, 1.0f);
            channel[1] = CMFT_CLAMP(channel[1], 0.0f, 1.0f);
            channel[2] = CMFT_CLAMP(channel[2], 0.0f, 1.0f);
            channel[3] = CMFT_CLAMP(channel[3], 0.0f, 1.0f);
        }

        // Move or convert to original format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, imageRgba32f, _allocator);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, imageRgba32f, _allocator);
            imageUnload(imageRgba32f, _allocator);
        }
    }

    void imageClamp(Image& _image, AllocatorI* _allocator)
    {
        // Operation is done in rgba32f format.
        ImageHardRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image, _allocator);

        // Iterate through image channels and clamp to [0.0-1.0] range.
        float* channel = (float*)imageRgba32f.m_data;
        const float* end = (const float*)((const uint8_t*)imageRgba32f.m_data + imageRgba32f.m_dataSize);

        for (;channel < end; channel+=4)
        {
            channel[0] = CMFT_CLAMP(channel[0], 0.0f, 1.0f);
            channel[1] = CMFT_CLAMP(channel[1], 0.0f, 1.0f);
            channel[2] = CMFT_CLAMP(channel[2], 0.0f, 1.0f);
            channel[3] = CMFT_CLAMP(channel[3], 0.0f, 1.0f);
        }

        // If image was converted, convert back to original format.
        if (imageRgba32f.isCopy())
        {
            imageConvert(_image, (TextureFormat::Enum)_image.m_format, imageRgba32f, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);
    }

    bool imageIsCubemap(const Image& _image)
    {
        return (6 == _image.m_numFaces) && (_image.m_width == _image.m_height);
    }

    bool imageIsLatLong(const Image& _image)
    {
        const float aspect = (float)(int32_t)_image.m_width/(float)(int32_t)_image.m_height;
        return cmft::equals(aspect, 2.0f, 0.00001f);
    }

    bool imageIsHStrip(const Image& _image)
    {
        return (_image.m_width == 6*_image.m_height);
    }

    bool imageIsVStrip(const Image& _image)
    {
        return (6*_image.m_width == _image.m_height);
    }

    bool imageIsOctant(const Image& _image)
    {
        return (_image.m_width == _image.m_height);
    }

    bool imageValidCubemapFaceList(const Image _faceList[6])
    {
        const uint32_t size = _faceList[0].m_width;
        const uint8_t numMips = _faceList[0].m_numMips;
        for (uint8_t face = 0; face < 6; ++face)
        {
            if (_faceList[face].m_width != _faceList[face].m_height
            ||  size    != _faceList[face].m_width
            ||  numMips != _faceList[face].m_numMips)
            {
                return false;
            }
        }
        return true;
    }

    bool imageIsCubeCross(const Image& _image, bool _fastCheck)
    {
        // Check face count.
        if (1 != _image.m_numFaces)
        {
            return false;
        }

        // Check aspect.
        const float aspect = (float)(int32_t)_image.m_width/(float)(int32_t)_image.m_height;
        const bool isVertical   = cmft::equals(aspect, 3.0f/4.0f, 0.0001f);
        const bool isHorizontal = cmft::equals(aspect, 4.0f/3.0f, 0.0001f);

        if (!isVertical && !isHorizontal)
        {
            return false;
        }

        // Aspect ratio is valid.
        if (_fastCheck)
        {
            return true;
        }

        // Define key points.
        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const uint32_t imagePitch = _image.m_width * bytesPerPixel;

        const uint32_t faceSize = cmft::alignf((float)(int32_t)_image.m_width / (isVertical ? 3.0f : 4.0f), bytesPerPixel);
        const uint32_t facePitch = faceSize * bytesPerPixel;
        const uint32_t rowDataSize = imagePitch * faceSize;

        const uint32_t halfFacePitch   = cmft::alignf((float)(int32_t)facePitch   / 2.0f, bytesPerPixel);
        const uint32_t halfRowDataSize = cmft::alignf((float)(int32_t)rowDataSize / 2.0f, bytesPerPixel);

        uint32_t keyPointsOffsets[6];
        if (isVertical)
        {
            //   ___ ___ ___
            //  |   |   |   | -> rowDataSize
            //  |___|___|___|
            //
            //   ___  -> facePitch
            //
            //   . -> keyPoint
            //       ___
            //    . |Y+ | .
            //   ___|___|___
            //  |X- |Z+ |X+ |
            //  |___|___|___|
            //    . |Y- | .
            //      |___|
            //    . |Z- | .
            //      |___|
            //
            const uint32_t leftCenter  = halfRowDataSize + halfFacePitch;
            const uint32_t rightCenter = halfRowDataSize + 5*halfFacePitch;
            const uint32_t firstRow  = 0;
            const uint32_t thirdRow  = 2*rowDataSize;
            const uint32_t fourthRow = 3*rowDataSize;
            keyPointsOffsets[0] =  leftCenter + firstRow;  //+x
            keyPointsOffsets[1] = rightCenter + firstRow;  //-x
            keyPointsOffsets[2] =  leftCenter + thirdRow;  //+y
            keyPointsOffsets[3] = rightCenter + thirdRow;  //-y
            keyPointsOffsets[4] =  leftCenter + fourthRow; //+z
            keyPointsOffsets[5] = rightCenter + fourthRow; //-z
        }
        else
        {
            //       ___
            //    . |+Y | .   .
            //   ___|___|___ ___
            //  |-X |+Z |+X |-Z |
            //  |___|___|___|___|
            //    . |-Y | .   .
            //      |___|
            //
            const uint32_t center0 = halfRowDataSize + halfFacePitch;
            const uint32_t center1 = halfRowDataSize + 5*halfFacePitch;
            const uint32_t center2 = halfRowDataSize + 7*halfFacePitch;
            const uint32_t firstRow = 0;
            const uint32_t thirdRow = 2*rowDataSize;
            keyPointsOffsets[0] = firstRow + center0; //+x
            keyPointsOffsets[1] = firstRow + center1; //-x
            keyPointsOffsets[2] = firstRow + center2; //+y
            keyPointsOffsets[3] = thirdRow + center0; //-y
            keyPointsOffsets[4] = thirdRow + center1; //+z
            keyPointsOffsets[5] = thirdRow + center2; //-z
        }

        // Check key points.
        bool result = true;
        switch(_image.m_format)
        {
        case TextureFormat::BGR8:
        case TextureFormat::RGB8:
        case TextureFormat::BGRA8:
        case TextureFormat::RGBA8:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint8_t* point = (const uint8_t*)_image.m_data + keyPointsOffsets[key];
                    const bool tap0 = point[0] < 2;
                    const bool tap1 = point[1] < 2;
                    const bool tap2 = point[2] < 2;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGB16:
        case TextureFormat::RGBA16:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint16_t* point = (const uint16_t*)((const uint8_t*)_image.m_data + keyPointsOffsets[key]);
                    const bool tap0 = point[0] < 2;
                    const bool tap1 = point[1] < 2;
                    const bool tap2 = point[2] < 2;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint16_t* point = (const uint16_t*)((const uint8_t*)_image.m_data + keyPointsOffsets[key]);
                    const bool tap0 = cmft::halfToFloat(point[0]) < 0.01f;
                    const bool tap1 = cmft::halfToFloat(point[1]) < 0.01f;
                    const bool tap2 = cmft::halfToFloat(point[2]) < 0.01f;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const float* point = (const float*)((const uint8_t*)_image.m_data + keyPointsOffsets[key]);
                    const bool tap0 = point[0] < 0.01f;
                    const bool tap1 = point[1] < 0.01f;
                    const bool tap2 = point[2] < 0.01f;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        case TextureFormat::RGBE:
            {
                for (uint8_t key = 0; (true == result) && (key < 6); ++key)
                {
                    const uint8_t* point = (const uint8_t*)_image.m_data + keyPointsOffsets[key];
                    const float exp = ldexp(1.0f, point[3] - (128+8));
                    const bool tap0 = float(point[0])*exp < 0.01f;
                    const bool tap1 = float(point[1])*exp < 0.01f;
                    const bool tap2 = float(point[2])*exp < 0.01f;
                    result &= (tap0 & tap1 & tap2);
                }
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "Unknown image format.");
                result = false;
            }
        break;
        };

        return result;
    }

    bool imageIsEnvironmentMap(const Image& _image, bool _fastCheck)
    {
        return imageIsCubemap(_image)
            || imageIsLatLong(_image)
            || imageIsHStrip(_image)
            || imageIsVStrip(_image)
            || imageIsCubeCross(_image, _fastCheck);
    }

    bool imageCubemapFromCross(Image& _dst, const Image& _src, AllocatorI* _allocator)
    {
        // Checking image aspect.
        const float aspect = (float)(int32_t)_src.m_width/(float)(int32_t)_src.m_height;
        const bool isVertical   = cmft::equals(aspect, 3.0f/4.0f, 0.0001f);
        const bool isHorizontal = cmft::equals(aspect, 4.0f/3.0f, 0.0001f);

        if (!isVertical && !isHorizontal)
        {
            return false;
        }

        // Get sizes.
        const uint32_t srcBytesPerPixel = getImageDataInfo(_src.m_format).m_bytesPerPixel;
        const uint32_t imagePitch = _src.m_width * srcBytesPerPixel;
        const uint32_t faceSize = isVertical ? (_src.m_width+2)/3 : (_src.m_width+3)/4;
        const uint32_t facePitch = faceSize * srcBytesPerPixel;
        const uint32_t faceDataSize = facePitch * faceSize;
        const uint32_t rowDataSize = imagePitch * faceSize;

        // Alloc data.
        const uint32_t dstDataSize = faceDataSize * CUBE_FACE_NUM;
        void* data = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(data);

        // Setup offsets.
        uint32_t faceOffsets[6];
        if (isVertical)
        {
            //   ___ ___ ___
            //  |   |   |   | -> rowDataSize
            //  |___|___|___|
            //
            //   ___  -> facePitch
            //
            //       ___
            //      |+Y |
            //   ___|___|___
            //  |-X |+Z |+X |
            //  |___|___|___|
            //      |-Y |
            //      |___|
            //      |-Z |
            //      |___|
            //
            faceOffsets[0] = rowDataSize + 2*facePitch; //+x
            faceOffsets[1] = rowDataSize;               //-x
            faceOffsets[2] = facePitch;                 //+y
            faceOffsets[3] = 2*rowDataSize + facePitch; //-y
            faceOffsets[4] = rowDataSize + facePitch;   //+z
            faceOffsets[5] = 3*rowDataSize + facePitch; //-z
        }
        else
        {
            //       ___
            //      |+Y |
            //   ___|___|___ ___
            //  |-X |+Z |+X |-Z |
            //  |___|___|___|___|
            //      |-Y |
            //      |___|
            //
            faceOffsets[0] = rowDataSize + 2*facePitch; //+x
            faceOffsets[1] = rowDataSize;               //-x
            faceOffsets[2] = facePitch;                 //+y
            faceOffsets[3] = 2*rowDataSize + facePitch; //-y
            faceOffsets[4] = rowDataSize + facePitch;   //+z
            faceOffsets[5] = rowDataSize + 3*facePitch; //-z
        }

        // Copy data.
        for (uint8_t face = 0; face < 6; ++face)
        {
            const uint8_t* srcFaceData = (const uint8_t*)_src.m_data + faceOffsets[face];
            uint8_t* dstFaceData = (uint8_t*)data + faceDataSize*face;
            for (uint32_t yy = 0; yy < faceSize; ++yy)
            {
                memcpy(&dstFaceData[facePitch*yy], &srcFaceData[imagePitch*yy], facePitch);
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = faceSize;
        result.m_height = faceSize;
        result.m_dataSize = dstDataSize;
        result.m_format = _src.m_format;
        result.m_numMips = 1;
        result.m_numFaces = 6;
        result.m_data = data;

        // Transform -Z face properly.
        if (isVertical)
        {
            imageTransform(result, IMAGE_FACE_NEGATIVEZ | IMAGE_OP_FLIP_X | IMAGE_OP_FLIP_Y);
        }

        // Output.
        imageMove(_dst, result, _allocator);

        return true;
    }

    bool imageCubemapFromCross(Image& _image, AllocatorI* _allocator)
    {
        Image tmp;
        if (imageCubemapFromCross(tmp, _image, _allocator))
        {
            imageMove(_image, tmp, _allocator);
            return true;
        }

        return false;
    }

    bool imageCubemapFromLatLong(Image& _dst, const Image& _src, bool _useBilinearInterpolation, AllocatorI* _allocator)
    {
        if (!imageIsLatLong(_src))
        {
            return false;
        }

        // Conversion is done in rgba32f format.
        ImageSoftRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);

        // Alloc data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstFaceSize = (imageRgba32f.m_height+1)/2;
        const uint32_t dstPitch = dstFaceSize * bytesPerPixel;
        const uint32_t dstFaceDataSize = dstPitch * dstFaceSize;
        const uint32_t dstDataSize = dstFaceDataSize * CUBE_FACE_NUM;
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source parameters.
        const float srcWidthMinusOne  = float(int32_t(imageRgba32f.m_width-1));
        const float srcHeightMinusOne = float(int32_t(imageRgba32f.m_height-1));
        const uint32_t srcPitch = imageRgba32f.m_width * bytesPerPixel;
        const float invDstFaceSizef = 1.0f/float(dstFaceSize);

        // Iterate over destination image (cubemap).
        for (uint8_t face = 0; face < 6; ++face)
        {
            uint8_t* dstFaceData = (uint8_t*)dstData + face*dstFaceDataSize;
            for (uint32_t yy = 0; yy < dstFaceSize; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*dstPitch;
                for (uint32_t xx = 0; xx < dstFaceSize; ++xx)
                {
                    float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                    // Cubemap (u,v) on current face.
                    const float uu = 2.0f*xx*invDstFaceSizef-1.0f;
                    const float vv = 2.0f*yy*invDstFaceSizef-1.0f;

                    // Get cubemap vector (x,y,z) from (u,v,faceIdx).
                    float vec[3];
                    texelCoordToVec(vec, uu, vv, face);

                    // Convert cubemap vector (x,y,z) to latlong (u,v).
                    float xSrcf;
                    float ySrcf;
                    latLongFromVec(xSrcf, ySrcf, vec);

                    // Convert from [0..1] to [0..(size-1)] range.
                    xSrcf *= srcWidthMinusOne;
                    ySrcf *= srcHeightMinusOne;

                    // Sample from latlong (u,v).
                    if (_useBilinearInterpolation)
                    {
                        const uint32_t x0 = cmft::ftou(xSrcf);
                        const uint32_t y0 = cmft::ftou(ySrcf);
                        const uint32_t x1 = CMFT_MIN(x0+1, imageRgba32f.m_width-1);
                        const uint32_t y1 = CMFT_MIN(y0+1, imageRgba32f.m_height-1);

                        const float *src0 = (const float*)((const uint8_t*)imageRgba32f.m_data + y0*srcPitch + x0*bytesPerPixel);
                        const float *src1 = (const float*)((const uint8_t*)imageRgba32f.m_data + y0*srcPitch + x1*bytesPerPixel);
                        const float *src2 = (const float*)((const uint8_t*)imageRgba32f.m_data + y1*srcPitch + x0*bytesPerPixel);
                        const float *src3 = (const float*)((const uint8_t*)imageRgba32f.m_data + y1*srcPitch + x1*bytesPerPixel);

                        const float tx = xSrcf - float(int32_t(x0));
                        const float ty = ySrcf - float(int32_t(y0));
                        const float invTx = 1.0f - tx;
                        const float invTy = 1.0f - ty;

                        float p0[4];
                        float p1[4];
                        float p2[4];
                        float p3[4];
                        vec4Mul(p0, src0, invTx*invTy);
                        vec4Mul(p1, src1,    tx*invTy);
                        vec4Mul(p2, src2, invTx*   ty);
                        vec4Mul(p3, src3,    tx*   ty);

                        const float rr = p0[0] + p1[0] + p2[0] + p3[0];
                        const float gg = p0[1] + p1[1] + p2[1] + p3[1];
                        const float bb = p0[2] + p1[2] + p2[2] + p3[2];
                        const float aa = p0[3] + p1[3] + p2[3] + p3[3];

                        dstColumnData[0] = rr;
                        dstColumnData[1] = gg;
                        dstColumnData[2] = bb;
                        dstColumnData[3] = aa;
                    }
                    else
                    {
                        const uint32_t xSrc = cmft::ftou(xSrcf);
                        const uint32_t ySrc = cmft::ftou(ySrcf);
                        const float *src = (const float*)((const uint8_t*)imageRgba32f.m_data + ySrc*srcPitch + xSrc*bytesPerPixel);

                        dstColumnData[0] = src[0];
                        dstColumnData[1] = src[1];
                        dstColumnData[2] = src[2];
                        dstColumnData[3] = src[3];
                    }

                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstFaceSize;
        result.m_height = dstFaceSize;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = 1;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Convert result to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result, _allocator);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result, _allocator);
            imageUnload(result, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);

        return true;
    }

    bool imageCubemapFromLatLong(Image& _image, bool _useBilinearInterpolation, AllocatorI* _allocator)
    {
        Image tmp;
        if (imageCubemapFromLatLong(tmp, _image, _useBilinearInterpolation, _allocator))
        {
            imageMove(_image, tmp, _allocator);
            return true;
        }

        return false;
    }

    bool imageLatLongFromCubemap(Image& _dst, const Image& _src, bool _useBilinearInterpolation, AllocatorI* _allocator)
    {
        // Input check.
        if (!imageIsCubemap(_src))
        {
            return false;
        }

        // Conversion is done in rgba32f format.
        ImageSoftRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);

        // Alloc data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstHeight = imageRgba32f.m_height*2;
        const uint32_t dstWidth = imageRgba32f.m_height*4;
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t dstMipWidth  = CMFT_MAX(UINT32_C(1), dstWidth  >> mip);
            const uint32_t dstMipHeight = CMFT_MAX(UINT32_C(1), dstHeight >> mip);
            dstDataSize += dstMipWidth * dstMipHeight * bytesPerPixel;
        }
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source image parameters.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, imageRgba32f);

        // Iterate over destination image (latlong).
        for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
        {
            const uint32_t dstMipWidth  = CMFT_MAX(UINT32_C(1), dstWidth  >> mip);
            const uint32_t dstMipHeight = CMFT_MAX(UINT32_C(1), dstHeight >> mip);
            const uint32_t dstMipPitch = dstMipWidth * bytesPerPixel;
            const float invDstWidthf  = 1.0f/float(dstMipWidth-1);
            const float invDstHeightf = 1.0f/float(dstMipHeight-1);

            const uint32_t srcMipSize = CMFT_MAX(UINT32_C(1), imageRgba32f.m_width >> mip);
            const uint32_t srcPitch = srcMipSize * bytesPerPixel;

            const uint32_t srcMipSizeMinOne  = srcMipSize-1;
            const float    srcMipSizeMinOnef = cmft::utof(srcMipSizeMinOne);

            uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];
            for (uint32_t yy = 0; yy < dstMipHeight; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstMipData + yy*dstMipPitch;
                for (uint32_t xx = 0; xx < dstMipWidth; ++xx)
                {
                    float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                    // Latlong (x,y).
                    const float xDst = cmft::utof(xx)*invDstWidthf;
                    const float yDst = cmft::utof(yy)*invDstHeightf;

                    // Get cubemap vector (x,y,z) coresponding to latlong (x,y).
                    float vec[3];
                    vecFromLatLong(vec, xDst, yDst);

                    // Get cubemap (u,v,faceIdx) from cubemap vector (x,y,z).
                    float xSrcf;
                    float ySrcf;
                    uint8_t faceIdx;
                    vecToTexelCoord(xSrcf, ySrcf, faceIdx, vec);

                    // Convert from [0..1] to [0..(size-1)] range.
                    xSrcf *= srcMipSizeMinOnef;
                    ySrcf *= srcMipSizeMinOnef;

                    // Sample from cubemap (u,v, faceIdx).
                    if (_useBilinearInterpolation)
                    {
                        const uint32_t x0 = cmft::ftou(xSrcf);
                        const uint32_t y0 = cmft::ftou(ySrcf);
                        const uint32_t x1 = CMFT_MIN(x0+1, srcMipSizeMinOne);
                        const uint32_t y1 = CMFT_MIN(y0+1, srcMipSizeMinOne);

                        const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[faceIdx][mip];
                        const float *src0 = (const float*)((const uint8_t*)srcFaceData + y0*srcPitch + x0*bytesPerPixel);
                        const float *src1 = (const float*)((const uint8_t*)srcFaceData + y0*srcPitch + x1*bytesPerPixel);
                        const float *src2 = (const float*)((const uint8_t*)srcFaceData + y1*srcPitch + x0*bytesPerPixel);
                        const float *src3 = (const float*)((const uint8_t*)srcFaceData + y1*srcPitch + x1*bytesPerPixel);

                        const float tx = xSrcf - float(int32_t(x0));
                        const float ty = ySrcf - float(int32_t(y0));
                        const float invTx = 1.0f - tx;
                        const float invTy = 1.0f - ty;

                        float p0[4];
                        float p1[4];
                        float p2[4];
                        float p3[4];
                        vec4Mul(p0, src0, invTx*invTy);
                        vec4Mul(p1, src1,    tx*invTy);
                        vec4Mul(p2, src2, invTx*   ty);
                        vec4Mul(p3, src3,    tx*   ty);

                        const float rr = p0[0] + p1[0] + p2[0] + p3[0];
                        const float gg = p0[1] + p1[1] + p2[1] + p3[1];
                        const float bb = p0[2] + p1[2] + p2[2] + p3[2];
                        const float aa = p0[3] + p1[3] + p2[3] + p3[3];

                        dstColumnData[0] = rr;
                        dstColumnData[1] = gg;
                        dstColumnData[2] = bb;
                        dstColumnData[3] = aa;
                    }
                    else
                    {
                        const uint32_t xSrc = cmft::ftou(xSrcf);
                        const uint32_t ySrc = cmft::ftou(ySrcf);

                        const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[faceIdx][mip];
                        const float *src = (const float*)((const uint8_t*)srcFaceData + ySrc*srcPitch + xSrc*bytesPerPixel);

                        dstColumnData[0] = src[0];
                        dstColumnData[1] = src[1];
                        dstColumnData[2] = src[2];
                        dstColumnData[3] = src[3];
                    }
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstWidth;
        result.m_height = dstHeight;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = imageRgba32f.m_numMips;
        result.m_numFaces = 1;
        result.m_data = dstData;

        // Convert back to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result, _allocator);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result, _allocator);
            imageUnload(result, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);

        return true;
    }

    bool imageLatLongFromCubemap(Image& _cubemap, bool _useBilinearInterpolation, AllocatorI* _allocator)
    {
        Image tmp;
        if (imageLatLongFromCubemap(tmp, _cubemap, _useBilinearInterpolation, _allocator))
        {
            imageMove(_cubemap, tmp, _allocator);
            return true;
        }

        return false;
    }

    bool imageStripFromCubemap(Image& _dst, const Image& _src, bool _vertical, AllocatorI* _allocator)
    {
        // Input check.
        if(!imageIsCubemap(_src))
        {
            return false;
        }

        // Calculate destination offsets and alloc data.
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        const uint32_t dstWidth  = _vertical ? _src.m_width   : _src.m_width*6;
        const uint32_t dstHeight = _vertical ? _src.m_width*6 : _src.m_width  ;
        const uint32_t bytesPerPixel = getImageDataInfo(_src.m_format).m_bytesPerPixel;
        for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t mipWidth  = CMFT_MAX(UINT32_C(1), dstWidth  >> mip);
            const uint32_t mipHeight = CMFT_MAX(UINT32_C(1), dstHeight >> mip);

            dstDataSize += mipWidth * mipHeight * bytesPerPixel;
        }
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source image offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, _src);

        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
            {
                // Get src data ptr for current mip and face.
                const uint8_t* srcFaceData = (const uint8_t*)_src.m_data + srcOffsets[face][mip];

                // Get dst ptr for current mip level.
                uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];

                const uint32_t mipFaceSize = CMFT_MAX(UINT32_C(1), _src.m_width >> mip);
                const uint32_t mipFacePitch = mipFaceSize * bytesPerPixel;
                //
                //   Horizontal strip.
                //
                //   .__................   ___  -> FacePitch
                //   .  .  .  .  .  .  .
                //   ...................
                //
                //
                //
                //   Vertical strip.
                //       ___
                //      |   |
                //      |___|    ___
                //      .   .   |   | -> FaceDataSize
                //      .....   |___|
                //      .   .
                //      .....
                //      .   .
                //      .....
                //      .   .
                //      .....
                //      .   .
                //      .....
                //
                // To get to the desired face in the strip, advance by:
                // - (mipFacePitch * faceIdx)    for hstrip
                // - (mipFaceDataSize * faceIdx) for vstrip
                // Note: mipFaceDataSize == mipFacePitch * mipFaceSize.
                const uint32_t advance = _vertical ? mipFacePitch * mipFaceSize : mipFacePitch;
                uint8_t* dstFaceData = (uint8_t*)dstMipData + advance*face;

                const uint32_t dstMipSize = CMFT_MAX(UINT32_C(1), dstWidth >> mip);
                const uint32_t dstMipPitch = dstMipSize*bytesPerPixel;

                for (uint32_t yy = 0; yy < mipFaceSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*mipFacePitch;
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*dstMipPitch;

                    memcpy(dstRowData, srcRowData, mipFacePitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstWidth;
        result.m_height = dstHeight;
        result.m_dataSize = dstDataSize;
        result.m_format = _src.m_format;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = 1;
        result.m_data = dstData;

        // Output.
        imageMove(_dst, result, _allocator);

        return true;
    }

    bool imageStripFromCubemap(Image& _cubemap, bool _vertical, AllocatorI* _allocator)
    {
        Image tmp;
        if (imageStripFromCubemap(tmp, _cubemap, _vertical, _allocator))
        {
            imageMove(_cubemap, tmp, _allocator);
            return true;
        }

        return false;
    }

    bool imageCubemapFromStrip(Image& _dst, const Image& _src, AllocatorI* _allocator)
    {
        // Input check.
        const bool isVertical   = imageIsVStrip(_src);
        const bool isHorizontal = imageIsHStrip(_src);
        if(!isVertical && !isHorizontal)
        {
            return false;
        }

        // Calculate destination offsets and alloc data.
        uint32_t dstDataSize = 0;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        const uint32_t dstSize = isHorizontal ? _src.m_height : _src.m_width;
        const uint32_t bytesPerPixel = getImageDataInfo(_src.m_format).m_bytesPerPixel;
        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
            {
                dstOffsets[face][mip] = dstDataSize;
                const uint32_t mipSize = CMFT_MAX(UINT32_C(1), dstSize >> mip);

                dstDataSize += mipSize * mipSize * bytesPerPixel;
            }
        }
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, _src);

        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < _src.m_numMips; ++mip)
            {
                // Get dst data ptr for current mip and face.
                uint8_t* dstFaceData = (uint8_t*)dstData + dstOffsets[face][mip];

                // Get src ptr for current mip level.
                const uint8_t* srcMipData = (const uint8_t*)_src.m_data + srcOffsets[0][mip];

                // Advance by (dstPitch * faceIdx) to get to the desired face in the strip.
                const uint32_t dstMipSize = CMFT_MAX(UINT32_C(1), dstSize >> mip);
                const uint32_t dstMipPitch = dstMipSize * bytesPerPixel;
                const uint8_t* srcFaceData = (const uint8_t*)srcMipData + dstMipPitch*face;

                const uint32_t srcMipPitch = dstMipPitch*6;

                for (uint32_t yy = 0; yy < dstMipSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*srcMipPitch;
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*dstMipPitch;

                    memcpy(dstRowData, srcRowData, dstMipPitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstSize;
        result.m_height = dstSize;
        result.m_dataSize = dstDataSize;
        result.m_format = _src.m_format;
        result.m_numMips = _src.m_numMips;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Output.
        imageMove(_dst, result, _allocator);

        return true;
    }

    bool imageCubemapFromStrip(Image& _image, AllocatorI* _allocator)
    {
        Image tmp;
        if (imageCubemapFromStrip(tmp, _image, _allocator))
        {
            imageMove(_image, tmp, _allocator);
            return true;
        }

        return false;
    }

    bool imageFaceListFromCubemap(Image _faceList[6], const Image& _cubemap, AllocatorI* _allocator)
    {
        // Input check.
        if(!imageIsCubemap(_cubemap))
        {
            return false;
        }

        // Get destination sizes and offsets.
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        const uint8_t bytesPerPixel = getImageDataInfo(_cubemap.m_format).m_bytesPerPixel;
        for (uint8_t mip = 0; mip < _cubemap.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t mipSize = CMFT_MAX(UINT32_C(1), _cubemap.m_width >> mip);
            dstDataSize += mipSize * mipSize * bytesPerPixel;
        }

        // Get source offsets.
        uint32_t cubemapOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(cubemapOffsets, _cubemap);

        for (uint8_t face = 0; face < 6; ++face)
        {
            void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
            MALLOC_CHECK(dstData);

            for (uint8_t mip = 0; mip < _cubemap.m_numMips; ++mip)
            {
                const uint8_t* srcFaceData = (const uint8_t*)_cubemap.m_data + cubemapOffsets[face][mip];
                uint8_t* dstFaceData = (uint8_t*)dstData + dstMipOffsets[mip];

                const uint32_t mipFaceSize = CMFT_MAX(UINT32_C(1), _cubemap.m_width >> mip);
                const uint32_t mipPitch = mipFaceSize * bytesPerPixel;

                for (uint32_t yy = 0; yy < mipFaceSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*mipPitch;
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*mipPitch;

                    memcpy(dstRowData, srcRowData, mipPitch);
                }
            }

            // Fill image structure.
            Image result;
            result.m_width = _cubemap.m_width;
            result.m_height = _cubemap.m_height;
            result.m_dataSize = dstDataSize;
            result.m_format = _cubemap.m_format;
            result.m_numMips = _cubemap.m_numMips;
            result.m_numFaces = 1;
            result.m_data = dstData;

            // Output.
            imageMove(_faceList[face], result, _allocator);
        }

        return true;
    }

    bool imageCubemapFromFaceList(Image& _cubemap, const Image _faceList[6], AllocatorI* _allocator)
    {
        // Input check.
        if (!imageValidCubemapFaceList(_faceList))
        {
            return false;
        }

        // Get source offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, _faceList[0]);
        const uint32_t bytesPerPixel = getImageDataInfo(_faceList[0].m_format).m_bytesPerPixel;

        // Alloc destination data.
        const uint32_t dstDataSize = _faceList[0].m_dataSize * 6;
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Copy data.
        uint32_t destinationOffset = 0;
        for (uint8_t face = 0; face < 6; ++face)
        {
            const uint8_t* srcFaceData = (const uint8_t*)_faceList[face].m_data;
            for (uint8_t mip = 0; mip < _faceList[0].m_numMips; ++mip)
            {
                const uint8_t* srcMipData = (const uint8_t*)srcFaceData + srcOffsets[0][mip];
                uint8_t* dstMipData = (uint8_t*)dstData + destinationOffset;

                const uint32_t mipFaceSize = CMFT_MAX(UINT32_C(1), _faceList[0].m_width >> mip);
                const uint32_t mipPitch = mipFaceSize * bytesPerPixel;
                const uint32_t mipFaceDataSize = mipPitch * mipFaceSize;

                destinationOffset += mipFaceDataSize;

                for (uint32_t yy = 0; yy < mipFaceSize; ++yy)
                {
                    const uint8_t* srcRowData = (const uint8_t*)srcMipData + yy*mipPitch;
                    uint8_t* dstRowData = (uint8_t*)dstMipData + yy*mipPitch;

                    memcpy(dstRowData, srcRowData, mipPitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = _faceList[0].m_width;
        result.m_height = _faceList[0].m_height;
        result.m_dataSize = dstDataSize;
        result.m_format = _faceList[0].m_format;
        result.m_numMips = _faceList[0].m_numMips;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Output.
        imageMove(_cubemap, result, _allocator);

        return true;
    }

    bool imageCrossFromCubemap(Image& _dst, const Image& _src, bool _vertical, AllocatorI* _allocator)
    {
        // Input check.
        if(!imageIsCubemap(_src))
        {
            return false;
        }

        // Copy source image.
        Image srcCpy;
        imageCopy(srcCpy, _src, _allocator);

        // Transform -z image face properly.
        if (_vertical)
        {
            imageTransform(srcCpy, IMAGE_FACE_NEGATIVEZ | IMAGE_OP_FLIP_X | IMAGE_OP_FLIP_Y);
        }

        // Calculate destination offsets and alloc data.
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        const uint32_t dstWidth  = (_vertical?3:4) * srcCpy.m_width;
        const uint32_t dstHeight = (_vertical?4:3) * srcCpy.m_width;
        const uint32_t bytesPerPixel = getImageDataInfo(srcCpy.m_format).m_bytesPerPixel;
        for (uint8_t mip = 0; mip < srcCpy.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t mipWidth  = CMFT_MAX(UINT32_C(1), dstWidth  >> mip);
            const uint32_t mipHeight = CMFT_MAX(UINT32_C(1), dstHeight >> mip);

            dstDataSize += mipWidth * mipHeight * bytesPerPixel;
        }
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get black pixel.
        void* blackPixel = alloca(bytesPerPixel);
        const float blackPixelRgba32f[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        fromRgba32f(blackPixel, TextureFormat::Enum(srcCpy.m_format), blackPixelRgba32f);

        // Fill with black.
        for (uint8_t mip = 0; mip < srcCpy.m_numMips; ++mip)
        {
            const uint32_t mipWidth  = CMFT_MAX(UINT32_C(1), dstWidth  >> mip);
            const uint32_t mipHeight = CMFT_MAX(UINT32_C(1), dstHeight >> mip);
            const uint32_t mipPitch = mipWidth*bytesPerPixel;

            uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];
            for (uint32_t yy = 0; yy < mipHeight; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstMipData + yy*mipPitch;
                for (uint32_t xx = 0; xx < mipWidth; ++xx)
                {
                    uint8_t* dstColumnData = (uint8_t*)dstRowData + xx*bytesPerPixel;
                    memcpy(dstColumnData, blackPixel, bytesPerPixel);
                }
            }
        }

        // Get source offsets.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, srcCpy);

        for (uint8_t mip = 0; mip < srcCpy.m_numMips; ++mip)
        {
            const uint32_t srcWidth = CMFT_MAX(UINT32_C(1), srcCpy.m_width >> mip);
            const uint32_t srcPitch = srcWidth * bytesPerPixel;

            const uint32_t mipWidth = CMFT_MAX(UINT32_C(1), dstWidth >> mip);
            const uint32_t mipPitch = mipWidth * bytesPerPixel;

            const uint32_t denominator = (_vertical?3:4);
            const uint32_t faceSize = mipWidth / denominator;

            const uint32_t oneFacePitch   =   (mipPitch) / denominator;
            const uint32_t twoFacePitch   = (2*mipPitch) / denominator;
            const uint32_t threeFacePitch = (3*mipPitch) / denominator;

            const uint32_t oneRowDataSize   =   (mipPitch*mipWidth) / denominator;
            const uint32_t twoRowDataSize   = (2*mipPitch*mipWidth) / denominator;
            const uint32_t threeRowDataSize = (3*mipPitch*mipWidth) / denominator;

            // Destination offsets.
            uint32_t faceOffsets[6];
            if (_vertical)
            {
                //   ___ ___ ___
                //  |   |   |   | -> rowDataSize
                //  |___|___|___|
                //
                //   ___  -> facePitch
                //
                //       ___
                //      |Y+ |
                //   ___|___|___
                //  |X- |Z+ |X+ |
                //  |___|___|___|
                //      |Y- |
                //      |___|
                //      |Z- |
                //      |___|
                //
                faceOffsets[0] = oneRowDataSize   + twoFacePitch; //+x
                faceOffsets[1] = oneRowDataSize;                  //-x
                faceOffsets[2] = oneFacePitch;                    //+y
                faceOffsets[3] = twoRowDataSize   + oneFacePitch; //-y
                faceOffsets[4] = oneRowDataSize   + oneFacePitch; //+z
                faceOffsets[5] = threeRowDataSize + oneFacePitch; //-z
            }
            else
            {
                //       ___
                //      |+Y |
                //   ___|___|___ ___
                //  |-X |+Z |+X |-Z |
                //  |___|___|___|___|
                //      |-Y |
                //      |___|
                //
                faceOffsets[0] = oneRowDataSize + twoFacePitch;   //+x
                faceOffsets[1] = oneRowDataSize;                  //-x
                faceOffsets[2] = oneFacePitch;                    //+y
                faceOffsets[3] = twoRowDataSize + oneFacePitch;   //-y
                faceOffsets[4] = oneRowDataSize + oneFacePitch;   //+z
                faceOffsets[5] = oneRowDataSize + threeFacePitch; //-z
            }

            uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];
            for (uint8_t face = 0; face < 6; ++face)
            {
                uint8_t* dstFaceData = (uint8_t*)dstMipData + faceOffsets[face];
                const uint8_t* srcFaceData = (uint8_t*)srcCpy.m_data + srcOffsets[face][mip];
                for (uint32_t yy = 0; yy < faceSize; ++yy)
                {
                    uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*mipPitch;
                    const uint8_t* srcRowData = (const uint8_t*)srcFaceData + yy*srcPitch;

                    memcpy(dstRowData, srcRowData, oneFacePitch);
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstWidth;
        result.m_height = dstHeight;
        result.m_dataSize = dstDataSize;
        result.m_format = srcCpy.m_format;
        result.m_numMips = srcCpy.m_numMips;
        result.m_numFaces = 1;
        result.m_data = dstData;

        // Output.
        imageMove(_dst, result, _allocator);

        // Cleanup.
        imageUnload(srcCpy, _allocator);

        return true;
    }

    bool imageCrossFromCubemap(Image& _image, bool _vertical, AllocatorI* _allocator)
    {
        Image tmp;
        if (imageCrossFromCubemap(tmp, _image, _vertical, _allocator))
        {
            imageMove(_image, tmp, _allocator);
            return true;
        }

        return false;
    }

    bool imageToCubemap(Image& _dst, const Image& _src, AllocatorI* _allocator)
    {
        if (imageIsCubemap(_src))
        {
            imageCopy(_dst, _src, _allocator);
            return true;
        }
        else if (imageCubemapFromCross(_dst, _src, _allocator)
             ||  imageCubemapFromLatLong(_dst, _src, true, _allocator)
             ||  imageCubemapFromStrip(_dst, _src, _allocator))
        {
            return true;
        }

        return false;
    }

    bool imageToCubemap(Image& _image, AllocatorI* _allocator)
    {
        if (!imageIsCubemap(_image))
        {
            if (imageIsCubeCross(_image, true))
            {
                imageCubemapFromCross(_image, _allocator);
            }
            else if (imageIsLatLong(_image))
            {
                imageCubemapFromLatLong(_image, true, _allocator);
            }
            else if (imageIsHStrip(_image) || imageIsVStrip(_image))
            {
                imageCubemapFromStrip(_image, _allocator);
            }
        }

        return imageIsValid(_image) && imageIsCubemap(_image);
    }

    bool imageOctantFromCubemap(Image& _dst, const Image& _src, bool _useBilinearInterpolation, AllocatorI* _allocator)
    {
        // Input check.
        if(!imageIsCubemap(_src))
        {
            return false;
        }

        // Conversion is done in rgba32f format.
        ImageSoftRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);

        // Alloc data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstSize = imageRgba32f.m_height*2;
        uint32_t dstDataSize = 0;
        uint32_t dstMipOffsets[MAX_MIP_NUM];
        for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
        {
            dstMipOffsets[mip] = dstDataSize;
            const uint32_t dstMipSize  = CMFT_MAX(UINT32_C(1), dstSize  >> mip);
            dstDataSize += dstMipSize * dstMipSize * bytesPerPixel;
        }
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source image parameters.
        uint32_t srcOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(srcOffsets, imageRgba32f);

        // Iterate over destination image (latlong).
        for (uint8_t mip = 0; mip < imageRgba32f.m_numMips; ++mip)
        {
            const uint32_t dstMipSize  = CMFT_MAX(UINT32_C(1), dstSize  >> mip);
            const uint32_t dstMipPitch = dstMipSize * bytesPerPixel;
            const float invDstSizef  = 1.0f/float(dstMipSize-1);

            const uint32_t srcMipSize = CMFT_MAX(UINT32_C(1), imageRgba32f.m_width >> mip);
            const uint32_t srcPitch = srcMipSize * bytesPerPixel;

            const uint32_t srcMipSizeMinOne  = srcMipSize-1;
            const float    srcMipSizeMinOnef = cmft::utof(srcMipSizeMinOne);

            uint8_t* dstMipData = (uint8_t*)dstData + dstMipOffsets[mip];
            for (uint32_t yy = 0; yy < dstMipSize; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstMipData + yy*dstMipPitch;
                for (uint32_t xx = 0; xx < dstMipSize; ++xx)
                {
                    float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                    // Latlong (x,y).
                    const float xDst = cmft::utof(xx)*invDstSizef;
                    const float yDst = cmft::utof(yy)*invDstSizef;

                    // Get cubemap vector (x,y,z) coresponding to latlong (x,y).
                    float vec[3];
                    vecFromOctant(vec, xDst, yDst);

                    // Get cubemap (u,v,faceIdx) from cubemap vector (x,y,z).
                    float xSrcf;
                    float ySrcf;
                    uint8_t faceIdx;
                    vecToTexelCoord(xSrcf, ySrcf, faceIdx, vec);

                    // Convert from [0..1] to [0..(size-1)] range.
                    xSrcf *= srcMipSizeMinOnef;
                    ySrcf *= srcMipSizeMinOnef;

                    // Sample from cubemap (u,v, faceIdx).
                    if (_useBilinearInterpolation)
                    {
                        const uint32_t x0 = cmft::ftou(xSrcf);
                        const uint32_t y0 = cmft::ftou(ySrcf);
                        const uint32_t x1 = CMFT_MIN(x0+1, srcMipSizeMinOne);
                        const uint32_t y1 = CMFT_MIN(y0+1, srcMipSizeMinOne);

                        const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[faceIdx][mip];
                        const float *src0 = (const float*)((const uint8_t*)srcFaceData + y0*srcPitch + x0*bytesPerPixel);
                        const float *src1 = (const float*)((const uint8_t*)srcFaceData + y0*srcPitch + x1*bytesPerPixel);
                        const float *src2 = (const float*)((const uint8_t*)srcFaceData + y1*srcPitch + x0*bytesPerPixel);
                        const float *src3 = (const float*)((const uint8_t*)srcFaceData + y1*srcPitch + x1*bytesPerPixel);

                        const float tx = xSrcf - float(int32_t(x0));
                        const float ty = ySrcf - float(int32_t(y0));
                        const float invTx = 1.0f - tx;
                        const float invTy = 1.0f - ty;

                        float p0[4];
                        float p1[4];
                        float p2[4];
                        float p3[4];
                        vec4Mul(p0, src0, invTx*invTy);
                        vec4Mul(p1, src1,    tx*invTy);
                        vec4Mul(p2, src2, invTx*   ty);
                        vec4Mul(p3, src3,    tx*   ty);

                        const float rr = p0[0] + p1[0] + p2[0] + p3[0];
                        const float gg = p0[1] + p1[1] + p2[1] + p3[1];
                        const float bb = p0[2] + p1[2] + p2[2] + p3[2];
                        const float aa = p0[3] + p1[3] + p2[3] + p3[3];

                        dstColumnData[0] = rr;
                        dstColumnData[1] = gg;
                        dstColumnData[2] = bb;
                        dstColumnData[3] = aa;
                    }
                    else
                    {
                        const uint32_t xSrc = cmft::ftou(xSrcf);
                        const uint32_t ySrc = cmft::ftou(ySrcf);

                        const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcOffsets[faceIdx][mip];
                        const float *src = (const float*)((const uint8_t*)srcFaceData + ySrc*srcPitch + xSrc*bytesPerPixel);

                        dstColumnData[0] = src[0];
                        dstColumnData[1] = src[1];
                        dstColumnData[2] = src[2];
                        dstColumnData[3] = src[3];
                    }
                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstSize;
        result.m_height = dstSize;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = imageRgba32f.m_numMips;
        result.m_numFaces = 1;
        result.m_data = dstData;

        // Convert back to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result, _allocator);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result, _allocator);
            imageUnload(result, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);

        return true;


    }

    bool imageCubemapFromOctant(Image& _dst, const Image& _src, bool _useBilinearInterpolation, AllocatorI* _allocator)
    {
        if (!imageIsOctant(_src))
        {
            return false;
        }

        // Conversion is done in rgba32f format.
        ImageSoftRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);

        // Alloc data.
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstFaceSize = (imageRgba32f.m_height+1)/2;
        const uint32_t dstPitch = dstFaceSize * bytesPerPixel;
        const uint32_t dstFaceDataSize = dstPitch * dstFaceSize;
        const uint32_t dstDataSize = dstFaceDataSize * CUBE_FACE_NUM;
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source parameters.
        const float srcWidthMinusOne  = float(int32_t(imageRgba32f.m_width-1));
        const float srcHeightMinusOne = float(int32_t(imageRgba32f.m_height-1));
        const uint32_t srcPitch = imageRgba32f.m_width * bytesPerPixel;
        const float invDstFaceSizef = 1.0f/float(dstFaceSize);

        // Iterate over destination image (cubemap).
        for (uint8_t face = 0; face < 6; ++face)
        {
            uint8_t* dstFaceData = (uint8_t*)dstData + face*dstFaceDataSize;
            for (uint32_t yy = 0; yy < dstFaceSize; ++yy)
            {
                uint8_t* dstRowData = (uint8_t*)dstFaceData + yy*dstPitch;
                for (uint32_t xx = 0; xx < dstFaceSize; ++xx)
                {
                    float* dstColumnData = (float*)((uint8_t*)dstRowData + xx*bytesPerPixel);

                    // Cubemap (u,v) on current face.
                    const float uu = 2.0f*xx*invDstFaceSizef-1.0f;
                    const float vv = 2.0f*yy*invDstFaceSizef-1.0f;

                    // Get cubemap vector (x,y,z) from (u,v,faceIdx).
                    float vec[3];
                    texelCoordToVec(vec, uu, vv, face);

                    // Convert cubemap vector (x,y,z) to latlong (u,v).
                    float xSrcf;
                    float ySrcf;
                    octantFromVec(xSrcf, ySrcf, vec);

                    // Convert from [0..1] to [0..(size-1)] range.
                    xSrcf *= srcWidthMinusOne;
                    ySrcf *= srcHeightMinusOne;

                    // Sample from latlong (u,v).
                    if (_useBilinearInterpolation)
                    {
                        const uint32_t x0 = cmft::ftou(xSrcf);
                        const uint32_t y0 = cmft::ftou(ySrcf);
                        const uint32_t x1 = CMFT_MIN(x0+1, imageRgba32f.m_width-1);
                        const uint32_t y1 = CMFT_MIN(y0+1, imageRgba32f.m_height-1);

                        const float *src0 = (const float*)((const uint8_t*)imageRgba32f.m_data + y0*srcPitch + x0*bytesPerPixel);
                        const float *src1 = (const float*)((const uint8_t*)imageRgba32f.m_data + y0*srcPitch + x1*bytesPerPixel);
                        const float *src2 = (const float*)((const uint8_t*)imageRgba32f.m_data + y1*srcPitch + x0*bytesPerPixel);
                        const float *src3 = (const float*)((const uint8_t*)imageRgba32f.m_data + y1*srcPitch + x1*bytesPerPixel);

                        const float tx = xSrcf - float(int32_t(x0));
                        const float ty = ySrcf - float(int32_t(y0));
                        const float invTx = 1.0f - tx;
                        const float invTy = 1.0f - ty;

                        float p0[4];
                        float p1[4];
                        float p2[4];
                        float p3[4];
                        vec4Mul(p0, src0, invTx*invTy);
                        vec4Mul(p1, src1,    tx*invTy);
                        vec4Mul(p2, src2, invTx*   ty);
                        vec4Mul(p3, src3,    tx*   ty);

                        const float rr = p0[0] + p1[0] + p2[0] + p3[0];
                        const float gg = p0[1] + p1[1] + p2[1] + p3[1];
                        const float bb = p0[2] + p1[2] + p2[2] + p3[2];
                        const float aa = p0[3] + p1[3] + p2[3] + p3[3];

                        dstColumnData[0] = rr;
                        dstColumnData[1] = gg;
                        dstColumnData[2] = bb;
                        dstColumnData[3] = aa;
                    }
                    else
                    {
                        const uint32_t xSrc = cmft::ftou(xSrcf);
                        const uint32_t ySrc = cmft::ftou(ySrcf);
                        const float *src = (const float*)((const uint8_t*)imageRgba32f.m_data + ySrc*srcPitch + xSrc*bytesPerPixel);

                        dstColumnData[0] = src[0];
                        dstColumnData[1] = src[1];
                        dstColumnData[2] = src[2];
                        dstColumnData[3] = src[3];
                    }

                }
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = dstFaceSize;
        result.m_height = dstFaceSize;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = 1;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Convert result to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result, _allocator);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result, _allocator);
            imageUnload(result, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);

        return true;
    }

    bool imageCubemapFromOctant(Image& _image, bool _useBilinearInterpolation, AllocatorI* _allocator)
    {
        Image tmp;
        if(imageCubemapFromOctant(tmp, _image, _useBilinearInterpolation, _allocator))
        {
            imageMove(_image, tmp, _allocator);
            return true;
        }

        return false;
    }

    // Image loading.
    //-----

    bool imageLoadDds(Image& _image, Rw* _rw, AllocatorI* _allocator)
    {
        size_t read;
        CMFT_UNUSED(read);

        bool didOpen = rwFileOpen(_rw, "rb");
        RwScopeFileClose scopeClose(_rw, didOpen);

        RwSeekFn seekFn = rwSeekFnFor(_rw);
        RwReadFn readFn = rwReadFnFor(_rw);

        // Read magic.
        uint32_t magic;
        readFn(_rw, &magic, sizeof(uint32_t));

        // Check magic.
        if (DDS_MAGIC != magic)
        {
            WARN("Dds magic invalid.");
            return false;
        }

        // Read header.
        DdsHeader ddsHeader;
        read = 0;
        read += readFn(_rw, &ddsHeader.m_size, sizeof(ddsHeader.m_size));
        read += readFn(_rw, &ddsHeader.m_flags, sizeof(ddsHeader.m_flags));
        read += readFn(_rw, &ddsHeader.m_height, sizeof(ddsHeader.m_height));
        read += readFn(_rw, &ddsHeader.m_width, sizeof(ddsHeader.m_width));
        read += readFn(_rw, &ddsHeader.m_pitchOrLinearSize, sizeof(ddsHeader.m_pitchOrLinearSize));
        read += readFn(_rw, &ddsHeader.m_depth, sizeof(ddsHeader.m_depth));
        read += readFn(_rw, &ddsHeader.m_mipMapCount, sizeof(ddsHeader.m_mipMapCount));
        read += readFn(_rw, &ddsHeader.m_reserved1, sizeof(ddsHeader.m_reserved1));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_size, sizeof(ddsHeader.m_pixelFormat.m_size));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_flags, sizeof(ddsHeader.m_pixelFormat.m_flags));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_fourcc, sizeof(ddsHeader.m_pixelFormat.m_fourcc));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_rgbBitCount, sizeof(ddsHeader.m_pixelFormat.m_rgbBitCount));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_rBitMask, sizeof(ddsHeader.m_pixelFormat.m_rBitMask));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_gBitMask, sizeof(ddsHeader.m_pixelFormat.m_gBitMask));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_bBitMask, sizeof(ddsHeader.m_pixelFormat.m_bBitMask));
        read += readFn(_rw, &ddsHeader.m_pixelFormat.m_aBitMask, sizeof(ddsHeader.m_pixelFormat.m_aBitMask));
        read += readFn(_rw, &ddsHeader.m_caps, sizeof(ddsHeader.m_caps));
        read += readFn(_rw, &ddsHeader.m_caps2, sizeof(ddsHeader.m_caps2));
        read += readFn(_rw, &ddsHeader.m_caps3, sizeof(ddsHeader.m_caps3));
        read += readFn(_rw, &ddsHeader.m_caps4, sizeof(ddsHeader.m_caps4));
        read += readFn(_rw, &ddsHeader.m_reserved2, sizeof(ddsHeader.m_reserved2));
        DEBUG_CHECK(read == DDS_HEADER_SIZE, "Error reading file header.");

        // Read DdsDxt10 header if present.
        DdsHeaderDxt10 ddsHeaderDxt10;
        memset(&ddsHeaderDxt10, 0, sizeof(DdsHeaderDxt10));
        const bool hasDdsDxt10 = (DDS_DX10 == ddsHeader.m_pixelFormat.m_fourcc && (ddsHeader.m_flags&DDPF_FOURCC));
        if (hasDdsDxt10)
        {
            read = 0;
            read += readFn(_rw, &ddsHeaderDxt10.m_dxgiFormat, sizeof(ddsHeaderDxt10.m_dxgiFormat));
            read += readFn(_rw, &ddsHeaderDxt10.m_resourceDimension, sizeof(ddsHeaderDxt10.m_resourceDimension));
            read += readFn(_rw, &ddsHeaderDxt10.m_miscFlags, sizeof(ddsHeaderDxt10.m_miscFlags));
            read += readFn(_rw, &ddsHeaderDxt10.m_arraySize, sizeof(ddsHeaderDxt10.m_arraySize));
            read += readFn(_rw, &ddsHeaderDxt10.m_miscFlags2, sizeof(ddsHeaderDxt10.m_miscFlags2));
            DEBUG_CHECK(read == DDS_DX10_HEADER_SIZE, "Error reading Dds dx10 file header.");
        }

        // Validate header.
        if (DDS_HEADER_SIZE != ddsHeader.m_size)
        {
            WARN("Invalid Dds header size!");
            return false;
        }

        if ((ddsHeader.m_flags & (DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT)) != (DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT))
        {
            WARN("Invalid Dds header flags!");
            return false;
        }

        if (0 == (ddsHeader.m_caps & DDSCAPS_TEXTURE))
        {
            WARN("Invalid Dds header caps!");
            return false;
        }

        if (0 == ddsHeader.m_mipMapCount)
        {
            WARN("Dds image mipmap count is 0. Setting to 1.");
            ddsHeader.m_mipMapCount = 1;
        }

        const bool isCubemap = (0 != (ddsHeader.m_caps2 & DDSCAPS2_CUBEMAP));
        if (isCubemap && (DDS_CUBEMAP_ALLFACES != (ddsHeader.m_caps2 & DDS_CUBEMAP_ALLFACES)))
        {
            WARN("Partial cubemap not supported!");
            return false;
        }

        // Get format.
        TextureFormat::Enum format = TextureFormat::Null;
        if (hasDdsDxt10)
        {
            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateDdsDxgiFormat); ii < end; ++ii)
            {
                if (s_translateDdsDxgiFormat[ii].m_dxgiFormat == ddsHeaderDxt10.m_dxgiFormat)
                {
                    format = s_translateDdsDxgiFormat[ii].m_textureFormat;
                    break;
                }
            }
        }
        else
        {
            uint32_t ddsBcFlag = 0;
            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateDdsPfBitCount); ii < end; ++ii)
            {
                if (s_translateDdsPfBitCount[ii].m_bitCount == ddsHeader.m_pixelFormat.m_rgbBitCount)
                {
                    ddsBcFlag = s_translateDdsPfBitCount[ii].m_flag;
                    break;
                }
            }

            const uint32_t ddsFormat = ddsHeader.m_pixelFormat.m_flags & DDPF_FOURCC
                ? ddsHeader.m_pixelFormat.m_fourcc
                : (ddsHeader.m_pixelFormat.m_flags | ddsBcFlag)
                ;

            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateDdsFormat); ii < end; ++ii)
            {
                if (s_translateDdsFormat[ii].m_format == ddsFormat)
                {
                    format = s_translateDdsFormat[ii].m_textureFormat;
                    break;
                }
            }
        }

        if (TextureFormat::Null == format)
        {
            const uint8_t bytesPerPixel = uint8_t(ddsHeader.m_pixelFormat.m_rgbBitCount/8);
            for (uint8_t ii = 0, end = CMFT_COUNTOF(s_ddsValidFormats); ii < end; ++ii)
            {
                if (bytesPerPixel == getImageDataInfo(s_ddsValidFormats[ii]).m_bytesPerPixel)
                {
                    format = TextureFormat::Enum(ii);
                }
            }

            WARN("DDS data format unknown. Guessing...");

            if (TextureFormat::Null == format)
            {
                WARN("DDS data format not supported!");
                return false;
            }
        }

        // Calculate data size.
        const uint8_t numFaces = isCubemap ? 6 : 1;
        const uint32_t bytesPerPixel = getImageDataInfo(format).m_bytesPerPixel;
        uint32_t dataSize = 0;
        for (uint8_t face = 0; face < numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < ddsHeader.m_mipMapCount; ++mip)
            {
                uint32_t width  = CMFT_MAX(UINT32_C(1), ddsHeader.m_width  >> mip);
                uint32_t height = CMFT_MAX(UINT32_C(1), ddsHeader.m_height >> mip);
                dataSize += width * height * bytesPerPixel;
            }
        }

        // Some software tools produce invalid dds file.
        // Flags claim there should be a ddsdxt10 header after dds header but in fact image data starts there.
        // Therefore, to handle those situations, image data size will be checked against remaining unread data size.

        // Seek to the end to get remaining data size.
        const int64_t currentPos = seekFn(_rw, 0, Whence::Current);
        const int64_t endPos     = seekFn(_rw, 0, Whence::End);
        const int64_t remaining  = endPos - currentPos;

        // Seek back to currentPos or 20 before currentPos in case remaining unread data size does match image data size.
        seekFn(_rw, currentPos - DDS_DX10_HEADER_SIZE*(remaining == dataSize-DDS_DX10_HEADER_SIZE), Whence::Begin);

        // Alloc and read data.
        void* data = CMFT_ALLOC(_allocator, dataSize);
        MALLOC_CHECK(data);
        read = readFn(_rw, data, dataSize);
        DEBUG_CHECK(read == dataSize, "Could not read dds image data.");

        // Fill image structure.
        Image result;
        result.m_width = ddsHeader.m_width;
        result.m_height = ddsHeader.m_height;
        result.m_dataSize = dataSize;
        result.m_format = format;
        result.m_numMips = uint8_t(ddsHeader.m_mipMapCount);
        result.m_numFaces = numFaces;
        result.m_data = data;

        // Output.
        imageMove(_image, result, _allocator);

        return true;
    }

    bool imageLoadKtx(Image& _image, Rw* _rw, AllocatorI* _allocator)
    {
        size_t read;
        CMFT_UNUSED(read);

        bool didOpen = rwFileOpen(_rw, "rb");
        RwScopeFileClose scopeClose(_rw, didOpen);

        RwSeekFn seekFn = rwSeekFnFor(_rw);
        RwReadFn readFn = rwReadFnFor(_rw);

        KtxHeader ktxHeader;

        // Read magic.
        uint8_t magic[12];
        read = readFn(_rw, &magic, KTX_MAGIC_LEN);
        DEBUG_CHECK(read == 12, "Could not read from file.");

        const uint8_t ktxMagic[12] = KTX_MAGIC;
        if (0 != memcmp(magic, ktxMagic, KTX_MAGIC_LEN))
        {
            WARN("Ktx magic invalid.");
            return false;
        }

        // Read header.
        read = 0;
        read += readFn(_rw, &ktxHeader.m_endianness, sizeof(ktxHeader.m_endianness));
        read += readFn(_rw, &ktxHeader.m_glType, sizeof(ktxHeader.m_glType));
        read += readFn(_rw, &ktxHeader.m_glTypeSize, sizeof(ktxHeader.m_glTypeSize));
        read += readFn(_rw, &ktxHeader.m_glFormat, sizeof(ktxHeader.m_glFormat));
        read += readFn(_rw, &ktxHeader.m_glInternalFormat, sizeof(ktxHeader.m_glInternalFormat));
        read += readFn(_rw, &ktxHeader.m_glBaseInternalFormat, sizeof(ktxHeader.m_glBaseInternalFormat));
        read += readFn(_rw, &ktxHeader.m_pixelWidth, sizeof(ktxHeader.m_pixelWidth));
        read += readFn(_rw, &ktxHeader.m_pixelHeight, sizeof(ktxHeader.m_pixelHeight));
        read += readFn(_rw, &ktxHeader.m_pixelDepth, sizeof(ktxHeader.m_pixelDepth));
        read += readFn(_rw, &ktxHeader.m_numArrayElements, sizeof(ktxHeader.m_numArrayElements));
        read += readFn(_rw, &ktxHeader.m_numFaces, sizeof(ktxHeader.m_numFaces));
        read += readFn(_rw, &ktxHeader.m_numMips, sizeof(ktxHeader.m_numMips));
        read += readFn(_rw, &ktxHeader.m_bytesKeyValue, sizeof(ktxHeader.m_bytesKeyValue));
        DEBUG_CHECK(read == KTX_HEADER_SIZE, "Error reading Ktx file header.");

        if (0 == ktxHeader.m_numMips)
        {
            WARN("Ktx image mipmap count is 0. Setting to 1.");
            ktxHeader.m_numMips = 1;
        }

        // Get format.
        TextureFormat::Enum format = TextureFormat::Null;
        for (uint8_t ii = 0, end = CMFT_COUNTOF(s_translateKtxFormat); ii < end; ++ii)
        {
            if (s_translateKtxFormat[ii].m_glInternalFormat == ktxHeader.m_glInternalFormat)
            {
                format = s_translateKtxFormat[ii].m_textureFormat;
                break;
            }
        }

        if (TextureFormat::Null == format)
        {
            WARN("Ktx file internal format unknown.");
            return false;
        }

        const uint32_t bytesPerPixel = getImageDataInfo(format).m_bytesPerPixel;

        // Compute data offsets.
        uint32_t offsets[MAX_MIP_NUM][CUBE_FACE_NUM];
        uint32_t dataSize = 0;
        for (uint8_t face = 0; face < ktxHeader.m_numFaces; ++face)
        {
            for (uint8_t mip = 0; mip < ktxHeader.m_numMips; ++mip)
            {
                offsets[mip][face] = dataSize;
                const uint32_t width  = CMFT_MAX(UINT32_C(1), ktxHeader.m_pixelWidth  >> mip);
                const uint32_t height = CMFT_MAX(UINT32_C(1), ktxHeader.m_pixelHeight >> mip);
                dataSize += width * height * bytesPerPixel;
            }
        }

        // Alloc data.
        void* data = (void*)CMFT_ALLOC(_allocator, dataSize);
        MALLOC_CHECK(data);

        // Jump header key-value data.
        seekFn(_rw, ktxHeader.m_bytesKeyValue, Whence::Current);

        // Read data.
        for (uint8_t mip = 0; mip < ktxHeader.m_numMips; ++mip)
        {
            const uint32_t width  = CMFT_MAX(UINT32_C(1), ktxHeader.m_pixelWidth  >> mip);
            const uint32_t height = CMFT_MAX(UINT32_C(1), ktxHeader.m_pixelHeight >> mip);
            const uint32_t pitch  = width * bytesPerPixel;

            // Read face size.
            uint32_t faceSize;
            read = readFn(_rw, &faceSize, sizeof(faceSize));
            DEBUG_CHECK(read == 4, "Error reading Ktx data.");

            const uint32_t mipSize = faceSize * ktxHeader.m_numFaces;
            const uint32_t pitchRounding = (KTX_UNPACK_ALIGNMENT-1)-((pitch    + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t faceRounding  = (KTX_UNPACK_ALIGNMENT-1)-((faceSize + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t mipRounding   = (KTX_UNPACK_ALIGNMENT-1)-((mipSize  + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));

            if (faceSize != ((pitch + pitchRounding) * height))
            {
                WARN("Ktx face size invalid.");
            }

            for (uint8_t face = 0; face < ktxHeader.m_numFaces; ++face)
            {
                uint8_t* faceData = (uint8_t*)data + offsets[mip][face];

                if (0 == pitchRounding)
                {
                    // Read entire face at once.
                    read = readFn(_rw, faceData, faceSize);
                    DEBUG_CHECK(read == faceSize, "Error reading Ktx face data.");
                }
                else
                {
                    // Read row by row.
                    for (uint32_t yy = 0; yy < ktxHeader.m_pixelHeight; ++yy)
                    {
                        // Read row.
                        uint8_t* dst = (uint8_t*)faceData + yy*pitch;
                        read = readFn(_rw, dst, pitch);
                        DEBUG_CHECK(read == pitch, "Error reading Ktx row data.");

                        // Jump row rounding.
                        seekFn(_rw, pitchRounding, Whence::Current);
                    }
                }

                // Jump face rounding.
                seekFn(_rw, faceRounding, Whence::Current);
            }

            // Jump mip rounding.
            seekFn(_rw, mipRounding, Whence::Current);
        }

        // Fill image structure.
        Image result;
        result.m_width = ktxHeader.m_pixelWidth;
        result.m_height = ktxHeader.m_pixelHeight;
        result.m_dataSize = dataSize;
        result.m_format = format;
        result.m_numMips = uint8_t(ktxHeader.m_numMips);
        result.m_numFaces = uint8_t(ktxHeader.m_numFaces);
        result.m_data = data;

        // Output.
        imageMove(_image, result, _allocator);

        return true;
    }

    static inline const char* readLine(Rw* _rw, RwSeekFn _rwSeekFn, RwReadFn _rwReadFn, char* _out, uint32_t _max)
    {
        _rwReadFn(_rw, _out, _max);

        _out[_max-1] = '\0'; // Make sure the string is null terminated.

        const char* eol = cmft::streol(_out);
        const char* nl  = cmft::strnl(eol);

        // Seek back right after newline character.
        if (NULL != nl)
        {
            const int64_t pos = nl - _out - int32_t(_max);
            _rwSeekFn(_rw, pos, Whence::Current);
        }

        return nl;
    }

    bool imageLoadHdr(Image& _image, Rw* _rw, AllocatorI* _allocator)
    {
        size_t read;
        CMFT_UNUSED(read);

        bool didOpen = rwFileOpen(_rw, "rb");
        RwScopeFileClose scopeClose(_rw, didOpen);

        RwSeekFn seekFn = rwSeekFnFor(_rw);
        RwReadFn readFn = rwReadFnFor(_rw);

        // Read magic.
        char magic[HDR_MAGIC_LEN];
        readFn(_rw, magic, HDR_MAGIC_LEN);

        // Skip nl char.
        seekFn(_rw, 1, Whence::Current);

        // Check magic.
        if (0 != strncmp(magic, HDR_MAGIC_FULL, HDR_MAGIC_LEN))
        {
            WARN("HDR magic not valid.");
            return false;
        }

        HdrHeader hdrHeader;
        hdrHeader.m_valid = 0;
        hdrHeader.m_gamma = 1.0f;
        hdrHeader.m_exposure = 1.0f;

        // Read header.
        bool formatDefined = false;
        for (uint8_t ii = 0, stop = 20; ii < stop; ++ii)
        {
            // Read next line.
            char buf[256];
            const char* nl = readLine(_rw, seekFn, readFn, buf, sizeof(buf));

            if ((0 == buf[0])
            || ('\n' == buf[0]))
            {
                // End of header.
                break;
            }

            const size_t len = (NULL != nl) ? nl-buf : sizeof(buf);
            if (0 == strncmp(buf, "FORMAT=32-bit_rle_rgbe\n", len))
            {
                formatDefined = true;
            }
            else if (1 == sscanf(buf, "GAMMA=%g", &hdrHeader.m_gamma))
            {
                hdrHeader.m_valid |= HDR_VALID_GAMMA;
            }
            else if (1 == sscanf(buf, "EXPOSURE=%g", &hdrHeader.m_exposure))
            {
                hdrHeader.m_valid |= HDR_VALID_EXPOSURE;
            }
        }

        if (!formatDefined)
        {
            WARN("Invalid Hdr header.");
        }

        // Read image size.
        int32_t width;
        int32_t height;
        char buf[256];
        readLine(_rw, seekFn, readFn, buf, sizeof(buf));
        sscanf(buf, "-Y %d +X %d", &height, &width);

        // Allocate data.
        const uint32_t dataSize = width * height * 4 /* bytesPerPixel */;
        uint8_t* data = (uint8_t*)CMFT_ALLOC(_allocator, dataSize);
        MALLOC_CHECK(data);

        // Read first chunk.
        unsigned char rgbe[4];
        readFn(_rw, rgbe, sizeof(rgbe));

        uint8_t* dataPtr = (uint8_t*)data;

        if ((width < 8)
        || (width > 0x7fff)
        || (rgbe[0] != 2)
        || (rgbe[1] != 2)
        || (rgbe[2] & 0x80))
        {
            // File not RLE.

            // Save already read pixel.
            dataPtr[0] = rgbe[0];
            dataPtr[1] = rgbe[1];
            dataPtr[2] = rgbe[2];
            dataPtr[3] = rgbe[3];
            dataPtr += 4;

            // Read rest of the file.
            const uint32_t remainingDataSize = dataSize - 4;
            read = readFn(_rw, dataPtr, remainingDataSize);
            DEBUG_CHECK(read == remainingDataSize, "Error reading Hdr image data.");
        }
        else
        {
            // File is RLE.

            uint8_t* scanlineBuffer = (uint8_t*)alloca(width*4);
            MALLOC_CHECK(scanlineBuffer);
            uint8_t* ptr;
            const uint8_t* ptrEnd;
            uint32_t numScanlines = height-1;
            int32_t count;
            for (;;)
            {
                DEBUG_CHECK(((uint16_t(rgbe[2])<<8)|(rgbe[3]&0xff)) == width, "Hdr file scanline width is invalid.");

                ptr = scanlineBuffer;
                for (uint8_t ii = 0; ii < 4; ++ii)
                {
                    ptrEnd = (const uint8_t*)scanlineBuffer + width*(ii+1);
                    while (ptr < ptrEnd)
                    {
                        unsigned char rle[2];
                        readFn(_rw, rle, sizeof(rle));

                        if (rle[0] > 128)
                        {
                            // RLE chunk.
                            count = rle[0] - 128;
                            DEBUG_CHECK((count != 0) && (count <= (ptrEnd - ptr)), "Bad scanline data!");
                            while (count-- > 0)
                            {
                                *ptr++ = rle[1];
                            }
                        }
                        else
                        {
                            // Normal chunk.
                            count = rle[0];
                            DEBUG_CHECK((count != 0) && (count <= (ptrEnd - ptr)), "Bad scanline data!");
                            *ptr++ = rle[1];
                            if (--count > 0)
                            {
                                read = readFn(_rw, ptr, count);
                                DEBUG_CHECK(int32_t(read) == count, "Error reading Hdr image data.");
                                ptr += count;
                            }
                        }
                    }
                }

                // Copy scanline data.
                for (int32_t ii = 0; ii < width; ++ii)
                {
                    dataPtr[0] = scanlineBuffer[ii+(0*width)];
                    dataPtr[1] = scanlineBuffer[ii+(1*width)];
                    dataPtr[2] = scanlineBuffer[ii+(2*width)];
                    dataPtr[3] = scanlineBuffer[ii+(3*width)];
                    dataPtr += 4;
                }

                // Break if reached the end.
                if (0 == numScanlines--)
                {
                    break;
                }

                // Read next scanline.
                readFn(_rw, rgbe, sizeof(rgbe));
            }
        }

        // Fill image structure.
        Image result;
        result.m_width = uint32_t(width);
        result.m_height = uint32_t(height);
        result.m_dataSize = dataSize;
        result.m_format = TextureFormat::RGBE;
        result.m_numMips = 1;
        result.m_numFaces = 1;
        result.m_data = (void*)data;

        // Output.
        imageMove(_image, result, _allocator);

        return true;
    }

    bool imageLoadTga(Image& _image, Rw* _rw, AllocatorI* _allocator)
    {
        size_t read;
        CMFT_UNUSED(read);

        bool didOpen = rwFileOpen(_rw, "rb");
        RwScopeFileClose scopeClose(_rw, didOpen);

        RwSeekFn seekFn = rwSeekFnFor(_rw);
        RwReadFn readFn = rwReadFnFor(_rw);

        // Load header.
        TgaHeader tgaHeader;
        read = 0;
        read += readFn(_rw, &tgaHeader.m_idLength, sizeof(tgaHeader.m_idLength));
        read += readFn(_rw, &tgaHeader.m_colorMapType, sizeof(tgaHeader.m_colorMapType));
        read += readFn(_rw, &tgaHeader.m_imageType, sizeof(tgaHeader.m_imageType));
        read += readFn(_rw, &tgaHeader.m_colorMapOrigin, sizeof(tgaHeader.m_colorMapOrigin));
        read += readFn(_rw, &tgaHeader.m_colorMapLength, sizeof(tgaHeader.m_colorMapLength));
        read += readFn(_rw, &tgaHeader.m_colorMapDepth, sizeof(tgaHeader.m_colorMapDepth));
        read += readFn(_rw, &tgaHeader.m_xOrigin, sizeof(tgaHeader.m_xOrigin));
        read += readFn(_rw, &tgaHeader.m_yOrigin, sizeof(tgaHeader.m_yOrigin));
        read += readFn(_rw, &tgaHeader.m_width, sizeof(tgaHeader.m_width));
        read += readFn(_rw, &tgaHeader.m_height, sizeof(tgaHeader.m_height));
        read += readFn(_rw, &tgaHeader.m_bitsPerPixel, sizeof(tgaHeader.m_bitsPerPixel));
        read += readFn(_rw, &tgaHeader.m_imageDescriptor, sizeof(tgaHeader.m_imageDescriptor));
        DEBUG_CHECK(read == TGA_HEADER_SIZE, "Error reading file header.");

        // Check header.
        if(0 == (TGA_IT_RGB & tgaHeader.m_imageType))
        {
            WARN("Tga file is not true-color image.");
            return false;
        }

        // Get format.
        TextureFormat::Enum format;
        if (24 == tgaHeader.m_bitsPerPixel)
        {
            format = TextureFormat::BGR8;
            DEBUG_CHECK(0x0 == (tgaHeader.m_imageDescriptor&0xf), "Alpha channel not properly defined.");
        }
        else if (32 == tgaHeader.m_bitsPerPixel)
        {
            format = TextureFormat::BGRA8;
            DEBUG_CHECK(0x8 == (tgaHeader.m_imageDescriptor&0xf), "Alpha channel not properly defined.");
        }
        else
        {
            WARN("Non-supported Tga pixel depth - %u.", tgaHeader.m_bitsPerPixel);
            return false;
        }

        // Alloc data.
        const uint32_t numBytesPerPixel = tgaHeader.m_bitsPerPixel/8;
        const uint32_t numPixels = tgaHeader.m_width * tgaHeader.m_height;
        const uint32_t dataSize = numPixels * numBytesPerPixel;
        uint8_t* data = (uint8_t*)CMFT_ALLOC(_allocator, dataSize);
        MALLOC_CHECK(data);

        // Skip to data.
        const uint32_t skip = tgaHeader.m_idLength + (tgaHeader.m_colorMapType&0x1)*tgaHeader.m_colorMapLength;
        seekFn(_rw, skip, Whence::Current);

        // Load data.
        const bool bCompressed = (0 != (tgaHeader.m_imageType&TGA_IT_RLE));
        if (bCompressed)
        {
            uint8_t buf[5];
            uint32_t n = 0;
            uint8_t* dataPtr = data;
            while (n < numPixels)
            {
                read = readFn(_rw, buf, 1+numBytesPerPixel);
                DEBUG_CHECK(read == (1+numBytesPerPixel), "Could not read from file.");

                const uint8_t count = buf[0] & 0x7f;

                memcpy(dataPtr, &buf[1], numBytesPerPixel);
                dataPtr += numBytesPerPixel;
                n++;

                if (buf[0] & 0x80)
                {
                    // RLE chunk.
                    for (uint8_t ii = 0; ii < count; ++ii)
                    {
                        memcpy(dataPtr, &buf[1], numBytesPerPixel);
                        dataPtr += numBytesPerPixel;
                        n++;
                    }
                }
                else
                {
                    // Normal chunk.
                    for (uint8_t ii = 0; ii < count; ++ii)
                    {
                        read = readFn(_rw, buf, numBytesPerPixel);
                        DEBUG_CHECK(read == +numBytesPerPixel, "Could not read from file.");

                        memcpy(dataPtr, buf, numBytesPerPixel);
                        dataPtr += numBytesPerPixel;
                        n++;
                    }
                }
            }
        }
        else
        {
            read = readFn(_rw, data, dataSize);
            DEBUG_CHECK(read == dataSize, "Could not read from file.");
        }

        // Fill image structure.
        Image result;
        result.m_width = tgaHeader.m_width;
        result.m_height = tgaHeader.m_height;
        result.m_dataSize = dataSize;
        result.m_format = format;
        result.m_numMips = 1;
        result.m_numFaces = 1;
        result.m_data = data;

        // Flip if necessary.
        const uint32_t flip = 0
                            | (tgaHeader.m_imageDescriptor & TGA_DESC_HORIZONTAL ? IMAGE_OP_FLIP_Y : 0)
                            | (tgaHeader.m_imageDescriptor & TGA_DESC_VERTICAL   ? 0 : IMAGE_OP_FLIP_X)
                            ;
        if (flip)
        {
            imageTransform(result, flip);
        }

        // Output.
        imageMove(_image, result, _allocator);

        return true;
    }

    static bool isTga(uint32_t _magic)
    {
        //byte 2 is imageType and must be: 1, 2, 3, 9, 10 or 11
        //byte 1 is colorMapType and must be 1 if imageType is 1 or 9, 0 otherwise
        const uint8_t colorMapType = uint8_t((_magic>> 8)&0xff);
        const uint8_t imageType    = uint8_t((_magic>>16)&0xff);
        switch(imageType)
        {
        case 1:
        case 9:
            return (1 == colorMapType);

        case 2:
        case 3:
        case 10:
        case 11:
            return (0 == colorMapType);
        };

        return false;
    }

    bool imageLoad(Image& _image, Rw* _rw, TextureFormat::Enum _convertTo, AllocatorI* _allocator)
    {
        bool didOpen = rwFileOpen(_rw, "rb");
        RwScopeFileClose scopeClose(_rw, didOpen);

        if (!rwFileOpened(_rw))
        {
            return false;
        }

        RwSeekFn seekFn = rwSeekFnFor(_rw);
        RwReadFn readFn = rwReadFnFor(_rw);

        // Read magic.
        uint32_t magic;
        readFn(_rw, &magic, sizeof(magic));

        // Seek to beginning.
        seekFn(_rw, 0, Whence::Begin);

        // Load image.
        bool loaded = false;
        if (DDS_MAGIC == magic)
        {
            loaded = imageLoadDds(_image, _rw, _allocator);
        }
        else if (HDR_MAGIC == magic)
        {
            loaded = imageLoadHdr(_image, _rw, _allocator);
        }
        else if (KTX_MAGIC_SHORT == magic)
        {
            loaded = imageLoadKtx(_image, _rw, _allocator);
        }
        else if (isTga(magic))
        {
            loaded = imageLoadTga(_image, _rw, _allocator);
        }

        if (!loaded)
        {
            return false;
        }

        // Convert if necessary.
        if (TextureFormat::Null != _convertTo
        &&  _image.m_format != _convertTo)
        {
            imageConvert(_image, _convertTo, _allocator);
        }

        return true;
    }

    bool imageLoad(Image& _image, const char* _filePath, TextureFormat::Enum _convertTo, AllocatorI* _allocator)
    {
        Rw rw;
        rwInit(&rw, _filePath);

        return imageLoad(_image, &rw, _convertTo, _allocator);
    }

    bool imageLoad(Image& _image, const void* _data, uint32_t _dataSize, TextureFormat::Enum _convertTo, AllocatorI* _allocator)
    {
        Rw rw;
        rwInit(&rw, const_cast<void*>(_data), _dataSize);

        return imageLoad(_image, &rw, _convertTo, _allocator);
    }

    ///
    bool imageLoadStb(Image& _image, const char* _filePath, TextureFormat::Enum _convertTo, AllocatorI* _allocator)
    {
        // Try loading the image through stb_image.
        int stbWidth, stbHeight, stbNumComponents;
        // Passing reqNumComponents as 4 forces RGBA8 in data.
        // After stbi_load, stbNumComponents will hold the actual # of components from the source image.
        const int reqNumComponents = 4;
        uint8_t* data = (uint8_t*)stbi_load(_filePath, &stbWidth, &stbHeight, &stbNumComponents, reqNumComponents);

        if (NULL == data)
        {
            return false;
        }

        // Fill image structure.
        Image result;
        result.m_width    = (uint16_t)stbWidth;
        result.m_height   = (uint16_t)stbHeight;
        result.m_dataSize = stbWidth*stbHeight*reqNumComponents;
        result.m_format   = cmft::TextureFormat::RGBA8;
        result.m_numMips  = 1;
        result.m_numFaces = 1;
        result.m_data     = data;

        // Convert if necessary.
        if (TextureFormat::Null != _convertTo
        &&  _image.m_format != _convertTo)
        {
            imageConvert(_image, _convertTo, result, _allocator);
        }
        else
        {
            imageCopy(_image, result, _allocator); //TODO: use imageMove instead of imageCopy if the same allocator was used from stbi_load().
        }

        stbi_image_free(data);

        return true;
    }

    ///
    bool imageLoadStb(Image& _image, const void* _data, uint32_t _dataSize, TextureFormat::Enum _convertTo, AllocatorI* _allocator)
    {
        // Try loading the image through stb_image.
        int stbWidth, stbHeight, stbNumComponents;
        // Passing reqNumComponents as 4 forces RGBA8 in data.
        // After stbi_load, stbNumComponents will hold the actual # of components from the source image.
        const int reqNumComponents = 4;
        uint8_t* data = (uint8_t*)stbi_load_from_memory((const stbi_uc*)_data, (int)_dataSize, &stbWidth, &stbHeight, &stbNumComponents, reqNumComponents);

        if (NULL == data)
        {
            return false;
        }

        // Fill image structure.
        Image result;
        result.m_width    = (uint16_t)stbWidth;
        result.m_height   = (uint16_t)stbHeight;
        result.m_dataSize = stbWidth*stbHeight*reqNumComponents;
        result.m_format   = cmft::TextureFormat::RGBA8;
        result.m_numMips  = 1;
        result.m_numFaces = 1;
        result.m_data     = data;

        // Convert if necessary.
        if (TextureFormat::Null != _convertTo
        &&  _image.m_format != _convertTo)
        {
            imageConvert(_image, _convertTo, result, _allocator);
        }
        else
        {
            imageCopy(_image, result, _allocator); //TODO: use imageMove instead of imageCopy if the same allocator was used from stbi_load().
        }

        stbi_image_free(data);

        return true;
    }

    bool imageIsValid(const Image& _image)
    {
        return (NULL != _image.m_data);
    }

    // Image saving.
    //-----

    bool imageSaveDds(const char* _fileName, const Image& _image)
    {
        size_t write;
        CMFT_UNUSED(write);

        char fileName[CMFT_PATH_LEN];
        strcpy(fileName, _fileName);
        cmft::strlcat(fileName, getFilenameExtensionStr(ImageFileType::DDS), CMFT_PATH_LEN);

        // Open file.
        FILE* fp = fopen(fileName, "wb");
        if (NULL == fp)
        {
            WARN("Could not open file %s for writing.", fileName);
            return false;
        }
        cmft::ScopeFclose cleanup(fp);

        DdsHeader ddsHeader;
        DdsHeaderDxt10 ddsHeaderDxt10;
        ddsHeaderFromImage(ddsHeader, &ddsHeaderDxt10, _image);

        // Write magic.
        const uint32_t magic = DDS_MAGIC;
        write = fwrite(&magic, 1, 4, fp);
        DEBUG_CHECK(write == sizeof(magic), "Error writing Dds magic.");
        FERROR_CHECK(fp);

        // Write header.
        write = 0;
        write += fwrite(&ddsHeader.m_size,                      1, sizeof(ddsHeader.m_size),                      fp);
        write += fwrite(&ddsHeader.m_flags,                     1, sizeof(ddsHeader.m_flags),                     fp);
        write += fwrite(&ddsHeader.m_height,                    1, sizeof(ddsHeader.m_height),                    fp);
        write += fwrite(&ddsHeader.m_width,                     1, sizeof(ddsHeader.m_width),                     fp);
        write += fwrite(&ddsHeader.m_pitchOrLinearSize,         1, sizeof(ddsHeader.m_pitchOrLinearSize),         fp);
        write += fwrite(&ddsHeader.m_depth,                     1, sizeof(ddsHeader.m_depth),                     fp);
        write += fwrite(&ddsHeader.m_mipMapCount,               1, sizeof(ddsHeader.m_mipMapCount),               fp);
        write += fwrite(&ddsHeader.m_reserved1,                 1, sizeof(ddsHeader.m_reserved1),                 fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_size,        1, sizeof(ddsHeader.m_pixelFormat.m_size),        fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_flags,       1, sizeof(ddsHeader.m_pixelFormat.m_flags),       fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_fourcc,      1, sizeof(ddsHeader.m_pixelFormat.m_fourcc),      fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_rgbBitCount, 1, sizeof(ddsHeader.m_pixelFormat.m_rgbBitCount), fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_rBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_rBitMask),    fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_gBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_gBitMask),    fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_bBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_bBitMask),    fp);
        write += fwrite(&ddsHeader.m_pixelFormat.m_aBitMask,    1, sizeof(ddsHeader.m_pixelFormat.m_aBitMask),    fp);
        write += fwrite(&ddsHeader.m_caps,                      1, sizeof(ddsHeader.m_caps),                      fp);
        write += fwrite(&ddsHeader.m_caps2,                     1, sizeof(ddsHeader.m_caps2),                     fp);
        write += fwrite(&ddsHeader.m_caps3,                     1, sizeof(ddsHeader.m_caps3),                     fp);
        write += fwrite(&ddsHeader.m_caps4,                     1, sizeof(ddsHeader.m_caps4),                     fp);
        write += fwrite(&ddsHeader.m_reserved2,                 1, sizeof(ddsHeader.m_reserved2),                 fp);
        DEBUG_CHECK(write == DDS_HEADER_SIZE, "Error writing Dds file header.");
        FERROR_CHECK(fp);

        if (DDS_DX10 == ddsHeader.m_pixelFormat.m_fourcc)
        {
            write = 0;
            write += fwrite(&ddsHeaderDxt10.m_dxgiFormat,        1, sizeof(ddsHeaderDxt10.m_dxgiFormat),        fp);
            write += fwrite(&ddsHeaderDxt10.m_resourceDimension, 1, sizeof(ddsHeaderDxt10.m_resourceDimension), fp);
            write += fwrite(&ddsHeaderDxt10.m_miscFlags,         1, sizeof(ddsHeaderDxt10.m_miscFlags),         fp);
            write += fwrite(&ddsHeaderDxt10.m_arraySize,         1, sizeof(ddsHeaderDxt10.m_arraySize),         fp);
            write += fwrite(&ddsHeaderDxt10.m_miscFlags2,        1, sizeof(ddsHeaderDxt10.m_miscFlags2),        fp);
            DEBUG_CHECK(write == DDS_DX10_HEADER_SIZE, "Error writing Dds dx10 file header.");
            FERROR_CHECK(fp);
        }

        // Write data.
        DEBUG_CHECK(NULL != _image.m_data, "Image data is null.");
        write = fwrite(_image.m_data, 1, _image.m_dataSize, fp);
        DEBUG_CHECK(write == _image.m_dataSize, "Error writing Dds image data.");
        FERROR_CHECK(fp);

        return true;
    }

    bool imageSaveKtx(const char* _fileName, const Image& _image)
    {
        char fileName[CMFT_PATH_LEN];
        strcpy(fileName, _fileName);
        cmft::strlcat(fileName, getFilenameExtensionStr(ImageFileType::KTX), CMFT_PATH_LEN);

        // Open file.
        FILE* fp = fopen(fileName, "wb");
        if (NULL == fp)
        {
            WARN("Could not open file %s for writing.", fileName);
            return false;
        }
        cmft::ScopeFclose cleanup(fp);

        KtxHeader ktxHeader;
        ktxHeaderFromImage(ktxHeader, _image);

        size_t write;
        CMFT_UNUSED(write);

        // Write magic.
        const uint8_t magic[KTX_MAGIC_LEN+1] = KTX_MAGIC;
        write = fwrite(&magic, 1, KTX_MAGIC_LEN, fp);
        DEBUG_CHECK(write == KTX_MAGIC_LEN, "Error writing Ktx magic.");
        FERROR_CHECK(fp);

        // Write header.
        write = 0;
        write += fwrite(&ktxHeader.m_endianness,           1, sizeof(ktxHeader.m_endianness),           fp);
        write += fwrite(&ktxHeader.m_glType,               1, sizeof(ktxHeader.m_glType),               fp);
        write += fwrite(&ktxHeader.m_glTypeSize,           1, sizeof(ktxHeader.m_glTypeSize),           fp);
        write += fwrite(&ktxHeader.m_glFormat,             1, sizeof(ktxHeader.m_glFormat),             fp);
        write += fwrite(&ktxHeader.m_glInternalFormat,     1, sizeof(ktxHeader.m_glInternalFormat),     fp);
        write += fwrite(&ktxHeader.m_glBaseInternalFormat, 1, sizeof(ktxHeader.m_glBaseInternalFormat), fp);
        write += fwrite(&ktxHeader.m_pixelWidth,           1, sizeof(ktxHeader.m_pixelWidth),           fp);
        write += fwrite(&ktxHeader.m_pixelHeight,          1, sizeof(ktxHeader.m_pixelHeight),          fp);
        write += fwrite(&ktxHeader.m_pixelDepth,           1, sizeof(ktxHeader.m_pixelDepth),           fp);
        write += fwrite(&ktxHeader.m_numArrayElements,     1, sizeof(ktxHeader.m_numArrayElements),     fp);
        write += fwrite(&ktxHeader.m_numFaces,             1, sizeof(ktxHeader.m_numFaces),             fp);
        write += fwrite(&ktxHeader.m_numMips,              1, sizeof(ktxHeader.m_numMips),              fp);
        write += fwrite(&ktxHeader.m_bytesKeyValue,        1, sizeof(ktxHeader.m_bytesKeyValue),        fp);
        DEBUG_CHECK(write == KTX_HEADER_SIZE, "Error writing Ktx header.");

        // Get source offsets.
        uint32_t offsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        imageGetMipOffsets(offsets, _image);

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;
        const uint8_t pad[4] = { 0, 0, 0, 0 };

        // Write data.
        DEBUG_CHECK(NULL != _image.m_data, "Image data is null.");
        for (uint8_t mip = 0; mip < _image.m_numMips; ++mip)
        {
            const uint32_t width  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
            const uint32_t height = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);

            const uint32_t pitch = width * bytesPerPixel;
            const uint32_t faceSize = pitch * height;
            const uint32_t mipSize = faceSize * _image.m_numFaces;

            const uint32_t pitchRounding = (KTX_UNPACK_ALIGNMENT-1)-((pitch    + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t faceRounding  = (KTX_UNPACK_ALIGNMENT-1)-((faceSize + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));
            const uint32_t mipRounding   = (KTX_UNPACK_ALIGNMENT-1)-((mipSize  + KTX_UNPACK_ALIGNMENT-1)&(KTX_UNPACK_ALIGNMENT-1));

            // Write face size.
            write = fwrite(&faceSize, sizeof(uint32_t), 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Ktx data.");
            FERROR_CHECK(fp);

            for (uint8_t face = 0; face < _image.m_numFaces; ++face)
            {
                const uint8_t* faceData = (const uint8_t*)_image.m_data + offsets[face][mip];

                if (0 == pitchRounding)
                {
                    // Write entire face at once.
                    write = fwrite(faceData, 1, faceSize, fp);
                    DEBUG_CHECK(write == faceSize, "Error writing Ktx face data.");
                    FERROR_CHECK(fp);
                }
                else
                {
                    // Write row by row.
                    for (uint32_t yy = 0; yy < height; ++yy)
                    {
                        // Write row.
                        const uint8_t* src = (const uint8_t*)faceData + yy*pitch;
                        write = fwrite(src, 1, pitch, fp);
                        DEBUG_CHECK(write == pitch, "Error writing Ktx row data.");
                        FERROR_CHECK(fp);

                        // Write row rounding.
                        write = fwrite(&pad, 1, pitchRounding, fp);
                        DEBUG_CHECK(write == pitchRounding, "Error writing Ktx row rounding.");
                        FERROR_CHECK(fp);
                    }
                }

                // Write face rounding.
                if (faceRounding)
                {
                    write = fwrite(&pad, 1, faceRounding, fp);
                    DEBUG_CHECK(write == faceRounding, "Error writing Ktx face rounding.");
                    FERROR_CHECK(fp);
                }
            }

            // Write mip rounding.
            if (mipRounding)
            {
                write = fwrite(&pad, 1, mipRounding, fp);
                DEBUG_CHECK(write == mipRounding, "Error writing Ktx mip rounding.");
                FERROR_CHECK(fp);
            }
        }

        return true;
    }

    bool imageSaveHdr(const char* _fileName, const Image& _image, AllocatorI* _allocator)
    {
        char fileName[CMFT_PATH_LEN];
        char mipName[CMFT_PATH_LEN];

        strcpy(fileName, _fileName);

        const uint32_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;

        for (uint8_t mip = 0, endMip = _image.m_numMips; mip < endMip; ++mip)
        {
            cmft::stracpy(mipName, fileName);

            const uint32_t mipWidth  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
            const uint32_t mipHeight = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);

            if (_image.m_numMips != 1)
            {
                char mipStr[8];
                cmft::snprintf(mipStr, sizeof(mipStr), "%d", mip);

                char mipWidthStr[8];
                cmft::snprintf(mipWidthStr, sizeof(mipWidthStr), "%d", mipWidth);

                char mipHeightStr[8];
                cmft::snprintf(mipHeightStr, sizeof(mipHeightStr), "%d", mipHeight);

                cmft::strlcat(mipName, "_",          CMFT_PATH_LEN);
                cmft::strlcat(mipName, mipStr,       CMFT_PATH_LEN);
                cmft::strlcat(mipName, "_",          CMFT_PATH_LEN);
                cmft::strlcat(mipName, mipWidthStr,  CMFT_PATH_LEN);
                cmft::strlcat(mipName, "x",          CMFT_PATH_LEN);
                cmft::strlcat(mipName, mipHeightStr, CMFT_PATH_LEN);
            }

            cmft::strlcat(mipName, getFilenameExtensionStr(ImageFileType::HDR), CMFT_PATH_LEN);

            // Open file.
            FILE* fp = fopen(mipName, "wb");
            if (NULL == fp)
            {
                WARN("Could not open file %s for writing.", mipName);
                return false;
            }
            cmft::ScopeFclose cleanup(fp);

            // Hdr file type assumes rgbe image format.
            ImageSoftRef imageRgbe;
            imageRefOrConvert(imageRgbe, TextureFormat::RGBE, _image, _allocator);

            // Get image offsets.
            uint32_t imageOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
            imageGetMipOffsets(imageOffsets, _image);
            const uint8_t* mipData = (const uint8_t*)imageRgbe.m_data + imageOffsets[0][mip];

            if (1 != imageRgbe.m_numFaces)
            {
                WARN("Image seems to be containing more than one face. "
                     "Only the first one will be saved due to the limits of HDR format."
                    );
            }

            HdrHeader hdrHeader;
            hdrHeaderFromImage(hdrHeader, imageRgbe);

            size_t write = 0;
            CMFT_UNUSED(write);

            // Write magic.
            char magic[HDR_MAGIC_LEN+1] = HDR_MAGIC_FULL;
            magic[HDR_MAGIC_LEN] = '\n';
            write = fwrite(&magic, HDR_MAGIC_LEN+1, 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Hdr magic.");
            FERROR_CHECK(fp);

            // Write comment.
            char comment[21] = "# Output from cmft.\n";
            write = fwrite(&comment, 20, 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Hdr comment.");
            FERROR_CHECK(fp);

            // Write format.
            const char format[24] = "FORMAT=32-bit_rle_rgbe\n";
            write = fwrite(&format, 23, 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Hdr format.");
            FERROR_CHECK(fp);

            // Don't write gamma for now...
            //char gamma[32];
            //sprintf(gamma, "GAMMA=%g\n", hdrHeader.m_gamma);
            //const size_t gammaLen = strlen(gamma);
            //write = fwrite(&gamma, gammaLen, 1, fp);
            //DEBUG_CHECK(write == 1, "Error writing Hdr gamma.");
            //FERROR_CHECK(fp);

            // Write exposure.
            char exposure[32];
            sprintf(exposure, "EXPOSURE=%g\n", hdrHeader.m_exposure);
            const size_t exposureLen = strlen(exposure);
            write = fwrite(&exposure, exposureLen, 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Hdr exposure.");
            FERROR_CHECK(fp);

            // Write header terminator.
            char headerTerminator = '\n';
            write = fwrite(&headerTerminator, 1, 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Hdr header terminator.");
            FERROR_CHECK(fp);

            // Write image size.
            char imageSize[32];
            sprintf(imageSize, "-Y %d +X %d\n", mipHeight, mipWidth);
            const size_t imageSizeLen = strlen(imageSize);
            write = fwrite(&imageSize, imageSizeLen, 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Hdr image size.");
            FERROR_CHECK(fp);

            // Write data.
            DEBUG_CHECK(NULL != imageRgbe.m_data, "Image data is null.");
            write = fwrite(mipData, bytesPerPixel * mipWidth * mipHeight, 1, fp);
            DEBUG_CHECK(write == 1, "Error writing Hdr data.");
            FERROR_CHECK(fp);

            // Cleanup.
            imageUnload(imageRgbe, _allocator);
        }

        return true;
    }

    bool imageSaveTga(const char* _fileName, const Image& _image, bool _yflip = true)
    {
        char fileName[CMFT_PATH_LEN];
        char mipName[CMFT_PATH_LEN];

        bool result = true;
        const uint8_t bytesPerPixel = getImageDataInfo(_image.m_format).m_bytesPerPixel;

        for (uint8_t face = 0, endFace = _image.m_numFaces; face < endFace; ++face)
        {
            cmft::stracpy(fileName, _fileName);

            if (_image.m_numFaces != 1)
            {
                cmft::strlcat(fileName, "_", CMFT_PATH_LEN);
                cmft::strlcat(fileName, getCubemapFaceIdStr(face), CMFT_PATH_LEN);
            }

            for (uint8_t mip = 0, endMip = _image.m_numMips; mip < endMip; ++mip)
            {
                cmft::stracpy(mipName, fileName);

                const uint32_t mipWidth  = CMFT_MAX(UINT32_C(1), _image.m_width  >> mip);
                const uint32_t mipHeight = CMFT_MAX(UINT32_C(1), _image.m_height >> mip);
                const uint32_t mipPitch  = mipWidth * bytesPerPixel;

                if (_image.m_numMips != 1)
                {
                    char mipStr[8];
                    cmft::snprintf(mipStr, sizeof(mipStr), "%d", mip);

                    char mipWidthStr[8];
                    cmft::snprintf(mipWidthStr, sizeof(mipWidthStr), "%d", mipWidth);

                    char mipHeightStr[8];
                    cmft::snprintf(mipHeightStr, sizeof(mipHeightStr), "%d", mipHeight);

                    cmft::strlcat(mipName, "_",          CMFT_PATH_LEN);
                    cmft::strlcat(mipName, mipStr,       CMFT_PATH_LEN);
                    cmft::strlcat(mipName, "_",          CMFT_PATH_LEN);
                    cmft::strlcat(mipName, mipWidthStr,  CMFT_PATH_LEN);
                    cmft::strlcat(mipName, "x",          CMFT_PATH_LEN);
                    cmft::strlcat(mipName, mipHeightStr, CMFT_PATH_LEN);
                }

                cmft::strlcat(mipName, getFilenameExtensionStr(ImageFileType::TGA), CMFT_PATH_LEN);

                // Open file.
                FILE* fp = fopen(mipName, "wb");
                if (NULL == fp)
                {
                    WARN("Could not open file %s for writing.", mipName);
                    result = false;
                }

                TgaHeader tgaHeader;
                tgaHeaderFromImage(tgaHeader, _image, mip);

                // Write header.
                size_t write = 0;
                CMFT_UNUSED(write);
                write += fwrite(&tgaHeader.m_idLength,        1, sizeof(tgaHeader.m_idLength),        fp);
                write += fwrite(&tgaHeader.m_colorMapType,    1, sizeof(tgaHeader.m_colorMapType),    fp);
                write += fwrite(&tgaHeader.m_imageType,       1, sizeof(tgaHeader.m_imageType),       fp);
                write += fwrite(&tgaHeader.m_colorMapOrigin,  1, sizeof(tgaHeader.m_colorMapOrigin),  fp);
                write += fwrite(&tgaHeader.m_colorMapLength,  1, sizeof(tgaHeader.m_colorMapLength),  fp);
                write += fwrite(&tgaHeader.m_colorMapDepth,   1, sizeof(tgaHeader.m_colorMapDepth),   fp);
                write += fwrite(&tgaHeader.m_xOrigin,         1, sizeof(tgaHeader.m_xOrigin),         fp);
                write += fwrite(&tgaHeader.m_yOrigin,         1, sizeof(tgaHeader.m_yOrigin),         fp);
                write += fwrite(&tgaHeader.m_width,           1, sizeof(tgaHeader.m_width),           fp);
                write += fwrite(&tgaHeader.m_height,          1, sizeof(tgaHeader.m_height),          fp);
                write += fwrite(&tgaHeader.m_bitsPerPixel,    1, sizeof(tgaHeader.m_bitsPerPixel),    fp);
                write += fwrite(&tgaHeader.m_imageDescriptor, 1, sizeof(tgaHeader.m_imageDescriptor), fp);
                DEBUG_CHECK(write == TGA_HEADER_SIZE, "Error writing Tga header.");
                FERROR_CHECK(fp);

                // Get image offsets.
                uint32_t imageOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
                imageGetMipOffsets(imageOffsets, _image);

                // Write data. //TODO: implement RLE option.
                DEBUG_CHECK(NULL != _image.m_data, "Image data is null.");
                const uint8_t* mipData = (const uint8_t*)_image.m_data + imageOffsets[face][mip];
                if (_yflip)
                {
                    const uint8_t* src = (const uint8_t*)mipData + mipHeight * mipPitch;
                    for (uint32_t yy = 0; yy < mipHeight; ++yy)
                    {
                        src-=mipPitch;
                        write = fwrite(src, 1, mipPitch, fp);
                        DEBUG_CHECK(write == mipPitch, "Error writing Tga data.");
                        FERROR_CHECK(fp);
                    }
                }
                else
                {
                    const uint8_t* src = (const uint8_t*)mipData;
                    for (uint32_t yy = 0; yy < mipHeight; ++yy)
                    {
                        write = fwrite(src, 1, mipPitch, fp);
                        DEBUG_CHECK(write == mipPitch, "Error writing Tga data.");
                        FERROR_CHECK(fp);
                        src+=mipPitch;
                    }
                }

                // Write footer.
                TgaFooter tgaFooter = { 0, 0, TGA_ID };
                write  = fwrite(&tgaFooter.m_extensionOffset, 1, sizeof(tgaFooter.m_extensionOffset), fp);
                write += fwrite(&tgaFooter.m_developerOffset, 1, sizeof(tgaFooter.m_developerOffset), fp);
                write += fwrite(&tgaFooter.m_signature,       1, sizeof(tgaFooter.m_signature),       fp);
                DEBUG_CHECK(TGA_FOOTER_SIZE == write, "Error writing Tga footer.");
                FERROR_CHECK(fp);

                // Cleanup.
                fclose(fp);
            }

        }

        return result;
    }

    bool imageSave(const Image& _image, const char* _fileName, ImageFileType::Enum _ft, TextureFormat::Enum _convertTo, AllocatorI* _allocator)
    {
        // Get image in desired format.
        ImageSoftRef image;
        if (TextureFormat::Null != _convertTo)
        {
            imageRefOrConvert(image, _convertTo, _image, _allocator);
        }
        else
        {
            imageRef(image, _image);
        }

        // Check for valid texture format and save.
        bool result = false;
        if (checkValidTextureFormat(_ft, image.m_format))
        {
            if (ImageFileType::DDS == _ft)
            {
                result = imageSaveDds(_fileName, image);
            }
            else if (ImageFileType::KTX == _ft)
            {
                result = imageSaveKtx(_fileName, image);
            }
            else if (ImageFileType::TGA == _ft)
            {
                result = imageSaveTga(_fileName, image);
            }
            else if (ImageFileType::HDR == _ft)
            {
                result = imageSaveHdr(_fileName, image, _allocator);
            }
        }
        else
        {
            char buf[1024];
            getValidTextureFormatsStr(buf, _ft);

            WARN("Could not save %s as *.%s image."
                " Valid internal formats are: %s."
                " Choose one of the valid internal formats or a different file type.\n"
                , getTextureFormatStr(image.m_format)
                , getFilenameExtensionStr(_ft)
                , buf
                );
        }

        // Cleanup.
        imageUnload(image, _allocator);

        return result;
    }

    bool imageSave(const Image& _image, const char* _fileName, ImageFileType::Enum _ft, OutputType::Enum _ot, TextureFormat::Enum _tf, bool _printOutput, AllocatorI* _allocator)
    {
        // Input check.
        const bool validOutputType = checkValidOutputType(_ft, _ot);
        if (!validOutputType)
        {
            char validOutputTypes[128];
            getValidOutputTypesStr(validOutputTypes, _ft);
            WARN("Invalid output type for requested file type. File type: %s. Output type: %s."
                 " Valid output types for requested file type are: %s."
                 , getFileTypeStr(_ft), getOutputTypeStr(_ot)
                 , validOutputTypes
                 );
            return false;
        }

        const bool validTextureFormat = checkValidTextureFormat(_ft, _tf);
        if (!validTextureFormat)
        {
            char validTextureFormats[128];
            getValidTextureFormatsStr(validTextureFormats, _ft);
            WARN("Invalid texture format for requested file type. File type: %s. Output type: %s."
                 " Valid texture formats for requested file type are: %s."
                 , getFileTypeStr(_ft), getTextureFormatStr(_tf)
                 , validTextureFormats
                 );
            return false;
        }

        bool result = false;

        // Face list is a special case because it is saving 6 images.
        if (OutputType::FaceList == _ot)
        {
            Image outputFaceList[6];
            imageFaceListFromCubemap(outputFaceList, _image, _allocator);

            result = true;

            for (uint8_t face = 0; face < 6; ++face)
            {
                char faceFileName[2048];
                sprintf(faceFileName, "%s_%s", _fileName, getCubemapFaceIdStr(face));

                if (_printOutput)
                {
                    INFO("Saving %s%s [%s %ux%u %s %s %u-faces %d-mips]."
                        , faceFileName
                        , getFilenameExtensionStr(_ft)
                        , getFileTypeStr(_ft)
                        , outputFaceList[face].m_width
                        , outputFaceList[face].m_height
                        , getTextureFormatStr(outputFaceList[face].m_format)
                        , getOutputTypeStr(_ot)
                        , outputFaceList[face].m_numFaces
                        , outputFaceList[face].m_numMips
                        );
                }

                const bool saved = imageSave(outputFaceList[face], faceFileName, _ft, _tf, _allocator);
                if (!saved)
                {
                    WARN("Saving failed!");
                }

                result &= saved;
            }

            for (uint8_t face = 0; face < 6; ++face)
            {
                imageUnload(outputFaceList[face], _allocator);
            }

        }
        // Cubemap is a special case becase no transformation is required.
        else if (OutputType::Cubemap == _ot)
        {
            if (_printOutput)
            {
                INFO("Saving %s%s [%s %ux%u %s %s %u-faces %d-mips]."
                    , _fileName
                    , getFilenameExtensionStr(_ft)
                    , getFileTypeStr(_ft)
                    , _image.m_width
                    , _image.m_height
                    , getTextureFormatStr(_tf)
                    , getOutputTypeStr(_ot)
                    , _image.m_numFaces
                    , _image.m_numMips
                    );
            }

            result = imageSave(_image, _fileName, _ft, _tf, _allocator);
            if (!result)
            {
                WARN("Saving failed!");
            }
        }
        else
        {
            Image outputImage;

            if (OutputType::LatLong == _ot)
            {
                imageLatLongFromCubemap(outputImage, _image, true, _allocator);
            }
            else if (OutputType::HCross == _ot)
            {
                imageCrossFromCubemap(outputImage, _image, false, _allocator);
            }
            else if (OutputType::VCross == _ot)
            {
                imageCrossFromCubemap(outputImage, _image, true, _allocator);
            }
            else if (OutputType::HStrip == _ot)
            {
                imageStripFromCubemap(outputImage, _image, false, _allocator);
            }
            else if (OutputType::VStrip == _ot)
            {
                imageStripFromCubemap(outputImage, _image, true, _allocator);
            }
            else if (OutputType::Octant == _ot)
            {
                imageOctantFromCubemap(outputImage, _image, true, _allocator);
            }
            else
            {
                WARN("Invalid output type.");
                return false;
            }

            if (_printOutput)
            {
                INFO("Saving %s%s [%s %ux%u %s %s %u-faces %d-mips]."
                    , _fileName
                    , getFilenameExtensionStr(_ft)
                    , getFileTypeStr(_ft)
                    , outputImage.m_width
                    , outputImage.m_height
                    , getTextureFormatStr(_tf)
                    , getOutputTypeStr(_ot)
                    , outputImage.m_numFaces
                    , outputImage.m_numMips
                    );
            }

            result = imageSave(outputImage, _fileName, _ft, _tf, _allocator);
            if (!result)
            {
                WARN("Saving failed!");
            }

            imageUnload(outputImage, _allocator);
        }

        return result;
    }

    // ImageRef
    //-----

    bool imageAsCubemap(ImageSoftRef& _dst, const Image& _src, AllocatorI* _allocator)
    {
        if (imageIsCubemap(_src))
        {
            imageRef(_dst, _src);
            return true;
        }
        else if (imageCubemapFromCross(_dst, _src, _allocator)
             ||  imageCubemapFromLatLong(_dst, _src, true, _allocator)
             ||  imageCubemapFromStrip(_dst, _src, _allocator))
        {
            return true;
        }

        return false;
    }

    void imageRefOrConvert(ImageHardRef& _dst, TextureFormat::Enum _format, Image& _src, AllocatorI* _allocator)
    {
        if (_format == _src.m_format)
        {
            imageRef(_dst, _src);
        }
        else
        {
            imageUnload(_dst, _allocator);
            imageConvert(_dst, _format, _src, _allocator);
        }
    }

    void imageRefOrConvert(ImageSoftRef& _dst, TextureFormat::Enum _format, const Image& _src, AllocatorI* _allocator)
    {
        if (_format == _src.m_format)
        {
            imageRef(_dst, _src);
        }
        else
        {
            imageUnload(_dst, _allocator);
            imageConvert(_dst, _format, _src, _allocator);
        }
    }

    void imageRef(ImageSoftRef& _dst, const Image& _src)
    {
        _dst.m_data     = _src.m_data;
        _dst.m_width    = _src.m_width;
        _dst.m_height   = _src.m_height;
        _dst.m_dataSize = _src.m_dataSize;
        _dst.m_format   = _src.m_format;
        _dst.m_numMips  = _src.m_numMips;
        _dst.m_numFaces = _src.m_numFaces;
        _dst.m_isRef    = true;
    }

    void imageRef(ImageHardRef& _dst, Image& _src)
    {
        _dst.m_data        = _src.m_data;
        _dst.m_width       = _src.m_width;
        _dst.m_height      = _src.m_height;
        _dst.m_dataSize    = _src.m_dataSize;
        _dst.m_format      = _src.m_format;
        _dst.m_numMips     = _src.m_numMips;
        _dst.m_numFaces    = _src.m_numFaces;
        _dst.m_origDataPtr = &_src.m_data;
    }

    void imageMove(Image& _dst, ImageSoftRef& _src, AllocatorI* _allocator)
    {
        if (_src.isRef())
        {
            DEBUG_CHECK(false, "Soft reference cannot be moved!");
            abort();
        }

        imageUnload(_dst, _allocator);
        _dst.m_data     = _src.m_data;
        _dst.m_width    = _src.m_width;
        _dst.m_height   = _src.m_height;
        _dst.m_dataSize = _src.m_dataSize;
        _dst.m_format   = _src.m_format;
        _dst.m_numMips  = _src.m_numMips;
        _dst.m_numFaces = _src.m_numFaces;

        _src.m_data     = NULL;
    }

    void imageMove(Image& _dst, ImageHardRef& _src, AllocatorI* _allocator)
    {
        imageUnload(_dst, _allocator);
        _dst.m_data     = _src.m_data;
        _dst.m_width    = _src.m_width;
        _dst.m_height   = _src.m_height;
        _dst.m_dataSize = _src.m_dataSize;
        _dst.m_format   = _src.m_format;
        _dst.m_numMips  = _src.m_numMips;
        _dst.m_numFaces = _src.m_numFaces;

        _src.m_data = NULL;
        if (_src.isRef())
        {
            *_src.m_origDataPtr = NULL;
        }
    }

    void imageUnload(ImageSoftRef& _image, AllocatorI* _allocator)
    {
        if (_image.isCopy() && _image.m_data)
        {
            CMFT_FREE(_allocator, _image.m_data);
            _image.m_data = NULL;
        }
    }

    void imageUnload(ImageHardRef& _image, AllocatorI* _allocator)
    {
        if (_image.isCopy() && _image.m_data)
        {
            CMFT_FREE(_allocator, _image.m_data);
            _image.m_data = NULL;
        }
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
