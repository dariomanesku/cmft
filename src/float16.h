/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_FLOAT16_H_HEADER_GUARD
#define CMFT_FLOAT16_H_HEADER_GUARD

#include <stdint.h>

#ifndef CMFT_USE_BRANCHLESS_FLOAT16_CONVERTER
    #define CMFT_USE_BRANCHLESS_FLOAT16_CONVERTER 1
#endif // CMFT_USE_BRANCHLESS_FLOAT16_CONVERTER

namespace cmft
{
    ///
    uint16_t float16Compress(float _float);

    ///
    float float16Decompress(uint16_t _halfFloat);

} // namespace cmft

#endif // CMFT_FLOAT16_H_HEADER_GUARD
