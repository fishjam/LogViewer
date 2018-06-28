// LogItemView.cpp : 实现文件
//

#include "stdafx.h"
#include "LogViewer.h"
#include "LogItemView.h"
#include "LogManager.h"

struct strColumnInfo
{
	LPCTSTR pszColumnText;
	int		nColumnWidth;
};

static strColumnInfo	columnInfos[] = 
{
	{TEXT("SeqNum"), 50,},
	{TEXT("PID"), 50,},
	{TEXT("TID"), 80,},
	{TEXT("Time"), 100,},
	{TEXT("Elapse"), 70,},
	{TEXT("Level"), 50,},
	{TEXT("ModuleName"), 20,},
	{TEXT("FunName"), 20,},
    {TEXT("SourceFile"), 20,},
	{TEXT("TraceInfo"), 800,}
};

static LPCTSTR pszTraceLevel[] = 
{
    TEXT("Detail"),
    TEXT("Info"),
    TEXT("Trace"),
    TEXT("Warn"),
    TEXT("Error"),
    TEXT("Unknown"),
};
// CLogItemView

IMPLEMENT_DYNCREATE(CLogItemView, CListView)

CLogItemView::CLogItemView()
{
    m_bInited = FALSE;
	m_SortContentType = type_Sequence;
	m_bSortAscending = TRUE;
    m_dwDefaultStyle |= ( LVS_REPORT | LVS_OWNERDATA );
	m_ptContextMenuClick.SetPoint(-1, -1);
}

CLogItemView::~CLogItemView()
{
}

BEGIN_MESSAGE_MAP(CLogItemView, CListView)
    ON_NOTIFY(HDN_ITEMCLICK, 0, &CLogItemView::OnHdnItemclickListAllLogitems)
    //ON_NOTIFY_RANGE(LVN_COLUMNCLICK,0,0xffff,OnColumnClick)
    //ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
    ON_NOTIFY_REFLECT(NM_CLICK, &CLogItemView::OnNMClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CLogItemView::OnNMDblclk)
	ON_COMMAND(ID_DETAILS_HIGHLIGHT_SAME_THREAD, &CLogItemView::OnDetailsHighLightSameThread)
	ON_COMMAND(ID_DETAILS_COPY_ITEM_TEXT, &CLogItemView::OnDetailsCopyItemText)
    ON_COMMAND(ID_DETAILS_COPY_LINE_TEXT, &CLogItemView::OnDetailsCopyLineText)
    ON_COMMAND(ID_DETAILS_COPY_FULL_LOG, &CLogItemView::OnDetailsCopyFullLog)
	ON_WM_CONTEXTMENU()
    ON_WM_ERASEBKGND()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CLogItemView::OnLvnItemchanged)
END_MESSAGE_MAP()


// CLogItemView 诊断

#ifdef _DEBUG
void CLogItemView::AssertValid() const
{
	CListView::AssertValid();
}

void CLogItemView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG


// CLogItemView 消息处理程序

void CLogItemView::PrepareCache(int /*iFrom*/, int /*iTo*/)
{

}

