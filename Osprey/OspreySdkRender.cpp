// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreySdkRender.h"
#include "OspreyPlugIn.h"
#include "OspreyUtil.h"

OspreySdkRender::OspreySdkRender(
	const std::shared_ptr<Osprey::Settings>& settings,
	const CRhinoCommandContext& context,
	CRhinoRenderPlugIn& plugin,
	const ON_wString& sCaption,
	UINT id,
	bool bPreview) :
	CRhRdkSdkRender(context, plugin, sCaption, id)
{
	m_bRenderQuick = bPreview;
	m_bCancel = false;
	m_bContinueModal = true;
	m_hRenderThread = NULL;

	auto& rhinoRenderWindow = GetRenderWindow();
	rhinoRenderWindow.ClearChannels();
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanDistanceFromCamera, sizeof(float));
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanNormalX, sizeof(float));
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanNormalY, sizeof(float));
	rhinoRenderWindow.AddChannel(IRhRdkRenderWindow::chanNormalZ, sizeof(float));

	_osprey = Osprey::Render::create();
	_osprey->setRendererName(Osprey::getRendererValues()[static_cast<size_t>(settings->observeRenderer()->get())]);
	_osprey->setPasses(Osprey::getPassesValues()[static_cast<size_t>(settings->observePasses()->get())]);
	_osprey->setPixelSamples(Osprey::getPixelSamplesValues()[static_cast<size_t>(settings->observePixelSamples()->get())]);
	_osprey->setAOSamples(Osprey::getAOSamplesValues()[static_cast<size_t>(settings->observeAOSamples()->get())]);
	_osprey->setDenoiserFound(settings->observeDenoiserFound()->get());
	_osprey->setDenoiserEnabled(settings->observeDenoiserEnabled()->get());
}

OspreySdkRender::~OspreySdkRender()
{
	if (m_hRenderThread != NULL)
	{
		::WaitForSingleObject(m_hRenderThread, INFINITE);
		::CloseHandle(m_hRenderThread);
		m_hRenderThread = NULL;
	}
}

void OspreySdkRender::setRenderer(Osprey::Renderer value)
{
	_osprey->setRendererName(Osprey::getRendererValues()[static_cast<size_t>(value)]);
}

void OspreySdkRender::setPasses(Osprey::Passes value)
{
	_osprey->setPasses(Osprey::getPassesValues()[static_cast<size_t>(value)]);
}

void OspreySdkRender::setPixelSamples(Osprey::PixelSamples value)
{
	_osprey->setPixelSamples(Osprey::getPixelSamplesValues()[static_cast<size_t>(value)]);
}

void OspreySdkRender::setAOSamples(Osprey::AOSamples value)
{
	_osprey->setAOSamples(Osprey::getAOSamplesValues()[static_cast<size_t>(value)]);
}

void OspreySdkRender::setDenoiserEnabled(bool value)
{
	_osprey->setDenoiserEnabled(value);
}

void OspreySdkRender::setDenoiserFound(bool value)
{
	_osprey->setDenoiserFound(value);
}

CRhinoSdkRender::RenderReturnCodes OspreySdkRender::Render(const ON_2iSize& sizeRender)
{
	if (!::RhRdkIsAvailable())
		return CRhinoSdkRender::render_error_starting_render;

	CRhinoDoc* rhinoDoc = CommandContext().Document();
	if (nullptr == rhinoDoc)
		return CRhinoSdkRender::render_error_starting_render;

	_osprey->setRenderSize(
		Osprey::fromRhino(sizeRender),
		ospcommon::math::box2i(
			ospcommon::math::vec2i(0, 0),
			ospcommon::math::vec2i(sizeRender.cx, sizeRender.cy)));
	_osprey->convert(rhinoDoc);

	CRhinoSdkRender::RenderReturnCodes rc = CRhRdkSdkRender::Render(sizeRender);
	return rc;
}

CRhinoSdkRender::RenderReturnCodes OspreySdkRender::RenderWindow(CRhinoView* pView, const LPRECT pRect, bool bInPopupWindow)
{
	if (!::RhRdkIsAvailable())
		return CRhinoSdkRender::render_error_starting_render;

	CRhinoDoc* rhinoDoc = CommandContext().Document();
	if (nullptr == rhinoDoc)
		return CRhinoSdkRender::render_error_starting_render;

	_osprey->setRenderSize(
		Osprey::fromRhino(RenderSize(*rhinoDoc, true)),
		ospcommon::math::box2i(
			ospcommon::math::vec2i(pRect->left, pRect->top),
			ospcommon::math::vec2i(pRect->right, pRect->bottom)));
	_osprey->convert(rhinoDoc);

	CRhinoSdkRender::RenderReturnCodes rc;
	if (bInPopupWindow)
	{
		const ON_4iRect rect(pRect->left, pRect->top, pRect->right, pRect->bottom);
		rc = __super::Render(rect.Size());
	}
	else
	{
		rc = __super::RenderWindow(pView, pRect, bInPopupWindow);
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

void OspreySdkRender::RenderThread(void* pv)
{
	reinterpret_cast<OspreySdkRender*>(pv)->ThreadedRender();
}

void OspreySdkRender::StartRendering()
{
	m_hRenderThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RenderThread, this, 0, NULL);
}

BOOL OspreySdkRender::StartRenderingInWindow(CRhinoView*, const LPCRECT)
{
	m_hRenderThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RenderThread, this, 0, NULL);

	return TRUE;
}

void OspreySdkRender::StopRendering()
{
	if (NULL != m_hRenderThread)
	{
		m_bCancel = true;

		::WaitForSingleObject(m_hRenderThread, INFINITE);
		::CloseHandle(m_hRenderThread);
		m_hRenderThread = NULL;
	}
}

int OspreySdkRender::ThreadedRender(void)
{
	m_bCancel = false;

	auto& rhinoRenderWindow = GetRenderWindow();
	_osprey->initRender(rhinoRenderWindow);
	const size_t passes = _osprey->getPasses();
	for (size_t i = 0; i < passes; ++i)
	{
		_osprey->renderPass(i, rhinoRenderWindow);
		if (m_bCancel)
			break;
	}

	SetContinueModal(false);
	return 0;
}
