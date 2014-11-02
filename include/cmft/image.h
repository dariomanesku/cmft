/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_IMAGE_H_HEADER_GUARD
#define CMFT_IMAGE_H_HEADER_GUARD

#include <stdio.h>
#include <stdint.h>

#ifndef UINT8_MAX // Fixing mingw bug.
#define UINT8_MAX (255)
#endif //UINT8_MAX

namespace cmft
{

#define CUBE_FACE_NUM  6
#define MAX_MIP_NUM   16

    enum ImageTransformArgs
    {
        IMAGE_FACE_POSITIVEX = 0x0000,
        IMAGE_FACE_NEGATIVEX = 0x0001,
        IMAGE_FACE_POSITIVEY = 0x0002,
        IMAGE_FACE_NEGATIVEY = 0x0003,
        IMAGE_FACE_POSITIVEZ = 0x0004,
        IMAGE_FACE_NEGATIVEZ = 0x0005,
        IMAGE_FACE_SHIFT     = 0,
        IMAGE_FACE_MASK      = 0x0007,

        IMAGE_OP_ROT_90      = 0x0100,
        IMAGE_OP_ROT_180     = 0x0200,
        IMAGE_OP_ROT_270     = 0x0400,
        IMAGE_OP_FLIP_X      = 0x1000,
        IMAGE_OP_FLIP_Y      = 0x2000,
        IMAGE_OP_SHIFT       = 8,
        IMAGE_OP_MASK        = 0xff00,
    };

    struct ImageFileType
    {
        enum Enum
        {
            DDS,
            KTX,
            TGA,
            HDR,

            Count
        };
    };

    struct OutputType
    {
        enum Enum
        {
            LatLong,
            Cubemap,
            HCross,
            VCross,
            HStrip,
            VStrip,
            FaceList,

            Count,
            Null = -1,
        };
    };

    struct TextureFormat
    {
        enum Enum
        {
            BGR8,
            RGB8,
            RGB16,
            RGB16F,
            RGB32F,
            RGBE,

            BGRA8,
            RGBA8,
            RGBA16,
            RGBA16F,
            RGBA32F,

            Count,
            Null = -1,
        };
    };

    struct ImageDataInfo
    {
        uint8_t m_bytesPerPixel;
        uint8_t m_numChanels;
        uint8_t m_hasAlpha;
        uint8_t m_pixelType;
    };

    struct Image
    {
        Image()
        {
            m_width    = 0;
            m_height   = 0;
            m_dataSize = 0;
            m_format   = TextureFormat::Null;
            m_numMips  = 0;
            m_numFaces = 0;
            m_data     = NULL;
        }

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_dataSize;
        TextureFormat::Enum m_format;
        uint8_t m_numMips;
        uint8_t m_numFaces;
        void* m_data;
    };

    ///
    const char* getFileTypeStr(ImageFileType::Enum _ft);

    ///
    const char* getOutputTypeStr(OutputType::Enum _ot);

    ///
    const char* getFilenameExtensionStr(ImageFileType::Enum _ft);

    ///
    const char* getTextureFormatStr(TextureFormat::Enum _format);

    /// Returns a OutputType::Null terminating array of valid output types for requested file type.
    const OutputType::Enum* getValidOutputTypes(ImageFileType::Enum _fileType);

    ///
    void getValidOutputTypesStr(char* _str, ImageFileType::Enum _fileType);

    ///
    bool checkValidOutputType(ImageFileType::Enum _fileType, OutputType::Enum _outputType);

    /// Returns a TextureFormat::Null terminating array of valid texture formats for requested file type.
    const TextureFormat::Enum* getValidTextureFormats(ImageFileType::Enum _fileType);

    ///
    void getValidTextureFormatsStr(char* _str, ImageFileType::Enum _fileType);

    ///
    bool checkValidTextureFormat(ImageFileType::Enum _fileType, TextureFormat::Enum _textureFormat);

    ///
    const ImageDataInfo& getImageDataInfo(TextureFormat::Enum _format);

