// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyRender.h"
#include "OspreyUtil.h"

namespace Osprey
{
    Render::Render()
    {}

	std::shared_ptr<Render> Render::create()
	{
		return std::shared_ptr<Render>(new Render());
	}

	void Render::init(const Options& options, const std::shared_ptr<Scene>& scene)
	{
        if (options.rendererName != _options.rendererName)
        {
            _renderer = ospray::cpp::Renderer(options.rendererName);
            _renderer.setParam("maxPathLength", 1);
        }

        if (options.previewPasses != _options.previewPasses ||
            options.denoiserFound != _options.denoiserFound ||
            options.denoiserEnabled != _options.denoiserEnabled ||
            options.toneMapperEnabled != _options.toneMapperEnabled ||
            options.toneMapperExposure != _options.toneMapperExposure ||
            options.flipY != _options.flipY ||
            scene->renderSize != _scene->renderSize)
        {
            _frameBufferSize = ospcommon::math::vec2i(0, 0);
        }

        _options = options;
        _scene = scene;

		_renderer.setParam("pixelSamples", static_cast<int>(_options.pixelSamples));
		_renderer.setParam("aoSamples", static_cast<int>(_options.aoSamples));
		_renderer.setParam("backgroundColor", _scene->background.color);
		_renderer.commit();

        _initFrameBuffers(_scene->renderRect.size());
    }

	void Render::render(size_t pass, IRhRdkRenderWindow& rdkRenderWindow)
	{
        if (_scene->renderSize.x > 0 && _scene->renderSize.y > 0)
        {
            // Setup the camera.
            _scene->camera.setParam("aspect", _scene->renderSize.x / static_cast<float>(_scene->renderSize.y));
            ospcommon::math::vec2f imageStart(
                _scene->renderRect.lower.x / static_cast<float>(_scene->renderSize.x - 1),
                1.F - (_scene->renderRect.upper.y / static_cast<float>(_scene->renderSize.y - 1)));
            ospcommon::math::vec2f imageEnd(
                _scene->renderRect.upper.x / static_cast<float>(_scene->renderSize.x - 1),
                1.F - (_scene->renderRect.lower.y / static_cast<float>(_scene->renderSize.y - 1)));
            _scene->camera.setParam("imageStart", imageStart);
            _scene->camera.setParam("imageEnd", imageEnd);
            _scene->camera.commit();

		    // Render a frame.
            size_t index = std::min(pass, _frameBuffers.size() - 1);
            _frameBuffers[index].renderFrame(_renderer, _scene->camera, _scene->world);

		    // Copy the RGBA channels to Rhino.
		    const ospcommon::math::vec2i renderSize = _scene->renderRect.size();
		    IRhRdkRenderWindow::IChannel* pChanRGBA = rdkRenderWindow.OpenChannel(IRhRdkRenderWindow::chanRGBA);
		    if (pChanRGBA)
		    {
			    void* fb = _frameBuffers[index].map(OSP_FB_COLOR);
			    float* fbP = reinterpret_cast<float*>(fb);
                const size_t scale = renderSize.x / _frameBuffersSizes[index].x;
                if (scale > 1)
                {
                    _scale(fbP, _frameBuffersSizes[index], _frameBufferTemp.data(), renderSize, scale, _options.flipY);
                    pChanRGBA->SetValueRect(
                        0,
                        0,
                        renderSize.x,
                        renderSize.y,
                        renderSize.x * 4 * sizeof(float),
                        ComponentOrder::RGBA,
                        _frameBufferTemp.data());
                }
                else if (_options.flipY)
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

        const size_t totalPasses = _options.previewPasses + 1;
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
            if (_options.toneMapperEnabled)
            {
                // Add the tone mapper operation.
                ospray::cpp::ImageOperation tonemapper("tonemapper");
                tonemapper.setParam("exposure", _options.toneMapperExposure);
                tonemapper.commit();
                imageOps.emplace_back(tonemapper);
            }
            if (_options.denoiserFound && _options.denoiserEnabled && i == totalPasses - 1)
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

        if (_options.previewPasses > 0 || _options.flipY)
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
