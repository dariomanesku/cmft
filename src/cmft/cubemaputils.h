/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CUBEMAPUTILS_H_HEADER_GUARD
#define CMFT_CUBEMAPUTILS_H_HEADER_GUARD

#include "common/fpumath.h"

namespace cmft
{
    #define CMFT_PI       3.14159265358979323846f
    #define CMFT_RPI      0.31830988618379067153f
    #define CMFT_2PI      6.28318530717958647692f
    #define CMFT_DEGTORAD 0.01745329251994329576f
    #define CMFT_RADTODEG 57.2957795130823208767f

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
            { CMFT_FACE_NEG_X, CMFT_EDGE_LEFT   },
            { CMFT_FACE_POS_Y, CMFT_EDGE_TOP    },
            { CMFT_FACE_NEG_Y, CMFT_EDGE_BOTTOM },
        }
    };

    /// _u and _v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
    static inline void texelCoordToVec(float* _out3f, float _u, float _v, uint8_t _faceId)
    {
        // out = u * s_faceUv[0] + v * s_faceUv[1] + s_faceUv[2].
        _out3f[0] = s_faceUvVectors[_faceId][0][0] * _u + s_faceUvVectors[_faceId][1][0] * _v + s_faceUvVectors[_faceId][2][0];
        _out3f[1] = s_faceUvVectors[_faceId][0][1] * _u + s_faceUvVectors[_faceId][1][1] * _v + s_faceUvVectors[_faceId][2][1];
        _out3f[2] = s_faceUvVectors[_faceId][0][2] * _u + s_faceUvVectors[_faceId][1][2] * _v + s_faceUvVectors[_faceId][2][2];

        // Normalize.
        const float invLen = 1.0f/sqrtf(_out3f[0]*_out3f[0] + _out3f[1]*_out3f[1] + _out3f[2]*_out3f[2]);
        _out3f[0] *= invLen;
        _out3f[1] *= invLen;
        _out3f[2] *= invLen;
    }

    /// Notice: _faceSize should not be equal to one!
    static inline float warpFixupFactor(float _faceSize)
    {
        // Edge fixup.
        // Based on Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/CubeSurface.cpp
        if (_faceSize == 1.0f)
        {
            return 1.0f;
        }

        const float fs = _faceSize;
        const float fsmo = fs - 1.0f;
        return (fs*fs) / (fsmo*fsmo*fsmo);
    }

    /// _u and _v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
    static inline void texelCoordToVecWarp(float* _out3f, float _u, float _v, uint8_t _faceId, float _warpFixup)
    {
        _u = (_warpFixup * _u*_u*_u) + _u;
        _v = (_warpFixup * _v*_v*_v) + _v;

        texelCoordToVec(_out3f, _u, _v, _faceId);
    }

    /// _u and _v are in [0.0 .. 1.0] range.
    static inline void vecToTexelCoord(float& _u, float& _v, uint8_t& _faceIdx, const float* _vec)
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
            _faceIdx = (_vec[0] >= 0.0f) ? uint8_t(CMFT_FACE_POS_X) : uint8_t(CMFT_FACE_NEG_X);
        }
        else if (max == absVec[1])
        {
            _faceIdx = (_vec[1] >= 0.0f) ? uint8_t(CMFT_FACE_POS_Y) : uint8_t(CMFT_FACE_NEG_Y);
        }
        else //if (max == absVec[2])
        {
            _faceIdx = (_vec[2] >= 0.0f) ? uint8_t(CMFT_FACE_POS_Z) : uint8_t(CMFT_FACE_NEG_Z);
        }

        // Divide by max component.
        float faceVec[3];
        vec3Mul(faceVec, _vec, 1.0f/max);

        // Project other two components to face uv basis.
        _u = (vec3Dot(s_faceUvVectors[_faceIdx][0], faceVec) + 1.0f) * 0.5f;
        _v = (vec3Dot(s_faceUvVectors[_faceIdx][1], faceVec) + 1.0f) * 0.5f;
    }

    static inline void latLongFromVec(float& _u, float& _v, const float _vec[3])
    {
        const float phi = atan2f(_vec[0], _vec[2]);
        const float theta = acosf(_vec[1]);

        _u = (CMFT_PI + phi)*(0.5f/CMFT_PI);
        _v = theta*CMFT_RPI;
    }

    static inline void vecFromLatLong(float _vec[3], float _u, float _v)
    {
        const float phi   = _u * CMFT_2PI;
        const float theta = _v * CMFT_PI;

        _vec[0] = -sinf(theta)*sinf(phi);
        _vec[1] = cosf(theta);
        _vec[2] = -sinf(theta)*cosf(phi);
    }

    // Assume normalized _vec.
    // Output is on [0, 1] for each component
    static inline void octantFromVec(float& _u, float& _v, const float _vec[3])
    {
        // Project the sphere onto the octahedron, and then onto the xy plane.
        float dot = fabsf(_vec[0]) + fabsf(_vec[1]) + fabsf(_vec[2]);
        float px = _vec[0] / dot;
        float py = _vec[2] / dot;

        // Reflect the folds of the lower hemisphere over the diagonals.
        if (_vec[1] <= 0.0f)
        {
            _u = ((1.0f - fabsf(py)) * fsign(px));
            _v = ((1.0f - fabsf(px)) * fsign(py));
        }
        else
        {
            _u = px;
            _v = py;
        }

        _u = _u * 0.5f + 0.5f;
        _v = _v * 0.5f + 0.5f;
    }

    static inline void vecFromOctant(float _vec[3], float _u, float _v)
    {
        _u = _u*2.0f - 1.0f;
        _v = _v*2.0f - 1.0f;

        _vec[1] = 1.0f - fabsf(_u) - fabsf(_v);

        if (_vec[1] < 0.0f)
        {
            _vec[0] = (1.0f - fabsf(_v)) * fsign(_u);
            _vec[2] = (1.0f - fabsf(_u)) * fsign(_v);
        }
        else
        {
            _vec[0] = _u;
            _vec[2] = _v;
        }

        const float invLen = 1.0f/vec3Length(_vec);
        _vec[0] *= invLen;
        _vec[1] *= invLen;
        _vec[2] *= invLen;
    }

    /// http://www.mpia-hd.mpg.de/~mathar/public/mathar20051002.pdf
    /// http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
    static inline float areaElement(float _x, float _y)
    {
        return atan2f(_x*_y, sqrtf(_x*_x + _y*_y + 1.0f));
    }

    /// _u and _v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
    static inline float texelSolidAngle(float _u, float _v, float _invFaceSize)
    {
        // Specify texel area.
        const float x0 = _u - _invFaceSize;
        const float x1 = _u + _invFaceSize;
        const float y0 = _v - _invFaceSize;
        const float y1 = _v + _invFaceSize;

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