void CLogItemView::OnInitialUpdate()
{
    //BOOL bRet = FALSE;
    CListView::OnInitialUpdate();
    CListCtrl& ListCtrl = GetListCtrl();
    if (FALSE == m_bInited)
    {
        ListCtrl.ModifyStyle(LVS_ICON,LVS_REPORT | LVS_OWNERDATA,0);
        DWORD dwExStyle = ListCtrl.GetExtendedStyle();
		//TODO: 虽然启用 LVS_EX_TRACKSELECT 后可以显示当前选中的行，但滚动时会自动随着鼠标的位置选择
        dwExStyle |= LVS_EX_FULLROWSELECT  | LVS_EX_ONECLICKACTIVATE 
			| LVS_EX_DOUBLEBUFFER|LVS_EX_GRIDLINES; //|LVS_EX_TRACKSELECT;// LVS_EX_INFOTIP
        ListCtrl.SetExtendedStyle(dwExStyle);
        m_ctlHeader.SubclassWindow(ListCtrl.GetHeaderCtrl()->GetSafeHwnd());

        LV_COLUMN lvc;
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
        for(int i = 0; i< _countof(columnInfos); i++)
        {
            lvc.iSubItem = i;
            lvc.pszText = (LPTSTR)columnInfos[i].pszColumnText;
            lvc.cx = columnInfos[i].nColumnWidth;
            lvc.fmt = LVCFMT_LEFT;
            ListCtrl.InsertColumn(i,&lvc);
        }
        m_bInited = TRUE;
    }
    CLogViewerDoc* pDoc = GetDocument();
    LONG lLogItemCount = pDoc->m_FTLogManager.GetDisplayLogItemCount();
    ListCtrl.SetItemCount(lLogItemCount);
#if 0
    ListCtrl.DeleteAllItems();

    LV_ITEM lvi = {0};
    int itemIndex = 0;
    CString strFormat;
    for (LONG index = 0; index < lLogItemCount; index++)
    {
        LPLogItem pLogItem = pDoc->m_FTLogManager.GetLogItem(index);
        lvi.mask = LVIF_TEXT;// | LVIF_STATE;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        strFormat.Format(TEXT("%d"),pLogItem->lSeqNum); 
        lvi.pszText = (LPTSTR)(LPCTSTR)strFormat;
        itemIndex = ListCtrl.InsertItem(&lvi);
        ASSERT(itemIndex == index);

        strFormat.Format(TEXT("%d"),pLogItem->lThreadId);
        API_VERIFY(ListCtrl.SetItemText(itemIndex,type_ThreadId,strFormat));

        if (pLogItem->stTime.dwHighDateTime != 0 && pLogItem->stTime.dwLowDateTime != 0)
        {
            CTime time(pLogItem->stTime);
            API_VERIFY(ListCtrl.SetItemText(itemIndex,type_Time,time.Format(TEXT("%H:%M:%S"))));
        }
        else
        {
            API_VERIFY(ListCtrl.SetItemText(itemIndex,type_Time,TEXT("<NoTime>")));
        }

        API_VERIFY(ListCtrl.SetItemText(itemIndex,type_TraceInfo,pLogItem->pszTraceInfo));
        API_VERIFY(ListCtrl.SetItemData(itemIndex,(DWORD_PTR)pLogItem));
    }
#endif
}

void CLogItemView::OnHdnItemclickListAllLogitems(NMHDR *pNMHDR, LRESULT *pResult)
{
    //LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);

    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    const LogItemContentType sortType = (LogItemContentType)pNMListView->iItem;

    // if it's a second click on the same column then reverse the sort order,
    // otherwise sort the new column in ascending order.
    Sort( sortType, sortType == m_SortContentType ? !m_bSortAscending : TRUE );
    *pResult = 0;
}

//int CALLBACK CLogItemView::CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData )
//{
//    CLogItemView* pThis = reinterpret_cast<CLogItemView*>( lParamData );
//
//    LPLogItem pLogItem1 = LPLogItem(lParam1);
//    LPLogItem pLogItem2 = LPLogItem(lParam2);
//
//    int nResult = 0;
//    switch (pThis->m_SortContentType)
//    {
//    case type_Sequence:
//        nResult = pLogItem1->lSeqNum - pLogItem2->lSeqNum;
//        break;
//    case type_ThreadId:
//        nResult = pLogItem1->lThreadId - pLogItem2->lThreadId;
//        break;
//    case type_Time:
//        {
//            nResult = pLogItem1->stTime.dwHighDateTime - pLogItem2->stTime.dwHighDateTime;
//            if(0 == nResult)
//            {
//                nResult = pLogItem1->stTime.dwLowDateTime - pLogItem2->stTime.dwLowDateTime;
//            }
//        }
//        break;
//    case type_TraceLevel:
//        nResult = pLogItem1->level - pLogItem2->level;
//        break;
//    case type_TraceInfo:
//        nResult = _tcscmp(pLogItem1->pszTraceInfo,pLogItem2->pszTraceInfo);
//        break;
//    default:
//        ASSERT(FALSE);
//        break;
//    }
//
//    if (pThis->m_bSortAscending)
//    {
//        return nResult;
//    }
//    else
//    {
//        return -nResult;
//    }
//}

