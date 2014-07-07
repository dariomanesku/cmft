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

            Unknown,

            Count,
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
            : m_width(0)
            , m_height(0)
            , m_dataSize(0)
            , m_format(TextureFormat::Unknown)
            , m_numMips(0)
            , m_numFaces(0)
            , m_data(NULL)
        {
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
    const char* getFilenameExtensionStr(ImageFileType::Enum _ft);

    ///
    const char* getTextureFormatStr(TextureFormat::Enum _format);

    ///
    const TextureFormat::Enum* getValidTextureFormats(ImageFileType::Enum _fileType);

    ///
    void getValidTextureFormatsStr(char* _str, ImageFileType::Enum _fileType);

    ///
    bool checkValidInternalFormat(ImageFileType::Enum _fileType, TextureFormat::Enum _internalFormat);

    ///
    const ImageDataInfo& getImageDataInfo(TextureFormat::Enum _format);

    ///
    void imageUnload(Image& _image);

    ///
    void imageRef(Image& _dst, const Image& _src);

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
    /// If requested format is the same as source, creates a reference to it (data ptr is the same, member variables are copied).
    /// Otherwise creates a converted copy of the image.
    /// If _dst is a reference to _src, function returns true.
    /// If _dst is a physical copy of _src, function returns false (data should be released with imageUnload()).
    ///
    bool imageRefOrConvert(Image& _dst, TextureFormat::Enum _format, const Image& _src);

    ///
    void imageGetPixel(void* _out, TextureFormat::Enum _format, uint32_t _x, uint32_t _y, uint8_t _mip, uint8_t _face, const Image& _image);

    /// Notice: only base image gets resized. Mipmaps are lost.
    void imageResize(Image& _dst, uint32_t _width, uint32_t _height, const Image& _src);

    /// Notice: only base image gets resized. Mipmaps are lost.
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

    ///
    ///     +------+  ....__.......   ___________                         ___   ___
    ///    /|     /|  .  |  |     .  |           |                       |___| |   |_
    ///   +-+----+ |  .__|__|__ __.  |           |   __ __ __ __ __ __   |___| |___| |_
    ///   | |    | |  |  |  |  |  |  |           |  |  |  |  |  |  |  |  |___|   |___| |_
    ///   | +----+-+  |__|__|__|__|  |           |  |__|__|__|__|__|__|  |___|     |___| |_
    ///   |/     |/   .  |  |     .  |           |                       |___|       |___| |_
    ///   +------+    ...|__|......  |___________|                       |___|         |___| |
    ///                                                                            |___|
    ///
    ///    Cubemap     Cube Cross      Lat Long     Horizonatal Strip  Vertical Strip  Face list
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
    bool imageIsCubeCross(Image& _image);

    ///
    bool imageCubemapFromCross(Image& _dst, const Image& _src);

    ///
    void imageCubemapFromCross(Image& _image);

    ///
    bool imageCubemapFromLatLong(Image& _dst, const Image& _src, bool _useBilinearInterpolation = true);

    ///
    void imageCubemapFromLatLong(Image& _image, bool _useBilinearInterpolation = true);

    ///
    bool imageLatLongFromCubemap(Image& _dst, const Image& _src, bool _useBilinearInterpolation = true);

    ///
    void imageLatLongFromCubemap(Image& _cubemap, bool _useBilinearInterpolation = true);

    ///
    bool imageHStripFromCubemap(Image& _dst, const Image& _src);

    ///
    void imageHStripFromCubemap(Image& _image);

    ///
    bool imageCubemapFromHStrip(Image& _dst, const Image& _src);

    ///
    void imageCubemapFromHStrip(Image& _image);

    ///
    bool imageVStripFromCubemap(Image& _dst, const Image& _src);

    ///
    void imageVStripFromCubemap(Image& _image);

    ///
    bool imageCubemapFromVStrip(Image& _dst, const Image& _src);

    ///
    void imageCubemapFromVStrip(Image& _image);

    ///
    bool imageFaceListFromCubemap(Image _faceList[6], const Image& _cubemap);

    ///
    bool imageCubemapFromFaceList(Image& _cubemap, const Image _faceList[6]);

    ///
    bool imageCrossFromCubemap(Image& _dst, const Image& _src, bool _vertical = true);

    ///
    void imageCrossFromCubemap(Image& _image, bool _vertical = true);

    ///
    bool imageLoad(Image& _image, const char* _filePath, TextureFormat::Enum _convertTo = TextureFormat::Unknown);

    ///
    bool imageSave(const Image& _image, const char* _fileName, ImageFileType::Enum _ft, TextureFormat::Enum _convertTo = TextureFormat::Unknown);

} // namespace cmft

#endif //CMFT_IMAGE_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
