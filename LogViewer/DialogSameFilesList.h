#pragma once


// CDialogSameFilesList dialog

class CDialogSameFilesList : public CDialog
{
	DECLARE_DYNAMIC(CDialogSameFilesList)

public:
	CDialogSameFilesList(SameNameFilePathListPtr spSameNameFilePathList, CWnd* pParent = NULL);   // standard constructor

	virtual ~CDialogSameFilesList();

	const CString& GetSelectFilePath() const {
		return m_strSelectFilePath;
	}

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SAME_FILE_LISTS };
#endif

protected:
	SameNameFilePathListPtr m_spSameNameFilePathList;
	CListBox m_SameFilesList;
	CString  m_strSelectFilePath;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLbnSelchangeListSameFiles();

};
