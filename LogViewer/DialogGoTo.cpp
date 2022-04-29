// DialogGoTo.cpp : implementation file
//

#include "stdafx.h"
#include "LogViewer.h"
#include "DialogGoTo.h"
#include "afxdialogex.h"


// CDialogGoTo dialog

IMPLEMENT_DYNAMIC(CDialogGoTo, CDialog)

CDialogGoTo::CDialogGoTo(UINT nInitGotoLineNum, CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DIALOG_GOTO, pParent)
    , m_nGotoLineNumber(nInitGotoLineNum)
{
}

CDialogGoTo::~CDialogGoTo()
{
}

void CDialogGoTo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_GOTO_LINENUM, m_nGotoLineNumber);
    DDX_Control(pDX, IDC_EDIT_GOTO_LINENUM, m_editGotoLineNumber);
}


BEGIN_MESSAGE_MAP(CDialogGoTo, CDialog)
END_MESSAGE_MAP()


// CDialogGoTo message handlers

BOOL CDialogGoTo::OnInitDialog()
{
    UpdateData(FALSE);

    m_editGotoLineNumber.SetFocus();
    m_editGotoLineNumber.SetSel(0, -1);
    
    return FALSE; // return TRUE unless you set the focus to a control
}

void CDialogGoTo::OnOK()
{
    if (UpdateData(TRUE))
    {
        __super::OnOK();
    }
}