void CLogItemView::Sort(LogItemContentType contentType, BOOL bAscending )
{
    m_SortContentType = contentType;
    m_bSortAscending = bAscending;
    m_ctlHeader.SetSortArrow(m_SortContentType,m_bSortAscending);
    GetDocument()->m_FTLogManager.SortDisplayItem(m_SortContentType,bAscending);
    Invalidate();
    //VERIFY( GetListCtrl().SortItems( CompareFunction, reinterpret_cast<DWORD>( this ) ) );
}

void CLogItemView::GetDispInfo(LVITEM* pItem)
{
    BOOL bRet = FALSE;
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    // called when the listview needs to display data

    //类似的有 LVIF_STATE(状态), LVIF_IMAGE(图像), LVIF_INDENT, LVIF_PARAM 
    if(pItem->mask & LVIF_TEXT) 
    {
		FTL::CFConversion conv;
        CString strFormat;
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(pItem->iItem);
        if (!pLogItem)
        {
            return ;
        }
        switch(pItem->iSubItem)
        {
        case type_Sequence:
            strFormat.Format(TEXT("%d"),pLogItem->seqNum);
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_ProcessId:
            strFormat = conv.UTF8_TO_TCHAR(pLogItem->processId.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_ThreadId:
			strFormat = conv.UTF8_TO_TCHAR(pLogItem->threadId.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_Time:
            if (pLogItem->time != 0)
            {
                int microSec = 0;
                SYSTEMTIME st = {0};
                if(pLogItem->time > MIN_TIME_WITH_DAY_INFO){
                    //带日期的时间
                    //FILETIME localFileTime = {0};
                    FILETIME tm;
                    tm.dwHighDateTime = HILONG(pLogItem->time);//(pLogItem->time & 0xFFFFFFFF00000000) >> 32;
                    tm.dwLowDateTime = LOLONG(pLogItem->time);// ( pLogItem->time & 0xFFFFFFFF);
                    API_VERIFY(FileTimeToSystemTime(&tm,&st));
                    strFormat.Format(TEXT("%4d-%02d-%02d %02d:%02d:%02d:%03d"),
                        st.wYear, st.wMonth, st.wDay,
                        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

                }else{
                    microSec = pLogItem->time % TIME_RESULT_TO_MILLISECOND;
                    LONGLONG tmpTime = pLogItem->time / TIME_RESULT_TO_MILLISECOND;
                    st.wSecond = tmpTime % 60;
                    tmpTime /= 60;
                    st.wMinute = tmpTime % 60;
                    st.wHour = (WORD)tmpTime / 60;
                    strFormat.Format(TEXT("%02d:%02d:%02d:%03d"),
                        st.wHour, st.wMinute, st.wSecond, microSec);
                }

                StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            }
            else
            {
                StringCchCopy(pItem->pszText,pItem->cchTextMax -1,TEXT("<NoTime>"));
            }
            break;
        case type_ElapseTime:
			{
				strFormat.Format(TEXT("%.3f s"), (double)pLogItem->elapseTime * 100 / NANOSECOND_PER_SECOND);
				StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
			}
            break;
        case type_ModuleName:
			if (NULL != pLogItem->pszModuleName)
			{
				StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)pLogItem->pszModuleName);
			}
            break;
		case type_FunName:
			{
				if (NULL != pLogItem->pszFunName)
				{
					StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)pLogItem->pszFunName);
				}
				break;
			}
        case type_FileName:
            {
                if (NULL != pLogItem->pszSrcFileName)
                {
                    strFormat.Format(TEXT("%s(%d)"), pLogItem->pszSrcFileName, pLogItem->srcFileline);
                    StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)strFormat);
                }
                break;
            }
        case type_TraceLevel:
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,pszTraceLevel[pLogItem->level]);
            break;
        case type_TraceInfo:
			if (NULL != pLogItem->pszTraceInfo){
				StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)pLogItem->pszTraceInfo);
			}
            break;
        default:
            ASSERT(FALSE);
            break;
        }
    }
}

