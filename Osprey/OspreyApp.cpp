// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#include "stdafx.h"
#include "OspreyApp.h"

BEGIN_MESSAGE_MAP(COspreyApp, CWinApp)
END_MESSAGE_MAP()

COspreyApp theApp;

BOOL COspreyApp::InitInstance()
{
	CWinApp::InitInstance();
	return TRUE;
}

int COspreyApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
