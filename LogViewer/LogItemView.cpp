// LogItemView.cpp : ʵ���ļ�
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
    {TEXT("TraceInfoLen"), 20,},
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
    ON_COMMAND(ID_DETAILS_SELECT_CURRENT_FILE, &CLogItemView::OnDetailSelectCurrentFile)
    ON_COMMAND(ID_DETAILS_CLEAR_FILTER_BY_FILES, &CLogItemView::OnDetailClearFilterByFiles)
    
    ON_WM_CONTEXTMENU()
    ON_WM_ERASEBKGND()
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CLogItemView::OnLvnItemchanged)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_SELECTED_LOGITEM, &CLogItemView::OnUpdateIndicatorSelectedLogItem)
END_MESSAGE_MAP()


// CLogItemView ���

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


// CLogItemView ��Ϣ�������

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
        //TODO: ��Ȼ���� LVS_EX_TRACKSELECT �������ʾ��ǰѡ�е��У�������ʱ���Զ���������λ��ѡ��
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

    //���Ƶ��� LVIF_STATE(״̬), LVIF_IMAGE(ͼ��), LVIF_INDENT, LVIF_PARAM 
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
        case type_TraceInfoLen:
            strFormat.Format(TEXT("%d"), pLogItem->traceInfoLen);
            StringCchCopy(pItem->pszText, pItem->cchTextMax - 1, (LPCTSTR)strFormat);
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
    //���ҵ�Ҫ���������ݺ�Ӧ�ðѸ����ݵ��������кţ����أ����û���ҵ����򷵻�-1
    FTLASSERT(FALSE);
    return -1;
}

//�������Ϣ������ �������Ƿ񵥻��� check box ������?
//void CLogItemView::OnClickList(NMHDR* pNMHDR, LRESULT* pResult){   ... 
//     hitinfo.pt = pNMListView->ptAction;
//     int item = m_list.HitTest(&hitinfo);
//     if( (hitinfo.flags & LVHT_ONITEMSTATEICON) != 0){ ... } 
//}

