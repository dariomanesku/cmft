/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <stdint.h>

#include <bx/string.h>
#define BX_CL_IMPLEMENTATION
#include <bx/cl.h>

#include <dm/misc.h> //dm::strscpya

#include "cmft/clcontext.h"
#include "base/config.h" //INFO, WARN
#include "base/printcallback.h" //_INFO, _WARN

namespace cmft
{
    bool ClContext::init(uint8_t _vendor
                       , cl_device_type _preferredDeviceType
                       , cl_uint _preferredDeviceIdx
                       , char* _vendorStrPart
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
                    const char* searchSpecific = bx::stristr(buffer, _vendorStrPart, 256);
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
                        const char* searchAmd = bx::stristr(buffer, "advanced micro devices", 256);
                        found |= (NULL != searchAmd);
                    }

                    if (_vendor&CMFT_CL_VENDOR_INTEL)
                    {
                        const char* searchIntel  = bx::stristr(buffer, "intel", 256);
                        found |= (NULL != searchIntel);
                    }

                    if (_vendor&CMFT_CL_VENDOR_NVIDIA)
                    {
                        const char* searchNvidia = bx::stristr(buffer, "nvidia", 256);
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
            err = clGetDeviceIDs(platforms[ii], _preferredDeviceType, 8, devices, &numDevices);
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
            return false;
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
                return false;
            }
        }

        // Get device name, vendor and type.
        char deviceVendor[128];
        CL_CHECK(clGetPlatformInfo(choosenPlatform, CL_PLATFORM_VENDOR, sizeof(deviceVendor), deviceVendor, NULL));
        char deviceName[128];
        CL_CHECK(clGetDeviceInfo(chosenDevice, CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL));
        CL_CHECK(clGetDeviceInfo(chosenDevice, CL_DEVICE_TYPE, sizeof(m_deviceType), &m_deviceType, NULL));

        // Create command queue
        cl_command_queue commandQueue;
        commandQueue = clCreateCommandQueue(context, chosenDevice, 0, &err);
        if (CL_SUCCESS != err)
        {
            WARN("OpenCL context initialization failed!");
            return false;
        }

        // Fill structure.
        dm::strscpya(m_deviceVendor, dm::trim(deviceVendor));
        dm::strscpya(m_deviceName, dm::trim(deviceName));
        m_device = chosenDevice;
        m_context = context;
        m_commandQueue = commandQueue;

        return true;
    }

    void ClContext::destroy()
    {
        if (NULL != m_commandQueue)
        {
            clReleaseCommandQueue(m_commandQueue);
            m_commandQueue = NULL;
        }

        if (NULL != m_context)
        {
            clReleaseContext(m_context);
            m_context = NULL;
        }
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
            if (NULL != bx::stristr(vendor, "advanced micro devices", 256))
            {
                dm::strscpya(platformOutputStr, "amd");
            }
            else if (NULL != bx::stristr(vendor, "intel", 256))
            {
                dm::strscpya(platformOutputStr, "intel");
            }
            else if (NULL != bx::stristr(vendor, "nvidia", 256))
            {
                dm::strscpya(platformOutputStr, "nvidia");
            }
            else
            {
                dm::strscpya(platformOutputStr, dm::trim(vendor));
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
                        dm::strscpya(deviceTypeOutputStr, "gpu");
                    }
                    else if (CMFT_CL_DEVICE_TYPE_CPU == deviceType)
                    {
                        dm::strscpya(deviceTypeOutputStr, "cpu");
                    }
                    else if (CMFT_CL_DEVICE_TYPE_ACCELERATOR == deviceType)
                    {
                        dm::strscpya(deviceTypeOutputStr, "accelerator");
                    }
                    else //if (CMFT_CL_DEVICE_TYPE_DEFAULT == deviceType)
                    {
                        dm::strscpya(deviceTypeOutputStr, "default");
                    }

                    // Print device info.
                    INFO("%-40s --clVendor %-6s --deviceIndex %u --deviceType %s"
                        , dm::trim(deviceName)
                        , platformOutputStr
                        , uint32_t(jj)
                        , deviceTypeOutputStr
                        );
                }
            }
        }
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
