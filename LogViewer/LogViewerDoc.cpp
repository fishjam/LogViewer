// LogViewerDoc.cpp : implementation of the CLogViewerDoc class
//

#include "stdafx.h"
#include "LogViewer.h"
#include "LogViewerDoc.h"
#include "StudioListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLogViewerDoc

IMPLEMENT_DYNCREATE(CLogViewerDoc, CDocument)

BEGIN_MESSAGE_MAP(CLogViewerDoc, CDocument)
    ON_COMMAND(ID_FILE_OPEN, &CLogViewerDoc::OnFileOpen)

    //采用另存的方式,免得覆盖源文件.如果想覆盖,自行选择同一个文件
    ON_COMMAND(ID_FILE_SAVE, &CLogViewerDoc::OnFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, &CLogViewerDoc::OnFileSaveAs)

    ON_COMMAND_RANGE(ID_FILE_EXPORT_FOR_LOG, ID_FILE_EXPORT_FOR_COMPARE, &CLogViewerDoc::OnFileExport)

    ON_COMMAND(ID_FILE_EXPLORER, &CLogViewerDoc::OnFileExplorer)
    ON_COMMAND(ID_FILE_CLOSE, &CLogViewerDoc::OnFileClose)
    ON_COMMAND(ID_FILE_RELOAD, &CLogViewerDoc::OnFileReload)
    //ON_COMMAND(ID_FILE_GENERATE_LOG, &CLogViewerDoc::OnFileGenerateLog)
END_MESSAGE_MAP()


// CLogViewerDoc construction/destruction

CLogViewerDoc::CLogViewerDoc()
{
    BOOL bRet = FALSE;
    TCHAR Path[MAX_PATH] = {0};
    API_VERIFY(0 != GetModuleFileName(NULL, Path, _countof(Path)));
    _tcsrchr(Path, TEXT('\\'))[1]='\0';
    CString strPluginPath(Path);
    strPluginPath += TEXT("Plugins");

    //m_logFilterPlugs.Init(strPluginPath, "GetLogFilter");
}

CLogViewerDoc::~CLogViewerDoc()
{
    //m_logFilterPlugs.UnInit();
}

BOOL CLogViewerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CLogViewerDoc serialization

void CLogViewerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CLogViewerDoc diagnostics

#ifdef _DEBUG
void CLogViewerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CLogViewerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CLogViewerDoc commands

void CLogViewerDoc::GetFileFilterString(CString & strFilter)
{
	strFilter += TEXT("Log Files (*.log;*.txt)|*.log;*.txt|");

    CString allFilter;
    VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
    strFilter += allFilter;
    strFilter += TEXT("|*.*||");   // next string please
	//strFilter.Replace(TEXT('|'), TEXT('\0'));
}

BOOL CLogViewerDoc::OnOpenDocument(LPCTSTR lpszPathName){
    BOOL bRet = FALSE;
    CStringArray allLogFiles;
    allLogFiles.Add(lpszPathName);
    API_VERIFY(m_FTLogManager.SetLogFiles(allLogFiles));
    if (bRet)
    {
        m_VSIdeHandler.ClearActiveIDE();
        SendInitialUpdate();
        UpdateAllViews(NULL);
    }
    return bRet;
}

void CLogViewerDoc::OnFileOpen()
{
    BOOL bRet = FALSE;
    CString strFilter;
    GetFileFilterString(strFilter);

    //获取当前打开的文件目录,以其作为初始目录,从而方便选相关文件 -- 实测总是不能正确选择初始目录, 为什么?
    CString strCurLogFilePath = m_FTLogManager.GetFirstLogFilePath();

    //strCurLogFilePath.Replace(TEXT("\\"), TEXT("/"));

    CFileDialog dlgFile(TRUE, NULL, strCurLogFilePath, OFN_READONLY| OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT,
        strFilter, NULL, 0);
    //dlgFile.m_ofn.lpstrInitialDir = strCurLogFilePath;  //设置了也无效

    CString title;
    VERIFY(title.LoadString(ID_FILE_OPEN));

    LPTSTR  pFilePathBuf = new TCHAR[1024 * 8];
    pFilePathBuf[0] = NULL;
    dlgFile.m_ofn.lpstrFile = pFilePathBuf;
    dlgFile.m_ofn.nMaxFile = 1024 * 8;
    INT_PTR nResult = dlgFile.DoModal();
    if (IDOK == nResult)
    {
        CStringArray allLogFiles;
        POSITION pos = dlgFile.GetStartPosition();
        while (pos)
        {
            CString strFilePath = dlgFile.GetNextPathName(pos);
            allLogFiles.Add(strFilePath);
        }
        m_FTLogManager.SetLogFiles(allLogFiles);
        m_VSIdeHandler.ClearActiveIDE();
        SendInitialUpdate();
        UpdateAllViews(NULL);
    }
    delete [] pFilePathBuf;
}

