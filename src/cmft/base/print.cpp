/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <cmft/print.h>
#include <stdio.h> // ::printf

#include <base/config.h>
#include <bx/string.h>

namespace cmft
{
    PrintFunc printfWarning = &::printf;
    PrintFunc printfInfo    = &::printf;

    void setWarningPrintf(PrintFunc _printf)
    {
        printfWarning = _printf;
    }

    void setInfoPrintf(PrintFunc _printf)
    {
        printfInfo = _printf;
    }

} // namespace cmft

/* vim: set sw=4 ts=4 expandtab: */
