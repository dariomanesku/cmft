/*
 * Copyright 2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CLCONTEXT_INTERNAL_H_HEADER_GUARD
#define CMFT_CLCONTEXT_INTERNAL_H_HEADER_GUARD

#include "common/cl.h"

namespace cmft
{
    struct ClContext
    {
        ClContext()
        {
            m_device = NULL;
            m_context = NULL;
            m_commandQueue = NULL;
            m_deviceVendor[0] = '\0';
            m_deviceName[0] = '\0';
        }

        cl_device_id m_device;
        cl_context m_context;
        cl_command_queue m_commandQueue;
        cl_device_type m_deviceType;
        char m_deviceVendor[128];
        char m_deviceName[128];
    };

} // namespace cmft

#endif //CMFT_CLCONTEXT_INTERNAL_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