int CLogItemView::FindItem(int /*iStart*/, LVFINDINFO* /*plvfi*/)
{
    //当找到要搜索的数据后，应该把该数据的索引（行号）返回，如果没有找到，则返回-1
    FTLASSERT(FALSE);
    return -1;
}

//鼠标点击消息处理中 检查鼠标是否单击在 check box 上面了?
//void CLogItemView::OnClickList(NMHDR* pNMHDR, LRESULT* pResult){   ... 
//     hitinfo.pt = pNMListView->ptAction;
//     int item = m_list.HitTest(&hitinfo);
//     if( (hitinfo.flags & LVHT_ONITEMSTATEICON) != 0){ ... } 
//}

BOOL CLogItemView::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    //如果直接使用CListCtrl，应该在对话框中响应这三个消息。
    //如果使用CListCtrl派生类，可以在派生类中响应这三个消息的反射消息
    //  ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
    if(message == WM_NOTIFY)
    {
        NMHDR* phdr = (NMHDR*)lParam;

        // these 3 notifications are only sent by virtual listviews
        switch(phdr->code)
        {
        case LVN_GETDISPINFO:   //需要数据的时候
            {
                NMLVDISPINFO* pLvdi;
                pLvdi = (NMLVDISPINFO*)lParam;
                GetDispInfo(&pLvdi->item);
            }
            if(pResult != NULL)
            {
                *pResult = 0;
            }
            break;
        case LVN_ODCACHEHINT:   //批量读取并缓冲数据(如从DB或网络等比较慢的地方读取数据时)
            {
                NMLVCACHEHINT* pHint = (NMLVCACHEHINT*)lParam;
                PrepareCache(pHint->iFrom, pHint->iTo);
            }
            if(pResult != NULL)
            {
                *pResult = 0;
            }
            break;
        case LVN_ODFINDITEM:    //用户试图查找某个元素的时候(如资源管理器中 直接输入字母定位)
            {
                NMLVFINDITEM* pFindItem = (NMLVFINDITEM*)lParam;
                int i = FindItem(pFindItem->iStart, &pFindItem->lvfi);
                if(pResult != NULL)
                {
                    *pResult = i;
                }
            }
            break;
        default:
            return CListView::OnChildNotify(message, wParam, lParam, pResult);
        }
    }
    else
    {
        return CListView::OnChildNotify(message, wParam, lParam, pResult);
    }
    return TRUE;

}
void CLogItemView::OnUpdate(CView* pSender, LPARAM /*lHint*/, CObject* /*pHint*/)
{
    if (pSender != this)
    {
        CLogViewerDoc* pDoc = GetDocument();
        LONG lLogItemCount = pDoc->m_FTLogManager.GetDisplayLogItemCount();
        CListCtrl& ListCtrl = GetListCtrl();
        ListCtrl.SetItemCount(lLogItemCount);
    }
}

void CLogItemView::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pResult);

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	UNREFERENCED_PARAMETER(pNMListView);

	TRACE(TEXT("click at iItem = %d,iSubItem=%d\n"), pNMListView->iItem, pNMListView->iSubItem);
}

void CLogItemView::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
    //LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
