/*
 * Copyright 2014-2016 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_CMFT_CLI_H_HEADER_GUARD
#define CMFT_CMFT_CLI_H_HEADER_GUARD

#include <stdio.h>
#include <stdint.h>

#include <common/config.h>      // INFO, WARN
#include <common/utils.h>
#include <common/commandline.h>
#include <common/cl.h>

#include <cmft/image.h>
#include <cmft/cubemapfilter.h>
#include <cmft/clcontext.h>
#include <cmft/print.h>         // setWarningPrintf(), setInfoPrintf()

using namespace cmft;

struct FilterType
{
    enum Enum
    {
        None,
        Radiance,
        Irradiance,
        ShCoeffs,
    };
};

struct CliOptionMap
{
    char m_str[64];
    uint32_t m_uint;
};
#define CLI_OPTION_MAP_TERMINATOR { "", UINT32_MAX }

static const CliOptionMap s_filterType[] =
{
    { "none",       FilterType::None       },
    { "radiance",   FilterType::Radiance   },
    { "irradiance", FilterType::Irradiance },
    { "shcoeffs",   FilterType::ShCoeffs   },
    CLI_OPTION_MAP_TERMINATOR,
};

static const CliOptionMap s_lightingModel[] =
{
    { "phong",     LightingModel::Phong     },
    { "phongbrdf", LightingModel::PhongBrdf },
    { "blinn",     LightingModel::Blinn     },
    { "blinnbrdf", LightingModel::BlinnBrdf },
    CLI_OPTION_MAP_TERMINATOR,
};

static const CliOptionMap s_edgeFixup[] =
{
    { "None", EdgeFixup::None },
    { "Warp", EdgeFixup::Warp },
    CLI_OPTION_MAP_TERMINATOR,
};

static const CliOptionMap s_clVendors[] =
{
    { "NONE_FROM_THE_LIST", (uint32_t)CMFT_CL_VENDOR_OTHER   },
    { "intel",              (uint32_t)CMFT_CL_VENDOR_INTEL   },
    { "amd",                (uint32_t)CMFT_CL_VENDOR_AMD     },
    { "ati",                (uint32_t)CMFT_CL_VENDOR_AMD     },
    { "nvidia",             (uint32_t)CMFT_CL_VENDOR_NVIDIA  },
    { "anyGpuVendor",       (uint32_t)CMFT_CL_VENDOR_ANY_GPU },
    { "anyCpuVendor",       (uint32_t)CMFT_CL_VENDOR_ANY_CPU },
    CLI_OPTION_MAP_TERMINATOR,
};

static const CliOptionMap s_deviceType[] =
{
    { "gpu",         (uint32_t)CMFT_CL_DEVICE_TYPE_GPU         },
    { "cpu",         (uint32_t)CMFT_CL_DEVICE_TYPE_CPU         },
    { "accelerator", (uint32_t)CMFT_CL_DEVICE_TYPE_ACCELERATOR },
    { "default",     (uint32_t)CMFT_CL_DEVICE_TYPE_DEFAULT     },
    CLI_OPTION_MAP_TERMINATOR,
};

static const CliOptionMap s_validFileTypes[] =
{
    { "dds", ImageFileType::DDS },
    { "ktx", ImageFileType::KTX },
    { "tga", ImageFileType::TGA },
    { "hdr", ImageFileType::HDR },
    CLI_OPTION_MAP_TERMINATOR,
};

static const CliOptionMap s_validTextureFormats[] =
{
    { "bgr8",    TextureFormat::BGR8    },
    { "rgb8",    TextureFormat::RGB8    },
    { "rgb16",   TextureFormat::RGB16   },
    { "rgb16f",  TextureFormat::RGB16F  },
    { "rgb32f",  TextureFormat::RGB32F  },
    { "rgbe",    TextureFormat::RGBE    },
    { "bgra8",   TextureFormat::BGRA8   },
    { "rgba8",   TextureFormat::RGBA8   },
    { "rgba16",  TextureFormat::RGBA16  },
    { "rgba16f", TextureFormat::RGBA16F },
    { "rgba32f", TextureFormat::RGBA32F },
    { "rgbm",    TextureFormat::RGBM    },
    CLI_OPTION_MAP_TERMINATOR,
};

static const CliOptionMap s_validOutputTypes[] =
{
    { "latlong",  OutputType::LatLong  },
    { "cubemap",  OutputType::Cubemap  },
    { "hcross",   OutputType::HCross   },
    { "vcross",   OutputType::VCross   },
    { "hstrip",   OutputType::HStrip   },
    { "vstrip",   OutputType::VStrip   },
    { "facelist", OutputType::FaceList },
    { "octant",   OutputType::Octant   },
    CLI_OPTION_MAP_TERMINATOR,
};

bool valueFromOptionMap(uint32_t& _val, const CliOptionMap* _cliOptionMap, const char* _optionStr)
{
    // Check for valid cliOptionMap.
    if (NULL == _cliOptionMap
    || (_cliOptionMap->m_str[0] == '\0' && _cliOptionMap->m_uint == UINT32_MAX))
    {
        return false;
    }

    // Use default value.
    _val = _cliOptionMap->m_uint;

    // Check for valid optionStr.
    if (NULL == _optionStr)
    {
        return false;
    }

    // Copy input string.
    char optionStr[128];
    cmft::stracpy(optionStr, _optionStr);

    // Try to find value in cliOptionMap.
    const CliOptionMap* iter;
    while (iter = _cliOptionMap++, iter->m_str[0] != '\0')
    {
        if (0 == cmft::stricmp(optionStr, iter->m_str))
        {
            _val = iter->m_uint;
            return true;
        }
    }

    return false;
}

struct OutputFile
{
    OutputFile()
    {
        m_fileType          = 0;
        m_textureFormat     = 0;
        m_outputType        = 0;
        m_optionalParam0[0] = '\0';
        m_optionalParam1[0] = '\0';
        m_fileName[0]       = '\0';
    }

    uint32_t m_fileType;
    uint32_t m_textureFormat;
    uint32_t m_outputType;
    char m_optionalParam0[128];
    char m_optionalParam1[128];
    char m_fileName[CMFT_PATH_LEN];
};

struct InputParameters
{
    // Input.
    char m_inputFilePath[CMFT_PATH_LEN];
    char m_inputPosXFace[CMFT_PATH_LEN];
    char m_inputNegXFace[CMFT_PATH_LEN];
    char m_inputPosYFace[CMFT_PATH_LEN];
    char m_inputNegYFace[CMFT_PATH_LEN];
    char m_inputPosZFace[CMFT_PATH_LEN];
    char m_inputNegZFace[CMFT_PATH_LEN];

    // Image Operations.
    float m_inputGammaPowNumerator;
    float m_inputGammaPowDenominator;
    float m_outputGammaPowNumerator;
    float m_outputGammaPowDenominator;
    bool m_generateMipMapChain;

    // Cubemap rotate/flip.
    uint32_t m_imageOpPosX;
    uint32_t m_imageOpPosY;
    uint32_t m_imageOpPosZ;
    uint32_t m_imageOpNegX;
    uint32_t m_imageOpNegY;
    uint32_t m_imageOpNegZ;

    // Filter parameters.
    uint32_t m_filterType;
    uint32_t m_srcFaceSize;
    bool m_excludeBase;
    uint32_t m_mipCount;
    uint32_t m_glossScale;
    uint32_t m_glossBias;
    uint32_t m_dstFaceSize;
    uint32_t m_lightingModel;
    uint32_t m_edgeFixup;

    // Processing devices.
    uint32_t m_numCpuProcessingThreads;
    bool m_useOpenCL;
    uint32_t m_clVendor;
    char m_vendorStrPart[1024];
    uint32_t m_deviceType;
    uint32_t m_deviceIndex;

    // Output.
    uint32_t m_outputFilesNum;
#define MAX_OUTPUT_NUM 16
    OutputFile m_outputFiles[MAX_OUTPUT_NUM];

    // Misc.
    bool m_silent;

    // Encode
    bool m_encodeRGBM;
};

void inputParametersFromCommandLine(InputParameters& _inputParameters, const cmft::CommandLine& _cmdLine)
{
    // Input.
    cmft::stracpy(_inputParameters.m_inputFilePath, _cmdLine.findOption("input"));
    cmft::stracpy(_inputParameters.m_inputPosXFace, _cmdLine.findOption("inputFacePosX"));
    cmft::stracpy(_inputParameters.m_inputNegXFace, _cmdLine.findOption("inputFaceNegX"));
    cmft::stracpy(_inputParameters.m_inputPosYFace, _cmdLine.findOption("inputFacePosY"));
    cmft::stracpy(_inputParameters.m_inputNegYFace, _cmdLine.findOption("inputFaceNegY"));
    cmft::stracpy(_inputParameters.m_inputPosZFace, _cmdLine.findOption("inputFacePosZ"));
    cmft::stracpy(_inputParameters.m_inputNegZFace, _cmdLine.findOption("inputFaceNegZ"));

    // Image Operations.
    _cmdLine.hasArg(_inputParameters.m_inputGammaPowNumerator,    '\0', "inputGamma");
    _cmdLine.hasArg(_inputParameters.m_inputGammaPowNumerator,    '\0', "inputGammaNumerator");
    _cmdLine.hasArg(_inputParameters.m_inputGammaPowDenominator,  '\0', "inputGammaDenominator");
    _cmdLine.hasArg(_inputParameters.m_outputGammaPowNumerator,   '\0', "outputGamma");
    _cmdLine.hasArg(_inputParameters.m_outputGammaPowNumerator,   '\0', "outputGammaNumerator");
    _cmdLine.hasArg(_inputParameters.m_outputGammaPowDenominator, '\0', "outputGammaDenominator");
    _cmdLine.hasArg(_inputParameters.m_generateMipMapChain,       '\0', "generateMipChain");

    // Cubemap rotate/flip.
    _inputParameters.m_imageOpPosX = 0
                                   | (_cmdLine.hasArg("posXrotate90")  ? IMAGE_OP_ROT_90  : 0)
                                   | (_cmdLine.hasArg("posXrotate180") ? IMAGE_OP_ROT_180 : 0)
                                   | (_cmdLine.hasArg("posXrotate270") ? IMAGE_OP_ROT_270 : 0)
                                   | (_cmdLine.hasArg("posXflipH")     ? IMAGE_OP_FLIP_X  : 0)
                                   | (_cmdLine.hasArg("posXflipV")     ? IMAGE_OP_FLIP_Y  : 0)
                                   ;

    _inputParameters.m_imageOpPosY = 0
                                   | (_cmdLine.hasArg("posYrotate90")  ? IMAGE_OP_ROT_90  : 0)
                                   | (_cmdLine.hasArg("posYrotate180") ? IMAGE_OP_ROT_180 : 0)
                                   | (_cmdLine.hasArg("posYrotate270") ? IMAGE_OP_ROT_270 : 0)
                                   | (_cmdLine.hasArg("posYflipH")     ? IMAGE_OP_FLIP_X  : 0)
                                   | (_cmdLine.hasArg("posYflipV")     ? IMAGE_OP_FLIP_Y  : 0)
                                   ;

    _inputParameters.m_imageOpPosZ = 0
                                   | (_cmdLine.hasArg("posZrotate90")  ? IMAGE_OP_ROT_90  : 0)
                                   | (_cmdLine.hasArg("posZrotate180") ? IMAGE_OP_ROT_180 : 0)
                                   | (_cmdLine.hasArg("posZrotate270") ? IMAGE_OP_ROT_270 : 0)
                                   | (_cmdLine.hasArg("posZflipH")     ? IMAGE_OP_FLIP_X  : 0)
                                   | (_cmdLine.hasArg("posZflipV")     ? IMAGE_OP_FLIP_Y  : 0)
                                   ;

    _inputParameters.m_imageOpNegX = 0
                                   | (_cmdLine.hasArg("negXrotate90")  ? IMAGE_OP_ROT_90  : 0)
                                   | (_cmdLine.hasArg("negXrotate180") ? IMAGE_OP_ROT_180 : 0)
                                   | (_cmdLine.hasArg("negXrotate270") ? IMAGE_OP_ROT_270 : 0)
                                   | (_cmdLine.hasArg("negXflipH")     ? IMAGE_OP_FLIP_X  : 0)
                                   | (_cmdLine.hasArg("negXflipV")     ? IMAGE_OP_FLIP_Y  : 0)
                                   ;

    _inputParameters.m_imageOpNegY = 0
                                   | (_cmdLine.hasArg("negYrotate90")  ? IMAGE_OP_ROT_90  : 0)
                                   | (_cmdLine.hasArg("negYrotate180") ? IMAGE_OP_ROT_180 : 0)
                                   | (_cmdLine.hasArg("negYrotate270") ? IMAGE_OP_ROT_270 : 0)
                                   | (_cmdLine.hasArg("negYflipH")     ? IMAGE_OP_FLIP_X  : 0)
                                   | (_cmdLine.hasArg("negYflipV")     ? IMAGE_OP_FLIP_Y  : 0)
                                   ;

    _inputParameters.m_imageOpNegZ = 0
                                   | (_cmdLine.hasArg("negZrotate90")  ? IMAGE_OP_ROT_90  : 0)
                                   | (_cmdLine.hasArg("negZrotate180") ? IMAGE_OP_ROT_180 : 0)
                                   | (_cmdLine.hasArg("negZrotate270") ? IMAGE_OP_ROT_270 : 0)
                                   | (_cmdLine.hasArg("negZflipH")     ? IMAGE_OP_FLIP_X  : 0)
                                   | (_cmdLine.hasArg("negZflipV")     ? IMAGE_OP_FLIP_Y  : 0)
                                   ;

    // Filter type.
    valueFromOptionMap(_inputParameters.m_filterType, s_filterType, _cmdLine.findOption("filter"));

    // Filter parameters.
    _cmdLine.hasArg(_inputParameters.m_srcFaceSize, '\0', "srcFaceSize");
    _cmdLine.hasArg(_inputParameters.m_excludeBase, '\0', "excludeBase");
    _cmdLine.hasArg(_inputParameters.m_mipCount,    '\0', "mipCount");
    _cmdLine.hasArg(_inputParameters.m_glossScale,  '\0', "glossScale");
    _cmdLine.hasArg(_inputParameters.m_glossBias,   '\0', "glossBias");
    _cmdLine.hasArg(_inputParameters.m_dstFaceSize, '\0', "dstFaceSize");

    // Lighting model.
    valueFromOptionMap(_inputParameters.m_lightingModel, s_lightingModel, _cmdLine.findOption("lightingModel"));

    // Edge fixup.
    valueFromOptionMap(_inputParameters.m_edgeFixup, s_edgeFixup, _cmdLine.findOption("edgeFixup"));

    // Processing devices.
    _cmdLine.hasArg(_inputParameters.m_numCpuProcessingThreads, '\0', "numCpuProcessingThreads");
    _cmdLine.hasArg(_inputParameters.m_useOpenCL, '\0', "useOpenCL");

    // Cl vendor.
    uint32_t clVendor = (uint32_t)CMFT_CL_VENDOR_ANY_GPU;
    const char* clVendorOption = _cmdLine.findOption("clVendor");
    if (!valueFromOptionMap(clVendor, s_clVendors, clVendorOption))
    {
        cmft::stracpy(_inputParameters.m_vendorStrPart, clVendorOption);
    }
    _inputParameters.m_clVendor = clVendor;

    // Device type/index.
    valueFromOptionMap(_inputParameters.m_deviceType, s_deviceType, _cmdLine.findOption("deviceType"));
    _cmdLine.hasArg(_inputParameters.m_deviceIndex, '\0', "deviceIndex");

    // Misc.
    _inputParameters.m_silent = _cmdLine.hasArg("silent");

    // Encode
    _inputParameters.m_encodeRGBM = _cmdLine.hasArg("rgbm");

    // Output.
    uint32_t outputCount = 0;
    uint32_t outputEnd = MAX_OUTPUT_NUM;
    _cmdLine.hasArg(outputEnd, '\0', "outputNum");
    outputEnd = CMFT_MIN(outputEnd, uint32_t(MAX_OUTPUT_NUM));
    for (uint8_t outputId = 0; outputId < outputEnd; ++outputId)
    {
        char outputNameOption[16];
        sprintf(outputNameOption, "output%u", outputId);

        char outputParamsOption[32];
        sprintf(outputParamsOption, "output%uparams", outputId);

        const char* outputName = _cmdLine.findOption(outputNameOption);
        if (NULL != outputName)
        {
            // Get file name.
            cmft::stracpy(_inputParameters.m_outputFiles[outputCount].m_fileName, outputName);

            // Set default parameters.
            uint32_t fileType      = ImageFileType::DDS;
            uint32_t textureFormat = TextureFormat::BGRA8;
            uint32_t outputType    = OutputType::LatLong;

            // Override default parameters with user supplied ones.
            const char* outputParams = _cmdLine.findOption(outputParamsOption);
            if (NULL != outputParams)
            {
                char buf[256];
                cmft::stracpy(buf, outputParams);

                const char* fileTypeStr      = strtok(buf,",");
                const char* textureFormatStr = strtok(NULL,",");
                const char* outputTypeStr    = strtok(NULL,",");
                const char* optionalParam0   = strtok(NULL,",");
                const char* optionalParam1   = strtok(NULL,",");

                // Check if present.
                if (NULL == fileTypeStr)
                {
                    WARN("Output(%u) - File type not specified. Defaulting to DDS.", outputId);
                }
                // Check if supported.
                else if (!valueFromOptionMap(fileType, s_validFileTypes, fileTypeStr))
                {
                    char requestedFileType[128];
                    cmft::stracpy(requestedFileType, outputParams);
                    WARN("Output(%u) - File type %s is invalid or not supported by cmft.", outputId, requestedFileType);
                }

                // Check if present.
                if (NULL == textureFormatStr)
                {
                    const TextureFormat::Enum* validTextureFormats = getValidTextureFormats((ImageFileType::Enum)fileType);
                    WARN("Output(%u) - Texture format not specified. Defaulting to %s.", outputId, getTextureFormatStr(validTextureFormats[0]));
                    textureFormat = uint32_t(validTextureFormats[0]);
                }
                // Check if supported.
                else if (!valueFromOptionMap(textureFormat, s_validTextureFormats, textureFormatStr))
                {
                    char requestedTextureFormat[128];
                    cmft::stracpy(requestedTextureFormat, textureFormatStr);

                    WARN("Output(%u) - Requested texture format %s is invalid or not supported by cmft.", outputId, requestedTextureFormat);

                    // Skip this output.
                    continue;
                }
                // Check if valid.
                else if (!checkValidTextureFormat((ImageFileType::Enum)fileType, (TextureFormat::Enum)textureFormat))
                {
                    const ImageFileType::Enum ft = (ImageFileType::Enum)fileType;
                    const TextureFormat::Enum tf = (TextureFormat::Enum)textureFormat;

                    char validInternalFormats[128];
                    getValidTextureFormatsStr(validInternalFormats, ft);

                    WARN("Output(%u) - File type %s does not support %s texture format."
                        " Valid internal formats for %s are: %s."
                        " Choose one of the valid internal formats or a different file type.\n"
                        , outputId, getFileTypeStr(ft), getTextureFormatStr(tf)
                        , getFileTypeStr(ft), validInternalFormats
                        );

                    // Skip this output.
                    continue;
                }

                // Check if present.
                if (NULL == outputTypeStr)
                {
                    WARN("Output(%u) - Texture format not specified. Defaulting to latlong.", outputId);
                }
                // Check if supported.
                else if (!valueFromOptionMap(outputType, s_validOutputTypes, outputTypeStr))
                {
                    char requestedOutputType[128];
                    cmft::stracpy(requestedOutputType, outputTypeStr);

                    WARN("Output(%u) - Requested output type %s is invalid or not supported by cmft.", outputId, requestedOutputType);

                    // Skip this output.
                    continue;
                }
                // Check if valid.
                else if (!checkValidOutputType((ImageFileType::Enum)fileType, (OutputType::Enum)outputType))
                {
                    const ImageFileType::Enum ft = (ImageFileType::Enum)fileType;

                    char requestedOutputType[128];
                    cmft::stracpy(requestedOutputType, outputTypeStr);

                    char validOutputTypes[128];
                    getValidOutputTypesStr(validOutputTypes, ft);

                    WARN("Output(%u) - File type %s does not support %s output type."
                         " Valid output types for %s are: %s."
                         " Choose one of the valid output types or a different file type."
                         , outputId, getFileTypeStr(ft), requestedOutputType
                         , getFileTypeStr(ft), validOutputTypes
                         );

                    // Skip this output.
                    continue;
                }

                // Copy optional parameters if present.
                if (NULL != optionalParam0)
                {
                    cmft::stracpy(_inputParameters.m_outputFiles[outputCount].m_optionalParam0, optionalParam0);
                }

                if (NULL != optionalParam1)
                {
                    cmft::stracpy(_inputParameters.m_outputFiles[outputCount].m_optionalParam1, optionalParam1);
                }
            }

            // Output is valid. Save parameters and move on to next output file.
            _inputParameters.m_outputFiles[outputCount].m_fileType      = fileType;
            _inputParameters.m_outputFiles[outputCount].m_textureFormat = textureFormat;
            _inputParameters.m_outputFiles[outputCount].m_outputType    = outputType;
            outputCount++;
        }
    }
    _inputParameters.m_outputFilesNum = outputCount;
}

void inputParametersDefault(InputParameters& _inputParameters)
{
    // Input.
    _inputParameters.m_inputFilePath[0] = '\0';
    _inputParameters.m_inputPosXFace[0] = '\0';
    _inputParameters.m_inputNegXFace[0] = '\0';
    _inputParameters.m_inputPosYFace[0] = '\0';
    _inputParameters.m_inputNegYFace[0] = '\0';
    _inputParameters.m_inputPosZFace[0] = '\0';
    _inputParameters.m_inputNegZFace[0] = '\0';

    // Output.
    _inputParameters.m_outputFilesNum = 0;

    // Image Operations.
    _inputParameters.m_inputGammaPowNumerator    = 1.0f;
    _inputParameters.m_inputGammaPowDenominator  = 1.0f;
    _inputParameters.m_outputGammaPowNumerator   = 1.0f;
    _inputParameters.m_outputGammaPowDenominator = 1.0f;
    _inputParameters.m_generateMipMapChain       = false;

    // Cubemap rotate/flip.
    _inputParameters.m_imageOpPosX = 0;
    _inputParameters.m_imageOpPosY = 0;
    _inputParameters.m_imageOpPosZ = 0;
    _inputParameters.m_imageOpNegX = 0;
    _inputParameters.m_imageOpNegY = 0;
    _inputParameters.m_imageOpNegZ = 0;

    // Filter parameters.
    _inputParameters.m_filterType    = 0;
    _inputParameters.m_srcFaceSize   = 0;
    _inputParameters.m_excludeBase   = false;
    _inputParameters.m_mipCount      = 9;
    _inputParameters.m_glossScale    = 10;
    _inputParameters.m_glossBias     = 1;
    _inputParameters.m_dstFaceSize   = 0;
    _inputParameters.m_lightingModel = 0;
    _inputParameters.m_edgeFixup     = 0;

    // Processing devices.
    _inputParameters.m_numCpuProcessingThreads = UINT32_MAX;
    _inputParameters.m_useOpenCL               = true;
    _inputParameters.m_deviceIndex             = 0;
    _inputParameters.m_clVendor                = CMFT_CL_VENDOR_ANY_GPU;
    _inputParameters.m_vendorStrPart[0]        = '\0';
    _inputParameters.m_deviceType              = CMFT_CL_DEVICE_TYPE_GPU;

    // Misc.
    _inputParameters.m_silent = false;
    _inputParameters.m_encodeRGBM = false;
}

/// Outputs C file.
void outputShCoeffs(const char* _pathName, double _shCoeffs[SH_COEFF_NUM][3])
{
    // Get base name.
    char baseName[128];
    if (!cmft::basename(baseName, 128, _pathName))
    {
        strcpy(baseName, "cmft");
    }
    baseName[0] = (char)toupper(baseName[0]);

    char baseNameUpper[128];
    cmft::strtoupper(baseNameUpper, baseName);

    // File content.
    char content[10240];
    sprintf(content,
           "#ifndef CMFT_%s_H_HEADER_GUARD\n"
           "#define CMFT_%s_H_HEADER_GUARD\n"
           "\n"
           "static const float s_shCoeffs%s[25][3] =\n"
           "{\n"
           "    /* Band 0 */ { %21.18f, %21.18f, %21.18f },\n"
           "    /* Band 1 */ { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f },\n"
           "    /* Band 2 */ { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f },\n"
           "    /* Band 3 */ { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f },\n"
           "    /* Band 4 */ { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }, { %21.18f, %21.18f, %21.18f }\n"
           "};\n"
           "\n"
           "#endif // CMFT_%s_H_HEADER_GUARD\n"
           , baseNameUpper
           , baseNameUpper
           , baseName
           , _shCoeffs[ 0][0], _shCoeffs[ 0][1], _shCoeffs[ 0][2]
           , _shCoeffs[ 1][0], _shCoeffs[ 1][1], _shCoeffs[ 1][2]
           , _shCoeffs[ 2][0], _shCoeffs[ 2][1], _shCoeffs[ 2][2]
           , _shCoeffs[ 3][0], _shCoeffs[ 3][1], _shCoeffs[ 3][2]
           , _shCoeffs[ 4][0], _shCoeffs[ 4][1], _shCoeffs[ 4][2]
           , _shCoeffs[ 5][0], _shCoeffs[ 5][1], _shCoeffs[ 5][2]
           , _shCoeffs[ 6][0], _shCoeffs[ 6][1], _shCoeffs[ 6][2]
           , _shCoeffs[ 7][0], _shCoeffs[ 7][1], _shCoeffs[ 7][2]
           , _shCoeffs[ 8][0], _shCoeffs[ 8][1], _shCoeffs[ 8][2]
           , _shCoeffs[ 9][0], _shCoeffs[ 9][1], _shCoeffs[ 9][2]
           , _shCoeffs[10][0], _shCoeffs[10][1], _shCoeffs[10][2]
           , _shCoeffs[11][0], _shCoeffs[11][1], _shCoeffs[11][2]
           , _shCoeffs[12][0], _shCoeffs[12][1], _shCoeffs[12][2]
           , _shCoeffs[13][0], _shCoeffs[13][1], _shCoeffs[13][2]
           , _shCoeffs[14][0], _shCoeffs[14][1], _shCoeffs[14][2]
           , _shCoeffs[15][0], _shCoeffs[15][1], _shCoeffs[15][2]
           , _shCoeffs[16][0], _shCoeffs[16][1], _shCoeffs[16][2]
           , _shCoeffs[17][0], _shCoeffs[17][1], _shCoeffs[17][2]
           , _shCoeffs[18][0], _shCoeffs[18][1], _shCoeffs[18][2]
           , _shCoeffs[19][0], _shCoeffs[19][1], _shCoeffs[19][2]
           , _shCoeffs[20][0], _shCoeffs[20][1], _shCoeffs[20][2]
           , _shCoeffs[21][0], _shCoeffs[21][1], _shCoeffs[21][2]
           , _shCoeffs[22][0], _shCoeffs[22][1], _shCoeffs[22][2]
           , _shCoeffs[23][0], _shCoeffs[23][1], _shCoeffs[23][2]
           , _shCoeffs[24][0], _shCoeffs[24][1], _shCoeffs[24][2]
           , baseNameUpper
           );

    // Append *.c extension.
    char filePath[CMFT_PATH_LEN];
    strcpy(filePath, _pathName);
    strcat(filePath, ".c");

    // Open file.
    FILE* fp = fopen(filePath, "wb");
    if (NULL == fp)
    {
        WARN("Could not open file %s for writing.", filePath);
        return;
    }
    cmft::ScopeFclose cleanup(fp);

    // Write content.
    size_t write;
    CMFT_UNUSED(write);
    write = fwrite(&content, strlen(content), 1, fp);
    DEBUG_CHECK(write == 1, "Error writing sh coeffs file content.");
    FERROR_CHECK(fp);
}

