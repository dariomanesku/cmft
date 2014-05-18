/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CUBEMAPUTILS_H_HEADER_GUARD
#define CMFT_CUBEMAPUTILS_H_HEADER_GUARD

#include "fpumath.h"

namespace cmft
{
    ///
    ///
    ///              +----------+
    ///              | +---->+x |
    ///              | |        |
    ///              | |  +y    |
    ///              |+z      2 |
    ///   +----------+----------+----------+----------+
    ///   | +---->+z | +---->+x | +---->-z | +---->-x |
    ///   | |        | |        | |        | |        |
    ///   | |  -x    | |  +z    | |  +x    | |  -z    |
    ///   |-y      1 |-y      4 |-y      0 |-y      5 |
    ///   +----------+----------+----------+----------+
    ///              | +---->+x |
    ///              | |        |
    ///              | |  -y    |
    ///              |-z      3 |
    ///              +----------+
    ///
    static const float s_faceUvVectors[6][3][3] =
    {
        { // +x face
            {  0.0f,  0.0f, -1.0f }, // u -> -z
            {  0.0f, -1.0f,  0.0f }, // v -> -y
            {  1.0f,  0.0f,  0.0f }, // +x face
        },
        { // -x face
            {  0.0f,  0.0f,  1.0f }, // u -> +z
            {  0.0f, -1.0f,  0.0f }, // v -> -y
            { -1.0f,  0.0f,  0.0f }, // -x face
        },
        { // +y face
            {  1.0f,  0.0f,  0.0f }, // u -> +x
            {  0.0f,  0.0f,  1.0f }, // v -> +z
            {  0.0f,  1.0f,  0.0f }, // +y face
        },
        { // -y face
            {  1.0f,  0.0f,  0.0f }, // u -> +x
            {  0.0f,  0.0f, -1.0f }, // v -> -z
            {  0.0f, -1.0f,  0.0f }, // -y face
        },
        { // +z face
            {  1.0f,  0.0f,  0.0f }, // u -> +x
            {  0.0f, -1.0f,  0.0f }, // v -> -y
            {  0.0f,  0.0f,  1.0f }, // +z face
        },
        { // -z face
            { -1.0f,  0.0f,  0.0f }, // u -> -x
            {  0.0f, -1.0f,  0.0f }, // v -> -y
            {  0.0f,  0.0f, -1.0f }, // -z face
        }
    };

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

    ///
    ///    --> U    _____
    ///   |        |     |
    ///   v        | +Y  |
    ///   V   _____|_____|_____ _____
    ///      |     |     |     |     |
    ///      | -X  | +Z  | +X  | -Z  |
    ///      |_____|_____|_____|_____|
    ///            |     |
    ///            | -Y  |
    ///            |_____|
    ///
    /// Neighbour faces in order: left, right, top, bottom.
    /// FaceEdge is the edge that belongs to the neighbour face.
    static const struct CubeFaceNeighbour
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
            { CMFT_FACE_POS_X, CMFT_EDGE_LEFT   },
            { CMFT_FACE_POS_Y, CMFT_EDGE_TOP    },
            { CMFT_FACE_POS_Y, CMFT_EDGE_BOTTOM },
        }
    };

    static void texelCoordToVec(float* _out3f, float _u, float _v, uint8_t _faceId, uint32_t _faceSize = 1)
    {
        if (1 != _faceSize)
        {
            // Edge fixup.
            // Code from Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/CubeSurface.cpp
            const float a = float(_faceSize*_faceSize) / powf(float(_faceSize - 1), 3.0f);
            _u = a * powf(_u, 3.0f) + _u;
            _v = a * powf(_v, 3.0f) + _v;
        }

        // This does: Dst = normalize(u * s_faceUv[0] + v * s_faceUv[1] + s_faceUv[2]).
        float tmp0[3];
        float tmp1[3];
        float tmp2[3];
        vec3Mul(tmp0, s_faceUvVectors[_faceId][0], _u);
        vec3Mul(tmp1, s_faceUvVectors[_faceId][1], _v);
        vec3Add(tmp2, tmp0, tmp1);
        vec3Add(tmp0, tmp2, s_faceUvVectors[_faceId][2]);
        vec3Norm(_out3f, tmp0);
    }

    static void vecToTexelCoord(float& _u, float& _v, uint8_t& _faceIdx, const float* _vec)
    {
        const float absVec[3] =
        {
            fabsf(_vec[0]),
            fabsf(_vec[1]),
            fabsf(_vec[2]),
        };
        const float max = fmaxf(fmaxf(absVec[0], absVec[1]), absVec[2]);

        // Get face id (max component == face vector).
        if (max == absVec[0])
        {
            _faceIdx = (_vec[0] > 0) ? uint8_t(CMFT_FACE_POS_X) : uint8_t(CMFT_FACE_NEG_X);
        }
        else if (max == absVec[1])
        {
            _faceIdx = (_vec[1] > 0) ? uint8_t(CMFT_FACE_POS_Y) : uint8_t(CMFT_FACE_NEG_Y);
        }
        else //if (max == absVec[2])
        {
            _faceIdx = (_vec[2] > 0) ? uint8_t(CMFT_FACE_POS_Z) : uint8_t(CMFT_FACE_NEG_Z);
        }

        // Divide by max component.
        float faceVec[3];
        vec3Mul(faceVec, _vec, 1.0f/max);

        // Project other two components to face uv basis.
        _u = (vec3Dot(s_faceUvVectors[_faceIdx][0], faceVec) + 1.0f) * 0.5f;
        _v = (vec3Dot(s_faceUvVectors[_faceIdx][1], faceVec) + 1.0f) * 0.5f;
    }

    /// For right-handed coordinate system.
    /// _x, _y are in [0.0-1.0] range.
    static void latLongFromVec(float& _x, float& _y, const float _vec[3])
    {
        const float theta = acosf(_vec[1]);
        const float phi = atan2f(_vec[2], -_vec[0]);

        const float halfInvPi = 0.15915494309f;
        const float invPi = 0.31830988618f;
        _x = (float(M_PI)+phi)*halfInvPi;
        _y = theta*invPi;
    }

    /// For right-handed coordinate system.
    /// _x, _y are in [0.0-1.0] range.
    static void vecFromLatLong(float _vec[3], float _x, float _y)
    {
        const float theta = _y * float(M_PI);
        const float phi = _x * float(M_PI)*2.0f;

        _vec[0] = sinf(theta)*cosf(phi);
        _vec[1] = cosf(theta);
        _vec[2] = -sinf(theta)*sinf(phi);
    }

    /// http://www.mpia-hd.mpg.de/~mathar/public/mathar20051002.pdf
    /// http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
    static float areaElement(float _x, float _y)
    {
        return atan2f(_x*_y, sqrt(_x*_x + _y*_y + 1*1));
    }

    static float texelSolidAngle(float _u, float _v, float _halfTexelSize)
    {
        // Specify texel area.
        const float x0 = _u - _halfTexelSize;
        const float x1 = _u + _halfTexelSize;
        const float y0 = _v - _halfTexelSize;
        const float y1 = _v + _halfTexelSize;

        // Compute solid angle of texel area.
        const float solidAngle = areaElement(x1, y1)
                               - areaElement(x0, y1)
                               - areaElement(x1, y0)
                               + areaElement(x0, y0)
                               ;

        return solidAngle;
    }

} // namespace cmft

#endif //CMFT_CUBEMAPUTILS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
