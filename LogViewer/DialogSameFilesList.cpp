// DialogSameFilesList.cpp : implementation file
//

#include "stdafx.h"
#include "LogViewer.h"
#include "DialogSameFilesList.h"
#include "afxdialogex.h"


// CDialogSameFilesList dialog

IMPLEMENT_DYNAMIC(CDialogSameFilesList, CDialog)

BEGIN_MESSAGE_MAP(CDialogSameFilesList, CDialog)
	ON_LBN_SELCHANGE(IDC_LIST_SAME_FILES, &CDialogSameFilesList::OnLbnSelchangeListSameFiles)
END_MESSAGE_MAP()


CDialogSameFilesList::CDialogSameFilesList(SameNameFilePathListPtr spSameNameFilePathList, CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DIALOG_SAME_FILE_LISTS, pParent)
	, m_spSameNameFilePathList(spSameNameFilePathList)
{

}

CDialogSameFilesList::~CDialogSameFilesList()
{
}

void CDialogSameFilesList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SAME_FILES, m_SameFilesList);
}

BOOL CDialogSameFilesList::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (m_spSameNameFilePathList)
	{
		for (SameNameFilePathList::iterator iter = m_spSameNameFilePathList->begin();
			iter != m_spSameNameFilePathList->end();
			iter++)
		{
			CString strFullPath = *iter;
			int itemIndex = m_SameFilesList.AddString(strFullPath);
		}
	}
	return TRUE;  // return TRUE unless you set the focus to a control
}



afx_msg void CDialogSameFilesList::OnLbnSelchangeListSameFiles()
{
	int curSel = m_SameFilesList.GetCurSel();
	if (curSel != LB_ERR)
	{
		m_SameFilesList.GetText(curSel, m_strSelectFilePath);
	}
	else {
		m_strSelectFilePath = TEXT("");
	}
}
// CDialogSameFilesList message handlers
