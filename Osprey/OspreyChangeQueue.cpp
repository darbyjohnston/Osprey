#include "stdafx.h"
#include "OspreyChangeQueue.h"
#include "OspreyUtil.h"

namespace Osprey
{
    namespace
    {
        // Angular diameter of the sun.
        const float sunAngularDiameter = .53F;

        // Ground plane scale factor.
        const float groundPlaneScale = 10.F;

        // Curve radius.
        const float curveRadius = .05F;

        // Light intensity multiplier.
        const float lightIntensityMul = 3.F;

        // Ambient light intensity.
        const float ambientIntensity = .1F;

    } // namespace

	ChangeQueue::ChangeQueue(const CRhinoDoc& rhinoDoc, const ON_3dmView& onView) :
		RhRdk::Realtime::ChangeQueue(rhinoDoc, ON_nil_uuid, onView, nullptr, false, false)
	{}

	const ospray::cpp::World& ChangeQueue::getWorld() const
	{
		return _world;
	}

	const ospray::cpp::Camera& ChangeQueue::getCamera() const
	{
		return _camera;
	}

    void ChangeQueue::setWorldBBox(const ON_BoundingBox& value)
    {
        _worldBBox = value;
    }

    void ChangeQueue::setRendererName(const std::string& value)
    {
        _rendererName = value;
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
            that->_camera = ospray::cpp::Camera("perspective");

            _camera.setParam("position", cameraPos);
            _camera.setParam("direction", cameraDir);
            _camera.setParam("up", cameraUp);
            _camera.setParam("nearClip", cameraNear);

            double halfDiagonalAngle = 0.0;
            double halfVerticalAngle = 0.0;
            double halfHorizontalAngle = 0.0;
            vp.GetCameraAngle(&halfDiagonalAngle, &halfVerticalAngle, &halfHorizontalAngle);
            const float cameraFovY = static_cast<float>(halfVerticalAngle * 2.0 / double(ospcommon::math::two_pi) * 360.0);
            _camera.setParam("fovy", cameraFovY);

            _camera.commit();
        }
        else if (vp.IsParallelProjection())
        {
            that->_camera = ospray::cpp::Camera("orthographic");

            _camera.setParam("position", cameraPos);
            _camera.setParam("direction", cameraDir);
            _camera.setParam("up", cameraUp);
            _camera.setParam("nearClip", cameraNear);

            const float height = static_cast<float>(vp.FrustumHeight());
            _camera.setParam("height", height);

            _camera.commit();
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
                ospray::cpp::Geometry geometry;
                if (rdkMeshes[k]->FaceCount() > 0)
                {
                    geometry = ospray::cpp::Geometry("mesh");
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
                j->second.setParam("xfm", fromRhino(rdkInstance->InstanceXform()));
                j->second.commit();
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
                            ospray::cpp::GeometricModel model(geometry);
                            model.setParam("material", that->_getMaterial(rdkInstance->MaterialId()));
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
            ospray::cpp::Light light("distant");

            light.setParam("direction", fromRhino(rhinoSun.Direction()));
            light.setParam("angularDiameter", sunAngularDiameter);

            light.setParam("color", static_cast<float>(rhinoSun.Diffuse()));
            light.setParam("intensity", static_cast<float>(rhinoSun.Intensity()) * lightIntensityMul);

            light.commit();
            that->_lights.emplace_back(light);
        }
	}