void printHelp()
{
    fprintf(stderr
          , "cmft - cubemap filtering tool\n"
            "Copyright 2014-2015 Dario Manesku. All rights reserved.\n"
            "License: http://www.opensource.org/licenses/BSD-2-Clause\n\n"
          );

    fprintf(stderr
          , "Usage: cmft [options]\n"

            "\n"
            "Typical uses:\n"
            "\n"
            "1. Lists available OpenCL devices that can be used with cmft for processing:\n"
            "\n"
            "    cmft --printCLDevices\n"

            "\n"
            "2. Typical parameters for irradiance filter:\n"
            "\n"
            "    cmft --input <file path>\n"
            "         --filter irradiance\n"
            "         --outputNum 1\n"
            "         --output0 <output name>\n"
            "         --output0params dds,bgra8,cubemap\n"

            "\n"
            "3. Typical parameters for generating spherical harmonics coefficients:\n"
            "\n"
            "    cmft --input <file path>\n"
            "         --filter shcoeffs\n"
            "         --outputNum 1\n"
            "         --output0 <output name>\n"

            "\n"
            "4. Typical parameters for radiance filter:\n"
            "\n"
            "    cmft --input <file path>\n"
            "         --filter radiance\n"
            "         --srcFaceSize 256\n"
            "         --excludeBase false\n"
            "         --mipCount 9\n"
            "         --glossScale 10\n"
            "         --glossBias 1\n"
            "         --lightingModel phongbrdf\n"
            "         --dstFaceSize 256\n"
            "         --numCpuProcessingThreads 4\n"
            "         --useOpenCL true\n"
            "         --clVendor anyGpuVendor\n"
            "         --deviceType gpu\n"
            "         --deviceIndex 0\n"
            "         --inputGammaNumerator 1.0\n"
            "         --inputGammaDenominator 1.0\n"
            "         --outputGammaNumerator 1.0\n"
            "         --outputGammaDenominator 1.0\n"
            "         --generateMipChain false\n"
            "         --outputNum 2\n"
            "         --output0 <output name>\n"
            "         --output0params dds,bgra8,cubemap\n"
            "         --output1 <output name>\n"
            "         --output1params ktx,rgba8,cubemap\n"

            "\n"
            "5. Cmft can be used without any filter for performing image manipulations and/or exporting different format(s) and type(s):\n"
            "\n"
            "    cmft --input <file path>\n"
            "         --filter none\n"
            "         --generateMipChain true\n"
            "         --posXrotate90\n"
            "         --posXrotate180\n"
            "         --posXrotate270\n"
            "         --posXflipH\n"
            "         --posXflipV\n"
            "         --negXrotate90\n"
            "         --negXrotate180\n"
            "         --negXrotate270\n"
            "         --negXflipH\n"
            "         --negXflipV\n"
            "         --posYrotate90\n"
            "         --posYrotate180\n"
            "         --posYrotate270\n"
            "         --posYflipH\n"
            "         --posYflipV\n"
            "         --negYrotate90\n"
            "         --negYrotate180\n"
            "         --negYrotate270\n"
            "         --negYflipH\n"
            "         --negYflipV\n"
            "         --posZrotate90\n"
            "         --posZrotate180\n"
            "         --posZrotate270\n"
            "         --posZflipH\n"
            "         --posZflipV\n"
            "         --negZrotate90\n"
            "         --negZrotate180\n"
            "         --negZrotate270\n"
            "         --negZflipH\n"
            "         --negZflipV\n"
            "         --outputGamma 1.0\n"
            "         --outputGammaDenominator 2.2\n"
            "         --outputNum 1\n"
            "         --output0 <output name>\n"
            "         --output0params dds,bgra8,cubemap\n"

            "\n"
            "All options listed:\n"
            "    --help                             Prints this message\n"
            "    --printCLDevices                   Prints OpenCL devices that can be used for processing. Although application allows CPU-type devices to be picked, GPU-type devices are meant to be used as OpenCL devices!\n"
            "    --input <file path>                Input environment map for filtering. Can be *.dds, *.ktx, *.hdr, *.exr, *.tga and in form of: cubemap, latlong image, horizontal or vertical cube cross or image strip.\n"
            "    --inputFacePosX <file path>        Input face +x in case --input is not specified.\n"
            "    --inputFaceNegX <file path>        Input face -x in case --input is not specified.\n"
            "    --inputFacePosY <file path>        Input face +y in case --input is not specified.\n"
            "    --inputFaceNegY <file path>        Input face -y in case --input is not specified.\n"
            "    --inputFacePosZ <file path>        Input face +z in case --input is not specified.\n"
            "    --inputFaceNegZ <file path>        Input face -z in case --input is not specified.\n"
            "    --filter <filter>                  Filter action to be executed.\n"
            "          radiance\n"
            "          irradiance\n"
            "          shCoeffs\n"
            "          none\n"
            "    --srcFaceSize <uint>               Resize input image to <uint>. If <uint> == 0, input face size is left as is.\n"
            "    --dstFaceSize <uint>               Filter output face size. If <uint> == 0, output face size will be same as srcFaceSize.\n"
            "    --excludeBase <bool>               Exclude base image when generating mipmaped radiance cubemap. [radiance filter param]\n"
            "    --mipCount <uint>                  Radiance cubemap mipmap number. Glossiness distribution is uniform. [radiance filter param]\n"
            "    --glossScale <uint>                Equation is glossScale * mipGlossiness + glossBias. [radiance filter param]\n"
            "    --glossBias <uint>                 Equation is glossScale * mipGlossiness + glossBias. [radiance filter param]\n"
            "    --lightingModel <model>            Lighting model that matches game lighting equation. [radiance filter param]\n"
            "          phong\n"
            "          phongbrdf\n"
            "          blinn\n"
            "          blinnbrdf\n"
            "    --edgeFixup <fixup>                DirectX9 and OpenGL without ARB_seamless_cube_map cannot sample cubemap across face edges. In those cases, use 'warp' edge fixup. Otherwise, choose 'none'. Cubemaps filtered with warp edge fixup also require some shader code to be executed at runtime. See 'cmft/include/cubemapfilter.h' for more details. [radiance filter param]\n"
            "          none\n"
            "          warp\n"
            "    --numCpuProcessingThreads <uint>   Should not be bigger than the number of physical CPU cores/threads. [radiance filter param]\n"
            "    --useOpenCL <bool>                 OpenCL processing can be used alongside processing on CPU. Therefore, OpenCL device should be GPU. [radiance filter param]\n"
            "    --clVendor <vendor>                This parameter should generally be 'anyGpuVendor'. If other vendor is to be choosen, type in part of the vendor name. Use 'cmft --printCLDevices' to list available devices and vendors. [radiance filter param]\n"
            "          intel\n"
            "          amd\n"
            "          ati\n"
            "          nvidia\n"
            "          anyGpuVendor\n"
            "          anyCpuVendor\n"
            "          [other]\n"
            "    --deviceType <type>                After selecting vendor, 'deviceType' is considered. If desired 'deviceType' is not present, value is ignored. [radiance filter param]\n"
            "          gpu\n"
            "          cpu\n"
            "          accelerator\n"
            "          default\n"
            "    --deviceIndex <uint>               If there are multiple devices of chosen vendor and type, <uint> is used for selection. There is no support for multiple OpenCL devices for now. [radiance filter param]\n"
            "    --generateMipChain <bool>          After processing, generate entire mip map chain.\n"
            "    --inputGammaNumerator <uint>       Gamma applied to cubemap before processing. Use this field to specify gamma numerator. Gamma equation is value^(numerator/denominator).\n"
            "    --inputGammaDenominator <uint>     Gamma applied to cubemap before processing. Use this field to specify gamma denominator. Gamma equation is value^(numerator/denominator).\n"
            "    --outputGammaNumerator <uint>      Gamma applied to cubemap after processing. Use this field to specify gamma numerator. Gamma equation is value^(numerator/denominator).\n"
            "    --outputGammaDenominator <uint>    Gamma applied to cubemap after processing. Use this field to specify gamma denominator. Gamma equation is value^(numerator/denominator).\n"
            "    --posXrotate90                     Rotate +x input cubemap face by 90 degrees.\n"
            "    --posXrotate180                    Rotate +x input cubemap face by 180 degrees.\n"
            "    --posXrotate270                    Rotate +x input cubemap face by 270 degrees.\n"
            "    --posXflipH                        Horizontal flip +x input cubemap face.\n"
            "    --posXflipV                        Vertical flip +x input cubemap face.\n"
            "    --negXrotate90                     Rotate -x input cubemap face by 90 degrees.\n"
            "    --negXrotate180                    Rotate -x input cubemap face by 180 degrees.\n"
            "    --negXrotate270                    Rotate -x input cubemap face by 270 degrees.\n"
            "    --negXflipH                        Horizontal flip -x input cubemap face.\n"
            "    --negXflipV                        Vertical flip -x input cubemap face.\n"
            "    --posYrotate90                     Rotate +y input cubemap face by 90 degrees.\n"
            "    --posYrotate180                    Rotate +y input cubemap face by 180 degrees.\n"
            "    --posYrotate270                    Rotate +y input cubemap face by 270 degrees.\n"
            "    --posYflipH                        Horizontal flip +y input cubemap face.\n"
            "    --posYflipV                        Vertical flip +y input cubemap face.\n"
            "    --negYrotate90                     Rotate -y input cubemap face by 90 degrees.\n"
            "    --negYrotate180                    Rotate -y input cubemap face by 180 degrees.\n"
            "    --negYrotate270                    Rotate -y input cubemap face by 270 degrees.\n"
            "    --negYflipH                        Horizontal flip -y input cubemap face.\n"
            "    --negYflipV                        Vertical flip -y input cubemap face.\n"
            "    --posZrotate90                     Rotate +z input cubemap face by 90 degrees.\n"
            "    --posZrotate180                    Rotate +z input cubemap face by 180 degrees.\n"
            "    --posZrotate270                    Rotate +z input cubemap face by 270 degrees.\n"
            "    --posZflipH                        Horizontal flip +z input cubemap face.\n"
            "    --posZflipV                        Vertical flip +z input cubemap face.\n"
            "    --negZrotate90                     Rotate -z input cubemap face by 90 degrees.\n"
            "    --negZrotate180                    Rotate -z input cubemap face by 180 degrees.\n"
            "    --negZrotate270                    Rotate -z input cubemap face by 270 degrees.\n"
            "    --negZflipH                        Horizontal flip -z input cubemap face.\n"
            "    --negZflipV                        Vertical flip -z input cubemap face.\n"
            "    --outputNum <N>                    Number of outputs to be considered. Should be equal or bigger than the number of outputs specified. Could be ommited. Default value is 16, maximum 16.\n"
            "    --output[0..N-1] <file name>       File name without extension.\n"
            "    --output[0..N-1]params <params>    Output parameters as following:\n"
            "          <params> = <fileFormat>,<textureFormat>,<outputType>\n"
            "          <fileFromat> = [dds,ktx,tga,hdr]\n"
            "          <dds_textureFormat> = [bgr8,bgra8,rgba16,rgba16f,rgba32f]\n"
            "          <ktx_textureFormat> = [rgb8,rgb16,rgb16f,rgb32f,rgba8,rgba16,rgba16f,rgba32f]\n"
            "          <tga_textureFormat> = [bgr8,bgra8]\n"
            "          <hdr_textureFormat> = [rgbe]\n"
            "          <dds_outputType> = [cubemap,latlong,hcross,vcross,hstrip,vstrip,facelist,octant]\n"
            "          <ktx_outputType> = [cubemap,latlong,hcross,vcross,hstrip,vstrip,facelist,octant]\n"
            "          <tga_outputType> = [latlong,hcross,vcross,hstrip,vstrip,facelist,octant]\n"
            "          <hdr_outputType> = [latlong,hcross,vcross,hstrip,vstrip,facelist,octant]\n"
            "    --silent                           Do not print any output.\n"
            "    --rgbm                             Encode image in RGBM.\n"

            "\n"
            "Command line parameters are case insenitive (except for file names and paths).\n"
            "For additional information, see https://github.com/dariomanesku/cmft\n"
          );
}

