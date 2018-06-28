// ThreadViewer.cpp : implementation file
//

#include "stdafx.h"
#include "LogViewer.h"
#include "ThreadView.h"


// CThreadView

IMPLEMENT_DYNCREATE(CThreadView, CListView)

CThreadView::CThreadView()
{
    m_bInited = FALSE;
    m_bUpdateLogItems = FALSE;
    //m_dwDefaultStyle |= ( LVS_REPORT | LVS_EX_CHECKBOXES);
}

CThreadView::~CThreadView()
{
}

BEGIN_MESSAGE_MAP(CThreadView, CListView)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CThreadView::OnLvnItemchanged)
    ON_COMMAND(ID_THREAD_SELECT_ALL, &CThreadView::OnThreadSelectAll)
    ON_COMMAND(ID_THREAD_UNSELECT_ALL, &CThreadView::OnThreadUnselectAll)
    ON_COMMAND(ID_THREAD_SELECT_REVERSE, &CThreadView::OnThreadSelectReverse)
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


// CThreadView diagnostics

#ifdef _DEBUG
void CThreadView::AssertValid() const
{
	CListView::AssertValid();
}

void CThreadView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG


// CThreadView message handlers

void CThreadView::OnInitialUpdate()
{
    CListView::OnInitialUpdate();
    CListCtrl& ListCtrl = GetListCtrl();
    if (FALSE == m_bInited)
    {
        ListCtrl.ModifyStyle(LVS_ICON,LVS_REPORT | LVS_SORTASCENDING,0);
        ListCtrl.SetExtendedStyle(ListCtrl.GetExtendedStyle() | LVS_EX_CHECKBOXES);

        LV_COLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        for(int i = 0; i< 1; i++)
        {
            lvc.iSubItem = i;
            lvc.pszText = TEXT("ThreadID");
            lvc.cx = 150;
            lvc.fmt = LVCFMT_LEFT;
            ListCtrl.InsertColumn(i,&lvc);
        }
        m_bInited = TRUE;
    }
    ListCtrl.DeleteAllItems();

    m_bUpdateLogItems = TRUE;

    CLogViewerDoc* pDoc = GetDocument();
    //LONG lLogFileCount = pDoc->m_FTLogManager.GetLogFileCount();
    LV_ITEM lvi = {0};
    CString strThreadID;
    AllThreadIdContainer allThreadIds;
    pDoc->m_FTLogManager.GetThreadIds(allThreadIds);

    int i = 0;
	FTL::CFConversion conv;
    for (AllThreadIdContainerIter iter = allThreadIds.begin();
        allThreadIds.end() != iter; 
		++iter)
    {
        //strThreadID.Format(TEXT("%d"),*iter); 
		strThreadID = conv.UTF8_TO_TCHAR(iter->c_str());

        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        lvi.state = INDEXTOSTATEIMAGEMASK(2);
        lvi.iItem = i++;
        lvi.iSubItem = 0;
        lvi.pszText = (LPTSTR)(LPCTSTR)strThreadID;
        //lvi.iImage = i;
        int nIndex = ListCtrl.InsertItem(&lvi);
        ListCtrl.SetCheck(nIndex,TRUE);
    }
    m_bUpdateLogItems = FALSE;
}

void CThreadView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/)
{
}

