// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyChangeQueue.h"
#include "OspreyDisplayMode.h"
#include "OspreyUtil.h"

namespace Osprey
{
    namespace
    {
        // Angular diameter of the sun.
        const float sunAngularDiameter = .53F;

        // Curve radius.
        const float curveRadius = .05F;

        // Light intensity multiplier.
        const float lightIntensityMul = 3.F;

        // Ambient light intensity.
        const float ambientIntensity = .1F;

    } // namespace

	ChangeQueue::ChangeQueue(
        const CRhinoDoc& rhinoDoc,
        const ON_3dmView& onView,
        const std::shared_ptr<Data>& data) :
		RhRdk::Realtime::ChangeQueue(rhinoDoc, ON_nil_uuid, onView, nullptr, false, true),
        _rhinoDoc(rhinoDoc),
        _data(data)
	{}

    ChangeQueue::~ChangeQueue()
    {}

    void ChangeQueue::setRendererName(const std::string& value, bool supportsMaterials)
    {
        if (value == _rendererName && supportsMaterials == _supportsMaterials)
            return;
        _rendererName = value;
        _supportsMaterials = supportsMaterials;
        _materials.clear();
        _instances.clear();
        _groundPlane.reset();
    }

    void ChangeQueue::Flush(bool bApplyChanges)
    {
        RhRdk::Realtime::ChangeQueue::Flush(bApplyChanges);

        if (_instancesInit)
        {
            _instancesInit = false;

            std::vector<ospray::cpp::Instance> instances;
            for (const auto& i : _instances)
            {
                instances.push_back(i.second);
            }
            if (_groundPlane)
            {
                instances.push_back(*_groundPlane);
            }
            if (instances.size())
            {
                _data->world->setParam("instance", ospray::cpp::Data(instances));
                _data->world->commit();
            }
        }

        if (_lightsInit)
        {
            _lightsInit = false;

            std::vector<ospray::cpp::Light> lights;
            if (_sun)
            {
                lights.push_back(*_sun);
            }
            if (_ambient)
            {
                lights.push_back(*_ambient);
            }
            for (const auto& i : _lights)
            {
                lights.push_back(i.second);
            }
            if (lights.size())
            {
                _data->world->setParam("light", ospray::cpp::Data(lights));
                _data->world->commit();
            }
        }
    }

    void ChangeQueue::NotifyBeginUpdates() const
    {}

    void ChangeQueue::NotifyEndUpdates() const
    {
        auto that = const_cast<ChangeQueue*>(this);
        {
            std::lock_guard<std::mutex> lock(that->_data->mutex);
            that->_data->updates = true;
        }
    }

    void ChangeQueue::NotifyDynamicUpdatesAreAvailable() const
    {
        auto that = const_cast<ChangeQueue*>(this);
    }

