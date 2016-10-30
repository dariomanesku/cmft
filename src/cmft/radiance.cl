/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

typedef char  int8_t;
typedef short int16_t;
typedef int   int32_t;

__constant sampler_t s_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

__constant float4 s_faceUvVectors[6][3] =
{
    { // +x face
        {  0.0f,  0.0f, -1.0f, 0.0f }, // u -> -z
        {  0.0f, -1.0f,  0.0f, 0.0f }, // v -> -y
        {  1.0f,  0.0f,  0.0f, 0.0f }, // +x face
    },
    { // -x face
        {  0.0f,  0.0f,  1.0f, 0.0f }, // u -> +z
        {  0.0f, -1.0f,  0.0f, 0.0f }, // v -> -y
        { -1.0f,  0.0f,  0.0f, 0.0f }, // -x face
    },
    { // +y face
        {  1.0f,  0.0f,  0.0f, 0.0f }, // u -> +x
        {  0.0f,  0.0f,  1.0f, 0.0f }, // v -> +z
        {  0.0f,  1.0f,  0.0f, 0.0f }, // +y face
    },
    { // -y face
        {  1.0f,  0.0f,  0.0f, 0.0f }, // u -> +x
        {  0.0f,  0.0f, -1.0f, 0.0f }, // v -> -z
        {  0.0f, -1.0f,  0.0f, 0.0f }, // -y face
    },
    { // +z face
        {  1.0f,  0.0f,  0.0f, 0.0f }, // u -> +x
        {  0.0f, -1.0f,  0.0f, 0.0f }, // v -> -y
        {  0.0f,  0.0f,  1.0f, 0.0f }, // +z face
    },
    { // -z face
        { -1.0f,  0.0f,  0.0f, 0.0f }, // u -> -x
        {  0.0f, -1.0f,  0.0f, 0.0f }, // v -> -y
        {  0.0f,  0.0f, -1.0f, 0.0f }, // -z face
    }
};

static float4 texelCoordToVecWarp(float _u, float _v, int8_t _faceIdx, float _warp)
{
    #ifdef WARP_FIXUP
        _u = _warp*(_u*_u*_u) + _u;
        _v = _warp*(_v*_v*_v) + _v;
    #endif //WARP_FIXUP

    float4 aa = s_faceUvVectors[_faceIdx][0];
    float4 bb = s_faceUvVectors[_faceIdx][1];
    float4 cc = s_faceUvVectors[_faceIdx][2];
    return normalize(_u*aa + _v*bb + cc);
}

