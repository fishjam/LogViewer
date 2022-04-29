// LogViewer.h : main header file for the LogViewer application
//
#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

// CLogViewerApp:
// See LogViewer.cpp for the implementation of this class
//
#include "LogViewerDoc.h"

class CLogViewerApp : public CWinApp
{
public:
    CLogViewerApp();


// Overrides
public:
    virtual BOOL InitInstance();

// Implementation
    //afx_msg void OnFileOpen();
    afx_msg void OnAppAbout();
    DECLARE_MESSAGE_MAP()
};

extern CLogViewerApp theApp;