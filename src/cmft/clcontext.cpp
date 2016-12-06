/*
 * Copyright 2014-2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#define CMFT_CL_IMPLEMENTATION
#include "common/cl.h"

#include <cmft/clcontext.h>
#include "clcontext_internal.h"

#include <stdint.h>

#include "common/config.h"
#include "common/utils.h"
#include "common/handlealloc.h"

namespace cmft
{
    struct ClContextStorage
    {
        ClContext* alloc()
        {
            return &m_context[m_handleAlloc.alloc()];
        }

        void free(ClContext* _context)
        {
            if (&m_context[0] <= _context && _context <= &m_context[MaxClContexts])
            {
                uint32_t idx = uint32_t(_context - m_context);
                if (m_handleAlloc.contains(idx))
                {
                    m_handleAlloc.free(idx);
                }
            }
        }

        enum { MaxClContexts = 1024 };

        HandleAllocT<MaxClContexts> m_handleAlloc;
        ClContext m_context[MaxClContexts];
    };
    static ClContextStorage s_clContextStorage;

    ClContext* clInit(uint32_t _vendor
                    , uint32_t _preferredDeviceType
                    , uint32_t _preferredDeviceIdx
                    , const char* _vendorStrPart
                    )
    {
        cl_int err = CL_SUCCESS;

        // Enumerate platforms.
        cl_platform_id platforms[8];
        cl_uint numPlatforms;
        CL_CHECK(clGetPlatformIDs(8, platforms, &numPlatforms));

        // Choose preferred platform.
        cl_platform_id choosenPlatform = platforms[0];
        if (NULL != _vendorStrPart)
        {
            char buffer[256];
            for (cl_uint ii = 0; ii < numPlatforms; ++ii)
            {
                // Get platform vendor.
                CL_CHECK(clGetPlatformInfo(platforms[ii], CL_PLATFORM_VENDOR, 256, buffer, NULL));

                if (_vendor&CMFT_CL_VENDOR_OTHER)
                {
                    // If specific vendor is requested, check for it.
                    const char* searchSpecific = cmft::stristr(buffer, _vendorStrPart, 256);
                    if (NULL != searchSpecific)
                    {
                        choosenPlatform = platforms[ii];
                        break;
                    }
                }
                else
                {
                    bool found = false;

                    // Check for predefined vendors.
                    if (_vendor&CMFT_CL_VENDOR_AMD)
                    {
                        const char* searchAmd = cmft::stristr(buffer, "advanced micro devices", 256);
                        found |= (NULL != searchAmd);
                    }

                    if (_vendor&CMFT_CL_VENDOR_INTEL)
                    {
                        const char* searchIntel  = cmft::stristr(buffer, "intel", 256);
                        found |= (NULL != searchIntel);
                    }

                    if (_vendor&CMFT_CL_VENDOR_NVIDIA)
                    {
                        const char* searchNvidia = cmft::stristr(buffer, "nvidia", 256);
                        found |= (NULL != searchNvidia);
                    }

                    if (found)
                    {
                        choosenPlatform = platforms[ii];
                        break;
                    }
                }
            }
        }

        // Enumerate devices.
        cl_device_id devices[8] = { 0 };
        cl_uint numDevices = 0;

        // First try to get preferred device type.
        for (cl_uint ii = 0; ii < numPlatforms; ++ii)
        {
            err = clGetDeviceIDs(platforms[ii], (cl_device_type)_preferredDeviceType, 8, devices, &numDevices);
            if (CL_SUCCESS == err)
            {
                choosenPlatform = platforms[ii];
                break;
            }
        }

        // If failed, just get anything there is.
        if (CL_SUCCESS != err)
        {
            for (cl_uint ii = 0; ii < numPlatforms; ++ii)
            {
                err = clGetDeviceIDs(platforms[ii], CL_DEVICE_TYPE_ALL, 8, devices, &numDevices);
                if (CL_SUCCESS == err)
                {
                    choosenPlatform = platforms[ii];
                    break;
                }
            }
        }

        if (CL_SUCCESS != err)
        {
            WARN("OpenCL context initialization failed!");
            return NULL;
        }

        // Choose preferred device and create context.
        cl_uint preferredDeviceIdx = (_preferredDeviceIdx < numDevices) ? _preferredDeviceIdx : 0;
        cl_device_id chosenDevice = devices[preferredDeviceIdx];
        cl_context context = clCreateContext(NULL, 1, &chosenDevice, NULL, NULL, &err);
        if (CL_SUCCESS != err)
        {
            chosenDevice = devices[0];
            context = clCreateContext(NULL, 1, &chosenDevice, NULL, NULL, &err);
            if (CL_SUCCESS != err)
            {
                WARN("OpenCL context initialization failed!");
                return NULL;
            }
        }

        // Create command queue
        cl_command_queue commandQueue;
        commandQueue = clCreateCommandQueue(context, chosenDevice, 0, &err);
        if (CL_SUCCESS != err)
        {
            WARN("OpenCL context initialization failed!");
            return NULL;
        }

        ClContext* clContext = s_clContextStorage.alloc();

        // Get device name, vendor and type.
        char deviceVendor[128];
        CL_CHECK(clGetPlatformInfo(choosenPlatform, CL_PLATFORM_VENDOR, sizeof(deviceVendor), deviceVendor, NULL));
        char deviceName[128];
        CL_CHECK(clGetDeviceInfo(chosenDevice, CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL));
        CL_CHECK(clGetDeviceInfo(chosenDevice, CL_DEVICE_TYPE, sizeof(clContext->m_deviceType), &clContext->m_deviceType, NULL));

        // Fill structure.
        cmft::stracpy(clContext->m_deviceVendor, cmft::trim(deviceVendor));
        cmft::stracpy(clContext->m_deviceName, cmft::trim(deviceName));
        clContext->m_device = chosenDevice;
        clContext->m_context = context;
        clContext->m_commandQueue = commandQueue;

        return clContext;
    }

    void clPrintDevices()
    {
        cl_int err;

        // Enumerate platforms.
        cl_platform_id platforms[8];
        cl_uint numPlatforms;
        CL_CHECK(clGetPlatformIDs(8, platforms, &numPlatforms));

        for (cl_uint ii = 0; ii < numPlatforms; ++ii)
        {
            // Get platform vendor.
            char vendor[256];
            CL_CHECK(clGetPlatformInfo(platforms[ii], CL_PLATFORM_VENDOR, 256, vendor, NULL));

            // Check for known vendors and save vendor str for later printing.
            char platformOutputStr[32];
            if (NULL != cmft::stristr(vendor, "advanced micro devices", 256))
            {
                cmft::stracpy(platformOutputStr, "amd");
            }
            else if (NULL != cmft::stristr(vendor, "intel", 256))
            {
                cmft::stracpy(platformOutputStr, "intel");
            }
            else if (NULL != cmft::stristr(vendor, "nvidia", 256))
            {
                cmft::stracpy(platformOutputStr, "nvidia");
            }
            else
            {
                cmft::stracpy(platformOutputStr, cmft::trim(vendor));
            }

            // Enumerate current platform devices.
            cl_device_id devices[8];
            cl_uint numDevices;
            err = clGetDeviceIDs(platforms[ii], CL_DEVICE_TYPE_ALL, 8, devices, &numDevices);
            if (CL_SUCCESS == err)
            {
                for (cl_uint jj = 0; jj < numDevices; ++jj)
                {
                    // Get device name.
                    char deviceName[128];
                    CL_CHECK(clGetDeviceInfo(devices[jj], CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL));

                    // Get device type.
                    cl_device_type deviceType;
                    CL_CHECK(clGetDeviceInfo(devices[jj], CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, NULL));

                    // Get device type str.
                    char deviceTypeOutputStr[16];
                    if (CMFT_CL_DEVICE_TYPE_GPU == deviceType)
                    {
                        cmft::stracpy(deviceTypeOutputStr, "gpu");
                    }
                    else if (CMFT_CL_DEVICE_TYPE_CPU == deviceType)
                    {
                        cmft::stracpy(deviceTypeOutputStr, "cpu");
                    }
                    else if (CMFT_CL_DEVICE_TYPE_ACCELERATOR == deviceType)
                    {
                        cmft::stracpy(deviceTypeOutputStr, "accelerator");
                    }
                    else //if (CMFT_CL_DEVICE_TYPE_DEFAULT == deviceType)
                    {
                        cmft::stracpy(deviceTypeOutputStr, "default");
                    }

                    // Print device info.
                    INFO("%-40s --clVendor %-6s --deviceIndex %u --deviceType %s"
                        , cmft::trim(deviceName)
                        , platformOutputStr
                        , uint32_t(jj)
                        , deviceTypeOutputStr
                        );
                }
            }
        }
    }

    void clDestroy(ClContext* _clContext)
    {
        if (NULL == _clContext)
        {
            return;
        }

        if (NULL != _clContext->m_commandQueue)
        {
            clReleaseCommandQueue(_clContext->m_commandQueue);
            _clContext->m_commandQueue = NULL;
        }

        if (NULL != _clContext->m_context)
        {
            clReleaseContext(_clContext->m_context);
            _clContext->m_context = NULL;
        }

        s_clContextStorage.free(_clContext);
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
