// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyRender.h"

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

        // Background color.
        const ospcommon::math::vec3f backgroundColor(65 / 255.F, 100 / 255.F, 170 / 255.F);

	} // namespace

	Render::Render()
	{}

	std::shared_ptr<Render> Render::create()
	{
		return std::shared_ptr<Render>(new Render);
	}

	const std::string& Render::getRendererName() const
	{
		return _rendererName;
	}

	size_t Render::getPasses() const
	{
		return _passes;
	}

	size_t Render::getPixelSamples() const
	{
		return _pixelSamples;
	}

	size_t Render::getAOSamples() const
	{
		return _aoSamples;
	}

	bool Render::isDenoiserFound() const
	{
		return _denoiserFound;
	}

	bool Render::isDenoiserEnabled() const
	{
		return _denoiserEnabled;
	}

	void Render::setRendererName(const std::string& value)
	{
		_rendererName = value;
	}

	void Render::setPasses(size_t value)
	{
		_passes = value;
	}

	void Render::setPixelSamples(size_t value)
	{
		_pixelSamples = value;
	}

	void Render::setAOSamples(size_t value)
	{
		_aoSamples = value;
	}

	void Render::setDenoiserFound(bool value)
	{
		_denoiserFound = value;
	}

	void Render::setDenoiserEnabled(bool value)
	{
		_denoiserEnabled = value;
	}

	void Render::setRenderSize(
		const ospcommon::math::vec2i& window,
		const ospcommon::math::box2i& rectangle)
	{
		_windowSize = window;
		_renderRect = rectangle;
	}

	void Render::convert(CRhinoDoc* rhinoDoc)
	{
		_convertMaterials(rhinoDoc);
		_convertObjects(rhinoDoc);
		_convertGroundPlane(rhinoDoc);
		if (_instances.size())
		{
			_world.setParam("instance", ospray::cpp::Data(_instances, true));
		}
		_convertLights(rhinoDoc);
		_world.commit();
        _convertEnvironment(rhinoDoc);
		_convertCamera(rhinoDoc);
	}

	void Render::initRender(IRhRdkRenderWindow& rhinoRenderWindow)
	{
		const ospcommon::math::vec2i& renderSize = _renderRect.size();
		rhinoRenderWindow.SetSize(Osprey::toRhino(renderSize));

		// Create the renderer.
		_renderer = ospray::cpp::Renderer(_rendererName);
		_renderer.setParam("pixelSamples", static_cast<int>(_pixelSamples));
		_renderer.setParam("aoSamples", static_cast<int>(_aoSamples));
		_renderer.setParam("backgroundColor", backgroundColor);
		if (_materials.size())
		{
			_renderer.setParam("material", ospray::cpp::Data(_materials));
		}
		_renderer.commit();

		// Create the frame buffer.
		_frameBuffer = ospray::cpp::FrameBuffer(
			renderSize,
			OSP_FB_RGBA32F,
			OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM | OSP_FB_VARIANCE | OSP_FB_NORMAL | OSP_FB_ALBEDO);
		std::vector<ospray::cpp::ImageOperation> imageOps;
		if (0)
		{
			// Add the tone mapper operation.
			ospray::cpp::ImageOperation tonemapper("tonemapper");
			//tonemapper.setParam("exposure", .5F);
			tonemapper.commit();
			imageOps.push_back(tonemapper);
		}
		if (_denoiserFound && _denoiserEnabled)
		{
			// Add the denoiser operation.
			ospray::cpp::ImageOperation denoiser("denoiser");
			denoiser.commit();
			imageOps.push_back(denoiser);
		}
		if (imageOps.size())
		{
			_frameBuffer.setParam("imageOperation", ospray::cpp::Data(imageOps));
		}
		_frameBuffer.commit();
		_frameBuffer.clear();
	}

	void Render::renderPass(size_t pass, IRhRdkRenderWindow& rhinoRenderWindow)
	{
		// Show progress message.
		ON_wString s = ON_wString::FormatToString(L"Rendering pass %d...", pass + 1);
		rhinoRenderWindow.SetProgress(s, static_cast<int>(pass / static_cast<float>(_passes) * 100));

		// Render a frame.
		_frameBuffer.renderFrame(_renderer, _camera, _world);

		// Copy the RGBA channels to Rhino.
		const ospcommon::math::vec2i& renderSize = _renderRect.size();
		IRhRdkRenderWindow::IChannel* pChanRGBA = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanRGBA);
		if (pChanRGBA)
		{
			void* fb = _frameBuffer.map(OSP_FB_COLOR);
			float* fbP = reinterpret_cast<float*>(fb);
			for (int y = 0; y < renderSize.y; ++y)
			{
				for (int x = 0; x < renderSize.x; ++x)
				{
					pChanRGBA->SetValue(x, renderSize.y - 1 - y, ComponentOrder::RGBA, fbP);
					fbP += 4;
				}
			}
			_frameBuffer.unmap(fb);
			pChanRGBA->Close();
		}

		// Copy the depth channel to Rhino.
		IRhRdkRenderWindow::IChannel* pChanDepth = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanDistanceFromCamera);
		if (pChanDepth)
		{
			void* fb = _frameBuffer.map(OSP_FB_DEPTH);
			float* fbP = reinterpret_cast<float*>(fb);
			for (int y = 0; y < renderSize.y; ++y)
			{
				for (int x = 0; x < renderSize.x; ++x)
				{
					pChanDepth->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
				}
			}
			_frameBuffer.unmap(fb);
			pChanDepth->Close();
		}

		// Copy the normal channels to Rhino.
		IRhRdkRenderWindow::IChannel* pChanNormalX = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalX);
		IRhRdkRenderWindow::IChannel* pChanNormalY = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalY);
		IRhRdkRenderWindow::IChannel* pChanNormalZ = rhinoRenderWindow.OpenChannel(IRhRdkRenderWindow::chanNormalZ);
		if (pChanNormalX && pChanNormalY && pChanNormalZ)
		{
			void* fb = _frameBuffer.map(OSP_FB_NORMAL);
			float* fbP = reinterpret_cast<float*>(fb);
			for (int y = 0; y < renderSize.y; ++y)
			{
				for (int x = 0; x < renderSize.x; ++x)
				{
					pChanNormalX->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
					pChanNormalY->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
					pChanNormalZ->SetValue(x, renderSize.y - 1 - y, ComponentOrder::Irrelevant, fbP);
					++fbP;
				}
			}
			_frameBuffer.unmap(fb);
		}
		if (pChanNormalX || pChanNormalY || pChanNormalZ)
		{
			pChanNormalX->Close();
			pChanNormalY->Close();
			pChanNormalZ->Close();
		}

		rhinoRenderWindow.Invalidate();

        if (pass == _passes - 1)
        {
            rhinoRenderWindow.SetProgress("Render finished.", 100);
        }
    }

	void Render::_convertMaterials(CRhinoDoc* rhinoDoc)
	{
		const auto& rhinoMaterialTable = rhinoDoc->m_material_table;
		for (int i = -1; i < rhinoMaterialTable.MaterialCount(); ++i)
		{
			ospray::cpp::Material material(_rendererName, "obj");

			const auto& rhinoMaterial = rhinoMaterialTable[i];
			const ospcommon::math::vec3f kd = fromRhino(rhinoMaterial.Diffuse());
			material.setParam("kd", kd);
			//const ospcommon::math::vec3f ks = fromRhino(rhinoMaterial.Specular());
			//material.setParam("ks", ks);
			material.setParam("ns", static_cast<float>(rhinoMaterial.Shine() / ON_Material::MaxShine) * 100.F);
			material.setParam("d", static_cast<float>(1.0 - rhinoMaterial.Transparency()));

			material.commit();
			_materials.emplace_back(material);
		}

		// Create materials for the layer colors.
		for (int i = 0; i < rhinoDoc->m_layer_table.LayerCount(); ++i)
		{
			const auto& layer = rhinoDoc->m_layer_table[i];
			if (!layer.IsDeleted())
			{
				ospray::cpp::Material material(_rendererName, "obj");

				const ospcommon::math::vec3f kd = fromRhino(layer.Color());
				material.setParam("kd", kd);

				material.commit();
				const size_t index = _materials.size();
				_materials.emplace_back(material);
				_onLayerToMaterial[layer.Id()] = index - 1;
			}
		}
	}

	void Render::_convertObjects(CRhinoDoc* rhinoDoc)
	{
		ObjectState state;
		state.xform = ospcommon::math::affine3f::scale(ospcommon::math::vec3f(1.F, 1.F, 1.F));
		CRhinoObjectIterator it(CRhinoObjectIterator::normal_objects, CRhinoObjectIterator::active_objects);
		for (CRhinoObject* rhinoObject = it.First(); rhinoObject; rhinoObject = it.Next())
		{
			_convertObject(rhinoDoc, rhinoObject, state);
		}
	}

	void Render::_convertObject(CRhinoDoc* rhinoDoc, const CRhinoObject* rhinoObject, const ObjectState& state)
	{
		if (rhinoObject->IsVisible())
		{
			// Find the material index.
			int materialIndex = -1;
			const auto& attr = rhinoObject->Attributes();
			switch (attr.MaterialSource())
			{
			case ON::material_from_object:
				materialIndex = attr.m_material_index;
				break;
			case ON::material_from_layer:
				materialIndex = rhinoObject->ObjectLayer().m_material_index;
				break;
			case ON::material_from_parent:
				materialIndex = state.materialIndex;
				break;
			default: break;
			}
			if (-1 == materialIndex)
			{
				if (const CRhinoCurveObject* rhinoCurve = CRhinoCurveObject::Cast(rhinoObject))
				{
					//! \todo Temporarily use layer colors for curve materials.
					const auto& layer = rhinoCurve->ObjectLayer();
					const auto i = _onLayerToMaterial.find(layer.Id());
					if (i != _onLayerToMaterial.end())
					{
						materialIndex = static_cast<int>(i->second);
					}
				}
			}

			// Convert the geometry.
			std::vector<size_t> geometries;
			ON_MeshParameters onMeshParameters;
			if (const CRhinoMeshObject* rhinoMesh = CRhinoMeshObject::Cast(rhinoObject))
			{
				if (auto onMesh = rhinoMesh->Mesh())
				{
					if (onMesh->FaceCount() > 0)
					{
						const auto i = _onMeshToGeometry.find(onMesh);
						if (i != _onMeshToGeometry.end())
						{
							geometries.push_back(i->second);
						}
						else
						{
							auto geometry = _convertMesh(onMesh);
							const size_t index = _geometry.size();
							_geometry.emplace_back(geometry);
                            _onMeshToGeometry[onMesh] = index;
							geometries.push_back(index);
						}
					}
				}
			}
			else if (const CRhinoExtrusionObject* rhinoExtrusion = CRhinoExtrusionObject::Cast(rhinoObject))
			{
				//if (0 == rhinoExtrusion->MeshCount(ON::render_mesh))
				//{
				//	rhinoExtrusion->CreateMeshes(ON::render_mesh, onMeshParameters);
				//}
				ON_SimpleArray<const ON_Mesh*> onMeshes;
				rhinoExtrusion->GetMeshes(ON::render_mesh, onMeshes);
				for (int i = 0; i < onMeshes.Count(); ++i)
				{
					if (onMeshes[i]->FaceCount() > 0)
					{
						const auto j = _onMeshToGeometry.find(onMeshes[i]);
						if (j != _onMeshToGeometry.end())
						{
							geometries.push_back(j->second);
						}
						else
						{
							auto geometry = _convertMesh(onMeshes[i]);
							const size_t index = _geometry.size();
							_geometry.emplace_back(geometry);
                            _onMeshToGeometry[onMeshes[i]] = index;
							geometries.push_back(index);
						}
					}
				}
			}
			else if (const CRhinoBrepObject* rhinoBrep = CRhinoBrepObject::Cast(rhinoObject))
			{
				//if (0 == rhinoBrep->MeshCount(ON::render_mesh))
				//{
				//	rhibnoBrep->CreateMeshes(ON::render_mesh, onMeshParameters);
				//}
				ON_SimpleArray<const ON_Mesh*> onMeshes;
				rhinoBrep->GetMeshes(ON::render_mesh, onMeshes);
				for (int i = 0; i < onMeshes.Count(); ++i)
				{
					if (onMeshes[i]->FaceCount() > 0)
					{
						const auto j = _onMeshToGeometry.find(onMeshes[i]);
						if (j != _onMeshToGeometry.end())
						{
							geometries.push_back(j->second);
						}
						else
						{
							auto geometry = _convertMesh(onMeshes[i]);
							const size_t index = _geometry.size();
							_geometry.emplace_back(geometry);
                            _onMeshToGeometry[onMeshes[i]] = index;
							geometries.push_back(index);
						}
					}
				}
			}
            else if (const CRhinoCurveObject* rhinoCurve = CRhinoCurveObject::Cast(rhinoObject))
            {
                if (auto onCurve = rhinoCurve->Curve())
                {
                    ON_SimpleArray<ON_3dPoint> onPoints;
                    ON_SimpleArray<double> onParameters;
                    if (onCurve->IsPolyline(&onPoints, &onParameters) >= 2)
                    {
                        const int onPointsCount = onPoints.Count();
                        ospray::cpp::Geometry curve("curve");

                        std::vector< ospcommon::math::vec3f> v;
                        for (int i = 0; i < onPointsCount; ++i)
                        {
                            v.push_back(fromRhino(onPoints[i]));
                        }
                        curve.setParam("vertex.position", ospray::cpp::Data(v));

                        std::vector<uint32_t> indices(onPointsCount - 1);
                        for (int i = 0; i < onPointsCount - 1; ++i)
                        {
                            indices[i] = i;
                        }
                        curve.setParam("index", ospray::cpp::Data(indices));

                        curve.setParam("radius", curveRadius);
                        curve.setParam("type", static_cast<int>(OSPCurveType::OSP_ROUND));
                        curve.setParam("basis", static_cast<int>(OSPCurveBasis::OSP_LINEAR));

                        curve.commit();

                        const size_t index = _geometry.size();
                        _geometry.emplace_back(curve);
                        geometries.push_back(index);
                    }
                }
            }
			else if (const CRhinoInstanceObject* rhinoInstance = CRhinoInstanceObject::Cast(rhinoObject))
			{
				if (auto idef = rhinoInstance->InstanceDefinition())
				{
					ON_SimpleArray<const CRhinoObject*> rhinoObjects;
					idef->GetObjects(rhinoObjects);
					for (int i = 0; i < rhinoObjects.Count(); ++i)
					{
						ObjectState childState = state;
						childState.xform *= fromRhino(rhinoInstance->InstanceXform());
						if (!childState.instanceDefinition)
						{
							childState.materialIndex = materialIndex;
						}
						childState.instanceDefinition = true;
						_convertObject(rhinoDoc, rhinoObjects[i], childState);
					}
				}
			}

			// Create the model, group, and instance to add the geometry to the world.
			std::vector<ospray::cpp::GeometricModel> models;
			for (const auto i : geometries)
			{
				ospray::cpp::GeometricModel model(_geometry[i]);
				model.setParam("material", _materials[materialIndex + 1]);
				model.commit();
				models.emplace_back(model);
			}
			if (models.size() > 0)
			{
				ospray::cpp::Group group;
				group.setParam("geometry", ospray::cpp::Data(models));
				group.commit();

				ospray::cpp::Instance instance(group);
				instance.setParam("xfm", state.xform);
				instance.commit();
				_instances.emplace_back(instance);
			}
		}
	}

	void Render::_convertGroundPlane(CRhinoDoc* rhinoDoc)
	{
		const auto& rhinoGroundPlane = rhinoDoc->GroundPlane();
		if (rhinoGroundPlane.On())
		{
			ospray::cpp::Geometry mesh("mesh");

			const ON_BoundingBox& const onBBox = rhinoDoc->BoundingBox(false, true, true);
			const ON_3dPoint& onCenter = onBBox.Center();
			const float size = static_cast<float>(onBBox.Diagonal().Length()) * groundPlaneScale;
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

			const auto& rhinoMaterialTable = rhinoDoc->m_material_table;
			ospray::cpp::GeometricModel model(mesh);
			const int materialIndex = rhinoMaterialTable.FindMaterial(rhinoGroundPlane.MaterialInstanceId());
			model.setParam("material", _materials[materialIndex + 1]);
			model.commit();

			ospray::cpp::Group group;
			group.setParam("geometry", ospray::cpp::Data(model));
			group.commit();

			ospray::cpp::Instance instance(group);
			instance.commit();
			_instances.emplace_back(instance);
		}
	}

	void Render::_convertLights(CRhinoDoc* rhinoDoc)
	{
		const ON_Viewport& vp = RhinoApp().ActiveView()->ActiveViewport().VP();

		// Convert the lights.
		std::vector<ospray::cpp::Light> lights;
		const auto& rhinoLightTable = rhinoDoc->m_light_table;
		for (int i = 0; i < rhinoLightTable.LightCount(); ++i)
		{
			const auto& rhinoLight = rhinoLightTable[i];
			if (!rhinoLight.IsDeleted())
			{
				if (auto light = _convertLight(rhinoLight.Light(), vp))
				{
					lights.push_back(light);
				}
			}
		}

		// Convert the sun.
		const auto& rhinoSun = rhinoDoc->Sun();
		if (rhinoSun.EnableOn())
		{
			ospray::cpp::Light light("distant");

			light.setParam("direction", fromRhino(rhinoSun.Vector()));
			light.setParam("angularDiameter", sunAngularDiameter);

			const auto& onLight = rhinoSun.Light();
			light.setParam("color", static_cast<float>(onLight.Diffuse()));
			light.setParam("intensity", static_cast<float>(onLight.Intensity()) * lightIntensityMul);

			light.commit();
			lights.emplace_back(light);
		}

		// Use the default light if there are no other lights.
		if (lights.empty())
		{
			const auto& rhinoLight = rhinoLightTable.DefaultLight();
			if (auto light = _convertLight(rhinoLight.Light(), vp))
			{
				lights.push_back(light);
			}
		}

		// Skylight.
		/*if (auto rhinoEnvironment = rhinoDoc->CurrentEnvironment().GetEnv(
			IRhRdkCurrentEnvironment::Usage::Skylighting,
			IRhRdkCurrentEnvironment::Purpose::Render))
		{

			CRhRdkContent::TextureChannelInfo rhinoTextureChannelInfo;
			rhinoEnvironment->GetTextureChannelInfo(
				CRhRdkEnvironment::ChildSlotUsage::Skylighting,
				rhinoTextureChannelInfo);
			if (auto rhinoBitmap = rhinoDoc->m_bitmap_table.FindBitmap(rhinoTextureChannelInfo.textureInstanceId))
			{
				auto id = rhinoBitmap->Id();
			}
		}*/
		const auto& rhinoSkylight = rhinoDoc->Skylight();
		if (rhinoSkylight.On())
		{
			ospray::cpp::Light light("ambient");
			light.setParam("intensity", ambientIntensity);
			light.commit();
			lights.emplace_back(light);
		}

		// Add the lights to the world.
		if (lights.size() > 0)
		{
			_world.setParam("light", ospray::cpp::Data(lights));
		}
	}

    void Render::_convertEnvironment(CRhinoDoc* rhinoDoc)
    {
        /*if (auto rhinoEnvironment = rhinoDoc->CurrentEnvironment().GetEnv(
            IRhRdkCurrentEnvironment::Usage::Background,
            IRhRdkCurrentEnvironment::Purpose::Render))
        {
            CRhRdkContent::TextureChannelInfo rhinoTextureChannelInfo;
            rhinoEnvironment->GetTextureChannelInfo(
                CRhRdkEnvironment::ChildSlotUsage::Background,
                rhinoTextureChannelInfo);
            if (auto rhinoBitmap = rhinoDoc->m_bitmap_table.FindBitmap(rhinoTextureChannelInfo.textureInstanceId))
            {
                auto id = rhinoBitmap->Id();
            }
        }*/
    }

    void Render::_convertCamera(CRhinoDoc* rhinoDoc)
    {
        const ON_Viewport& vp = RhinoApp().ActiveView()->ActiveViewport().VP();
		ospray::cpp::Camera camera;
		ospcommon::math::vec3f cameraPos = fromRhino(vp.CameraLocation());
		ospcommon::math::vec3f cameraDir = fromRhino(vp.CameraDirection());
		ospcommon::math::vec3f cameraUp = fromRhino(vp.CameraUp());
		const float cameraNear = vp.PerspectiveMinNearDist();
		if (vp.IsPerspectiveProjection())
		{
			_camera = ospray::cpp::Camera("perspective");

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

			_camera.setParam("aspect", _windowSize.x / static_cast<float>(_windowSize.y));
		}
		else if (vp.IsParallelProjection())
		{
			_camera = ospray::cpp::Camera("orthographic");

			_camera.setParam("position", cameraPos);
			_camera.setParam("direction", cameraDir);
			_camera.setParam("up", cameraUp);
			_camera.setParam("nearClip", cameraNear);

			const float height = static_cast<float>(vp.FrustumHeight());
			_camera.setParam("height", height);

			_camera.setParam("aspect", _windowSize.x / static_cast<float>(_windowSize.y));
		}
		if (_camera)
		{
			ospcommon::math::vec2f imageStart(
				_renderRect.lower.x / static_cast<float>(_windowSize.x - 1),
				1.F - (_renderRect.upper.y / static_cast<float>(_windowSize.y - 1)));
			ospcommon::math::vec2f imageEnd(
				_renderRect.upper.x / static_cast<float>(_windowSize.x - 1),
				1.F - (_renderRect.lower.y / static_cast<float>(_windowSize.y - 1)));
			_camera.setParam("imageStart", imageStart);
			_camera.setParam("imageEnd", imageEnd);

			_camera.commit();
		}
	}

	ospray::cpp::Geometry Render::_convertMesh(const ON_Mesh* onMesh)
	{
		ospray::cpp::Geometry mesh("mesh");

		// Convert the mesh vertices. This assumes that the ON and ospcommon data types
		// have the same memory layout...
		if (onMesh->m_V.Count() > 0)
		{
			mesh.setParam(
				"vertex.position",
				ospray::cpp::Data(onMesh->m_V.Count(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_V.First())));
		}
		if (onMesh->m_N.Count() > 0)
		{
			mesh.setParam(
				"vertex.normal",
				ospray::cpp::Data(onMesh->m_N.Count(), reinterpret_cast<const ospcommon::math::vec3f*>(onMesh->m_N.First())));
		}
		if (onMesh->m_T.Count() > 0)
		{
			mesh.setParam(
				"vertex.texccord",
				ospray::cpp::Data(onMesh->m_T.Count(), reinterpret_cast<const ospcommon::math::vec2f*>(onMesh->m_T.First())));
		}
		if (onMesh->m_C.Count() > 0)
		{
			mesh.setParam(
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
		mesh.setParam("index", ospray::cpp::Data(indices));

		mesh.commit();
		return mesh;
	}

	ospray::cpp::Light Render::_convertLight(const ON_Light& onLight, const ON_Viewport& vp)
	{
		ospray::cpp::Light light = nullptr;
		if (onLight.IsEnabled())
		{
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
			}
		}
		return light;
	}

} // namespace Osprey
