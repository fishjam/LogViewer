#pragma once


// CCDialogTextLength dialog

class CDialogTextLength : public CDialog
{
	DECLARE_DYNAMIC(CDialogTextLength)

public:
	CDialogTextLength(INT nInitTextLength, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogTextLength();

    INT GetTextLength() const {
        return m_nInitTextLength;
    }

// Dialog Data

#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_DIALOG_LENGTH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    INT m_nInitTextLength;
    CEdit m_editTextLength;
	DECLARE_MESSAGE_MAP()
};
