#include "stdafx.h"
#include "OspreyPlugin.h"
#include "OspreyRenderUI.h"
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
        for (const auto& i : getRendererLabels())
        {
            _rendererComboBox.AddString(i.c_str());
        }
        _rendererComboBox.SetCurSel(static_cast<int>(_settings->observeRenderer()->get()));

        _passesComboBox.ResetContent();
        for (const auto& i : getPassesLabels())
        {
            _passesComboBox.AddString(i.c_str());
        }
        _passesComboBox.SetCurSel(static_cast<int>(_settings->observePasses()->get()));

        _pixelSamplesComboBox.ResetContent();
        for (const auto& i : getPixelSamplesLabels())
        {
            _pixelSamplesComboBox.AddString(i.c_str());
        }
        _pixelSamplesComboBox.SetCurSel(static_cast<int>(_settings->observePixelSamples()->get()));

        _aoSamplesComboBox.ResetContent();
        for (const auto& i : getAOSamplesLabels())
        {
            _aoSamplesComboBox.AddString(i.c_str());
        }
        _aoSamplesComboBox.SetCurSel(static_cast<int>(_settings->observeAOSamples()->get()));

        _denoiserCheckBox.SetCheck(_settings->observeDenoiserEnabled()->get() ? BST_CHECKED : BST_UNCHECKED);
    }

    BEGIN_MESSAGE_MAP(RenderUI, CRhRdkRenderSettingsSection_MFC)
        ON_CBN_SELCHANGE(IDD_OPTIONS_RENDERER_COMBOBOX, OnRendererComboBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_PASSES_COMBOBOX, OnPassesComboBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_PIXEL_SAMPLES_COMBOBOX, OnPixelSamplesComboBox)
        ON_CBN_SELCHANGE(IDD_OPTIONS_AO_SAMPLES_COMBOBOX, OnAOSamplesComboBox)
        ON_BN_CLICKED(IDD_OPTIONS_DENOISER_CHECKBOX, OnDenoiserCheckBox)
    END_MESSAGE_MAP()

    void RenderUI::DoDataExchange(CDataExchange* pDX)
    {
        DDX_Control(pDX, IDD_OPTIONS_RENDERER_COMBOBOX, _rendererComboBox);
        DDX_Control(pDX, IDD_OPTIONS_PASSES_COMBOBOX, _passesComboBox);
        DDX_Control(pDX, IDD_OPTIONS_PIXEL_SAMPLES_COMBOBOX, _pixelSamplesComboBox);
        DDX_Control(pDX, IDD_OPTIONS_AO_SAMPLES_COMBOBOX, _aoSamplesComboBox);
        DDX_Control(pDX, IDD_OPTIONS_DENOISER_CHECKBOX, _denoiserCheckBox);
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

} // Osprey
