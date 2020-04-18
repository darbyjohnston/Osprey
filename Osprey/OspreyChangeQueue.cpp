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

	eRhRdkBakingFunctions ChangeQueue::BakeFor() const
	{
		return eRhRdkBakingFunctions::kAll;
	}

	bool ChangeQueue::ProvideOriginalObject() const
	{
		return false;
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
            that->_data->camera = std::make_shared<ospray::cpp::Camera>("perspective");

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
            that->_data->camera = std::make_shared<ospray::cpp::Camera>("orthographic");

            _data->camera->setParam("position", cameraPos);
            _data->camera->setParam("direction", cameraDir);
            _data->camera->setParam("up", cameraUp);
            _data->camera->setParam("nearClip", cameraNear);

            const float height = static_cast<float>(vp.FrustumHeight());
            _data->camera->setParam("height", height);

            _data->camera->commit();
        }
	}

	void ChangeQueue::ApplyMeshChanges(
        const ON_SimpleArray<const UUID*>& deleted,
        const ON_SimpleArray<const Mesh*>& addedOrChanged) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        for (int i = 0; i < deleted.Count(); ++i)
        {
            const auto j = _geometry.find(*deleted[i]);
            if (j != _geometry.end())
            {
                that->_geometry.erase(j);
            }
        }
        for (int i = 0; i < addedOrChanged.Count(); ++i)
        {
            const auto rdkMesh = addedOrChanged[i];
            auto& geometryList = that->_geometry[rdkMesh->UuidId()];
            geometryList.clear();
            const auto& rdkMeshes = rdkMesh->Meshes();
            const int rdkMeshesCount = rdkMeshes.Count();
            for (int k = 0; k < rdkMeshesCount; ++k)
            {
                std::shared_ptr<ospray::cpp::Geometry> geometry;
                if (rdkMeshes[k]->FaceCount() > 0)
                {
                    geometry = std::make_shared<ospray::cpp::Geometry>("mesh");
                    _convertMesh(rdkMeshes[k], geometry);
                }
                geometryList.emplace_back(geometry);
            }
        }
	}

	void ChangeQueue::ApplyMeshInstanceChanges(
        const ON_SimpleArray<ON__UINT32>& deleted,
        const ON_SimpleArray<const MeshInstance*>& addedOrChanged) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        for (int i = 0; i < deleted.Count(); ++i)
        {
            const auto j = _instances.find(deleted[i]);
            if (j != _instances.end())
            {
                that->_instances.erase(j);
            }
        }
        for (int i = 0; i < addedOrChanged.Count(); ++i)
        {
            const auto rdkInstance = addedOrChanged[i];
            const auto rdkInstanceID = rdkInstance->InstanceId();
            const auto j = _instances.find(rdkInstanceID);
            if (j != _instances.end())
            {
                j->second->setParam("xfm", fromRhino(rdkInstance->InstanceXform()));
                j->second->commit();
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
                        const auto& geometry = k->second[rdkMeshIndex];
                        if (geometry)
                        {
                            ospray::cpp::GeometricModel model(*geometry);
                            if (auto material = that->_getMaterial(MaterialFromId(rdkInstance->MaterialId())))
                            {
                                model.setParam("material", *material);
                            }
                            model.commit();

                            ospray::cpp::Group group;
                            group.setParam("geometry", ospray::cpp::Data(model));
                            group.commit();

                            auto instance = std::make_shared<ospray::cpp::Instance>(group);
                            instance->setParam("xfm", fromRhino(rdkInstance->InstanceXform()));
                            instance->commit();
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
            that->_sun = std::make_shared<ospray::cpp::Light>("distant");

            _sun->setParam("direction", fromRhino(rhinoSun.Direction()));
            _sun->setParam("angularDiameter", sunAngularDiameter);

            _sun->setParam("color", static_cast<float>(rhinoSun.Diffuse()));
            _sun->setParam("intensity", static_cast<float>(rhinoSun.Intensity()) * lightIntensityMul);

            _sun->commit();
        }
        else
        {
            that->_sun.reset();
        }
	}

	void ChangeQueue::ApplySkylightChanges(const Skylight& rhinoSkylight) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        if (rhinoSkylight.On())
        {
            that->_ambient = std::make_shared<ospray::cpp::Light>("ambient");

            const float shadowIntensity = rhinoSkylight.ShadowIntensity();
            _ambient->setParam("intensity", ambientIntensity);

            _ambient->commit();
        }
        else
        {
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
                if (onLight.IsEnabled())
                {
                    std::shared_ptr<ospray::cpp::Light> light;
                    if (onLight.IsPointLight())
                    {
                        light = std::make_shared<ospray::cpp::Light>("sphere");
                    }
                    else if (onLight.IsDirectionalLight())
                    {
                        light = std::make_shared<ospray::cpp::Light>("distant");
                    }
                    else if (onLight.IsSpotLight())
                    {
                        light = std::make_shared<ospray::cpp::Light>("spot");
                    }
                    else if (onLight.IsLinearLight())
                    {

                    }
                    else if (onLight.IsRectangularLight())
                    {

                    }
                    if (light)
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
            const std::string instanceName = ON_String(rdkMaterial->InstanceName());
            const auto l = _materials.find(instanceName);
            if (l != _materials.end())
            {
                _convertMaterial(rdkMaterial, l->second);
            }
            else
            {
                auto material = std::make_shared<ospray::cpp::Material>(_rendererName, "obj");
                _convertMaterial(rdkMaterial, material);
                that->_materials[instanceName] = material;
            }
        }
    }

	void ChangeQueue::ApplyEnvironmentChanges(IRhRdkCurrentEnvironment::Usage) const
	{
	}

	void ChangeQueue::ApplyGroundPlaneChanges(const GroundPlane& rhinoGroundPlane) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        if (rhinoGroundPlane.Enabled())
        {
            ospray::cpp::Geometry geom("plane");

            const float altitude = rhinoGroundPlane.Altitude();
            const std::vector<ospcommon::math::vec4f> coefficients =
            {
                ospcommon::math::vec4f(0.F, 0.F, 1.F, altitude)
            };
            geom.setParam("plane.coefficients", ospray::cpp::Data(coefficients));
            geom.commit();

            ospray::cpp::GeometricModel model(geom);
            if (auto material = that->_getMaterial(MaterialFromId(rhinoGroundPlane.MaterialId())))
            {
                model.setParam("material", *material);
            }
            model.commit();

            ospray::cpp::Group group;
            group.setParam("geometry", ospray::cpp::Data(model));
            group.commit();

            that->_groundPlane = std::make_shared<ospray::cpp::Instance>(group);
            _groundPlane->commit();
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
    {}

    void ChangeQueue::Flush(bool bApplyChanges)
    {
        RhRdk::Realtime::ChangeQueue::Flush(bApplyChanges);

        std::vector<ospray::cpp::Instance> instances;
        for (const auto& i : _instances)
        {
            instances.push_back(*i.second);
        }
        if (_groundPlane)
        {
            instances.push_back(*_groundPlane);
        }
        if (instances.size())
        {
            _data->world->setParam("instance", ospray::cpp::Data(instances));
        }

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
            lights.push_back(*i.second);
        }
        if (lights.size())
        {
            _data->world->setParam("light", ospray::cpp::Data(lights));
        }

        _data->world->commit();
    }

    void ChangeQueue::_convertMesh(const ON_Mesh* onMesh, const std::shared_ptr<ospray::cpp::Geometry>& out)
    {
        // Convert the mesh vertices. This assumes that the ON and ospcommon data types
        // have the same memory layout...
        if (onMesh->m_V.Count() > 0)
        {
            out->setParam(
                "vertex.position",
                ospray::cpp::Data(onMesh->m_V.Count(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_V.First())));
        }
        if (onMesh->m_N.Count() > 0)
        {
            out->setParam(
                "vertex.normal",
                ospray::cpp::Data(onMesh->m_N.Count(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_N.First())));
        }
        if (onMesh->m_T.Count() > 0)
        {
            out->setParam(
                "vertex.texcoord",
                ospray::cpp::Data(onMesh->m_T.Count(), reinterpret_cast<const ospcommon::math::vec2f*>(onMesh->m_T.First())));
        }
        if (onMesh->m_C.Count() > 0)
        {
            out->setParam(
                "vertex.color",
                ospray::cpp::Data(onMesh->m_C.Count(), reinterpret_cast<const ospcommon::math::vec4f*>(onMesh->m_C.First())));
        }

        // Convert the mesh indices.
        std::vector<ospcommon::math::vec3ui> indices;
        const int faceCount = onMesh->FaceCount();
        for (int i = 0; i < faceCount; ++i)
        {
            const ON_MeshFace& f = onMesh->m_F[i];
            if (f.IsQuad())
            {
                indices.emplace_back(ospcommon::math::vec3ui(f.vi[0], f.vi[1], f.vi[2]));
                indices.emplace_back(ospcommon::math::vec3ui(f.vi[2], f.vi[3], f.vi[0]));
            }
            else
            {
                indices.emplace_back(ospcommon::math::vec3ui(f.vi[0], f.vi[1], f.vi[2]));
            }
        }
        if (indices.size())
        {
            out->setParam("index", ospray::cpp::Data(indices));
        }

        out->commit();
    }

    void ChangeQueue::_convertLight(const ON_Light& onLight, const ON_Viewport& vp, const std::shared_ptr<ospray::cpp::Light>& out)
    {
        if (onLight.IsPointLight())
        {
            const auto cs = onLight.CoordinateSystem();
            ON_Xform onXform;
            onLight.GetLightXform(vp, ON::world_cs, onXform);
            const auto& onLocation = onLight.Location();
            const ospcommon::math::vec3f pos = fromRhino(onXform * onLocation);
            out->setParam("position", pos);

            out->setParam("radius", 0.F);
        }
        else if (onLight.IsDirectionalLight())
        {
            const auto cs = onLight.CoordinateSystem();
            ON_Xform onXform;
            onLight.GetLightXform(vp, ON::world_cs, onXform);
            out->setParam("direction", fromRhino(onXform * onLight.Direction()));
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
            out->setParam("position", pos);
            out->setParam("direction", dir);

            const float angle = onLight.SpotAngleDegrees();
            out->setParam("openingAngle", angle * 2.F);
            out->setParam("penumbraAngle", 0.F);
            out->setParam("radius", 0.F);
        }
        else if (onLight.IsLinearLight())
        {

        }
        else if (onLight.IsRectangularLight())
        {

        }

        out->setParam("color", fromRhino(onLight.Diffuse()));
        out->setParam("intensity", static_cast<float>(onLight.Intensity()) * lightIntensityMul);

        out->commit();
    }

    void ChangeQueue::_convertMaterial(const CRhRdkMaterial* rdkMaterial, const std::shared_ptr<ospray::cpp::Material>& out)
    {
        auto onMaterial = rdkMaterial->SimulatedMaterial();
        const ospcommon::math::vec3f kd = fromRhino(onMaterial.Diffuse());
        out->setParam("kd", kd);

        //const ospcommon::math::vec3f ks = fromRhino(onMaterial.Specular());
        //out.setParam("ks", ks);

        out->setParam("ns", static_cast<float>(onMaterial.Shine() / ON_Material::MaxShine) * 100.F);
        out->setParam("d", static_cast<float>(1.0 - onMaterial.Transparency()));

        out->commit();
    }

    std::shared_ptr<ospray::cpp::Material> ChangeQueue::_getMaterial(const CRhRdkMaterial* rdkMaterial)
    {
        std::shared_ptr<ospray::cpp::Material> out;
        const std::string instanceName = ON_String(rdkMaterial->InstanceName());
        const auto l = _materials.find(instanceName);
        if (l != _materials.end())
        {
            out = l->second;
        }
        else if (_supportsMaterials)
        {
            out = std::make_shared<ospray::cpp::Material>(_rendererName, "obj");
            _convertMaterial(rdkMaterial, out);
            _materials[instanceName] = out;
        }
        return out;
    }

} // namespace Osprey
