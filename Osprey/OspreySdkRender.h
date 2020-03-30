// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"
#include "OspreyRender.h"
#include "OspreySettings.h"

class OspreySdkRender : public CRhRdkSdkRender
{
public:
	OspreySdkRender(
		const std::shared_ptr<Osprey::Settings>&,
		const CRhinoCommandContext& context,
		CRhinoRenderPlugIn& pPlugin,
		const ON_wString& sCaption,
		UINT idIcon,
		bool bPreview);
	virtual ~OspreySdkRender();

	void setRenderer(Osprey::Renderer);
	void setPasses(Osprey::Passes);
	void setPixelSamples(Osprey::PixelSamples);
	void setAOSamples(Osprey::AOSamples);
	void setDenoiserEnabled(bool);
	void setDenoiserFound(bool);

	int ThreadedRender(void);
	void SetContinueModal(bool b);

	CRhinoSdkRender::RenderReturnCodes Render(const ON_2iSize& sizeRender) override;
	CRhinoSdkRender::RenderReturnCodes RenderWindow(CRhinoView* pView, const LPRECT pRect, bool bInPopupWindow) override;

	virtual BOOL RenderSceneWithNoMeshes() override { return TRUE; }
	virtual BOOL IgnoreRhinoObject(const CRhinoObject*) override { return FALSE; }
	virtual BOOL RenderPreCreateWindow() override;
	virtual BOOL RenderEnterModalLoop() override { return TRUE; }
	virtual BOOL RenderContinueModal() override;
	virtual BOOL RenderExitModalLoop() override { return TRUE; }
	virtual bool ReuseRenderWindow(void) const override { return true; }
	virtual BOOL NeedToProcessGeometryTable() override;
	virtual BOOL NeedToProcessLightTable() override;
	virtual BOOL StartRenderingInWindow(CRhinoView* pView, const LPCRECT pRect) override;
	virtual void StopRendering() override;
	virtual void StartRendering() override;

protected:
	static void RenderThread(void* pv);

private:
	HANDLE m_hRenderThread;
	bool m_bContinueModal;
	bool m_bRenderQuick;
	bool m_bCancel;

	std::shared_ptr<Osprey::Render> _osprey;
};
