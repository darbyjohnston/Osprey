#pragma once

namespace Osprey
{
    class ChangeQueue : public RhRdk::Realtime::ChangeQueue
    {
	public:
		ChangeQueue(const CRhinoDoc&, const ON_3dmView&);

		const ospray::cpp::World& getWorld() const;
		const ospray::cpp::Camera& getCamera() const;

        void setWorldBBox(const ON_BoundingBox&);
        void setRendererName(const std::string&);

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
        void Flush(bool bApplyChanges = true) override;

    private:
        static void _convertMesh(const ON_Mesh*, ospray::cpp::Geometry&);
        ospray::cpp::Material _getMaterial(ON__UINT32);

        ON_BoundingBox _worldBBox;
        std::string _rendererName;
        std::map<ON_UUID, std::vector<ospray::cpp::Geometry> > _geometry;
        std::map<const CRhRdkMaterial*, ospray::cpp::Material> _materials;
        std::map<ON__UINT32, ospray::cpp::Instance> _instances;
        std::vector<ospray::cpp::Light> _lights;
        ospray::cpp::Instance _groundPlane;
        ospray::cpp::World _world;
		ospray::cpp::Camera _camera;
    };

} // Osprey
