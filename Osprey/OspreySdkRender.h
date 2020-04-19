// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"
#include "OspreyData.h"

namespace Osprey
{
    class ChangeQueue;
    class Render;
    class Settings;

} // namespace Osprey;

class OspreySdkRender : public CRhRdkSdkRender
{
public:
	OspreySdkRender(
        const std::shared_ptr<Osprey::Settings>&,
		const CRhinoCommandContext&,
		CRhinoRenderPlugIn&,
		const ON_wString& sCaption,
		UINT idIcon);
	virtual ~OspreySdkRender();

	void ThreadedRender(void);
	void SetContinueModal(bool);

	CRhinoSdkRender::RenderReturnCodes Render(const ON_2iSize& sizeRender) override;
	CRhinoSdkRender::RenderReturnCodes RenderWindow(CRhinoView*, const LPRECT, bool bInPopupWindow) override;

    virtual BOOL RenderSceneWithNoMeshes() override;
    virtual BOOL IgnoreRhinoObject(const CRhinoObject*) override;
	virtual BOOL RenderPreCreateWindow() override;
    virtual BOOL RenderEnterModalLoop() override;
	virtual BOOL RenderContinueModal() override;
    virtual BOOL RenderExitModalLoop() override;
    virtual bool ReuseRenderWindow(void) const override;
	virtual BOOL NeedToProcessGeometryTable() override;
	virtual BOOL NeedToProcessLightTable() override;
	virtual BOOL StartRenderingInWindow(CRhinoView*, const LPCRECT) override;
	virtual void StopRendering() override;
	virtual void StartRendering() override;

private:
	bool m_bContinueModal = true;
	bool m_bRenderQuick = false;
	bool m_bCancel = false;

    Osprey::Options _options;
    std::shared_ptr<Osprey::Update> _update;
    std::shared_ptr<Osprey::Scene> _scene;
    std::shared_ptr<Osprey::ChangeQueue> _changeQueue;
    std::shared_ptr<Osprey::Render> _render;
    std::thread _renderThread;
};
