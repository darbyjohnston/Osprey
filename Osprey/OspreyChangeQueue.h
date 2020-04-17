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

		eRhRdkBakingFunctions BakeFor() const override;
		bool ProvideOriginalObject() const override;

		void ApplyViewChange(const ON_3dmView&) const override;
		void ApplyMeshChanges(const ON_SimpleArray<const UUID*>& deleted, const ON_SimpleArray<const Mesh*>& addedOrChanged) const override;
		void ApplyMeshInstanceChanges(const ON_SimpleArray<ON__UINT32>& deleted, const ON_SimpleArray<const MeshInstance*>& addedOrChanged) const override;
		void ApplySunChanges(const ON_Light&un) const override;
		void ApplySkylightChanges(const Skylight&) const override;
		void ApplyLightChanges(const ON_SimpleArray<const Light*>&) const override;
		void ApplyEnvironmentChanges(IRhRdkCurrentEnvironment::Usage) const override;
		void ApplyGroundPlaneChanges(const GroundPlane&) const override;

        void NotifyBeginUpdates() const override;
        void NotifyEndUpdates() const override;
        void NotifyDynamicUpdatesAreAvailable() const override;
        void Flush(bool bApplyChanges = true) override;

    private:
        static void _convertMesh(const ON_Mesh*, const std::shared_ptr<ospray::cpp::Geometry>&);
        std::shared_ptr<ospray::cpp::Material> _getMaterial(ON__UINT32);

        const CRhinoDoc& _rhinoDoc;
        std::shared_ptr<Data> _data;
        std::string _rendererName;
        bool _supportsMaterials = true;
        std::map<ON_UUID, std::vector<std::shared_ptr<ospray::cpp::Geometry> > > _geometry;
        std::map<const CRhRdkMaterial*, std::shared_ptr<ospray::cpp::Material> > _materials;
        std::map<ON__UINT32, std::shared_ptr<ospray::cpp::Instance> > _instances;
        std::shared_ptr<ospray::cpp::Light>_sun;
        std::shared_ptr<ospray::cpp::Light> _ambient;
        std::map<ON_UUID, ospray::cpp::Light> _lights;
        std::shared_ptr<ospray::cpp::Instance> _groundPlane;
    };

} // Osprey
