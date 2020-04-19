// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"

namespace Osprey
{
    struct Options
    {
        std::string rendererName = "scivis";
        bool supportsMaterials = true;
        size_t previewPasses = 0;
        size_t passes = 0;
        size_t pixelSamples = 1;
        size_t aoSamples = 1;
        bool denoiserFound = false;
        bool denoiserEnabled = true;
        bool toneMapperEnabled = false;
        float toneMapperExposure = 1.F;
        bool flipY = false;
    };

    struct Update
    {
        bool update = false;
        std::condition_variable cv;
        std::mutex mutex;
    };

    struct Background
    {
        BackgroundType type;
        ospcommon::math::vec4f color;
        ospcommon::math::vec4f color2;
    };

    struct Scene
    {
        Background background;
        ospray::cpp::World world;
        ospray::cpp::Camera camera;
        ospcommon::math::vec2i renderSize = { 0, 0 };
        ospcommon::math::box2i renderRect = { { 0, 0 }, { 0, 0 } };
    };

} // namespace Osprey
