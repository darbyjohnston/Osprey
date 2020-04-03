// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "StdAfx.h"
#include "rhinoSdkPlugInDeclare.h"
#include "OspreyPlugIn.h"
#include "Resource.h"
#include "OspreyRdkPlugIn.h"
#include "OspreySdkRender.h"
#include "OspreyUtil.h"

// The plug-in object must be constructed before any plug-in classes derived
// from CRhinoCommand. The #pragma init_seg(lib) ensures that this happens.
#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
#pragma warning(pop)

RHINO_PLUG_IN_DECLARE
RHINO_PLUG_IN_NAME(L"Osprey");
RHINO_PLUG_IN_ID(L"AE17E3AF-8D7F-4E53-96DD-66DEA33B4AAD");
RHINO_PLUG_IN_VERSION(__DATE__ "  " __TIME__)
RHINO_PLUG_IN_DESCRIPTION(L"Osprey is an open source plugin that integrates the OSPRay renderer with Rhino");
RHINO_PLUG_IN_ICON_RESOURCE_ID(IDI_ICON);

RHINO_PLUG_IN_DEVELOPER_ORGANIZATION(L"");
RHINO_PLUG_IN_DEVELOPER_ADDRESS(L"");
RHINO_PLUG_IN_DEVELOPER_COUNTRY(L"");
RHINO_PLUG_IN_DEVELOPER_PHONE(L"");
RHINO_PLUG_IN_DEVELOPER_FAX(L"");
RHINO_PLUG_IN_DEVELOPER_EMAIL(L"darbyjohnston@yahoo.com");
RHINO_PLUG_IN_DEVELOPER_WEBSITE(L"https://github.com/darbyjohnston/Osprey");
RHINO_PLUG_IN_UPDATE_URL(L"https://github.com/darbyjohnston/Osprey");

static class COspreyPlugIn thePlugIn;

COspreyPlugIn& OspreyPlugIn()
{
	return thePlugIn;
}

COspreyPlugIn::COspreyPlugIn()
{
	m_plugin_version = RhinoPlugInVersion();

	m_pRdkPlugIn = nullptr;
}

const wchar_t* COspreyPlugIn::PlugInName() const
{
	return RhinoPlugInName();
}

const wchar_t* COspreyPlugIn::PlugInVersion() const
{
	return m_plugin_version;
}

GUID COspreyPlugIn::PlugInID() const
{
	return ON_UuidFromString(RhinoPlugInId());
}

BOOL COspreyPlugIn::OnLoadPlugIn()
{
    ASSERT(RhRdkIsAvailable());

    // Initialize OSPRay.
    ON_wString pluginFolder;
    GetPlugInFolder(pluginFolder);
    size_t envSize = 0;
    wchar_t* envP = 0;
    if (0 == _wdupenv_s(&envP, &envSize, L"PATH"))
    {
        if (envP)
        {
            ON_wString path = ON_wString(envP) + L";" + pluginFolder;
            _wputenv_s(L"PATH", path);
        }
    }
	_settings = Osprey::Settings::create();
	_settings->setDenoiserFound(ospLoadModule("denoiser") == OSP_NO_ERROR);
    OSPError ospError = ospInit();
	if (ospError != OSP_NO_ERROR)
	{
        Osprey::printError("Cannot initialize: " + Osprey::getErrorMessage(ospError));
		return false;
	}
	OSPDevice device = ospGetCurrentDevice();
	ospDeviceSetErrorFunc(device, Osprey::errorFunc);
	ospDeviceSetStatusFunc(device, Osprey::messageFunc);
	ospDeviceSetParam(device, "logLevel", OSPDataType::OSP_STRING, "debug");
	ospDeviceCommit(device);
	ospDeviceRelease(device);
    _render = Osprey::Render::create();

	// Initialize RDK plugin.
	m_pRdkPlugIn = new OspreyRdkPlugIn(_settings);
	ON_wString str;
	if (!m_pRdkPlugIn->Initialize())
	{
		delete m_pRdkPlugIn;
		m_pRdkPlugIn = nullptr;
		str.Format(L"Failed to load %s, version %s. RDK initialization failed\n", PlugInName(), PlugInVersion());
		RhinoApp().Print(str);
		return FALSE;
	}

	str.Format(L"Loading %s, version %s\n", PlugInName(), PlugInVersion());
	RhinoApp().Print(str);

	m_event_watcher.Register();
	m_event_watcher.Enable(TRUE);

	CRhinoDoc* doc = RhinoApp().ActiveDoc();
	if (doc)
		m_event_watcher.OnNewDocument(*doc);

	return TRUE;
}

