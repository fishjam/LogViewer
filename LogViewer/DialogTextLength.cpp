// DialogTextLength.cpp : implementation file
//

#include "stdafx.h"
#include "LogViewer.h"
#include "DialogTextLength.h"
#include "afxdialogex.h"


// CDialogTextLength dialog

IMPLEMENT_DYNAMIC(CDialogTextLength, CDialog)

CDialogTextLength::CDialogTextLength(INT nInitTextLength, CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DIALOG_LENGTH, pParent)
    , m_nInitTextLength(nInitTextLength)
{
}

CDialogTextLength::~CDialogTextLength()
{
}

void CDialogTextLength::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_TEXT_LENGTH, m_nInitTextLength);
    DDX_Control(pDX, IDC_EDIT_TEXT_LENGTH, m_editTextLength);
}


BEGIN_MESSAGE_MAP(CDialogTextLength, CDialog)
END_MESSAGE_MAP()


// CDialogTextLength message handlers

BOOL CDialogTextLength::OnInitDialog()
{
    UpdateData(FALSE);

    m_editTextLength.SetFocus();
    m_editTextLength.SetSel(0, -1);
    
    return FALSE; // return TRUE unless you set the focus to a control
}

void CDialogTextLength::OnOK()
{
    if (UpdateData(TRUE))
    {
        __super::OnOK();
    }
}
