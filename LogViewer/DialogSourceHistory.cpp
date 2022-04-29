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

CString CDialogSourceHistory::GetSelectPath()
{
    return m_strSelectPath;
}

void CDialogSourceHistory::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_SRC_PATH_HISTORY, m_cmbSrcPathHistory);
}

BOOL CDialogSourceHistory::OnInitDialog()
{
    BOOL bRet = FALSE;
    API_VERIFY(UpdateData(FALSE));

    CAtlString strChooseSourceDir = AfxGetApp()->GetProfileStringW(SECTION_CONFIG, ENTRY_SOURCE_DIR);
    CAtlString strSourcesHistory = AfxGetApp()->GetProfileStringW(SECTION_CONFIG, ENTRY_SOURCES_HISTORY);

    std::list<CAtlString> listSourceDirHistory;
    FTL::Split(strSourcesHistory, TEXT(";"), false, listSourceDirHistory);

    int preSelectIndex = -1;
    int curSourceIndex = 0;
    for (std::list<CAtlString>::iterator iter = listSourceDirHistory.begin();
        iter != listSourceDirHistory.end();
        ++iter)
    {
        m_cmbSrcPathHistory.AddString(*iter);
        if (strChooseSourceDir.CompareNoCase(*iter) == 0)
        {
            preSelectIndex = curSourceIndex;
        }
        curSourceIndex++;
    }
    if (preSelectIndex != -1)
    {
        m_cmbSrcPathHistory.SetCurSel(preSelectIndex);
    }
    
    if (m_cmbSrcPathHistory.GetCurSel() == -1 && !strChooseSourceDir.IsEmpty())
    {
        m_cmbSrcPathHistory.AddString(strChooseSourceDir);
        m_cmbSrcPathHistory.SetCurSel(0);
    }
    return TRUE;
}

void CDialogSourceHistory::OnOK()
{
    BOOL bRet = FALSE;
    int curSelIndex = m_cmbSrcPathHistory.GetCurSel();
    if (curSelIndex != -1)
    {
        //获取当前选择的
        m_cmbSrcPathHistory.GetLBText(curSelIndex, m_strSelectPath);
    }

    CString strSourceHistory;
    BOOL isFirst = TRUE;
    for (int i = 0; i < m_cmbSrcPathHistory.GetCount(); i++)
    {
        CString strCurText;
        m_cmbSrcPathHistory.GetLBText(i, strCurText);
        if (isFirst)
        {
            strSourceHistory.AppendFormat(TEXT("%s"), strCurText);
            isFirst = FALSE;
        }
        else {
            strSourceHistory.AppendFormat(TEXT(";%s"), strCurText);
        }
    }
    AfxGetApp()->WriteProfileString(SECTION_CONFIG, ENTRY_SOURCES_HISTORY, strSourceHistory);
    __super::OnOK();
}

BEGIN_MESSAGE_MAP(CDialogSourceHistory, CDialog)
    ON_BN_CLICKED(IDC_BTN_CHOOSE_SRC_PATH, &CDialogSourceHistory::OnBnClickedBtnChooseSrcPath)
END_MESSAGE_MAP()


// CDialogSourceHistory message handlers


void CDialogSourceHistory::OnBnClickedBtnChooseSrcPath()
{
    CString strChooseSourceDir;
    int nCurSel = m_cmbSrcPathHistory.GetCurSel();
    if (-1 != nCurSel)
    {
        m_cmbSrcPathHistory.GetLBText(nCurSel, strChooseSourceDir);
    }

    FTL::CFDirBrowser dirBrowser(TEXT("Choose Project Source Root Path"), m_hWnd, strChooseSourceDir);
    if (dirBrowser.DoModal())
    {
        CString strFolderPath = dirBrowser.GetSelectPath();

        //查找,并删除,后续重新插入(更改顺序)
        int matchIndex = m_cmbSrcPathHistory.FindString(0, strFolderPath);

        //int matchIndex = _findMatchPathIndex(strFolderPath, TRUE);
        if (matchIndex != -1)
        {
            m_cmbSrcPathHistory.DeleteString(matchIndex);
        }
        else if (m_cmbSrcPathHistory.GetCount() >= SOURCES_HISTORY_COUNT)
        {
            //新的,检测个数,如果太多, 则删除老的
            m_cmbSrcPathHistory.DeleteString(m_cmbSrcPathHistory.GetCount() - 1);
        }
        int nIndex = m_cmbSrcPathHistory.AddString(strFolderPath);
        m_cmbSrcPathHistory.SetCurSel(nIndex);
    }
}

// int CDialogSourceHistory::_findMatchPathIndex(const CAtlString& strCurPath, BOOL bRemove /* = TRUE */)
// {
//     int matchIndex = -1;
//     int curSourceIndex = 0;
// 
//     for (std::list<CAtlString>::iterator iter = m_listSourceDirHistory.begin();
//         iter != m_listSourceDirHistory.end();
//         ++iter)
//     {
//         if (strCurPath.CompareNoCase(*iter) == 0)
//         {
//             matchIndex = curSourceIndex;
//             if (bRemove)
//             {
//                 m_listSourceDirHistory.erase(iter);
//             }
//             break;
//         }
//         curSourceIndex++;
//     }
//     
//     return matchIndex;
// }
// 
// void CDialogSourceHistory::_removeSpecialPath(const CAtlString& strPath)
// {
//     for (std::list<CAtlString>::iterator iter = m_listSourceDirHistory.begin();
//         iter != m_listSourceDirHistory.end();
//         ++iter)
//     {
//         if (strPath.CompareNoCase(*iter))
//         {
//             m_listSourceDirHistory.erase(iter);
//             break;
//         }
//     }
// }