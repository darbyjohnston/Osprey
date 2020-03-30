// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyRdkPlugIn.h"
#include "OspreyPlugIn.h"
#include "OspreyRenderUI.h"

OspreyRdkPlugIn::OspreyRdkPlugIn(const std::shared_ptr<Osprey::Settings>& settings) :
	_settings(settings)
{}

UUID OspreyRdkPlugIn::PlugInId() const
{
	return ::OspreyPlugIn().PlugInID();
}

CRhinoPlugIn& OspreyRdkPlugIn::RhinoPlugIn() const
{
	return ::OspreyPlugIn();
}

bool OspreyRdkPlugIn::Initialize()
{
	return __super::Initialize();
}

void OspreyRdkPlugIn::Uninitialize()
{
	__super::Uninitialize();
}

void OspreyRdkPlugIn::RegisterExtensions() const
{
	__super::RegisterExtensions();
}

void OspreyRdkPlugIn::AbortRender()
{}

bool OspreyRdkPlugIn::CreatePreview(const ON_2iSize& sizeImage, RhRdkPreviewQuality quality, const IRhRdkPreviewSceneServer* pSceneServer, IRhRdkPreviewCallbacks* pNotify, CRhinoDib& dibOut)
{
	UNREFERENCED_PARAMETER(sizeImage);
	UNREFERENCED_PARAMETER(quality);
	UNREFERENCED_PARAMETER(pSceneServer);
	UNREFERENCED_PARAMETER(pNotify);
	UNREFERENCED_PARAMETER(dibOut);

	return false;
}

bool OspreyRdkPlugIn::CreatePreview(const ON_2iSize& sizeImage, const CRhRdkTexture& texture, CRhinoDib& dibOut)
{
	UNREFERENCED_PARAMETER(sizeImage);
	UNREFERENCED_PARAMETER(texture);
	UNREFERENCED_PARAMETER(dibOut);

	return false;
}

bool OspreyRdkPlugIn::SupportsFeature(const UUID& uuidFeature) const
{
	if (uuidFeature == uuidFeatureCustomRenderMeshes)
		return false;
	if (uuidFeature == uuidFeatureDecals)
		return false;
	if (uuidFeature == uuidFeatureGroundPlane)
		return true;
	if (uuidFeature == uuidFeatureSun)
		return true;
	return true;
}

void OspreyRdkPlugIn::AddCustomRenderSettingsSections(RhRdkUiModalities rhinoModalities, ON_SimpleArray<IRhinoUiSection*>& rhinoSections) const
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	auto ui = new Osprey::RenderUI(_settings, rhinoModalities);
	rhinoSections.Append(ui);

	CRhRdkRenderPlugIn::AddCustomRenderSettingsSections(rhinoModalities, rhinoSections);
}