//     CString strFormat;
//     strFormat.Format(TEXT("iItem = %d,iSubItem=%d"),
//         pNMListView->iItem, pNMListView->iSubItem);
//     AfxGetMainWnd()->SetWindowText(strFormat);
    if (pNMListView->iItem >= 0)
    {
        CString strFileName;
        int line = 0;
        LogItemPointer pLogItem = GetDocument()->m_FTLogManager.GetDisplayLogItem(pNMListView->iItem);

        if (pLogItem->pszSrcFileName != NULL && pLogItem->srcFileline != 0)
        {
            strFileName = pLogItem->pszSrcFileName;
            line = pLogItem->srcFileline;
        }
        else
        {
            CString strTraceInfo(pLogItem->pszTraceInfo);

            int leftBraPos = strTraceInfo.Find(TEXT('(')); //左括号
            int rightBraPos = strTraceInfo.Find(TEXT(')'));//右括号
            if (rightBraPos != -1)
            {
                int semPos = strTraceInfo.Find(TEXT(':'),rightBraPos);
                if (-1 != semPos)  //找到分号，可能带有文件名和路径
                {
                    strTraceInfo.SetAt(semPos,TEXT('\0'));
                    if (-1 != leftBraPos && -1 != rightBraPos && rightBraPos > leftBraPos)  
                    {
                        strFileName = strTraceInfo.Left(leftBraPos);
                        CString strLine = strTraceInfo.Mid(leftBraPos+1 , rightBraPos - leftBraPos - 1);
                        line = _ttoi(strLine);
                    }
                }
            }
        }
        if (!strFileName.IsEmpty() && line != 0)
        {
            TCHAR szPathFull[MAX_PATH] = {0};
            if (PathIsRelative(strFileName))
            {
                if (m_strFolderPath.IsEmpty())
                {
                    CFDirBrowser dirBrowser(TEXT("Choose Project Source Path(relative to the source file)"), m_hWnd);
                    if (dirBrowser.DoModal())
                    {
                        m_strFolderPath = dirBrowser.GetSelectPath();
                    }
                }
                PathCombine(szPathFull, m_strFolderPath, strFileName);
                //PathCombine(szPathFull, _T("E:\\Code\\EPubViewer\\HostApp\\Release\\.."), strFileName);
            }
            else
            {
                StringCchCopy(szPathFull, _countof(szPathFull), strFileName);
            }
            CString path(szPathFull);
            path.Replace(_T('/'), _T('\\'));
            GetDocument()->GoToLineInSourceCode(path,line);
        }
     }
    *pResult = 0;
}

void CLogItemView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	//if(KEY_DOWN(VK_CONTROL))
	{
		CMenu menuDetails;
		BOOL bRet = FALSE;
		API_VERIFY(menuDetails.LoadMenu(IDR_MENU_DETAIL));
		CMenu* pThreadMenu = menuDetails.GetSubMenu(0);
		FTLASSERT(pThreadMenu);
		m_ptContextMenuClick = point;
		API_VERIFY(pThreadMenu->TrackPopupMenu(TPM_TOPALIGN|TPM_LEFTBUTTON,point.x,point.y,pWnd));
	}
}

void CLogItemView::OnDetailsHighLightSameThread()
{
	CLogManager& logManager = GetDocument()->m_FTLogManager;
	CListCtrl& ListCtrl = GetListCtrl();

	POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		int nItem = ListCtrl.GetNextSelectedItem(pos);
		LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
		if (pLogItem)
		{
			_HighlightSameThread(pLogItem->threadId);
		}
	}
}

LVHITTESTINFO CLogItemView::GetCurrentSelectInfo()
{
    //CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    LVHITTESTINFO lvHistTestInfo = {0};
    lvHistTestInfo.pt = m_ptContextMenuClick;
    ListCtrl.ScreenToClient(&lvHistTestInfo.pt);
    ListCtrl.SubItemHitTest(&lvHistTestInfo);
    //ListCtrl.HitTest(&lvHistTestInfo);
    
    return lvHistTestInfo;
}
void CLogItemView::OnDetailsCopyItemText()
{
	BOOL bRet = FALSE;
    CString strText;

    CListCtrl& listCtrl = GetListCtrl();
    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
	if (lvHistTestInfo.iItem != -1 && lvHistTestInfo.iSubItem != -1)
	{
		strText = listCtrl.GetItemText(lvHistTestInfo.iItem, lvHistTestInfo.iSubItem);
		if(!strText.IsEmpty())
		{
			API_VERIFY(CFSystemUtil::CopyTextToClipboard(strText)); //, m_hWnd);
		}
	}
    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR, 
            TEXT("pos=[%d,%d], text=%s, Last Error=%d"), 
            lvHistTestInfo.iItem, lvHistTestInfo.iSubItem, strText, GetLastError());
    }
}

