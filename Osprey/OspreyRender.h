// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyData.h"

namespace Osprey
{
	//! This class provides the interface between Rhino and the OSPRay renderer.
	class Render
	{
		Render();
		Render(const Render&) = delete;
		Render & operator = (const Render&) = delete;

	public:
		// Create a new instance.
		static std::shared_ptr<Render> create();

		//! Initialize rendering.
        //! \param Size of the render window.
        //! \param Rectangle within the window to render.
        void init(const Options&, const std::shared_ptr<Scene>&);

        //! Render a pass.
		void render(size_t pass, IRhRdkRenderWindow&);

	private:
        void _initFrameBuffers(const ospcommon::math::vec2i&);

        static void _scale(
            const float* in,
            const ospcommon::math::vec2i& inSize,
            float* out,
            const ospcommon::math::vec2i& outSize,
            int scale,
            bool flipY);

        static void _flipImage(
            const float* in,
            float* out,
            const ospcommon::math::vec2i& size);

        Options _options;
        std::shared_ptr<Scene> _scene;
		ospray::cpp::Renderer _renderer;
        ospcommon::math::vec2i _frameBufferSize;
        std::vector<ospray::cpp::FrameBuffer> _frameBuffers;
        std::vector<ospcommon::math::vec2i> _frameBuffersSizes;
        std::vector<float> _frameBufferTemp;
	};

} // namespace Osprey

