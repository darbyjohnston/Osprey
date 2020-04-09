// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"
#include "OspreyValueObserver.h"

namespace Osprey
{
    class Settings
    {
		Settings();
		Settings(const Settings&) = delete;
		Settings& operator = (const Settings&) = delete;

    public:
		// Create a new instance.
		static std::shared_ptr<Settings> create();

		std::shared_ptr<IValueSubject<Renderer> > observeRenderer() const;
        std::shared_ptr<IValueSubject<Passes> > observePasses() const;
        std::shared_ptr<IValueSubject<PreviewPasses> > observePreviewPasses() const;
		std::shared_ptr<IValueSubject<PixelSamples> > observePixelSamples() const;
		std::shared_ptr<IValueSubject<AOSamples> > observeAOSamples() const;
		std::shared_ptr<IValueSubject<bool> > observeDenoiserFound() const;
        std::shared_ptr<IValueSubject<bool> > observeDenoiserEnabled() const;
        std::shared_ptr<IValueSubject<bool> > observeToneMapperEnabled() const;
        std::shared_ptr<IValueSubject<Exposure> > observeToneMapperExposure() const;

		void setRenderer(Renderer);
        void setPasses(Passes);
        void setPreviewPasses(PreviewPasses);
        void setPixelSamples(PixelSamples);
		void setAOSamples(AOSamples);
		void setDenoiserFound(bool);
        void setDenoiserEnabled(bool);
        void setToneMapperEnabled(bool);
        void setToneMapperExposure(Exposure);

	private:
		std::shared_ptr<ValueSubject<Renderer> > _renderer;
        std::shared_ptr<ValueSubject<Passes> > _passes;
        std::shared_ptr<ValueSubject<PreviewPasses> > _previewPasses;
        std::shared_ptr<ValueSubject<PixelSamples> > _pixelSamples;
		std::shared_ptr<ValueSubject<AOSamples> > _aoSamples;
		std::shared_ptr<ValueSubject<bool> > _denoiserFound;
        std::shared_ptr<ValueSubject<bool> > _denoiserEnabled;
        std::shared_ptr<ValueSubject<bool> > _toneMapperEnabled;
        std::shared_ptr<ValueSubject<Exposure> > _toneMapperExposure;
	};

} // namespace Osprey

