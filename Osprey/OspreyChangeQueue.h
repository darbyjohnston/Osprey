// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyUtil.h"

namespace Osprey
{
    struct DisplayModeData;

    class ChangeQueue : public RhRdk::Realtime::ChangeQueue
    {
	public:
        ChangeQueue(
            const CRhinoDoc&,
            const ON_3dmView&,
            const std::shared_ptr<Data>&);
        ~ChangeQueue() override;

        void setRendererName(const std::string&, bool supportsMaterials = true);

        void Flush(bool bApplyChanges = true) override;

        void NotifyBeginUpdates() const override;
        void NotifyEndUpdates() const override;
        void NotifyDynamicUpdatesAreAvailable() const override;

		void ApplyViewChange(const ON_3dmView&) const override;
        void ApplyDynamicObjectTransforms(const ON_SimpleArray<const DynamicObject*>&) const;
        void ApplyDynamicLightChanges(const ON_SimpleArray<const ON_Light*>&) const;
		void ApplyMeshChanges(const ON_SimpleArray<const UUID*>& deleted, const ON_SimpleArray<const Mesh*>& addedOrChanged) const override;
		void ApplyMeshInstanceChanges(const ON_SimpleArray<ON__UINT32>& deleted, const ON_SimpleArray<const MeshInstance*>& addedOrChanged) const override;
		void ApplySunChanges(const ON_Light&un) const override;
		void ApplySkylightChanges(const Skylight&) const override;
		void ApplyLightChanges(const ON_SimpleArray<const Light*>&) const override;
        void ApplyMaterialChanges(const ON_SimpleArray<const Material*>&) const override;
		void ApplyEnvironmentChanges(IRhRdkCurrentEnvironment::Usage) const override;
		void ApplyGroundPlaneChanges(const GroundPlane&) const override;
        void ApplyLinearWorkflowChanges(const IRhRdkLinearWorkflow&) const;
        void ApplyRenderSettingsChanges(const ON_3dmRenderSettings&) const;
        void ApplyClippingPlaneChanges(const ON_SimpleArray<const UUID*>& deleted, const ON_SimpleArray<const ClippingPlane*>& addedOrChanged) const;
        void ApplyDynamicClippingPlaneChanges(const ON_SimpleArray<const ClippingPlane*>&) const;

        eRhRdkBakingFunctions BakeFor() const override;
        bool ProvideOriginalObject() const override;

    private:
        static void _convertMesh(const ON_Mesh*, Mesh&);
        static void _convertLight(const ON_Light&, const ON_Viewport&, ospray::cpp::Light&);
        static void _convertMaterial(const CRhRdkMaterial*, ospray::cpp::Material&);
        ospray::cpp::Material _getMaterial(const CRhRdkMaterial*);

        const CRhinoDoc& _rhinoDoc;
        std::shared_ptr<Data> _data;
        std::string _rendererName;
        bool _supportsMaterials = true;
        std::map<ON_UUID, std::vector<ospray::cpp::Geometry> > _geometry;
        //! \todo Is the material instance name the right key to use?
        std::map<const std::wstring, ospray::cpp::Material> _materials;
        std::map<ON__UINT32, ospray::cpp::Instance> _instances;
        bool _instancesInit = true;
        std::shared_ptr<ospray::cpp::Light>_sun;
        std::shared_ptr<ospray::cpp::Light> _ambient;
        std::map<ON_UUID, ospray::cpp::Light> _lights;
        bool _lightsInit = true;
        std::shared_ptr<ospray::cpp::Instance> _groundPlane;
    };

} // Osprey
