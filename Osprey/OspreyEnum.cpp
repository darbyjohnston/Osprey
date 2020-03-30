#include "stdafx.h"
#include "OspreyEnum.h"

namespace Osprey
{
    std::vector<std::string> getRendererValues()
    {
        return std::vector<std::string>
        {
            "pathtracer",
            "scivis",
            //"debug"
        };
    }

    std::vector<std::wstring> getRendererLabels()
    {
        return std::vector<std::wstring>
        {
            L"Path Tracer",
            L"SciVis",
            //L"Debug"
        };
    }

    std::vector<size_t> getPassesValues()
    {
        return std::vector<size_t>
        {
            1,
            4,
            8,
            16,
            32,
            64,
            128,
            256
        };
    }

    std::vector<std::wstring> getPassesLabels()
    {
        return std::vector<std::wstring>
        {
            L"1",
            L"4",
            L"8",
            L"16",
            L"32",
            L"64",
            L"128",
            L"256"
        };
    }

    std::vector<size_t> getPixelSamplesValues()
    {
        return std::vector<size_t>
        {
            1,
            4,
            8,
            16,
            32,
            64,
            128,
            256
        };
    }

    std::vector<std::wstring> getPixelSamplesLabels()
    {
        return std::vector<std::wstring>
        {
            L"1",
            L"4",
            L"8",
            L"16",
            L"32",
            L"64",
            L"128",
            L"256"
        };
    }

    std::vector<size_t> getAOSamplesValues()
    {
        return std::vector<size_t>
        {
            1,
            4,
            8,
            16,
            32,
            64,
            128,
            256
        };
    }

    std::vector<std::wstring> getAOSamplesLabels()
    {
        return std::vector<std::wstring>
        {
            L"1",
            L"4",
            L"8",
            L"16",
            L"32",
            L"64",
            L"128",
            L"256"
        };
    }

} // namespace Osprey
