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

		const std::string& getRendererName() const;
		size_t getPasses() const;
		size_t getPixelSamples() const;
		size_t getAOSamples() const;
		bool isDenoiserFound() const;
		bool isDenoiserEnabled() const;

		void setRendererName(const std::string&);
		void setPasses(size_t);
		void setPixelSamples(size_t);
		void setAOSamples(size_t);
		void setDenoiserFound(bool);
		void setDenoiserEnabled(bool);

		///@}

		//! \name Rendering
		///@{

		//! Set the render size.
		//! \param Size of the render window.
		//! \param Rectangle within the window to render.
		void setRenderSize(
			const ospcommon::math::vec2i& window,
			const ospcommon::math::box2i& rectangle);

		//! Convert the Rhino document to OSPRay.
		//void convert(CRhinoDoc*);
		void setWorld(
			const ospray::cpp::World&,
			const ospray::cpp::Camera&);

		//! Initialize rendering.
		void initRender(IRhRdkRenderWindow&);

		//! Render a pass.
		void renderPass(size_t, IRhRdkRenderWindow&);

		///@}

	private:
        void _initFrameBuffer(const ospcommon::math::vec2i&);

		std::string _rendererName = "scivis";
		size_t _passes = 8;
		size_t _pixelSamples = 1;
		size_t _aoSamples = 16;
		bool _denoiserFound = false;
		bool _denoiserEnabled = true;

		ospcommon::math::vec2i _windowSize;
		ospcommon::math::box2i _renderRect;
        ospcommon::math::vec2i _frameBufferSize;

		ospray::cpp::World _world;
		ospray::cpp::Camera _camera;
		ospray::cpp::Renderer _renderer;
		ospray::cpp::FrameBuffer _frameBuffer;
	};

} // namespace Osprey