void CLogItemView::OnDetailsCopyLineText()
{
    BOOL bRet = FALSE;
    CString strLineText, strItemText;

    CListCtrl& listCtrl = GetListCtrl();
    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
    if (lvHistTestInfo.iItem != -1)
    {
        int columnCount = _countof(columnInfos);
        for (int i = 0; i < columnCount; i++)
        {
            strItemText = listCtrl.GetItemText(lvHistTestInfo.iItem, i);
            strLineText += strItemText;
            if (i != columnCount - 1)
            {
                strLineText += TEXT(",");
            }
        }
        API_VERIFY(CFSystemUtil::CopyTextToClipboard(strLineText)); //, m_hWnd);
    }
    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR, 
            TEXT("pos=[%d,%d], text=%s, Last Error=%d"), 
            lvHistTestInfo.iItem, lvHistTestInfo.iSubItem, strLineText, GetLastError());
    }
}

void CLogItemView::OnDetailsCopyFullLog()
{
    BOOL bRet = FALSE;
    CString strText;

    //CListCtrl& listCtrl = GetListCtrl();
    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
    if (lvHistTestInfo.iItem != -1)
    {
        const LogItemPointer pLogItem = GetDocument()->m_FTLogManager.GetDisplayLogItem(lvHistTestInfo.iItem);
        if (pLogItem && pLogItem->pszFullLog != NULL)
        {
            API_VERIFY(CFSystemUtil::CopyTextToClipboard(pLogItem->pszFullLog)); //, m_hWnd);
        }
    }
    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR, 
            TEXT("pos=[%d,%d], text=%s, Last Error=%d"), 
            lvHistTestInfo.iItem, lvHistTestInfo.iSubItem, strText, GetLastError());
    }
}

void CLogItemView::_HighlightSameThread(THREAD_ID_TYPE threadId)
{
	CLogManager& logManager = GetDocument()->m_FTLogManager;
	CListCtrl& ListCtrl = GetListCtrl();

	int nTotalCount = ListCtrl.GetItemCount();
	for (int nItem = 0; nItem < nTotalCount; nItem++)
	{
		LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
		FTLASSERT(pLogItem);
		if (pLogItem && pLogItem->threadId == threadId)
		{
			ListCtrl.SetItemState(nItem,  LVIS_SELECTED, LVIS_SELECTED);
			ListCtrl.SetSelectionMark(nItem);
		}
	}
}

BOOL CLogItemView::OnEraseBkgnd(CDC* pDC)
{
//减少闪烁
   return __super::OnEraseBkgnd(pDC);
   //return TRUE;
}


//void CLogItemView::OnPaint()
//{
//    //响应WM_PAINT消息
//    CPaintDC dc(this); // device context for painting
//    CRect rect;
//    CRect headerRect;
//    CDC MenDC;//内存ID表  
//    CBitmap MemMap;
//    GetClientRect(&rect);   
//    GetDlgItem(0)->GetWindowRect(&headerRect);  
//    MenDC.CreateCompatibleDC(&dc);  
//    MemMap.CreateCompatibleBitmap(&dc,rect.Width(),rect.Height());
//    MenDC.SelectObject(&MemMap);
//    MenDC.FillSolidRect(&rect,RGB(228,236,243));  
//
//    //这一句是调用默认的OnPaint(),把图形画在内存DC表上  
//    DefWindowProc(WM_PAINT,(WPARAM)MenDC.m_hDC,(LPARAM)0);      
//    //输出  
//    dc.BitBlt(0,headerRect.Height(),rect.Width(),  rect.Height(),&MenDC,0, headerRect.Height(),SRCCOPY);  
//    MenDC.DeleteDC();
//    MemMap.DeleteObject();
//}


void CLogItemView::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	ATLTRACE(TEXT("OnLvnItemchanged, iItem=%d,iSubItem=%d, uNewState=%d, uOldState=%d\n"), 
		pNMLV->iItem, pNMLV->iSubItem, pNMLV->uNewState, pNMLV->uOldState);

	//会响应三次
	if (pNMLV->iItem >= 0 
		&& ((pNMLV->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))!= 0) )
	{
		GetDocument()->m_FTLogManager.setActiveItemIndex(pNMLV->iItem);
		GetDocument()->UpdateAllViews(this);
	}
}
