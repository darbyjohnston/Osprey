// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"
#include "OspreyUtil.h"

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

    ospcommon::math::vec2i _windowSize;
    ospcommon::math::box2i _renderRect;
    std::shared_ptr<Osprey::Data> _data;
    std::shared_ptr<Osprey::ChangeQueue> _changeQueue;
    std::thread _renderThread;
    std::shared_ptr<Osprey::Render> _render;
    size_t _passes = 0;
    size_t _previewPasses = 0;
};
