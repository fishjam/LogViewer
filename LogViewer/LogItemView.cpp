// LogItemView.cpp : 实现文件
//

#include "stdafx.h"
#include "LogViewer.h"
#include "LogItemView.h"
#include "LogManager.h"
#include "MachinePidTidTreeView.h"
#include "DialogSourceHistory.h"
#include "DialogGoTo.h"
#include "DialogSameFilesList.h"

#include <ftlShell.h>
#include <regex>

struct strColumnInfo
{
    LPCTSTR pszColumnText;
    int		nColumnWidth;
};

static strColumnInfo	columnInfos[] = 
{
    {TEXT("Line"), 50,},
    {TEXT("Seq"), 50,},
    {TEXT("Machine"), 50,},
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
    m_SortContentType = type_LineNum;
    m_bSortAscending = TRUE;
    m_dwDefaultStyle |= ( LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA );
    m_ptContextMenuClick.SetPoint(-1, -1);
    m_nLastGotToLineNumber = 0L;
}

CLogItemView::~CLogItemView()
{
}

BEGIN_MESSAGE_MAP(CLogItemView, CListView)
	ON_COMMAND_EX(ID_EDIT_GOTO, &CLogItemView::OnEditGoTo)
	ON_COMMAND_EX(ID_EDIT_CLEAR_CACHE, &CLogItemView::OnEditClearCache)
    ON_COMMAND_EX(ID_EDIT_SET_SRC_PATHS, &CLogItemView::OnEditSetSrcPaths)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CLogItemView::OnHdnItemclickListAllLogitems)
    //ON_NOTIFY_RANGE(LVN_COLUMNCLICK,0,0xffff,OnColumnClick)
    //ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
    ON_NOTIFY_REFLECT(NM_CLICK, &CLogItemView::OnNMClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, &CLogItemView::OnNMDblclk)
    ON_COMMAND(ID_DETAILS_HIGHLIGHT_SAME_THREAD, &CLogItemView::OnDetailsHighLightSameThread)
    ON_COMMAND(ID_DETAILS_COPY_ITEM_TEXT, &CLogItemView::OnDetailsCopyItemText)
    ON_COMMAND(ID_DETAILS_COPY_LINE_TEXT, &CLogItemView::OnDetailsCopyLineText)
    ON_COMMAND(ID_DETAILS_COPY_FULL_LOG, &CLogItemView::OnDetailsCopyFullLog)
    ON_COMMAND(ID_DETAILS_DELETE_SELECT_ITEMS, &CLogItemView::OnDetailDeleteSelectItems)
    ON_COMMAND(ID_DETAILS_SELECT_CURRENT_PID, &CLogItemView::OnDetailSelectCurrentPid)
    ON_COMMAND(ID_DETAILS_SELECT_CURRENT_TID, &CLogItemView::OnDetailSelectCurrentTid)
    ON_WM_CONTEXTMENU()
    ON_WM_ERASEBKGND()
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CLogItemView::OnLvnItemchanged)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_SELECTED_LOGITEM, &CLogItemView::OnUpdateIndicatorSelectedLogItem)
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
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    UINT nPreSelectItemLineIndex = _GetCurrentFirstSelectLine();
    FTLTRACE("before sort, previous select line index = %d", nPreSelectItemLineIndex);

    m_SortContentType = contentType;
    m_bSortAscending = bAscending;
    m_ctlHeader.SetSortArrow(m_SortContentType,m_bSortAscending);
    logManager.SortDisplayItem(m_SortContentType,bAscending);
    
    _GotoSpecialLine(nPreSelectItemLineIndex);
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
        case type_LineNum:
            strFormat.Format(TEXT("%d"), pLogItem->lineNum);
            StringCchCopy(pItem->pszText, pItem->cchTextMax - 1, (LPCTSTR)strFormat);
            break;
        case type_SeqNum:
            strFormat.Format(TEXT("%d"),pLogItem->seqNum);
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_Machine:
            //strFormat = conv.UTF8_TO_TCHAR(pLogItem->machine.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, pLogItem->machine.c_str());
            break;
        case type_ProcessId:
            //strFormat = conv.UTF8_TO_TCHAR(pLogItem->processId.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, pLogItem->processId.c_str());
            break;
        case type_ThreadId:
            //strFormat = conv.UTF8_TO_TCHAR(pLogItem->threadId.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, pLogItem->threadId.c_str());
            break;
        case type_Time:
            if (pLogItem->time != 0)
            {
                strFormat = logManager.FormatDateTime(pLogItem->time, logManager.m_logConfig.m_dateTimeType);
                StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            }
            else
            {
                StringCchCopy(pItem->pszText,pItem->cchTextMax -1,TEXT("<NoTime>"));
            }
            break;
        case type_ElapseTime:
            {
                strFormat = logManager.FormatElapseTime(pLogItem->elapseTime, logManager.m_logConfig.m_dateTimeType);
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
        case type_FilePos:
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
void CLogItemView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    //if (pSender != this)
    {
        CLogViewerDoc* pDoc = GetDocument();
        CLogManager& rLogManager = pDoc->m_FTLogManager;
        CListCtrl& ListCtrl = GetListCtrl();

        LONG lLogItemCount = rLogManager.GetDisplayLogItemCount();
        ListCtrl.SetItemCount(lLogItemCount);
        FTLTRACE(TEXT("CLogItemView::OnUpdate, logItemCount=%ld"), lLogItemCount);

        //TODO: is there better solution?
        _GotoSpecialLine(rLogManager.GetActiveLineIndex());
    }
}

