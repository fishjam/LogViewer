#pragma once


// CDialogGoTo dialog

class CDialogGoTo : public CDialog
{
	DECLARE_DYNAMIC(CDialogGoTo)

public:
	CDialogGoTo(UINT nInitGotoSeqNum, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogGoTo();

    UINT GetGotoSeqNumber() const {
        return m_nGotoSeqNumber;
    }

// Dialog Data

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_GOTO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    UINT m_nGotoSeqNumber;
    CEdit m_editGotoSeqNumber;
	DECLARE_MESSAGE_MAP()
};