	void ChangeQueue::ApplyViewChange(const ON_3dmView& view) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        const auto& vp = view.m_vp;
        ospcommon::math::vec3f cameraPos = fromRhino(vp.CameraLocation());
        ospcommon::math::vec3f cameraDir = fromRhino(vp.CameraDirection());
        ospcommon::math::vec3f cameraUp = fromRhino(vp.CameraUp());
        const float cameraNear = vp.PerspectiveMinNearDist();
        if (vp.IsPerspectiveProjection())
        {
            if (!_data->camera || _data->cameraType != CameraType::Perspective)
            {
                that->_data->camera = std::make_shared<ospray::cpp::Camera>("perspective");
                that->_data->cameraType = CameraType::Perspective;
            }

            _data->camera->setParam("position", cameraPos);
            _data->camera->setParam("direction", cameraDir);
            _data->camera->setParam("up", cameraUp);
            _data->camera->setParam("nearClip", cameraNear);

            double halfDiagonalAngle = 0.0;
            double halfVerticalAngle = 0.0;
            double halfHorizontalAngle = 0.0;
            vp.GetCameraAngle(&halfDiagonalAngle, &halfVerticalAngle, &halfHorizontalAngle);
            const float cameraFovY = static_cast<float>(halfVerticalAngle * 2.0 / double(ospcommon::math::two_pi) * 360.0);
            _data->camera->setParam("fovy", cameraFovY);

            _data->camera->commit();
        }
        else if (vp.IsParallelProjection())
        {
            if (!_data->camera || _data->cameraType != CameraType::Orthographic)
            {
                that->_data->camera = std::make_shared<ospray::cpp::Camera>("orthographic");
                that->_data->cameraType = CameraType::Orthographic;
            }

            _data->camera->setParam("position", cameraPos);
            _data->camera->setParam("direction", cameraDir);
            _data->camera->setParam("up", cameraUp);
            _data->camera->setParam("nearClip", cameraNear);

            const float height = static_cast<float>(vp.FrustumHeight());
            _data->camera->setParam("height", height);

            _data->camera->commit();
        }
        else
        {
            _data->camera.reset();
        }
	}

    void ChangeQueue::ApplyDynamicObjectTransforms(const ON_SimpleArray<const DynamicObject*>&) const
    {
        auto that = const_cast<ChangeQueue*>(this);
    }

    void ChangeQueue::ApplyDynamicLightChanges(const ON_SimpleArray<const ON_Light*>&) const
    {
        auto that = const_cast<ChangeQueue*>(this);
    }

    namespace
    {
        struct Mesh
        {
            ON_UUID id;
            std::vector<ospcommon::math::vec3f> v;
            std::vector<ospcommon::math::vec3f> n;
            std::vector<ospcommon::math::vec2f> t;
            std::vector<ospcommon::math::vec4f> c;
            std::vector<ospcommon::math::vec3ui> i;
        };

        class ConvertMesh
        {
        public:
            ConvertMesh(
                const ON_SimpleArray<const RhRdk::Realtime::ChangeQueue::Mesh*>& rhinoMeshes,
                std::map<ON_UUID, std::vector<Osprey::Mesh> >& meshes) :
                _rhinoMeshes(rhinoMeshes),
                _meshes(meshes)
            {}

            void operator()(const tbb::blocked_range<size_t>& r) const
            {
                for (size_t i = r.begin(); i != r.end(); ++i)
                {
                    const auto& onMeshes = _rhinoMeshes[i]->Meshes();
                    const int count = onMeshes.Count();
                    auto& meshes = _meshes[_rhinoMeshes[i]->UuidId()];
                    meshes.resize(count);
                    for (int j = 0; j < count; ++j)
                    {
                        const auto onMesh = onMeshes[j];
                        auto& mesh = meshes[j];

                        // Convert the mesh vertices. This assumes that the ON and ospcommon data types
                        // have the same memory layout...
                        int count = onMesh->m_V.Count();
                        if (count > 0)
                        {
                            mesh.v.resize(count);
                            memcpy(mesh.v.data(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_V.First()), count * sizeof(ospcommon::math::vec3f));
                        }
                        count = onMesh->m_N.Count();
                        if (count > 0)
                        {
                            mesh.n.resize(count);
                            memcpy(mesh.n.data(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_N.First()), count * sizeof(ospcommon::math::vec3f));
                        }
                        count = onMesh->m_T.Count();
                        if (count > 0)
                        {
                            mesh.t.resize(count);
                            memcpy(mesh.t.data(), reinterpret_cast<const ospcommon::math::vec2f*>(onMesh->m_T.First()), count * sizeof(ospcommon::math::vec2f));
                        }
                        count = onMesh->m_C.Count();
                        if (count > 0)
                        {
                            mesh.c.resize(count);
                            memcpy(mesh.c.data(), reinterpret_cast<const ospcommon::math::vec4f*>(onMesh->m_C.First()), count * sizeof(ospcommon::math::vec4f));
                        }

                        // Convert the mesh indices.
                        const int faceCount = onMesh->FaceCount();
                        for (const ON_MeshFace* f = onMesh->m_F, * fEnd = onMesh->m_F + faceCount; f < fEnd; ++f)
                        {
                            if (f->IsQuad())
                            {
                                mesh.i.emplace_back(ospcommon::math::vec3ui(f->vi[0], f->vi[1], f->vi[2]));
                                mesh.i.emplace_back(ospcommon::math::vec3ui(f->vi[2], f->vi[3], f->vi[0]));
                            }
                            else
                            {
                                mesh.i.emplace_back(ospcommon::math::vec3ui(f->vi[0], f->vi[1], f->vi[2]));
                            }
                        }

                        meshes[j] = mesh;
                    }
                }
            }

        private:
            const ON_SimpleArray<const RhRdk::Realtime::ChangeQueue::Mesh*>& _rhinoMeshes;
            std::map<ON_UUID, std::vector<Osprey::Mesh> >& _meshes;
        };

    } // namespace

	void ChangeQueue::ApplyMeshChanges(
        const ON_SimpleArray<const UUID*>& deleted,
        const ON_SimpleArray<const Mesh*>& addedOrChanged) const
	{
        auto that = const_cast<ChangeQueue*>(this);

        // Remove meshes.
        for (int i = 0; i < deleted.Count(); ++i)
        {
            const auto j = _geometry.find(*deleted[i]);
            if (j != _geometry.end())
            {
                that->_geometry.erase(j);
            }
        }

        // Add meshes.
        const int count = addedOrChanged.Count();
        std::map<ON_UUID, std::vector<Osprey::Mesh> > meshes;
        for (int i = 0; i < count; ++i)
        {
            meshes[addedOrChanged[i]->UuidId()] = std::vector<Osprey::Mesh>();
        }
        tbb::parallel_for(tbb::blocked_range<size_t>(0, count), ConvertMesh(addedOrChanged, meshes));

        for (const auto& i : meshes)
        {
            auto& list = that->_geometry[i.first];
            list.clear();
            for (const auto& j : i.second)
            {
                ospray::cpp::Geometry geometry;
                if (j.i.size())
                {
                    geometry = ospray::cpp::Geometry("mesh");
                    if (j.v.size())
                    {
                        geometry.setParam("vertex.position", ospray::cpp::Data(j.v));
                    }
                    if (j.n.size())
                    {
                        geometry.setParam("vertex.normal", ospray::cpp::Data(j.n));
                    }
                    if (j.t.size())
                    {
                        geometry.setParam("vertex.texcoord", ospray::cpp::Data(j.t));
                    }
                    if (j.c.size())
                    {
                        geometry.setParam("vertex.color", ospray::cpp::Data(j.c));
                    }
                    if (j.i.size())
                    {
                        geometry.setParam("index", ospray::cpp::Data(j.i));
                    }
                    geometry.commit();
                }
                list.push_back(geometry);
            }
        }
	}

	void ChangeQueue::ApplyMeshInstanceChanges(
        const ON_SimpleArray<ON__UINT32>& deleted,
        const ON_SimpleArray<const MeshInstance*>& addedOrChanged) const
	{
        auto that = const_cast<ChangeQueue*>(this);

        that->_instancesInit = true;

        // Remove instances.
        for (int i = 0; i < deleted.Count(); ++i)
        {
            const auto j = _instances.find(deleted[i]);
            if (j != _instances.end())
            {
                that->_instances.erase(j);
            }
        }

        // Add instances.
        for (int i = 0; i < addedOrChanged.Count(); ++i)
        {
            const auto rdkInstance = addedOrChanged[i];
            const auto rdkInstanceID = rdkInstance->InstanceId();
            const auto j = _instances.find(rdkInstanceID);
            if (j != _instances.end())
            {
                j->second.setParam("xfm", fromRhino(rdkInstance->InstanceXform()));
            }
            else
            {
                const auto rdkMeshID = rdkInstance->MeshId();
                const auto k = _geometry.find(rdkMeshID);
                if (k != _geometry.end())
                {
                    const int rdkMeshIndex = rdkInstance->MeshIndex();
                    if (rdkMeshIndex < k->second.size())
                    {
                        const auto& mesh = k->second[rdkMeshIndex];
                        if (mesh.handle())
                        {
                            ospray::cpp::GeometricModel model(mesh);
                            auto material = that->_getMaterial(MaterialFromId(rdkInstance->MaterialId()));
                            if (material.handle())
                            {
                                model.setParam("material", material);
                            }
                            model.commit();

                            ospray::cpp::Group group;
                            group.setParam("geometry", ospray::cpp::Data(model));
                            group.commit();

                            ospray::cpp::Instance instance(group);
                            instance.setParam("xfm", fromRhino(rdkInstance->InstanceXform()));
                            instance.commit();
                            that->_instances[rdkInstanceID] = instance;
                        }
                    }
                }
            }
        }
	}

	void ChangeQueue::ApplySunChanges(const ON_Light& rhinoSun) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        if (rhinoSun.IsEnabled())
        {
            if (!_sun)
            {
                that->_lightsInit = true;
                that->_sun = std::make_shared<ospray::cpp::Light>("distant");
            }

            _sun->setParam("direction", fromRhino(rhinoSun.Direction()));
            _sun->setParam("angularDiameter", sunAngularDiameter);
            _sun->setParam("color", static_cast<float>(rhinoSun.Diffuse()));
            _sun->setParam("intensity", static_cast<float>(rhinoSun.Intensity()) * lightIntensityMul);
            _sun->commit();
        }
        else if (_sun)
        {
            that->_lightsInit = true;
            that->_sun.reset();
        }
	}

	void ChangeQueue::ApplySkylightChanges(const Skylight& rhinoSkylight) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        if (rhinoSkylight.On())
        {
            if (!_ambient)
            {
                that->_lightsInit = true;
                that->_ambient = std::make_shared<ospray::cpp::Light>("ambient");
            }

            const float shadowIntensity = rhinoSkylight.ShadowIntensity();
            _ambient->setParam("intensity", ambientIntensity);
            _ambient->commit();
        }
        else if (_ambient)
        {
            that->_lightsInit = true;
            that->_ambient.reset();
        }
	}

	void ChangeQueue::ApplyLightChanges(const ON_SimpleArray<const Light*>& rhinoLights) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        const auto& vp = QueueView()->m_vp;
        for (int i = 0; i < rhinoLights.Count(); ++i)
        {
            const auto rhinoLight = rhinoLights[i];
            const auto& onLight = rhinoLights[i]->LightData();
            switch (rhinoLight->Event())
            {
            case CRhinoEventWatcher::light_event::light_added:
            case CRhinoEventWatcher::light_event::light_undeleted:
            {
                that->_lightsInit = true;

                if (onLight.IsEnabled())
                {
                    ospray::cpp::Light light;
                    if (onLight.IsPointLight())
                    {
                        light = ospray::cpp::Light("sphere");
                    }
                    else if (onLight.IsDirectionalLight())
                    {
                        light = ospray::cpp::Light("distant");
                    }
                    else if (onLight.IsSpotLight())
                    {
                        light = ospray::cpp::Light("spot");
                    }
                    else if (onLight.IsLinearLight())
                    {

                    }
                    else if (onLight.IsRectangularLight())
                    {

                    }
                    if (light.handle())
                    {
                        _convertLight(onLight, vp, light);
                        that->_lights[onLight.m_light_id] = light;
                    }
                }
                else
                {
                    const auto j = that->_lights.find(onLight.m_light_id);
                    if (j != that->_lights.end())
                    {
                        that->_lights.erase(j);
                    }
                }
                break;
            }
            case CRhinoEventWatcher::light_event::light_deleted:
            {
                that->_lightsInit = true;

                const auto j = that->_lights.find(onLight.m_light_id);
                if (j != that->_lights.end())
                {
                    that->_lights.erase(j);
                }
                break;
            }
            case CRhinoEventWatcher::light_event::light_modified:
            {
                const auto j = that->_lights.find(onLight.m_light_id);
                if (j != that->_lights.end())
                {
                    _convertLight(onLight, vp, j->second);
                }
                break;
            }
            default: break;
            }
        }
	}

    void ChangeQueue::ApplyMaterialChanges(const ON_SimpleArray<const Material*>& rhinoMaterials) const
    {
        auto that = const_cast<ChangeQueue*>(this);
        for (int i = 0; i < rhinoMaterials.Count(); ++i)
        {
            const auto rhinoMaterial = rhinoMaterials[i];
            const auto rdkMaterial = MaterialFromId(rhinoMaterial->MaterialId());
            const std::wstring instanceName = rdkMaterial->InstanceName();
            const auto l = that->_materials.find(instanceName);
            if (l != _materials.end())
            {
                _convertMaterial(rdkMaterial, l->second);
            }
            else
            {
                ospray::cpp::Material material(_rendererName, "obj");
                _convertMaterial(rdkMaterial, material);
                that->_materials[instanceName] = material;
            }
        }
    }

	void ChangeQueue::ApplyEnvironmentChanges(IRhRdkCurrentEnvironment::Usage) const
	{
        auto that = const_cast<ChangeQueue*>(this);
    }

	void ChangeQueue::ApplyGroundPlaneChanges(const GroundPlane& rhinoGroundPlane) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        if (rhinoGroundPlane.Enabled())
        {
            that->_instancesInit = true;

            auto geometry = ospray::cpp::Geometry("plane");
            const float altitude = rhinoGroundPlane.Altitude();
            const std::vector<ospcommon::math::vec4f> coefficients =
            {
                ospcommon::math::vec4f(0.F, 0.F, 1.F, altitude)
            };
            geometry.setParam("plane.coefficients", ospray::cpp::Data(coefficients));
            geometry.commit();

            auto model = ospray::cpp::GeometricModel(geometry);
            auto material = that->_getMaterial(MaterialFromId(rhinoGroundPlane.MaterialId()));
            if (material.handle())
            {
                model.setParam("material", material);
            }
            model.commit();

            ospray::cpp::Group group;
            group.setParam("geometry", ospray::cpp::Data(model));
            group.commit();

            that->_groundPlane = std::make_shared<ospray::cpp::Instance>(group);
            _groundPlane->commit();
        }
        else if (_groundPlane)
        {
            that->_instancesInit = true;
            that->_groundPlane.reset();
        }
    }

    void ChangeQueue::ApplyLinearWorkflowChanges(const IRhRdkLinearWorkflow&) const
    {
        auto that = const_cast<ChangeQueue*>(this);
    }

    void ChangeQueue::ApplyRenderSettingsChanges(const ON_3dmRenderSettings&) const
    {
        auto that = const_cast<ChangeQueue*>(this);
    }

    void ChangeQueue::ApplyClippingPlaneChanges(
        const ON_SimpleArray<const UUID*>& deleted,
        const ON_SimpleArray<const ClippingPlane*>& addedOrChanged) const
    {
        auto that = const_cast<ChangeQueue*>(this);
    }

    void ChangeQueue::ApplyDynamicClippingPlaneChanges(const ON_SimpleArray<const ClippingPlane*>&) const
    {
        auto that = const_cast<ChangeQueue*>(this);
    }

    eRhRdkBakingFunctions ChangeQueue::BakeFor() const
    {
        return eRhRdkBakingFunctions::kAll;
    }

    bool ChangeQueue::ProvideOriginalObject() const
    {
        return false;
    }

    void ChangeQueue::_convertLight(const ON_Light& onLight, const ON_Viewport& vp, ospray::cpp::Light& out)
    {
        if (onLight.IsPointLight())
        {
            const auto cs = onLight.CoordinateSystem();
            ON_Xform onXform;
            onLight.GetLightXform(vp, ON::world_cs, onXform);
            const auto& onLocation = onLight.Location();
            const ospcommon::math::vec3f pos = fromRhino(onXform * onLocation);
            out.setParam("position", pos);
            out.setParam("radius", 0.F);
        }
        else if (onLight.IsDirectionalLight())
        {
            const auto cs = onLight.CoordinateSystem();
            ON_Xform onXform;
            onLight.GetLightXform(vp, ON::world_cs, onXform);
            out.setParam("direction", fromRhino(onXform * onLight.Direction()));
        }
        else if (onLight.IsSpotLight())
        {
            const auto cs = onLight.CoordinateSystem();
            ON_Xform onXform;
            onLight.GetLightXform(vp, ON::world_cs, onXform);
            const auto& onLocation = onLight.Location();
            const auto& onDirection = onLight.Direction();
            const ospcommon::math::vec3f pos = fromRhino(onXform * onLocation);
            const ospcommon::math::vec3f dir = fromRhino(onXform * onDirection);
            out.setParam("position", pos);
            out.setParam("direction", dir);

            const float angle = onLight.SpotAngleDegrees();
            out.setParam("openingAngle", angle * 2.F);
            out.setParam("penumbraAngle", 0.F);
            out.setParam("radius", 0.F);
        }
        else if (onLight.IsLinearLight())
        {

        }
        else if (onLight.IsRectangularLight())
        {

        }

        out.setParam("color", ospcommon::math::vec3f(fromRhino(onLight.Diffuse())));
        out.setParam("intensity", static_cast<float>(onLight.Intensity()) * (onLight.m_bOn ? 1.F : 0.F) * lightIntensityMul);
        out.commit();
    }

    void ChangeQueue::_convertMaterial(const CRhRdkMaterial* rdkMaterial, ospray::cpp::Material& out)
    {
        auto onMaterial = rdkMaterial->SimulatedMaterial();
        out.setParam("kd", ospcommon::math::vec3f(fromRhino(onMaterial.Diffuse())));
        out.setParam("ns", static_cast<float>(onMaterial.Shine() / ON_Material::MaxShine) * 100.F);
        out.setParam("d",  static_cast<float>(1.0 - onMaterial.Transparency()));
        out.commit();
    }

    ospray::cpp::Material ChangeQueue::_getMaterial(const CRhRdkMaterial* rdkMaterial)
    {
        ospray::cpp::Material out;
        const std::wstring instanceName = rdkMaterial->InstanceName();
        const auto l = _materials.find(instanceName);
        if (l != _materials.end())
        {
            out = l->second;
        }
        else if (_supportsMaterials)
        {
            out = ospray::cpp::Material(_rendererName, "obj");
            _convertMaterial(rdkMaterial, out);
            _materials[instanceName] = out;
        }
        return out;
    }

} // namespace Osprey