int cmftMain(int _argc, char const* const* _argv)
{
    cmft::CommandLine cmdLine(_argc, _argv);

    // Action for --help.
    if (1 == _argc || cmdLine.hasArg('h', "help"))
    {
        printHelp();
        return EXIT_SUCCESS;
    }

    // Action for --printCLDevices.
    if (cmdLine.hasArg("printCLDevices"))
    {
        if (cmft::clLoad())
        {
            clPrintDevices();
            cmft::clUnload();
            return EXIT_SUCCESS;
        }

        WARN("ERROR! OpenCL is not set up properly on the machine.");
        return EXIT_FAILURE;
    }

    #if CMFT_ALWAYS_FLUSH_OUTPUT
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    #endif

    InputParameters inputParameters;
    inputParametersDefault(inputParameters);
    inputParametersFromCommandLine(inputParameters, cmdLine);

    if (0 == inputParameters.m_outputFilesNum)
    {
        WARN("There are no valid specified outputs! Execution will not terminate.");
        return EXIT_FAILURE;
    }

    if (inputParameters.m_silent)
    {
        setWarningPrintf(NULL);
        setInfoPrintf(NULL);
    }

    Image image;
    Image imageFaceList[6];

    bool imageLoaded = false;

    // Load image.
    if (0 != strcmp("", inputParameters.m_inputFilePath))
    {
       imageLoaded = imageLoad   (image, inputParameters.m_inputFilePath, TextureFormat::RGBA32F)
                  || imageLoadStb(image, inputParameters.m_inputFilePath, TextureFormat::RGBA32F)
                   ;
    }
    else
    {
        if (0 != strcmp("", inputParameters.m_inputPosXFace)
        &&  0 != strcmp("", inputParameters.m_inputNegXFace)
        &&  0 != strcmp("", inputParameters.m_inputPosYFace)
        &&  0 != strcmp("", inputParameters.m_inputNegYFace)
        &&  0 != strcmp("", inputParameters.m_inputPosZFace)
        &&  0 != strcmp("", inputParameters.m_inputNegZFace))
        {
            imageLoaded = imageLoad(imageFaceList[0], inputParameters.m_inputPosXFace, TextureFormat::RGBA32F)
                       && imageLoad(imageFaceList[1], inputParameters.m_inputNegXFace, TextureFormat::RGBA32F)
                       && imageLoad(imageFaceList[2], inputParameters.m_inputPosYFace, TextureFormat::RGBA32F)
                       && imageLoad(imageFaceList[3], inputParameters.m_inputNegYFace, TextureFormat::RGBA32F)
                       && imageLoad(imageFaceList[4], inputParameters.m_inputPosZFace, TextureFormat::RGBA32F)
                       && imageLoad(imageFaceList[5], inputParameters.m_inputNegZFace, TextureFormat::RGBA32F)
                       ;

            if (imageLoaded)
            {
                INFO("Assembling cubemap from image list.");
                imageCubemapFromFaceList(image, imageFaceList);
            }

            for (uint8_t ii = 0; ii < 6; ++ii)
            {
                imageUnload(imageFaceList[ii]);
            }
        }
    }

    if (!imageLoaded)
    {
        WARN("Invalid input!\n");
        return EXIT_FAILURE;
    }

    // Assemble cubemap.
    if (!imageIsCubemap(image))
    {
        if (imageIsCubeCross(image))
        {
            INFO("Converting cube cross to cubemap.");
            imageCubemapFromCross(image);
        }
        else if (imageIsLatLong(image))
        {
            INFO("Converting latlong image to cubemap.");
            imageCubemapFromLatLong(image);
        }
        else if (imageIsHStrip(image))
        {
            INFO("Converting hstrip image to cubemap.");
            imageCubemapFromStrip(image);
        }
        else if (imageIsVStrip(image))
        {
            INFO("Converting vstrip image to cubemap.");
            imageCubemapFromStrip(image);
        }
        else if (imageIsOctant(image))
        {
            INFO("Converting octant image to cubemap.");
            imageCubemapFromOctant(image);
        }
        else
        {
            INFO("Image is not cubemap(6 faces), cubecross(ratio 3:4 or 4:3), latlong(ratio 2:1), hstrip(ratio 6:1), vstrip(ration 1:6)");
        }
    }

    if (!imageIsCubemap(image))
    {
        INFO("Conversion failed. Exiting...");
        return EXIT_FAILURE;
    }

    // Resize if requested.
    if (0 != inputParameters.m_srcFaceSize && image.m_width != inputParameters.m_srcFaceSize)
    {
        INFO("Resizing source image from %ux%u to %ux%u."
            , image.m_width
            , image.m_height
            , inputParameters.m_srcFaceSize
            , inputParameters.m_srcFaceSize
            );
        imageResize(image, inputParameters.m_srcFaceSize, inputParameters.m_srcFaceSize);
    }

    // Transform cubemap if requested.
    imageTransform(image
                 , IMAGE_FACE_POSITIVEX | inputParameters.m_imageOpPosX
                 , IMAGE_FACE_NEGATIVEX | inputParameters.m_imageOpNegX
                 , IMAGE_FACE_POSITIVEY | inputParameters.m_imageOpPosY
                 , IMAGE_FACE_NEGATIVEY | inputParameters.m_imageOpNegY
                 , IMAGE_FACE_POSITIVEZ | inputParameters.m_imageOpPosZ
                 , IMAGE_FACE_NEGATIVEZ | inputParameters.m_imageOpNegZ
                 );

    // Apply gamma on input image.
    imageApplyGamma(image, inputParameters.m_inputGammaPowNumerator / inputParameters.m_inputGammaPowDenominator);

    // Filter cubemap.
    if (FilterType::Radiance == inputParameters.m_filterType)
    {
        ClContext* clContext = NULL;

        int32_t clLoaded = 0;
        if (inputParameters.m_useOpenCL)
        {
            // Dynamically load opencl lib.
            clLoaded = cmft::clLoad();
            if (clLoaded)
            {
                clContext = clInit(inputParameters.m_clVendor
                                 , inputParameters.m_deviceType
                                 , inputParameters.m_deviceIndex
                                 );
            }
        }

        // Start filter.
        imageRadianceFilter(image
                          , inputParameters.m_dstFaceSize
                          , (LightingModel::Enum)inputParameters.m_lightingModel
                          , (bool)inputParameters.m_excludeBase
                          , (uint8_t)inputParameters.m_mipCount
                          , (uint8_t)inputParameters.m_glossScale
                          , (uint8_t)inputParameters.m_glossBias
                          , (EdgeFixup::Enum)inputParameters.m_edgeFixup
                          , (int8_t)inputParameters.m_numCpuProcessingThreads
                          , clContext
                          );

        clDestroy(clContext);

        // Unload opencl lib.
        if (clLoaded)
        {
            cmft::clUnload();
        }
    }
    else if (FilterType::Irradiance == inputParameters.m_filterType)
    {
        imageIrradianceFilterSh(image, inputParameters.m_dstFaceSize);
    }
    else if (FilterType::ShCoeffs == inputParameters.m_filterType)
    {
        double shCoeffs[SH_COEFF_NUM][3];
        imageShCoeffs(shCoeffs, image);

        for (uint32_t ii = 0; ii < inputParameters.m_outputFilesNum; ++ii)
        {
            const char* fileName = inputParameters.m_outputFiles[ii].m_fileName;
            INFO("Saving spherical harmonics coefficients to %s.c", fileName);
            outputShCoeffs(fileName, shCoeffs);
        }

        INFO("Done.");
        return EXIT_SUCCESS;
    }
    else if (FilterType::None == inputParameters.m_filterType)
    {
        if (0 != inputParameters.m_dstFaceSize && image.m_width != inputParameters.m_dstFaceSize)
        {
            INFO("Resizing destination image from %ux%u to %ux%u."
                , image.m_width
                , image.m_height
                , inputParameters.m_dstFaceSize
                , inputParameters.m_dstFaceSize
                );
            imageResize(image, inputParameters.m_dstFaceSize, inputParameters.m_dstFaceSize);
        }
    }

    // Generate mip map chain if requested.
    if (inputParameters.m_generateMipMapChain)
    {
        imageGenerateMipMapChain(image);
    }

    // Apply gamma on output image.
    imageApplyGamma(image, inputParameters.m_outputGammaPowNumerator / inputParameters.m_outputGammaPowDenominator);

    // Encode RGBM (using --rgbm arg)
    if (inputParameters.m_encodeRGBM)
    {
        INFO("Encoding RGBM");
        imageEncodeRGBM(image);
    }

    // Save output images.
    for (uint32_t outputIdx = 0; outputIdx < inputParameters.m_outputFilesNum; ++outputIdx)
    {
        const OutputFile& output = inputParameters.m_outputFiles[outputIdx];

        OutputType::Enum    ot = (   OutputType::Enum)output.m_outputType;
        ImageFileType::Enum ft = (ImageFileType::Enum)output.m_fileType;
        TextureFormat::Enum tf = (TextureFormat::Enum)output.m_textureFormat;

        // Encode RGBM (using texture format)
        if( tf == TextureFormat::RGBM )
        {
            INFO("Encoding RGBM");
            imageEncodeRGBM(image);
            tf = TextureFormat::BGRA8;	// Change file format to BGRA8 for saving
        }

        imageSave(image, output.m_fileName, ft, ot, tf, true);
    }

    // Cleanup.
    imageUnload(image);

    INFO("Done.");
    return EXIT_SUCCESS;
}

#endif //CMFT_CMFT_CLI_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