#if !CMFT_COMPUTE_FILTER_AREA_ON_CPU
    static void aabbAdd(float4* _aabb, float _x, float _y)
    {
        _aabb->x = fmin(_aabb->x, _x);
        _aabb->y = fmin(_aabb->y, _y);
        _aabb->z = fmax(_aabb->z, _x);
        _aabb->w = fmax(_aabb->w, _y);
    }

    static void aabbClamp(float4* _aabb, float _min, float _max)
    {
        _aabb->x = fmax(_aabb->x, _min);
        _aabb->y = fmax(_aabb->y, _min);
        _aabb->z = fmin(_aabb->z, _max);
        _aabb->w = fmin(_aabb->w, _max);
    }

    enum
    {
        CMFT_FACE_POS_X = 0,
        CMFT_FACE_NEG_X = 1,
        CMFT_FACE_POS_Y = 2,
        CMFT_FACE_NEG_Y = 3,
        CMFT_FACE_POS_Z = 4,
        CMFT_FACE_NEG_Z = 5,
    };

    enum
    {
        CMFT_EDGE_LEFT   = 0,
        CMFT_EDGE_RIGHT  = 1,
        CMFT_EDGE_TOP    = 2,
        CMFT_EDGE_BOTTOM = 3,
    };

    __constant struct CubeFaceNeighbour
    {
        uint8_t m_faceIdx;
        uint8_t m_faceEdge;
    } s_cubeFaceNeighbours[6][4] =
    {
        { //POS_X
            { CMFT_FACE_POS_Z, CMFT_EDGE_RIGHT },
            { CMFT_FACE_NEG_Z, CMFT_EDGE_LEFT  },
            { CMFT_FACE_POS_Y, CMFT_EDGE_RIGHT },
            { CMFT_FACE_NEG_Y, CMFT_EDGE_RIGHT },
        },
        { //NEG_X
            { CMFT_FACE_NEG_Z, CMFT_EDGE_RIGHT },
            { CMFT_FACE_POS_Z, CMFT_EDGE_LEFT  },
            { CMFT_FACE_POS_Y, CMFT_EDGE_LEFT  },
            { CMFT_FACE_NEG_Y, CMFT_EDGE_LEFT  },
        },
        { //POS_Y
            { CMFT_FACE_NEG_X, CMFT_EDGE_TOP },
            { CMFT_FACE_POS_X, CMFT_EDGE_TOP },
            { CMFT_FACE_NEG_Z, CMFT_EDGE_TOP },
            { CMFT_FACE_POS_Z, CMFT_EDGE_TOP },
        },
        { //NEG_Y
            { CMFT_FACE_NEG_X, CMFT_EDGE_BOTTOM },
            { CMFT_FACE_POS_X, CMFT_EDGE_BOTTOM },
            { CMFT_FACE_POS_Z, CMFT_EDGE_BOTTOM },
            { CMFT_FACE_NEG_Z, CMFT_EDGE_BOTTOM },
        },
        { //POS_Z
            { CMFT_FACE_NEG_X, CMFT_EDGE_RIGHT  },
            { CMFT_FACE_POS_X, CMFT_EDGE_LEFT   },
            { CMFT_FACE_POS_Y, CMFT_EDGE_BOTTOM },
            { CMFT_FACE_NEG_Y, CMFT_EDGE_TOP    },
        },
        { //NEG_Z
            { CMFT_FACE_POS_X, CMFT_EDGE_RIGHT  },
            { CMFT_FACE_NEG_X, CMFT_EDGE_LEFT   },
            { CMFT_FACE_POS_Y, CMFT_EDGE_TOP    },
            { CMFT_FACE_NEG_Y, CMFT_EDGE_BOTTOM },
        }
    };

    static void vecToTexelCoord(float* _u, float* _v, uint8_t* _faceIdx, float4 _vec)
    {
        float4 absVec = fabs(_vec);
        float max = fmax(fmax(absVec.x, absVec.y), absVec.z);

        // Get face id (max component == face vector).
        if (max == absVec.x)
        {
            *_faceIdx = (_vec.x >= 0.0f) ? (uint8_t)CMFT_FACE_POS_X : (uint8_t)CMFT_FACE_NEG_X;
        }
        else if (max == absVec.y)
        {
            *_faceIdx = (_vec.y >= 0.0f) ? (uint8_t)CMFT_FACE_POS_Y : (uint8_t)CMFT_FACE_NEG_Y;
        }
        else //if (max == absVec.z)
        {
            *_faceIdx = (_vec.z >= 0.0f) ? (uint8_t)CMFT_FACE_POS_Z : (uint8_t)CMFT_FACE_NEG_Z;
        }

        // Divide by max component.
        float4 faceVec = _vec/max;

        *_u = (dot(s_faceUvVectors[*_faceIdx][0], faceVec) + 1.0f) * 0.5f;
        *_v = (dot(s_faceUvVectors[*_faceIdx][1], faceVec) + 1.0f) * 0.5f;
    }

    static void determineFilterArea(float4* _filterArea, float4 _tapVec, float _filterSize)
    {
        float uu, vv;
        uint8_t hitFaceIdx;
        vecToTexelCoord(&uu, &vv, &hitFaceIdx, _tapVec);

        float4 bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
        aabbAdd(&bounds, uu-_filterSize, vv-_filterSize);
        aabbAdd(&bounds, uu+_filterSize, vv+_filterSize);
        aabbClamp(&bounds, 0.0f, 1.0f);

        _filterArea[hitFaceIdx] = bounds;

        enum NeighbourSides
        {
            Left,
            Right,
            Top,
            Bottom,

            Count,
        };

        float3 bleed[Count] = /*amount, bbmin, bbmax*/
        {
            { // Left
                _filterSize - uu,
                bounds.y,
                bounds.w,
            },
            { // Right
                uu + _filterSize - 1.0f,
                bounds.y,
                bounds.w,
            },
            { // Top
                _filterSize - vv,
                bounds.x,
                bounds.z,
            },
            { // Bottom
                vv + _filterSize - 1.0f,
                bounds.x,
                bounds.z,
            },
        };

        // Determine bleeding for each side.
        for (uint8_t side = 0; side < 4; ++side)
        {
            uint8_t currentFaceIdx = hitFaceIdx;

            for (float bleedAmount = bleed[side].x; bleedAmount > 0.0f; bleedAmount -= 1.0f)
            {
                uint8_t neighbourFaceIdx  = s_cubeFaceNeighbours[currentFaceIdx][side].m_faceIdx;
                uint8_t neighbourFaceEdge = s_cubeFaceNeighbours[currentFaceIdx][side].m_faceEdge;
                currentFaceIdx = neighbourFaceIdx;

                float bbMin = bleed[side].y;
                float bbMax = bleed[side].z;
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
                        aabbAdd(&_filterArea[neighbourFaceIdx], 0.0f, bbMin);
                        aabbAdd(&_filterArea[neighbourFaceIdx], bleedAmount, bbMax);
                    }
                break;

                case CMFT_EDGE_RIGHT:
                    {
                        aabbAdd(&_filterArea[neighbourFaceIdx], 1.0f - bleedAmount, bbMin);
                        aabbAdd(&_filterArea[neighbourFaceIdx], 1.0f, bbMax);
                    }
                break;

                case CMFT_EDGE_TOP:
                    {
                        aabbAdd(&_filterArea[neighbourFaceIdx], bbMin, 0.0f);
                        aabbAdd(&_filterArea[neighbourFaceIdx], bbMax, bleedAmount);
                    }
                break;

                case CMFT_EDGE_BOTTOM:
                    {
                        aabbAdd(&_filterArea[neighbourFaceIdx], bbMin, 1.0f - bleedAmount);
                        aabbAdd(&_filterArea[neighbourFaceIdx], bbMax, 1.0f);
                    }
                break;
                }

                // Clamp bounding box to face size.
                aabbClamp(&_filterArea[neighbourFaceIdx], 0.0f, 1.0f);
            }
        }
    }
#endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

__kernel void radianceFilterSingleFace(__write_only image2d_t _out
                                     , __read_only  image2d_t _srcData
                                     , __read_only  image2d_t _normalSolidAngle
                                     , int8_t _dstFaceId
                                     , int32_t _dstFaceSize
                                     , float _specularPower
                                     , float _specularAngle
                                     , float _filterSize
                                     , float _warp
                                     , int8_t _srcFaceIdx
                                     , float _srcFaceSize
                                     #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                                     , __read_only image2d_t _area
                                     #endif //!CMFT_COMPUTE_FILTER_AREA_ON_CPU
                                     )
{

    const int xx = get_global_id(1);
    const int yy = get_global_id(0);

    float4 colorWeight = { 0.0f, 0.0f, 0.0f, 0.0f };

    const float invDstFaceSize_Mul2 = 2.0f/_dstFaceSize;
    const float vv = ((float)yy + 0.5f)*invDstFaceSize_Mul2 - 1.0f;
    const float uu = ((float)xx + 0.5f)*invDstFaceSize_Mul2 - 1.0f;
    const float4 tapVec = texelCoordToVecWarp(uu, vv, _srcFaceIdx, _warp);

#if CMFT_COMPUTE_FILTER_AREA_ON_CPU
    const int2 areaCoord = { xx*6 + _dstFaceId, yy };
    const float4 area = read_imagef(_area, s_sampler, areaCoord);
#else
    float4 filterArea[6] =
    {
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    determineFilterArea(filterArea, tapVec, _filterSize);
    const float4 area = filterArea[_dstFaceId];
#endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

    const int4 minmax = convert_int4(area*_srcFaceSize);
    const int32_t minX = minmax.x;
    const int32_t minY = minmax.y;
    const int32_t maxX = minmax.z;
    const int32_t maxY = minmax.w;

    for (int32_t yy = minY; yy < maxY; ++yy)
    {
        for (int32_t xx = minX; xx < maxX; ++xx)
        {
            const int2 coord = { xx, yy };
            const float4 normal = read_imagef(_normalSolidAngle, s_sampler, coord);
            const float dp = dot(normal, tapVec);
            if (dp >= _specularAngle)
            {
                float4 sample = read_imagef(_srcData, s_sampler, coord);
                sample.w = 1.0f;
                colorWeight += sample
                             * normal.w
                             * native_powr(dp, _specularPower);
            }
        }
    }

    const int2 dst = { xx, yy };
    write_imagef(_out, dst, colorWeight);
}

__kernel void sum(__write_only image2d_t _out
                , __read_only image2d_t _src0
                , __read_only image2d_t _src1
                , __read_only image2d_t _src2
                , __read_only image2d_t _src3
                , __read_only image2d_t _src4
                , __read_only image2d_t _src5
                )
{
    const int xx = get_global_id(1);
    const int yy = get_global_id(0);
    const int2 coord = { xx, yy };

    float4 color = read_imagef(_src0, s_sampler, coord)
                 + read_imagef(_src1, s_sampler, coord)
                 + read_imagef(_src2, s_sampler, coord)
                 + read_imagef(_src3, s_sampler, coord)
                 + read_imagef(_src4, s_sampler, coord)
                 + read_imagef(_src5, s_sampler, coord)
                 ;

    if (0.0 != color.w)
    {
        color /= color.w;
    }

    write_imagef(_out, coord, color);
}

__kernel void radianceFilter(__write_only image2d_t _out
                           , int32_t _dstFaceSize
                           , float _specularPower
                           , float _specularAngle
                           , float _filterSize
                           , float _warp
                           , int8_t _srcFaceIdx
                           , float _srcFaceSize
                           , __read_only image2d_t _srcData0
                           , __read_only image2d_t _srcData1
                           , __read_only image2d_t _srcData2
                           , __read_only image2d_t _srcData3
                           , __read_only image2d_t _srcData4
                           , __read_only image2d_t _srcData5
                           , __read_only image2d_t _normalSolidAngle0
                           , __read_only image2d_t _normalSolidAngle1
                           , __read_only image2d_t _normalSolidAngle2
                           , __read_only image2d_t _normalSolidAngle3
                           , __read_only image2d_t _normalSolidAngle4
                           , __read_only image2d_t _normalSolidAngle5
                           #if CMFT_COMPUTE_FILTER_AREA_ON_CPU
                           , __read_only image2d_t _area
                           #endif //!CMFT_COMPUTE_FILTER_AREA_ON_CPU
                           )
{
    const int xx = get_global_id(1);
    const int yy = get_global_id(0);

    float4 colorWeight = { 0.0f, 0.0f, 0.0f, 0.0f };

    const float invDstFaceSize_Mul2 = 2.0f/_dstFaceSize;
    const float vv = ((float)yy + 0.5f)*invDstFaceSize_Mul2 - 1.0f;
    const float uu = ((float)xx + 0.5f)*invDstFaceSize_Mul2 - 1.0f;
    const float4 tapVec = texelCoordToVecWarp(uu, vv, _srcFaceIdx, _warp);

#if CMFT_COMPUTE_FILTER_AREA_ON_CPU
    #define FLOAT4_AREA(_face) const int2 areaCoord = { xx*6 + _face, yy }; const float4 area = read_imagef(_area, s_sampler, areaCoord);
#else
    float4 filterArea[6] =
    {
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    determineFilterArea(filterArea, tapVec, _filterSize);
    #define FLOAT4_AREA(_face) const float4 area = filterArea[_face];
#endif //CMFT_COMPUTE_FILTER_AREA_ON_CPU

    #define PROCESS_FACE(_ii)                                                                  \
    {                                                                                          \
        FLOAT4_AREA(_ii)                                                                       \
                                                                                               \
        const int4 minmax = convert_int4(area*_srcFaceSize);                                   \
        const int32_t minX = minmax.x;                                                         \
        const int32_t minY = minmax.y;                                                         \
        const int32_t maxX = minmax.z;                                                         \
        const int32_t maxY = minmax.w;                                                         \
                                                                                               \
        for (int32_t yy = minY; yy < maxY; ++yy)                                               \
        {                                                                                      \
            for (int32_t xx = minX; xx < maxX; ++xx)                                           \
            {                                                                                  \
                const int2 coord = { xx, yy };                                                 \
                const float4 normal = read_imagef(_normalSolidAngle ## _ii, s_sampler, coord); \
                const float dp = dot(normal, tapVec);                                          \
                if (dp >= _specularAngle)                                                      \
                {                                                                              \
                    float4 sample = read_imagef(_srcData ## _ii, s_sampler, coord);            \
                    sample.w = 1.0f;                                                           \
                    colorWeight += sample                                                      \
                                 * normal.w                                                    \
                                 * native_powr(dp, _specularPower);                            \
                }                                                                              \
            }                                                                                  \
        }                                                                                      \
    }

    PROCESS_FACE(0);
    PROCESS_FACE(1);
    PROCESS_FACE(2);
    PROCESS_FACE(3);
    PROCESS_FACE(4);
    PROCESS_FACE(5);

    if (0.0 != colorWeight.w)
    {
        colorWeight /= colorWeight.w;
    }

    const int2 dst = { xx, yy };
    write_imagef(_out, dst, colorWeight);
}

/* vim: set sw=4 ts=4 expandtab: */
