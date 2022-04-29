#pragma once


// CDialogGoTo dialog

class CDialogGoTo : public CDialog
{
	DECLARE_DYNAMIC(CDialogGoTo)

public:
	CDialogGoTo(UINT nInitGotoLineNum, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogGoTo();

    UINT GetGotoLineNumber() const {
        return m_nGotoLineNumber;
    }

// Dialog Data

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_GOTO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    UINT m_nGotoLineNumber;
    CEdit m_editGotoLineNumber;
	DECLARE_MESSAGE_MAP()
};
