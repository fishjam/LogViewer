#pragma once


// CMachinePidTidTreeView view

class IEnumTreeCtrlItrmCallback {
public:
    virtual void onHandleItem(CTreeCtrl& treeCtrl, const CString& strItem, HTREEITEM hItem, DWORD_PTR param) = 0;
};

class CMachinePidTidTreeView 
    : public CTreeView
    , public IEnumTreeCtrlItrmCallback
{
    DECLARE_DYNCREATE(CMachinePidTidTreeView)

protected:
    CMachinePidTidTreeView();           // protected constructor used by dynamic creation
    virtual ~CMachinePidTidTreeView();

public:
#ifdef _DEBUG
    virtual void AssertValid() const;
#ifndef _WIN32_WCE
    virtual void Dump(CDumpContext& dc) const;
#endif
#endif

    CLogViewerDoc* GetDocument()
    {
        ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CLogViewerDoc)));
        return (CLogViewerDoc*) m_pDocument;
    }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

    DECLARE_MESSAGE_MAP()
public:
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);

    BOOL m_bInited;
    DWORD_PTR m_filterHint;
public:
    //IEnumTreeCtrlItrmCallback
    virtual void onHandleItem(CTreeCtrl& treeCtrl, const CString& strItem, HTREEITEM hItem, DWORD_PTR param);
private:
    void _InitIdsTree(CTreeCtrl& treeCtrl, MachinePidTidContainer& allMachinePidTidInfos);
public:
    afx_msg void OnNMTVStateImageChanging(NMHDR *pNMHDR, LRESULT *pResult);
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    void SetChildCheck(CTreeCtrl& treeCtrl, HTREEITEM hItem, BOOL bCheck);
    void SetParentCheck(CTreeCtrl& treeCtrl, HTREEITEM hItem, BOOL bCheck);
    void enumTreeCtrl(IEnumTreeCtrlItrmCallback* pCallBack, DWORD_PTR param);
};


