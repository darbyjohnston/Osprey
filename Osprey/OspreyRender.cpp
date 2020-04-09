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
        _renderer.setParam("maxPathLength", 1);
    }

	std::shared_ptr<Render> Render::create()
	{
		return std::shared_ptr<Render>(new Render());
	}

	void Render::setRendererName(const std::string& value)
	{
        if (value == _rendererName)
            return;
		_rendererName = value;
        _frameBufferSize = ospcommon::math::vec2i();
        _renderer = ospray::cpp::Renderer(_rendererName);
	}

    void Render::setPreviewPasses(size_t value)
    {
        if (value == _previewPasses)
            return;
        _previewPasses = value;
        _frameBufferSize = ospcommon::math::vec2i();
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
        if (_denoiserEnabled == value)
            return;
        _denoiserEnabled = value;
        _frameBufferSize = ospcommon::math::vec2i();
    }

    void Render::setToneMapperEnabled(bool value)
    {
        if (_toneMapperEnabled == value)
            return;
        _toneMapperEnabled = value;
        _frameBufferSize = ospcommon::math::vec2i();
    }

    void Render::setToneMapperExposure(float value)
    {
        if (_toneMapperExposure == value)
            return;
        _toneMapperExposure = value;
        _frameBufferSize = ospcommon::math::vec2i();
    }

    void Render::setFlipY(bool value)
    {
        if (_flipY == value)
            return;
        _flipY = value;
        _frameBufferSize = ospcommon::math::vec2i();
    }

	void Render::initRender(
        const ospcommon::math::vec2i& windowSize,
        const ospcommon::math::box2i& renderRect)
	{
        _windowSize = windowSize;
        _renderRect = renderRect;

		_renderer.setParam("pixelSamples", static_cast<int>(_pixelSamples));
		_renderer.setParam("aoSamples", static_cast<int>(_aoSamples));
		_renderer.setParam("backgroundColor", backgroundColor);
		_renderer.commit();

        _initFrameBuffers(_renderRect.size());
    }

	void Render::renderPass(
        size_t pass,
        const std::shared_ptr<ospray::cpp::World>& world,
        const std::shared_ptr<ospray::cpp::Camera>& camera,
        IRhRdkRenderWindow& rdkRenderWindow)
	{
        // Setup the camera.
        if (camera)
        {
            camera->setParam("aspect", _windowSize.x / static_cast<float>(_windowSize.y));
            ospcommon::math::vec2f imageStart(
                _renderRect.lower.x / static_cast<float>(_windowSize.x - 1),
                1.F - (_renderRect.upper.y / static_cast<float>(_windowSize.y - 1)));
            ospcommon::math::vec2f imageEnd(
                _renderRect.upper.x / static_cast<float>(_windowSize.x - 1),
                1.F - (_renderRect.lower.y / static_cast<float>(_windowSize.y - 1)));
            camera->setParam("imageStart", imageStart);
            camera->setParam("imageEnd", imageEnd);
            camera->commit();
        }

		// Render a frame.
        size_t index = std::min(pass, _frameBuffers.size() - 1);
        if (camera && world)
        {
            _frameBuffers[index].renderFrame(_renderer, *camera, *world);
        }

		// Copy the RGBA channels to Rhino.
		const ospcommon::math::vec2i& renderSize = _renderRect.size();
		IRhRdkRenderWindow::IChannel* pChanRGBA = rdkRenderWindow.OpenChannel(IRhRdkRenderWindow::chanRGBA);
		if (pChanRGBA)
		{
			void* fb = _frameBuffers[index].map(OSP_FB_COLOR);
			float* fbP = reinterpret_cast<float*>(fb);
            const size_t scale = renderSize.x / _frameBuffersSizes[index].x;
            if (scale > 1)
            {
                _scale(fbP, _frameBuffersSizes[index], _frameBufferTemp.data(), renderSize, scale, _flipY);
                pChanRGBA->SetValueRect(
                    0,
                    0,
                    renderSize.x,
                    renderSize.y,
                    renderSize.x * 4 * sizeof(float),
                    ComponentOrder::RGBA,
                    _frameBufferTemp.data());
            }
            else if (_flipY)
            {
                _flipImage(fbP, _frameBufferTemp.data(), _frameBuffersSizes[index]);
                pChanRGBA->SetValueRect(
                    0,
                    0,
                    renderSize.x,
                    renderSize.y,
                    renderSize.x * 4 * sizeof(float),
                    ComponentOrder::RGBA,
                    _frameBufferTemp.data());
            }
            else
            {
                pChanRGBA->SetValueRect(
                    0,
                    0,
                    _frameBuffersSizes[index].x,
                    _frameBuffersSizes[index].y,
                    _frameBuffersSizes[index].x * 4 * sizeof(float),
                    ComponentOrder::RGBA,
                    fbP);
            }
			_frameBuffers[index].unmap(fb);
			pChanRGBA->Close();
		}

		// Copy the depth channel to Rhino.
		/*IRhRdkRenderWindow::IChannel* pChanDepth = rdkRenderWindow.OpenChannel(IRhRdkRenderWindow::chanDistanceFromCamera);
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
		}*/

		// Copy the normal channels to Rhino.
		/*IRhRdkRenderWindow::IChannel* pChanNormalX = rdkRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalX);
		IRhRdkRenderWindow::IChannel* pChanNormalY = rdkRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalY);
		IRhRdkRenderWindow::IChannel* pChanNormalZ = rdkRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalZ);
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
		}*/

		rdkRenderWindow.Invalidate();
    }

    void Render::_initFrameBuffers(const ospcommon::math::vec2i& size)
    {
        for (auto& i : _frameBuffers)
        {
            i.clear();
        }

        if (size == _frameBufferSize)
            return;

        _frameBufferSize = size;

        const size_t totalPasses = _previewPasses + 1;
        _frameBuffers.resize(totalPasses);
        _frameBuffersSizes.resize(totalPasses);
        for (size_t i = 0; i < totalPasses; ++i)
        {
            auto frameBufferSize = _frameBufferSize / (totalPasses - i);
            _frameBuffers[i] = ospray::cpp::FrameBuffer(
                frameBufferSize,
                OSP_FB_RGBA32F,
                OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM | OSP_FB_VARIANCE | OSP_FB_NORMAL | OSP_FB_ALBEDO);
            _frameBuffersSizes[i] = frameBufferSize;

            std::vector<ospray::cpp::ImageOperation> imageOps;
            if (_toneMapperEnabled)
            {
                // Add the tone mapper operation.
                ospray::cpp::ImageOperation tonemapper("tonemapper");
                tonemapper.setParam("exposure", _toneMapperExposure);
                tonemapper.commit();
                imageOps.emplace_back(tonemapper);
            }
            if (_denoiserFound && _denoiserEnabled && i == totalPasses - 1)
            {
                // Add the denoiser operation.
                ospray::cpp::ImageOperation denoiser("denoiser");
                denoiser.commit();
                imageOps.emplace_back(denoiser);
            }
            if (imageOps.size())
            {
                _frameBuffers[i].setParam("imageOperation", ospray::cpp::Data(imageOps));
            }

            _frameBuffers[i].commit();
        }

        if (_previewPasses > 0 || _flipY)
        {
            _frameBufferTemp.resize(_frameBufferSize.x * _frameBufferSize.y * 4);
        }
        else
        {
            _frameBufferTemp.clear();
        }
    }

    void Render::_scale(
        const float* in,
        const ospcommon::math::vec2i& inSize,
        float* out,
        const ospcommon::math::vec2i& outSize,
        int scale,
        bool flipY)
    {
        for (int y = 0; y < outSize.y; ++y)
        {
            float* outP = out + y * outSize.x * 4;
            for (int x = 0; x < outSize.x; ++x)
            {
                const int xx = std::min(x / scale, inSize.x - 1);
                const int yy = std::min(y / scale, inSize.y - 1);
                const float* inP = in + (flipY ? inSize.y - 1 - yy : yy) * inSize.x * 4 + xx * 4;
                outP[0] = inP[0];
                outP[1] = inP[1];
                outP[2] = inP[2];
                outP[3] = inP[3];
                outP += 4;
            }
        }
    }

    void Render::_flipImage(
        const float* in,
        float* out,
        const ospcommon::math::vec2i& size)
    {
        for (int y = 0; y < size.y; ++y)
        {
            const float* inP = in + y * size.x * 4;
            float* outP = out + (size.y - 1 - y) * size.x * 4;
            memcpy(outP, inP, size.x * 4 * sizeof(float));
        }
    }

} // namespace Osprey
