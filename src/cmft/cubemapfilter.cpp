/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "common/config.h"
#include "common/utils.h"
#include "common/timer.h"

#include <cmft/cubemapfilter.h>
#include <cmft/clcontext.h>
#include <cmft/allocator.h>

#include "clcontext_internal.h"

#include "cubemaputils.h"
#include "radiance.h"

#include <string.h>       //memset
#include <math.h>         //pow, sqrt
#include <float.h>        //FLT_MAX

#include <thread> // C++11
#include <mutex>  // C++11

#define CMFT_COMPUTE_FILTER_AREA_ON_CPU 1

#ifndef CMFT_COMPUTE_FILTER_AREA_ON_CPU
    #define CMFT_COMPUTE_FILTER_AREA_ON_CPU 0
#endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

namespace cmft
{
    #define PI      3.1415926535897932384626433832795028841971693993751058
    #define PI4     12.566370614359172953850573533118011536788677597500423
    #define PI16    50.265482457436691815402294132472046147154710390001693
    #define PI64    201.06192982974676726160917652988818458861884156000677
    #define SQRT_PI 1.7724538509055160272981674833411451827975494561223871

    static inline size_t cubemapNormalSolidAngleSize(uint32_t _cubemapFaceSize)
    {
        return (_cubemapFaceSize /*width*/
              * _cubemapFaceSize /*height*/
              * 6 /*numFaces*/
              * 4 /*numChannels*/
              * 4 /*bytesPerChannel*/
              );
    }

    /// Creates a cubemap containing tap vectors and solid angle of each texel on the cubemap.
    /// Output consists of 6 faces of specified size containing (x,y,z,angle) floats for each texel.
    void* buildCubemapNormalSolidAngle(void* _mem, size_t _size, uint32_t _cubemapFaceSize, EdgeFixup::Enum _fixup = EdgeFixup::None)
    {
        const float cfs = float(int32_t(_cubemapFaceSize));
        const float invCfs = 1.0f/cfs;

        if (EdgeFixup::None == _fixup)
        {
            float* dstPtr = (float*)_mem;
            for(uint8_t face = 0; face < 6; ++face)
            {
                float yyf = 1.0f;
                for (uint32_t yy = 0; yy < _cubemapFaceSize; ++yy, yyf+=2.0f)
                {
                    float xxf = 1.0f;
                    for (uint32_t xx = 0; xx < _cubemapFaceSize; ++xx, xxf+=2.0f)
                    {
                        // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                        // Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
                        //      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
                        const float uu = xxf*invCfs - 1.0f;
                        const float vv = yyf*invCfs - 1.0f;

                        texelCoordToVec(dstPtr, uu, vv, face);
                        dstPtr[3] = texelSolidAngle(uu, vv, invCfs);

                        dstPtr += 4;
                    }
                }
            }
        }
        else //if (EdgeFixup::Warp == _fixup)#
        {
            const float warp = warpFixupFactor(cfs);

            float* dstPtr = (float*)_mem;
            for(uint8_t face = 0; face < 6; ++face)
            {
                float yyf = 1.0f;
                for (uint32_t yy = 0; yy < _cubemapFaceSize; ++yy, yyf+=2.0f)
                {
                    float xxf = 1.0f;
                    for (uint32_t xx = 0; xx < _cubemapFaceSize; ++xx, xxf+=2.0f)
                    {
                        // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                        // Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
                        //      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
                        const float uu = xxf*invCfs - 1.0f;
                        const float vv = yyf*invCfs - 1.0f;

                        texelCoordToVecWarp(dstPtr, uu, vv, face, warp);
                        dstPtr[3] = texelSolidAngle(uu, vv, invCfs);

                        dstPtr += 4;
                    }
                }
            }
        }

        return (uint8_t*)_mem + _size;
    }

    float* buildCubemapNormalSolidAngle(uint32_t _cubemapFaceSize, EdgeFixup::Enum _fixup = EdgeFixup::None, AllocatorI* _allocator = g_allocator)
    {
        const size_t size = cubemapNormalSolidAngleSize(_cubemapFaceSize);
        float* mem = (float*)CMFT_ALLOC(_allocator, size);
        MALLOC_CHECK(mem);

        buildCubemapNormalSolidAngle(mem, size, _cubemapFaceSize, _fixup);

        return mem;
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
        float* cubemapVectors = buildCubemapNormalSolidAngle(_faceSize, EdgeFixup::None, g_allocator);
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
        // so it doesn't change almost anything, but it doesn't cost much be more correct.
        const double norm = PI4 / weightAccum;
        for (uint8_t ii = 0; ii < SH_COEFF_NUM; ++ii)
        {
            _shCoeffs[ii][0] *= norm;
            _shCoeffs[ii][1] *= norm;
            _shCoeffs[ii][2] *= norm;
        }

        CMFT_FREE(g_allocator, cubemapVectors);
    }

    bool imageShCoeffs(double _shCoeffs[SH_COEFF_NUM][3], const Image& _image, AllocatorI* _allocator)
    {
        // Input image must be a cubemap.
        if (!imageIsCubemap(_image))
        {
            return false;
        }

        // Processing is done in Rgba32f format.
        ImageSoftRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _image, _allocator);

        // Get face data offsets.
        uint32_t faceOffsets[6];
        imageGetFaceOffsets(faceOffsets, imageRgba32f);

