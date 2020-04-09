// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyPlugin.h"
#include "OspreyRenderUI.h"
#include "OspreySettings.h"
#include "Resource.h"

namespace Osprey
{
    namespace
    {
        const UUID uuid =
        {
            0x5f0b612f,
            0xb47f,
            0x446f,
            { 0x97, 0x2c, 0x17, 0x62, 0x77, 0xbb, 0x8a, 0xf2 }
        };
    
    } // namespace

    RenderUI::RenderUI(const std::shared_ptr<Settings>& settings, RhRdkUiModalities rhinoModalities) :
        CRhRdkRenderSettingsSection_MFC(rhinoModalities, IDD_OPTIONS_SECTION),
        _settings(settings)
    {
        _rendererObserver = ValueObserver<Renderer>::create(
            settings->observeRenderer(),
            [this](Renderer value)
        {
            _rendererComboBox.SetCurSel(static_cast<int>(value));
        });
        _passesObserver = ValueObserver<Passes>::create(
            settings->observePasses(),
            [this](Passes value)
        {
            _passesComboBox.SetCurSel(static_cast<int>(value));
        });
        _previewPassesObserver = ValueObserver<PreviewPasses>::create(
            settings->observePreviewPasses(),
            [this](PreviewPasses value)
        {
            _previewPassesComboBox.SetCurSel(static_cast<int>(value));
        });
        _pixelSamplesObserver = ValueObserver<PixelSamples>::create(
            settings->observePixelSamples(),
            [this](PixelSamples value)
        {
            _pixelSamplesComboBox.SetCurSel(static_cast<int>(value));
        });
        _aoSamplesObserver = ValueObserver<AOSamples>::create(
            settings->observeAOSamples(),
            [this](AOSamples value)
        {
            _aoSamplesComboBox.SetCurSel(static_cast<int>(value));
        });
        _denoiserFoundObserver = ValueObserver<bool>::create(
            settings->observeDenoiserFound(),
            [this](bool value)
        {
            //_denoiserCheckBox.
        });
        _denoiserEnabledObserver = ValueObserver<bool>::create(
            settings->observeDenoiserEnabled(),
            [this](bool value)
        {
            _denoiserCheckBox.SetCheck(value ? BST_CHECKED : BST_UNCHECKED);
        });
        _toneMapperEnabledObserver = ValueObserver<bool>::create(
            settings->observeToneMapperEnabled(),
            [this](bool value)
        {
            _toneMapperCheckBox.SetCheck(value ? BST_CHECKED : BST_UNCHECKED);
        });
        _toneMapperExposureObserver = ValueObserver<Exposure>::create(
            settings->observeToneMapperExposure(),
            [this](Exposure value)
        {
            _toneMapperExposureComboBox.SetCurSel(static_cast<int>(value));
        });
    }

    RenderUI::~RenderUI()
    {}

    unsigned int RenderUI::GetIDD(void) const
    {
        return IDD_OPTIONS_SECTION;
    }

    UUID RenderUI::Uuid(void) const
    {
        return uuid;
    }

    ON_wString RenderUI::Caption(bool bAlwaysEnglish) const
    {
        return L"Osprey";
    }

    BOOL RenderUI::OnInitDialog()
    {
        CRhRdkRenderSettingsSection_MFC::OnInitDialog();

        return TRUE;
    }

    void RenderUI::DisplayData(void)
    {
        CRhRdkRenderSettingsSection_MFC::DisplayData();

        if (RhinoApp().GetDefaultRenderApp() != OspreyPlugIn().PlugInID())
            return;

        _rendererComboBox.ResetContent();
        for (const auto& i : getRendererEnums())
        {
            _rendererComboBox.AddString(getRendererLabel(i).c_str());
        }
        _rendererComboBox.SetCurSel(static_cast<int>(_settings->observeRenderer()->get()));

        _passesComboBox.ResetContent();
        for (const auto& i : getPassesEnums())
        {
            _passesComboBox.AddString(getPassesLabel(i).c_str());
        }
        _passesComboBox.SetCurSel(static_cast<int>(_settings->observePasses()->get()));

        _previewPassesComboBox.ResetContent();
        for (const auto& i : getPreviewPassesEnums())
        {
            _previewPassesComboBox.AddString(getPreviewPassesLabel(i).c_str());
        }
        _previewPassesComboBox.SetCurSel(static_cast<int>(_settings->observePreviewPasses()->get()));

        _pixelSamplesComboBox.ResetContent();
        for (const auto& i : getPixelSamplesEnums())
        {
            _pixelSamplesComboBox.AddString(getPixelSamplesLabel(i).c_str());
        }
        _pixelSamplesComboBox.SetCurSel(static_cast<int>(_settings->observePixelSamples()->get()));

        _aoSamplesComboBox.ResetContent();
        for (const auto& i : getAOSamplesEnums())
        {
            _aoSamplesComboBox.AddString(getAOSamplesLabel(i).c_str());
        }
        _aoSamplesComboBox.SetCurSel(static_cast<int>(_settings->observeAOSamples()->get()));

        _denoiserCheckBox.SetCheck(_settings->observeDenoiserEnabled()->get() ? BST_CHECKED : BST_UNCHECKED);

        _toneMapperCheckBox.SetCheck(_settings->observeToneMapperEnabled()->get() ? BST_CHECKED : BST_UNCHECKED);

        _toneMapperExposureComboBox.ResetContent();
        for (const auto& i : getExposureEnums())
        {
            _toneMapperExposureComboBox.AddString(getExposureLabel(i).c_str());
        }
        _toneMapperExposureComboBox.SetCurSel(static_cast<int>(_settings->observeToneMapperExposure()->get()));
    }

