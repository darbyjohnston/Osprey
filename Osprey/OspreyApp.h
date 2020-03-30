// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"

class COspreyApp : public CWinApp
{
public:
	COspreyApp() = default;

	BOOL InitInstance() override;
	int ExitInstance() override;
  
	DECLARE_MESSAGE_MAP()
};
