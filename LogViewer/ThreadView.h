#pragma once

// CThreadView view
class CThreadView : public CListView
{
	DECLARE_DYNCREATE(CThreadView)

protected:
	CThreadView();           // protected constructor used by dynamic creation
	virtual ~CThreadView();

public:
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
	DECLARE_MESSAGE_MAP()
public:
    virtual void OnInitialUpdate();
protected:
    virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
    BOOL m_bInited;
    BOOL m_bUpdateLogItems;
public:
    afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnThreadSelectAll();
    afx_msg void OnThreadUnselectAll();
    afx_msg void OnThreadSelectReverse();
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};


