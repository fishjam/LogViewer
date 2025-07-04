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
            //�°汾����, ��ʽΪ: ·��|״̬
            int nIndex = m_listSourcePaths.AddString(*listSrcWithSelectStatus.begin());
            CAtlString strStatus = *listSrcWithSelectStatus.rbegin();
            int nCheck = StrToInt(strStatus);
            m_listSourcePaths.SetCheck(nIndex, nCheck);
        }
        else {
            //�ϰ汾����,û��ѡ��״̬
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
            //check folder path exist
            if (ptFolder != FTL::CFPath::GetPathType(strPath)) {
                FTL::FormatMessageBox(m_hWnd, TEXT("Wrong source path, please the path exist"), MB_OK | MB_ICONWARNING, TEXT("Path:%s"), strPath);
                m_listSourcePaths.SetCurSel(index);
                return;
            }
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
        BOOL needAdd = FALSE;
        CString strFolderPath = dirBrowser.GetSelectPath();

        int matchIndex = m_listSourcePaths.FindString(0, strFolderPath);
        if (-1 == matchIndex)
        {
            //û�оͲ���
            needAdd = TRUE;
        } else {
            // �п����ҵ�����·��(����:������Ŀ¼, ���Լ��븸Ŀ¼), �������Ƚ�����·��
            CString strFoundFolderPath;
            m_listSourcePaths.GetText(matchIndex, strFoundFolderPath);
            if (0 != strFoundFolderPath.CompareNoCase(strFolderPath)) {
                //��һ��,Ҳ��Ҫ����
                needAdd = TRUE;
            }
        }

        if (needAdd)
        {
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