void CThreadView::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    if (!m_bUpdateLogItems)
    {
        LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
        CListCtrl& ListCtrl = GetListCtrl();
        if(pNMLV->iItem != -1 )
        {
            CLogViewerDoc* pDoc = GetDocument();

            //check Check Status       
            BOOL   bPrevState   =   (BOOL)(((pNMLV->uOldState & LVIS_STATEIMAGEMASK)>>12)-1);       
            if   (bPrevState   <   0)         //   On   startup   there's   no   previous   state     
                bPrevState   =   0;   //   so   assign   as   false   (unchecked)   
            //   New   check   box   state   
            BOOL   bChecked   = (BOOL)(((pNMLV->uNewState   &   LVIS_STATEIMAGEMASK)>>12)-1);  
            if   (bChecked   <   0)   //   On   non-checkbox   notifications   assume   false   
                bChecked   =   0;     
            if   (bPrevState   !=   bChecked)   //  change   in   check   box   
            {
				FTL::CFConversion conv;
                CString strThreadId = ListCtrl.GetItemText(pNMLV->iItem, pNMLV->iSubItem);
                THREAD_ID_TYPE threadId = conv.TCHAR_TO_UTF8(strThreadId);//  _tstoi64((LPCTSTR)strThreadId);
                if(bChecked)  // 选中状态
                {
                    pDoc->m_FTLogManager.SetThreadIdChecked(threadId,TRUE);
                }
                else //取消状态
                {
                    pDoc->m_FTLogManager.SetThreadIdChecked(threadId,FALSE);
                }
                pDoc->UpdateAllViews(this);
            }
        }
    }
    *pResult = 0;
}

void CThreadView::OnThreadSelectAll()
{
    CLogViewerDoc* pDoc = GetDocument();
    //LONG lLogFileCount = pDoc->m_FTLogManager.GetLogFileCount();
    CListCtrl& listCtrl = GetListCtrl();

    AllThreadIdContainer allThreadIds;
    pDoc->m_FTLogManager.GetThreadIds(allThreadIds);

    int i = 0;
    for (AllThreadIdContainerIter iter = allThreadIds.begin();
        allThreadIds.end() != iter ; ++iter)
    {
        listCtrl.SetCheck(i,TRUE);
        i++;
        pDoc->m_FTLogManager.SetThreadIdChecked(*iter,TRUE);

    }
    pDoc->m_FTLogManager.DoFilterLogItems();
}

void CThreadView::OnThreadUnselectAll()
{
    CLogViewerDoc* pDoc = GetDocument();
    //LONG lLogFileCount = pDoc->m_FTLogManager.GetLogFileCount();
    CListCtrl& listCtrl = GetListCtrl();

    AllThreadIdContainer allThreadIds;
    pDoc->m_FTLogManager.GetThreadIds(allThreadIds);

    int i = 0;
    for (AllThreadIdContainerIter iter = allThreadIds.begin();
        allThreadIds.end() != iter ; ++iter)
    {
        listCtrl.SetCheck(i,FALSE);
        i++;
        pDoc->m_FTLogManager.SetThreadIdChecked(*iter,FALSE);
    }
    pDoc->m_FTLogManager.DoFilterLogItems();
}

void CThreadView::OnThreadSelectReverse()
{
    CLogViewerDoc* pDoc = GetDocument();
    //LONG lLogFileCount = pDoc->m_FTLogManager.GetLogFileCount();
    CListCtrl& listCtrl = GetListCtrl();

    AllThreadIdContainer allThreadIds;
    pDoc->m_FTLogManager.GetThreadIds(allThreadIds);

    int i = 0;
    for (AllThreadIdContainerIter iter = allThreadIds.begin();
        allThreadIds.end() != iter ; ++iter)
    {
        BOOL bChecked = listCtrl.GetCheck(i);
        listCtrl.SetCheck(i, !bChecked);
        i++;
        pDoc->m_FTLogManager.SetThreadIdChecked(*iter, !bChecked);
    }
    pDoc->m_FTLogManager.DoFilterLogItems();
}

void CThreadView::OnContextMenu(CWnd* pWnd, CPoint point)
{
    //if(KEY_DOWN(VK_CONTROL))
    {
        CMenu menuThread;
        BOOL bRet = FALSE;
        API_VERIFY(menuThread.LoadMenu(IDR_MENU_THREAD));
        CMenu* pThreadMenu = menuThread.GetSubMenu(0);
        FTLASSERT(pThreadMenu);
        API_VERIFY(pThreadMenu->TrackPopupMenu(TPM_TOPALIGN|TPM_LEFTBUTTON,point.x,point.y,pWnd));
    }
}