void CLogItemView::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    UNREFERENCED_PARAMETER(pResult);

    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    UNREFERENCED_PARAMETER(pNMListView);

    FTLTRACE(TEXT("click at iItem = %d,iSubItem=%d"), pNMListView->iItem, pNMListView->iSubItem);
}

void CLogItemView::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
    BOOL bRet = FALSE;
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
        CLogManager& rLogManager = GetDocument()->m_FTLogManager;
        LogItemPointer pLogItem = rLogManager.GetDisplayLogItem(pNMListView->iItem);

        //找源码对应的文件名和行号
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
                        CString strLine = strTraceInfo.Mid(leftBraPos + 1, rightBraPos - leftBraPos - 1);
                        line = _ttoi(strLine);

                        strFileName = strTraceInfo.Left(leftBraPos);
                        rLogManager.TryReparseRealFileName(strFileName);
                    }
                }
            }
        }

        //如果找到文件名和行号,可以尝试通过源码定位
        if (!strFileName.IsEmpty() && line != 0)
        {
			TCHAR szPathFull[MAX_PATH] = { 0 };

			//优先在缓存里面查找(主要是减少 重复文件时的选择问题), 如果有, 则直接使用, 否则再走后面的逻辑
			CString strFileLineCache;
			strFileLineCache.Format(TEXT("%s:%d"), strFileName, line);
			CString strFullPathFromUserCache = rLogManager.GetFullPathFromUserCache(strFileLineCache);
			if (!strFullPathFromUserCache.IsEmpty())
			{
				StringCchCopy(szPathFull, _countof(szPathFull), strFullPathFromUserCache);
			}
			else {
				if (PathIsRelative(strFileName))
				{
					if (rLogManager.NeedScanSourceFiles())
					{
						//CString strExistSourceDir = AfxGetApp()->GetProfileString(SECTION_CONFIG, ENTRY_SOURCE_DIR);
                        CDialogSourceHistory dlgSourceHistory(this);
						//CFDirBrowser dirBrowser(TEXT("Choose Project Source Root Path"), m_hWnd, strExistSourceDir);
						if (dlgSourceHistory.DoModal())
						{
                            const CStringArray& selectedPaths = dlgSourceHistory.GetSelectPaths();
							rLogManager.ScanSourceFiles(selectedPaths);
						}
					}

					SameNameFilePathListPtr spFilePathList = rLogManager.FindFileFullPath(ATLPath::FindFileName(strFileName));
					if (spFilePathList != nullptr)
					{
						int nSameFileNameCount = (int)spFilePathList->size();
						if (nSameFileNameCount > 1)
						{
							FTLTRACEEX(FTL::tlWarn, TEXT("find %d source files with %s"), nSameFileNameCount, strFileName);
							CDialogSameFilesList dlgSameFilesList(spFilePathList);
							if (IDOK == dlgSameFilesList.DoModal())
							{
								StringCchCopy(szPathFull, _countof(szPathFull), dlgSameFilesList.GetSelectFilePath());
								rLogManager.SetFullPathForUserCache(strFileLineCache, szPathFull);
							}
						}
						if (szPathFull[0] == TEXT('\0'))
						{
							StringCchCopy(szPathFull, _countof(szPathFull), spFilePathList->front());
						}
						//TODO: if there are more than one file with same name, then prompt user choose
                    }
                    else {
                        // can not find file in special folder
                    }
				}
				else
				{
					StringCchCopy(szPathFull, _countof(szPathFull), strFileName);
				}
			}

            CString path(szPathFull);
            if (!path.IsEmpty())
            {
                path.Replace(_T('/'), _T('\\'));
                API_VERIFY(GetDocument()->GoToLineInSourceCode(path, line));
            }
            else {
                FTL::FormatMessageBox(m_hWnd, TEXT("Find file failed"), MB_OK,
                    TEXT("Can not find source file \"%s\" "),
                    strFileName);
            }
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
        CMenu* pDetailMenu = menuDetails.GetSubMenu(0);
        FTLASSERT(pDetailMenu);
        m_ptContextMenuClick = point;
#if ENABLE_COPY_FULL_LOG
        pDetailMenu->EnableMenuItem(ID_DETAILS_COPY_FULL_LOG, MF_ENABLED | MF_BYCOMMAND);
#else 
        pDetailMenu->EnableMenuItem(ID_DETAILS_COPY_FULL_LOG, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
#endif
        API_VERIFY(pDetailMenu->TrackPopupMenu(TPM_TOPALIGN|TPM_LEFTBUTTON,point.x,point.y,pWnd));
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
            _HighlightSameThread(pLogItem);
        }
    }
}