BOOL CLogItemView::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    //���ֱ��ʹ��CListCtrl��Ӧ���ڶԻ�������Ӧ��������Ϣ��
    //���ʹ��CListCtrl�����࣬����������������Ӧ��������Ϣ�ķ�����Ϣ
    //  ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
    if(message == WM_NOTIFY)
    {
        NMHDR* phdr = (NMHDR*)lParam;

        // these 3 notifications are only sent by virtual listviews
        switch(phdr->code)
        {
        case LVN_GETDISPINFO:   //��Ҫ���ݵ�ʱ��
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
        case LVN_ODCACHEHINT:   //������ȡ����������(���DB������ȱȽ����ĵط���ȡ����ʱ)
            {
                NMLVCACHEHINT* pHint = (NMLVCACHEHINT*)lParam;
                PrepareCache(pHint->iFrom, pHint->iTo);
            }
            if(pResult != NULL)
            {
                *pResult = 0;
            }
            break;
        case LVN_ODFINDITEM:    //�û���ͼ����ĳ��Ԫ�ص�ʱ��(����Դ�������� ֱ��������ĸ��λ)
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

        //��Դ���Ӧ���ļ������к�
        if (pLogItem->pszSrcFileName != NULL && pLogItem->srcFileline != 0)
        {
            strFileName = pLogItem->pszSrcFileName;
            line = pLogItem->srcFileline;
        }
        else
        {
            CString strTraceInfo(pLogItem->pszTraceInfo);
            rLogManager.ParseFileNameAndPos(strTraceInfo, strFileName, line);
        }

        //����ҵ��ļ������к�,���Գ���ͨ��Դ�붨λ
        if (!strFileName.IsEmpty() && line != 0)
        {
			TCHAR szPathFull[MAX_PATH] = { 0 };

			//�����ڻ����������(��Ҫ�Ǽ��� �ظ��ļ�ʱ��ѡ������), �����, ��ֱ��ʹ��, �������ߺ�����߼�
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
							StringCchCopy(szPathFull, _countof(szPathFull), *spFilePathList->begin());
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

        CLogManager& logManager = GetDocument()->m_FTLogManager;
        if (logManager.m_logConfig.m_nEnableFullLog)
        {
            pDetailMenu->EnableMenuItem(ID_DETAILS_COPY_FULL_LOG, MF_ENABLED | MF_BYCOMMAND);
        } else {
            pDetailMenu->EnableMenuItem(ID_DETAILS_COPY_FULL_LOG, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
        }
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

            if (0 == strFormater.GetStringLength())
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

int CLogItemView::_GetSelectedIdTypeValue(MachinePIdTIdTypeList& idTypeList) {
    LogItemsContainer selectedLogItems;
    INT nSelectedSubItem = _GetSelectedIdLogItems(selectedLogItems);

    for (LogItemsContainerIter iter = selectedLogItems.begin(); iter != selectedLogItems.end(); ++iter)
    {
        const LogItemPointer pLogItem = *iter;
        MachinePIdTIdType idType;
        idType.machine = pLogItem->machine;
        idType.pid = pLogItem->processId;
        idType.tid = pLogItem->threadId;
        idTypeList.push_back(idType);
        FTLTRACE(TEXT("Filter machine=%s, pid=%s, tid=%s"),
            idType.machine.c_str(), idType.pid.c_str(), idType.tid.c_str());
    }
    return nSelectedSubItem;
}

int CLogItemView::_GetSelectedIdLogItems(LogItemsContainer& selectedLogItems){
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    LogIndexContainer selectedItemList;
    INT nSelectedSubItem = -1;
    LONG nSelectCount = _GetSelectedLines(selectedItemList, nSelectedSubItem);
    if (nSelectCount > 0 && nSelectedSubItem != -1)
    {
        for (LogIndexContainerIter iter = selectedItemList.begin(); iter != selectedItemList.end(); ++iter)
        {
            const LogItemPointer pLogItem = logManager.GetDisplayLogItem(*iter);
            if (pLogItem) {
                selectedLogItems.push_back(pLogItem);
            }
        }
    }

    return nSelectedSubItem;
}

void CLogItemView::OnDetailSelectCurrentPid() {
    int oldSelectIndex = 0;
    MachinePIdTIdTypeList selectIdTypeList;

    if(-1 != (oldSelectIndex = _GetSelectedIdTypeValue(selectIdTypeList))) {
        CLogManager& logManager = GetDocument()->m_FTLogManager;

        UINT nCurSelectLine = _GetCurrentFirstSelectLine();
        logManager.OnlySelectSpecialItems(selectIdTypeList, ONLY_SELECT_TYPE::ostProcessId);
        GetDocument()->UpdateAllViews(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_PID, (CObject*)&selectIdTypeList);
        OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_PID, (CObject*)&selectIdTypeList);
        _GotoSpecialLine(nCurSelectLine);
    }
}

void CLogItemView::OnDetailSelectCurrentTid() {
    int oldSelectIndex = 0;
    MachinePIdTIdTypeList selectIdTypeList;
    if (-1 != (oldSelectIndex = _GetSelectedIdTypeValue(selectIdTypeList))) {
        CLogManager& logManager = GetDocument()->m_FTLogManager;

        UINT nCurSelectLine = _GetCurrentFirstSelectLine();
        logManager.OnlySelectSpecialItems(selectIdTypeList, ONLY_SELECT_TYPE::ostThreadId);
        GetDocument()->UpdateAllViews(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_TID, (CObject*)&selectIdTypeList);
        OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_TID, (CObject*)&selectIdTypeList);
        _GotoSpecialLine(nCurSelectLine);
    }
}

void CLogItemView::OnDetailSelectCurrentFile() {
    int oldSelectIndex = 0;
    LogItemsContainer selectedLogItems;
    if (-1 != (oldSelectIndex = _GetSelectedIdLogItems(selectedLogItems))){ 
        CLogManager& logManager = GetDocument()->m_FTLogManager;

        // ��ȡ�����û�ѡ�����Ч�ļ���
        std::set<CString> selectFileNames;
        for(LogItemsContainerIter iter = selectedLogItems.begin(); iter != selectedLogItems.end(); ++iter){
            const LogItemPointer pLogItem = *iter;
            if (pLogItem->pszSrcFileName)
            {
                CString strFileName(pLogItem->pszSrcFileName);
                if (!strFileName.IsEmpty())
                {
                    selectFileNames.insert(pLogItem->pszSrcFileName);
                }
            }
        }
        if (!selectFileNames.empty())
        {
            UINT nCurSelectLine = _GetCurrentFirstSelectLine();
            logManager.SetFilterFileNames(selectFileNames);

            OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_FILENAME, NULL);
            _GotoSpecialLine(nCurSelectLine);
        }
    }
}

void CLogItemView::OnDetailClearFilterByFiles() {
    UINT nCurSelectLine = _GetCurrentFirstSelectLine();
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    if(logManager.ClearFilterFileNames()){
        OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_FILENAME, NULL);
        _GotoSpecialLine(nCurSelectLine);
    }
}


BOOL CLogItemView::OnEraseBkgnd(CDC* pDC)
{
//������˸
   return __super::OnEraseBkgnd(pDC);
   //return TRUE;
}


//void CLogItemView::OnPaint()
//{
//    //��ӦWM_PAINT��Ϣ
//    CPaintDC dc(this); // device context for painting
//    CRect rect;
//    CRect headerRect;
//    CDC MenDC;//�ڴ�ID��  
//    CBitmap MemMap;
//    GetClientRect(&rect);   
//    GetDlgItem(0)->GetWindowRect(&headerRect);  
//    MenDC.CreateCompatibleDC(&dc);  
//    MemMap.CreateCompatibleBitmap(&dc,rect.Width(),rect.Height());
//    MenDC.SelectObject(&MemMap);
//    MenDC.FillSolidRect(&rect,RGB(228,236,243));  
//
//    //��һ���ǵ���Ĭ�ϵ�OnPaint(),��ͼ�λ����ڴ�DC����  
//    DefWindowProc(WM_PAINT,(WPARAM)MenDC.m_hDC,(LPARAM)0);      
//    //���  
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

    //����Ӧ����
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

    //ѡ���û�ָ����
    SortContent sortContent = logManager.GetFirstSortContent();
    if (type_LineNum == sortContent.contentType && sortContent.bSortAscending
        && logManager.GetTotalLogItemCount() == logManager.GetDisplayLogItemCount()  //no filter
        )
    {
        nClosestItem = nGotoLineNumber - 1; //listCtrl ���� 0 ��ַ��
        nClosestItemLineNum = nGotoLineNumber;
    }
    else {
        //��Ҫ����
        int nTotalCount = ListCtrl.GetItemCount();
        for (int nItem = 0; nItem < nTotalCount; nItem++)
        {
            LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
            FTLASSERT(pLogItem);
            if (pLogItem) {
                if (pLogItem->lineNum == nGotoLineNumber) {
                    //׼ȷ�ҵ�
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