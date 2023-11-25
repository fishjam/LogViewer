// DialogSourceHistory.cpp : implementation file
//

#include "stdafx.h"
#include "LogViewer.h"
#include "DialogSourceHistory.h"
#include "afxdialogex.h"
#include <ftlShell.h>
#include <ftlFunctional.h>

// CDialogSourceHistory dialog

IMPLEMENT_DYNAMIC(CDialogSourceHistory, CDialog)

CDialogSourceHistory::CDialogSourceHistory(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DIALOG_SOURCE_HISTORY, pParent)
{
}

CDialogSourceHistory::~CDialogSourceHistory()
{
}

const CStringArray& CDialogSourceHistory::GetSelectPaths() const
{
    return m_selectedPaths;
}

void CDialogSourceHistory::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_SRC_PATHS, m_listSourcePaths);
}

BOOL CDialogSourceHistory::OnInitDialog()
{
    BOOL bRet = FALSE;
    API_VERIFY(UpdateData(FALSE));

    CAtlString strSourcesHistory = AfxGetApp()->GetProfileStringW(SECTION_CONFIG, ENTRY_SOURCES_HISTORY);

    std::list<CAtlString> listSourceDirHistory;
    FTL::Split(strSourcesHistory, TEXT(";"), false, listSourceDirHistory);

    for (std::list<CAtlString>::iterator iter = listSourceDirHistory.begin();
        iter != listSourceDirHistory.end();
        ++iter)
    {
        CAtlString strSrcWithStatus = *iter;
        std::list<CAtlString> listSrcWithSelectStatus;
        FTL::Split(strSrcWithStatus, TEXT("|"), false, listSrcWithSelectStatus);
        if (listSrcWithSelectStatus.size() == 2) {
            //新版本数据, 格式为: 路径|状态
            int nIndex = m_listSourcePaths.AddString(*listSrcWithSelectStatus.begin());
            CAtlString strStatus = *listSrcWithSelectStatus.rbegin();
            int nCheck = StrToInt(strStatus);
            m_listSourcePaths.SetCheck(nIndex, nCheck);
        }
        else {
            //老版本数据,没有选择状态
            m_listSourcePaths.AddString(*iter);
        }
    }
    return TRUE;
}

void CDialogSourceHistory::OnOK()
{
    BOOL bRet = FALSE;
    BOOL isFirst = TRUE;
    CString strSourcePaths;
    m_selectedPaths.RemoveAll();

    for (int index = 0; index < m_listSourcePaths.GetCount(); index++) {
        CString strPath;
        int nCheck = m_listSourcePaths.GetCheck(index);
        m_listSourcePaths.GetText(index, strPath);

        if (nCheck > 0) {
            m_selectedPaths.Add(strPath);
        }
        if (isFirst)
        {
            strSourcePaths.AppendFormat(TEXT("%s|%d"), strPath, nCheck);
            isFirst = FALSE;
        }
        else {
            strSourcePaths.AppendFormat(TEXT(";%s|%d"), strPath, nCheck);
        }
    }
   
    AfxGetApp()->WriteProfileString(SECTION_CONFIG, ENTRY_SOURCES_HISTORY, strSourcePaths);
    __super::OnOK();
}

BEGIN_MESSAGE_MAP(CDialogSourceHistory, CDialog)
    ON_BN_CLICKED(IDC_BTN_ADD_SRC_PATH, &CDialogSourceHistory::OnBnClickedBtnAddSrcPath)
    ON_BN_CLICKED(IDC_BTN_DEL_SRC_PATH, &CDialogSourceHistory::OnBnClickedBtnDelSrcPath)
END_MESSAGE_MAP()


void CDialogSourceHistory::OnBnClickedBtnAddSrcPath()
{
    CString strSelectPath;
    int nSelectIndex = m_listSourcePaths.GetCurSel();
    if (nSelectIndex > -1) {
        m_listSourcePaths.GetText(nSelectIndex, strSelectPath);
    }
    
    FTL::CFDirBrowser dirBrowser(TEXT("Choose Project Source Root Path"), m_hWnd, strSelectPath);
    if (dirBrowser.DoModal())
    {
        CString strFolderPath = dirBrowser.GetSelectPath();

        int matchIndex = m_listSourcePaths.FindString(0, strFolderPath);
        if (-1 == matchIndex)
        {
            //没有才插入
            int nIndex = m_listSourcePaths.AddString(strFolderPath);
            m_listSourcePaths.SetCheck(nIndex, 1);
        }
    }
}


void CDialogSourceHistory::OnBnClickedBtnDelSrcPath()
{
    int nSelectedIndex = m_listSourcePaths.GetCurSel();
    if (-1 != nSelectedIndex) {
        m_listSourcePaths.DeleteString(nSelectedIndex);
    }
}
