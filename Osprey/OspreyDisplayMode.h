// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyData.h"
#include "OspreyEnum.h"
#include "OspreyValueObserver.h"

namespace Osprey
{
    class ChangeQueue;
    class Render;
    class Settings;

    class DisplayMode : public RhRdk::Realtime::DisplayMode
    {
	public:
		DisplayMode(
            const CRhinoDisplayPipeline&,
            const std::shared_ptr<Settings>&);
		~DisplayMode() override;

		static const GUID id;

		const UUID& ClassId() const override;

		bool StartRenderer(
			const ON_2iSize& frameSize,
			const CRhinoDoc&,
			const ON_3dmView&,
			const ON_Viewport&,
			const RhRdk::Realtime::DisplayMode*) override;
		bool OnRenderSizeChanged(const ON_2iSize& newFrameSize) override;
		void ShutdownRenderer() override;

		bool RendererIsAvailable() const override;

		void CreateWorld(const CRhinoDoc& doc, const ON_3dmView& view, const CDisplayPipelineAttributes& attributes) override;

		int LastRenderedPass() const override;

		bool ShowCaptureProgress() const override;
		double Progress() const override;

		bool IsRendererStarted() const override;

		bool IsCompleted() const override;

		bool IsFrameBufferAvailable(const ON_3dmView& vp) const override;

		bool DrawOrLockRendererFrameBuffer(const FRAME_BUFFER_INFO_INPUTS& input, FRAME_BUFFER_INFO_OUTPUTS& outputs) override;
		void UnlockRendererFrameBuffer() override;

		bool UseFastDraw() override;

	private:
		void _startRenderer();

		std::unique_ptr<IRhRdkRenderWindow> _rdkRenderWindow;

        Options _options;
        std::shared_ptr<Update> _update;
        std::shared_ptr<Scene> _scene;
        std::shared_ptr<ChangeQueue> _changeQueue;
		std::shared_ptr<Render> _render;
		std::thread _renderThread;
		std::atomic<bool> _renderRunning;
        size_t _pass = 0;

        std::shared_ptr<ValueObserver<Renderer> > _rendererObserver;
        std::shared_ptr<ValueObserver<Passes> > _passesObserver;
        std::shared_ptr<ValueObserver<PreviewPasses> > _previewPassesObserver;
        std::shared_ptr<ValueObserver<PixelSamples> > _pixelSamplesObserver;
        std::shared_ptr<ValueObserver<AOSamples> > _aoSamplesObserver;
        std::shared_ptr<ValueObserver<bool> > _denoiserFoundObserver;
        std::shared_ptr<ValueObserver<bool> > _denoiserEnabledObserver;
        std::shared_ptr<ValueObserver<bool> > _toneMapperEnabledObserver;
        std::shared_ptr<ValueObserver<Exposure> > _toneMapperExposureObserver;
    };

	class DisplayModeFactory : public RhRdk::Realtime::DisplayMode::Factory, public CRhRdkObject
	{
	public:
        DisplayModeFactory(const std::shared_ptr<Settings>&);

		virtual ON_wString Name(void) const override;
		virtual const UUID& ClassId(void) const override;
		virtual std::shared_ptr<RhRdk::Realtime::DisplayMode> MakeDisplayEngine(const CRhinoDisplayPipeline&) const override;
		void* EVF(const wchar_t*, void*) override;

    private:
        std::shared_ptr<Settings> _settings;
	};

} // namespace Osprey
