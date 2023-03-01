// MainFrm.h : interface of the CMainFrame class
//


#pragma once

#define IDC_SETTING_CONFIG_INI_BEGIN (WM_USER + 0x600)
#define IDC_SETTING_CONFIG_INI_END   (WM_USER + 0x63f)

#define IDC_CODE_PAGE_BEGIN			 ID_CODE_PAGE_UTF8
#define IDC_CODE_PAGE_END            ID_CODE_PAGE_KOREAN

#define MENU_INDEX_VIEW              2
#define MENU_INDEX_CODEPAGE          4

class CMainFrame : public CFrameWnd
    , public FTL::IFileFindCallback
{
    
protected: // create from serialization only
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    //IFileFindCallback
    virtual FileFindResultHandle OnFindFile(LPCTSTR pszFilePath, const WIN32_FIND_DATA& findData, LPVOID pParam);
    virtual FileFindResultHandle OnError(LPCTSTR pszFilePath, DWORD dwError, LPVOID pParam);

// Implementation
public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
    CStatusBar      m_wndStatusBar;
    CToolBar        m_wndToolBar;
    CSplitterWnd    m_wndIdVertSplitter;
    CSplitterWnd    m_wndVertSplitter;
    CSplitterWnd    m_wndHorzSplitter;
    CStringArray    m_iniFiles;
    CMenu           m_menuIni;
    TCHAR           m_szModulePath[MAX_PATH];
// Generated message map functions
protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
public:
    afx_msg void OnUpdateIndicatorFileCount(CCmdUI *pCmdUI);
    afx_msg void OnUpdateIndicatorProcessCount(CCmdUI* pCmdUI);
    afx_msg void OnUpdateIndicatorThreadCount(CCmdUI* pCmdUI);
    afx_msg void OnUpdateIndicatorLogItemCount(CCmdUI *pCmdUI);
    afx_msg void OnDropFiles(HDROP hDropInfo);
    afx_msg void OnDisplayTimeFormatSelected(UINT nID);
    afx_msg void OnSettingConfigIniChange(UINT nID);
    afx_msg void OnCodePageChange(UINT nID);
    afx_msg BOOL OnToolsCheckMissingSeqNumber(UINT nID);
    afx_msg BOOL OnToolsCheckReverseSeqNumber(UINT nID);
    afx_msg void OnToolsStatistics(UINT nID);
};


