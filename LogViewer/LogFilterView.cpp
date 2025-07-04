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
    m_nStartLineNumber = 0;
    m_nEndLineNumber = INT_MAX;
    m_lastFilterStringChangeTick = 0;
    m_bFilterStringChanged = FALSE;
    m_UpdateFilterStringTimerID = 0;
	m_bInited = FALSE;
}

CLogFilterView::~CLogFilterView()
{
}

void CLogFilterView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_START_LINE_NUMBER, m_nStartLineNumber);
    DDX_Text(pDX, IDC_EDIT_END_LINE_NUMBER, m_nEndLineNumber);
    DDX_Text(pDX, IDC_EDIT_FILTER_STRING, m_strFilterString);
    DDX_Text(pDX, IDC_EDIT_FULL_TRACEINFO, m_strFullTraceInfo);

	DDX_Control(pDX, IDC_COMBO_FILTER, m_comboBoxFilter);
}

BEGIN_MESSAGE_MAP(CLogFilterView, CFormView)
    ON_WM_SIZE()
    ON_CONTROL_RANGE(BN_CLICKED,IDC_CHECK_DETAIL,IDC_CHECK_ERROR,&CLogFilterView::OnBnClickedCheckTraceLevel)
    ON_EN_CHANGE(IDC_EDIT_START_LINE_NUMBER, &CLogFilterView::OnEnChangeEditFilter)
    ON_EN_CHANGE(IDC_EDIT_END_LINE_NUMBER, &CLogFilterView::OnEnChangeEditFilter)
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
    CRect rcClient;
    GetClientRect(&rcClient);

	FTLTRACE(TEXT("Enter OnInitialUpdate, x=%d, y=%d, width=%d, height=%d"), 
        rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height());

	if (FALSE == m_bInited)
	{
		CFormView::OnInitialUpdate();
		m_UpdateFilterStringTimerID = SetTimer(1, 10, NULL);
		for (UINT nId = IDC_CHECK_DETAIL; nId <= IDC_CHECK_ERROR; nId++)
		{
			CheckDlgButton(nId, BST_CHECKED);
		}
		m_comboBoxFilter.SetCurSel(ftAll);

        CWnd* pEditFullTraceInfo =  GetDlgItem(IDC_EDIT_FULL_TRACEINFO);

        CRect rcFullTraceWin;
        pEditFullTraceInfo->GetWindowRect(&rcFullTraceWin);
        this->ScreenToClient(rcFullTraceWin);

        int Margin = rcFullTraceWin.left;

        rcFullTraceWin.right = rcClient.Width() - Margin * 2;
        rcFullTraceWin.bottom = rcClient.Height() - Margin;
        FTLTRACE(TEXT("move edit full trace to {%d, %d}, {%d x %d}"), 
            rcFullTraceWin.left, rcFullTraceWin.top, rcFullTraceWin.Width(), rcFullTraceWin.Height());
        pEditFullTraceInfo->MoveWindow(rcFullTraceWin, TRUE);

        CRect rcFilterStringWin;
        CWnd* pEditFilterString = GetDlgItem(IDC_EDIT_FILTER_STRING);
        pEditFilterString->GetWindowRect(rcFilterStringWin);
        this->ScreenToClient(rcFilterStringWin);
        rcFilterStringWin.right = rcFullTraceWin.right;
        pEditFilterString->MoveWindow(rcFilterStringWin, TRUE);
        FTLTRACE(TEXT("move edit filter to {%d, %d}, {%d x %d}"),
            rcFilterStringWin.left, rcFilterStringWin.top, rcFilterStringWin.Width(), rcFilterStringWin.Height());

		this->InitAutoSizeInfo();
		m_bInited = TRUE;
	}
}

void CLogFilterView::OnSize(UINT nType, int cx, int cy)
{
    CFormView::OnSize(nType, cx, cy);
    FTLTRACE(TEXT("CLogFilterView::OnSize, nTYpe=%d, cx=%d, cy=%d"), nType, cx, cy)
    if (cx > 0 && cy > 0)
    {
        this->AutoResizeUpdateLayout(cx, cy);
    }
    
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

        //ֹͣ����500�����Ժ����Ч
        if (dwCurTicket - m_lastFilterStringChangeTick > 500)
        {
            m_bFilterStringChanged = FALSE;
            m_lastFilterStringChangeTick = dwCurTicket;
            BOOL isExceptionHappened = FALSE;

            API_VERIFY(UpdateData(TRUE));
            FilterType filterType = (FilterType)m_comboBoxFilter.GetCurSel();
            if (ftRegex == filterType)
            {
                //���������ʽ�Ƿ�Ϸ�
                try
                {
                    std::tr1::wregex regularPattern(m_strFilterString);
                    std::tr1::wcmatch regularResults;

                    //TODO: ���û������Ĵ���, ϵͳ�Ƿ�Ὣ regularPattern �Ż���?
                    BOOL bEmptyMatchResult = std::tr1::regex_match(TEXT(""), regularResults, regularPattern);
                    if (!bEmptyMatchResult)
                    {
                        FTLTRACE(TEXT("check text reg filter: %d"), bEmptyMatchResult);
                    }
                }
                catch (const std::tr1::regex_error& e)
                {
                    isExceptionHappened = TRUE;
                    FTL::CFConversion convText, convError;
                    FTL::FormatMessageBox(NULL, TEXT("Regex for Text Filter"), MB_OK,
                        TEXT("Wrong Regex, reason is %s"), convError.UTF8_TO_TCHAR(e.what()));
                }
            }

            if (!isExceptionHappened)
            {
                CLogViewerDoc* pDoc = GetDocument();
                pDoc->m_FTLogManager.SetFilterLineNumber(m_nStartLineNumber, m_nEndLineNumber);
                pDoc->m_FTLogManager.SetLogInfoFilterString(m_strFilterString, filterType);
                pDoc->UpdateAllViews(this, VIEW_UPDATE_HINT_SELECT_LINE_INDEX, NULL);
            }
        }
    }
    __super::OnTimer(nIDEvent);
}

//ʹ�� Filter �����еı༭��֧�� ���ơ�ճ�� �ȿ�ݼ�����
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