LVHITTESTINFO CLogItemView::GetCurrentSelectInfo()
{
    //CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    LVHITTESTINFO lvHistTestInfo = { 0 };
    lvHistTestInfo.pt = m_ptContextMenuClick;
    ListCtrl.ScreenToClient(&lvHistTestInfo.pt);
    ListCtrl.SubItemHitTest(&lvHistTestInfo);
    //ListCtrl.HitTest(&lvHistTestInfo);

    return lvHistTestInfo;
}

LONG CLogItemView::_GetSelectedLines(LogIndexContainer& selectedItemsList, INT& nSelectSubItem)
{
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();
    LONG nSelectedCount = 0;

    LVHITTESTINFO lvHistTestInfo = { 0 };
    lvHistTestInfo.pt = m_ptContextMenuClick;
    ListCtrl.ScreenToClient(&lvHistTestInfo.pt);
    ListCtrl.SubItemHitTest(&lvHistTestInfo);

    nSelectSubItem = lvHistTestInfo.iSubItem;

    POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
    while (pos != NULL)
    {
        int nSelectItem = ListCtrl.GetNextSelectedItem(pos);
        selectedItemsList.push_back(nSelectItem);
        nSelectedCount++;
    }
    return nSelectedCount;
}

//TODO: textType: 0(item), 1(line), 2(full)
CString CLogItemView::_GetSelectedText(int textType)
{
    CListCtrl& listCtrl = GetListCtrl();

    FTL::CFStringFormater strFormater;
    LogIndexContainer selectedItemList;
    INT nSelectedSubItem = -1;
    LONG nSelectCount = _GetSelectedLines(selectedItemList, nSelectedSubItem);

    if (nSelectCount > 0 && nSelectedSubItem != -1)
    {
        for (LogIndexContainerIter iter = selectedItemList.begin(); iter != selectedItemList.end(); ++iter)
        {
            CString strText;
            switch (textType)
            {
                case 0:  //item
                    strText = listCtrl.GetItemText(*iter, nSelectedSubItem);
                    break;
                case 1: //line
                {
                    int columnCount = _countof(columnInfos);
                    for (int i = 0; i < columnCount; i++)
                    {
                        CString strItemText = listCtrl.GetItemText(*iter, i);
                        if (strText.IsEmpty())
                        {
                            //first column
                            strText.AppendFormat(TEXT("%s"), strItemText);
                        }
                        else {
                            strText.AppendFormat(TEXT(",%s"), strItemText);
                        }
                    }
                    break;
                }
                case 2: //full(TODO)
                    break;
            }

            if (NULL == strFormater.GetString())
            {
                //first line
                strFormater.AppendFormat(TEXT("%s"), strText);
            }
            else {
                strFormater.AppendFormat(TEXT("\r\n%s"), strText);
            }
        }
    }
    return strFormater.GetString();
}

