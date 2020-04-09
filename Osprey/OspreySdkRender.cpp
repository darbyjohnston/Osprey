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
	CRhRdkSdkRender(context, plugin, sCaption, id),
    _passes(Osprey::getPassesValue(settings->observePasses()->get())),
    _previewPasses(Osprey::getPreviewPassesValue(settings->observePreviewPasses()->get()))
{
    _data = std::make_shared<Osprey::Data>();
    _data->world = std::make_shared<ospray::cpp::World>();

    const auto rhinoDoc = context.Document();
    const auto& view = RhinoApp().ActiveView()->ActiveViewport().View();
    _changeQueue = std::shared_ptr<Osprey::ChangeQueue>(new Osprey::ChangeQueue(*rhinoDoc, view, _data));
    const Osprey::Renderer renderer = settings->observeRenderer()->get();
    const std::string rendererName = Osprey::getRendererValue(renderer);
    _changeQueue->setRendererName(rendererName, Osprey::getRendererSupportsMaterials(renderer));
    _changeQueue->CreateWorld();

    _render = Osprey::Render::create();
    _render->setRendererName(rendererName);
    _render->setPreviewPasses(Osprey::getPreviewPassesValue(settings->observePreviewPasses()->get()));
    _render->setPixelSamples(Osprey::getPixelSamplesValue(settings->observePixelSamples()->get()));
    _render->setAOSamples(Osprey::getAOSamplesValue(settings->observeAOSamples()->get()));
    _render->setDenoiserFound(settings->observeDenoiserFound()->get());
    _render->setDenoiserEnabled(settings->observeDenoiserEnabled()->get());
    _render->setToneMapperEnabled(settings->observeToneMapperEnabled()->get());
    _render->setToneMapperExposure(getExposureValue(settings->observeToneMapperExposure()->get()));
    _render->setFlipY(true);

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

    _windowSize = Osprey::fromRhino(renderSize);
    _renderRect = ospcommon::math::box2i(
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

    _windowSize = Osprey::fromRhino(RenderSize(*rhinoDoc, true));
    _renderRect = ospcommon::math::box2i(
        ospcommon::math::vec2i(pRect->left, pRect->top),
        ospcommon::math::vec2i(pRect->right, pRect->bottom));

	auto& rdkRenderWindow = GetRenderWindow();
	rdkRenderWindow.SetSize(Osprey::toRhino(_renderRect.size()));

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

    _render->initRender(_windowSize, _renderRect);

	auto& rhinoRenderWindow = GetRenderWindow();
	for (size_t pass = 0; pass < _passes + _previewPasses; ++pass)
	{
        ON_wString s = ON_wString::FormatToString(L"Rendering pass %d...", pass + 1);
        rhinoRenderWindow.SetProgress(s, static_cast<int>(pass / static_cast<float>(_passes) * 100));

        _render->renderPass(pass, _data->world, _data->camera, rhinoRenderWindow);

        if (pass == _passes - 1)
        {
            rhinoRenderWindow.SetProgress("Render finished.", 100);
        }

		if (m_bCancel)
			break;
	}

	SetContinueModal(false);
}
