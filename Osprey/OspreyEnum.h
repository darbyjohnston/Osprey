#pragma once

namespace Osprey
{
    enum class Renderer
    {
        PathTracer,
        SciVis,
        Debug,

        Count,
        First = PathTracer
    };
    std::vector<std::string> getRendererValues();
    std::vector<std::wstring> getRendererLabels();

    enum class Passes
    {
        _1,
        _4,
        _8,
        _16,
        _32,
        _64,
        _128,
        _256,

        Count,
        First = _1
    };
    std::vector<size_t> getPassesValues();
    std::vector<std::wstring> getPassesLabels();

    enum class PixelSamples
    {
        _1,
        _4,
        _8,
        _16,
        _32,
        _64,
        _128,
        _256,

        Count,
        First = _1
    };
    std::vector<size_t> getPixelSamplesValues();
    std::vector<std::wstring> getPixelSamplesLabels();

    enum class AOSamples
    {
        _1,
        _4,
        _8,
        _16,
        _32,
        _64,
        _128,
        _256,

        Count,
        First = _1
    };
    std::vector<size_t> getAOSamplesValues();
    std::vector<std::wstring> getAOSamplesLabels();

} // namespace Osprey
