/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CLCONTEXT_H_HEADER_GUARD
#define CMFT_CLCONTEXT_H_HEADER_GUARD

#include "config.h"

#include <stdint.h>
#include <bx/string.h>

#include <bx/cl.h>

namespace cmft
{

#define CL_VENDOR_INTEL    (0x1)
#define CL_VENDOR_AMD      (0x2)
#define CL_VENDOR_NVIDIA   (0x4)
#define CL_VENDOR_OTHER    (0x8)
#define CL_VENDOR_ANY_GPU  (CL_VENDOR_AMD|CL_VENDOR_NVIDIA)
#define CL_VENDOR_ANY_CPU  (CL_VENDOR_AMD|CL_VENDOR_INTEL)

#define CMFT_ENABLE_CL_CHECK 0

// CL check.
#ifndef CMFT_ENABLE_CL_CHECK
    #define CMFT_ENABLE_CL_CHECK 0
#endif

#if CMFT_ENABLE_CL_CHECK
    #define CL_CHECK            _CL_CHECK
    #define CL_CHECK_ERR        _CL_CHECK_ERR
#else
    #define CL_CHECK(_expr)     _expr
    #define CL_CHECK_ERR(_expr) _expr
#endif

#define _CL_CHECK(_expr)                                                                 \
    do                                                                                   \
    {                                                                                    \
        cl_int err = _expr;                                                              \
        if (CL_SUCCESS != err)                                                           \
        {                                                                                \
            fprintf(stderr, "CMFT OpenCL Error: '%s' returned %d!\n", #_expr, (int)err); \
            abort();                                                                     \
        }                                                                                \
    } while (0)

/// Notice: 'cl_int err;' should be defined earlier and _expr should be called with 'err' parameter for the error field.
#define _CL_CHECK_ERR(_expr)                                                              \
    _expr;                                                                            \
    if (CL_SUCCESS != err)                                                            \
    {                                                                                 \
        fprintf(stderr, "CMFT OpenCL Error: '%s' returned %d!\n", #_expr, (int)err);  \
        abort();                                                                      \
    }

    struct ClContext
    {
        ClContext()
            : m_device(NULL)
            , m_context(NULL)
            , m_commandQueue(NULL)
        {
            m_deviceVendor[0] = '\0';
            m_deviceName[0] = '\0';
        }

        bool init(uint8_t _vendor                     = CL_VENDOR_ANY_GPU
                , cl_device_type _preferredDeviceType = CL_DEVICE_TYPE_GPU
                , cl_uint _preferredDeviceIdx         = 0
                , char* _vendorStrPart                = NULL
                );

        void destroy();

        cl_device_id m_device;
        cl_context m_context;
        cl_command_queue m_commandQueue;
        cl_device_type m_deviceType;
        char m_deviceVendor[128];
        char m_deviceName[128];
    };

    ///
    void clPrintDevices();


} // namespace cmft

#endif //CMFT_CLCONTEXT_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
