// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyChangeQueue.h"
#include "OspreyDisplayMode.h"
#include "OspreyRender.h"
#include "OspreySettings.h"

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
        _data = std::make_shared<Data>();
        _data->updates = false;
        _data->world = std::make_shared<ospray::cpp::World>();
        //_data->world.setParam("dynamicScene", int(RTC_SCENE_FLAG_DYNAMIC));
        _renderRunning = false;

        // Listen for settings changes.
        _rendererObserver = ValueObserver<Renderer>::create(
            settings->observeRenderer(),
            [this](Renderer value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _renderer = value;
        });
        _passesObserver = ValueObserver<Passes>::create(
            settings->observePasses(),
            [this](Passes value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _passes = value;
        });
        _previewPassesObserver = ValueObserver<PreviewPasses>::create(
            settings->observePreviewPasses(),
            [this](PreviewPasses value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _previewPasses = value;
        });
        _pixelSamplesObserver = ValueObserver<PixelSamples>::create(
            settings->observePixelSamples(),
            [this](PixelSamples value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _pixelSamples = value;
        });
        _aoSamplesObserver = ValueObserver<AOSamples>::create(
            settings->observeAOSamples(),
            [this](AOSamples value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _aoSamples = value;
        });
        _denoiserFoundObserver = ValueObserver<bool>::create(
            settings->observeDenoiserFound(),
            [this](bool value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _denoiserFound = value;
        });
        _denoiserEnabledObserver = ValueObserver<bool>::create(
            settings->observeDenoiserEnabled(),
            [this](bool value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _denoiserEnabled = value;
        });
        _toneMapperEnabledObserver = ValueObserver<bool>::create(
            settings->observeToneMapperEnabled(),
            [this](bool value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _toneMapperEnabled = value;
        });
        _toneMapperExposureObserver = ValueObserver<Exposure>::create(
            settings->observeToneMapperExposure(),
            [this](Exposure value)
        {
            std::lock_guard<std::mutex> lock(_data->mutex);
            _data->updates = true;
            _toneMapperExposure = value;
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
        _changeQueue = std::shared_ptr<ChangeQueue>(new ChangeQueue(rhinoDoc, onView, _data));
        const auto rendererName = getRendererValue(_renderer);
        _changeQueue->setRendererName(rendererName, getRendererSupportsMaterials(_renderer));
        _changeQueue->CreateWorld();

        // Create the renderer.
		_render = Render::create();
        _render->setRendererName(rendererName);
        _render->setPreviewPasses(getPreviewPassesValue(_previewPasses));
        _render->setPixelSamples(getPixelSamplesValue(_pixelSamples));
        _render->setAOSamples(getAOSamplesValue(_aoSamples));
        _render->setDenoiserFound(_denoiserFound);
        _render->setDenoiserEnabled(_denoiserEnabled);
        _render->setToneMapperEnabled(_toneMapperEnabled);
        _render->setToneMapperExposure(getExposureValue(_toneMapperExposure));
        _startRenderer();

		return true;
	}

	bool DisplayMode::OnRenderSizeChanged(const ON_2iSize& onSize)
	{
		_rdkRenderWindow->SetSize(onSize);
		if (!_rdkRenderWindow->EnsureDib())
		{
			_rdkRenderWindow.reset();
			return false;
		}

		_renderRunning = false;
		if (_renderThread.joinable())
		{
			_renderThread.join();
		}

        _data->updates = true;
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
        const ospcommon::math::vec2i renderSize = fromRhino(_rdkRenderWindow->Size());
        _render->initRender(renderSize, ospcommon::math::box2i(ospcommon::math::vec2i(), renderSize));

        _renderRunning = true;
		_renderThread = std::thread([this]
		{
            bool updates = false;
            Renderer renderer = Renderer::First;
            bool rendererChanged = false;
            Passes passes = Passes::First;
            PreviewPasses previewPasses = PreviewPasses::First;
            PixelSamples pixelSamples = PixelSamples::First;
            AOSamples aoSamples = AOSamples::First;
            bool denoiserFound = false;
            bool denoiserEnabled = false;
            bool toneMapperEnabled = false;
            Exposure toneMapperExposure = Exposure::First;

            while (_renderRunning)
			{
                // Check for updates or settings changes.
                {
                    std::unique_lock<std::mutex> lock(_data->mutex, std::try_to_lock);
                    if (lock.owns_lock())
                    {
                        updates = _data->updates;
                        if (_data->updates)
                        {
                            _data->updates = false;
                        }
                        if (_renderer != renderer)
                        {
                            renderer = _renderer;
                            rendererChanged = true;
                        }
                        passes = _passes;
                        previewPasses = _previewPasses;
                        pixelSamples = _pixelSamples;
                        aoSamples = _aoSamples;
                        denoiserFound = _denoiserFound;
                        denoiserEnabled = _denoiserEnabled;
                        toneMapperEnabled = _toneMapperEnabled;
                        toneMapperExposure = _toneMapperExposure;
                    }
                }

                const size_t previewPassesCount = getPreviewPassesValue(previewPasses);
                if (updates)
                {
                    // Update the change queue.
                    const auto rendererName = getRendererValue(renderer);
                    _changeQueue->setRendererName(rendererName, getRendererSupportsMaterials(renderer));
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
                    _render->setRendererName(rendererName);
                    _render->setPreviewPasses(previewPassesCount);
                    _render->setPixelSamples(getPixelSamplesValue(pixelSamples));
                    _render->setAOSamples(getAOSamplesValue(aoSamples));
                    _render->setDenoiserFound(denoiserFound);
                    _render->setDenoiserEnabled(denoiserEnabled);
                    _render->setToneMapperEnabled(toneMapperEnabled);
                    _render->setToneMapperExposure(getExposureValue(toneMapperExposure));
                    const ospcommon::math::vec2i renderSize = fromRhino(_rdkRenderWindow->Size());
                    _render->initRender(renderSize, ospcommon::math::box2i(ospcommon::math::vec2i(), renderSize));
                    _pass = 0;
                }

                // Render a pass.
                const size_t passesCount = getPassesValue(passes);
                if (_pass < passesCount + previewPassesCount)
                {
                    _render->renderPass(_pass, _data->world, _data->camera, *_rdkRenderWindow);
                    ++_pass;
                    SignalUpdate();
                }
                else
                {
                    Sleep(50);
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
