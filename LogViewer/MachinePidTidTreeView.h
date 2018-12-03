#pragma once


// CMachinePidTidTreeView view

class CMachinePidTidTreeView : public CTreeView
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
    DECLARE_MESSAGE_MAP()
public:
    virtual void OnInitialUpdate();

    BOOL m_bInited;
private:
    void _InitIdsTree(CTreeCtrl& treeCtrl, MachinePidTidContainer& allMachinePidTidInfos);
public:
    afx_msg void OnNMTVStateImageChanging(NMHDR *pNMHDR, LRESULT *pResult);
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    void SetChildCheck(CTreeCtrl& treeCtrl, HTREEITEM hItem, BOOL bCheck);
    void SetParentCheck(CTreeCtrl& treeCtrl, HTREEITEM hItem, BOOL bCheck);

};


