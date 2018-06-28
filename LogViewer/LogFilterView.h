#pragma once
#include "afxcmn.h"



// CLogFilterView form view
#include "SortHeaderCtrl.h"
class CLogFilterView : public CFormView,
    public CFWindowAutoSize<CLogFilterView, CFMFCDlgAutoSizeTraits<CLogFilterView> >
{
	DECLARE_DYNCREATE(CLogFilterView)

public:
    BEGIN_WINDOW_RESIZE_MAP(CLogFilterView)
        WINDOW_RESIZE_CONTROL(IDC_EDIT_FILTER_STRING,WINSZ_SIZE_X)
		WINDOW_RESIZE_CONTROL(IDC_EDIT_FULL_TRACEINFO,WINSZ_SIZE_X|WINSZ_SIZE_Y)
    END_WINDOW_RESIZE_MAP()

protected:
	CLogFilterView();           // protected constructor used by dynamic creation
	virtual ~CLogFilterView();

public:
	enum { IDD = IDD_LOG_FILTER_VIEW };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    CLogViewerDoc* GetDocument()
    {
        ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CLogViewerDoc)));
        return (CLogViewerDoc*) m_pDocument;
    }

protected:
    //int  m_LogItemListWidthMargin;
    //int  m_LogItemListHeightMargin;

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    virtual void OnInitialUpdate();
    afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
    virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
    afx_msg void OnBnClickedCheckTraceLevel(UINT nId);
public:
    afx_msg void OnEnChangeEditFilter();
	afx_msg void OnComboboxFilterChange();
	CComboBox m_comboBoxFilter;
    CString m_strFilterString;
	CString m_strFullTraceInfo;
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    BOOL    m_bFilterStringChanged;
    DWORD   m_lastFilterStringChangeTick;
    UINT_PTR m_UpdateFilterStringTimerID;
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnBnClickedCheckIncludeText();
};


