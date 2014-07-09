/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CLCONTEXT_H_HEADER_GUARD
#define CMFT_CLCONTEXT_H_HEADER_GUARD

#include <stdint.h>

#include <bx/cl.h>

#define CMFT_CL_VENDOR_INTEL             (0x1)
#define CMFT_CL_VENDOR_AMD               (0x2)
#define CMFT_CL_VENDOR_NVIDIA            (0x4)
#define CMFT_CL_VENDOR_OTHER             (0x8)
#define CMFT_CL_VENDOR_ANY_GPU           (CMFT_CL_VENDOR_AMD|CMFT_CL_VENDOR_NVIDIA)
#define CMFT_CL_VENDOR_ANY_CPU           (CMFT_CL_VENDOR_AMD|CMFT_CL_VENDOR_INTEL)

#define CMFT_CL_DEVICE_TYPE_DEFAULT      CL_DEVICE_TYPE_DEFAULT
#define CMFT_CL_DEVICE_TYPE_CPU          CL_DEVICE_TYPE_CPU
#define CMFT_CL_DEVICE_TYPE_GPU          CL_DEVICE_TYPE_GPU
#define CMFT_CL_DEVICE_TYPE_ACCELERATOR  CL_DEVICE_TYPE_ACCELERATOR
#define CMFT_CL_DEVICE_TYPE_ALL          CL_DEVICE_TYPE_ALL

namespace cmft
{
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

        bool init(uint8_t _vendor                     = CMFT_CL_VENDOR_ANY_GPU
                , cl_device_type _preferredDeviceType = CMFT_CL_DEVICE_TYPE_GPU
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
