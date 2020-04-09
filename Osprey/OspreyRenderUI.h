// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"
#include "OspreyValueObserver.h"

namespace Osprey
{
    class Settings;

    class RenderUI : public CRhRdkRenderSettingsSection_MFC
    {
    public:
        RenderUI(const std::shared_ptr<Settings>&, RhRdkUiModalities);
        ~RenderUI() override;

	protected:
		unsigned int GetIDD(void) const override;
		UUID Uuid(void) const override;
		ON_wString Caption(bool bAlwaysEnglish) const override;
		BOOL OnInitDialog() override;
		void DisplayData(void) override;
		void DoDataExchange(CDataExchange*) override;
		UUID PlugInId(void) const override;
		AFX_MODULE_STATE* GetModuleState(void) const override;
		void OnEvent(IRhinoUiController&, const UUID&, IRhinoUiController::EventPriority, const IRhinoUiEventInfo*) override;
		CRhinoCommandOptionName CommandOptionName(void) const override;
		CRhinoCommand::result RunScript(CRhRdkControllerPtr) override;

	protected:
		afx_msg void OnRendererComboBox();
        afx_msg void OnPassesComboBox();
        afx_msg void OnPreviewPassesComboBox();
        afx_msg void OnPixelSamplesComboBox();
		afx_msg void OnAOSamplesComboBox();
        afx_msg void OnDenoiserCheckBox();
        afx_msg void OnToneMapperCheckBox();
        afx_msg void OnToneMapperExposureComboBox();
        DECLARE_MESSAGE_MAP()

	private:
		std::shared_ptr<Settings> _settings;

		CComboBox _rendererComboBox;
        CComboBox _passesComboBox;
        CComboBox _previewPassesComboBox;
        CComboBox _pixelSamplesComboBox;
		CComboBox _aoSamplesComboBox;
        CButton _denoiserCheckBox;
        CButton _toneMapperCheckBox;
        CComboBox _toneMapperExposureComboBox;

		std::shared_ptr<ValueObserver<Renderer> > _rendererObserver;
        std::shared_ptr<ValueObserver<Passes> > _passesObserver;
        std::shared_ptr<ValueObserver<PreviewPasses> > _previewPassesObserver;
        std::shared_ptr<ValueObserver<PixelSamples> > _pixelSamplesObserver;
		std::shared_ptr<ValueObserver<AOSamples> > _aoSamplesObserver;
		std::shared_ptr<ValueObserver<bool> > _denoiserFoundObserver;
		std::shared_ptr<ValueObserver<bool> > _denoiserEnabledObserver;
        std::shared_ptr<ValueObserver<bool> > _toneMapperEnabledObserver;
        std::shared_ptr<ValueObserver<Exposure> > _toneMapperExposureObserver;
    };

} // namespace Osprey
