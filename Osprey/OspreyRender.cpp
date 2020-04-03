// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyRender.h"

#include "OspreyUtil.h"

namespace Osprey
{
	namespace
	{
        // Background color.
        const ospcommon::math::vec3f backgroundColor(65 / 255.F, 100 / 255.F, 170 / 255.F);

	} // namespace

    Render::Render()
    {
        _renderer = ospray::cpp::Renderer(_rendererName);
    }

	std::shared_ptr<Render> Render::create()
	{
		return std::shared_ptr<Render>(new Render);
	}

	const std::string& Render::getRendererName() const
	{
		return _rendererName;
	}

	size_t Render::getPasses() const
	{
		return _passes;
	}

	size_t Render::getPixelSamples() const
	{
		return _pixelSamples;
	}

	size_t Render::getAOSamples() const
	{
		return _aoSamples;
	}

	bool Render::isDenoiserFound() const
	{
		return _denoiserFound;
	}

	bool Render::isDenoiserEnabled() const
	{
		return _denoiserEnabled;
	}

	void Render::setRendererName(const std::string& value)
	{
        if (value == _rendererName)
            return;
		_rendererName = value;
        _renderer = ospray::cpp::Renderer(_rendererName);
	}

	void Render::setPasses(size_t value)
	{
		_passes = value;
	}

	void Render::setPixelSamples(size_t value)
	{
		_pixelSamples = value;
	}

	void Render::setAOSamples(size_t value)
	{
		_aoSamples = value;
	}

	void Render::setDenoiserFound(bool value)
	{
		_denoiserFound = value;
	}

	void Render::setDenoiserEnabled(bool value)
	{
		_denoiserEnabled = value;
	}

	void Render::setRenderSize(
		const ospcommon::math::vec2i& window,
		const ospcommon::math::box2i& rectangle)
	{
		_windowSize = window;
		_renderRect = rectangle;
	}

	void Render::setWorld(
		const ospray::cpp::World& world,
		const ospray::cpp::Camera& camera)
	{
		_world = world;
		_camera = camera;
	}

	void Render::initRender(IRhRdkRenderWindow& rhinoRenderWindow)
	{
        _camera.setParam("aspect", _windowSize.x / static_cast<float>(_windowSize.y));
        ospcommon::math::vec2f imageStart(
            _renderRect.lower.x / static_cast<float>(_windowSize.x - 1),
            1.F - (_renderRect.upper.y / static_cast<float>(_windowSize.y - 1)));
        ospcommon::math::vec2f imageEnd(
            _renderRect.upper.x / static_cast<float>(_windowSize.x - 1),
            1.F - (_renderRect.lower.y / static_cast<float>(_windowSize.y - 1)));
        _camera.setParam("imageStart", imageStart);
        _camera.setParam("imageEnd", imageEnd);
        _camera.commit();

		_renderer.setParam("pixelSamples", static_cast<int>(_pixelSamples));
		_renderer.setParam("aoSamples", static_cast<int>(_aoSamples));
		_renderer.setParam("backgroundColor", backgroundColor);
		_renderer.commit();

        const ospcommon::math::vec2i& renderSize = _renderRect.size();
        rhinoRenderWindow.SetSize(Osprey::toRhino(renderSize));
        _initFrameBuffer(renderSize);
        _frameBuffer.clear();
    }

	void Render::renderPass(size_t pass, IRhRdkRenderWindow& rhinoRenderWindow)
	{
		// Show progress message.
		ON_wString s = ON_wString::FormatToString(L"Rendering pass %d...", pass + 1);
		rhinoRenderWindow.SetProgress(s, static_cast<int>(pass / static_cast<float>(_passes) * 100));

		// Render a frame.
		_frameBuffer.renderFrame(_renderer, _camera, _world);

		// Copy the RGBA channels to Rhino.
		const ospcommon::math::vec2i& renderSize = _renderRect.size();
		IRhRdkRenderWindow::IChannel* pChanRGBA = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanRGBA);
		if (pChanRGBA)
		{
			void* fb = _frameBuffer.map(OSP_FB_COLOR);
			float* fbP = reinterpret_cast<float*>(fb);
			for (int y = 0; y < renderSize.y; ++y)
			{
				for (int x = 0; x < renderSize.x; ++x)
				{
					pChanRGBA->SetValue(x, renderSize.y - 1 - y, ComponentOrder::RGBA, fbP);
					fbP += 4;
				}
			}
			_frameBuffer.unmap(fb);
			pChanRGBA->Close();
		}

		// Copy the depth channel to Rhino.
		IRhRdkRenderWindow::IChannel* pChanDepth = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanDistanceFromCamera);
		if (pChanDepth)
		{
			void* fb = _frameBuffer.map(OSP_FB_DEPTH);
			float* fbP = reinterpret_cast<float*>(fb);
			for (int y = 0; y < renderSize.y; ++y)
			{
				for (int x = 0; x < renderSize.x; ++x)
				{
					pChanDepth->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
				}
			}
			_frameBuffer.unmap(fb);
			pChanDepth->Close();
		}

		// Copy the normal channels to Rhino.
		IRhRdkRenderWindow::IChannel* pChanNormalX = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalX);
		IRhRdkRenderWindow::IChannel* pChanNormalY = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalY);
		IRhRdkRenderWindow::IChannel* pChanNormalZ = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalZ);
		if (pChanNormalX && pChanNormalY && pChanNormalZ)
		{
			void* fb = _frameBuffer.map(OSP_FB_NORMAL);
			float* fbP = reinterpret_cast<float*>(fb);
			for (int y = 0; y < renderSize.y; ++y)
			{
				for (int x = 0; x < renderSize.x; ++x)
				{
					pChanNormalX->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
					pChanNormalY->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
					pChanNormalZ->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
				}
			}
			_frameBuffer.unmap(fb);
		}
		if (pChanNormalX || pChanNormalY || pChanNormalZ)
		{
			pChanNormalX->Close();
			pChanNormalY->Close();
			pChanNormalZ->Close();
		}

		rhinoRenderWindow.Invalidate();

        if (pass == _passes - 1)
        {
            rhinoRenderWindow.SetProgress("Render finished.", 100);
        }
    }

    void Render::_initFrameBuffer(const ospcommon::math::vec2i& size)
    {
        if (size == _frameBufferSize)
            return;

        _frameBufferSize = size;

        _frameBuffer = ospray::cpp::FrameBuffer(
            _frameBufferSize,
            OSP_FB_RGBA32F,
            OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM | OSP_FB_VARIANCE | OSP_FB_NORMAL | OSP_FB_ALBEDO);

        std::vector<ospray::cpp::ImageOperation> imageOps;
        if (0)
        {
            ospray::cpp::ImageOperation tonemapper("tonemapper");
            //tonemapper.setParam("exposure", .5F);
            tonemapper.commit();
            imageOps.push_back(tonemapper);
        }
        if (_denoiserFound && _denoiserEnabled)
        {
            // Add the denoiser operation.
            ospray::cpp::ImageOperation denoiser("denoiser");
            denoiser.commit();
            imageOps.push_back(denoiser);
        }
        if (imageOps.size())
        {
            _frameBuffer.setParam("imageOperation", ospray::cpp::Data(imageOps));
        }

        _frameBuffer.commit();
    }

} // namespace Osprey