void CLogViewerDoc::OnFileSave()
{
    OnFileExport(ID_FILE_EXPORT_FOR_LOG);
}

void CLogViewerDoc::OnFileSaveAs()
{
    OnFileExport(ID_FILE_EXPORT_FOR_LOG);
}

void CLogViewerDoc::OnFileExport(UINT nID)
{
    BOOL bRet = FALSE;
    CString strFilter;
    GetFileFilterString(strFilter);
    
    DWORD dwField = EXPORT_FIELD_DEFAULT;
    if (ID_FILE_EXPORT_FOR_COMPARE == nID)
    {
        dwField = EXPORT_FIELD_COMPARE;
    }

    CFileDialog dlgFile(FALSE, TEXT("log"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        strFilter, NULL, 0);
    INT_PTR nResult = dlgFile.DoModal();
    if (IDOK == nResult)
    {
        CString strPathName = dlgFile.GetPathName();
        API_VERIFY(m_FTLogManager.ExportLogItems(strPathName, dwField, FALSE));
    }
}

void CLogViewerDoc::OnFileExplorer() 
{
    CString strFirstLogFilePath = m_FTLogManager.GetFirstLogFilePath();
    if (!strFirstLogFilePath.IsEmpty())
    {
        HRESULT hr = E_FAIL;
        COM_VERIFY(CFShellUtil::ExplorerToSpecialFile(strFirstLogFilePath));
    }
}

void CLogViewerDoc::OnFileClose()
{
    m_VSIdeHandler.ClearActiveIDE();
	m_FTLogManager.ClearAllLogItems();
	SendInitialUpdate();
}

void CLogViewerDoc::OnFileReload()
{
    m_VSIdeHandler.ClearActiveIDE();
	m_FTLogManager.ReloadLogItems();
	SendInitialUpdate();
}

// void CLogViewerDoc::OnFileGenerateLog(){
//     //生成 Xx W记录
//     int ret = FormatMessageBox(NULL, TEXT("Confirm"), MB_OKCANCEL, 
//         TEXT("Now will generate 100K record file log, Yes Continue"));
//     if (IDOK == ret)
//     {
//         CFileDialog dlg(FALSE);
//         if(IDOK == dlg.DoModal()){
//             m_FTLogManager.GenerateLog(dlg.GetPathName(), 100000);
//         }
//     }
// }

BOOL CLogViewerDoc::GoToLineInSourceCode(LPCTSTR pszFileName,int line)
{
    HRESULT hr = E_FAIL;

    if (!m_FTLogManager.TryOpenByTool(pszFileName, line)) {

        _SelectActiveIde();
        if (m_VSIdeHandler.HadSelectedActiveIDE())
        {
            COM_VERIFY(m_VSIdeHandler.GoToLineInSourceCode(pszFileName, line));
        }
    }
    return TRUE;
}

BOOL CLogViewerDoc::_SelectActiveIde() {
    BOOL bRet = FALSE;
    if (FALSE == m_VSIdeHandler.HadSelectedActiveIDE())
    {
        CStudioListDlg listDlg(m_VSIdeHandler);
        if (IDOK == listDlg.DoModal())
        {
            if (listDlg.m_SelectedIDE != NULL)
            {
                m_VSIdeHandler.SetActiveIDE(listDlg.m_SelectedIDE);
            }
        }
    }
    return bRet;
}
