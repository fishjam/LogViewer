// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "LogViewer.h"
#include "MachinePidTidTreeView.h"
#include "LogFilterView.h"
#include "LogItemView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SECTION_CONFIG      TEXT("Config")
#define ENTRY_ACTIVE_INI    TEXT("ActiveIni")

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_COUNT, &CMainFrame::OnUpdateIndicatorFileCount)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_THREAD_COUNT, &CMainFrame::OnUpdateIndicatorThreadCount)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LOGITEM_COUNT, &CMainFrame::OnUpdateIndicatorLogItemCount)
    ON_WM_DROPFILES()

    ON_COMMAND_RANGE(IDC_SETTING_CONFIG_INI_BEGIN, IDC_SETTING_CONFIG_INI_END, &CMainFrame::OnSettingConfigIniChange)
    ON_COMMAND_RANGE(IDC_CODE_PAGE_BEGIN, IDC_CODE_PAGE_END, &CMainFrame::OnCodePageChange)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_FILE_COUNT,
    ID_INDICATOR_THREAD_COUNT,
    ID_INDICATOR_LOGITEM_COUNT,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    // TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

FileFindResultHandle CMainFrame::OnFindFile(LPCTSTR pszFilePath, const WIN32_FIND_DATA& findData, LPVOID pParam)
{
    UNREFERENCED_PARAMETER(pszFilePath);
    UNREFERENCED_PARAMETER(pParam);

    m_iniFiles.Add(findData.cFileName);
    return rhContinue;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    BOOL bRet = FALSE;

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
        | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // TODO: Delete these three lines if you don't want the toolbar to be dockable
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);

    CMenu* pMenuCodePage = GetMenu()->GetSubMenu(MENU_INDEX_CODEPAGE);
    if (pMenuCodePage)
    {
        //缺省编码方式是UTF8
        pMenuCodePage->CheckMenuRadioItem(IDC_CODE_PAGE_BEGIN, IDC_CODE_PAGE_END, ID_CODE_PAGE_UTF8, 
            MF_BYCOMMAND|MF_CHECKED);
    }
    //查找当前目录下的所有 .ini 文件，然后放入菜单
    FTL::CFFileFinder finder;
    TCHAR szModulePath[MAX_PATH] = {0};
    finder.SetCallback(this, this);
    API_VERIFY(0 != GetModuleFileName(NULL, szModulePath, _countof(szModulePath)));
    _tcsrchr(szModulePath, TEXT('\\'))[1]='\0';
    API_VERIFY_EXCEPT1(finder.Find(szModulePath, _T("*.ini"), FALSE), ERROR_FILE_NOT_FOUND);

    CString strActiveIni = AfxGetApp()->GetProfileString(SECTION_CONFIG, ENTRY_ACTIVE_INI);
    UINT nActiveMenuId = (UINT)(-1);
    m_menuIni.CreatePopupMenu();
    for (int i = 0; i < m_iniFiles.GetCount(); i++)
    {
        CString strIniFileName = m_iniFiles.GetAt(i);
        m_menuIni.AppendMenu(MF_STRING, IDC_SETTING_CONFIG_INI_BEGIN + i, strIniFileName);
        if (!strActiveIni.IsEmpty() && strActiveIni.CompareNoCase(strIniFileName) == 0)
        {
            nActiveMenuId = IDC_SETTING_CONFIG_INI_BEGIN + i;
        }
    }
    if (nActiveMenuId != (UINT)(-1))
    {
        API_VERIFY(m_menuIni.CheckMenuRadioItem(IDC_SETTING_CONFIG_INI_BEGIN, IDC_SETTING_CONFIG_INI_END
            ,nActiveMenuId, MF_BYCOMMAND|MF_CHECKED));
        ((CLogViewerDoc*)GetActiveDocument())->m_FTLogManager.m_logConfig.LoadConfig(strActiveIni);
    }
    
    GetMenu()->AppendMenu(MF_POPUP, (UINT)m_menuIni.m_hMenu, TEXT("ConfigIni"));

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers




BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    BOOL bRet = FALSE;
    FTLTRACE(TEXT("OnCreateClient, lpcs={cx=%d, cy=%d}\n"),
        lpcs->cx, lpcs->cy);
    // create a splitter with 1 row, 2 columns
    CRect rcWin;
    GetWindowRect(&rcWin);
    API_VERIFY(m_wndHorzSplitter.CreateStatic(this, 1, 2));
    
    API_VERIFY(m_wndHorzSplitter.CreateView(0, 0, RUNTIME_CLASS(CMachinePidTidTreeView), 
        CSize(200, 300), pContext));

//  	API_VERIFY(m_wndIdVertSplitter.CreateStatic(&m_wndHorzSplitter, 1, 1,
//  		WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
//  		m_wndHorzSplitter.IdFromRowCol(0,0)));
// 
// 	API_VERIFY(m_wndIdVertSplitter.CreateView(0, 0,
// 		RUNTIME_CLASS(CProcessView), CSize(200, 300), pContext));
// 
// 	API_VERIFY(m_wndIdVertSplitter.CreateView(1, 0,
// 		RUNTIME_CLASS(CThreadView), CSize(200, 300), pContext));
// 	m_wndHorzSplitter.SetColumnInfo(0, 150, 10);

