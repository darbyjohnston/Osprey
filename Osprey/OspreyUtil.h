// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

namespace Osprey
{
	std::string getErrorMessage(OSPError);
	void printError(const std::string&);
	void printMessage(const std::string&);

	void errorFunc(OSPError, const char*);
	void messageFunc(const char*);

    enum class CameraType
    {
        Perspective,
        Orthographic
    };

    struct Data
    {
        //! \todo Add a condition variable.
        std::mutex mutex;
        bool updates;
        std::shared_ptr<ospray::cpp::World> world;
        std::shared_ptr<ospray::cpp::Camera> camera;
        CameraType cameraType;
    };

	ospcommon::math::vec2i fromRhino(const ON_2iSize&);
	ospcommon::math::vec2f fromRhino(const ON_2dPoint&);
	ospcommon::math::vec2f fromRhino(const ON_2dVector&);

	ospcommon::math::vec3f fromRhino(const ON_3dPoint&);
	ospcommon::math::vec3f fromRhino(const ON_3dVector&);

	ospcommon::math::vec4f fromRhino(const ON_Color&);

	ospcommon::math::affine3f fromRhino(const ON_Xform&);

	ON_2iSize toRhino(const ospcommon::math::vec2i&);

} // namespace Osprey

bool operator < (const GUID&, const GUID&);