void CLogItemView::OnDetailsCopyItemText()
{
    BOOL bRet = FALSE;
    CString strFullText = _GetSelectedText(0);

    if(!strFullText.IsEmpty())
    {
        API_VERIFY(CFSystemUtil::CopyTextToClipboard(strFullText)); //, m_hWnd);
    }

    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR,
            TEXT("CopyTextToClipboard, Error=%d"), GetLastError());
    }
}

void CLogItemView::OnDetailsCopyLineText()
{
    BOOL bRet = FALSE;
    CString strFullText = _GetSelectedText(1);

    if (!strFullText.IsEmpty())
    {
        API_VERIFY(CFSystemUtil::CopyTextToClipboard(strFullText)); //, m_hWnd);
    }

    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR,
            TEXT("CopyTextToClipboard, Error=%d"), GetLastError());
    }
}

void CLogItemView::OnDetailsCopyFullLog()
{
#if ENABLE_COPY_FULL_LOG
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
#endif
}

void CLogItemView::OnDetailDeleteSelectItems() {
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    int nSelectedCount = ListCtrl.GetSelectedCount();
    if (nSelectedCount <= 0)
    {
        return;
    }

    if (nSelectedCount > 1)
    {
        if (FTL::FormatMessageBox(m_hWnd, TEXT("Confirm"), MB_OKCANCEL
            , TEXT("Do you want delete %d log items"), nSelectedCount) != IDOK)
        {
            return;
        }
    }

    int nSelectItem = -1;
    std::set<LONG> delItemsLineNum;
    std::list<INT> delItemsIndex;
    POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
    while (pos != NULL)
    {
        nSelectItem = ListCtrl.GetNextSelectedItem(pos);
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(nSelectItem);
        if (pLogItem)
        {
            delItemsLineNum.insert(pLogItem->lineNum);
            delItemsIndex.push_back(nSelectItem);
            //TRACE("will delete seq: %ld\n", pLogItem->lineNum);
        }
    }
    if (!delItemsLineNum.empty())
    {
        logManager.DeleteItems(delItemsLineNum);
        
        //clear select(is there better way?)
        std::list<INT>::iterator iter = delItemsIndex.begin();
        for ( iter++;  //skip the first select
            iter != delItemsIndex.end(); ++iter)
        {
            ListCtrl.SetItemState(*iter, 0, LVIS_SELECTED | LVIS_FOCUSED);
        }
        Invalidate();
    }
}

void CLogItemView::_HighlightSameThread(LogItemPointer pCompareLogItem)
{
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    int nTotalCount = ListCtrl.GetItemCount();
    for (int nItem = 0; nItem < nTotalCount; nItem++)
    {
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
        FTLASSERT(pLogItem);
        if (pLogItem) {
            if (pLogItem->machine == pCompareLogItem->machine
                && pLogItem->processId == pCompareLogItem->processId
                && pLogItem->threadId == pCompareLogItem->threadId){
                ListCtrl.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
                ListCtrl.SetSelectionMark(nItem);
            }
        }
    }
}

