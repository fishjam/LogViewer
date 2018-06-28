// StudioListDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "LogViewer.h"
#include "StudioListDlg.h"


// CStudioListDlg 对话框

IMPLEMENT_DYNAMIC(CStudioListDlg, CDialog)

CStudioListDlg::CStudioListDlg(CVsIdeHandler& vsIdeHandler,CWnd* pParent /*=NULL*/)
	: CDialog(CStudioListDlg::IDD, pParent)
    , m_VsIdeHandler(vsIdeHandler)
{
    m_SelectedIDE = NULL;    
}

CStudioListDlg::~CStudioListDlg()
{
}

void CStudioListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_STUDIOS, m_StudiosList);
}


BEGIN_MESSAGE_MAP(CStudioListDlg, CDialog)
    ON_WM_CLOSE()
    ON_LBN_SELCHANGE(IDC_LIST_STUDIOS, &CStudioListDlg::OnLbnSelchangeListStudios)
END_MESSAGE_MAP()


// CStudioListDlg 消息处理程序

BOOL CStudioListDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    HRESULT hr = E_FAIL;
    StudioInfo studioInfo = {0};
    COM_VERIFY(m_VsIdeHandler.StartFindStudio());
    if(SUCCEEDED(hr))
    {
        while(S_OK == hr)
        {
            hr = m_VsIdeHandler.FindNextStudio(&studioInfo);
            if (S_OK == hr)
            {
                int itemIndex = m_StudiosList.AddString(studioInfo.strDisplayName);
                m_StudiosList.SetItemData(itemIndex,(DWORD_PTR)studioInfo.pStudioIDE);
            }
        }
        m_VsIdeHandler.CloseFind();
    }
    return TRUE;  // return TRUE unless you set the focus to a control
}

void CStudioListDlg::OnClose()
{
    for (int i=0; i < m_StudiosList.GetCount(); i++)
    {
        IUnknown* pUnk = (IUnknown*)m_StudiosList.GetItemData(i);
        FTLASSERT(pUnk);
        pUnk->Release();
        m_StudiosList.SetItemData(i,NULL);
    }
    CDialog::OnClose();
}

void CStudioListDlg::OnLbnSelchangeListStudios()
{
    int curSel = m_StudiosList.GetCurSel();
    if (curSel != LB_ERR)
    {
        m_SelectedIDE = (IUnknown*)m_StudiosList.GetItemData(curSel);
    }
    else
    {
        m_SelectedIDE = NULL;
    }
}
