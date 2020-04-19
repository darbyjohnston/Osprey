// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#define OSPREY_ENUM_HELPER(T) \
    std::vector<T> get##T##Enums()

#define OSPREY_ENUM_HELPER_DEF(T) \
    std::vector<T> get##T##Enums() \
    { \
        std::vector<T> out; \
        for (size_t i = static_cast<size_t>(T::First); i <  static_cast<size_t>(T::Count); ++i) \
        { \
            out.push_back(static_cast<T>(i)); \
        } \
        return out; \
    }

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
    OSPREY_ENUM_HELPER(Renderer);
    std::string getRendererValue(Renderer);
    bool getRendererSupportsMaterials(Renderer);
    std::wstring getRendererLabel(Renderer);

    enum class Passes
    {
        _1,
        _2,
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
    OSPREY_ENUM_HELPER(Passes);
    size_t getPassesValue(Passes);
    std::wstring getPassesLabel(Passes);

    enum class PreviewPasses
    {
        _0,
        _1,
        _2,
        _4,
        _8,
        _16,

        Count,
        First = _0
    };
    OSPREY_ENUM_HELPER(PreviewPasses);
    size_t getPreviewPassesValue(PreviewPasses);
    std::wstring getPreviewPassesLabel(PreviewPasses);

    enum class PixelSamples
    {
        _1,
        _2,
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
    OSPREY_ENUM_HELPER(PixelSamples);
    size_t getPixelSamplesValue(PixelSamples);
    std::wstring getPixelSamplesLabel(PixelSamples);

    enum class AOSamples
    {
        _1,
        _2,
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
    OSPREY_ENUM_HELPER(AOSamples);
    size_t getAOSamplesValue(AOSamples);
    std::wstring getAOSamplesLabel(AOSamples);

    enum class Exposure
    {
        _0_5,
        _1_0,
        _2_0,
        _4_0,
        _8_0,

        Count,
        First = _0_5
    };
    OSPREY_ENUM_HELPER(Exposure);
    float getExposureValue(Exposure);
    std::wstring getExposureLabel(Exposure);

    enum class BackgroundType
    {
        Solid,
        Image,
        Gradient,
        Environment
    };

} // namespace Osprey
