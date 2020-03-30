#include "stdafx.h"
#include "OspreySettings.h"

namespace Osprey
{
	Settings::Settings()
	{
		_renderer = ValueSubject<Renderer>::create(Renderer::PathTracer);
		_passes = ValueSubject<Passes>::create(Passes::_8);
		_pixelSamples = ValueSubject<PixelSamples>::create(PixelSamples::_1);
		_aoSamples = ValueSubject<AOSamples>::create(AOSamples::_16);
		_denoiserFound = ValueSubject<bool>::create(false);
		_denoiserEnabled = ValueSubject<bool>::create(true);
	}

	std::shared_ptr<Settings> Settings::create()
	{
		return std::shared_ptr<Settings>(new Settings);
	}

	std::shared_ptr<IValueSubject<Renderer> > Settings::observeRenderer() const
	{
		return _renderer;
	}

	std::shared_ptr<IValueSubject<Passes> > Settings::observePasses() const
	{
		return _passes;
	}

	std::shared_ptr<IValueSubject<PixelSamples> > Settings::observePixelSamples() const
	{
		return _pixelSamples;
	}

	std::shared_ptr<IValueSubject<AOSamples> > Settings::observeAOSamples() const
	{
		return _aoSamples;
	}

	std::shared_ptr<IValueSubject<bool> > Settings::observeDenoiserFound() const
	{
		return _denoiserFound;
	}

	std::shared_ptr<IValueSubject<bool> > Settings::observeDenoiserEnabled() const
	{
		return _denoiserEnabled;
	}

	void Settings::setRenderer(Renderer value)
	{
		_renderer->setIfChanged(value);
	}

	void Settings::setPasses(Passes value)
	{
		_passes->setIfChanged(value);
	}

	void Settings::setPixelSamples(PixelSamples value)
	{
		_pixelSamples->setIfChanged(value);
	}

	void Settings::setAOSamples(AOSamples value)
	{
		_aoSamples->setIfChanged(value);
	}

	void Settings::setDenoiserFound(bool value)
	{
		_denoiserFound->setIfChanged(value);
	}

	void Settings::setDenoiserEnabled(bool value)
	{
		_denoiserEnabled->setIfChanged(value);
	}

} // namespace Osprey
