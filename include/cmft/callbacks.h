/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CALLBACKS_H_HEADER_GUARD
#define CMFT_CALLBACKS_H_HEADER_GUARD

namespace cmft
{

    struct PrintCallbackI
    {
        virtual ~PrintCallbackI() = 0;
        virtual void printWarning(const char* _msg) const = 0;
        virtual void printInfo(const char* _msg) const = 0;
    };

    inline PrintCallbackI::~PrintCallbackI()
    {
    }

    void setCallback(PrintCallbackI* _callback);

} // namespace cmft

#endif //CMFT_CALLBACKS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */

