/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_TESTS_H_HEADER_GUARD
#define CMFT_TESTS_H_HEADER_GUARD

#include "../cmft_cli/cmft_cli.h"
#include "tokenize.h"

static const char s_radianceTest[] =
{
    "--input \"okretnica.tga\"           "
    "--filter radiance                   "
    "--srcFaceSize 256                   "
    "--excludeBase false                 "
    "--mipCount 7                        "
    "--glossScale 10                     "
    "--glossBias 3                       "
    "--lightingModel blinnbrdf           "
    "--dstFaceSize 256                   "
    "--numCpuProcessingThreads 4         "
    "--useOpenCL true                    "
    "--clVendor anyGpuVendor             "
    "--deviceType gpu                    "
    "--deviceIndex 0                     "
    "--inputGammaNumerator 2.2           "
    "--inputGammaDenominator 1.0         "
    "--outputGammaNumerator 1.0          "
    "--outputGammaDenominator 2.2        "
    "--generateMipChain false            "
    "--outputNum 1                       "
    "--output0 \"okretnicaPmrem\"        "
    "--output0params dds,bgra8,cubemap   "
};

static const char s_tgaRadianceTest[] =
{
    "--input \"okretnica.tga\"           "
    "--filter radiance                   "
    "--srcFaceSize 256                   "
    "--excludeBase false                 "
    "--mipCount 7                        "
    "--glossScale 10                     "
    "--glossBias 3                       "
    "--lightingModel blinnbrdf           "
    "--dstFaceSize 256                   "
    "--numCpuProcessingThreads 4         "
    "--useOpenCL true                    "
    "--clVendor anyGpuVendor             "
    "--deviceType gpu                    "
    "--deviceIndex 0                     "
    "--inputGammaNumerator 2.2           "
    "--inputGammaDenominator 1.0         "
    "--outputGammaNumerator 1.0          "
    "--outputGammaDenominator 2.2        "
    "--generateMipChain false            "
    "--outputNum 1                       "
    "--output0 \"okretnicaPmrem\"        "
    "--output0params tga,bgra8,facelist  "
};

static const char s_outputTest[] =
{
    "--input \"okretnica.tga\"          "
    "--filter none                      "
    "--outputNum 7                      "
    "--output0 \"okretnica_cubemap\"    "
    "--output0params dds,bgra8,cubemap  "
    "--output1 \"okretnica_latlong\"    "
    "--output1params dds,bgra8,latlong  "
    "--output2 \"okretnica_hcross\"     "
    "--output2params dds,bgra8,hcross   "
    "--output3 \"okretnica_vcross\"     "
    "--output3params dds,bgra8,vcross   "
    "--output4 \"okretnica_hstrip\"     "
    "--output4params dds,bgra8,hstrip   "
    "--output5 \"okretnica_vstrip\"     "
    "--output5params dds,bgra8,vstrip   "
    "--output6 \"okretnica_facelist\"   "
    "--output6params dds,bgra8,facelist "
};

static const char s_gpuTest[] =
{
    "--input \"okretnica.tga\"           "
    "--filter radiance                   "
    "--srcFaceSize 256                   "
    "--excludeBase false                 "
    "--mipCount 7                        "
    "--glossScale 10                     "
    "--glossBias 3                       "
    "--lightingModel blinnbrdf           "
    "--edgeFixup none                    "
    "--dstFaceSize 256                   "
    "--numCpuProcessingThreads 0         "
    "--useOpenCL true                    "
    "--clVendor anyGpuVendor             "
    "--deviceType gpu                    "
    "--deviceIndex 0                     "
    "--inputGammaNumerator 2.2           "
    "--inputGammaDenominator 1.0         "
    "--outputGammaNumerator 1.0          "
    "--outputGammaDenominator 2.2        "
    "--generateMipChain false            "
    "--outputNum 1                       "
    "--output0 \"okretnica_gpu\"         "
    "--output0params dds,bgra8,latlong   "
};

static const char s_test0[] =
{
    "--input \"okretnica.tga\"          "
    "--outputGammaNumerator 1.0         "
    "--outputGammaDenominator 2.2       "
    "--outputNum 1                      "
    "--output0 \"okretnica_test0\"      "
    "--output0params dds,bgra8,cubemap  "
};

static const char s_test1[] =
{
    "--input \"okretnica.tga\"           "
    "--filter irradiance                 "
    "--useOpenCL true                    "
    "--mipCount 12                       "
    "--generateMipChain true             "
    "--dstFaceSize 256                   "
    "--outputNum 1                       "
    "--output0 \"okretnica_test1\"       "
    "--output0params dds,rgba16,cubemap  "
};

int test(const char* _cmd)
{
    char data[2048];
    uint32_t dataSize;
    int argc;
    char* argv[128];
    tokenizeCommandLine(_cmd, data, dataSize, argc, argv, CMFT_COUNTOF(argv), '\0');
    return cmftMain(argc, argv);
}

int testsMain(int /*_argc*/, char const* const* /*_argv*/)
{
    test(s_radianceTest);
    //test(s_tgaRadianceTest);
    //test(s_outputTest);
    //test(s_gpuTest);
    //test(s_test0);
    //test(s_test1);

    char c;
    const int unused = scanf("%c", &c);
    CMFT_UNUSED(c);
    CMFT_UNUSED(unused);

    return 0;
}

#endif //CMFT_TESTS_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
