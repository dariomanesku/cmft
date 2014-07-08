/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <stdint.h>
#define BX_CL_IMPLEMENTATION
#include <bx/cl.h>

#include "base/utils.h" //strtolower, cmft_strscpy

#include "clcontext.h"
#include "messages.h"

namespace cmft
{
    bool ClContext::init(uint8_t _vendor
                       , cl_device_type _preferredDeviceType
                       , cl_uint _preferredDeviceIdx
                       , char* _vendorStrPart
                       )
    {
        cl_int err;

        // Enumerate platforms.
        cl_platform_id platforms[8];
        cl_uint numPlatforms;
        CL_CHECK(clGetPlatformIDs(8, platforms, &numPlatforms));

        // Choose preferred platform.
        cl_platform_id choosenPlatform = platforms[0];
        if (NULL != _vendorStrPart)
        {
            // If specific vendor is requested, transform input to lowercase.
            if (_vendor&CL_VENDOR_OTHER)
            {
                strtolower(_vendorStrPart);
            }

            char buffer[256];
            for (cl_uint ii = 0; ii < numPlatforms; ++ii)
            {
                // Get platform vendor.
                CL_CHECK(clGetPlatformInfo(platforms[ii], CL_PLATFORM_VENDOR, 256, buffer, NULL));

                // Transform it to lower case for easier searching.
                strtolower(buffer);

                if (_vendor&CL_VENDOR_OTHER)
                {
                    // If specific vendor is requested, check for it.
                    const char* searchSpecific = bx::strnstr(buffer, _vendorStrPart, 256);
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
                    if (_vendor&CL_VENDOR_AMD)
                    {
                        const char* searchAmd = bx::strnstr(buffer, "advanced micro devices", 256);
                        found |= (NULL != searchAmd);
                    }

                    if (_vendor&CL_VENDOR_INTEL)
                    {
                        const char* searchIntel  = bx::strnstr(buffer, "intel", 256);
                        found |= (NULL != searchIntel);
                    }

                    if (_vendor&CL_VENDOR_NVIDIA)
                    {
                        const char* searchNvidia = bx::strnstr(buffer, "nvidia", 256);
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

        // Try to enumerate devices.
        cl_device_id devices[8];
        cl_uint numDevices;
        err = clGetDeviceIDs(choosenPlatform, _preferredDeviceType, 8, devices, &numDevices);
        if (CL_SUCCESS != err)
        {
            err = clGetDeviceIDs(choosenPlatform, CL_DEVICE_TYPE_ALL, 8, devices, &numDevices);
            if (CL_SUCCESS != err)
            {
                err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 8, devices, &numDevices);
                if (CL_SUCCESS != err)
                {
                    WARN("OpenCL context initialization failed!");
                    return false;
                }
            }
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
        cmft_strscpy(m_deviceVendor, deviceVendor, 128);
        cmft_strscpy(m_deviceName, deviceName, 128);
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

    /// Notice: do NOT use return value of this function for memory deallocation!
    char* trimWhitespace(char* _str)
    {
        // Advance ptr until a non-space character is reached.
        while(isspace(*_str))
        {
            ++_str;
        }

        // If end is reached (_str contained all spaces), return.
        if ('\0' == *_str)
        {
            return _str;
        }

        // Point to the last non-whitespace character.
        char* end = _str + strlen(_str)-1;
        while (isspace(*end--) && end > _str)
        {
            // Empty body.
        }

        // Add string terminator.
        end[1] = '\0';

        return _str;
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

            // Transform it to lower case for easier searching.
            strtolower(vendor);

            // Check for known vendors and save vendor str for later printing.
            char platformOutputStr[32];
            if (NULL != bx::strnstr(vendor, "advanced micro devices", 256))
            {
                cmft_strscpy(platformOutputStr, "amd", 32);
            }
            else if (NULL != bx::strnstr(vendor, "intel", 256))
            {
                cmft_strscpy(platformOutputStr, "intel", 32);
            }
            else if (NULL != bx::strnstr(vendor, "nvidia", 256))
            {
                cmft_strscpy(platformOutputStr, "nvidia", 32);
            }
            else
            {
                cmft_strscpy(platformOutputStr, trimWhitespace(vendor), 32);
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
                    if (CL_DEVICE_TYPE_GPU == deviceType)
                    {
                        cmft_strscpy(deviceTypeOutputStr, "gpu", 16);
                    }
                    else if (CL_DEVICE_TYPE_CPU == deviceType)
                    {
                        cmft_strscpy(deviceTypeOutputStr, "cpu", 16);
                    }
                    else if (CL_DEVICE_TYPE_ACCELERATOR == deviceType)
                    {
                        cmft_strscpy(deviceTypeOutputStr, "accelerator", 16);
                    }
                    else //if (CL_DEVICE_TYPE_DEFAULT == deviceType)
                    {
                        cmft_strscpy(deviceTypeOutputStr, "default", 16);
                    }

                    // Print device info.
                    INFO("%-40s --clVendor %-6s --deviceIndex %u --deviceType %s"
                        , trimWhitespace(deviceName)
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
