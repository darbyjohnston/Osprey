// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#include "OspreyEnum.h"

namespace Osprey
{
	std::string getErrorMessage(OSPError);
	void printError(const std::string&);
	void printMessage(const std::string&);

	void errorFunc(OSPError, const char*);
	void messageFunc(const char*);

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
