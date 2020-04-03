// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreySettings.h"

class OspreyRdkPlugIn : public CRhRdkRenderPlugIn
{
public:
	OspreyRdkPlugIn(const std::shared_ptr<Osprey::Settings>&);

	virtual bool Initialize() override;
	virtual void Uninitialize() override;

	virtual UUID PlugInId() const override;

	virtual void AbortRender() override;

	CRhinoPlugIn& RhinoPlugIn() const override;

protected:
	virtual void RegisterExtensions() const override;

	virtual bool CreatePreview(const ON_2iSize&, RhRdkPreviewQuality, const IRhRdkPreviewSceneServer*, IRhRdkPreviewCallbacks*, CRhinoDib&) override;
	virtual bool CreatePreview(const ON_2iSize&, const CRhRdkTexture&, CRhinoDib&) override;

	virtual bool SupportsFeature(const UUID&) const override;

	virtual void AddCustomRenderSettingsSections(RhRdkUiModalities, ON_SimpleArray<IRhinoUiSection*>&) const override;

private:
	std::shared_ptr<Osprey::Settings> _settings;
};
