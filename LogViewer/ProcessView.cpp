// ProcessViewer.cpp : implementation file
//

#include "stdafx.h"
#include "LogViewer.h"
#include "ProcessView.h"


// CProcessView

IMPLEMENT_DYNCREATE(CProcessView, CListView)

CProcessView::CProcessView()
{
    m_bInited = FALSE;
    m_bUpdateLogItems = FALSE;
    //m_dwDefaultStyle |= ( LVS_REPORT | LVS_EX_CHECKBOXES);
}

CProcessView::~CProcessView()
{
}

BEGIN_MESSAGE_MAP(CProcessView, CListView)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CProcessView::OnLvnItemchanged)
    ON_COMMAND(ID_PROCESS_SELECT_ALL, &CProcessView::OnProcessSelectAll)
    ON_COMMAND(ID_PROCESS_UNSELECT_ALL, &CProcessView::OnProcessUnselectAll)
    ON_COMMAND(ID_PROCESS_SELECT_REVERSE, &CProcessView::OnProcessSelectReverse)
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


// CProcessView diagnostics

#ifdef _DEBUG
void CProcessView::AssertValid() const
{
	CListView::AssertValid();
}

void CProcessView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG


// CProcessView message handlers

void CProcessView::OnInitialUpdate()
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
            lvc.pszText = TEXT("ProcessID");
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
    CString strProcessID;
    AllProcessIdContainer allProcessIds;
    pDoc->m_FTLogManager.GetProcessIds(allProcessIds);

    int i = 0;
    FTL::CFConversion conv;
    for (AllProcessIdContainerIter iter = allProcessIds.begin();
        allProcessIds.end() != iter; 
		++iter)
    {
        //strProcessID.Format(TEXT("%d"),*iter); 
        strProcessID = conv.UTF8_TO_TCHAR(iter->c_str());

        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        lvi.state = INDEXTOSTATEIMAGEMASK(2);
        lvi.iItem = i++;
        lvi.iSubItem = 0;
        lvi.pszText = (LPTSTR)(LPCTSTR)strProcessID;
        //lvi.iImage = i;
        int nIndex = ListCtrl.InsertItem(&lvi);
        ListCtrl.SetCheck(nIndex,TRUE);
    }
    m_bUpdateLogItems = FALSE;
}

void CProcessView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/)
{
}

void CProcessView::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
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
                CString strProcessId = ListCtrl.GetItemText(pNMLV->iItem, pNMLV->iSubItem);
                PROCESS_ID_TYPE processId = conv.TCHAR_TO_UTF8(strProcessId);//  _tstoi64((LPCTSTR)strThreadId);
                if(bChecked)  // 选中状态
                {
                    pDoc->m_FTLogManager.SetProcessIdChecked(processId,TRUE);
                }
                else //取消状态
                {
                    pDoc->m_FTLogManager.SetProcessIdChecked(processId,FALSE);
                }
                pDoc->UpdateAllViews(this);
            }
        }
    }
    *pResult = 0;
}

void CProcessView::OnProcessSelectAll()
{
    CLogViewerDoc* pDoc = GetDocument();
    //LONG lLogFileCount = pDoc->m_FTLogManager.GetLogFileCount();
    CListCtrl& listCtrl = GetListCtrl();

    AllProcessIdContainer allProcessIds;
    pDoc->m_FTLogManager.GetProcessIds(allProcessIds);

    int i = 0;
    for (AllProcessIdContainerIter iter = allProcessIds.begin();
        allProcessIds.end() != iter ; ++iter)
    {
        listCtrl.SetCheck(i,TRUE);
        i++;
        pDoc->m_FTLogManager.SetProcessIdChecked(*iter,TRUE);

    }
    pDoc->m_FTLogManager.DoFilterLogItems();
}

void CProcessView::OnProcessUnselectAll()
{
    CLogViewerDoc* pDoc = GetDocument();
    //LONG lLogFileCount = pDoc->m_FTLogManager.GetLogFileCount();
    CListCtrl& listCtrl = GetListCtrl();

    AllProcessIdContainer allProcessIds;
    pDoc->m_FTLogManager.GetProcessIds(allProcessIds);

    int i = 0;
    for (AllProcessIdContainerIter iter = allProcessIds.begin();
        allProcessIds.end() != iter ; ++iter)
    {
        listCtrl.SetCheck(i,FALSE);
        i++;
        pDoc->m_FTLogManager.SetProcessIdChecked(*iter,FALSE);
    }
    pDoc->m_FTLogManager.DoFilterLogItems();
}

void CProcessView::OnProcessSelectReverse()
{
    CLogViewerDoc* pDoc = GetDocument();
    //LONG lLogFileCount = pDoc->m_FTLogManager.GetLogFileCount();
    CListCtrl& listCtrl = GetListCtrl();

    AllProcessIdContainer allProcessIds;
    pDoc->m_FTLogManager.GetProcessIds(allProcessIds);

    int i = 0;
    for (AllProcessIdContainerIter iter = allProcessIds.begin();
        allProcessIds.end() != iter ; ++iter)
    {
        BOOL bChecked = listCtrl.GetCheck(i);
        listCtrl.SetCheck(i, !bChecked);
        i++;
        pDoc->m_FTLogManager.SetProcessIdChecked(*iter, !bChecked);
    }
    pDoc->m_FTLogManager.DoFilterLogItems();
}

void CProcessView::OnContextMenu(CWnd* pWnd, CPoint point)
{
    //if(KEY_DOWN(VK_CONTROL))
    {
        CMenu menuProcess;
        BOOL bRet = FALSE;
        API_VERIFY(menuProcess.LoadMenu(IDR_MENU_PROCESS));
        CMenu* pProcessMenu = menuProcess.GetSubMenu(0);
        FTLASSERT(pProcessMenu);
        API_VERIFY(pProcessMenu->TrackPopupMenu(TPM_TOPALIGN|TPM_LEFTBUTTON,point.x,point.y,pWnd));
    }
}