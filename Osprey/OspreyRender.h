// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

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

		//! \name Settings
		///@{

        //! Set the renderer name (e.g., "pathtracer", "scivis", "debug").
		void setRendererName(const std::string&);

        //! Set the number of low resolution preview passes.
        void setPreviewPasses(size_t);

        //! Set the number of pixel samples.
		void setPixelSamples(size_t);

        //! Set the number of ambient occlusion samples.
		void setAOSamples(size_t);

        //! Set whether the denoiser module was found.
		void setDenoiserFound(bool);

        //! Set whether the denoise is enabled.
		void setDenoiserEnabled(bool);

        //! Set whether the tone mapper is enabled.
        void setToneMapperEnabled(bool);

        //! Set the tone mapper exposure.
        void setToneMapperExposure(float);

        //! Set whether to flip the y-axis of the frame buffer.
        void setFlipY(bool);

		///@}

		//! \name Rendering
		///@{

		//! Initialize rendering.
        //! \param Size of the render window.
        //! \param Rectangle within the window to render.
        void initRender(
            const ospcommon::math::vec2i& window,
            const ospcommon::math::box2i& rectangle);

		//! Render a pass.
		void renderPass(
            size_t pass,
            const std::shared_ptr<ospray::cpp::World>&,
            const std::shared_ptr<ospray::cpp::Camera>&,
            IRhRdkRenderWindow&);

		///@}

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

		std::string _rendererName = "scivis";
        size_t _previewPasses = 0;
		size_t _pixelSamples = 1;
		size_t _aoSamples = 1;
		bool _denoiserFound = false;
		bool _denoiserEnabled = true;
        bool _toneMapperEnabled = false;
        float _toneMapperExposure = 1.F;
        bool _flipY = false;

		ospcommon::math::vec2i _windowSize;
		ospcommon::math::box2i _renderRect;
        ospcommon::math::vec2i _frameBufferSize;

		ospray::cpp::Renderer _renderer;
		std::vector<ospray::cpp::FrameBuffer> _frameBuffers;
        std::vector<ospcommon::math::vec2i> _frameBuffersSizes;
        std::vector<float> _frameBufferTemp;
	};

} // namespace Osprey

