// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"
#include "OspreyChangeQueue.h"
#include "OspreyRender.h"
#include "OspreySettings.h"

class OspreySdkRender : public CRhRdkSdkRender
{
public:
	OspreySdkRender(
        const std::shared_ptr<Osprey::ChangeQueue>&,
        const std::shared_ptr<Osprey::Render>&,
		const CRhinoCommandContext&,
		CRhinoRenderPlugIn&,
		const ON_wString& sCaption,
		UINT idIcon);
	virtual ~OspreySdkRender();

	void ThreadedRender(void);
	void SetContinueModal(bool);

	CRhinoSdkRender::RenderReturnCodes Render(const ON_2iSize& sizeRender) override;
	CRhinoSdkRender::RenderReturnCodes RenderWindow(CRhinoView*, const LPRECT, bool bInPopupWindow) override;

	virtual BOOL RenderSceneWithNoMeshes() override { return TRUE; }
	virtual BOOL IgnoreRhinoObject(const CRhinoObject*) override { return FALSE; }
	virtual BOOL RenderPreCreateWindow() override;
	virtual BOOL RenderEnterModalLoop() override { return TRUE; }
	virtual BOOL RenderContinueModal() override;
	virtual BOOL RenderExitModalLoop() override { return TRUE; }
	virtual bool ReuseRenderWindow(void) const override { return true; }
	virtual BOOL NeedToProcessGeometryTable() override;
	virtual BOOL NeedToProcessLightTable() override;
	virtual BOOL StartRenderingInWindow(CRhinoView*, const LPCRECT) override;
	virtual void StopRendering() override;
	virtual void StartRendering() override;

private:
	bool m_bContinueModal = true;
	bool m_bRenderQuick = false;
	bool m_bCancel = false;

    std::shared_ptr<Osprey::ChangeQueue> _changeQueue;
    std::thread _renderThread;
    std::shared_ptr<Osprey::Render> _render;
};