        // Compute spherical harmonic coefficients.
        cubemapShCoeffs(_shCoeffs, imageRgba32f.m_data, imageRgba32f.m_width, faceOffsets);

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);

        return true;
    }

    bool imageIrradianceFilterSh(Image& _dst, uint32_t _dstFaceSize, const Image& _src, AllocatorI* _allocator)
    {
        // Input image must be a cubemap.
        if (!imageIsCubemap(_src))
        {
            return false;
        }

        // Processing is done in Rgba32f format.
        ImageSoftRef imageRgba32f;
        imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);

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
        void* dstData = CMFT_ALLOC(_allocator, dstDataSize);
        MALLOC_CHECK(dstData);

        // Build cubemap texel vectors.
        float* cubemapVectors = buildCubemapNormalSolidAngle(dstFaceSize, EdgeFixup::None, g_allocator);
        const uint8_t vectorBytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        const uint32_t vectorPitch = dstFaceSize * vectorBytesPerPixel;
        const uint32_t vectorFaceDataSize = vectorPitch * dstFaceSize;

        uint64_t totalTime = cmft::getHPCounter();

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
        const double freq = double(cmft::getHPFrequency());
        const double toSec = 1.0/freq;
        totalTime = cmft::getHPCounter() - totalTime;
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
            imageMove(_dst, result, _allocator);
        }
        else
        {
            imageConvert(_dst, (TextureFormat::Enum)_src.m_format, result, _allocator);
            imageUnload(result, _allocator);
        }

        // Cleanup.
        imageUnload(imageRgba32f, _allocator);

        CMFT_FREE(g_allocator, cubemapVectors);

        return true;
    }

    void imageIrradianceFilterSh(Image& _image, uint32_t _faceSize, AllocatorI* _allocator)
    {
        Image tmp;
        if (imageIrradianceFilterSh(tmp, _faceSize, _image, _allocator))
        {
            imageMove(_image, tmp, _allocator);
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
            m_min[0] = CMFT_MIN(m_min[0], _x);
            m_min[1] = CMFT_MIN(m_min[1], _y);
            m_max[0] = CMFT_MAX(m_max[0], _x);
            m_max[1] = CMFT_MAX(m_max[1], _y);
        }

        inline void clampMin(float _x, float _y)
        {
            m_min[0] = CMFT_MAX(m_min[0], _x);
            m_min[1] = CMFT_MAX(m_min[1], _y);
        }

        inline void clampMax(float _x, float _y)
        {
            m_max[0] = CMFT_MIN(m_max[0], _x);
            m_max[1] = CMFT_MIN(m_max[1], _y);
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

            NeighbourSidesCount,
        };

        struct NeighourFaceBleed
        {
            float m_amount;
            float m_bbMin;
            float m_bbMax;
        } bleed[NeighbourSidesCount] =
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

    static inline size_t cubemapFilterAreaSize(uint32_t _cubemapFaceSize)
    {
        return (_cubemapFaceSize /*width*/
              * _cubemapFaceSize /*height*/
              * (6*4) /*numChannels*/
              * 4 /*bytesPerChannel*/
              );
    }

    void* buildCubemapFilterArea(void* _mem, size_t _size, uint8_t _faceIdx, uint32_t _cubemapFaceSize, float _filterSize, EdgeFixup::Enum _fixup = EdgeFixup::None)
    {
        const float cfs = float(int32_t(_cubemapFaceSize));
        const float invCfs = 1.0f/cfs;

        if (EdgeFixup::None == _fixup)
        {
            float* dstPtr = (float*)_mem;

            float yyf = 1.0f;
            for (uint32_t yy = 0; yy < _cubemapFaceSize; ++yy, yyf+=2.0f)
            {
                float xxf = 1.0f;
                for (uint32_t xx = 0; xx < _cubemapFaceSize; ++xx, xxf+=2.0f)
                {
                    // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                    // Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
                    //      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
                    const float uu = xxf*invCfs - 1.0f;
                    const float vv = yyf*invCfs - 1.0f;

                    float tapVec[3];
                    texelCoordToVec(tapVec, uu, vv, _faceIdx);

                    Aabb* faces = (Aabb*)dstPtr;
                    determineFilterArea(faces, tapVec, _filterSize);

                    dstPtr += (6*4); // six faces -> 6 x Aabb
                }
            }
        }
        else //if (EdgeFixup::Warp == _fixup)#
        {
            const float warp = warpFixupFactor(cfs);
            float* dstPtr = (float*)_mem;

            float yyf = 1.0f;
            for (uint32_t yy = 0; yy < _cubemapFaceSize; ++yy, yyf+=2.0f)
            {
                float xxf = 1.0f;
                for (uint32_t xx = 0; xx < _cubemapFaceSize; ++xx, xxf+=2.0f)
                {
                    // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                    // Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
                    //      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
                    const float uu = xxf*invCfs - 1.0f;
                    const float vv = yyf*invCfs - 1.0f;

                    float tapVec[3];
                    texelCoordToVecWarp(tapVec, uu, vv, _faceIdx, warp);

                    Aabb* faces = (Aabb*)dstPtr;
                    determineFilterArea(faces, tapVec, _filterSize);

                    dstPtr += (6*4); // six faces -> 6 x Aabb
                }
            }
        }

        return (uint8_t*)_mem + _size;
    }

    float* buildCubemapFilterArea(uint8_t _faceIdx, uint32_t _cubemapFaceSize, float _filterSize, EdgeFixup::Enum _fixup = EdgeFixup::None, AllocatorI* _allocator = g_allocator)
    {
        const size_t size = cubemapFilterAreaSize(_cubemapFaceSize);
        float* mem = (float*)CMFT_ALLOC(_allocator, size);
        MALLOC_CHECK(mem);

        buildCubemapFilterArea(mem, size, _faceIdx, _cubemapFaceSize, _filterSize, _fixup);

        return mem;
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
        const uint32_t normalFaceSize = pitch*_srcFaceSize;
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
            const uint8_t* faceNormals = (const uint8_t*)_cubemapNormalSolidAngle + normalFaceSize*face;

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

    void radianceFilter(float* _dstPtr
                      , uint8_t _face
                      , uint32_t _mipFaceSize
                      , float _filterSize
                      , float _specularPower
                      , float _specularAngle
                      , const float* _cubemapVectors
                      , const Image* _imageRgba32f
                      , const uint32_t _faceOffsets[CUBE_FACE_NUM]
                      , EdgeFixup::Enum _fixup
                      )
    {
        const float mfs = float(int32_t(_mipFaceSize));
        const float invMfs = 1.0f/mfs;

        if (EdgeFixup::None == _fixup)
        {
            float yyf = 1.0f;
            for (uint32_t yy = 0; yy < _mipFaceSize; ++yy, yyf+=2.0f)
            {
                float xxf = 1.0f;
                for (uint32_t xx = 0; xx < _mipFaceSize; ++xx, xxf+=2.0f)
                {
                    // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                    // Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
                    //      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
                    const float uu = xxf*invMfs - 1.0f;
                    const float vv = yyf*invMfs - 1.0f;

                    float tapVec[3];
                    texelCoordToVec(tapVec, uu, vv, _face);

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
        else //if (EdgeFixup::Warp == _fixup)#
        {
            const float warp = warpFixupFactor(mfs);

            float yyf = 1.0f;
            for (uint32_t yy = 0; yy < _mipFaceSize; ++yy, yyf+=2.0f)
            {
                float xxf = 1.0f;
                for (uint32_t xx = 0; xx < _mipFaceSize; ++xx, xxf+=2.0f)
                {
                    // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
                    // Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
                    //      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
                    const float uu = xxf*invMfs - 1.0f;
                    const float vv = yyf*invMfs - 1.0f;

                    float tapVec[3];
                    texelCoordToVecWarp(tapVec, uu, vv, _face, warp);

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
    }

    struct RadianceFilterGlobalState
    {
        RadianceFilterGlobalState()
        {
            reset();
        }

        void reset()
        {
            m_startTime         = 0;
            m_completedTasksGpu = 0;
            m_completedTasksCpu = 0;
            m_totalTasks        = 0;
            m_threadId          = 0;
        }

        uint8_t getThreadId()
        {
            std::lock_guard<std::mutex> lock(m_threadIdMutex);
            return m_threadId++;
        }

        void incrCompletedTasksGpu()
        {
            std::lock_guard<std::mutex> lock(m_completedTasks);
            m_completedTasksGpu++;
            CMFT_PROGRESS("%d %d", m_completedTasksCpu + m_completedTasksGpu, m_totalTasks);
        }

        void incrCompletedTasksCpu()
        {
            std::lock_guard<std::mutex> lock(m_completedTasks);
            m_completedTasksCpu++;
            CMFT_PROGRESS("%d %d", m_completedTasksCpu + m_completedTasksGpu, m_totalTasks);
        }

        uint64_t m_startTime;
        uint16_t m_completedTasksGpu;
        uint16_t m_completedTasksCpu;
        uint16_t m_totalTasks;
        uint8_t m_threadId;
        std::mutex m_threadIdMutex;
        std::mutex m_completedTasks;
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
        EdgeFixup::Enum m_edgeFixup;
    };

    struct RadianceFilterTaskList
    {
        RadianceFilterTaskList(uint8_t _mipBegin, uint8_t _mipTotalCount)
        {
            m_mipStart = _mipBegin;
            m_mipEnd   = _mipTotalCount-1;
            memset(m_mipFace, 0, MAX_MIP_NUM);

            m_unfinishedCount = 0;
        }

        void set(uint8_t _mip, uint8_t _face, RadianceFilterParams* _params)
        {
            memcpy(&m_params[_mip][_face], _params, sizeof(RadianceFilterParams));
        }

        // Returns cube face radiance filter parameters starting from the top mip level.
        const RadianceFilterParams* getFromTop()
        {
            std::lock_guard<std::mutex> lock(m_access);

            while (m_mipStart <= m_mipEnd)
            {
                if (m_mipFace[m_mipStart] >= 6)
                {
                    m_mipStart++;
                }
                else
                {
                    const uint8_t face = m_mipFace[m_mipStart]++;
                    return &m_params[m_mipStart][face];
                }

            }

            return NULL;
        }

        // Returns cube face radiance filter parameters starting from the bottom mip level.
        const RadianceFilterParams* getFromBottom()
        {
            std::lock_guard<std::mutex> lock(m_access);

            while (m_mipStart <= m_mipEnd)
            {
                if (m_mipFace[m_mipEnd] >= 6)
                {
                    m_mipEnd--;
                }
                else
                {
                    const uint8_t face = m_mipFace[m_mipEnd]++;
                    return &m_params[m_mipEnd][face];
                }
            }

            return NULL;
        }

        void pushUnfinished(const RadianceFilterParams* _params)
        {
            std::lock_guard<std::mutex> lock(m_accessUnfinished);
            m_unfinished[m_unfinishedCount++] = _params;
        }

        const RadianceFilterParams* popUnfinished()
        {
            std::lock_guard<std::mutex> lock(m_accessUnfinished);
            return (0 == m_unfinishedCount) ? NULL : m_unfinished[--m_unfinishedCount];
        }

        uint16_t unfinishedCount() const
        {
            return m_unfinishedCount;
        }

    private:
        std::mutex m_access;
        int8_t m_mipStart;
        int8_t m_mipEnd;
        int8_t m_mipFace[MAX_MIP_NUM];
        RadianceFilterParams m_params[MAX_MIP_NUM][CUBE_FACE_NUM];

        std::mutex m_accessUnfinished;
        uint16_t m_unfinishedCount;
        const RadianceFilterParams* m_unfinished[MAX_MIP_NUM*CUBE_FACE_NUM];
    };

    int32_t radianceFilterCpu(void* _taskList)
    {
        const uint8_t threadId = s_globalState.getThreadId();
        const double freq = double(cmft::getHPFrequency());
        const double toSec = 1.0/freq;

        RadianceFilterTaskList* taskList = (RadianceFilterTaskList*)_taskList;

        // Cpu is processing from the bottom level mip map to the top.
        const RadianceFilterParams* params;
        while ((params = taskList->getFromBottom()) != NULL
        ||     (params = taskList->popUnfinished()) != NULL)
        {
            // Start timer.
            const uint64_t startTime = cmft::getHPCounter();

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
                         , params->m_edgeFixup
                         );

            // Determine task duration.
            const uint64_t currentTime = cmft::getHPCounter();
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
            s_globalState.incrCompletedTasksCpu();
        }

        return EXIT_SUCCESS;
    }

    struct RadianceProgram
    {
        RadianceProgram()
        {
            m_clContext       = NULL;
            m_program         = NULL;
            m_radFilter       = NULL;
            m_radFilterSingle = NULL;
            m_sum             = NULL;
            m_event           = NULL;
            m_memOut          = NULL;
            m_prevDstFaceSize = 0;
            m_srcFaceSize     = 0.0f;
            m_memFaceData[0]  = NULL;
            m_memFaceData[1]  = NULL;
            m_memFaceData[2]  = NULL;
            m_memFaceData[3]  = NULL;
            m_memFaceData[4]  = NULL;
            m_memFaceData[5]  = NULL;
            m_memNormalSolidAngle[0] = NULL;
            m_memNormalSolidAngle[1] = NULL;
            m_memNormalSolidAngle[2] = NULL;
            m_memNormalSolidAngle[3] = NULL;
            m_memNormalSolidAngle[4] = NULL;
            m_memNormalSolidAngle[5] = NULL;
        }

        void setDeviceContext(const ClContext* _clContext)
        {
            m_clContext = _clContext;
        }

        bool hasValidDeviceContext() const
        {
            return (NULL != m_clContext && NULL != m_clContext->m_context);
        }

        bool isValid() const
        {
            return (NULL != m_program);
        }

        void invalidate()
        {
            if (NULL != m_program)
            {
                clReleaseProgram(m_program);
                m_program = NULL;
            }
        }

        bool createFromStr(const char* _source, size_t _sourceSize, const char* _header, size_t _headerSize)
        {
            const size_t headerEnd = _headerSize-1;
            const size_t sourceEnd = _sourceSize-1;
            const size_t total = headerEnd+sourceEnd;
            char* src = (char*)alloca(total);

            memcpy(src,           _header, headerEnd);
            memcpy(src+headerEnd, _source, sourceEnd);

            return createFromStr(src, total);
        }

        bool createFromStr(const char* _source, size_t _sourceSize)
        {
            cl_int err;

            // Create program.
            const char* sources[1] = { _source };
            const size_t sizes[1] = { _sourceSize };
            m_program = clCreateProgramWithSource(m_clContext->m_context, 1, sources, sizes, &err);
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
                char buffer[1024*16];
                clGetProgramBuildInfo(m_program, m_clContext->m_device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
                WARN("CL Compilation failed:\n%s", buffer);

                return false;
            }

            // Create kernels.
            m_radFilter = clCreateKernel(m_program, "radianceFilter", &err);
            if (CL_SUCCESS != err)
            {
                WARN("Could not create OpenCL kernel. Kernel name probably inavlid! Should be: 'radianceFilter'");
                return false;
            }

            m_radFilterSingle = clCreateKernel(m_program, "radianceFilterSingleFace", &err);
            if (CL_SUCCESS != err)
            {
                WARN("Could not create OpenCL kernel. Kernel name probably inavlid! Should be: 'radianceFilterSingleFace'");
                return false;
            }

            m_sum = clCreateKernel(m_program, "sum", &err);
            if (CL_SUCCESS != err)
            {
                WARN("Could not create OpenCL kernel. Kernel name probably inavlid! Should be: 'sum'");
                return false;
            }

            return true;
        }

        bool createFromFile(const char* _filePath, const char* _header = NULL, size_t _headerSize = 0)
        {
            size_t read;
            CMFT_UNUSED(read);

            // Open file.
            FILE* fp = fopen(_filePath, "rb");
            if (NULL == fp)
            {
                WARN("Could not open file %s for reading.", _filePath);
                return false;
            }

            // Alloc data for string.
            const size_t fileSize = cmft::fsize(fp);
            char* sourceData = (char*)CMFT_ALLOC(g_allocator, fileSize);

            // Read opencl source file.
            read = fread(sourceData, fileSize, 1, fp);
            DEBUG_CHECK(read == 1, "Could not read from file.");
            FERROR_CHECK(fp);

            bool result;
            if (NULL != _header)
            {
                result = createFromStr(sourceData, fileSize, _header, _headerSize);
            }
            else
            {
                result = createFromStr(sourceData, fileSize);
            }

            CMFT_FREE(g_allocator, sourceData);
            fclose(fp);

            return result;
        }

        #define CL_CHECK_RETURN(_err)       \
            do                              \
            {                               \
                if (CL_SUCCESS != _err)     \
                {                           \
                    WARN("OpenCL failed!"); \
                    return false;           \
                }                           \
            } while(0)

        #define CL_CHECK_EXPR_RETURN(_expr) \
            do                              \
            {                               \
                cl_int result = _expr;      \
                if (CL_SUCCESS != result)   \
                {                           \
                    WARN("OpenCL failed!"); \
                    return false;           \
                }                           \
            } while(0)

        bool initDeviceMemory(const Image& _image, float* _cubemapNormalSolidAngle)
        {
            cl_int err;

            uint32_t faceOffsets[CUBE_FACE_NUM];
            imageGetFaceOffsets(faceOffsets, _image);
            const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
            const uint32_t normalFaceSize = _image.m_width * _image.m_width * bytesPerPixel;
            static const cl_image_format sc_imageFormat = { CL_RGBA, CL_FLOAT };

            for (uint8_t face = 0; face < 6; ++face)
            {
                m_memFaceData[face] = clCreateImage2D(m_clContext->m_context
                                                    , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                                    , &sc_imageFormat
                                                    , _image.m_width
                                                    , _image.m_height
                                                    , _image.m_width*bytesPerPixel
                                                    , ((uint8_t*)_image.m_data + faceOffsets[face])
                                                    , &err
                                                    );
                CL_CHECK_RETURN(err);

                m_memNormalSolidAngle[face] = clCreateImage2D(m_clContext->m_context
                                                            , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                                            , &sc_imageFormat
                                                            , _image.m_width
                                                            , _image.m_height
                                                            , _image.m_width*bytesPerPixel
                                                            , ((uint8_t*)_cubemapNormalSolidAngle + normalFaceSize*face)
                                                            , &err
                                                            );
                CL_CHECK_RETURN(err);
            }

            m_srcFaceSize = float(int32_t(_image.m_width));

            return true;
        }

        bool processAllAtOnce(void* _out
                            , uint8_t _faceIdx
                            , uint32_t _dstFaceSize
                            , float _specularPower
                            , float _specularAngle
                            , float _filterSize
                            , EdgeFixup::Enum _edgeFixup = EdgeFixup::None
                            )
        {
            cl_int err;
            static const cl_image_format sc_imageFormat = { CL_RGBA, CL_FLOAT };

            const float warp = warpFixupFactor(float(int32_t(_dstFaceSize)));
            const size_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;

            // Create output image.
            if (m_prevDstFaceSize != _dstFaceSize)
            {
                if (NULL != m_memOut)
                {
                    clReleaseMemObject(m_memOut);
                }

                m_memOut = clCreateImage2D(m_clContext->m_context
                                         , CL_MEM_WRITE_ONLY
                                         , &sc_imageFormat
                                         , _dstFaceSize
                                         , _dstFaceSize
                                         , 0
                                         , NULL
                                         , &err
                                         );
                CL_CHECK_RETURN(err);
            }
            m_prevDstFaceSize = _dstFaceSize;

            #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                // Build filter area info.
                float* filterArea = buildCubemapFilterArea(_faceIdx, _dstFaceSize, _filterSize, _edgeFixup, &g_crtAllocator);
                const size_t width = _dstFaceSize*6;
                const size_t height = _dstFaceSize;
                cl_mem area = clCreateImage2D(m_clContext->m_context
                                            , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                            , &sc_imageFormat
                                            , width
                                            , height
                                            , width*bytesPerPixel
                                            , filterArea
                                            , &err
                                            );
                CL_CHECK_RETURN(err);
            #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU
            CMFT_UNUSED(_edgeFixup);

            // Set arguments.
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  0, sizeof(cl_mem),  (const void*)&m_memOut));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  1, sizeof(int32_t), (const void*)&_dstFaceSize));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  2, sizeof(float),   (const void*)&_specularPower));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  3, sizeof(float),   (const void*)&_specularAngle));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  4, sizeof(float),   (const void*)&_filterSize));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  5, sizeof(float),   (const void*)&warp));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  6, sizeof(int8_t),  (const void*)&_faceIdx));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  7, sizeof(float),   (const void*)&m_srcFaceSize));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  8, sizeof(cl_mem),  (const void*)&m_memFaceData[0]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter,  9, sizeof(cl_mem),  (const void*)&m_memFaceData[1]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 10, sizeof(cl_mem),  (const void*)&m_memFaceData[2]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 11, sizeof(cl_mem),  (const void*)&m_memFaceData[3]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 12, sizeof(cl_mem),  (const void*)&m_memFaceData[4]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 13, sizeof(cl_mem),  (const void*)&m_memFaceData[5]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 14, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[0]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 15, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[1]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 16, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[2]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 17, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[3]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 18, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[4]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 19, sizeof(cl_mem),  (const void*)&m_memNormalSolidAngle[5]));
            #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilter, 20, sizeof(cl_mem),  (const void*)&area));
            #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

            // Process in tiles of 64x64.
            const uint32_t tileSize = 64;
            const uint32_t count = ((_dstFaceSize-1)/tileSize)+1;
            for (uint32_t yy = 0; yy < count; ++yy)
            {
                for (uint32_t xx = 0; xx < count; ++xx)
                {
                    const size_t workOffset[2] = { xx*tileSize, yy*tileSize };
                    const size_t workSize[2] =
                    {
                        CMFT_MIN(tileSize, _dstFaceSize-workOffset[0]),
                        CMFT_MIN(tileSize, _dstFaceSize-workOffset[1]),
                    };
                    CL_CHECK_EXPR_RETURN(clEnqueueNDRangeKernel(m_clContext->m_commandQueue
                                                              , m_radFilter
                                                              , 2
                                                              , workOffset
                                                              , workSize
                                                              , NULL
                                                              , 0
                                                              , NULL
                                                              , &m_event
                                                              ));
                }
            }

            const size_t origin[3] = { 0, 0, 0 };
            const size_t region[3] = { _dstFaceSize, _dstFaceSize, 1 };
            CL_CHECK_EXPR_RETURN(clEnqueueReadImage(m_clContext->m_commandQueue
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

            #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                clReleaseMemObject(area);
                CMFT_FREE(g_allocator, filterArea);
            #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

            return true;
        }

        bool processFaceByFaceAndSum(void* _out
                                   , uint8_t _faceIdx
                                   , uint32_t _dstFaceSize
                                   , float _specularPower
                                   , float _specularAngle
                                   , float _filterSize
                                   , EdgeFixup::Enum _edgeFixup = EdgeFixup::None
                                   )
        {
            cl_int err;
            static const cl_image_format sc_imageFormat = { CL_RGBA, CL_FLOAT };

            const float warp = warpFixupFactor(float(int32_t(_dstFaceSize)));
            const size_t workSize[2] = { _dstFaceSize, _dstFaceSize };
            const size_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;

            #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                // Build filter area info.
                float* filterArea = buildCubemapFilterArea(_faceIdx, _dstFaceSize, _filterSize, _edgeFixup, &g_crtAllocator);
                const size_t width = _dstFaceSize*6;
                const size_t height = _dstFaceSize;
                cl_mem area = clCreateImage2D(m_clContext->m_context
                                            , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                            , &sc_imageFormat
                                            , width
                                            , height
                                            , width*bytesPerPixel
                                            , filterArea
                                            , &err
                                            );
                CL_CHECK_RETURN(err);
            #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU
            CMFT_UNUSED(_edgeFixup);

            // Set arguments that do not change for the entire task.
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle,  4, sizeof(int32_t), (const void*)&_dstFaceSize));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle,  5, sizeof(float),   (const void*)&_specularPower));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle,  6, sizeof(float),   (const void*)&_specularAngle));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle,  7, sizeof(float),   (const void*)&_filterSize));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle,  8, sizeof(float),   (const void*)&warp));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle,  9, sizeof(int8_t),  (const void*)&_faceIdx));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle, 10, sizeof(float),   (const void*)&m_srcFaceSize));
            #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle, 11, sizeof(cl_mem),  (const void*)&area));
            #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

            // Process each face separately in tiles of 64x64.
            const uint32_t tileSize = 64;
            const uint32_t count = ((_dstFaceSize-1)/tileSize)+1;
            cl_mem faces[6];
            for (uint8_t ii = 0; ii < 6; ++ii)
            {
                faces[ii] = clCreateImage2D(m_clContext->m_context
                                          , CL_MEM_READ_WRITE
                                          , &sc_imageFormat
                                          , _dstFaceSize
                                          , _dstFaceSize
                                          , 0
                                          , NULL
                                          , &err
                                          );
                CL_CHECK_RETURN(err);

                const uint8_t faceIdx = ii;
                CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle, 0, sizeof(cl_mem), (const void*)&faces[ii]));
                CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle, 1, sizeof(cl_mem), (const void*)&m_memFaceData[ii]));
                CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle, 2, sizeof(cl_mem), (const void*)&m_memNormalSolidAngle[ii]));
                CL_CHECK_EXPR_RETURN(clSetKernelArg(m_radFilterSingle, 3, sizeof(int8_t), (const void*)&faceIdx));

                for (uint32_t yy = 0; yy < count; ++yy)
                {
                    for (uint32_t xx = 0; xx < count; ++xx)
                    {
                        const size_t tileWorkOffset[2] = { xx*tileSize, yy*tileSize };
                        const size_t tileWorkSize[2] =
                        {
                            CMFT_MIN(tileSize, _dstFaceSize-tileWorkOffset[0]),
                            CMFT_MIN(tileSize, _dstFaceSize-tileWorkOffset[1]),
                        };
                        CL_CHECK_EXPR_RETURN(clEnqueueNDRangeKernel(m_clContext->m_commandQueue
                                                                  , m_radFilterSingle
                                                                  , 2
                                                                  , tileWorkOffset
                                                                  , tileWorkSize
                                                                  , NULL
                                                                  , 0
                                                                  , NULL
                                                                  , &m_event
                                                                  ));
                    }
                }
            }

            // Sum partial results.
            if (m_prevDstFaceSize != _dstFaceSize)
            {
                if (NULL != m_memOut)
                {
                    clReleaseMemObject(m_memOut);
                }

                m_memOut = clCreateImage2D(m_clContext->m_context
                                         , CL_MEM_WRITE_ONLY
                                         , &sc_imageFormat
                                         , _dstFaceSize
                                         , _dstFaceSize
                                         , 0
                                         , NULL
                                         , &err
                                         );
                CL_CHECK_RETURN(err);
            }
            m_prevDstFaceSize = _dstFaceSize;

            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_sum, 0, sizeof(cl_mem), (const void*)&m_memOut));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_sum, 1, sizeof(cl_mem), (const void*)&faces[0]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_sum, 2, sizeof(cl_mem), (const void*)&faces[1]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_sum, 3, sizeof(cl_mem), (const void*)&faces[2]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_sum, 4, sizeof(cl_mem), (const void*)&faces[3]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_sum, 5, sizeof(cl_mem), (const void*)&faces[4]));
            CL_CHECK_EXPR_RETURN(clSetKernelArg(m_sum, 6, sizeof(cl_mem), (const void*)&faces[5]));
            CL_CHECK_EXPR_RETURN(clEnqueueNDRangeKernel(m_clContext->m_commandQueue, m_sum, 2, NULL, workSize, NULL, 0, NULL, &m_event));

            // Read result.
            const size_t origin[3] = { 0, 0, 0 };
            const size_t region[3] = { _dstFaceSize, _dstFaceSize, 1 };
            CL_CHECK_EXPR_RETURN(clEnqueueReadImage(m_clContext->m_commandQueue
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

            #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                clReleaseMemObject(area);
                CMFT_FREE(&g_crtAllocator, filterArea);
            #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

            return true;
        }

        #undef CL_CHECK_RETURN
        #undef CL_CHECK_EXPR_RETURN

        bool run(void* _out, uint8_t _faceIdx, uint32_t _dstFaceSize, float _specularPower, float _specularAngle, float _filterSize, EdgeFixup::Enum _edgeFixup = EdgeFixup::None)
        {
            /// if (m_srcFaceSize > 512)
            /// {
            ///     // Prevents driver crash by running 6+1 smaller kernels instead of a big one.
            ///     return processFaceByFaceAndSum(_out, _faceIdx, _dstFaceSize, _specularPower, _specularAngle, _filterSize, _edgeFixup);
            /// }
            /// else
            /// {
            ///     return processAllAtOnce(_out, _faceIdx, _dstFaceSize, _specularPower, _specularAngle, _filterSize, _edgeFixup);
            /// }

            return processFaceByFaceAndSum(_out, _faceIdx, _dstFaceSize, _specularPower, _specularAngle, _filterSize, _edgeFixup);
        }

        bool isIdle() const
        {
            cl_int status;
            clGetEventInfo(m_event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), (void*)&status, NULL);

            return ((CL_COMPLETE == status) || (NULL == m_event));
        }

        void finish() const
        {
            CL_CHECK(clFinish(m_clContext->m_commandQueue));
        }

        void releaseDeviceMemory()
        {
        #define RELEASE_CL_MEM(_mem) do { if (NULL != _mem) { clReleaseMemObject(_mem); _mem = NULL; } } while (0)
            RELEASE_CL_MEM(m_memOut);
            RELEASE_CL_MEM(m_memFaceData[0]);
            RELEASE_CL_MEM(m_memFaceData[1]);
            RELEASE_CL_MEM(m_memFaceData[2]);
            RELEASE_CL_MEM(m_memFaceData[3]);
            RELEASE_CL_MEM(m_memFaceData[4]);
            RELEASE_CL_MEM(m_memFaceData[5]);
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
        #define RELEASE_CL_PROG(_prog)     do { if (NULL != _prog  ) { clReleaseProgram(_prog);  _prog   = NULL; } } while (0)
        #define RELEASE_CL_KERNEL(_kernel) do { if (NULL != _kernel) { clReleaseKernel(_kernel); _kernel = NULL; } } while (0)
            RELEASE_CL_PROG(m_program);
            RELEASE_CL_KERNEL(m_radFilter);
            RELEASE_CL_KERNEL(m_radFilterSingle);
            RELEASE_CL_KERNEL(m_sum);
        #undef RELEASE_CL_KERNEL
        #undef RELEASE_CL_PROG
        }

        const ClContext* m_clContext;
        cl_program m_program;
        cl_kernel m_radFilter;
        cl_kernel m_radFilterSingle;
        cl_kernel m_sum;
        cl_event m_event;
        cl_mem m_memOut;
        uint32_t m_prevDstFaceSize;
        float m_srcFaceSize;
        cl_mem m_memFaceData[6];
        cl_mem m_memNormalSolidAngle[6];
    };
    RadianceProgram s_radianceProgram;

    int32_t radianceFilterGpu(void* _taskList)
    {
        if (!s_radianceProgram.isValid())
        {
            return EXIT_FAILURE;
        }

        const double freq = double(cmft::getHPFrequency());
        const double toSec = 1.0/freq;

        RadianceFilterTaskList* taskList = (RadianceFilterTaskList*)_taskList;

        // Gpu is processing from the top level mip map to the bottom.
        const RadianceFilterParams* params;
        while ((params = taskList->getFromTop()) != NULL)
        {
            // Start timer.
            const uint64_t startTime = cmft::getHPCounter();

            // Run radiance program.
            const bool result = s_radianceProgram.run(params->m_dstPtr
                                                    , params->m_face
                                                    , params->m_mipFaceSize
                                                    , params->m_specularPower
                                                    , params->m_specularAngle
                                                    , params->m_filterSize
                                                    , params->m_edgeFixup
                                                    );
            if (result)
            {
                // Determine task duration.
                const uint64_t currentTime = cmft::getHPCounter();
                const uint64_t taskDuration = currentTime - startTime;
                const uint64_t totalDuration = currentTime - s_globalState.m_startTime;

                // Output process info.
                INFO("Radiance ->  <GPU>  | %4u | %7.3fs | %7.3fs"
                     , params->m_mipFaceSize
                     , double(taskDuration)*toSec
                     , double(totalDuration)*toSec
                    );

                // Update task counter.
                s_globalState.incrCompletedTasksGpu();
            }
            else
            {
                taskList->pushUnfinished(params);
                return EXIT_FAILURE;
            }
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

    float specularPowerFor(float _mip, float _mipCount, float _glossScale, float _glossBias)
    {
        const float glossiness = CMFT_MAX(0.0f, 1.0f - _mip/(_mipCount-(1.0f+0.0000001f)));
        const float specularPower = powf(2.0f, _glossScale * glossiness + _glossBias);
        return specularPower;
    }

    float applyLightningModel(float _specularPower, LightingModel::Enum _lightingModel)
    {
        /// http://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/
        /// http://seblagarde.wordpress.com/2012/03/29/relationship-between-phong-and-blinn-lighting-model/
        switch (_lightingModel)
        {
        case LightingModel::Phong:
            {
                return _specularPower;
            }
        break;

        case LightingModel::PhongBrdf:
            {
                return _specularPower + 1.0f;
            }
        break;

        case LightingModel::Blinn:
            {
                return _specularPower/4.0f;
            }
        break;

        case LightingModel::BlinnBrdf:
            {
                return _specularPower/4.0f + 1.0f;
            }
        break;

        default:
            {
                DEBUG_CHECK(false, "ERROR! This should never happen!");
                return _specularPower;
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
                           , EdgeFixup::Enum _edgeFixup
                           , uint8_t _numCpuProcessingThreads
                           , ClContext* _clContext
                           , AllocatorI* _allocator
                           )
    {
        // Input image must be a cubemap.
        if (!imageIsCubemap(_src))
        {
            WARN("Image is not cubemap.");

            return false;
        }

        // Multi-threading parameters.
        std::thread cpuThreads[64];
        uint32_t activeCpuThreads = 0;
        const uint32_t maxActiveCpuThreads = (uint32_t)CMFT_CLAMP(_numCpuProcessingThreads, 0, 64);

        // Prepare OpenCL kernel and device memory.

        s_radianceProgram.setDeviceContext(_clContext);
        if (s_radianceProgram.hasValidDeviceContext())
        {
            if (EdgeFixup::Warp == _edgeFixup)
            {
                #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                    const char header[] = "#define CMFT_COMPUTE_FILTER_AREA_ON_CPU 1\n"
                                          "#define WARP_FIXUP\n";
                #else
                    const char header[] = "#define CMFT_COMPUTE_FILTER_AREA_ON_CPU 0\n"
                                          "#define WARP_FIXUP\n";
                #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

                s_radianceProgram.createFromStr((const char*)sc_radianceSource, sizeof(sc_radianceSource), header, sizeof(header));
                //s_radianceProgram.createFromFile("radiance.cl", header, sizeof(header));
            }
            else
            {
                #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                    const char header[] = "#define CMFT_COMPUTE_FILTER_AREA_ON_CPU 1\n";
                #else
                    const char header[] = "#define CMFT_COMPUTE_FILTER_AREA_ON_CPU 0\n";
                #endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

                s_radianceProgram.createFromStr((const char*)sc_radianceSource, sizeof(sc_radianceSource), header, sizeof(header));
                //s_radianceProgram.createFromFile("radiance.cl", header, sizeof(header));
            }
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
        if (NULL != _clContext
        &&  _clContext->m_deviceType&CL_DEVICE_TYPE_CPU
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
        ImageSoftRef imageRgba32f;
        if (_allocator != &g_crtAllocator)
        {
            // Must be allocated with crtAllocator. Otherwise, opencl gpu driver will crash.
            Image imageCrtMemory;
            imageCopy(imageCrtMemory, _src, &g_crtAllocator);
            imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, &g_crtAllocator);
        }
        else
        {
            imageRefOrConvert(imageRgba32f, TextureFormat::RGBA32F, _src, _allocator);
        }

        // Alloc dst data.
        const uint32_t dstFaceSize = (0 == _dstFaceSize) ? _src.m_width : _dstFaceSize;
        const uint8_t mipMin = 1;
        const uint8_t mipMax = uint8_t(cmft::ftou(cmft::log2f(cmft::utof(dstFaceSize))) + 1);
        const uint8_t mipCount = CMFT_CLAMP(_mipCount, mipMin, mipMax);
        const uint32_t bytesPerPixel = 4 /*numChannels*/ * 4 /*bytesPerChannel*/;
        uint32_t dstOffsets[CUBE_FACE_NUM][MAX_MIP_NUM];
        uint32_t dstDataSize = 0;
        for (uint8_t face = 0; face < 6; ++face)
        {
            for (uint8_t mip = 0; mip < mipCount; ++mip)
            {
                dstOffsets[face][mip] = dstDataSize;
                uint32_t faceSize = CMFT_MAX(1, dstFaceSize >> mip);
                dstDataSize += faceSize * faceSize * bytesPerPixel;
            }
        }
        void* dstData = CMFT_ALLOC(&g_crtAllocator, dstDataSize);
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
             , &"false\0true"[6*_excludeBase]
             , mipCount
             , _glossScale
             , _glossBias
             , dstFaceSize
             );

        // Resize and copy base image.
        if (_excludeBase)
        {
            INFO("Radiance -> Excluding base image.");

            const float    dstToSrcRatiof = cmft::utof(imageRgba32f.m_width)/cmft::utof(dstFaceSize);
            const uint32_t dstToSrcRatio  = CMFT_MAX(UINT32_C(1), cmft::ftou(dstToSrcRatiof));
            const uint32_t dstFacePitch   = dstFaceSize * bytesPerPixel;
            const uint32_t srcFacePitch   = imageRgba32f.m_width * bytesPerPixel;

            // For all top level cubemap faces:
            for(uint8_t face = 0; face < 6; ++face)
            {
                const uint8_t* srcFaceData = (const uint8_t*)imageRgba32f.m_data + srcFaceOffsets[face];
                uint8_t* dstFaceData = (uint8_t*)dstData + dstOffsets[face][0];

                // Iterate through destination pixels.
                float yDstf = 0.0f;
                for (uint32_t yDst = 0; yDst < dstFaceSize; ++yDst, yDstf+=1.0f)
                {
                    uint8_t* dstFaceRow = (uint8_t*)dstFaceData + yDst*dstFacePitch;

                    float xDstf = 0.0f;
                    for (uint32_t xDst = 0; xDst < dstFaceSize; ++xDst, xDstf+=1.0f)
                    {
                        float* dstFaceColumn = (float*)((uint8_t*)dstFaceRow + xDst*bytesPerPixel);

                        // For each destination pixel, sample and accumulate color from source.
                        float color[3] = { 0.0f, 0.0f, 0.0f };
                        uint32_t weightAccum = 0;

                        for (uint32_t ySrc = cmft::ftou(yDstf*dstToSrcRatiof)
                            , yEnd = ySrc + dstToSrcRatio
                            ; ySrc < yEnd ; ++ySrc)
                        {
                            const uint8_t* srcRowData = (const uint8_t*)srcFaceData + ySrc*srcFacePitch;

                            for (uint32_t xSrc = cmft::ftou(xDstf*dstToSrcRatiof)
                                , xEnd = xSrc + dstToSrcRatio
                                ; xSrc < xEnd ; ++xSrc)
                            {
                                const float* srcColumnData = (const float*)((const uint8_t*)srcRowData + xSrc*bytesPerPixel);
                                color[0] += srcColumnData[0];
                                color[1] += srcColumnData[1];
                                color[2] += srcColumnData[2];
                                weightAccum++;
                            }
                        }

                        // Divide by weight and save to destination pixel.
                        const float invWeight = 1.0f/cmft::utof(CMFT_MAX(weightAccum, UINT32_C(1)));
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
            float* cubemapVectors = buildCubemapNormalSolidAngle(imageRgba32f.m_width, _edgeFixup, &g_crtAllocator);

            // Enqueue memory transfer for cl device.
            if (s_radianceProgram.isValid())
            {
                const bool success = s_radianceProgram.initDeviceMemory(imageRgba32f, cubemapVectors);
                if (!success)
                {
                    s_radianceProgram.invalidate();
                }
            }

            // Start global timer.
            s_globalState.reset();
            s_globalState.m_startTime = cmft::getHPCounter();
            s_globalState.m_totalTasks = mipCount*6;
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

            const float mipCountf   = float(int32_t(mipCount));
            const float glossScalef = float(int32_t(_glossScale));
            const float glossBiasf  = float(int32_t(_glossBias));

            //Prepare processing tasks parameters.
            for (uint32_t mip = mipStart; mip < mipCount; ++mip)
            {
                // Determine filter parameters.
                const uint32_t mipFaceSize = CMFT_MAX(1, dstFaceSize >> mip);
                const float mipFaceSizef = float(int32_t(mipFaceSize));
                const float minAngle = atan2f(1.0f, mipFaceSizef);
                const float maxAngle = (0.5f*CMFT_PI);
                const float toFilterSize = 1.0f/(minAngle*mipFaceSizef*2.0f);
                const float specularPowerRef = specularPowerFor(float(int32_t(mip)), mipCountf, glossScalef, glossBiasf);
                const float specularPower = applyLightningModel(specularPowerRef, _lightingModel);
                const float filterAngle = CMFT_CLAMP(cosinePowerFilterAngle(specularPower), minAngle, maxAngle);
                const float cosAngle = CMFT_MAX(0.0f, cosf(filterAngle));
                const float texelSize = 1.0f/mipFaceSizef;
                const float filterSize = CMFT_MAX(texelSize, filterAngle * toFilterSize);

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
                        _edgeFixup,
                    };

                    // Enqueue processing parameters.
                    taskList.set(mip, face, &taskParams);
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
                while (activeCpuThreads < maxActiveCpuThreads - 1)
                {
                    cpuThreads[activeCpuThreads++] = std::thread(radianceFilterCpu, (void*)&taskList);
                }

                // Start one GPU host thread.
                if (s_radianceProgram.isValid() && s_radianceProgram.isIdle())
                {
                    cpuThreads[activeCpuThreads++] = std::thread(radianceFilterGpu, (void*)&taskList);
                }

                // Wait for everything to finish.
                for (uint32_t ii = 0; ii < activeCpuThreads; ++ii)
                {
                    cpuThreads[ii].join();
                }

                // OpenCL failed and no CPU threads were selected for procesing.
                const uint16_t unfinished = taskList.unfinishedCount();
                if (unfinished > 0 && 0 == maxActiveCpuThreads)
                {
                    return false;
                }

                // Process unfinished tasks on CPU.
                const uint8_t numThreads = CMFT_MIN(unfinished, maxActiveCpuThreads);
                for (uint8_t ii = 0; ii < numThreads; ++ii)
                {
                    radianceFilterCpu((void*)&taskList);
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
            const double freq = double(cmft::getHPFrequency());
            const double toSec = 1.0/freq;
            const uint64_t totalTime = cmft::getHPCounter() - s_globalState.m_startTime;

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
            s_globalState.reset();

            CMFT_FREE(&g_crtAllocator, cubemapVectors);
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

    bool imageRadianceFilter(Image& _image
                           , uint32_t _dstFaceSize
                           , LightingModel::Enum _lightingModel
                           , bool _excludeBase
                           , uint8_t _mipCount
                           , uint8_t _glossScale
                           , uint8_t _glossBias
                           , EdgeFixup::Enum _edgeFixup
                           , uint8_t _numCpuProcessingThreads
                           , ClContext* _clContext
                           , AllocatorI* _allocator
                           )
    {
        Image tmp;
        if (imageRadianceFilter(tmp, _dstFaceSize, _lightingModel, _excludeBase, _mipCount, _glossScale, _glossBias, _image, _edgeFixup, _numCpuProcessingThreads, _clContext, _allocator))
        {
            imageMove(_image, tmp, _allocator);
            return true;
        }

        return false;
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
