/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

/*
 * Copyright 2010-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

// Copyright 2006 Mike Acton <macton@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE

#ifndef CMFT_HALFLOAT_H_HEADER_GUARD
#define CMFT_HALFLOAT_H_HEADER_GUARD

#include "platform.h"
#include <stdint.h>

#if CMFT_COMPILER_MSVC
#   if CMFT_PLATFORM_WINDOWS
#       include <math.h> // math.h is included because VS bitches:
                         // warning C4985: 'ceil': attributes not present on previous declaration.
                         // must be included before intrin.h.
#       include <intrin.h>
#       pragma intrinsic(_BitScanForward)
#       pragma intrinsic(_BitScanReverse)
#       if CMFT_ARCH_64BIT
#           pragma intrinsic(_BitScanForward64)
#           pragma intrinsic(_BitScanReverse64)
#       endif // CMFT_ARCH_64BIT
#   endif // CMFT_PLATFORM_WINDOWS
#endif // CMFT_COMPILER_MSVC

#define CMFT_HALF_FLOAT_ZERO UINT16_C(0)
#define CMFT_HALF_FLOAT_HALF UINT16_C(0x3800)
#define CMFT_HALF_FLOAT_ONE  UINT16_C(0x3c00)
#define CMFT_HALF_FLOAT_TWO  UINT16_C(0x4000)

namespace cmft
{
    inline uint32_t uint32_li(uint32_t _a)
    {
        return _a;
    }

    inline uint32_t uint32_dec(uint32_t _a)
    {
        return _a - 1;
    }

    inline uint32_t uint32_inc(uint32_t _a)
    {
        return _a + 1;
    }

    inline uint32_t uint32_not(uint32_t _a)
    {
        return ~_a;
    }

    inline uint32_t uint32_neg(uint32_t _a)
    {
        return -(int32_t)_a;
    }

    inline uint32_t uint32_ext(uint32_t _a)
    {
        return ( (int32_t)_a)>>31;
    }

    inline uint32_t uint32_and(uint32_t _a, uint32_t _b)
    {
        return _a & _b;
    }

    inline uint32_t uint32_andc(uint32_t _a, uint32_t _b)
    {
        return _a & ~_b;
    }

    inline uint32_t uint32_xor(uint32_t _a, uint32_t _b)
    {
        return _a ^ _b;
    }

    inline uint32_t uint32_xorl(uint32_t _a, uint32_t _b)
    {
        return !_a != !_b;
    }

    inline uint32_t uint32_or(uint32_t _a, uint32_t _b)
    {
        return _a | _b;
    }

    inline uint32_t uint32_orc(uint32_t _a, uint32_t _b)
    {
        return _a | ~_b;
    }

    inline uint32_t uint32_sll(uint32_t _a, int _sa)
    {
        return _a << _sa;
    }

    inline uint32_t uint32_srl(uint32_t _a, int _sa)
    {
        return _a >> _sa;
    }

    inline uint32_t uint32_sra(uint32_t _a, int _sa)
    {
        return ( (int32_t)_a) >> _sa;
    }

    inline uint32_t uint32_rol(uint32_t _a, int _sa)
    {
        return ( _a << _sa) | (_a >> (32-_sa) );
    }

    inline uint32_t uint32_ror(uint32_t _a, int _sa)
    {
        return ( _a >> _sa) | (_a << (32-_sa) );
    }

    inline uint32_t uint32_add(uint32_t _a, uint32_t _b)
    {
        return _a + _b;
    }

    inline uint32_t uint32_sub(uint32_t _a, uint32_t _b)
    {
        return _a - _b;
    }

    inline uint32_t uint32_mul(uint32_t _a, uint32_t _b)
    {
        return _a * _b;
    }

    inline uint32_t uint32_div(uint32_t _a, uint32_t _b)
    {
        return (_a / _b);
    }

    inline uint32_t uint32_mod(uint32_t _a, uint32_t _b)
    {
        return (_a % _b);
    }

    inline uint32_t uint32_cmpeq(uint32_t _a, uint32_t _b)
    {
        return -(_a == _b);
    }

    inline uint32_t uint32_cmpneq(uint32_t _a, uint32_t _b)
    {
        return -(_a != _b);
    }

    inline uint32_t uint32_cmplt(uint32_t _a, uint32_t _b)
    {
        return -(_a < _b);
    }

    inline uint32_t uint32_cmple(uint32_t _a, uint32_t _b)
    {
        return -(_a <= _b);
    }

    inline uint32_t uint32_cmpgt(uint32_t _a, uint32_t _b)
    {
        return -(_a > _b);
    }

    inline uint32_t uint32_cmpge(uint32_t _a, uint32_t _b)
    {
        return -(_a >= _b);
    }

    inline uint32_t uint32_setnz(uint32_t _a)
    {
        return -!!_a;
    }

    inline uint32_t uint32_satadd(uint32_t _a, uint32_t _b)
    {
        const uint32_t add    = uint32_add(_a, _b);
        const uint32_t lt     = uint32_cmplt(add, _a);
        const uint32_t result = uint32_or(add, lt);

        return result;
    }

    inline uint32_t uint32_satsub(uint32_t _a, uint32_t _b)
    {
        const uint32_t sub    = uint32_sub(_a, _b);
        const uint32_t le     = uint32_cmple(sub, _a);
        const uint32_t result = uint32_and(sub, le);

        return result;
    }

    inline uint32_t uint32_satmul(uint32_t _a, uint32_t _b)
    {
        const uint64_t mul    = (uint64_t)_a * (uint64_t)_b;
        const uint32_t hi     = mul >> 32;
        const uint32_t nz     = uint32_setnz(hi);
        const uint32_t result = uint32_or(uint32_t(mul), nz);

        return result;
    }

    inline uint32_t uint32_sels(uint32_t test, uint32_t _a, uint32_t _b)
    {
        const uint32_t mask   = uint32_ext(test);
        const uint32_t sel_a  = uint32_and(_a, mask);
        const uint32_t sel_b  = uint32_andc(_b, mask);
        const uint32_t result = uint32_or(sel_a, sel_b);

        return (result);
    }

    inline uint32_t uint32_selb(uint32_t _mask, uint32_t _a, uint32_t _b)
    {
        const uint32_t sel_a  = uint32_and(_a, _mask);
        const uint32_t sel_b  = uint32_andc(_b, _mask);
        const uint32_t result = uint32_or(sel_a, sel_b);

        return (result);
    }

    inline uint32_t uint32_imin(uint32_t _a, uint32_t _b)
    {
        const uint32_t a_sub_b = uint32_sub(_a, _b);
        const uint32_t result  = uint32_sels(a_sub_b, _a, _b);

        return result;
    }

    inline uint32_t uint32_imax(uint32_t _a, uint32_t _b)
    {
        const uint32_t b_sub_a = uint32_sub(_b, _a);
        const uint32_t result  = uint32_sels(b_sub_a, _a, _b);

        return result;
    }

    inline uint32_t uint32_min(uint32_t _a, uint32_t _b)
    {
        return _a > _b ? _b : _a;
    }

    inline uint32_t uint32_min(uint32_t _a, uint32_t _b, uint32_t _c)
    {
        return uint32_min(_a, uint32_min(_b, _c) );
    }

    inline uint32_t uint32_max(uint32_t _a, uint32_t _b)
    {
        return _a > _b ? _a : _b;
    }

    inline uint32_t uint32_max(uint32_t _a, uint32_t _b, uint32_t _c)
    {
        return uint32_max(_a, uint32_max(_b, _c) );
    }

    inline uint32_t uint32_clamp(uint32_t _a, uint32_t _min, uint32_t _max)
    {
        const uint32_t tmp    = uint32_max(_a, _min);
        const uint32_t result = uint32_min(tmp, _max);

        return result;
    }

    inline uint32_t uint32_iclamp(uint32_t _a, uint32_t _min, uint32_t _max)
    {
        const uint32_t tmp    = uint32_imax(_a, _min);
        const uint32_t result = uint32_imin(tmp, _max);

        return result;
    }

    inline uint32_t uint32_incwrap(uint32_t _val, uint32_t _min, uint32_t _max)
    {
        const uint32_t inc          = uint32_inc(_val);
        const uint32_t max_diff     = uint32_sub(_max, _val);
        const uint32_t neg_max_diff = uint32_neg(max_diff);
        const uint32_t max_or       = uint32_or(max_diff, neg_max_diff);
        const uint32_t max_diff_nz  = uint32_ext(max_or);
        const uint32_t result       = uint32_selb(max_diff_nz, inc, _min);

        return result;
    }

    inline uint32_t uint32_decwrap(uint32_t _val, uint32_t _min, uint32_t _max)
    {
        const uint32_t dec          = uint32_dec(_val);
        const uint32_t min_diff     = uint32_sub(_min, _val);
        const uint32_t neg_min_diff = uint32_neg(min_diff);
        const uint32_t min_or       = uint32_or(min_diff, neg_min_diff);
        const uint32_t min_diff_nz  = uint32_ext(min_or);
        const uint32_t result       = uint32_selb(min_diff_nz, dec, _max);

        return result;
    }

    /// Count number of bits set.
    inline uint32_t uint32_cntbits(uint32_t _val)
    {
    #if CMFT_COMPILER_GCC || CMFT_COMPILER_CLANG
        return __builtin_popcount(_val);
    #elif CMFT_COMPILER_MSVC && CMFT_PLATFORM_WINDOWS
        return __popcnt(_val);
    #else
        return uint32_cntbits_ref(_val);
    #endif // CMFT_COMPILER_
    }

    inline uint32_t uint32_cntlz_ref(uint32_t _val)
    {
        const uint32_t tmp0   = uint32_srl(_val, 1);
        const uint32_t tmp1   = uint32_or(tmp0, _val);
        const uint32_t tmp2   = uint32_srl(tmp1, 2);
        const uint32_t tmp3   = uint32_or(tmp2, tmp1);
        const uint32_t tmp4   = uint32_srl(tmp3, 4);
        const uint32_t tmp5   = uint32_or(tmp4, tmp3);
        const uint32_t tmp6   = uint32_srl(tmp5, 8);
        const uint32_t tmp7   = uint32_or(tmp6, tmp5);
        const uint32_t tmp8   = uint32_srl(tmp7, 16);
        const uint32_t tmp9   = uint32_or(tmp8, tmp7);
        const uint32_t tmpA   = uint32_not(tmp9);
        const uint32_t result = uint32_cntbits(tmpA);

        return result;
    }

    /// Count number of leading zeros.
    inline uint32_t uint32_cntlz(uint32_t _val)
    {
    #if CMFT_COMPILER_GCC || CMFT_COMPILER_CLANG
        return __builtin_clz(_val);
    #elif CMFT_COMPILER_MSVC && CMFT_PLATFORM_WINDOWS
        unsigned long index;
        _BitScanReverse(&index, _val);
        return 31 - index;
    #else
        return uint32_cntlz_ref(_val);
    #endif // CMFT_COMPILER_
    }

    inline float halfToFloat(uint16_t _a)
    {
        const uint32_t h_e_mask              = uint32_li(0x00007c00);
        const uint32_t h_m_mask              = uint32_li(0x000003ff);
        const uint32_t h_s_mask              = uint32_li(0x00008000);
        const uint32_t h_f_s_pos_offset      = uint32_li(0x00000010);
        const uint32_t h_f_e_pos_offset      = uint32_li(0x0000000d);
        const uint32_t h_f_bias_offset       = uint32_li(0x0001c000);
        const uint32_t f_e_mask              = uint32_li(0x7f800000);
        const uint32_t f_m_mask              = uint32_li(0x007fffff);
        const uint32_t h_f_e_denorm_bias     = uint32_li(0x0000007e);
        const uint32_t h_f_m_denorm_sa_bias  = uint32_li(0x00000008);
        const uint32_t f_e_pos               = uint32_li(0x00000017);
        const uint32_t h_e_mask_minus_one    = uint32_li(0x00007bff);
        const uint32_t h_e                   = uint32_and(_a, h_e_mask);
        const uint32_t h_m                   = uint32_and(_a, h_m_mask);
        const uint32_t h_s                   = uint32_and(_a, h_s_mask);
        const uint32_t h_e_f_bias            = uint32_add(h_e, h_f_bias_offset);
        const uint32_t h_m_nlz               = uint32_cntlz(h_m);
        const uint32_t f_s                   = uint32_sll(h_s, h_f_s_pos_offset);
        const uint32_t f_e                   = uint32_sll(h_e_f_bias, h_f_e_pos_offset);
        const uint32_t f_m                   = uint32_sll(h_m, h_f_e_pos_offset);
        const uint32_t f_em                  = uint32_or(f_e, f_m);
        const uint32_t h_f_m_sa              = uint32_sub(h_m_nlz, h_f_m_denorm_sa_bias);
        const uint32_t f_e_denorm_unpacked   = uint32_sub(h_f_e_denorm_bias, h_f_m_sa);
        const uint32_t h_f_m                 = uint32_sll(h_m, h_f_m_sa);
        const uint32_t f_m_denorm            = uint32_and(h_f_m, f_m_mask);
        const uint32_t f_e_denorm            = uint32_sll(f_e_denorm_unpacked, f_e_pos);
        const uint32_t f_em_denorm           = uint32_or(f_e_denorm, f_m_denorm);
        const uint32_t f_em_nan              = uint32_or(f_e_mask, f_m);
        const uint32_t is_e_eqz_msb          = uint32_dec(h_e);
        const uint32_t is_m_nez_msb          = uint32_neg(h_m);
        const uint32_t is_e_flagged_msb      = uint32_sub(h_e_mask_minus_one, h_e);
        const uint32_t is_zero_msb           = uint32_andc(is_e_eqz_msb, is_m_nez_msb);
        const uint32_t is_inf_msb            = uint32_andc(is_e_flagged_msb, is_m_nez_msb);
        const uint32_t is_denorm_msb         = uint32_and(is_m_nez_msb, is_e_eqz_msb);
        const uint32_t is_nan_msb            = uint32_and(is_e_flagged_msb, is_m_nez_msb);
        const uint32_t is_zero               = uint32_ext(is_zero_msb);
        const uint32_t f_zero_result         = uint32_andc(f_em, is_zero);
        const uint32_t f_denorm_result       = uint32_sels(is_denorm_msb, f_em_denorm, f_zero_result);
        const uint32_t f_inf_result          = uint32_sels(is_inf_msb, f_e_mask, f_denorm_result);
        const uint32_t f_nan_result          = uint32_sels(is_nan_msb, f_em_nan, f_inf_result);
        const uint32_t f_result              = uint32_or(f_s, f_nan_result);

        union { uint32_t ui; float flt; } utof;
        utof.ui = f_result;
        return utof.flt;
    }

    inline uint16_t halfFromFloat(float _a)
    {
        union { uint32_t ui; float flt; } ftou;
        ftou.flt = _a;

        const uint32_t one                        = uint32_li(0x00000001);
        const uint32_t f_s_mask                   = uint32_li(0x80000000);
        const uint32_t f_e_mask                   = uint32_li(0x7f800000);
        const uint32_t f_m_mask                   = uint32_li(0x007fffff);
        const uint32_t f_m_hidden_bit             = uint32_li(0x00800000);
        const uint32_t f_m_round_bit              = uint32_li(0x00001000);
        const uint32_t f_snan_mask                = uint32_li(0x7fc00000);
        const uint32_t f_e_pos                    = uint32_li(0x00000017);
        const uint32_t h_e_pos                    = uint32_li(0x0000000a);
        const uint32_t h_e_mask                   = uint32_li(0x00007c00);
        const uint32_t h_snan_mask                = uint32_li(0x00007e00);
        const uint32_t h_e_mask_value             = uint32_li(0x0000001f);
        const uint32_t f_h_s_pos_offset           = uint32_li(0x00000010);
        const uint32_t f_h_bias_offset            = uint32_li(0x00000070);
        const uint32_t f_h_m_pos_offset           = uint32_li(0x0000000d);
        const uint32_t h_nan_min                  = uint32_li(0x00007c01);
        const uint32_t f_h_e_biased_flag          = uint32_li(0x0000008f);
        const uint32_t f_s                        = uint32_and(ftou.ui, f_s_mask);
        const uint32_t f_e                        = uint32_and(ftou.ui, f_e_mask);
        const uint16_t h_s              = (uint16_t)uint32_srl(f_s, f_h_s_pos_offset);
        const uint32_t f_m                        = uint32_and(ftou.ui, f_m_mask);
        const uint16_t f_e_amount       = (uint16_t)uint32_srl(f_e, f_e_pos);
        const uint32_t f_e_half_bias              = uint32_sub(f_e_amount, f_h_bias_offset);
        const uint32_t f_snan                     = uint32_and(ftou.ui, f_snan_mask);
        const uint32_t f_m_round_mask             = uint32_and(f_m, f_m_round_bit);
        const uint32_t f_m_round_offset           = uint32_sll(f_m_round_mask, one);
        const uint32_t f_m_rounded                = uint32_add(f_m, f_m_round_offset);
        const uint32_t f_m_denorm_sa              = uint32_sub(one, f_e_half_bias);
        const uint32_t f_m_with_hidden            = uint32_or(f_m_rounded, f_m_hidden_bit);
        const uint32_t f_m_denorm                 = uint32_srl(f_m_with_hidden, f_m_denorm_sa);
        const uint32_t h_m_denorm                 = uint32_srl(f_m_denorm, f_h_m_pos_offset);
        const uint32_t f_m_rounded_overflow       = uint32_and(f_m_rounded, f_m_hidden_bit);
        const uint32_t m_nan                      = uint32_srl(f_m, f_h_m_pos_offset);
        const uint32_t h_em_nan                   = uint32_or(h_e_mask, m_nan);
        const uint32_t h_e_norm_overflow_offset   = uint32_inc(f_e_half_bias);
        const uint32_t h_e_norm_overflow          = uint32_sll(h_e_norm_overflow_offset, h_e_pos);
        const uint32_t h_e_norm                   = uint32_sll(f_e_half_bias, h_e_pos);
        const uint32_t h_m_norm                   = uint32_srl(f_m_rounded, f_h_m_pos_offset);
        const uint32_t h_em_norm                  = uint32_or(h_e_norm, h_m_norm);
        const uint32_t is_h_ndenorm_msb           = uint32_sub(f_h_bias_offset, f_e_amount);
        const uint32_t is_f_e_flagged_msb         = uint32_sub(f_h_e_biased_flag, f_e_half_bias);
        const uint32_t is_h_denorm_msb            = uint32_not(is_h_ndenorm_msb);
        const uint32_t is_f_m_eqz_msb             = uint32_dec(f_m);
        const uint32_t is_h_nan_eqz_msb           = uint32_dec(m_nan);
        const uint32_t is_f_inf_msb               = uint32_and(is_f_e_flagged_msb, is_f_m_eqz_msb);
        const uint32_t is_f_nan_underflow_msb     = uint32_and(is_f_e_flagged_msb, is_h_nan_eqz_msb);
        const uint32_t is_e_overflow_msb          = uint32_sub(h_e_mask_value, f_e_half_bias);
        const uint32_t is_h_inf_msb               = uint32_or(is_e_overflow_msb, is_f_inf_msb);
        const uint32_t is_f_nsnan_msb             = uint32_sub(f_snan, f_snan_mask);
        const uint32_t is_m_norm_overflow_msb     = uint32_neg(f_m_rounded_overflow);
        const uint32_t is_f_snan_msb              = uint32_not(is_f_nsnan_msb);
        const uint32_t h_em_overflow_result       = uint32_sels(is_m_norm_overflow_msb, h_e_norm_overflow, h_em_norm);
        const uint32_t h_em_nan_result            = uint32_sels(is_f_e_flagged_msb, h_em_nan, h_em_overflow_result);
        const uint32_t h_em_nan_underflow_result  = uint32_sels(is_f_nan_underflow_msb, h_nan_min, h_em_nan_result);
        const uint32_t h_em_inf_result            = uint32_sels(is_h_inf_msb, h_e_mask, h_em_nan_underflow_result);
        const uint32_t h_em_denorm_result         = uint32_sels(is_h_denorm_msb, h_m_denorm, h_em_inf_result);
        const uint32_t h_em_snan_result           = uint32_sels(is_f_snan_msb, h_snan_mask, h_em_denorm_result);
        const uint32_t h_result                   = uint32_or(h_s, h_em_snan_result);

        return (uint16_t)(h_result);
    }
}

#endif // CMFT_HALFLOAT_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
