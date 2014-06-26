/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "cmft/cubemapfilter.h"

#include "base/config.h"
#include "base/utils.h"
#include "cubemaputils.h"
#include "radiance.h"
#include "messages.h"

#include <stdlib.h> //malloc
#include <string.h> //memset
#include <math.h> //pow, sqrt
#include <float.h> //FLT_MAX

#include <bx/timer.h> //bx::getHPFrequency
#include <bx/os.h> //bx::sleep
#include <bx/thread.h> //bx::thread
#include <bx/mutex.h> //bx::mutex

namespace cmft
{

#define PI      3.1415926535897932384626433832795028841971693993751058
#define PI4     12.566370614359172953850573533118011536788677597500423
#define PI16    50.265482457436691815402294132472046147154710390001693
#define PI64    201.06192982974676726160917652988818458861884156000677
#define SQRT_PI 1.7724538509055160272981674833411451827975494561223871

    /// Creates a cubemap containing tap vectors and solid angle of each texel on the cubemap.
    /// Output consists of 6 faces of specified size containing (x,y,z,angle) floats for each texel.
    ///
    /// Memory should be freed outside of the function !
    float* buildCubemapNormalSolidAngle(uint32_t _cubemapFaceSize)
    {
        const uint32_t size = _cubemapFaceSize /*width*/
                            * _cubemapFaceSize /*height*/
                            * 6 /*numFaces*/
                            * 4 /*numChannels*/
                            * 4 /*bytesPerChannel*/
                            ;
        float* dst = (float*)malloc(size);
        MALLOC_CHECK(dst);

        const float invFaceSize = 1.0f/float(int32_t(_cubemapFaceSize));

        float* dstPtr = dst;
        for(uint8_t face = 0; face < 6; ++face)
        {
            for (uint32_t yy = 0; yy < _cubemapFaceSize; ++yy)
            {
                for (uint32_t xx = 0; xx < _cubemapFaceSize; ++xx)
                {
                    // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                    const float xxf = float(int32_t(xx));
                    const float yyf = float(int32_t(yy));
                    const float uu = 2.0f*(xxf+0.5f)*invFaceSize - 1.0f;
                    const float vv = 2.0f*(yyf+0.5f)*invFaceSize - 1.0f;

                    texelCoordToVec(dstPtr, uu, vv, face, _cubemapFaceSize);
                    dstPtr[3] = texelSolidAngle(uu, vv, invFaceSize);

                    dstPtr += 4;
                }
            }
        }

        return dst;
    }

    // Irradiance.
    //-----

    void evalSHBasis5(double* _shBasis, const float* _dir)
    {
        const double x = double(_dir[0]);
        const double y = double(_dir[1]);
        const double z = double(_dir[2]);

        const double x2 = x*x;
        const double y2 = y*y;
        const double z2 = z*z;

        const double z3 = pow(z, 3.0);

        const double x4 = pow(x, 4.0);
        const double y4 = pow(y, 4.0);
        const double z4 = pow(z, 4.0);

        //Equations based on data from: http://ppsloan.org/publications/StupidSH36.pdf
        _shBasis[ 0] =  1.0/(2.0*SQRT_PI);

        _shBasis[ 1] = -sqrt(3.0/PI4)*y;
        _shBasis[ 2] =  sqrt(3.0/PI4)*z;
        _shBasis[ 3] = -sqrt(3.0/PI4)*x;

        _shBasis[ 4] =  sqrt(15.0/PI4)*y*x;
        _shBasis[ 5] = -sqrt(15.0/PI4)*y*z;
        _shBasis[ 6] =  sqrt(5.0/PI16)*(3.0*z2-1.0);
        _shBasis[ 7] = -sqrt(15.0/PI4)*x*z;
        _shBasis[ 8] =  sqrt(15.0/PI16)*(x2-y2);

        _shBasis[ 9] = -sqrt( 70.0/PI64)*y*(3*x2-y2);
        _shBasis[10] =  sqrt(105.0/ PI4)*y*x*z;
        _shBasis[11] = -sqrt( 21.0/PI16)*y*(-1.0+5.0*z2);
        _shBasis[12] =  sqrt(  7.0/PI16)*(5.0*z3-3.0*z);
        _shBasis[13] = -sqrt( 42.0/PI64)*x*(-1.0+5.0*z2);
        _shBasis[14] =  sqrt(105.0/PI16)*(x2-y2)*z;
        _shBasis[15] = -sqrt( 70.0/PI64)*x*(x2-3.0*y2);

        _shBasis[16] =  3.0*sqrt(35.0/PI16)*x*y*(x2-y2);
        _shBasis[17] = -3.0*sqrt(70.0/PI64)*y*z*(3.0*x2-y2);
        _shBasis[18] =  3.0*sqrt( 5.0/PI16)*y*x*(-1.0+7.0*z2);
        _shBasis[19] = -3.0*sqrt(10.0/PI64)*y*z*(-3.0+7.0*z2);
        _shBasis[20] =  (105.0*z4-90.0*z2+9.0)/(16.0*SQRT_PI);
        _shBasis[21] = -3.0*sqrt(10.0/PI64)*x*z*(-3.0+7.0*z2);
        _shBasis[22] =  3.0*sqrt( 5.0/PI64)*(x2-y2)*(-1.0+7.0*z2);
        _shBasis[23] = -3.0*sqrt(70.0/PI64)*x*z*(x2-3.0*y2);
        _shBasis[24] =  3.0*sqrt(35.0/(4.0*PI64))*(x4-6.0*y2*x2+y4);
    }

    void cubemapShCoeffs(double _shCoeffs[SH_COEFF_NUM][3], void* _data, uint32_t _faceSize, uint32_t _faceOffsets[6])
    {
        memset(_shCoeffs, 0, SH_COEFF_NUM*3*sizeof(double));

        double weightAccum = 0.0;

        // Build cubemap vectors.
        float* cubemapVectors = buildCubemapNormalSolidAngle(_faceSize);
        ScopeFree cleanup(cubemapVectors);
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t vectorPitch = _faceSize * bytesPerPixel;
        const uint32_t vectorFaceDataSize = vectorPitch * _faceSize;

        // Evaluate spherical harmonics coefficients.
        for (uint8_t face = 0; face < 6; ++face)
        {
            const float* srcPtr = (const float*)((const uint8_t*)_data + _faceOffsets[face]);
            const float* vecPtr = (const float*)((const uint8_t*)cubemapVectors + vectorFaceDataSize*face);

            for (uint32_t texel = 0, count = _faceSize*_faceSize; texel < count; ++texel, srcPtr+=4, vecPtr+=4)
            {
                const double rr = double(srcPtr[0]);
                const double gg = double(srcPtr[1]);
                const double bb = double(srcPtr[2]);

                double shBasis[SH_COEFF_NUM];
                evalSHBasis5(shBasis, vecPtr);

                const double weight = (double)vecPtr[3];

                for (uint8_t ii = 0; ii < SH_COEFF_NUM; ++ii)
                {
                    _shCoeffs[ii][0] += rr * shBasis[ii] * weight;
                    _shCoeffs[ii][1] += gg * shBasis[ii] * weight;
                    _shCoeffs[ii][2] += bb * shBasis[ii] * weight;
                }

                weightAccum += weight;
            }
        }

        // Normalization.
        // This is not really necesarry because usually PI*4 - weightAccum ~= 0.000003
        // so it doesn't change almost anything, but it doesn't cost much to have more corectness.
        const double norm = PI4 / weightAccum;
        for (uint8_t ii = 0; ii < SH_COEFF_NUM; ++ii)
        {
            _shCoeffs[ii][0] *= norm;
            _shCoeffs[ii][1] *= norm;
            _shCoeffs[ii][2] *= norm;
        }
    }