    ///
    void imageCreate(Image& _image, uint32_t _width, uint32_t _height, uint32_t _rgba = 0x303030ff, uint8_t _numMips = 1, uint8_t _numFaces = 1, TextureFormat::Enum _format = TextureFormat::RGBA32F);

    ///
    void imageUnload(Image& _image);

    ///
    void imageMove(Image& _dst, Image& _src);

    ///
    void imageCopy(Image& _dst, const Image& _src);

    ///
    uint32_t imageGetNumPixels(const Image& _image);

    ///
    void imageGetMipOffsets(uint32_t _offsets[CUBE_FACE_NUM][MAX_MIP_NUM], const Image& _image);

    ///
    void imageGetFaceOffsets(uint32_t _faceOffsets[CUBE_FACE_NUM], const Image& _image);

    ///
    void toRgba32f(float _rgba32f[4], TextureFormat::Enum _srcFormat, const void* _src);

    ///
    void imageToRgba32f(Image& _dst, const Image& _src);

    ///
    void imageToRgba32f(Image& _image);

    ///
    void fromRgba32f(void* _out, TextureFormat::Enum _format, const float _rgba32f[4]);

    ///
    void imageFromRgba32f(Image& _dst, TextureFormat::Enum _dstFormat, const Image& _src);

    ///
    void imageFromRgba32f(Image& _image, TextureFormat::Enum _textureFormat);

    ///
    void imageConvert(Image& _dst, TextureFormat::Enum _dstFormat, const Image& _src);

    ///
    void imageConvert(Image& _image, TextureFormat::Enum _format);

    ///
    void imageGetPixel(void* _out, TextureFormat::Enum _format, uint32_t _x, uint32_t _y, uint8_t _mip, uint8_t _face, const Image& _image);

    ///
    void imageResize(Image& _dst, uint32_t _width, uint32_t _height, const Image& _src);

    ///
    void imageResize(Image& _image, uint32_t _width, uint32_t _height);

    /// Notice: because all transformations are done on data in place,
    /// rotations work properly only when image width == image height (which is true for cubemap images).
    /// Flip operations work properly regardless of aspect ratio.
#define imageTransform(_image, ...) imageTransformUseMacroInstead(&(_image), __VA_ARGS__, UINT32_MAX)
    void imageTransformUseMacroInstead(Image* _image, ...);

    ///
    void imageGenerateMipMapChain(Image& _image, uint8_t _numMips=UINT8_MAX);

    ///
    void imageApplyGamma(Image& _image, float _gammaPow);

    ///
    void imageClamp(Image& _dst, const Image& _src);

    ///
    void imageClamp(Image& _image);

    ///                                .....___.....
    ///     +------+   ....__.......   .   |   |   .    _________________                           ___     ___
    ///    /|     /|   .  |  |     .   .___|___|___.   |                 |                         |___|   |   |_
    ///   +-+----+ |   .__|__|__ __.   |   |   |   |   |                 |    __ __ __ __ __ __    |___|   |___| |_
    ///   | |    | |   |  |  |  |  |   |___|___|___|   |                 |   |  |  |  |  |  |  |   |___|     |___| |_
    ///   | +----+-+   |__|__|__|__|   .   |   |   .   |                 |   |__|__|__|__|__|__|   |___|       |___| |_
    ///   |/     |/    .  |  |     .   .   |___|   .   |_________________|                         |___|         |___| |_
    ///   +------+     ...|__|......   .   |   |   .                                               |___|           |___| |
    ///                                ....|___|....                                                                 |___|
    ///
    ///    Cubemap        HCross           VCross           Lat Long               HStrip          VStrip     Face list
    ///

    ///
    bool imageIsCubemap(const Image& _image);

    ///
    /// Checks if image is a latlong image. Not an actual test, just checks the image ratio.
    ///
    bool imageIsLatLong(const Image& _image);

    ///
    bool imageIsHStrip(const Image& _image);

