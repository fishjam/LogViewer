#pragma once


// CLogItemView ��ͼ
#include "SortHeaderCtrl.h"

class CLogItemView : public CListView
{
    DECLARE_DYNCREATE(CLogItemView)

protected:
    CLogItemView();           // ��̬������ʹ�õ��ܱ����Ĺ��캯��
    virtual ~CLogItemView();

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
    BOOL m_bInited;
    LogItemContentType m_SortContentType;
    BOOL m_bSortAscending;
    CSortHeaderCtrl	m_ctlHeader;
    UINT m_nLastGotToLineNumber;

    void Sort( LogItemContentType contentType, BOOL bAscending );
    //static int CALLBACK CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData );

    void PrepareCache(int iFrom, int iTo);
    void GetDispInfo(LVITEM* pItem);
    int FindItem(int iStart, LVFINDINFO* plvfi);

    LVHITTESTINFO GetCurrentSelectInfo();
    LONG _GetSelectedLines(LogIndexContainer& selectedItemsList, INT& nSelectSubItem);

    CString _GetSelectedText(int textType);

    void _HighlightSameThread(LogItemPointer pCompareLogItem);
    DECLARE_MESSAGE_MAP()
public:
    virtual void OnInitialUpdate();
    //afx_msg void OnColumnClick(UINT id, NMHDR* pNotifyStruct, LRESULT* pResult);
    afx_msg void OnHdnItemclickListAllLogitems(NMHDR *pNMHDR, LRESULT *pResult);
    virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
protected:
    virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
public:
    CPoint  m_ptContextMenuClick; //TODO: change to get item/subItem index when onContextMenu
    afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void OnDetailsHighLightSameThread();
    afx_msg void OnDetailsCopyItemText();
    afx_msg void OnDetailsCopyLineText();
    afx_msg void OnDetailsCopyFullLog();
    afx_msg void OnDetailDeleteSelectItems();
    afx_msg void OnDetailSelectCurrentPid();
    afx_msg void OnDetailSelectCurrentTid();
    afx_msg void OnDetailSelectCurrentFile();
    afx_msg void OnDetailClearFilterByFiles();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnEditGoTo(UINT nID);
    afx_msg BOOL OnEditClearCache(UINT nID);
    afx_msg BOOL OnEditSetSrcPaths(UINT nID);

    afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnUpdateIndicatorSelectedLogItem(CCmdUI *pCmdUI);
private:
    int _GetSelectedIdTypeValue(MachinePIdTIdTypeList& idTypeList);
    int _GetSelectedIdLogItems(LogItemsContainer& selectedLogItem);
    UINT _GetCurrentFirstSelectLine();
    BOOL _GotoSpecialLine(UINT nGotoLineNumber);
};