	void ChangeQueue::ApplySkylightChanges(const Skylight& rhinoSkylight) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        if (rhinoSkylight.On())
        {
            ospray::cpp::Light light("ambient");
            const float shadowIntensity = rhinoSkylight.ShadowIntensity();
            light.setParam("intensity", ambientIntensity);
            light.commit();
            that->_lights.emplace_back(light);
        }
	}

	void ChangeQueue::ApplyLightChanges(const ON_SimpleArray<const Light*>& onLights) const
	{
        auto that = const_cast<ChangeQueue*>(this);
        const auto& vp = QueueView()->m_vp;
        for (int i = 0; i < onLights.Count(); ++i)
        {
            const auto& onLight = onLights[i]->LightData();
            if (onLight.IsEnabled())
            {
                ospray::cpp::Light light;
                if (onLight.IsPointLight())
                {
                    light = ospray::cpp::Light("sphere");

                    const auto cs = onLight.CoordinateSystem();
                    ON_Xform onXform;
                    onLight.GetLightXform(vp, ON::world_cs, onXform);
                    const auto& onLocation = onLight.Location();
                    const ospcommon::math::vec3f pos = fromRhino(onXform * onLocation);
                    light.setParam("position", pos);

                    light.setParam("radius", 0.F);
                }
                else if (onLight.IsDirectionalLight())
                {
                    light = ospray::cpp::Light("distant");

                    const auto cs = onLight.CoordinateSystem();
                    ON_Xform onXform;
                    onLight.GetLightXform(vp, ON::world_cs, onXform);
                    light.setParam("direction", fromRhino(onXform * onLight.Direction()));
                }
                else if (onLight.IsSpotLight())
                {
                    light = ospray::cpp::Light("spot");

                    const auto cs = onLight.CoordinateSystem();
                    ON_Xform onXform;
                    onLight.GetLightXform(vp, ON::world_cs, onXform);
                    const auto& onLocation = onLight.Location();
                    const auto& onDirection = onLight.Direction();
                    const ospcommon::math::vec3f pos = fromRhino(onXform * onLocation);
                    const ospcommon::math::vec3f dir = fromRhino(onXform * onDirection);
                    light.setParam("position", pos);
                    light.setParam("direction", dir);

                    const float angle = onLight.SpotAngleDegrees();
                    light.setParam("openingAngle", angle * 2.F);
                    light.setParam("penumbraAngle", 0.F);
                    light.setParam("radius", 0.F);
                }
                else if (onLight.IsLinearLight())
                {

                }
                else if (onLight.IsRectangularLight())
                {

                }
                if (light)
                {
                    light.setParam("color", fromRhino(onLight.Diffuse()));
                    light.setParam("intensity", static_cast<float>(onLight.Intensity()) * lightIntensityMul);

                    light.commit();
                    that->_lights.emplace_back(light);
                }
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
            ospray::cpp::Geometry mesh("mesh");

            const ON_3dPoint& onCenter = _worldBBox.Center();
            const float size = static_cast<float>(_worldBBox.Diagonal().Length()) * groundPlaneScale;
            const ospcommon::math::vec2f min =
            {
                static_cast<float>(onCenter.x) - size,
                static_cast<float>(onCenter.y) - size
            };
            const ospcommon::math::vec2f max =
            {
                static_cast<float>(onCenter.x) + size,
                static_cast<float>(onCenter.y) + size
            };
            const float altitude = rhinoGroundPlane.Altitude();
            const std::vector<ospcommon::math::vec3f> v =
            {
                ospcommon::math::vec3f{ min.x, min.y, altitude },
                ospcommon::math::vec3f{ max.x, min.y, altitude },
                ospcommon::math::vec3f{ max.x, max.y, altitude },
                ospcommon::math::vec3f{ min.x, max.y, altitude }
            };
            mesh.setParam("vertex.position", ospray::cpp::Data(v));

            const std::vector<ospcommon::math::vec3f> n =
            {
                ospcommon::math::vec3f{ 0.F, 0.F, 1.F },
                ospcommon::math::vec3f{ 0.F, 0.F, 1.F },
                ospcommon::math::vec3f{ 0.F, 0.F, 1.F },
                ospcommon::math::vec3f{ 0.F, 0.F, 1.F }
            };
            mesh.setParam("vertex.normal", ospray::cpp::Data(n));

            const std::vector<ospcommon::math::vec3ui> indices =
            {
                ospcommon::math::vec3ui(0, 1, 2),
                ospcommon::math::vec3ui(2, 3, 0)
            };
            mesh.setParam("index", ospray::cpp::Data(indices));

            mesh.commit();

            ospray::cpp::GeometricModel model(mesh);
            model.setParam("material", that->_getMaterial(rhinoGroundPlane.MaterialId()));
            model.commit();

            ospray::cpp::Group group;
            group.setParam("geometry", ospray::cpp::Data(model));
            group.commit();

            that->_groundPlane = ospray::cpp::Instance(group);
            _groundPlane.commit();
        }
    }

    void ChangeQueue::NotifyBeginUpdates() const
    {}

    void ChangeQueue::NotifyEndUpdates() const
    {}

    void ChangeQueue::Flush(bool bApplyChanges)
    {
        RhRdk::Realtime::ChangeQueue::Flush(bApplyChanges);
        auto that = const_cast<ChangeQueue*>(this);

        std::vector<ospray::cpp::Instance> instances;
        for (const auto& i : _instances)
        {
            instances.push_back(i.second);
        }
        if (_groundPlane)
        {
            instances.push_back(_groundPlane);
        }

        that->_world = ospray::cpp::World();
        if (instances.size())
        {
            that->_world.setParam("instance", ospray::cpp::Data(instances));
        }
        if (_lights.size())
        {
            that->_world.setParam("light", ospray::cpp::Data(_lights));
        }
        that->_world.commit();
    }

    void ChangeQueue::_convertMesh(const ON_Mesh* onMesh, ospray::cpp::Geometry& out)
    {
        // Convert the mesh vertices. This assumes that the ON and ospcommon data types
        // have the same memory layout...
        if (onMesh->m_V.Count() > 0)
        {
            out.setParam(
                "vertex.position",
                ospray::cpp::Data(onMesh->m_V.Count(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_V.First())));
        }
        if (onMesh->m_N.Count() > 0)
        {
            out.setParam(
                "vertex.normal",
                ospray::cpp::Data(onMesh->m_N.Count(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_N.First())));
        }
        if (onMesh->m_T.Count() > 0)
        {
            out.setParam(
                "vertex.texccord",
                ospray::cpp::Data(onMesh->m_T.Count(), reinterpret_cast<const ospcommon::math::vec2f*>(onMesh->m_T.First())));
        }
        if (onMesh->m_C.Count() > 0)
        {
            out.setParam(
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
            out.setParam("index", ospray::cpp::Data(indices));
        }

        out.commit();
    }

    ospray::cpp::Material ChangeQueue::_getMaterial(ON__UINT32 value)
    {
        ospray::cpp::Material out;
        const auto rdkMaterial = MaterialFromId(value);
        const auto l = _materials.find(rdkMaterial);
        if (l != _materials.end())
        {
            out = l->second;
        }
        else
        {
            auto onMaterial = rdkMaterial->SimulatedMaterial();
            out = ospray::cpp::Material(_rendererName, "obj");

            const ospcommon::math::vec3f kd = fromRhino(onMaterial.Diffuse());
            out.setParam("kd", kd);
            //const ospcommon::math::vec3f ks = fromRhino(onMaterial.Specular());
            //out.setParam("ks", ks);
            out.setParam("ns", static_cast<float>(onMaterial.Shine() / ON_Material::MaxShine) * 100.F);
            out.setParam("d", static_cast<float>(1.0 - onMaterial.Transparency()));

            out.commit();
            _materials[rdkMaterial] = out;
        }
        return out;
    }

} // namespace Osprey
