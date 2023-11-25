#pragma once
#include "afxwin.h"


// CDialogSourceHistory dialog

class CDialogSourceHistory : public CDialog
{
	DECLARE_DYNAMIC(CDialogSourceHistory)

public:
	CDialogSourceHistory(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogSourceHistory();
    const CStringArray& GetSelectPaths() const;
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
    afx_msg void OnBnClickedBtnAddSrcPath();
    afx_msg void OnBnClickedBtnDelSrcPath();
protected:
    CStringArray m_selectedPaths;
    CCheckListBox m_listSourcePaths;
};
