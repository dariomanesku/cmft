/*
 * Copyright 2011-2014 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

// FPU math lib

#ifndef FPU_MATH_H_HEADER_GUARD
#define FPU_MATH_H_HEADER_GUARD

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(_MSC_VER)
inline float fminf(float _a, float _b)
{
	return _a < _b ? _a : _b;
}

inline float fmaxf(float _a, float _b)
{
	return _a > _b ? _a : _b;
}
#endif // BX_COMPILER_MSVC

inline float toRad(float _deg)
{
	return _deg * float(M_PI / 180.0);
}

inline float toDeg(float _rad)
{
	return _rad * float(180.0 / M_PI);
}

inline float fclamp(float _a, float _min, float _max)
{
	return fminf(fmaxf(_a, _min), _max);
}

inline float fsaturate(float _a)
{
	return fclamp(_a, 0.0f, 1.0f);
}

inline float flerp(float _a, float _b, float _t)
{
	return _a + (_b - _a) * _t;
}

inline float fsign(float _a)
{
	return _a < 0.0f ? -1.0f : 1.0f;
}

inline void vec3Move(float* __restrict _result, const float* __restrict _a)
{
	_result[0] = _a[0];
	_result[1] = _a[1];
	_result[2] = _a[2];
}

inline void vec3Abs(float* __restrict _result, const float* __restrict _a)
{
	_result[0] = fabsf(_a[0]);
	_result[1] = fabsf(_a[1]);
	_result[2] = fabsf(_a[2]);
}

inline void vec3Neg(float* __restrict _result, const float* __restrict _a)
{
	_result[0] = -_a[0];
	_result[1] = -_a[1];
	_result[2] = -_a[2];
}

inline void vec3Add(float* __restrict _result, const float* __restrict _a, const float* __restrict _b)
{
	_result[0] = _a[0] + _b[0];
	_result[1] = _a[1] + _b[1];
	_result[2] = _a[2] + _b[2];
}

inline void vec3Sub(float* __restrict _result, const float* __restrict _a, const float* __restrict _b)
{
	_result[0] = _a[0] - _b[0];
	_result[1] = _a[1] - _b[1];
	_result[2] = _a[2] - _b[2];
}

inline void vec3Mul(float* __restrict _result, const float* __restrict _a, const float* __restrict _b)
{
	_result[0] = _a[0] * _b[0];
	_result[1] = _a[1] * _b[1];
	_result[2] = _a[2] * _b[2];
}

inline void vec3Mul(float* __restrict _result, const float* __restrict _a, float _b)
{
	_result[0] = _a[0] * _b;
	_result[1] = _a[1] * _b;
	_result[2] = _a[2] * _b;
}

inline void vec4Mul(float* __restrict _result, const float* __restrict _a, float _b)
{
	_result[0] = _a[0] * _b;
	_result[1] = _a[1] * _b;
	_result[2] = _a[2] * _b;
	_result[3] = _a[3] * _b;
}

inline float vec3Dot(const float* __restrict _a, const float* __restrict _b)
{
	return _a[0]*_b[0] + _a[1]*_b[1] + _a[2]*_b[2];
}

inline void vec3Cross(float* __restrict _result, const float* __restrict _a, const float* __restrict _b)
{
	_result[0] = _a[1]*_b[2] - _a[2]*_b[1];
	_result[1] = _a[2]*_b[0] - _a[0]*_b[2];
	_result[2] = _a[0]*_b[1] - _a[1]*_b[0];
}

inline float vec3Length(const float* _a)
{
	return sqrtf(vec3Dot(_a, _a) );
}

inline float vec3Norm(float* __restrict _result, const float* __restrict _a)
{
	const float len = vec3Length(_a);
	const float invLen = 1.0f/len;
	_result[0] = _a[0] * invLen;
	_result[1] = _a[1] * invLen;
	_result[2] = _a[2] * invLen;
	return len;
}

#endif // FPU_MATH_H_HEADER_GUARD