int CLogItemView::_GetSelectedIdTypeValue(MachinePIdTIdType& idType) {
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
    if (lvHistTestInfo.iItem != -1)
    {
        const LogItemPointer pLogItem = logManager.GetDisplayLogItem(lvHistTestInfo.iItem);
        if (pLogItem) {
            idType.machine = pLogItem->machine;
            idType.pid = pLogItem->processId;
            idType.tid = pLogItem->threadId;
        }
    }
    return lvHistTestInfo.iItem;
}

void CLogItemView::OnDetailSelectCurrentPid() {
    int oldSelectIndex = 0;
    MachinePIdTIdType selectedIdType;

    if(-1 != (oldSelectIndex = _GetSelectedIdTypeValue(selectedIdType))) {
        CLogManager& logManager = GetDocument()->m_FTLogManager;

        UINT nCurSelectLine = _GetCurrentFirstSelectLine();
        logManager.OnlySelectSpecialItems(selectedIdType, ONLY_SELECT_TYPE::ostProcessId);
        GetDocument()->UpdateAllViews(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_PID, (CObject*)&selectedIdType);
        OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_PID, (CObject*)&selectedIdType);
        _GotoSpecialLine(nCurSelectLine);
    }
}

void CLogItemView::OnDetailSelectCurrentTid() {
    int oldSelectIndex = 0;
    MachinePIdTIdType selectedIdType;
    if (-1 != (oldSelectIndex = _GetSelectedIdTypeValue(selectedIdType))) {
        CLogManager& logManager = GetDocument()->m_FTLogManager;

        UINT nCurSelectLine = _GetCurrentFirstSelectLine();
        logManager.OnlySelectSpecialItems(selectedIdType, ONLY_SELECT_TYPE::ostThreadId);
        GetDocument()->UpdateAllViews(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_TID, (CObject*)&selectedIdType);
        OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_TID, (CObject*)&selectedIdType);
        _GotoSpecialLine(nCurSelectLine);
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
    CLogManager& rLogManager = GetDocument()->m_FTLogManager;

    // TODO: Add your control notification handler code here
    *pResult = 0;
    FTLTRACE(TEXT("OnLvnItemchanged, iItem=%d,iSubItem=%d, uNewState=%d, uOldState=%d"), 
        pNMLV->iItem, pNMLV->iSubItem, pNMLV->uNewState, pNMLV->uOldState);

    //会响应三次
    if (pNMLV->iItem >= 0 
        && ((pNMLV->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))!= 0) )
    {
        LONG lineIndex = INVALID_LINE_INDEX;
        LogItemPointer pLogItem = rLogManager.GetDisplayLogItem(pNMLV->iItem);
        if (pLogItem)
        {
            lineIndex = pLogItem->lineNum;
        }
        rLogManager.setActiveItemIndex(lineIndex, pNMLV->iItem);
        GetDocument()->UpdateAllViews(this);
    }
}

void CLogItemView::OnUpdateIndicatorSelectedLogItem(CCmdUI *pCmdUI)
{
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    UINT nFirstLineNum = _GetCurrentFirstSelectLine();
    UINT nSelectedCount = ListCtrl.GetSelectedCount();

    CString strFormat;
    strFormat.Format(ID_INDICATOR_SELECTED_LOGITEM, nFirstLineNum, nSelectedCount);
    pCmdUI->SetText(strFormat);
}

UINT CLogItemView::_GetCurrentFirstSelectLine()
{
    CListCtrl& ListCtrl = GetListCtrl();
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    UINT nCurrentFirstSelectLine = INVALID_LINK_INDEX;
    POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
    if (pos != NULL)
    {
        int nItem = ListCtrl.GetNextSelectedItem(pos);
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
        if (pLogItem)
        {
            nCurrentFirstSelectLine = pLogItem->lineNum;
        }
    }
    return nCurrentFirstSelectLine;
}