void COspreyPlugIn::OnUnloadPlugIn()
{
	m_event_watcher.Enable(FALSE);
	m_event_watcher.UnRegister();

	if (nullptr != m_pRdkPlugIn)
	{
		m_pRdkPlugIn->Uninitialize();
		delete m_pRdkPlugIn;
		m_pRdkPlugIn = nullptr;
	}
}

CRhinoCommand::result COspreyPlugIn::Render(const CRhinoCommandContext& context, bool bPreview)
{
	const auto rhinoDoc = context.Document();
	if (nullptr == rhinoDoc)
		return CRhinoCommand::failure; 

    const auto& view = RhinoApp().ActiveView()->ActiveViewport().View();
    const std::string rendererName = Osprey::getRendererValues()[static_cast<size_t>(_settings->observeRenderer()->get())];
    _changeQueue.reset(new Osprey::ChangeQueue(*rhinoDoc, view));
    _changeQueue->setWorldBBox(rhinoDoc->BoundingBox(false, true, true));
    _changeQueue->setRendererName(rendererName);
    _changeQueue->CreateWorld();

    _render->setRendererName(rendererName);
    _render->setPasses(Osprey::getPassesValues()[static_cast<size_t>(_settings->observePasses()->get())]);
    _render->setPixelSamples(Osprey::getPixelSamplesValues()[static_cast<size_t>(_settings->observePixelSamples()->get())]);
    _render->setAOSamples(Osprey::getAOSamplesValues()[static_cast<size_t>(_settings->observeAOSamples()->get())]);
    _render->setDenoiserFound(_settings->observeDenoiserFound()->get());
    _render->setDenoiserEnabled(_settings->observeDenoiserEnabled()->get());
    _render->setWorld(_changeQueue->getWorld(), _changeQueue->getCamera());

	OspreySdkRender render(_changeQueue, _render, context, *this, L"Osprey", IDI_RENDER);
	const auto size = render.RenderSize(*rhinoDoc, true);
	if (CRhinoSdkRender::render_ok != render.Render(size))
		return CRhinoCommand::failure;

	return CRhinoCommand::success;
}

CRhinoCommand::result COspreyPlugIn::RenderWindow(
	const CRhinoCommandContext& context,
	bool,
	CRhinoView* view,
	const LPRECT rect,
	bool bInWindow,
	bool)
{
    const auto rhinoDoc = context.Document();
    if (nullptr == rhinoDoc)
        return CRhinoCommand::failure;

    const std::string rendererName = Osprey::getRendererValues()[static_cast<size_t>(_settings->observeRenderer()->get())];
    _changeQueue.reset(new Osprey::ChangeQueue(*rhinoDoc, view->ActiveViewport().View()));
    _changeQueue->setWorldBBox(rhinoDoc->BoundingBox(false, true, true));
    _changeQueue->setRendererName(rendererName);
    _changeQueue->CreateWorld();

    _render->setRendererName(rendererName);
    _render->setPasses(Osprey::getPassesValues()[static_cast<size_t>(_settings->observePasses()->get())]);
    _render->setPixelSamples(Osprey::getPixelSamplesValues()[static_cast<size_t>(_settings->observePixelSamples()->get())]);
    _render->setAOSamples(Osprey::getAOSamplesValues()[static_cast<size_t>(_settings->observeAOSamples()->get())]);
    _render->setDenoiserFound(_settings->observeDenoiserFound()->get());
    _render->setDenoiserEnabled(_settings->observeDenoiserEnabled()->get());
    _render->setWorld(_changeQueue->getWorld(), _changeQueue->getCamera());

	OspreySdkRender render(_changeQueue, _render, context, *this, L"Osprey", IDI_RENDER);
	if (CRhinoSdkRender::render_ok == render.RenderWindow(view, rect, bInWindow))
		return CRhinoCommand::success;
	return CRhinoCommand::failure;
}

CRhinoCommand::result COspreyPlugIn::RenderQuiet(const CRhinoCommandContext&, bool)
{
	return CRhinoCommand::failure;
}

BOOL COspreyPlugIn::SaveRenderedImage(ON_wString filename)
{
	return FALSE;
}

BOOL COspreyPlugIn::CloseRenderWindow()
{
	return FALSE;
}

UINT COspreyPlugIn::MainFrameResourceID() const
{
	return IDR_RENDER;
}

BOOL COspreyPlugIn::SceneChanged() const
{
	return m_event_watcher.RenderSceneModified();
}

void COspreyPlugIn::SetSceneChanged(BOOL bChanged)
{
	m_event_watcher.SetRenderMeshFlags(bChanged);
	m_event_watcher.SetMaterialFlags(bChanged);
}

BOOL COspreyPlugIn::LightingChanged() const
{
	return m_event_watcher.RenderLightingModified();
}

void COspreyPlugIn::SetLightingChanged(BOOL bChanged)
{
	m_event_watcher.SetLightFlags(bChanged);
}
