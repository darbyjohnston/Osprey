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
		std::shared_ptr<IValueSubject<PixelSamples> > observePixelSamples() const;
		std::shared_ptr<IValueSubject<AOSamples> > observeAOSamples() const;
		std::shared_ptr<IValueSubject<bool> > observeDenoiserFound() const;
		std::shared_ptr<IValueSubject<bool> > observeDenoiserEnabled() const;

		void setRenderer(Renderer);
		void setPasses(Passes);
		void setPixelSamples(PixelSamples);
		void setAOSamples(AOSamples);
		void setDenoiserFound(bool);
		void setDenoiserEnabled(bool);

	private:
		std::shared_ptr<ValueSubject<Renderer> > _renderer;
		std::shared_ptr<ValueSubject<Passes> > _passes;
		std::shared_ptr<ValueSubject<PixelSamples> > _pixelSamples;
		std::shared_ptr<ValueSubject<AOSamples> > _aoSamples;
		std::shared_ptr<ValueSubject<bool> > _denoiserFound;
		std::shared_ptr<ValueSubject<bool> > _denoiserEnabled;
	};

} // namespace Osprey

