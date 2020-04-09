// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyEnum.h"

namespace Osprey
{
    OSPREY_ENUM_HELPER_DEF(Renderer);

    std::string getRendererValue(Renderer value)
    {
        return std::vector<std::string>
        {
            "pathtracer",
            "scivis",
            "debug"
        }
            [static_cast<size_t>(value)];
    }

    bool getRendererSupportsMaterials(Renderer value)
    {
        return std::vector<bool>
        {
            true,
            true,
            false
        }
            [static_cast<size_t>(value)];
    }

    std::wstring getRendererLabel(Renderer value)
    {
        return std::vector<std::wstring>
        {
            L"Path Tracer",
            L"SciVis",
            L"Debug"
        }
            [static_cast<size_t>(value)];
    }

    OSPREY_ENUM_HELPER_DEF(Passes);

    size_t getPassesValue(Passes value)
    {
        return std::vector<size_t>
        {
            1,
            2,
            4,
            8,
            16,
            32,
            64,
            128,
            256
        }
            [static_cast<size_t>(value)];
    }

    std::wstring getPassesLabel(Passes value)
    {
        return std::vector<std::wstring>
        {
            L"1",
            L"2",
            L"4",
            L"8",
            L"16",
            L"32",
            L"64",
            L"128",
            L"256"
        }
            [static_cast<size_t>(value)];
    }

    OSPREY_ENUM_HELPER_DEF(PreviewPasses);

    size_t getPreviewPassesValue(PreviewPasses value)
    {
        return std::vector<size_t>
        {
            0,
            1,
            2,
            4,
            8,
            16
        }
            [static_cast<size_t>(value)];
    }

    std::wstring getPreviewPassesLabel(PreviewPasses value)
    {
        return std::vector<std::wstring>
        {
            L"0",
            L"1",
            L"2",
            L"4",
            L"8",
            L"16"
        }
            [static_cast<size_t>(value)];
    }

    OSPREY_ENUM_HELPER_DEF(PixelSamples);

    size_t getPixelSamplesValue(PixelSamples value)
    {
        return std::vector<size_t>
        {
            1,
            2,
            4,
            8,
            16,
            32,
            64,
            128,
            256
        }
            [static_cast<size_t>(value)];
    }

    std::wstring getPixelSamplesLabel(PixelSamples value)
    {
        return std::vector<std::wstring>
        {
            L"1",
            L"2",
            L"4",
            L"8",
            L"16",
            L"32",
            L"64",
            L"128",
            L"256"
        }
            [static_cast<size_t>(value)];
    }

    OSPREY_ENUM_HELPER_DEF(AOSamples);

    size_t getAOSamplesValue(AOSamples value)
    {
        return std::vector<size_t>
        {
            1,
            2,
            4,
            8,
            16,
            32,
            64,
            128,
            256
        }
            [static_cast<size_t>(value)];
    }

    std::wstring getAOSamplesLabel(AOSamples value)
    {
        return std::vector<std::wstring>
        {
            L"1",
            L"2",
            L"4",
            L"8",
            L"16",
            L"32",
            L"64",
            L"128",
            L"256"
        }
            [static_cast<size_t>(value)];
    }

    OSPREY_ENUM_HELPER_DEF(Exposure);

    float getExposureValue(Exposure value)
    {
        return std::vector<float>
        {
            .5F,
            1.F,
            2.F,
            4.F,
            8.F
        }
        [static_cast<size_t>(value)];
    }

    std::wstring getExposureLabel(Exposure value)
    {
        return std::vector<std::wstring>
        {
            L"0.5",
            L"1.0",
            L"2.0",
            L"4.0",
            L"8.0"
        }
        [static_cast<size_t>(value)];
    }

} // namespace Osprey
