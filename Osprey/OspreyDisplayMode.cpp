// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyChangeQueue.h"
#include "OspreyDisplayMode.h"
#include "OspreyRender.h"
#include "OspreySettings.h"
#include "OspreyUtil.h"

namespace Osprey
{
    const GUID DisplayMode::id =
    {
        0x057ca451,
        0x20a1,
        0x41cb,
        { 0xab, 0x88, 0x66, 0x19, 0xe1, 0xe0, 0x25, 0x68 }
    };

	DisplayMode::DisplayMode(const CRhinoDisplayPipeline& pipeline, const std::shared_ptr<Settings>& settings) :
		RhRdk::Realtime::DisplayMode(pipeline)
	{
        _update = std::make_shared<Update>();

        _scene = std::make_shared<Scene>();
        _scene->world = ospray::cpp::World();
        //_scene->world.setParam("dynamicScene", int(RTC_SCENE_FLAG_DYNAMIC));
        _renderRunning = false;

        // Listen for settings changes.
        _rendererObserver = ValueObserver<Renderer>::create(
            settings->observeRenderer(),
            [this](Renderer value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.rendererName = getRendererValue(value);
            }
            _update->cv.notify_one();
        });
        _passesObserver = ValueObserver<Passes>::create(
            settings->observePasses(),
            [this](Passes value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.passes = getPassesValue(value);
            }
            _update->cv.notify_one();
        });
        _previewPassesObserver = ValueObserver<PreviewPasses>::create(
            settings->observePreviewPasses(),
            [this](PreviewPasses value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.previewPasses = getPreviewPassesValue(value);
            }
            _update->cv.notify_one();
        });
        _pixelSamplesObserver = ValueObserver<PixelSamples>::create(
            settings->observePixelSamples(),
            [this](PixelSamples value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.pixelSamples = getPixelSamplesValue(value);
            }
            _update->cv.notify_one();
        });
        _aoSamplesObserver = ValueObserver<AOSamples>::create(
            settings->observeAOSamples(),
            [this](AOSamples value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.aoSamples = getAOSamplesValue(value);
            }
            _update->cv.notify_one();
        });
        _denoiserFoundObserver = ValueObserver<bool>::create(
            settings->observeDenoiserFound(),
            [this](bool value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.denoiserFound = value;
            }
            _update->cv.notify_one();
        });
        _denoiserEnabledObserver = ValueObserver<bool>::create(
            settings->observeDenoiserEnabled(),
            [this](bool value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.denoiserEnabled = value;
            }
            _update->cv.notify_one();
        });
        _toneMapperEnabledObserver = ValueObserver<bool>::create(
            settings->observeToneMapperEnabled(),
            [this](bool value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.toneMapperEnabled = value;
            }
            _update->cv.notify_one();
        });
        _toneMapperExposureObserver = ValueObserver<Exposure>::create(
            settings->observeToneMapperExposure(),
            [this](Exposure value)
        {
            {
                std::lock_guard<std::mutex> lock(_update->mutex);
                _update->update = true;
                _options.toneMapperExposure = getExposureValue(value);
            }
            _update->cv.notify_one();
        });
	}

	DisplayMode::~DisplayMode()
	{
		_renderRunning = false;
		if (_renderThread.joinable())
		{
			_renderThread.join();
		}
	}

	const UUID& DisplayMode::ClassId() const
	{
		return id;
	}

	bool DisplayMode::StartRenderer(
		const ON_2iSize& onSize,
		const CRhinoDoc& rhinoDoc,
		const ON_3dmView& onView,
		const ON_Viewport& onVp,
		const RhRdk::Realtime::DisplayMode* pParent)
	{
        _update->update = true;
        _scene->renderSize = fromRhino(onSize);
        _scene->renderRect.upper = _scene->renderSize;

        // Create the render window.
		_rdkRenderWindow.reset(IRhRdkRenderWindow::New());
		if (!_rdkRenderWindow)
			return false;
		_rdkRenderWindow->SetSize(onSize);
		if (!_rdkRenderWindow->EnsureDib())
		{
			_rdkRenderWindow.reset();
			return false;
		}

        // Create the change queue.
        _changeQueue = std::shared_ptr<ChangeQueue>(new ChangeQueue(rhinoDoc, onView, _update, _scene));
        _changeQueue->setRendererName(_options.rendererName, _options.supportsMaterials);
        _changeQueue->CreateWorld();

        // Create the renderer.
		_render = Render::create();
        _startRenderer();

		return true;
	}

	bool DisplayMode::OnRenderSizeChanged(const ON_2iSize& onSize)
	{
        _renderRunning = false;
        if (_renderThread.joinable())
        {
            _renderThread.join();
        }
        
        _rdkRenderWindow->SetSize(onSize);
		if (!_rdkRenderWindow->EnsureDib())
		{
			_rdkRenderWindow.reset();
			return false;
		}

        _update->update = true;
        _scene->renderSize = fromRhino(_rdkRenderWindow->Size());
        _scene->renderRect.upper = _scene->renderSize;

		_startRenderer();

		return true;
	}

	void DisplayMode::ShutdownRenderer()
	{
		_renderRunning = false;
		if (_renderThread.joinable())
		{
			_renderThread.join();
		}

		_render.reset();
        _changeQueue.reset();
        _rdkRenderWindow.reset();
	}

	bool DisplayMode::RendererIsAvailable() const
	{
		return true;
	}

	void DisplayMode::CreateWorld(
		const CRhinoDoc& rhinoDoc,
		const ON_3dmView& onView,
		const CDisplayPipelineAttributes& attributes)
	{}

	int DisplayMode::LastRenderedPass() const
	{
		return 0;
	}

	bool DisplayMode::ShowCaptureProgress() const
	{
		return false;
	}

	double DisplayMode::Progress() const
	{
		return 1.0;
	}

	bool DisplayMode::IsRendererStarted() const
	{
		return _render.get();
	}

	bool DisplayMode::IsCompleted() const
	{
		return true;
	}

	bool DisplayMode::IsFrameBufferAvailable(const ON_3dmView& vp) const
	{
		return true;
	}

	bool DisplayMode::DrawOrLockRendererFrameBuffer(
		const FRAME_BUFFER_INFO_INPUTS& input,
		FRAME_BUFFER_INFO_OUTPUTS& outputs)
	{
		if (!outputs.client_render_success)
		{
			const CRhinoDib* pDib = _rdkRenderWindow->LockDib();
			if (!pDib)
				return false;
			outputs.pointer_to_dib = pDib;
            outputs.flip_y = true;
		}
		return true;
	}

	void DisplayMode::UnlockRendererFrameBuffer()
	{
		_rdkRenderWindow->UnlockDib();
	}

	bool DisplayMode::UseFastDraw()
	{
		return false;
	}

	void DisplayMode::_startRenderer()
	{
        _renderRunning = true;
		_renderThread = std::thread([this]
		{
            Options options;
            {
                std::unique_lock<std::mutex> lock(_update->mutex);
                options = _options;
            }
            while (_renderRunning)
			{
                // Check for updates or settings changes.
                bool update = false;
                bool rendererChanged = false;
                {
                    std::unique_lock<std::mutex> lock(_update->mutex);
                    if (_update->cv.wait_for(
                        lock,
                        std::chrono::milliseconds(100),
                        [this]
                    {
                        return _update->update;
                    }))
                    {
                        update = true;
                        rendererChanged = _options.rendererName != options.rendererName;
                        options = _options;
                        _update->update = false;
                    }
                }

                if (update)
                {
                    // Update the change queue.
                    _changeQueue->setRendererName(options.rendererName, options.supportsMaterials);
                    if (rendererChanged)
                    {
                        rendererChanged = false;
                        _changeQueue->CreateWorld();
                    }
                    else
                    {
                        _changeQueue->Flush();
                    }

                    // Update the renderer.
                    _render->init(options, _scene);
                    _pass = 0;
                }

                // Render a pass.
                if (_pass < options.passes + options.previewPasses)
                {
                    _render->render(_pass, *_rdkRenderWindow);
                    ++_pass;
                    SignalUpdate();
                }
            }
		});
	}

    DisplayModeFactory::DisplayModeFactory(const std::shared_ptr<Settings>& settings) :
        _settings(settings)
    {}

	ON_wString DisplayModeFactory::Name(void) const
	{
		return L"Osprey";
	}

	const UUID& DisplayModeFactory::ClassId(void) const
	{
		return DisplayMode::id;
	}

	std::shared_ptr<RhRdk::Realtime::DisplayMode> DisplayModeFactory::MakeDisplayEngine(const CRhinoDisplayPipeline& pipeline) const
	{
		return std::make_shared<DisplayMode>(pipeline, _settings);
	}

	void* DisplayModeFactory::EVF(const wchar_t*, void*)
	{
		return nullptr;
	}

} // namespace Osprey
