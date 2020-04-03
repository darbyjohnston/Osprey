// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyChangeQueue.h"
#include "OspreySdkRender.h"
#include "OspreyPlugIn.h"
#include "OspreyUtil.h"

OspreySdkRender::OspreySdkRender(
    const std::shared_ptr<Osprey::ChangeQueue>& changeQueue,
    const std::shared_ptr<Osprey::Render>& render,
	const CRhinoCommandContext& context,
	CRhinoRenderPlugIn& plugin,
	const ON_wString& sCaption,
	UINT id) :
	CRhRdkSdkRender(context, plugin, sCaption, id),
    _changeQueue(changeQueue),
    _render(render)
{
	auto& rhinoRenderWindow = GetRenderWindow();
	rhinoRenderWindow.ClearChannels();
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanDistanceFromCamera, sizeof(float));
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanNormalX, sizeof(float));
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanNormalY, sizeof(float));
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanNormalZ, sizeof(float));
}

OspreySdkRender::~OspreySdkRender()
{
    if (_renderThread.joinable())
    {
        _renderThread.join();
    }
}

CRhinoSdkRender::RenderReturnCodes OspreySdkRender::Render(const ON_2iSize& sizeRender)
{
	if (!::RhRdkIsAvailable())
		return CRhinoSdkRender::render_error_starting_render;

	_render->setRenderSize(
		Osprey::fromRhino(sizeRender),
		ospcommon::math::box2i(
			ospcommon::math::vec2i(0, 0),
			ospcommon::math::vec2i(sizeRender.cx, sizeRender.cy)));

    CRhinoSdkRender::RenderReturnCodes rc = CRhRdkSdkRender::Render(sizeRender);
	return rc;
}

CRhinoSdkRender::RenderReturnCodes OspreySdkRender::RenderWindow(CRhinoView* rhinoView, const LPRECT pRect, bool bInPopupWindow)
{
	if (!::RhRdkIsAvailable())
		return CRhinoSdkRender::render_error_starting_render;

	CRhinoDoc* rhinoDoc = CommandContext().Document();
	if (!rhinoDoc)
		return CRhinoSdkRender::render_error_starting_render;

    _render->setRenderSize(
		Osprey::fromRhino(RenderSize(*rhinoDoc, true)),
		ospcommon::math::box2i(
			ospcommon::math::vec2i(pRect->left, pRect->top),
			ospcommon::math::vec2i(pRect->right, pRect->bottom)));

	CRhinoSdkRender::RenderReturnCodes rc;
	if (bInPopupWindow)
	{
		const ON_4iRect rect(pRect->left, pRect->top, pRect->right, pRect->bottom);
		rc = __super::Render(rect.Size());
	}
	else
	{
		rc = __super::RenderWindow(rhinoView, pRect, bInPopupWindow);
	}
	return rc;
}

BOOL OspreySdkRender::NeedToProcessGeometryTable()
{
	return ::OspreyPlugIn().SceneChanged();
}

BOOL OspreySdkRender::NeedToProcessLightTable()
{
	return ::OspreyPlugIn().LightingChanged();
}

BOOL OspreySdkRender::RenderPreCreateWindow()
{
	::OspreyPlugIn().SetSceneChanged(FALSE);
	::OspreyPlugIn().SetLightingChanged(FALSE);

	return TRUE;
}

BOOL OspreySdkRender::RenderContinueModal()
{
	return m_bContinueModal;
}

void OspreySdkRender::SetContinueModal(bool b)
{
	m_bContinueModal = b;
}

void OspreySdkRender::StartRendering()
{
    _renderThread = std::thread(&OspreySdkRender::ThreadedRender, this);
}

BOOL OspreySdkRender::StartRenderingInWindow(CRhinoView*, const LPCRECT)
{
    _renderThread = std::thread(&OspreySdkRender::ThreadedRender, this);
    return TRUE;
}

void OspreySdkRender::StopRendering()
{
    m_bCancel = true;
    if (_renderThread.joinable())
    {
        _renderThread.join();
    }
}

void OspreySdkRender::ThreadedRender(void)
{
	m_bCancel = false;

	auto& rhinoRenderWindow = GetRenderWindow();
    _render->initRender(rhinoRenderWindow);
	const size_t passes = _render->getPasses();
	for (size_t i = 0; i < passes; ++i)
	{
        _render->renderPass(i, rhinoRenderWindow);
		if (m_bCancel)
			break;
	}

	SetContinueModal(false);
}