    ///
    bool imageIsVStrip(const Image& _image);

    ///
    bool imageValidCubemapFaceList(const Image _faceList[6]);

    ///
    bool imageIsCubeCross(const Image& _image, bool _fastCheck = false);

    ///
    bool imageIsEnvironmentMap(const Image& _image, bool _fastCheck = false);

    ///
    bool imageCubemapFromCross(Image& _dst, const Image& _src);

    ///
    bool imageCubemapFromCross(Image& _image);

    ///
    bool imageCubemapFromLatLong(Image& _dst, const Image& _src, bool _useBilinearInterpolation = true);

    ///
    bool imageCubemapFromLatLong(Image& _image, bool _useBilinearInterpolation = true);

    ///
    bool imageLatLongFromCubemap(Image& _dst, const Image& _src, bool _useBilinearInterpolation = true);

    ///
    bool imageLatLongFromCubemap(Image& _cubemap, bool _useBilinearInterpolation = true);

    ///
    bool imageStripFromCubemap(Image& _dst, const Image& _src, bool _vertical = false);

    ///
    bool imageStripFromCubemap(Image& _image, bool _vertical = false);

    ///
    bool imageCubemapFromStrip(Image& _dst, const Image& _src);

    ///
    bool imageCubemapFromStrip(Image& _image);

    ///
    bool imageFaceListFromCubemap(Image _faceList[6], const Image& _cubemap);

    ///
    bool imageCubemapFromFaceList(Image& _cubemap, const Image _faceList[6]);

    ///
    bool imageCrossFromCubemap(Image& _dst, const Image& _src, bool _vertical = true);

    ///
    bool imageCrossFromCubemap(Image& _image, bool _vertical = true);

    ///
    bool imageToCubemap(Image& _dst, const Image& _src);

    ///
    bool imageToCubemap(Image& _image);

    ///
    bool imageLoad(Image& _image, const char* _filePath, TextureFormat::Enum _convertTo = TextureFormat::Null);

    ///
    bool imageIsValid(const Image& _image);

    ///
    bool imageSave(const Image& _image, const char* _fileName, ImageFileType::Enum _ft, TextureFormat::Enum _convertTo = TextureFormat::Null);

    ///
    bool imageSave(const Image& _image, const char* _fileName, ImageFileType::Enum _ft, OutputType::Enum _ot, TextureFormat::Enum _tf = TextureFormat::Null, bool _printOutput = false);

    // ImageRef
    //-----

    struct ImageSoftRef : public Image
    {
        ImageSoftRef()
        {
            m_isRef = false;
        }

        inline bool isRef()  const { return m_isRef; }
        inline bool isCopy() const { return !m_isRef; }

        bool m_isRef;
    };

    struct ImageHardRef : public Image
    {
        ImageHardRef()
        {
            m_origDataPtr = NULL;
        }

        inline bool isRef()  const { return (NULL != m_origDataPtr); }
        inline bool isCopy() const { return (NULL == m_origDataPtr); }

        void** m_origDataPtr;
    };

    /// If requested format is the same as source, _dst becomes a reference to _src.
    /// Otherwise, _dst is filled with a converted copy of the image.
    /// Either way, imageUnload() should be called on _dst and it will free the data in case a copy was made.
    void imageRefOrConvert(ImageHardRef& _dst, TextureFormat::Enum _format, Image& _src);

    ///
    void imageRefOrConvert(ImageSoftRef& _dst, TextureFormat::Enum _format, const Image& _src);

    ///
    void imageRef(ImageSoftRef& _dst, const Image& _src);

    ///
    void imageRef(ImageHardRef& _dst, Image& _src);

    ///
    void imageMove(Image& _dst, ImageSoftRef& _src);

    ///
    void imageMove(Image& _dst, ImageHardRef& _src);

    ///
    void imageUnload(ImageSoftRef& _image);

    ///
    void imageUnload(ImageHardRef& _image);

} // namespace cmft

#endif //CMFT_IMAGE_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
