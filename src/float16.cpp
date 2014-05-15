/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "float16.h"
#include <float.h> //FLT_MAX

namespace cmft
{

#if CMFT_USE_BRANCHLESS_FLOAT16_CONVERTER

    //Adopted from:
    //http://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion/3542975#3542975
    struct FloatCompressor
    {
        union Bits
        {
            float f;
            int32_t si;
            uint32_t ui;
        };

        static const int shift = 13;
        static const int shiftSign = 16;

        static const int32_t infN = 0x7F800000; // flt32 infinity
        static const int32_t maxN = 0x477FE000; // max flt16 normal as a flt32
        static const int32_t minN = 0x38800000; // min flt16 normal as a flt32
        static const int32_t signN = 0x80000000; // flt32 sign bit

        static const int32_t infC = infN >> shift;
        static const int32_t nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
        static const int32_t maxC = maxN >> shift;
        static const int32_t minC = minN >> shift;
        static const int32_t signC = uint32_t(signN) >> shiftSign; // flt16 sign bit

        static const int32_t mulN = 0x52000000; // (1 << 23) / minN
        static const int32_t mulC = 0x33800000; // minN / (1 << (23 - shift))

        static const int32_t subC = 0x003FF; // max flt32 subnormal down shifted
        static const int32_t norC = 0x00400; // min flt32 normal down shifted

        static const int32_t maxD = infC - maxC - 1;
        static const int32_t minD = minC - subC - 1;

        uint16_t compress(float _val)
        {
            Bits v, s;
            v.f = _val;
            uint32_t sign = v.si & signN;
            v.si ^= sign;
            sign >>= shiftSign; // logical shift
            s.si = mulN;
            s.si = uint32_t(s.f * v.f); // correct subnormals
            v.si ^= (s.si ^ v.si) & -(minN > v.si);
            v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
            v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
            v.ui >>= shift; // logical shift
            v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
            v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
            return v.ui | sign;
        }

        float decompress(uint16_t _val)
        {
            Bits v;
            v.ui = _val;
            uint32_t sign = v.si & signC;
            v.si ^= sign;
            sign <<= shiftSign;
            v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
            v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
            Bits s;
            s.si = mulC;
            s.f *= v.si;
            uint32_t mask = -(norC > v.si);
            v.si <<= shift;
            v.si ^= (s.si ^ v.si) & mask;
            v.si |= sign;
            return v.f;
        }
    };
    static FloatCompressor s_floatCompressor;

#else //!CMFT_USE_BRANCHLESS_FLOAT16_CONVERTER

    //TODO: not tested !
    struct FloatCompressor
    {
        union Bits
        {
            float f;
            int32_t si;
            uint32_t ui;
        };

        // Legal values:
        // _min <= 0 < _epsilon < _max
        // 0 <= _precision <= 23
        FloatCompressor(float _min, float _epsilon, float _max, int _precision)
        {
            m_shift = 23 - _precision;

            Bits v;
            v.f = _min;
            m_minF = v.si;
            v.f = _epsilon;
            m_epsF = v.si;
            v.f = _max;
            m_maxF = v.si;

            hasNegatives = m_minF < 0;
            noLoss = m_shift == 0;

            int32_t pepsU, nepsU;
            if(noLoss)
            {
                nepsU = m_epsF;
                pepsU = m_epsF ^ signF;
                m_maxC = m_maxF ^ signF;
                m_zeroC = signF;
            }
            else
            {
                nepsU = uint32_t(m_epsF ^ signF) >> m_shift;
                pepsU = uint32_t(m_epsF) >> m_shift;
                m_maxC = uint32_t(m_maxF) >> m_shift;
                m_zeroC = 0;
            }

            m_pDelta = pepsU - m_zeroC - 1;
            m_nDelta = nepsU - m_maxC - 1;
        }

        uint32_t compress(float _val)
        {
            Bits v;
            v.f = clamp(_val);

            if(noLoss)
            {
                v.si ^= signF;
            }
            else
            {
                v.ui >>= m_shift;
            }

            if(hasNegatives)
            {
                v.si ^= ((v.si - m_nDelta) ^ v.si) & -(v.si > m_maxC);
            }
            v.si ^= ((v.si - m_pDelta) ^ v.si) & -(v.si > m_zeroC);

            if(noLoss)
            {
                v.si ^= signF;
            }

            return v.ui;
        }

        float decompress(uint32_t _val)
        {
            Bits v;
            v.ui = _val;

            if(noLoss)
            {
                v.si ^= signF;
            }

            v.si ^= ((v.si + m_pDelta) ^ v.si) & -(v.si > m_zeroC);
            if(hasNegatives)
            {
                v.si ^= ((v.si + m_nDelta) ^ v.si) & -(v.si > m_maxC);
            }

            if(noLoss)
            {
                v.si ^= signF;
            }
            else
            {
                v.si <<= m_shift;
            }

            return v.f;
        }

    private:
        float clamp(float _val)
        {
            Bits v;
            v.f = _val;

            int32_t max = m_maxF;
            if(hasNegatives)
            {
                max ^= (m_minF ^ m_maxF) & -(0 > v.si);
            }

            v.si ^= (max ^ v.si) & -(v.si > max);
            v.si &= -(m_epsF <= (v.si & absF));

            return v.f;
        }

        static const int32_t signF = 0x80000000;
        static const int32_t absF = ~signF;

        bool hasNegatives;
        bool noLoss;
        int32_t m_maxF;
        int32_t m_minF;
        int32_t m_epsF;
        int32_t m_maxC;
        int32_t m_zeroC;
        int32_t m_pDelta;
        int32_t m_nDelta;
        int m_shift;
    };
    static FloatCompressor s_floatCompressor(-FLT_MAX, FLT_EPSILON, FLT_MAX, 23);

#endif // CMFT_USE_BRANCHLESS_FLOAT16_CONVERTER

    uint16_t float16Compress(float _float)
    {
        return s_floatCompressor.compress(_float);
    }

    float float16Decompress(uint16_t _halfFloat)
    {
        return s_floatCompressor.decompress(_halfFloat);
    }

} // namespace cmft
