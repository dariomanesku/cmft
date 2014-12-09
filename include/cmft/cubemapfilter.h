/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CUBEMAPFILTER_H_HEADER_GUARD
#define CMFT_CUBEMAPFILTER_H_HEADER_GUARD

#include "image.h"
#include <stdint.h> //uint32_t

namespace cmft
{
#define SH_COEFF_NUM 25

    /// Computes spherical harominics coefficients for given cubemap data.
    /// Input data should be in RGBA32F format.
    void cubemapShCoeffs(double _shCoeffs[SH_COEFF_NUM][3], void* _data, uint32_t _faceSize, uint32_t _faceOffsets[6]);

    /// Computes spherical harominics coefficients for given cubemap image.
    bool imageShCoeffs(double _shCoeffs[SH_COEFF_NUM][3], const Image& _image);

    /// Creates irradiance cubemap. Uses fast spherical harmonics implementation.
    bool imageIrradianceFilterSh(Image& _dst, uint32_t _dstFaceSize, const Image& _src);

    /// Converts cubemap image into irradiance cubemap. Uses fast spherical harmonics implementation.
    void imageIrradianceFilterSh(Image& _image, uint32_t _faceSize);

    struct LightingModel
    {
        enum Enum
        {
            Phong,
            PhongBrdf,
            Blinn,
            BlinnBrdf,

            Count
        };
    };

    /// Helper functions.
    float specularPowerFor(float _mip, float _mipCount, float _glossScale, float _glossBias);
    float applyLightningModel(float _specularPower, LightingModel::Enum _lightingModel);

    struct ClContext;

    /// Creates radiance cubemap image.
    bool imageRadianceFilter(Image& _dst
                           , uint32_t _dstFaceSize
                           , LightingModel::Enum _lightingModel
                           , bool _excludeBase
                           , uint8_t _mipCount
                           , uint8_t _glossScale
                           , uint8_t _glossBias
                           , const Image& _src
                           , int8_t _numCpuProcessingThreads = -1
                           , const ClContext* _clContext = NULL
                           );

    /// Converts cubemap image into radiance cubemap.
    void imageRadianceFilter(Image& _image
                           , uint32_t _dstFaceSize
                           , LightingModel::Enum _lightingModel
                           , bool _excludeBase
                           , uint8_t _mipCount
                           , uint8_t _glossScale
                           , uint8_t _glossBias
                           , int8_t _numCpuProcessingThreads = -1
                           , const ClContext* _clContext = NULL
                           );

} // namespace cmft

#endif // CMFT_CUBEMAPFILTER_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