    BEGIN_MESSAGE_MAP(RenderUI, CRhRdkRenderSettingsSection_MFC)
        ON_CBN_SELCHANGE(IDD_OPTIONS_RENDERER_COMBOBOX, OnRendererComboBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_PASSES_COMBOBOX, OnPassesComboBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_PREVIEW_PASSES_COMBOBOX, OnPreviewPassesComboBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_PIXEL_SAMPLES_COMBOBOX, OnPixelSamplesComboBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_AO_SAMPLES_COMBOBOX, OnAOSamplesComboBox)
        ON_BN_CLICKED(IDD_OPTIONS_DENOISER_CHECKBOX, OnDenoiserCheckBox)
        ON_BN_CLICKED(IDD_OPTIONS_TONE_MAPPER_CHECKBOX, OnToneMapperCheckBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_TONE_MAPPER_EXPOSURE_COMBOBOX, OnToneMapperExposureComboBox)
    END_MESSAGE_MAP()

    void RenderUI::DoDataExchange(CDataExchange* pDX)
    {
        DDX_Control(pDX, IDD_OPTIONS_RENDERER_COMBOBOX, _rendererComboBox);
        DDX_Control(pDX, IDD_OPTIONS_PASSES_COMBOBOX, _passesComboBox);
        DDX_Control(pDX, IDD_OPTIONS_PREVIEW_PASSES_COMBOBOX, _previewPassesComboBox);
        DDX_Control(pDX, IDD_OPTIONS_PIXEL_SAMPLES_COMBOBOX, _pixelSamplesComboBox);
        DDX_Control(pDX, IDD_OPTIONS_AO_SAMPLES_COMBOBOX, _aoSamplesComboBox);
        DDX_Control(pDX, IDD_OPTIONS_DENOISER_CHECKBOX, _denoiserCheckBox);
        DDX_Control(pDX, IDD_OPTIONS_TONE_MAPPER_CHECKBOX, _toneMapperCheckBox);
        DDX_Control(pDX, IDD_OPTIONS_TONE_MAPPER_EXPOSURE_COMBOBOX, _toneMapperExposureComboBox);
        __super::DoDataExchange(pDX);
    }

    UUID RenderUI::PlugInId(void) const
    {
        return OspreyPlugIn().PlugInID();
    }

    AFX_MODULE_STATE* RenderUI::GetModuleState(void) const
    {
        return AfxGetStaticModuleState();
    }

    void RenderUI::OnEvent(IRhinoUiController& con, const UUID& uuidData, IRhinoUiController::EventPriority ep, const IRhinoUiEventInfo* pInfo)
    {
    }

    CRhinoCommandOptionName RenderUI::CommandOptionName(void) const
    {
        return CRhinoCommandOptionName(L"Osprey", L"Osprey");
    }

    CRhinoCommand::result RenderUI::RunScript(CRhRdkControllerPtr)
    {
        return CRhinoCommand::result::success;
    }

    void RenderUI::OnRendererComboBox()
    {
        _settings->setRenderer(static_cast<Renderer>(_rendererComboBox.GetCurSel()));
    }

    void RenderUI::OnPassesComboBox()
    {
        _settings->setPasses(static_cast<Passes>(_passesComboBox.GetCurSel()));
    }

    void RenderUI::OnPreviewPassesComboBox()
    {
        _settings->setPreviewPasses(static_cast<PreviewPasses>(_previewPassesComboBox.GetCurSel()));
    }

    void RenderUI::OnPixelSamplesComboBox()
    {
        _settings->setPixelSamples(static_cast<PixelSamples>(_pixelSamplesComboBox.GetCurSel()));
    }

    void RenderUI::OnAOSamplesComboBox()
    {
        _settings->setAOSamples(static_cast<AOSamples>(_aoSamplesComboBox.GetCurSel()));
    }

    void RenderUI::OnDenoiserCheckBox()
    {
        const bool value = !(_denoiserCheckBox.GetCheck() == BST_CHECKED);
        _settings->setDenoiserEnabled(value);
    }

    void RenderUI::OnToneMapperCheckBox()
    {
        const bool value = !(_toneMapperCheckBox.GetCheck() == BST_CHECKED);
        _settings->setToneMapperEnabled(value);
    }

    void RenderUI::OnToneMapperExposureComboBox()
    {
        _settings->setToneMapperExposure(static_cast<Exposure>(_toneMapperExposureComboBox.GetCurSel()));
    }

} // namespace Osprey
