/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <common/config.h>

#ifndef CMFT_TEST_BUILD
    #define CMFT_TEST_BUILD 0
#endif

#if CMFT_TEST_BUILD
    #include "cmft_tests/tests.h"
    int main(int _argc, char const* const* _argv)
    {
        return testsMain(_argc, _argv);
    }
#else
    #include "cmft_cli/cmft_cli.h"
    int main(int _argc, char const* const* _argv)
    {
        return cmftMain(_argc, _argv);
    }
#endif //CMFT_TEST_BUILD

/* vim: set sw=4 ts=4 expandtab: */
