// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyChangeQueue.h"
#include "OspreyPlugIn.h"
#include "OspreyRender.h"
#include "OspreySdkRender.h"
#include "OspreySettings.h"
#include "OspreyUtil.h"

OspreySdkRender::OspreySdkRender(
    const std::shared_ptr<Osprey::Settings>& settings,
	const CRhinoCommandContext& context,
	CRhinoRenderPlugIn& plugin,
	const ON_wString& sCaption,
	UINT id) :
	CRhRdkSdkRender(context, plugin, sCaption, id)
{
    _options.rendererName = Osprey::getRendererValue(settings->observeRenderer()->get());
    _options.supportsMaterials = Osprey::getRendererSupportsMaterials(settings->observeRenderer()->get());
    _options.passes = Osprey::getPassesValue(settings->observePasses()->get());
    _options.previewPasses = Osprey::getPreviewPassesValue(settings->observePreviewPasses()->get());
    _options.pixelSamples = Osprey::getPixelSamplesValue(settings->observePixelSamples()->get());
    _options.aoSamples = Osprey::getAOSamplesValue(settings->observeAOSamples()->get());
    _options.denoiserFound = settings->observeDenoiserFound()->get();
    _options.denoiserEnabled = settings->observeDenoiserEnabled()->get();
    _options.toneMapperEnabled = settings->observeToneMapperEnabled()->get();
    _options.toneMapperExposure = Osprey::getExposureValue(settings->observeToneMapperExposure()->get());
    _options.flipY = true;

    _update = std::make_shared<Osprey::Update>();

    _scene = std::make_shared<Osprey::Scene>();
    _scene->world = ospray::cpp::World();

    const auto rhinoDoc = context.Document();
    const auto& view = RhinoApp().ActiveView()->ActiveViewport().View();
    _changeQueue = std::shared_ptr<Osprey::ChangeQueue>(new Osprey::ChangeQueue(*rhinoDoc, view, _update, _scene));
    _changeQueue->setRendererName(_options.rendererName, _options.supportsMaterials);
    _changeQueue->CreateWorld();

    _render = Osprey::Render::create();

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

CRhinoSdkRender::RenderReturnCodes OspreySdkRender::Render(const ON_2iSize& renderSize)
{
	if (!::RhRdkIsAvailable())
		return CRhinoSdkRender::render_error_starting_render;

	auto& rdkRenderWindow = GetRenderWindow();
	rdkRenderWindow.SetSize(renderSize);

    _scene->renderSize = Osprey::fromRhino(renderSize);
    _scene->renderRect = ospcommon::math::box2i(
        ospcommon::math::vec2i(0, 0),
        ospcommon::math::vec2i(renderSize.cx, renderSize.cy));

    CRhinoSdkRender::RenderReturnCodes rc = CRhRdkSdkRender::Render(renderSize);
	return rc;
}

CRhinoSdkRender::RenderReturnCodes OspreySdkRender::RenderWindow(CRhinoView* rhinoView, const LPRECT pRect, bool bInPopupWindow)
{
	if (!::RhRdkIsAvailable())
		return CRhinoSdkRender::render_error_starting_render;

	CRhinoDoc* rhinoDoc = CommandContext().Document();
	if (!rhinoDoc)
		return CRhinoSdkRender::render_error_starting_render;

    _scene->renderSize = Osprey::fromRhino(RenderSize(*rhinoDoc, true));
    _scene->renderRect = ospcommon::math::box2i(
        ospcommon::math::vec2i(pRect->left, pRect->top),
        ospcommon::math::vec2i(pRect->right, pRect->bottom));

	auto& rdkRenderWindow = GetRenderWindow();
	rdkRenderWindow.SetSize(Osprey::toRhino(_scene->renderRect.size()));

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

BOOL OspreySdkRender::RenderSceneWithNoMeshes()
{
    return TRUE;
}

BOOL OspreySdkRender::IgnoreRhinoObject(const CRhinoObject*)
{
    return FALSE;
}

BOOL OspreySdkRender::RenderPreCreateWindow()
{
	::OspreyPlugIn().SetSceneChanged(FALSE);
	::OspreyPlugIn().SetLightingChanged(FALSE);

	return TRUE;
}

BOOL OspreySdkRender::RenderEnterModalLoop()
{
    return TRUE;
}

BOOL OspreySdkRender::RenderContinueModal()
{
	return m_bContinueModal;
}

BOOL OspreySdkRender::RenderExitModalLoop()
{
    return TRUE;
}

bool OspreySdkRender::ReuseRenderWindow(void) const
{
    return true;
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

    _render->init(_options, _scene);

	auto& rhinoRenderWindow = GetRenderWindow();
    const size_t totalPasses = _options.passes + _options.previewPasses;
	for (size_t pass = 0; pass < totalPasses && !m_bCancel; ++pass)
	{
        ON_wString s = ON_wString::FormatToString(L"Rendering pass %d...", pass + 1);
        rhinoRenderWindow.SetProgress(s, static_cast<int>(pass / static_cast<float>(totalPasses) * 100));

        _render->render(pass, rhinoRenderWindow);

        if (pass == totalPasses - 1)
        {
            rhinoRenderWindow.SetProgress("Render finished.", 100);
        }
	}

	SetContinueModal(false);
}
