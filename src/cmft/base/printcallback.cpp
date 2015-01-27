/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "cmft/callbacks.h"
#include "printcallback.h"
#include <bx/string.h>
#include <alloca.h>

namespace cmft
{
    bool g_printInfo     = true;
    bool g_printWarnings = true;

    struct PrintCallbackStub : public PrintCallbackI
    {
        virtual ~PrintCallbackStub()
        {
        }

        virtual void printWarning(const char* _msg) const
        {
            fputs(_msg, stderr);
        }

        virtual void printInfo(const char* _msg) const
        {
            fputs(_msg, stdout);
        }
    };
    static PrintCallbackStub s_messageCallbackStub;

    static PrintCallbackI* s_printCallback = &s_messageCallbackStub;

    void setCallback(PrintCallbackI* _callback)
    {
        s_printCallback = _callback;
    }

    char s_buffer[8192];
    static const char* printfVargs(const char* _format, va_list _argList)
    {
        char* out = s_buffer;
        int32_t len = bx::vsnprintf(out, sizeof(s_buffer), _format, _argList);
        if ((int32_t)sizeof(s_buffer) < len)
        {
            out = (char*)alloca(len+1);
            len = bx::vsnprintf(out, len, _format, _argList);
        }
        out[len] = '\0';
        return out;
    }

    void printInfo(const char* _format, ...)
    {
        va_list argList;
        va_start(argList, _format);
        const char* msg = printfVargs(_format, argList);
        va_end(argList);

        s_printCallback->printInfo(msg);
    }

    void printWarning(const char* _format, ...)
    {
        va_list argList;
        va_start(argList, _format);
        const char* msg = printfVargs(_format, argList);
        va_end(argList);

        s_printCallback->printWarning(msg);
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
