#pragma once
#include "afxwin.h"


// CStudioListDlg 对话框
#include "VsIdeHandler.h"

class CStudioListDlg : public CDialog
{
	DECLARE_DYNAMIC(CStudioListDlg)

public:
	CStudioListDlg(CVsIdeHandler& vsIdeHandler,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CStudioListDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_STUDIO_LIST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
    IUnknown*  m_SelectedIDE;
    CListBox m_StudiosList;
    CVsIdeHandler& m_VsIdeHandler;
    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    afx_msg void OnLbnSelchangeListStudios();
};
