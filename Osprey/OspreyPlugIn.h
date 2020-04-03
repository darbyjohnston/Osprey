// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyChangeQueue.h"
#include "OspreyEventWatcher.h"
#include "OspreyRender.h"
#include "OspreySettings.h"

class OspreyRdkPlugIn;

class COspreyPlugIn : public CRhinoRenderPlugIn
{
public:
	COspreyPlugIn();
	~COspreyPlugIn() = default;

	const wchar_t* PlugInName() const override;
	const wchar_t* PlugInVersion() const override;
	GUID PlugInID() const override;
	BOOL OnLoadPlugIn() override;
	void OnUnloadPlugIn() override;

	CRhinoCommand::result Render(const CRhinoCommandContext&, bool bPreview) override;
	CRhinoCommand::result RenderWindow(
		const CRhinoCommandContext& context, 
		bool render_preview, 
		CRhinoView* view, 
		const LPRECT rect, 
		bool bInWindow, 
		bool bBlowUp) override;

	BOOL SaveRenderedImage(ON_wString filename) override;
	BOOL CloseRenderWindow() override;

	CRhinoCommand::result RenderQuiet( const CRhinoCommandContext&, bool bPreview);
	BOOL SceneChanged() const;
	void SetSceneChanged(BOOL bChanged);
	BOOL LightingChanged() const;
	void SetLightingChanged(BOOL bChanged);
	UINT MainFrameResourceID() const;

private:
    std::shared_ptr<Osprey::Settings> _settings;
    std::shared_ptr<Osprey::ChangeQueue> _changeQueue;
    std::shared_ptr<Osprey::Render> _render;
    ON_wString m_plugin_version;
	COspreyEventWatcher m_event_watcher;
	OspreyRdkPlugIn* m_pRdkPlugIn;
};

COspreyPlugIn& OspreyPlugIn();