BOOL CLogItemView::_GotoSpecialLine(UINT nGotoLineNumber) 
{
    FUNCTION_BLOCK_TRACE(10);
    BOOL bRet = FALSE;
    CListCtrl& ListCtrl = GetListCtrl();
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    if (INVALID_LINE_INDEX == nGotoLineNumber)
    {
        FTLASSERT(TEXT("invalid line index")); //example: user have not select any line before sort
		Invalidate();
        return FALSE;
    }

//     POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
//     while (pos != NULL)
//     {
//         int oldSelectItem = ListCtrl.GetNextSelectedItem(pos);
//         ListCtrl.SetItemState(oldSelectItem, 0, LVIS_SELECTED);
//     }
    //Invalidate();

    int nClosestItem = 0, nClosestItemLineNum = 0;

    //选择用户指定的
    SortContent sortContent = logManager.GetFirstSortContent();
    if (type_LineNum == sortContent.contentType && sortContent.bSortAscending
        && logManager.GetTotalLogItemCount() == logManager.GetDisplayLogItemCount()  //no filter
        )
    {
        nClosestItem = nGotoLineNumber - 1; //listCtrl 中是 0 基址的
        nClosestItemLineNum = nGotoLineNumber;
    }
    else {
        //需要遍历
        int nTotalCount = ListCtrl.GetItemCount();
        for (int nItem = 0; nItem < nTotalCount; nItem++)
        {
            LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
            FTLASSERT(pLogItem);
            if (pLogItem) {
                if (pLogItem->lineNum == nGotoLineNumber) {
                    //准确找到
                    nClosestItem = nItem;
                    nClosestItemLineNum = nGotoLineNumber;
                    break;
                }

                LONG diff1 = FTL_ABS((LONG)nGotoLineNumber - pLogItem->lineNum);
                LONG diff2 = FTL_ABS((LONG)nGotoLineNumber - nClosestItemLineNum);
                if (diff1 <= diff2)
                {
                    nClosestItem = nItem;
                    nClosestItemLineNum = pLogItem->lineNum;
                }
            }
        }
    }

    FTLTRACE(TEXT("try got %d, real %d, item index=%d"),
        nGotoLineNumber, nClosestItemLineNum, nClosestItem);
	if (nClosestItem > 0) {
		API_VERIFY(ListCtrl.EnsureVisible(nClosestItem, TRUE));
		ListCtrl.SetItemState(nClosestItem, LVIS_SELECTED, LVIS_SELECTED);
		ListCtrl.SetSelectionMark(nClosestItem);
	}
	Invalidate();

	return bRet;
}

BOOL CLogItemView::OnEditGoTo(UINT nID)
{
    BOOL bRet = TRUE;
    CDialogGoTo dlg(m_nLastGotToLineNumber, this);
    if (dlg.DoModal())
    {
        CLogManager& logManager = GetDocument()->m_FTLogManager;
        UINT nGotoLineNumber = dlg.GetGotoLineNumber();
        if (nGotoLineNumber >= 0 && nGotoLineNumber < (UINT)logManager.GetTotalLogItemCount())
        {
            API_VERIFY(_GotoSpecialLine(nGotoLineNumber));
            m_nLastGotToLineNumber = nGotoLineNumber;
        }
    }
    return bRet;
}

BOOL CLogItemView::OnEditClearCache(UINT nID)
{
	CLogManager& rLogManager = GetDocument()->m_FTLogManager;
    rLogManager.ClearUserFullPathCache();
	return TRUE;
}

BOOL CLogItemView::OnEditSetSrcPaths(UINT nID) 
{
    CLogManager& rLogManager = GetDocument()->m_FTLogManager;
    CDialogSourceHistory dlgSourceHistory(this);
    if (dlgSourceHistory.DoModal())
    {
        const CStringArray& selectedPaths = dlgSourceHistory.GetSelectPaths();
        //rLogManager.ScanSourceFiles(strFolderPath);
    }
    return TRUE;
}