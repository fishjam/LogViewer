// LogFilterView.cpp : implementation file
//

#include "stdafx.h"
#include "LogViewer.h"
#include "LogFilterView.h"

// CLogFilterView

IMPLEMENT_DYNCREATE(CLogFilterView, CFormView)

CLogFilterView::CLogFilterView()
	: CFormView(CLogFilterView::IDD)
{
    m_nStartSeqNumber = 0;
    m_nEndSeqNumber = INT_MAX;
    m_lastFilterStringChangeTick = 0;
    m_bFilterStringChanged = FALSE;
    m_UpdateFilterStringTimerID = 0;
}

CLogFilterView::~CLogFilterView()
{
}

void CLogFilterView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_START_SEQ_NUMBER, m_nStartSeqNumber);
    DDX_Text(pDX, IDC_EDIT_END_SEQ_NUMBER, m_nEndSeqNumber);
    DDX_Text(pDX, IDC_EDIT_FILTER_STRING, m_strFilterString);
    DDX_Text(pDX, IDC_EDIT_FULL_TRACEINFO, m_strFullTraceInfo);

	DDX_Control(pDX, IDC_COMBO_FILTER, m_comboBoxFilter);
}

BEGIN_MESSAGE_MAP(CLogFilterView, CFormView)
    ON_WM_SIZE()
    ON_CONTROL_RANGE(BN_CLICKED,IDC_CHECK_DETAIL,IDC_CHECK_ERROR,&CLogFilterView::OnBnClickedCheckTraceLevel)
    ON_EN_CHANGE(IDC_EDIT_START_SEQ_NUMBER, &CLogFilterView::OnEnChangeEditFilter)
    ON_EN_CHANGE(IDC_EDIT_END_SEQ_NUMBER, &CLogFilterView::OnEnChangeEditFilter)
    ON_EN_CHANGE(IDC_EDIT_FILTER_STRING, &CLogFilterView::OnEnChangeEditFilter)
    ON_CBN_SELCHANGE(IDC_COMBO_FILTER, &CLogFilterView::OnComboboxFilterChange)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CLogFilterView diagnostics

#ifdef _DEBUG
void CLogFilterView::AssertValid() const
{
	CFormView::AssertValid();
}

void CLogFilterView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG


// CLogFilterView message handlers

void CLogFilterView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();
    m_UpdateFilterStringTimerID = SetTimer(1,10,NULL);
    for (UINT nId = IDC_CHECK_DETAIL; nId <= IDC_CHECK_ERROR; nId++)
    {
        CheckDlgButton(nId,BST_CHECKED);
    }
	m_comboBoxFilter.SetCurSel(ftAll);
    this->InitAutoSizeInfo();
}

void CLogFilterView::OnSize(UINT nType, int cx, int cy)
{
    CFormView::OnSize(nType, cx, cy);
    this->AutoResizeUpdateLayout(cx,cy);
    //if (::IsWindow(m_AllLogItemsList.GetSafeHwnd()))
    //{
    //    if (-1 == m_LogItemListWidthMargin)
    //    {
    //        CRect ViewWinRect;
    //        CRect LogItemListWinRect;
    //        GetWindowRect(&ViewWinRect);
    //        m_AllLogItemsList.GetWindowRect(&LogItemListWinRect);
    //        m_LogItemListWidthMargin = ViewWinRect.Width() - LogItemListWinRect.Width();
    //        m_LogItemListHeightMargin = ViewWinRect.Height() - LogItemListWinRect.Height();
    //    }
    //    CRect LogItemListRect;
    //    m_AllLogItemsList.GetClientRect(&LogItemListRect);
    //    m_AllLogItemsList.SetWindowPos(NULL,0,0,cx - m_LogItemListWidthMargin,
    //        cy - m_LogItemListHeightMargin,SWP_NOMOVE);
    //}
    //else
    //{
    //    TRACE(TEXT("False"));
    //}
}


void CLogFilterView::OnUpdate(CView* pSender, LPARAM /*lHint*/, CObject* /*pHint*/)
{
    if (pSender != this)
    {
        CLogViewerDoc *pDoc = GetDocument();
		m_strFullTraceInfo = pDoc->m_FTLogManager.getActiveItemTraceInfo();
		UpdateData(FALSE);
    //    LV_ITEM item = {0};
    //    for (int itemIndex = 0; itemIndex < m_AllLogItemsList.GetItemCount(); itemIndex++)
    //    {
    //        LPLogItem pLogItem = (LPLogItem)m_AllLogItemsList.GetItemData(itemIndex);
    //        ASSERT(pLogItem);
    //    }
    //}
    //else
    //{
    //    FTLASSERT(FALSE);
    }
    
}

void CLogFilterView::OnBnClickedCheckTraceLevel(UINT nId)
{
    int level = nId - IDC_CHECK_DETAIL;
    ASSERT(level >= 0);
    ASSERT(level < tlEnd);
   
    CLogViewerDoc* pDoc = GetDocument();
    pDoc->m_FTLogManager.SetTraceLevelDisplay((TraceLevel)level,(IsDlgButtonChecked(nId) == BST_CHECKED));
    pDoc->UpdateAllViews(this);
}

void CLogFilterView::OnEnChangeEditFilter()
{
    m_bFilterStringChanged = TRUE;
    m_lastFilterStringChangeTick = GetTickCount();
}

void CLogFilterView::OnComboboxFilterChange(){
	m_bFilterStringChanged = TRUE;
	m_lastFilterStringChangeTick = GetTickCount();
}

void CLogFilterView::OnTimer(UINT_PTR nIDEvent)
{
    BOOL bRet = FALSE;

    if (m_bFilterStringChanged)
    {
        DWORD dwCurTicket = GetTickCount();

        //停止输入500毫秒以后才生效
        if (dwCurTicket - m_lastFilterStringChangeTick > 500)
        {
            m_bFilterStringChanged = FALSE;
            m_lastFilterStringChangeTick = dwCurTicket;

            API_VERIFY(UpdateData(TRUE));
            CLogViewerDoc* pDoc = GetDocument();
            pDoc->m_FTLogManager.SetFilterSeqNumber(m_nStartSeqNumber, m_nEndSeqNumber);
            pDoc->m_FTLogManager.SetLogInfoFilterString(m_strFilterString,
                (FilterType)m_comboBoxFilter.GetCurSel());
            pDoc->UpdateAllViews(this);
        }
    }
    __super::OnTimer(nIDEvent);
}

//使得 Filter 窗口中的编辑框支持 复制、粘贴 等快捷键操作
BOOL CLogFilterView::PreTranslateMessage(MSG* pMsg)
{
    //if (IsDialogMessage(pMsg))
    //DUMP_WINDOWS_MSG(__FILE__LINE__, NULL, 0, pMsg->message, pMsg->wParam, pMsg->lParam);

    if( IsDialogMessage( pMsg ) )
    {
        return TRUE;
    }
    else
    {
        return __super::PreTranslateMessage( pMsg );
    }
}