//     API_VERIFY(m_wndHorzSplitter.CreateView(0, 0,
//         pContext->m_pNewViewClass, CSize(100, 600), pContext));
    
    API_VERIFY(m_wndVertSplitter.CreateStatic(&m_wndHorzSplitter, 2, 1,
        WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
        m_wndHorzSplitter.IdFromRowCol(0,1)));

    //API_VERIFY(m_wndVertSplitter.CreateView(0, 0,
    //    RUNTIME_CLASS(CSplitterWnd),CSize(0, 0), pContext));
    
    API_VERIFY(m_wndVertSplitter.CreateView(0, 0,
        RUNTIME_CLASS(CLogFilterView), CSize(0, rcWin.Height() - 380), pContext));

    API_VERIFY(m_wndVertSplitter.CreateView(1, 0,
        RUNTIME_CLASS(CLogItemView), CSize(0, 0), pContext));

    // activate the input view
    SetActiveView((CView*)m_wndVertSplitter.GetPane(1,0));

    return TRUE;
}

void CMainFrame::OnUpdateIndicatorFileCount(CCmdUI *pCmdUI)
{
    LONG lFileCount = ((CLogViewerDoc*)GetActiveDocument())->m_FTLogManager.GetLogFileCount();
    CString strFormat;
    strFormat.Format(ID_INDICATOR_FILE_COUNT,lFileCount);
    pCmdUI->SetText(strFormat);
}

void CMainFrame::OnUpdateIndicatorThreadCount(CCmdUI* pCmdUI)
{
    LONG nSelectedThreadCount = ((CLogViewerDoc*)GetActiveDocument())->m_FTLogManager.GetSelectedThreadCount();
    CString strFormat;
    strFormat.Format(ID_INDICATOR_THREAD_COUNT,nSelectedThreadCount);
    pCmdUI->SetText(strFormat);
}

void CMainFrame::OnUpdateIndicatorLogItemCount(CCmdUI *pCmdUI)
{
    LONG lLogItemCount = ((CLogViewerDoc*)GetActiveDocument())->m_FTLogManager.GetDisplayLogItemCount();
    CString strFormat;
    strFormat.Format(ID_INDICATOR_LOGITEM_COUNT,lLogItemCount);
    pCmdUI->SetText(strFormat);
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
    UINT nCount = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
    //for(UINT i = 0; i < nCount; i++ ) 
    if (1 == nCount)
    {
        UINT nSize = ::DragQueryFile(hDropInfo, 0, NULL, 0);
        FTL::CFMemAllocator<TCHAR> buf(++nSize);
        ::DragQueryFile(hDropInfo, 0, buf.GetMemory(), nSize);
        FTLTRACE(TEXT("DragFiles[%d]=%s\n"), 0, buf.GetMemory());

        CStringArray allLogFiles;
        allLogFiles.Add(buf.GetMemory());
        ((CLogViewerDoc*)GetActiveDocument())->m_FTLogManager.SetLogFiles(allLogFiles);
        ((CLogViewerDoc*)GetActiveDocument())->SendInitialUpdate();
    }
    //CFrameWnd::OnDropFiles(hDropInfo);
}

void CMainFrame::OnSettingConfigIniChange(UINT nID)
{
    BOOL bRet = FALSE;
    UINT index = nID - IDC_SETTING_CONFIG_INI_BEGIN;
    FTLTRACE(TEXT("OnSettingConfigIniChange, index=%d\n"), index);
    API_VERIFY(m_menuIni.CheckMenuRadioItem(IDC_SETTING_CONFIG_INI_BEGIN, IDC_SETTING_CONFIG_INI_END
        ,nID, MF_BYCOMMAND|MF_CHECKED));

    CString strIniFileName;
    m_menuIni.GetMenuString(nID, strIniFileName, MF_BYCOMMAND);
    AfxGetApp()->WriteProfileString(SECTION_CONFIG, ENTRY_ACTIVE_INI, strIniFileName);

    ((CLogViewerDoc*)GetActiveDocument())->m_FTLogManager.m_logConfig.LoadConfig(strIniFileName);
}

void CMainFrame::OnCodePageChange(UINT nID)
{
    BOOL bRet = FALSE;
    
    CString strMenuText;
    CMenu* pMenuCodePage = GetMenu()->GetSubMenu(MENU_INDEX_CODEPAGE);
    if (pMenuCodePage)
    {
        //pMenuCodePage->GetMenuString(0, strMenuText, MF_BYPOSITION);
        //FTLTRACE(TEXT("OnCodePageChange, nID=%d, strMenuText=%s\n"), nID, strMenuText);

        API_VERIFY(pMenuCodePage->CheckMenuRadioItem(IDC_CODE_PAGE_BEGIN, IDC_CODE_PAGE_END
            ,nID, MF_BYCOMMAND|MF_CHECKED));
    }

    //TODO:codepage?
    CLogManager& logManager = ((CLogViewerDoc*)GetActiveDocument())->m_FTLogManager;
    switch (nID)
    {
    case ID_CODE_PAGE_UTF8:
        logManager.SetCodepage(CP_UTF8);
        break;
    case ID_CODE_PAGE_GB2312:
        //MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
        logManager.SetCodepage(936); 
        break;
    case ID_CODE_PAGE_JAPANESE:
        logManager.SetCodepage(932);
        break;
    case ID_CODE_PAGE_KOREAN:
        logManager.SetCodepage(949);
        break;
    default:
        FTLASSERT(FALSE);
        break;
    }
}