    bool imageShCoeffs(double _shCoeffs[SH_COEFF_NUM][3], const Image& _image)
    {
        // Input image must be a cubemap.
        if (!imageIsCubemap(_image))
        {
            return false;
        }

        // Processing is done in Rgba32f format.
        Image imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image);

        // Get face data offsets.
        uint32_t faceOffsets[6];
        imageGetFaceOffsets(faceOffsets, imageRgba32f);

        // Compute spherical harmonic coefficients.
        cubemapShCoeffs(_shCoeffs, imageRgba32f.m_data, imageRgba32f.m_width, faceOffsets);

        // Cleanup.
        if (TextureFormat::RGBA32F != _image.m_format)
        {
            imageUnload(imageRgba32f);
        }

        return true;
    }

    bool imageIrradianceFilterSh(Image& _dst, uint32_t _dstFaceSize, const Image& _src)
    {
        // Input image must be a cubemap.
        if (!imageIsCubemap(_src))
        {
            return false;
        }

        // Processing is done in Rgba32f format.
        Image imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src);

        // Get face data offsets.
        uint32_t faceOffsets[6];
        imageGetFaceOffsets(faceOffsets, imageRgba32f);

        // Compute spherical harmonic coefficients.
        double shRgb[SH_COEFF_NUM][3];
        cubemapShCoeffs(shRgb, imageRgba32f.m_data, imageRgba32f.m_width, faceOffsets);

        // Alloc dst data.
        const uint32_t dstFaceSize = (0 == _dstFaceSize) ? _src.m_width : _dstFaceSize;
        const uint8_t dstBytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t dstPitch = dstFaceSize*dstBytesPerPixel;
        const uint32_t dstFaceDataSize = dstPitch * dstFaceSize;
        const uint32_t dstDataSize = dstFaceDataSize * 6 /*numFaces*/;
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Build cubemap texel vectors.
        float* cubemapVectors = buildCubemapNormalSolidAngle(dstFaceSize);
        ScopeFree cleanup(cubemapVectors);
        const uint8_t vectorBytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t vectorPitch = dstFaceSize * vectorBytesPerPixel;
        const uint32_t vectorFaceDataSize = vectorPitch * dstFaceSize;

        uint64_t totalTime = bx::getHPCounter();

        // Output info.
        INFO("Running irradiance filter for:\n"
             "\t[srcFaceSize=%u]\n"
             "\t[shOrder=5]\n"
             "\t[dstFaceSize=%u]"
             , imageRgba32f.m_width
             , dstFaceSize
             );

        // Compute irradiance using SH data.
        for (uint8_t face = 0; face < 6; ++face)
        {
            float* dstPtr = (float*)((uint8_t*)dstData + dstFaceDataSize*face);
            const float* vecPtr = (const float*)((const uint8_t*)cubemapVectors + vectorFaceDataSize*face);

            for (uint32_t texel = 0, count = dstFaceSize*dstFaceSize; texel < count ;++texel, dstPtr+=4, vecPtr+=4)
            {
                double shBasis[SH_COEFF_NUM];
                evalSHBasis5(shBasis, vecPtr);

                double rgb[3] = { 0.0, 0.0, 0.0 };

                // Band 0 (factor 1.0)
                rgb[0] += shRgb[0][0] * shBasis[0] * 1.0f;
                rgb[1] += shRgb[0][1] * shBasis[0] * 1.0f;
                rgb[2] += shRgb[0][2] * shBasis[0] * 1.0f;

                // Band 1 (factor 2/3).
                uint8_t ii = 1;
                for (; ii < 4; ++ii)
                {
                    rgb[0] += shRgb[ii][0] * shBasis[ii] * (2.0f/3.0f);
                    rgb[1] += shRgb[ii][1] * shBasis[ii] * (2.0f/3.0f);
                    rgb[2] += shRgb[ii][2] * shBasis[ii] * (2.0f/3.0f);
                }

                // Band 2 (factor 1/4).
                for (; ii < 9; ++ii)
                {
                    rgb[0] += shRgb[ii][0] * shBasis[ii] * (1.0f/4.0f);
                    rgb[1] += shRgb[ii][1] * shBasis[ii] * (1.0f/4.0f);
                    rgb[2] += shRgb[ii][2] * shBasis[ii] * (1.0f/4.0f);
                }

                // Band 3 (factor 0).
                ii = 16;

                // Band 4 (factor -1/24).
                for (; ii < 25; ++ii)
                {
                    rgb[0] += shRgb[ii][0] * shBasis[ii] * (-1.0f/24.0f);
                    rgb[1] += shRgb[ii][1] * shBasis[ii] * (-1.0f/24.0f);
                    rgb[2] += shRgb[ii][2] * shBasis[ii] * (-1.0f/24.0f);
                }

                dstPtr[0] = float(rgb[0]);
                dstPtr[1] = float(rgb[1]);
                dstPtr[2] = float(rgb[2]);
                dstPtr[3] = 1.0f;
            }
        }

        // Output progress info.
        const double freq = double(bx::getHPFrequency());
        const double toSec = 1.0/freq;
        totalTime = bx::getHPCounter() - totalTime;
        INFO("Irradiance -> Done! Total time: %.3f seconds.", double(totalTime)*toSec);

        // Fill structure.
        Image result;
        result.m_width = dstFaceSize;
        result.m_height = dstFaceSize;
        result.m_dataSize = dstFaceSize /*width*/
                          * dstFaceSize /*height*/
                          * 6 /*numFaces*/
                          * 4 /*numChannels*/
                          * 4 /*bytesPerChannel*/
                          ;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = 1;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Convert back to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result);
            imageUnload(result);
        }

        return true;
    }

    void imageIrradianceFilterSh(Image& _image, uint32_t _faceSize)
    {
        Image tmp;
        if (imageIrradianceFilterSh(tmp, _faceSize, _image))
        {
            imageMove(_image, tmp);
        }
    }

    // Radiance.
    //-----

    struct Aabb
    {
        Aabb()
        {
            m_min[0] = FLT_MAX;
            m_min[1] = FLT_MAX;
            m_max[0] = -FLT_MAX;
            m_max[1] = -FLT_MAX;
        }

        void add(float _x, float _y)
        {
            m_min[0] = min(m_min[0], _x);
            m_min[1] = min(m_min[1], _y);
            m_max[0] = max(m_max[0], _x);
            m_max[1] = max(m_max[1], _y);
        }

        inline void clampMin(float _x, float _y)
        {
            m_min[0] = max(m_min[0], _x);
            m_min[1] = max(m_min[1], _y);
        }

        inline void clampMax(float _x, float _y)
        {
            m_max[0] = min(m_max[0], _x);
            m_max[1] = min(m_max[1], _y);
        }

        void clamp(float _minX, float _minY, float _maxX, float _maxY)
        {
            clampMin(_minX, _minY);
            clampMax(_maxX, _maxY);
        }

        void clamp(float _min, float _max)
        {
            clampMin(_min, _min);
            clampMax(_max, _max);
        }

        bool isEmpty()
        {
            // Has to have at least two points added so that no value is equal to initial state.
            return ((m_min[0] ==  FLT_MAX)
                  ||(m_min[1] ==  FLT_MAX)
                  ||(m_max[0] == -FLT_MAX)
                  ||(m_max[1] == -FLT_MAX)
                   );
        }

        float m_min[2];
        float m_max[2];
    };

    /// Computes filter area for each of the cubemap faces for given tap vector and filter size.
    void determineFilterArea(Aabb _filterArea[6], const float* _tapVec, float _filterSize)
    {
        ///   ______
        ///  |      |
        ///  |      |
        ///  |    x |
        ///  |______|
        ///
        // Get face and hit coordinates.
        float uu, vv;
        uint8_t hitFaceIdx;
        vecToTexelCoord(uu, vv, hitFaceIdx, _tapVec);

        ///  ........
        ///  .      .
        ///  .   ___.
        ///  .  | x |
        ///  ...|___|
        ///
        // Calculate hit face filter bounds.
        Aabb hitFaceFilterBounds;
        hitFaceFilterBounds.add(uu-_filterSize, vv-_filterSize);
        hitFaceFilterBounds.add(uu+_filterSize, vv+_filterSize);
        hitFaceFilterBounds.clamp(0.0f, 1.0f);

        // Output result for hit face.
        DEBUG_CHECK(6 > hitFaceIdx, "Face idx should be in range 0-5");
        memcpy(&_filterArea[hitFaceIdx], &hitFaceFilterBounds, sizeof(Aabb));

        /// Filter area might extend on neighbour faces.
        /// Case when extending over the right edge:
        ///
        ///  --> U
        /// |        ......
        /// v       .      .
        /// V       .      .
        ///         .      .
        ///  ....... ...... .......
        ///  .      .      .      .
        ///  .      .  .....__min .
        ///  .      .  .   .  |  -> amount
        ///  ....... .....x.__|....
        ///         .  .   .  max
        ///         .  ........
        ///         .      .
        ///          ......
        ///         .      .
        ///         .      .
        ///         .      .
        ///          ......
        ///

        enum NeighbourSides
        {
            Left,
            Right,
            Top,
            Bottom,

            Count,
        };

        struct NeighourFaceBleed
        {
            float m_amount;
            float m_bbMin;
            float m_bbMax;
        } bleed[NeighbourSides::Count] =
        {
            { // Left
                _filterSize - uu,
                hitFaceFilterBounds.m_min[1],
                hitFaceFilterBounds.m_max[1],
            },
            { // Right
                uu + _filterSize - 1.0f,
                hitFaceFilterBounds.m_min[1],
                hitFaceFilterBounds.m_max[1],
            },
            { // Top
                _filterSize - vv,
                hitFaceFilterBounds.m_min[0],
                hitFaceFilterBounds.m_max[0],
            },
            { // Bottom
                vv + _filterSize - 1.0f,
                hitFaceFilterBounds.m_min[0],
                hitFaceFilterBounds.m_max[0],
            },
        };

        // Determine bleeding for each side.
        for (uint8_t side = 0; side < 4; ++side)
        {
            uint8_t currentFaceIdx = hitFaceIdx;

            for (float bleedAmount = bleed[side].m_amount; bleedAmount > 0.0f; bleedAmount -= 1.0f)
            {
                uint8_t neighbourFaceIdx  = s_cubeFaceNeighbours[currentFaceIdx][side].m_faceIdx;
                uint8_t neighbourFaceEdge = s_cubeFaceNeighbours[currentFaceIdx][side].m_faceEdge;
                currentFaceIdx = neighbourFaceIdx;

                ///
                /// https://code.google.com/p/cubemapgen/source/browse/trunk/CCubeMapProcessor.cpp#773
                ///
                /// Handle situations when bbMin and bbMax should be flipped.
                ///
                ///    L - Left           ....................T-T
                ///    R - Right          v                     .
                ///    T - Top        __________                .
                ///    B - Bottom    .          |               .
                ///                  .          |               .
                ///                  .          |<...R-T        .
                ///                  .          |    v          v
                ///        .......... ..........|__________ __________
                ///       .          .          .          .          .
                ///       .          .          .          .          .
                ///       .          .          .          .          .
                ///       .          .          .          .          .
                ///        __________ .......... .......... __________
                ///            ^     |          .               ^
                ///            .     |          .               .
                ///            B-L..>|          .               .
                ///                  |          .               .
                ///                  |__________.               .
                ///                       ^                     .
                ///                       ....................B-B
                ///
                /// Those are:
                ///     B-L, B-B
                ///     T-R, T-T
                ///     (and in reverse order, R-T and L-B)
                ///
                /// If we add, R-R and L-L (which never occur), we get:
                ///     B-L, B-B
                ///     T-R, T-T
                ///     R-T, R-R
                ///     L-B, L-L
                ///
                /// And if L = 0, R = 1, T = 2, B = 3 as in NeighbourSides enumeration,
                /// a general rule can be derived for when to flip bbMin and bbMax:
                ///     if ((a+b) == 3 || (a == b))
                ///     {
                ///        ..flip bbMin and bbMax
                ///     }
                ///
                float bbMin = bleed[side].m_bbMin;
                float bbMax = bleed[side].m_bbMax;
                if ((side == neighbourFaceEdge)
                || (3 == (side + neighbourFaceEdge)))
                {
                    // Flip.
                    bbMin = 1.0f - bbMin;
                    bbMax = 1.0f - bbMax;
                }

                switch (neighbourFaceEdge)
                {
                case CMFT_EDGE_LEFT:
                    {
                        ///  --> U
                        /// |  .............
                        /// v  .           .
                        /// V  x___        .
                        ///    |   |       .
                        ///    |   |       .
                        ///    |___x       .
                        ///    .           .
                        ///    .............
                        ///
                        _filterArea[neighbourFaceIdx].add(0.0f, bbMin);
                        _filterArea[neighbourFaceIdx].add(bleedAmount, bbMax);
                    }
                break;

                case CMFT_EDGE_RIGHT:
                    {
                        ///  --> U
                        /// |  .............
                        /// v  .           .
                        /// V  .       x___.
                        ///    .       |   |
                        ///    .       |   |
                        ///    .       |___x
                        ///    .           .
                        ///    .............
                        ///
                        _filterArea[neighbourFaceIdx].add(1.0f - bleedAmount, bbMin);
                        _filterArea[neighbourFaceIdx].add(1.0f, bbMax);
                    }
                break;

                case CMFT_EDGE_TOP:
                    {
                        ///  --> U
                        /// |  ...x____ ...
                        /// v  .  |    |  .
                        /// V  .  |____x  .
                        ///    .          .
                        ///    .          .
                        ///    .          .
                        ///    ............
                        ///
                        _filterArea[neighbourFaceIdx].add(bbMin, 0.0f);
                        _filterArea[neighbourFaceIdx].add(bbMax, bleedAmount);
                    }
                break;

                case CMFT_EDGE_BOTTOM:
                    {
                        ///  --> U
                        /// |  ............
                        /// v  .          .
                        /// V  .          .
                        ///    .          .
                        ///    .  x____   .
                        ///    .  |    |  .
                        ///    ...|____x...
                        ///
                        _filterArea[neighbourFaceIdx].add(bbMin, 1.0f - bleedAmount);
                        _filterArea[neighbourFaceIdx].add(bbMax, 1.0f);
                    }
                break;
                }

                // Clamp bounding box to face size.
                _filterArea[neighbourFaceIdx].clamp(0.0f, 1.0f);
            }
        }
    }

    template <typename floatOrDouble>
    void processFilterArea(floatOrDouble _res[3]
                         , float _specularPower
                         , float _specularAngle
                         , const float* _tapVec
                         , const float* _cubemapNormalSolidAngle
                         , Aabb _filterArea[6]
                         , uint32_t _srcFaceSize
                         , const void* _srcData
                         , const uint32_t _faceOffsets[6]
                         )
    {
        floatOrDouble colorWeight[4] = { floatOrDouble(0.0), floatOrDouble(0.0), floatOrDouble(0.0), floatOrDouble(0.0) };

        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t pitch = _srcFaceSize*bytesPerPixel;
        const float faceSize_MinusOne = float(int32_t(_srcFaceSize-1));

        for (uint8_t face = 0; face < 6; ++face)
        {
            if (_filterArea[face].isEmpty())
            {
                continue;
            }

            const uint32_t minX = uint32_t(_filterArea[face].m_min[0] * faceSize_MinusOne);
            const uint32_t maxX = uint32_t(_filterArea[face].m_max[0] * faceSize_MinusOne);
            const uint32_t minY = uint32_t(_filterArea[face].m_min[1] * faceSize_MinusOne);
            const uint32_t maxY = uint32_t(_filterArea[face].m_max[1] * faceSize_MinusOne);

            const uint8_t* faceData    = (const uint8_t*)_srcData                 + _faceOffsets[face];
            const uint8_t* faceNormals = (const uint8_t*)_cubemapNormalSolidAngle + _faceOffsets[face];

            for (uint32_t yy = minY; yy <= maxY; ++yy)
            {
                const uint8_t* rowData    = (const uint8_t*)faceData    + yy*pitch;
                const uint8_t* rowNormals = (const uint8_t*)faceNormals + yy*pitch;

                for (uint32_t xx = minX; xx <= maxX; ++xx)
                {
                    const float* normalPtr = (const float*)((const uint8_t*)rowNormals + xx*bytesPerPixel);
                    const float dotProduct = vec3Dot(normalPtr, _tapVec);

                    if (dotProduct >= _specularAngle)
                    {
                        const float solidAngle = normalPtr[3];
                        const floatOrDouble weight = floatOrDouble(solidAngle * powf(dotProduct, _specularPower));

                        const float* dataPtr = (const float*)((const uint8_t*)rowData + xx*bytesPerPixel);
                        colorWeight[0] += floatOrDouble(dataPtr[0]) * weight;
                        colorWeight[1] += floatOrDouble(dataPtr[1]) * weight;
                        colorWeight[2] += floatOrDouble(dataPtr[2]) * weight;
                        colorWeight[3] += floatOrDouble(1.0)        * weight;
                    }
                }
            }
        }

        // Divide color by colorWeight and store result.
        if (0.0f != colorWeight[3])
        {
            const floatOrDouble invWeight = floatOrDouble(1.0)/colorWeight[3];
            _res[0] = colorWeight[0] * invWeight;
            _res[1] = colorWeight[1] * invWeight;
            _res[2] = colorWeight[2] * invWeight;
        }
        // Else if colorWeight == 0 (result of convolution is zero) take a direct color sample.
        else
        {
            float uu, vv;
            uint8_t hitFaceIdx;
            vecToTexelCoord(uu, vv, hitFaceIdx, _tapVec);

            const uint32_t xx = uint32_t(uu*float(_srcFaceSize));
            const uint32_t yy = uint32_t(vv*float(_srcFaceSize));

            const float* dataPtr = (const float*)((const uint8_t*)_srcData
                                 + _faceOffsets[hitFaceIdx]
                                 + yy*pitch
                                 + xx*bytesPerPixel
                                 );

            _res[0] = floatOrDouble(dataPtr[0]);
            _res[1] = floatOrDouble(dataPtr[1]);
            _res[2] = floatOrDouble(dataPtr[2]);
        }
    }

    void radianceFilter(float *_dstPtr
                      , uint8_t _face
                      , uint32_t _mipFaceSize
                      , float _filterSize
                      , float _specularPower
                      , float _specularAngle
                      , const float* _cubemapVectors
                      , const Image* _imageRgba32f
                      , const uint32_t _faceOffsets[CUBE_FACE_NUM]
                      )
    {
        const float invFaceSize = 1.0f/float(int32_t(_mipFaceSize));

        for (uint32_t yy = 0; yy < _mipFaceSize; ++yy)
        {
            for (uint32_t xx = 0; xx < _mipFaceSize; ++xx)
            {
                // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                const float xxf = float(int32_t(xx));
                const float yyf = float(int32_t(yy));
                const float uu = 2.0f*(xxf+0.5f)*invFaceSize - 1.0f;
                const float vv = 2.0f*(yyf+0.5f)*invFaceSize - 1.0f;

                float tapVec[3];
                texelCoordToVec(tapVec, uu, vv, _face, _mipFaceSize);

                Aabb facesBb[6];
                determineFilterArea(facesBb, tapVec, _filterSize);

                float color[3];
                processFilterArea<float>(color
                                       , _specularPower
                                       , _specularAngle
                                       , tapVec
                                       , _cubemapVectors
                                       , facesBb
                                       , _imageRgba32f->m_width
                                       , _imageRgba32f->m_data
                                       , _faceOffsets
                                       );

                _dstPtr[0] = float(color[0]);
                _dstPtr[1] = float(color[1]);
                _dstPtr[2] = float(color[2]);
                _dstPtr[3] = 1.0f;

                _dstPtr += 4;
            }
        }
    }

    struct RadianceFilterGlobalState
    {
        RadianceFilterGlobalState()
            : m_startTime(0)
            , m_completedTasksGpu(0)
            , m_completedTasksCpu(0)
            , m_threadId(0)
        {
        }

        uint8_t getThreadId()
        {
            bx::MutexScope lock(m_threadIdMutex);
            return m_threadId++;
        }

        void incrCompletedTasksGpu()
        {
            bx::MutexScope lock(m_completedTasksCpuMutex);
            m_completedTasksCpu++;
        }

        void incrCompletedTasksCpu()
        {
            bx::MutexScope lock(m_completedTasksGpuMutex);
            m_completedTasksGpu++;
        }

        uint64_t m_startTime;
        uint16_t m_completedTasksGpu;
        uint16_t m_completedTasksCpu;
        uint8_t m_threadId;
        bx::Mutex m_threadIdMutex;
        bx::Mutex m_completedTasksGpuMutex;
        bx::Mutex m_completedTasksCpuMutex;
    };
    static RadianceFilterGlobalState s_globalState;

    struct RadianceFilterParams
    {
        float* m_dstPtr;
        uint8_t m_face;
        uint32_t m_mipFaceSize;
        float m_filterSize;
        float m_specularPower;
        float m_specularAngle;
        const float* m_cubemapVectors;
        const Image* m_imageRgba32f;
        const uint32_t* m_faceOffsets;
    };

    struct RadianceFilterTaskList
    {
        RadianceFilterTaskList(uint8_t _mipStart, uint8_t _mipCount)
            : m_topMipIndex(_mipStart)
            , m_bottomMipIndex(_mipCount-1)
            , m_totalMipCount(_mipCount)
        {
            memset(m_mipFaceIdx, 0, MAX_MIP_NUM);
        }

        // Returns cube face radiance filter parameters starting from the top mip level.
        const RadianceFilterParams* getFromTop()
        {
            bx::MutexScope lock(m_indexMutex);
            while (m_topMipIndex <= m_bottomMipIndex)
            {
                if (m_mipFaceIdx[m_topMipIndex] >= 6)
                {
                    m_topMipIndex++;
                }
                else
                {
                    return &m_params[m_topMipIndex][m_mipFaceIdx[m_topMipIndex]++];
                }

            }
            return NULL;
        }

        // Returns cube face radiance filter parameters starting from the bottom mip level.
        const RadianceFilterParams* getFromBottom(uint8_t _numLevels = 0)
        {
            bx::MutexScope lock(m_indexMutex);
            const uint8_t barrier = (0 == _numLevels) ? m_topMipIndex : max(m_topMipIndex, uint8_t(m_totalMipCount-_numLevels));
            while (barrier <= m_bottomMipIndex)
            {
                if (m_mipFaceIdx[m_bottomMipIndex] >= 6)
                {
                    m_bottomMipIndex--;
                }
                else
                {
                    return &m_params[m_bottomMipIndex][m_mipFaceIdx[m_bottomMipIndex]++];
                }
            }
            return NULL;
        }

        bx::Mutex m_indexMutex;
        uint8_t m_topMipIndex;
        uint8_t m_bottomMipIndex;
        uint8_t m_totalMipCount;
        uint8_t m_mipFaceIdx[MAX_MIP_NUM];
        RadianceFilterParams m_params[MAX_MIP_NUM][CUBE_FACE_NUM];
    };

    int32_t radianceFilterCpu(void* _taskList)
    {
        const uint8_t threadId = s_globalState.getThreadId();
        const double freq = double(bx::getHPFrequency());
        const double toSec = 1.0/freq;

        RadianceFilterTaskList* taskList = (RadianceFilterTaskList*)_taskList;

        // Gpu is processing from the top level mip map to the bottom.
        const RadianceFilterParams* params;
        while ((params = taskList->getFromTop()) != NULL)
        {
            // Start timer.
            const uint64_t startTime = bx::getHPCounter();

            // Process data.
            radianceFilter(params->m_dstPtr
                         , params->m_face
                         , params->m_mipFaceSize
                         , params->m_filterSize
                         , params->m_specularPower
                         , params->m_specularAngle
                         , params->m_cubemapVectors
                         , params->m_imageRgba32f
                         , params->m_faceOffsets
                         );

            // Determine task duration.
            const uint64_t currentTime = bx::getHPCounter();
            const uint64_t taskDuration = currentTime - startTime;
            const uint64_t totalDuration = currentTime - s_globalState.m_startTime;

            // Output process info.
            char cpuId[16];
            sprintf(cpuId, "[CPU%u]", threadId);
            INFO("Radiance -> %-8s| %4u | %7.3fs | %7.3fs"
                , cpuId
                , params->m_mipFaceSize
                , double(taskDuration)*toSec
                , double(totalDuration)*toSec
                );

            // Update task counter.
            s_globalState.incrCompletedTasksGpu();
        }

        return EXIT_SUCCESS;
    }

    struct RadianceProgram
    {
        RadianceProgram()
            : m_clContext(NULL)
            , m_program(NULL)
            , m_kernel(NULL)
            , m_event(NULL)
            , m_memOut(NULL)
        {
            m_memSrcData[0] = NULL;
            m_memSrcData[1] = NULL;
            m_memSrcData[2] = NULL;
            m_memSrcData[3] = NULL;
            m_memSrcData[4] = NULL;
            m_memSrcData[5] = NULL;
            m_memNormalSolidAngle[0] = NULL;
            m_memNormalSolidAngle[1] = NULL;
            m_memNormalSolidAngle[2] = NULL;
            m_memNormalSolidAngle[3] = NULL;
            m_memNormalSolidAngle[4] = NULL;
            m_memNormalSolidAngle[5] = NULL;
        }

        void setClContext(const ClContext* _clContext)
        {
            m_clContext = _clContext;
        }

        bool hasValidDeviceContext() const
        {
            return (NULL != m_clContext->m_context);
        }

        bool isValid() const
        {
            return (NULL != m_program);
        }

        bool createFromStr(const char* _sourceCode, const char* _kernelName)
        {
            cl_int err;

            // Create program.
            m_program = clCreateProgramWithSource(m_clContext->m_context, 1, (const char**)&_sourceCode, NULL, &err);
            if (CL_SUCCESS != err)
            {
                WARN("Could not create OpenCL program. OpenCL source file probably missing!");
                return false;
            }

            // Build program.
            err = clBuildProgram(m_program, 1, &m_clContext->m_device, NULL, NULL, NULL);
            if (CL_SUCCESS != err)
            {
                // Print error.
                char buffer[10240];
                clGetProgramBuildInfo(m_program, m_clContext->m_device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
                WARN("CL Compilation failed:\n%s", buffer);

                return false;
            }

            // Create OpenCL Kernel.
            m_kernel = clCreateKernel(m_program, _kernelName, &err);
            if (CL_SUCCESS != err)
            {
                WARN("Could not create OpenCL kernel. Kernel name probably inavlid! Should be: %s", _kernelName);
                return false;
            }

            return true;
        }

        bool createFromFile(const char* _filePath, const char* _kernelName)
        {
            CMFT_UNUSED size_t read;

            // Open file.
            FILE* fp = fopen(_filePath, "rb");
            if (NULL == fp)
            {
                WARN("Could not open file %s for reading.", _filePath);
                return false;
            }
            ScopeFclose cleanup0(fp);

            // Alloc data for string.
            const long int fileSize = fsize(fp);
            char* sourceData = (char*)malloc(fileSize+1);
            ScopeFree cleanup1(sourceData);

            // Read opencl source file.
            read = fread(sourceData, fileSize, 1, fp);
            sourceData[fileSize] = '\0';
            DEBUG_CHECK(read == 1, "Could not read from file.");
            FERROR_CHECK(fp);

            bool result = createFromStr(sourceData, _kernelName);

            return result;
        }

        void initDeviceMemory(const Image& _image, float* _cubemapNormalSolidAngle)
        {
            cl_int err;

            uint32_t faceOffsets[CUBE_FACE_NUM];
            imageGetFaceOffsets(faceOffsets, _image);
            const cl_image_format imageFormat = { CL_RGBA, CL_FLOAT };
            const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;

            for (uint8_t face = 0; face < 6; ++face)
            {
                m_memSrcData[face] = CL_CHECK_ERR(clCreateImage2D(m_clContext->m_context
                                                , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                                , &imageFormat
                                                , _image.m_width
                                                , _image.m_height
                                                , _image.m_width*bytesPerPixel
                                                , (void*)((uint8_t*)_image.m_data + faceOffsets[face])
                                                , &err
                                                ));

                m_memNormalSolidAngle[face] = CL_CHECK_ERR(clCreateImage2D(m_clContext->m_context
                                                         , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                                         , &imageFormat
                                                         , _image.m_width
                                                         , _image.m_height
                                                         , _image.m_width*bytesPerPixel
                                                         , (void*)((uint8_t*)_cubemapNormalSolidAngle + faceOffsets[face])
                                                         , &err
                                                         ));
            }

            CL_CHECK(clSetKernelArg(m_kernel,   5, sizeof(int32_t), (const void*)&_image.m_width));
            CL_CHECK(clSetKernelArg(m_kernel,   6, sizeof(cl_mem),  (const void*)&m_memSrcData[0]));
            CL_CHECK(clSetKernelArg(m_kernel,   7, sizeof(cl_mem),  (const void*)&m_memSrcData[1]));
            CL_CHECK(clSetKernelArg(m_kernel,   8, sizeof(cl_mem),  (const void*)&m_memSrcData[2]));
            CL_CHECK(clSetKernelArg(m_kernel,   9, sizeof(cl_mem),  (const void*)&m_memSrcData[3]));
            CL_CHECK(clSetKernelArg(m_kernel,  10, sizeof(cl_mem),  (const void*)&m_memSrcData[4]));
            CL_CHECK(clSetKernelArg(m_kernel,  11, sizeof(cl_mem),  (const void*)&m_memSrcData[5]));
            CL_CHECK(clSetKernelArg(m_kernel,  12, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[0]));
            CL_CHECK(clSetKernelArg(m_kernel,  13, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[1]));
            CL_CHECK(clSetKernelArg(m_kernel,  14, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[2]));
            CL_CHECK(clSetKernelArg(m_kernel,  15, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[3]));
            CL_CHECK(clSetKernelArg(m_kernel,  16, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[4]));
            CL_CHECK(clSetKernelArg(m_kernel,  17, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[5]));
        }

        void setupOutputBuffer(uint32_t _dstFaceSize)
        {
            cl_int err;

            if (NULL != m_memOut)
            {
                clReleaseMemObject(m_memOut);
                m_memOut = NULL;
            }

            const cl_image_format imageFormat = { CL_RGBA, CL_FLOAT };
            m_memOut = CL_CHECK_ERR(clCreateImage2D(m_clContext->m_context
                        , CL_MEM_WRITE_ONLY
                        , &imageFormat
                        , _dstFaceSize
                        , _dstFaceSize
                        , 0
                        , NULL
                        , &err
                        ));

            CL_CHECK(clSetKernelArg(m_kernel, 0, sizeof(cl_mem), (const void*)&m_memOut));
        }

        void setArgs(uint8_t _faceId, uint32_t _dstFaceSize, float _specularPower, float _specularAngle) const
        {
            CL_CHECK(clSetKernelArg(m_kernel, 1, sizeof(float),   (const void*)&_specularPower));
            CL_CHECK(clSetKernelArg(m_kernel, 2, sizeof(float),   (const void*)&_specularAngle));
            CL_CHECK(clSetKernelArg(m_kernel, 3, sizeof(int32_t), (const void*)&_dstFaceSize));
            CL_CHECK(clSetKernelArg(m_kernel, 4, sizeof(uint8_t), (const void*)&_faceId));
        }

        bool isIdle() const
        {
            cl_int status;
            clGetEventInfo(m_event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), (void*)&status, NULL);

            return ((CL_COMPLETE == status) || (NULL == m_event));
        }

        void run(uint32_t _dstFaceSize)
        {
            size_t workSize[2] = { _dstFaceSize, _dstFaceSize };
            CL_CHECK(clEnqueueNDRangeKernel(m_clContext->m_commandQueue, m_kernel, 2, NULL, workSize, NULL, 0, NULL, &m_event));
        }

        void readResults(void* _out, uint32_t _dstFaceSize)
        {
            const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
            const size_t origin[3] = { 0, 0, 0 };
            const size_t region[3] = { _dstFaceSize, _dstFaceSize, 1 };
            CL_CHECK(clEnqueueReadImage(m_clContext->m_commandQueue
                                      , m_memOut
                                      , CL_TRUE
                                      , origin
                                      , region
                                      , _dstFaceSize*bytesPerPixel
                                      , 0
                                      , _out
                                      , 0
                                      , NULL
                                      , &m_event
                                      ));
        }

        void finish() const
        {
            clFinish(m_clContext->m_commandQueue);
        }

        void releaseDeviceMemory()
        {
#define RELEASE_CL_MEM(_mem) do { if (NULL != (_mem)) { clReleaseMemObject(_mem); (_mem) = NULL; } } while (0)
            RELEASE_CL_MEM(m_memOut);
            RELEASE_CL_MEM(m_memSrcData[0]);
            RELEASE_CL_MEM(m_memSrcData[1]);
            RELEASE_CL_MEM(m_memSrcData[2]);
            RELEASE_CL_MEM(m_memSrcData[3]);
            RELEASE_CL_MEM(m_memSrcData[4]);
            RELEASE_CL_MEM(m_memSrcData[5]);
            RELEASE_CL_MEM(m_memNormalSolidAngle[0]);
            RELEASE_CL_MEM(m_memNormalSolidAngle[1]);
            RELEASE_CL_MEM(m_memNormalSolidAngle[2]);
            RELEASE_CL_MEM(m_memNormalSolidAngle[3]);
            RELEASE_CL_MEM(m_memNormalSolidAngle[4]);
            RELEASE_CL_MEM(m_memNormalSolidAngle[5]);
#undef RELEASE_CL_MEM
        }

        void destroy()
        {
            if (NULL != m_program)
            {
                clReleaseProgram(m_program);
                m_program = NULL;
            }

            if (NULL != m_kernel)
            {
                clReleaseKernel(m_kernel);
                m_kernel = NULL;
            }
        }

        const ClContext* m_clContext;
        cl_program m_program;
        cl_kernel m_kernel;
        cl_event m_event;
        cl_mem m_memOut;
        cl_mem m_memSrcData[6];
        cl_mem m_memNormalSolidAngle[6];
    };
    RadianceProgram s_radianceProgram;

    int32_t radianceFilterGpu(void* _taskList)
    {
        if (!s_radianceProgram.isValid())
        {
            return EXIT_FAILURE;
        }

        const double freq = double(bx::getHPFrequency());
        const double toSec = 1.0/freq;

        RadianceFilterTaskList* taskList = (RadianceFilterTaskList*)_taskList;

        // Gpu is processing from the top level mip map to the bottom.
        const RadianceFilterParams* params;
        while ((params = taskList->getFromTop()) != NULL)
        {
            // Start timer.
            const uint64_t startTime = bx::getHPCounter();

            // Prepare parameters.
            s_radianceProgram.setupOutputBuffer(params->m_mipFaceSize);
            s_radianceProgram.setArgs(params->m_face
                                    , params->m_mipFaceSize
                                    , params->m_specularPower
                                    , params->m_specularAngle
                                    );

            // Enqueue processing job.
            s_radianceProgram.run(params->m_mipFaceSize);

            // Read results.
            s_radianceProgram.readResults(params->m_dstPtr, params->m_mipFaceSize);

            // Determine task duration.
            const uint64_t currentTime = bx::getHPCounter();
            const uint64_t taskDuration = currentTime - startTime;
            const uint64_t totalDuration = currentTime - s_globalState.m_startTime;

            // Output process info.
            INFO("Radiance ->  <GPU>  | %4u | %7.3fs | %7.3fs"
                , params->m_mipFaceSize
                , double(taskDuration)*toSec
                , double(totalDuration)*toSec
                );

            // Update task counter.
            s_globalState.incrCompletedTasksCpu();
        }

        return EXIT_SUCCESS;
    }

    static const char* s_lightingModelStr[LightingModel::Count] =
    {
        "phong",
        "phongbrdf",
        "blinn",
        "blinnbrdf",
    };

    const char* getLightingModelStr(LightingModel::Enum _lightingModel)
    {
        DEBUG_CHECK(_lightingModel < LightingModel::Count, "Reading array out of bounds!");
        return s_lightingModelStr[uint8_t(_lightingModel)];
    }

    /// Returns the angle of cosine power function where the results are above a small empirical treshold.
    static float cosinePowerFilterAngle(float _cosinePower)
    {
        // Bigger value leads to performance improvement but might hurt the results.
        // 0.00001f was tested empirically and it gives almost the same values as reference.
        const float treshold = 0.00001f;

        // Cosine power filter is: pow(cos(angle), power).
        // We want the value of the angle above each result is <= treshold.
        // So: angle = acos(pow(treshold, 1.0 / power))
        return acosf(powf(treshold, 1.0f / _cosinePower));
    }

    float applyLightningModel(float _specularPowerRef, LightingModel::Enum _lightingModel)
    {
        /// http://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/
        /// http://seblagarde.wordpress.com/2012/03/29/relationship-between-phong-and-blinn-lighting-model/
        switch (_lightingModel)
        {
        case LightingModel::Phong:
            {
                return _specularPowerRef;
            }
        break;

        case LightingModel::PhongBrdf:
            {
                return _specularPowerRef + 1.0f;
            }
        break;

        case LightingModel::Blinn:
            {
                return _specularPowerRef/4.0f;
            }
        break;

        case LightingModel::BlinnBrdf:
            {
                return _specularPowerRef/4.0f + 1.0f;
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "ERROR! This should never happen!");
                WARN("Lighting model error. Please check for invalid parameters!");
                return _specularPowerRef;
            }
        break;
        };
    }

    bool imageRadianceFilter(Image& _dst
                           , uint32_t _dstFaceSize
                           , LightingModel::Enum _lightingModel
                           , bool _excludeBase
                           , uint8_t _mipCount
                           , uint8_t _glossScale
                           , uint8_t _glossBias
                           , const Image& _src
                           , int8_t _numCpuProcessingThreads
                           , const ClContext* _clContext
                           )
    {
        // Input image must be a cubemap.
        if (!imageIsCubemap(_src))
        {
            WARN("Image is not cubemap.");

            return false;
        }

        // Multi-threading parameters.
        bx::Thread cpuThreads[64];
        uint8_t activeCpuThreads = 0;
        const uint8_t maxActiveCpuThreads = (uint8_t)max(int8_t(0), min(_numCpuProcessingThreads, int8_t(64)));

        // Prepare OpenCL kernel and device memory.
        s_radianceProgram.setClContext(_clContext);
        if (s_radianceProgram.hasValidDeviceContext())
        {
            s_radianceProgram.createFromStr(s_radianceProgramSource, "radianceFilter");
        }

        // Check at least some processig device is valid and choosen for filtering.
        if (0 == maxActiveCpuThreads && !s_radianceProgram.isValid())
        {
            WARN("No hardware devices selected for processing."
                " OpenCL context is invalid and 0 CPU processing theads are choosen for filtering."
                );

            return false;
        }

        // Don't use the same CPU device for OpenCL and CPU processing!
        if (_clContext->m_deviceType&CL_DEVICE_TYPE_CPU
        &&  maxActiveCpuThreads != 0)
        {
            WARN(" !! Choosing CPU device as OpenCL device and running CPU processing"
                 " threads on the SAME device is NOT a good idea. It will work, but it is"
                 " certainly not the best utilization of processing resources."
                 " In case OpenCL device type is CPU but it is a different processing"
                 " device other than the host device, simply ignore this warning. !!"
                 );
        }

        // Processing is done in Rgba32f format.
        Image imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src);

        // Alloc dst data.
        const uint32_t dstFaceSize = (0 == _dstFaceSize) ? _src.m_width : _dstFaceSize;
        const uint8_t mipMin = 1;
        const uint8_t mipMax = (uint8_t)log2f(float(int32_t(dstFaceSize)))+uint8_t(1);
        const uint8_t mipCount = clamp(_mipCount, mipMin, mipMax);
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        uint32_t dstDataSize = 0;
        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < mipCount; ++mip)
            {
                dstOffsets[face][mip] = dstDataSize;
                uint32_t faceSize = max(uint32_t(1), dstFaceSize >> mip);
                dstDataSize += faceSize * faceSize * bytesPerPixel;
            }
        }
        void* dstData = malloc(dstDataSize);
        MALLOC_CHECK(dstData);

        // Get source image offsets.
        uint32_t srcFaceOffsets[CUBE_FACE_NUM];
        imageGetFaceOffsets(srcFaceOffsets, imageRgba32f);

        // Output info.
        INFO("Running radiance filter for:"
             "\n\t[srcFaceSize=%u]"
             "\n\t[lightingModel=%s]"
             "\n\t[excludeBase=%s]"
             "\n\t[mipCount=%u]"
             "\n\t[glossScale=%u]"
             "\n\t[glossBias=%u]"
             "\n\t[dstFaceSize=%u]"
             , imageRgba32f.m_width
             , getLightingModelStr(_lightingModel)
             , "false\0true"+6*_excludeBase
             , mipCount
             , _glossScale
             , _glossBias
             , dstFaceSize
             );

        // Resize and copy base image.
        if (_excludeBase)
        {
            INFO("Radiance -> Excluding base image.");

            const float dstToSrcRatio = float(int32_t(imageRgba32f.m_width))/float(int32_t(dstFaceSize));
            const uint32_t dstFacePitch = dstFaceSize * bytesPerPixel;
            const uint32_t srcFacePitch = imageRgba32f.m_width * bytesPerPixel;

            // For all top level cubemap faces:
            for(uint8_t face = 0; face < 6; ++face)
            {
                const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcFaceOffsets[face];
                uint8_t* dstFaceData = (uint8_t*)dstData + dstOffsets[face][0];

                // Iterate through destination pixels.
                for (uint32_t yDst = 0; yDst < dstFaceSize; ++yDst)
                {
                    uint8_t* dstFaceRow = (uint8_t*)dstFaceData + yDst*dstFacePitch;
                    for (uint32_t xDst = 0; xDst < dstFaceSize; ++xDst)
                    {
                        float* dstFaceColumn = (float*)((uint8_t*)dstFaceRow + xDst*bytesPerPixel);

                        // For each destination pixel, sample and accumulate color from source.
                        float color[3] = { 0.0f, 0.0f, 0.0f };
                        uint32_t weightAccum = 0;

                        for (uint32_t ySrc = uint32_t(float(yDst)*dstToSrcRatio)
                            , end = ySrc+max(uint32_t(1), uint32_t(dstToSrcRatio))
                            ; ySrc < end
                            ; ++ySrc)
                        {
                            const uint8_t* srcRowData = (const uint8_t*)srcFaceData + ySrc*srcFacePitch;

                            for (uint32_t xSrc = uint32_t(float(xDst)*dstToSrcRatio)
                                , end = xSrc+max(uint32_t(1), uint32_t(dstToSrcRatio))
                                ; xSrc < end
                                ; ++xSrc)
                            {
                                const float* srcColumnData = (const float*)((const uint8_t*)srcRowData + xSrc*bytesPerPixel);
                                color[0] += srcColumnData[0];
                                color[1] += srcColumnData[1];
                                color[2] += srcColumnData[2];
                                weightAccum++;
                            }
                        }

                        // Divide by weight and save to destination pixel.
                        const float invWeight = 1.0f/float(int32_t(weightAccum));
                        dstFaceColumn[0] = color[0] * invWeight;
                        dstFaceColumn[1] = color[1] * invWeight;
                        dstFaceColumn[2] = color[2] * invWeight;
                        dstFaceColumn[3] = 1.0f;
                    }
                }
            }
        }

        if (mipCount - uint8_t(_excludeBase) <= 0)
        {
            INFO("Radiance -> Nothing left for processing... Increase mip count or do not exclude base image.");
        }
        else
        {
            // Build cubemap vectors.
            float* cubemapVectors = buildCubemapNormalSolidAngle(imageRgba32f.m_width);
            ScopeFree cleanup(cubemapVectors);

            // Enqueue memory transfer for cl device.
            if (s_radianceProgram.isValid())
            {
                s_radianceProgram.initDeviceMemory(imageRgba32f, cubemapVectors);
            }

            // Start global timer.
            s_globalState.m_startTime = bx::getHPCounter();
            INFO("Radiance -> Starting filter...");

            INFO("Radiance -> Utilizing %u CPU processing thread%s%s%s."
                 , maxActiveCpuThreads
                 , maxActiveCpuThreads==1?"":"s"
                 , !s_radianceProgram.isValid()?"":" and "
                 , !s_radianceProgram.isValid()?"":s_radianceProgram.m_clContext->m_deviceName
                 );

            // Alloc data for tasks parameters.
            const uint8_t mipStart = uint8_t(_excludeBase);
            RadianceFilterTaskList taskList(mipStart, mipCount);

            const float glossScalef = float(int32_t(_glossScale));
            const float glossBiasf = float(int32_t(_glossBias));

            //Prepare processing tasks parameters.
            for (uint32_t mip = mipStart; mip < mipCount; ++mip)
            {
                // Determine filter parameters.
                const uint32_t mipFaceSize = max(UINT32_C(1), dstFaceSize >> mip);
                const float mipFaceSizef = float(int32_t(mipFaceSize));
                const float minAngle = atan2f(1.0f, mipFaceSizef);
                const float maxAngle = float(M_PI)/2.0f;
                const float toFilterSize = 1.0f/(minAngle*mipFaceSizef*2.0f);
                const float glossiness = (mipCount == 1)
                                       ? 1.0f
                                       : max(0.0f, 1.0f - (float)(int32_t)mip/(float)(int32_t)(mipCount-1))
                                       ;
                const float specularPowerRef = powf(2.0f, glossScalef * glossiness + glossBiasf);
                const float specularPower = applyLightningModel(specularPowerRef, _lightingModel);
                const float filterAngle = clamp(cosinePowerFilterAngle(specularPower), minAngle, maxAngle);
                const float cosAngle = max(0.0f, cosf(filterAngle));
                const float texelSize = 1.0f/mipFaceSizef;
                const float filterSize = max(texelSize, filterAngle * toFilterSize);

                for (uint8_t face = 0; face < 6; ++face)
                {
                    float* dstPtr = (float*)((uint8_t*)dstData + dstOffsets[face][mip]);

                    RadianceFilterParams taskParams =
                    {
                        dstPtr,
                        face,
                        mipFaceSize,
                        filterSize,
                        specularPower,
                        cosAngle,
                        cubemapVectors,
                        &imageRgba32f,
                        srcFaceOffsets,
                    };

                    // Enqueue processing parameters.
                    memcpy(&taskList.m_params[mip][face], &taskParams, sizeof(RadianceFilterParams));
                }
            }

            // Output process header info.
            INFO("Radiance -> ------------------------------------");
            INFO("Radiance ->  Device / Face /     Time /    Total");
            INFO("Radiance -> ------------------------------------");

            // Single thread, no OpenCL.
            if (maxActiveCpuThreads == 1 && !s_radianceProgram.isValid())
            {
                radianceFilterCpu((void*)&taskList);
            }
            // Multi thread (with or without OpenCL).
            else
            {
                // Start CPU processing threads.
                while (activeCpuThreads < maxActiveCpuThreads)
                {
                    cpuThreads[activeCpuThreads++].init(radianceFilterCpu, (void*)&taskList);
                }

                // Start one GPU host thread.
                if (s_radianceProgram.isValid() && s_radianceProgram.isIdle())
                {
                    cpuThreads[activeCpuThreads++].init(radianceFilterGpu, (void*)&taskList);
                }

                // Wait for everything to finish.
                for (uint8_t ii = 0; ii < activeCpuThreads; ++ii)
                {
                    cpuThreads[ii].shutdown();
                }
            }

            // Average 1x1 face size.
            if ((dstFaceSize>>(mipCount-1)) <= 1)
            {
                float* face0 = (float*)((uint8_t*)dstData + dstOffsets[0][mipCount-1]);
                float* face1 = (float*)((uint8_t*)dstData + dstOffsets[1][mipCount-1]);
                float* face2 = (float*)((uint8_t*)dstData + dstOffsets[2][mipCount-1]);
                float* face3 = (float*)((uint8_t*)dstData + dstOffsets[3][mipCount-1]);
                float* face4 = (float*)((uint8_t*)dstData + dstOffsets[4][mipCount-1]);
                float* face5 = (float*)((uint8_t*)dstData + dstOffsets[5][mipCount-1]);

                const float color[3] =
                {
                    (face0[0] + face1[0] + face2[0] + face3[0] + face4[0] + face5[0]) / 6.0f,
                    (face0[1] + face1[1] + face2[1] + face3[1] + face4[1] + face5[1]) / 6.0f,
                    (face0[2] + face1[2] + face2[2] + face3[2] + face4[2] + face5[2]) / 6.0f,
                };

                face0[0] = face1[0] = face2[0] = face3[0] = face4[0] = face5[0] = color[0];
                face0[1] = face1[1] = face2[1] = face3[1] = face4[1] = face5[1] = color[1];
                face0[2] = face1[2] = face2[2] = face3[2] = face4[2] = face5[2] = color[2];
            }

            // Get filter duration.
            const double freq = double(bx::getHPFrequency());
            const double toSec = 1.0/freq;
            const uint64_t totalTime = bx::getHPCounter() - s_globalState.m_startTime;

            // Output progress info.
            INFO("Radiance -> ------------------------------------");
            INFO("Radiance -> Total faces processed on [CPU]: %u", s_globalState.m_completedTasksCpu);
            INFO("Radiance -> Total faces processed on <GPU>: %u", s_globalState.m_completedTasksGpu);
            INFO("Radiance -> Total time: %.3f seconds.", double(totalTime)*toSec);

            // Cleanup.
            if (s_radianceProgram.isValid())
            {
                s_radianceProgram.releaseDeviceMemory();
                s_radianceProgram.destroy();
            }
        }

        // Fill result structure.
        Image result;
        result.m_width = dstFaceSize;
        result.m_height = dstFaceSize;
        result.m_dataSize = dstDataSize;
        result.m_format = TextureFormat::RGBA32F;
        result.m_numMips = mipCount;
        result.m_numFaces = 6;
        result.m_data = dstData;

        // Convert back to source format.
        if (TextureFormat::RGBA32F == _src.m_format)
        {
            imageMove(_dst, result);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result);
            imageUnload(result);
        }

        return true;
    }

    void imageRadianceFilter(Image& _image
                           , uint32_t _dstFaceSize
                           , LightingModel::Enum _lightingModel
                           , bool _excludeBase
                           , uint8_t _mipCount
                           , uint8_t _glossScale
                           , uint8_t _glossBias
                           , int8_t _numCpuProcessingThreads
                           , const ClContext* _clContext
                           )
    {
        Image tmp;
        if(imageRadianceFilter(tmp, _dstFaceSize, _lightingModel, _excludeBase, _mipCount, _glossScale, _glossBias, _image, _numCpuProcessingThreads, _clContext))
        {
            imageMove(_image, tmp);
        }
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
