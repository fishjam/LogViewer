#pragma once
#include "afxwin.h"


// CDialogSourceHistory dialog

class CDialogSourceHistory : public CDialog
{
	DECLARE_DYNAMIC(CDialogSourceHistory)

public:
	CDialogSourceHistory(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogSourceHistory();
    CString GetSelectPath();
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SOURCE_HISTORY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedBtnChooseSrcPath();
protected:
    CComboBox m_cmbSrcPathHistory;
    CString   m_strSelectPath;
    //int _findMatchPathIndex(const CAtlString& strCurPath, BOOL bRemove = FALSE);
    //void _removeSpecialPath(const CAtlString& strPath);
};
