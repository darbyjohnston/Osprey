// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyUtil.h"

namespace Osprey
{
	std::string getErrorMessage(OSPError value)
	{
		const std::vector<std::string> messages =
		{
			"None",
			"Unknown",
			"Invalid argument",
			"Invalid operation",
			"Out of memory",
			"Unsupported CPU",
			"Version mismatch"
		};
		return messages[value];
	}

	void printError(const std::string& value)
	{
		RhinoApp().Print("Osprey ERROR: %s\n", value.c_str());
	}

	void printMessage(const std::string& value)
	{
		RhinoApp().Print("Osprey: %s\n", value.c_str());
	}

	void errorFunc(OSPError ospError, const char* buf)
	{
		std::stringstream ss;
		ss << getErrorMessage(ospError) << ": " << buf;
		printError(ss.str());
	}

	void messageFunc(const char* buf)
	{
		printMessage(buf);
	}

	ospcommon::math::vec2i fromRhino(const ON_2iSize& value)
	{
		return ospcommon::math::vec2i{ value.cx, value.cy };
	}

	ospcommon::math::vec2f fromRhino(const ON_2dPoint& value)
	{
		return ospcommon::math::vec2f{
			static_cast<float>(value.x),
			static_cast<float>(value.y) };
	}

	ospcommon::math::vec2f fromRhino(const ON_2dVector& value)
	{
		return ospcommon::math::vec2f{
			static_cast<float>(value.x),
			static_cast<float>(value.y) };
	}

	ospcommon::math::vec3f fromRhino(const ON_3dPoint& value)
	{
		return ospcommon::math::vec3f{
			static_cast<float>(value.x),
			static_cast<float>(value.y),
			static_cast<float>(value.z) };
	}

	ospcommon::math::vec3f fromRhino(const ON_3dVector& value)
	{
		return ospcommon::math::vec3f{
			static_cast<float>(value.x),
			static_cast<float>(value.y),
			static_cast<float>(value.z) };
	}

	ospcommon::math::vec4f fromRhino(const ON_Color& value)
	{
		return ospcommon::math::vec4f{
			static_cast<float>(value.Red() / 255.F),
			static_cast<float>(value.Green() / 255.F),
			static_cast<float>(value.Blue() / 255.F),
			static_cast<float>(value.Alpha() / 255.F) };
	}

	ospcommon::math::affine3f fromRhino(const ON_Xform& value)
	{
		ON_3dVector translation;
		ON_Xform linear;
		value.DecomposeAffine(translation, linear);
		return ospcommon::math::affine3f(
			ospcommon::math::vec3f{
				static_cast<float>(linear[0][0]),
				static_cast<float>(linear[1][0]),
				static_cast<float>(linear[2][0]) },
			ospcommon::math::vec3f{
				static_cast<float>(linear[0][1]),
				static_cast<float>(linear[1][1]),
				static_cast<float>(linear[2][1]) },
			ospcommon::math::vec3f{
				static_cast<float>(linear[0][2]),
				static_cast<float>(linear[1][2]),
				static_cast<float>(linear[2][2]) },
			fromRhino(translation));
	}

	ON_2iSize toRhino(const ospcommon::math::vec2i& value)
	{
		return ON_2iSize(value.x, value.y);
	}

} // namespace Osprey

bool operator < (const GUID& a, const GUID& b)
{
	if (a.Data1 != b.Data1)
	{
		return a.Data1 < b.Data1;
	}
	if (a.Data2 != b.Data2)
	{
		return a.Data2 < b.Data2;
	}
	if (a.Data3 != b.Data3)
	{
		return a.Data3 < b.Data3;
	}
	for (int i = 0; i < 8; i++)
	{
		if (a.Data4[i] != b.Data4[i])
		{
			return a.Data4[i] < b.Data4[i];
		}
	}
	return false;
}
