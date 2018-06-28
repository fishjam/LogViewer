#ifndef FTL_WINDOW_HPP
#define FTL_WINDOW_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlwindow.h"
#endif

#include <commdlg.h>
#include <ShellAPI.h>

#include <prsht.h>
#include <CommCtrl.h>
#include <zmouse.h>
#include "ftlConversion.h"
//#include "ftlNLS.h"

namespace FTL
{

    template <typename T, typename TWindowAutoSizeTraits >
    CFWindowAutoSize<T,TWindowAutoSizeTraits>::CFWindowAutoSize()
    {
        m_sizeWindow.cx = 0;
        m_sizeWindow.cy = 0; 
        m_ptMinTrackSize.x = -1;
        m_ptMinTrackSize.y = -1;
        m_bGripper = FALSE;
    }

    template <typename T, typename TWindowAutoSizeTraits >
    BOOL CFWindowAutoSize<T,TWindowAutoSizeTraits>::InitAutoSizeInfo(BOOL bAddGripper /* = TRUE */, BOOL bUseMinTrackSize /* = TRUE */)
    {
        BOOL bRet = FALSE;
        T* pT = static_cast<T*>(this);
        HWND hWnd = TWindowAutoSizeTraits::GetWinHwndProxy(pT);
        FTLASSERT(::IsWindow(hWnd));
        DWORD dwStyle = (DWORD)::GetWindowLong(hWnd, GWL_STYLE);
#ifdef FTL_DEBUG
        // Debug only: Check if top level dialogs have a resizeable border.
        if(((dwStyle & WS_CHILD) == 0) && ((dwStyle & WS_THICKFRAME) == 0))
        {
            FTLTRACEEX(tlWarn,TEXT("top level dialog without the WS_THICKFRAME style - user cannot resize it\n"));
        }
#endif // _DEBUG

        {
            // Cleanup in case of multiple initialization
            // block: first check for the gripper control, destroy it if needed
            HWND HwndGripper = ::GetDlgItem(hWnd, TWindowAutoSizeTraits::GetStatusBarCtrlID());
            if( ::IsWindow(HwndGripper) && m_allResizeData.size() > 0 && (m_allResizeData[0].m_dwResizeFlags & _WINSZ_GRIPPER) != 0)
            {
                API_VERIFY(::DestroyWindow(HwndGripper));
            }
        }
        // clear out everything else
        m_allResizeData.clear();
        m_sizeWindow.cx = 0;
        m_sizeWindow.cy = 0;
        m_ptMinTrackSize.x = -1;
        m_ptMinTrackSize.y = -1;

        // Get initial dialog client size
        RECT rectDlg = { 0 };
        API_VERIFY(::GetClientRect(hWnd,&rectDlg));
        m_sizeWindow.cx = rectDlg.right;
        m_sizeWindow.cy = rectDlg.bottom;

        // Create gripper if requested
        m_bGripper = FALSE;
        if(bAddGripper)
        {
            // shouldn't exist already
            FTLASSERT(!::IsWindow(::GetDlgItem(hWnd, TWindowAutoSizeTraits::GetStatusBarCtrlID())));
            if(!::IsWindow(::GetDlgItem(hWnd, TWindowAutoSizeTraits::GetStatusBarCtrlID())))
            {
                HWND hWndGripper = ::CreateWindowEx(0,_T("SCROLLBAR"),NULL, 
                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEBOX | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
                    rectDlg.left,
                    rectDlg.top,
                    rectDlg.right - rectDlg.left,
                    rectDlg.bottom - rectDlg.top,
                    hWnd,
                    (HMENU)(TWindowAutoSizeTraits::GetStatusBarCtrlID()), 
                    NULL,
                    NULL);
                FTLASSERT(::IsWindow(hWndGripper));
                if(::IsWindow(hWndGripper))
                {
                    m_bGripper = TRUE;
                    RECT rectCtl = { 0 };
                    API_VERIFY(::GetWindowRect(hWndGripper,&rectCtl));
                    API_VERIFY(::MapWindowPoints(NULL, hWnd, (LPPOINT)&rectCtl, 2) != 0);
                    _WindowResizeData data = 
                    {
                        TWindowAutoSizeTraits::GetStatusBarCtrlID(), 
                        WINSZ_MOVE_X | WINSZ_MOVE_Y | WINSZ_REPAINT | _WINSZ_GRIPPER, 
                        { 
                            rectCtl.left, 
                                rectCtl.top, 
                                rectCtl.right, 
                                rectCtl.bottom 
                        }
                    };
                    m_allResizeData.push_back(data);
                }
            }
        }

        // Get min track position if requested
        if(bUseMinTrackSize)
        {
            if((dwStyle & WS_CHILD) != 0)
            {
                RECT rect = { 0 };
                API_VERIFY(::GetClientRect(hWnd,&rect));
                m_ptMinTrackSize.x = rect.right - rect.left;
                m_ptMinTrackSize.y = rect.bottom - rect.top;
            }
            else
            {
                RECT rect = { 0 };
                API_VERIFY(::GetWindowRect(hWnd, &rect));
                m_ptMinTrackSize.x = rect.right - rect.left;
                m_ptMinTrackSize.y = rect.bottom - rect.top;
            }
        }

        // Walk the map and initialize data
        const _WindowResizeMap* pMap = pT->GetWindowResizeMap();
        FTLASSERT(pMap != NULL);
        int nGroupStart = -1;
        for(int nCount = 1; !(pMap->m_nCtlID == -1 && pMap->m_dwResizeFlags == 0); nCount++, pMap++)
        {
            if(pMap->m_nCtlID == -1)    //开始或结束一个组
            {
                switch(pMap->m_dwResizeFlags)
                {
                case _WINSZ_BEGIN_GROUP:
                    FTLASSERT(nGroupStart == -1);
                    nGroupStart = static_cast<int>(m_allResizeData.size());
                    break;
                case _WINSZ_END_GROUP:
                    {
                        FTLASSERT(nGroupStart != -1);
                        int nGroupCount = static_cast<int>(m_allResizeData.size()) - nGroupStart;
                        m_allResizeData[nGroupStart].SetGroupCount(nGroupCount);
                        nGroupStart = -1;
                    }
                    break;
                default:
                    FTLASSERT(FALSE && _T("Invalid WINDOWRESIZE Map Entry"));
                    break;
                }
            }
            else
            {
                // this ID conflicts with the default gripper one
                FTLASSERT(m_bGripper ? (pMap->m_nCtlID != TWindowAutoSizeTraits::GetStatusBarCtrlID()) : TRUE);
                HWND hWndCtrl = ::GetDlgItem(hWnd,pMap->m_nCtlID);
                FTLASSERT(::IsWindow(hWndCtrl));
                RECT rectCtl = { 0 };
                API_VERIFY(::GetWindowRect(hWndCtrl,&rectCtl));
                API_VERIFY(::MapWindowPoints(NULL, hWnd, (LPPOINT)&rectCtl, 2)!= 0);

                DWORD dwGroupFlag = (nGroupStart != -1 && static_cast<int>(m_allResizeData.size()) == nGroupStart) ? _WINSZ_BEGIN_GROUP : 0;
                _WindowResizeData data = 
                {
                    pMap->m_nCtlID, 
                    pMap->m_dwResizeFlags | dwGroupFlag, 
                    { 
                        rectCtl.left, 
                            rectCtl.top, 
                            rectCtl.right, 
                            rectCtl.bottom 
                    } 
                };
                m_allResizeData.push_back(data);
            }
        }
        FTLASSERT((nGroupStart == -1) && _T("No End Group Entry in the WINDOWRESIZE Map"));
        return bRet;
    }

    template <typename T, typename TWindowAutoSizeTraits >
    BOOL CFWindowAutoSize<T,TWindowAutoSizeTraits>::AutoResizeUpdateLayout(int cxWidth, int cyHeight)
    {
        BOOL bRet = FALSE;
        T* pT = static_cast<T*>(this);
        HWND hWnd = TWindowAutoSizeTraits::GetWinHwndProxy(pT);

        FTLASSERT(::IsWindow(hWnd));

        // Restrict minimum size if requested
        if((((DWORD)::GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) != 0) && m_ptMinTrackSize.x != -1 && m_ptMinTrackSize.y != -1)
        {
            if(cxWidth < m_ptMinTrackSize.x)
            {
                cxWidth = m_ptMinTrackSize.x;
            }
            if(cyHeight < m_ptMinTrackSize.y)
            {
                cyHeight = m_ptMinTrackSize.y;
            }
        }

        BOOL bVisible = ::IsWindowVisible(hWnd);
        if(bVisible)
        {
            ::SendMessage(hWnd, WM_SETREDRAW, (WPARAM)FALSE, 0);
            //pT->SetRedraw(FALSE);
        }

        for(size_t i = 0; i < m_allResizeData.size(); i++)
        {
            if((m_allResizeData[i].m_dwResizeFlags & _WINSZ_BEGIN_GROUP) != 0)   // start of a group
            {
                int nGroupCount = m_allResizeData[i].GetGroupCount();
                FTLASSERT(nGroupCount > 0 && i + nGroupCount - 1 < m_allResizeData.size());
                RECT rectGroup = m_allResizeData[i].m_rect;

                int j = 1;
                for(j = 1; j < nGroupCount; j++)
                {
                    rectGroup.left = min(rectGroup.left, m_allResizeData[i + j].m_rect.left);
                    rectGroup.top = min(rectGroup.top, m_allResizeData[i + j].m_rect.top);
                    rectGroup.right = max(rectGroup.right, m_allResizeData[i + j].m_rect.right);
                    rectGroup.bottom = max(rectGroup.bottom, m_allResizeData[i + j].m_rect.bottom);
                }

                for(j = 0; j < nGroupCount; j++)
                {
                    _WindowResizeData* pDataPrev = NULL;
                    if(j > 0)
                        pDataPrev = &(m_allResizeData[i + j - 1]);
                    pT->AutoPositionControl(cxWidth, cyHeight, rectGroup, m_allResizeData[i + j], true, pDataPrev);
                }

                i += nGroupCount - 1;   // increment to skip all group controls
            }
            else // one control entry
            {
                RECT rectGroup = { 0, 0, 0, 0 };
                pT->AutoPositionControl(cxWidth, cyHeight, rectGroup, m_allResizeData[i], false);
            }
        }

        if(bVisible)
        {
            ::SendMessage(hWnd, WM_SETREDRAW, (WPARAM)TRUE, 0);
            //pT->SetRedraw(TRUE);
        }
        ::RedrawWindow(hWnd,NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        return bRet;
    }

    template <typename T, typename TWindowAutoSizeTraits >
    BOOL CFWindowAutoSize<T,TWindowAutoSizeTraits>::AutoPositionControl(int cxWidth, int cyHeight, 
        RECT& rectGroup, _WindowResizeData& data, 
        bool bGroup, _WindowResizeData* pDataPrev /* = NULL */)
    {
        //BOOL bRet = FALSE;
        T* pT = static_cast<T*>(this);
        HWND hWnd = TWindowAutoSizeTraits::GetWinHwndProxy(pT);
        FTLASSERT(::IsWindow(hWnd));

        HWND hWndCtrl = NULL;
        RECT rectCtl = { 0 };

        hWndCtrl = ::GetDlgItem(hWnd, data.m_nCtlID);
        FTLASSERT(::IsWindow(hWndCtrl));
        ::GetWindowRect(hWndCtrl, &rectCtl);
        ::MapWindowPoints(NULL, hWnd, (LPPOINT)&rectCtl, 2);

        if(bGroup)
        {
            if((data.m_dwResizeFlags & WINSZ_CENTER_X) != 0)
            {
                int cxRight = rectGroup.right + cxWidth - m_sizeWindow.cx;
                int cxCtl = data.m_rect.right - data.m_rect.left;
                rectCtl.left = rectGroup.left + (cxRight - rectGroup.left - cxCtl) / 2;
                rectCtl.right = rectCtl.left + cxCtl;
            }
            else if((data.m_dwResizeFlags & (WINSZ_SIZE_X | WINSZ_MOVE_X)) != 0)
            {
                rectCtl.left = rectGroup.left + ::MulDiv(data.m_rect.left - rectGroup.left, rectGroup.right - rectGroup.left + (cxWidth - m_sizeWindow.cx), rectGroup.right - rectGroup.left);

                if((data.m_dwResizeFlags & WINSZ_SIZE_X) != 0)
                {
                    rectCtl.right = rectGroup.left + ::MulDiv(data.m_rect.right - rectGroup.left, rectGroup.right - rectGroup.left + (cxWidth - m_sizeWindow.cx), rectGroup.right - rectGroup.left);

                    if(pDataPrev != NULL)
                    {
                        HWND hWndCtlPrev = ::GetDlgItem(hWnd,pDataPrev->m_nCtlID);
                        FTLASSERT(::IsWindow(hWndCtlPrev));
                        RECT rcPrev = { 0 };
                        ::GetWindowRect(hWndCtlPrev, &rcPrev);
                        ::MapWindowPoints(NULL, hWnd, (LPPOINT)&rcPrev, 2);
                        int dxAdjust = (rectCtl.left - rcPrev.right) - (data.m_rect.left - pDataPrev->m_rect.right);
                        rcPrev.right += dxAdjust;
                        ::SetWindowPos(hWndCtlPrev, NULL, rcPrev.left,rcPrev.top,rcPrev.right - rcPrev.left,
                            rcPrev.bottom-rcPrev.top,SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
                    }
                }
                else
                {
                    rectCtl.right = rectCtl.left + (data.m_rect.right - data.m_rect.left);
                }
            }

            if((data.m_dwResizeFlags & WINSZ_CENTER_Y) != 0)
            {
                int cyBottom = rectGroup.bottom + cyHeight - m_sizeWindow.cy;
                int cyCtl = data.m_rect.bottom - data.m_rect.top;
                rectCtl.top = rectGroup.top + (cyBottom - rectGroup.top - cyCtl) / 2;
                rectCtl.bottom = rectCtl.top + cyCtl;
            }
            else if((data.m_dwResizeFlags & (WINSZ_SIZE_Y | WINSZ_MOVE_Y)) != 0)
            {
                rectCtl.top = rectGroup.top + ::MulDiv(data.m_rect.top - rectGroup.top, rectGroup.bottom - rectGroup.top + (cyHeight - m_sizeWindow.cy), rectGroup.bottom - rectGroup.top);

                if((data.m_dwResizeFlags & WINSZ_SIZE_Y) != 0)
                {
                    rectCtl.bottom = rectGroup.top + ::MulDiv(data.m_rect.bottom - rectGroup.top, rectGroup.bottom - rectGroup.top + (cyHeight - m_sizeWindow.cy), rectGroup.bottom - rectGroup.top);

                    if(pDataPrev != NULL)
                    {
                        HWND hWndCtlPrev = ::GetDlgItem(hWnd, pDataPrev->m_nCtlID);
                        FTLASSERT(::IsWindow(hWndCtlPrev));
                        RECT rcPrev = { 0 };
                        ::GetWindowRect(hWndCtlPrev, &rcPrev);
                        ::MapWindowPoints(NULL, hWnd, (LPPOINT)&rcPrev, 2);
                        int dxAdjust = (rectCtl.top - rcPrev.bottom) - (data.m_rect.top - pDataPrev->m_rect.bottom);
                        rcPrev.bottom += dxAdjust;
                        ::SetWindowPos(hWndCtlPrev, NULL, rcPrev.left,rcPrev.top,rcPrev.right - rcPrev.left,
                            rcPrev.bottom-rcPrev.top,SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
                    }
                }
                else
                {
                    rectCtl.bottom = rectCtl.top + (data.m_rect.bottom - data.m_rect.top);
                }
            }
        }
        else // no group
        {
            if((data.m_dwResizeFlags & WINSZ_CENTER_X) != 0)
            {
                int cxCtl = data.m_rect.right - data.m_rect.left;
                rectCtl.left = (cxWidth - cxCtl) / 2;
                rectCtl.right = rectCtl.left + cxCtl;
            }
            else if((data.m_dwResizeFlags & (WINSZ_SIZE_X | WINSZ_MOVE_X)) != 0)
            {
                rectCtl.right = data.m_rect.right + (cxWidth - m_sizeWindow.cx);

                if((data.m_dwResizeFlags & WINSZ_MOVE_X) != 0)
                    rectCtl.left = rectCtl.right - (data.m_rect.right - data.m_rect.left);
            }

            if((data.m_dwResizeFlags & WINSZ_CENTER_Y) != 0)
            {
                int cyCtl = data.m_rect.bottom - data.m_rect.top;
                rectCtl.top = (cyHeight - cyCtl) / 2;
                rectCtl.bottom = rectCtl.top + cyCtl;
            }
            else if((data.m_dwResizeFlags & (WINSZ_SIZE_Y | WINSZ_MOVE_Y)) != 0)
            {
                rectCtl.bottom = data.m_rect.bottom + (cyHeight - m_sizeWindow.cy);

                if((data.m_dwResizeFlags & WINSZ_MOVE_Y) != 0)
                    rectCtl.top = rectCtl.bottom - (data.m_rect.bottom - data.m_rect.top);
            }
        }

        if((data.m_dwResizeFlags & WINSZ_REPAINT) != 0)
        {
            ::InvalidateRect(hWndCtrl, NULL, TRUE);
        }
        if((data.m_dwResizeFlags & (WINSZ_SIZE_X | WINSZ_SIZE_Y | WINSZ_MOVE_X | WINSZ_MOVE_Y | WINSZ_REPAINT | WINSZ_CENTER_X | WINSZ_CENTER_Y)) != 0)
        {
            ::SetWindowPos(hWndCtrl, NULL, rectCtl.left,rectCtl.top,rectCtl.right - rectCtl.left,
                rectCtl.bottom-rectCtl.top,SWP_NOZORDER | SWP_NOACTIVATE);

        }
        return TRUE;
    }

    BOOL CFWinUtil::SetWindowFullScreen(HWND hWnd,BOOL isFullScreen, BOOL &oldZoomedState)
    {
        BOOL bRet = FALSE;
        //获取标题栏 SM_CXFRAME, SM_CYFRAME, SM_CXScreen 的大小
        int cyCaption = ::GetSystemMetrics(SM_CYCAPTION);
        int cxFrame = ::GetSystemMetrics(SM_CXFRAME);
        int cyFrame = ::GetSystemMetrics(SM_CYFRAME);
        int cyMenu = ::GetSystemMetrics(SM_CYMENU);
        int cxScreen = ::GetSystemMetrics(SM_CXSCREEN);
        int cyScreen = ::GetSystemMetrics(SM_CYSCREEN);
        //int cxBorder = ::GetSystemMetrics(SM_CXBORDER);
        //int cyBorder = ::GetSystemMetrics(SM_CYBORDER);

        if (isFullScreen)
        {
            //oldZoomedState = ::IsZoomed(hWnd); //保存当前是否是最大化状态
            //if(oldZoomedState) //当前是最大化
            {
                BringWindowToTop(hWnd);
                bRet = ::SetWindowPos(hWnd,HWND_TOPMOST,-cxFrame,-(cyFrame + cyCaption + cyMenu),
                    cxScreen + 2 * cxFrame, cyScreen + 2 * cyFrame + cyCaption + cyMenu,
                    0);//SWP_NOOWNERZORDER
                //SetForegroundWindow

            }
            //else //当前是普通状态，进最大化
            //{
            //    bRet = ::ShowWindow(hWnd,SW_SHOWMAXIMIZED);
            //}
        }
        else //恢复原窗口大小
        {
            if(oldZoomedState)
            {
                //是否错了？？？
                bRet = SetWindowPos(hWnd,NULL,-cxFrame,-cyFrame,cxScreen + 2*cxFrame, cyScreen + 2*cyFrame, SWP_NOZORDER);
            }
            else
            {
                bRet = ShowWindow(hWnd,SW_RESTORE);
            }
        }
        return bRet;
    }

    BOOL CFWinUtil::CenterWindow(HWND hWndCenter, BOOL bCurrMonitor)
    {
        BOOL bRet = FALSE;

        FTLASSERT ( ::IsWindow ( hWndCenter ) ) ;
        // determine owner window to center against
        LONG_PTR dwStyle = ::GetWindowLongPtr (hWndCenter, GWL_STYLE ) ;
        if( dwStyle & WS_CHILD ){
            FTLASSERT(FALSE && TEXT("do not support child window"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if( ( dwStyle & WS_VISIBLE  ) || ( dwStyle & WS_MINIMIZE )){
            // Don't center against invisible or minimized windows
            return FALSE; //TRUE ?
        }
        //if ( NULL == hWndParent )
        //{
        //    if( dwStyle & WS_CHILD )
        //    {
        //        hWndParent = ::GetParent ( hWndCenter ) ;
        //    }
        //    else
        //    {
        //        hWndParent = ::GetWindow ( hWndCenter , GW_OWNER ) ;
        //    }

        //    if (NULL == hWndParent)
        //    {
        //        hWndParent = ::GetDesktopWindow(); 
        //    }
        //}

        // Get coordinates of the window relative to its parent
        RECT rcDlg = {0};
        API_VERIFY(::GetWindowRect (hWndCenter, &rcDlg ));
        RECT rcArea = {0};
        RECT rcCenter = {0};

        if ( ! bCurrMonitor )
        {
            // Center within screen coordinates
            API_VERIFY(::SystemParametersInfo ( SPI_GETWORKAREA , NULL, &rcArea, NULL));
            rcCenter = rcArea;
        }
        else
        {
            // Center based on the monitor containing the majority of the window.
            HMONITOR hMon = MonitorFromWindow ( hWndCenter  , MONITOR_DEFAULTTONEAREST);
            MONITORINFO stMI = {0};
            stMI.cbSize = sizeof ( MONITORINFO ) ;
            API_VERIFY(GetMonitorInfo ( hMon , &stMI));
            rcArea = stMI.rcWork;
            rcCenter = stMI.rcMonitor;
        }


        int DlgWidth = rcDlg.right - rcDlg.left ;
        int DlgHeight = rcDlg.bottom - rcDlg.top ;

        // Find dialog's upper left based on rcCenter
        int xLeft = (rcArea.left + rcArea.right) / 2 - DlgWidth / 2 ;
        int yTop = (rcArea.top + rcArea.bottom) / 2 - DlgHeight / 2 ;

        // If the dialog is outside the screen, move it inside
        if ( xLeft < rcArea.left )
        {
            xLeft = rcArea.left ;
        }
        else if ( xLeft + DlgWidth > rcArea.right )
        {
            xLeft = rcArea.right - DlgWidth ;
        }

        if ( yTop < rcArea.top )
        {
            yTop = rcArea.top ;
        }
        else if ( yTop + DlgHeight > rcArea.bottom )
        {
            yTop = rcArea.bottom - DlgHeight ;
        }

        // Map screen coordinates to child coordinates
        API_VERIFY(::SetWindowPos ( hWndCenter, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) ;
        return bRet;
    }


    LRESULT CFWinUtil::CalcNcHitTestPostion(LPPOINT pPtClient, LPCRECT prcClient, LPCRECT prcCaption, BOOL bZoomed)
    {
        //检测时各个方向的阈值(XP:4, Win7:8)
        int nCxFrame = GetSystemMetrics(SM_CXFRAME);
        int nCyFrame = GetSystemMetrics(SM_CYFRAME);
        RECT rcSizeBox = {nCxFrame, nCyFrame, nCxFrame, nCyFrame}; 

        POINT pt = *pPtClient;
        RECT rcClient = *prcClient;

        FTLASSERT(pt.x < 0xFFFF);
        FTLASSERT(pt.y < 0xFFFF);
        //什么时候会出现这种情况？一边拖拽，一边按 Win+D 等键?
        if (pt.x >= 0xffff) { pt.x -= 0xffff; }
        if (pt.y >= 0xffff) { pt.y -= 0xffff; }

        if( !bZoomed ) {
            if( pt.y < rcClient.top + rcSizeBox.top ) {
                if( pt.x < rcClient.left + rcSizeBox.left ) return HTTOPLEFT;
                if( pt.x > rcClient.right - rcSizeBox.right ) return HTTOPRIGHT;
                return HTTOP;
            }
            else if( pt.y > rcClient.bottom - rcSizeBox.bottom ) {
                if( pt.x < rcClient.left + rcSizeBox.left ) return HTBOTTOMLEFT;
                if( pt.x > rcClient.right - rcSizeBox.right ) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            if( pt.x < rcClient.left + rcSizeBox.left ) return HTLEFT;
            if( pt.x > rcClient.right - rcSizeBox.right ) return HTRIGHT;
        }

        RECT rcCaption = *prcCaption;
        if( pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right
            && pt.y >= rcCaption.top && pt.y < rcCaption.bottom ) {
//#pragma TODO(需要排除 关闭、最小化、最大化、Option菜单等控件的位置)
                //如果这些地方是按钮的话，按钮会优先处理，因此不需要排除
                //但如果类似DUI，是自绘的话，则必须排除

                //CControlUI* pControl = static_cast<CControlUI*>(m_pm.FindControl(pt));
                //if( pControl && _tcscmp(pControl->GetClass(), _T("ButtonUI")) != 0 && 
                //	_tcscmp(pControl->GetClass(), _T("OptionUI")) != 0 &&
                //	_tcscmp(pControl->GetClass(), _T("TextUI")) != 0 )
                return HTCAPTION;
        }
        return HTCLIENT;
    }

    HWND CFWinUtil::GetProcessMainWindow(DWORD dwProcessId)
    {
        HWND hWnd = ::GetTopWindow(NULL);
        while (hWnd)
        {
            DWORD dwPid = 0;
            DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwPid);
            UNREFERENCED_PARAMETER(dwThreadId);
            if (dwPid != 0 && dwPid == dwProcessId)
            {
                return hWnd;
            }
            hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT);
        }
        return NULL;
    }

    BOOL CFWinUtil::ActiveAndForegroundWindow(HWND hWnd)
    {
        //TODO: AllowSetForegroundWindow
        //IPMSG 中也有一个 SetForceForegroundWindow 方法，大致相同
//#pragma TODO(Foreground 和 Active 有啥区别)
        BOOL bRet = TRUE;
        HWND   hForegdWnd   =  ::GetForegroundWindow();
        if(NULL == hForegdWnd)
        {
            FTLTRACEEX(tlWarn, TEXT("GetForegroundWindow is NULL\n"));
            hForegdWnd = ::GetActiveWindow();
        }
        if (NULL == hForegdWnd)
        {
            FTLTRACEEX(tlWarn, TEXT("GetActiveWindow is NULL\n"));
            hForegdWnd = GetFocus();
        }
        API_VERIFY(NULL != hForegdWnd);

        if (hForegdWnd == hWnd)
        {
            return TRUE;
        }

        DWORD   dwThisThreadID   =   ::GetWindowThreadProcessId(hWnd, NULL);
        DWORD   dwForeThreadID   =   ::GetWindowThreadProcessId(hForegdWnd,NULL);
        if (dwThisThreadID != dwForeThreadID)
        {
            API_VERIFY(::AttachThreadInput(dwThisThreadID,dwForeThreadID,TRUE));

            //备份以前的值，然后设置为0
            DWORD sp_time = 0;
            SystemParametersInfo( SPI_GETFOREGROUNDLOCKTIMEOUT,0,&sp_time,0);
            SystemParametersInfo( SPI_SETFOREGROUNDLOCKTIMEOUT,0,(LPVOID)NULL, SPIF_SENDCHANGE); //0);

            API_VERIFY(::BringWindowToTop( hWnd ));
            API_VERIFY(::SetForegroundWindow(hWnd));

            SystemParametersInfo( SPI_SETFOREGROUNDLOCKTIMEOUT,0,&sp_time, SPIF_SENDCHANGE); //0);
            API_VERIFY(::AttachThreadInput(dwThisThreadID,dwForeThreadID,FALSE));
            //ShowWindow(hWnd,...)?
        }
        else
        {
            API_VERIFY(::BringWindowToTop( hWnd ));
            API_VERIFY(::SetForegroundWindow(hWnd));
            //ShowWindow(hWnd,...)?
        }
        SwitchToThisWindow(hWnd, TRUE);

        //#pragma TODO(how to active a minimize window without change it min/max status)
        //        if (IsIconic(hWnd))
        //        {
        //            API_VERIFY(ShowWindow(hWnd,SW_RESTORE));
        //        }
        //        else
        //        {
        //            API_VERIFY(ShowWindow(hWnd,SW_SHOW));
        //        }

        //是否需要这些代码
#if 0
        if ( ::IsIconic ( hWnd ) )
        {
            ::ShowWindow( hWnd, SW_RESTORE );
            //::SendMessage ( hWnd, WM_SYSCOMMAND, SC_RESTORE, 0 );
        }
        else
        {
            SetForegroundWindow( hMovWnd );
        }
#endif 

        return bRet;
    }

    BOOL CFWinUtil::HideTaskBar(BOOL bHide)
    {
        BOOL bRet = FALSE;
        int nCmdShow = SW_HIDE; 
        HWND hWnd = NULL; 
        LPARAM lParam = 0;  

        API_VERIFY(NULL != (hWnd = FindWindow(_T("Shell_TrayWnd"),NULL)));  
        if(bHide)  
        {  
            nCmdShow = SW_HIDE;  
            lParam = ABS_AUTOHIDE | ABS_ALWAYSONTOP;  
        }  
        else  
        {  
            nCmdShow = SW_SHOW;  
            lParam = ABS_ALWAYSONTOP;  
        }  

        ShowWindow(hWnd,nCmdShow);  

        HWND hWndStartBtn=::FindWindow(_T("Button"),NULL);//win7下隐藏开始按钮  
        ShowWindow(hWndStartBtn,nCmdShow);  
        //EnableWindow(hWndStartBtn, !bHide);

        APPBARDATA apBar = {0};   
        apBar.cbSize = sizeof(apBar);   
        apBar.hWnd = hWnd;   
        if(apBar.hWnd != NULL)   
        {   
            apBar.lParam = lParam;   
            API_VERIFY((BOOL)SHAppBarMessage(ABM_SETSTATE, &apBar));  
        }   
        return bRet;
    }

    LPCDLGTEMPLATE CFWinUtil::LoadDialog(HMODULE hModuleRes, LPCTSTR szResource, HINSTANCE * phInstFoundIn)
    {
        UNREFERENCED_PARAMETER(phInstFoundIn);
        // Find the dialog resource.
        HRSRC hRSRC = FindResourceEx ( hModuleRes ,RT_DIALOG,szResource,MAKELANGID ( LANG_NEUTRAL,SUBLANG_NEUTRAL )) ;
        if ( NULL == hRSRC )
        {
            return ( NULL ) ;
        }
        // Now load it.
        HGLOBAL hResource = LoadResource ( hModuleRes , hRSRC ) ;
        FTLASSERT ( NULL != hResource ) ;
        if ( NULL == hResource )
        {
            return ( NULL ) ;
        }
        LPCDLGTEMPLATE lpDlgTemplate = (LPCDLGTEMPLATE)LockResource ( hResource ) ;
        return lpDlgTemplate;
    }

    LPCTSTR CFWinUtil::GetColorString(ColorType clrType, int nColorIndex)
    {
        switch (clrType)
        {
        case ctCtrlColor:
            {
                switch(nColorIndex)
                {
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_MSGBOX);
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_EDIT);
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_LISTBOX);
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_BTN);
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_DLG);
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_SCROLLBAR);
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_STATIC);
                    HANDLE_CASE_RETURN_STRING(CTLCOLOR_MAX);
                }
                break;
            }
        case ctSysColor:
            {
                switch(nColorIndex)
                {
                    HANDLE_CASE_RETURN_STRING(COLOR_SCROLLBAR);
                    HANDLE_CASE_RETURN_STRING(COLOR_BACKGROUND);
                    HANDLE_CASE_RETURN_STRING(COLOR_ACTIVECAPTION);
                    HANDLE_CASE_RETURN_STRING(COLOR_INACTIVECAPTION);
                    HANDLE_CASE_RETURN_STRING(COLOR_MENU);
                    HANDLE_CASE_RETURN_STRING(COLOR_WINDOW);
                    HANDLE_CASE_RETURN_STRING(COLOR_WINDOWFRAME);
                    HANDLE_CASE_RETURN_STRING(COLOR_MENUTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_WINDOWTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_CAPTIONTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_ACTIVEBORDER);
                    HANDLE_CASE_RETURN_STRING(COLOR_INACTIVEBORDER);
                    HANDLE_CASE_RETURN_STRING(COLOR_APPWORKSPACE);
                    HANDLE_CASE_RETURN_STRING(COLOR_HIGHLIGHT);
                    HANDLE_CASE_RETURN_STRING(COLOR_HIGHLIGHTTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_BTNFACE);
                    HANDLE_CASE_RETURN_STRING(COLOR_BTNSHADOW);
                    HANDLE_CASE_RETURN_STRING(COLOR_GRAYTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_BTNTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_INACTIVECAPTIONTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_BTNHIGHLIGHT);
#if(WINVER >= 0x0400)
                    HANDLE_CASE_RETURN_STRING(COLOR_3DDKSHADOW);
                    HANDLE_CASE_RETURN_STRING(COLOR_3DLIGHT);
                    HANDLE_CASE_RETURN_STRING(COLOR_INFOTEXT);
                    HANDLE_CASE_RETURN_STRING(COLOR_INFOBK);
#endif /* WINVER >= 0x0400 */

#if(WINVER >= 0x0500)
                    HANDLE_CASE_RETURN_STRING(COLOR_HOTLIGHT);
                    HANDLE_CASE_RETURN_STRING(COLOR_GRADIENTACTIVECAPTION);
                    HANDLE_CASE_RETURN_STRING(COLOR_GRADIENTINACTIVECAPTION);
#endif /* WINVER >= 0x0501 */

#if(WINVER >= 0x0501)
                    HANDLE_CASE_RETURN_STRING(COLOR_MENUHILIGHT);
                    HANDLE_CASE_RETURN_STRING(COLOR_MENUBAR);
#endif /* WINVER >= 0x0500 */
                }
                break;
            }
        }
        FTLTRACEEX(FTL::tlWarn, TEXT("Unknown Color type(%d) and Index(%d)\n"), clrType, nColorIndex);
        return TEXT("Unknown");
    }

    LPCTSTR CFWinUtil::GetScrollBarCodeString(UINT nSBCode)
    {
        switch(nSBCode)
        {
            HANDLE_CASE_RETURN_STRING((SB_LINEUP|SB_LINELEFT));
            HANDLE_CASE_RETURN_STRING((SB_LINEDOWN|SB_LINERIGHT));
            HANDLE_CASE_RETURN_STRING((SB_PAGEUP|SB_PAGELEFT));
            HANDLE_CASE_RETURN_STRING((SB_PAGEDOWN|SB_PAGERIGHT));
            HANDLE_CASE_RETURN_STRING(SB_THUMBPOSITION);
            HANDLE_CASE_RETURN_STRING(SB_THUMBTRACK);
            HANDLE_CASE_RETURN_STRING((SB_TOP|SB_LEFT));
            HANDLE_CASE_RETURN_STRING((SB_BOTTOM|SB_RIGHT));
            HANDLE_CASE_RETURN_STRING(SB_ENDSCROLL);
        default:
            FTLTRACEEX(FTL::tlWarn, TEXT("Unknown ScrollBar Code, %d\n"), nSBCode);
            return TEXT("Unknown");
        }
    }

    LPCTSTR CFWinUtil::GetVirtualKeyString(int nVirtKey)
    {
        switch (nVirtKey)
        {
            HANDLE_CASE_RETURN_STRING(VK_LBUTTON);
            HANDLE_CASE_RETURN_STRING(VK_RBUTTON);
            HANDLE_CASE_RETURN_STRING(VK_CANCEL);
            HANDLE_CASE_RETURN_STRING(VK_MBUTTON);
#if(_WIN32_WINNT >= 0x0500)
            HANDLE_CASE_RETURN_STRING(VK_XBUTTON1);
            HANDLE_CASE_RETURN_STRING(VK_XBUTTON2);
#endif /* _WIN32_WINNT >= 0x0500 */

            // * 0x07 : unassigned
            HANDLE_CASE_RETURN_STRING(VK_BACK);
            HANDLE_CASE_RETURN_STRING(VK_TAB);
            HANDLE_CASE_RETURN_STRING(VK_CLEAR);
            HANDLE_CASE_RETURN_STRING(VK_RETURN);
            HANDLE_CASE_RETURN_STRING(VK_SHIFT);
            HANDLE_CASE_RETURN_STRING(VK_CONTROL);
            HANDLE_CASE_RETURN_STRING(VK_MENU);
            HANDLE_CASE_RETURN_STRING(VK_PAUSE);
            HANDLE_CASE_RETURN_STRING(VK_CAPITAL);
            HANDLE_CASE_RETURN_STRING((VK_KANA|VK_HANGEUL|VK_HANGUL)); //have some old name
            HANDLE_CASE_RETURN_STRING(VK_JUNJA);
            HANDLE_CASE_RETURN_STRING(VK_FINAL);
            HANDLE_CASE_RETURN_STRING((VK_HANJA | VK_KANJI));
            HANDLE_CASE_RETURN_STRING(VK_ESCAPE);
            HANDLE_CASE_RETURN_STRING(VK_CONVERT);
            HANDLE_CASE_RETURN_STRING(VK_NONCONVERT);
            HANDLE_CASE_RETURN_STRING(VK_ACCEPT);
            HANDLE_CASE_RETURN_STRING(VK_MODECHANGE);
            HANDLE_CASE_RETURN_STRING(VK_SPACE);
            HANDLE_CASE_RETURN_STRING(VK_PRIOR);
            HANDLE_CASE_RETURN_STRING(VK_NEXT);
            HANDLE_CASE_RETURN_STRING(VK_END);
            HANDLE_CASE_RETURN_STRING(VK_HOME);
            HANDLE_CASE_RETURN_STRING(VK_LEFT);
            HANDLE_CASE_RETURN_STRING(VK_UP);
            HANDLE_CASE_RETURN_STRING(VK_RIGHT);
            HANDLE_CASE_RETURN_STRING(VK_DOWN);
            HANDLE_CASE_RETURN_STRING(VK_SELECT);
            HANDLE_CASE_RETURN_STRING(VK_PRINT);
            HANDLE_CASE_RETURN_STRING(VK_EXECUTE);
            HANDLE_CASE_RETURN_STRING(VK_SNAPSHOT);
            HANDLE_CASE_RETURN_STRING(VK_INSERT);
            HANDLE_CASE_RETURN_STRING(VK_DELETE);
            HANDLE_CASE_RETURN_STRING(VK_HELP);

            //VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
            HANDLE_CASE_RETURN_STRING('0');
            HANDLE_CASE_RETURN_STRING('1');
            HANDLE_CASE_RETURN_STRING('2');
            HANDLE_CASE_RETURN_STRING('3');
            HANDLE_CASE_RETURN_STRING('4');
            HANDLE_CASE_RETURN_STRING('5');
            HANDLE_CASE_RETURN_STRING('6');
            HANDLE_CASE_RETURN_STRING('7');
            HANDLE_CASE_RETURN_STRING('8');
            HANDLE_CASE_RETURN_STRING('9');
            //VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
            HANDLE_CASE_RETURN_STRING('A');
            HANDLE_CASE_RETURN_STRING('B');
            HANDLE_CASE_RETURN_STRING('C');
            HANDLE_CASE_RETURN_STRING('D');
            HANDLE_CASE_RETURN_STRING('E');
            HANDLE_CASE_RETURN_STRING('F');
            HANDLE_CASE_RETURN_STRING('G');
            HANDLE_CASE_RETURN_STRING('H');
            HANDLE_CASE_RETURN_STRING('I');
            HANDLE_CASE_RETURN_STRING('J');
            HANDLE_CASE_RETURN_STRING('K');
            HANDLE_CASE_RETURN_STRING('L');
            HANDLE_CASE_RETURN_STRING('M');
            HANDLE_CASE_RETURN_STRING('N');
            HANDLE_CASE_RETURN_STRING('O');
            HANDLE_CASE_RETURN_STRING('P');
            HANDLE_CASE_RETURN_STRING('Q');
            HANDLE_CASE_RETURN_STRING('R');
            HANDLE_CASE_RETURN_STRING('S');
            HANDLE_CASE_RETURN_STRING('T');
            HANDLE_CASE_RETURN_STRING('U');
            HANDLE_CASE_RETURN_STRING('V');
            HANDLE_CASE_RETURN_STRING('W');
            HANDLE_CASE_RETURN_STRING('X');
            HANDLE_CASE_RETURN_STRING('Y');
            HANDLE_CASE_RETURN_STRING('Z');

            HANDLE_CASE_RETURN_STRING(VK_LWIN);
            HANDLE_CASE_RETURN_STRING(VK_RWIN);
            HANDLE_CASE_RETURN_STRING(VK_APPS);
            HANDLE_CASE_RETURN_STRING(VK_SLEEP);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD0);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD1);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD2);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD3);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD4);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD5);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD6);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD7);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD8);
            HANDLE_CASE_RETURN_STRING(VK_NUMPAD9);
            HANDLE_CASE_RETURN_STRING(VK_MULTIPLY);
            HANDLE_CASE_RETURN_STRING(VK_ADD);
            HANDLE_CASE_RETURN_STRING(VK_SEPARATOR);
            HANDLE_CASE_RETURN_STRING(VK_SUBTRACT);
            HANDLE_CASE_RETURN_STRING(VK_DECIMAL);
            HANDLE_CASE_RETURN_STRING(VK_DIVIDE);
            HANDLE_CASE_RETURN_STRING(VK_F1);
            HANDLE_CASE_RETURN_STRING(VK_F2);
            HANDLE_CASE_RETURN_STRING(VK_F3);
            HANDLE_CASE_RETURN_STRING(VK_F4);
            HANDLE_CASE_RETURN_STRING(VK_F5);
            HANDLE_CASE_RETURN_STRING(VK_F6);
            HANDLE_CASE_RETURN_STRING(VK_F7);
            HANDLE_CASE_RETURN_STRING(VK_F8);
            HANDLE_CASE_RETURN_STRING(VK_F9);
            HANDLE_CASE_RETURN_STRING(VK_F10);
            HANDLE_CASE_RETURN_STRING(VK_F11);
            HANDLE_CASE_RETURN_STRING(VK_F12);
            HANDLE_CASE_RETURN_STRING(VK_F13);
            HANDLE_CASE_RETURN_STRING(VK_F14);
            HANDLE_CASE_RETURN_STRING(VK_F15);
            HANDLE_CASE_RETURN_STRING(VK_F16);
            HANDLE_CASE_RETURN_STRING(VK_F17);
            HANDLE_CASE_RETURN_STRING(VK_F18);
            HANDLE_CASE_RETURN_STRING(VK_F19);
            HANDLE_CASE_RETURN_STRING(VK_F20);
            HANDLE_CASE_RETURN_STRING(VK_F21);
            HANDLE_CASE_RETURN_STRING(VK_F22);
            HANDLE_CASE_RETURN_STRING(VK_F23);
            HANDLE_CASE_RETURN_STRING(VK_F24);
            HANDLE_CASE_RETURN_STRING(VK_NUMLOCK);
            HANDLE_CASE_RETURN_STRING(VK_SCROLL);
            HANDLE_CASE_RETURN_STRING((VK_OEM_NEC_EQUAL | VK_OEM_FJ_JISHO));
            HANDLE_CASE_RETURN_STRING(VK_OEM_FJ_MASSHOU);
            HANDLE_CASE_RETURN_STRING(VK_OEM_FJ_TOUROKU);
            HANDLE_CASE_RETURN_STRING(VK_OEM_FJ_LOYA);
            HANDLE_CASE_RETURN_STRING(VK_OEM_FJ_ROYA);
            HANDLE_CASE_RETURN_STRING(VK_LSHIFT);
            HANDLE_CASE_RETURN_STRING(VK_RSHIFT);
            HANDLE_CASE_RETURN_STRING(VK_LCONTROL);
            HANDLE_CASE_RETURN_STRING(VK_RCONTROL);
            HANDLE_CASE_RETURN_STRING(VK_LMENU);
            HANDLE_CASE_RETURN_STRING(VK_RMENU);
#if(_WIN32_WINNT >= 0x0500)
            HANDLE_CASE_RETURN_STRING(VK_BROWSER_BACK);
            HANDLE_CASE_RETURN_STRING(VK_BROWSER_FORWARD);
            HANDLE_CASE_RETURN_STRING(VK_BROWSER_REFRESH);
            HANDLE_CASE_RETURN_STRING(VK_BROWSER_STOP);
            HANDLE_CASE_RETURN_STRING(VK_BROWSER_SEARCH);
            HANDLE_CASE_RETURN_STRING(VK_BROWSER_FAVORITES);
            HANDLE_CASE_RETURN_STRING(VK_BROWSER_HOME);
            HANDLE_CASE_RETURN_STRING(VK_VOLUME_MUTE);
            HANDLE_CASE_RETURN_STRING(VK_VOLUME_DOWN);
            HANDLE_CASE_RETURN_STRING(VK_VOLUME_UP);
            HANDLE_CASE_RETURN_STRING(VK_MEDIA_NEXT_TRACK);
            HANDLE_CASE_RETURN_STRING(VK_MEDIA_PREV_TRACK);
            HANDLE_CASE_RETURN_STRING(VK_MEDIA_STOP);
            HANDLE_CASE_RETURN_STRING(VK_MEDIA_PLAY_PAUSE);
            HANDLE_CASE_RETURN_STRING(VK_LAUNCH_MAIL);
            HANDLE_CASE_RETURN_STRING(VK_LAUNCH_MEDIA_SELECT);
            HANDLE_CASE_RETURN_STRING(VK_LAUNCH_APP1);
            HANDLE_CASE_RETURN_STRING(VK_LAUNCH_APP2);
#endif /* _WIN32_WINNT >= 0x0500 */
            HANDLE_CASE_RETURN_STRING(VK_OEM_1);		// ';:' for US
            HANDLE_CASE_RETURN_STRING(VK_OEM_PLUS);		// '+' any country
            HANDLE_CASE_RETURN_STRING(VK_OEM_COMMA);	// ',' any country
            HANDLE_CASE_RETURN_STRING(VK_OEM_MINUS);	// '-' any country
            HANDLE_CASE_RETURN_STRING(VK_OEM_PERIOD);	// '.' any country
            HANDLE_CASE_RETURN_STRING(VK_OEM_2);		// '/?' for US
            HANDLE_CASE_RETURN_STRING(VK_OEM_3);		// '`~' for US
            HANDLE_CASE_RETURN_STRING(VK_OEM_4);		//  '[{' for US
            HANDLE_CASE_RETURN_STRING(VK_OEM_5);		//  '\|' for US
            HANDLE_CASE_RETURN_STRING(VK_OEM_6);		//  ']}' for US
            HANDLE_CASE_RETURN_STRING(VK_OEM_7);		//  ''"' for US
            HANDLE_CASE_RETURN_STRING(VK_OEM_8);		//  'AX' key on Japanese AX kbd
            HANDLE_CASE_RETURN_STRING(VK_OEM_AX);		//  "<>" or "\|" on RT 102-key kbd.
            HANDLE_CASE_RETURN_STRING(VK_OEM_102);		//  Help key on ICO
            HANDLE_CASE_RETURN_STRING(VK_ICO_HELP);		//  00 key on ICO
            HANDLE_CASE_RETURN_STRING(VK_ICO_00);
#if(WINVER >= 0x0400)
            HANDLE_CASE_RETURN_STRING(VK_PROCESSKEY);
#endif /* WINVER >= 0x0400 */
            HANDLE_CASE_RETURN_STRING(VK_ICO_CLEAR);
#if(_WIN32_WINNT >= 0x0500)
            HANDLE_CASE_RETURN_STRING(VK_PACKET);
#endif /* _WIN32_WINNT >= 0x0500 */

            //Nokia/Ericsson definitions
            HANDLE_CASE_RETURN_STRING(VK_OEM_RESET);
            HANDLE_CASE_RETURN_STRING(VK_OEM_JUMP);
            HANDLE_CASE_RETURN_STRING(VK_OEM_PA1);
            HANDLE_CASE_RETURN_STRING(VK_OEM_PA2);
            HANDLE_CASE_RETURN_STRING(VK_OEM_PA3);
            HANDLE_CASE_RETURN_STRING(VK_OEM_WSCTRL);
            HANDLE_CASE_RETURN_STRING(VK_OEM_CUSEL);
            HANDLE_CASE_RETURN_STRING(VK_OEM_ATTN);
            HANDLE_CASE_RETURN_STRING(VK_OEM_FINISH);
            HANDLE_CASE_RETURN_STRING(VK_OEM_COPY);
            HANDLE_CASE_RETURN_STRING(VK_OEM_AUTO);
            HANDLE_CASE_RETURN_STRING(VK_OEM_ENLW);
            HANDLE_CASE_RETURN_STRING(VK_OEM_BACKTAB);

            HANDLE_CASE_RETURN_STRING(VK_ATTN);
            HANDLE_CASE_RETURN_STRING(VK_CRSEL);
            HANDLE_CASE_RETURN_STRING(VK_EXSEL);
            HANDLE_CASE_RETURN_STRING(VK_EREOF);
            HANDLE_CASE_RETURN_STRING(VK_PLAY);
            HANDLE_CASE_RETURN_STRING(VK_ZOOM);
            HANDLE_CASE_RETURN_STRING(VK_NONAME);
            HANDLE_CASE_RETURN_STRING(VK_PA1);
            HANDLE_CASE_RETURN_STRING(VK_OEM_CLEAR);
        default:
            FTLTRACEEX(FTL::tlWarn, TEXT("Unknown VirtualKey, 0x%x\n"), nVirtKey);
            return TEXT("Unknown");
        }
    }

    LPCTSTR CFWinUtil::GetNotifyCodeString(HWND hWnd, UINT nCode, LPTSTR pszCommandNotify, int nLength, 
        TranslateWndClassProc pTransProc/* = g_pTranslateWndClassProc*/)
    {
        UNREFERENCED_PARAMETER(hWnd);
        UNREFERENCED_PARAMETER(pszCommandNotify);
        UNREFERENCED_PARAMETER(nLength);
        UNREFERENCED_PARAMETER(pTransProc);

        switch(nCode)
        {
            // generic to all controls
            HANDLE_CASE_RETURN_STRING(NM_FIRST);
            HANDLE_CASE_RETURN_STRING(NM_OUTOFMEMORY);
            HANDLE_CASE_RETURN_STRING(NM_CLICK);
            HANDLE_CASE_RETURN_STRING(NM_DBLCLK);
            HANDLE_CASE_RETURN_STRING(NM_RETURN);
            HANDLE_CASE_RETURN_STRING(NM_RCLICK);
            HANDLE_CASE_RETURN_STRING(NM_RDBLCLK);
            HANDLE_CASE_RETURN_STRING(NM_SETFOCUS);
            HANDLE_CASE_RETURN_STRING(NM_KILLFOCUS);
            HANDLE_CASE_RETURN_STRING(NM_CUSTOMDRAW);
            HANDLE_CASE_RETURN_STRING(NM_HOVER);
            HANDLE_CASE_RETURN_STRING(NM_NCHITTEST);
            HANDLE_CASE_RETURN_STRING(NM_KEYDOWN);
            HANDLE_CASE_RETURN_STRING(NM_RELEASEDCAPTURE);
            HANDLE_CASE_RETURN_STRING(NM_SETCURSOR);
            HANDLE_CASE_RETURN_STRING(NM_CHAR);
            HANDLE_CASE_RETURN_STRING(NM_TOOLTIPSCREATED);
#if (_WIN32_IE >= 0x0500)
            HANDLE_CASE_RETURN_STRING(NM_LDOWN);
            HANDLE_CASE_RETURN_STRING(NM_RDOWN);
            HANDLE_CASE_RETURN_STRING(NM_THEMECHANGED);
#endif //_WIN32_IE >= 0x0500

#if (_WIN32_WINNT >= 0x0600)
            HANDLE_CASE_RETURN_STRING(NM_FONTCHANGED);
            //HANDLE_CASE_RETURN_STRING(NM_CUSTOMTEXT);
            HANDLE_CASE_RETURN_STRING((NM_TVSTATEIMAGECHANGING | NM_CUSTOMTEXT)); //Same ID
#endif
            HANDLE_CASE_RETURN_STRING(NM_LAST);

            // listview
            HANDLE_CASE_RETURN_STRING(LVN_ITEMCHANGING);
            HANDLE_CASE_RETURN_STRING(LVN_ITEMCHANGED);
            HANDLE_CASE_RETURN_STRING(LVN_INSERTITEM);
            HANDLE_CASE_RETURN_STRING(LVN_DELETEITEM);
            HANDLE_CASE_RETURN_STRING(LVN_DELETEALLITEMS);
            HANDLE_CASE_RETURN_STRING(LVN_BEGINLABELEDITA);
            HANDLE_CASE_RETURN_STRING(LVN_BEGINLABELEDITW);
            HANDLE_CASE_RETURN_STRING(LVN_ENDLABELEDITA);
            HANDLE_CASE_RETURN_STRING(LVN_ENDLABELEDITW);
            HANDLE_CASE_RETURN_STRING(LVN_COLUMNCLICK);
            HANDLE_CASE_RETURN_STRING(LVN_BEGINDRAG);
            HANDLE_CASE_RETURN_STRING(LVN_BEGINRDRAG);
#if (_WIN32_IE >= 0x0300)
            HANDLE_CASE_RETURN_STRING(LVN_ODCACHEHINT);
            HANDLE_CASE_RETURN_STRING(LVN_ODFINDITEMA);
            HANDLE_CASE_RETURN_STRING(LVN_ODFINDITEMW);
            HANDLE_CASE_RETURN_STRING(LVN_ITEMACTIVATE);
            HANDLE_CASE_RETURN_STRING(LVN_ODSTATECHANGED);
#endif      // _WIN32_IE >= 0x0300


#if (_WIN32_IE >= 0x0400)
            HANDLE_CASE_RETURN_STRING(LVN_HOTTRACK);
#endif
            HANDLE_CASE_RETURN_STRING(LVN_GETDISPINFOA);
            HANDLE_CASE_RETURN_STRING(LVN_GETDISPINFOW);
            HANDLE_CASE_RETURN_STRING(LVN_SETDISPINFOA);
            HANDLE_CASE_RETURN_STRING(LVN_SETDISPINFOW);
            HANDLE_CASE_RETURN_STRING(LVN_KEYDOWN);
            HANDLE_CASE_RETURN_STRING(LVN_MARQUEEBEGIN);
            HANDLE_CASE_RETURN_STRING(LVN_GETINFOTIPA);
            HANDLE_CASE_RETURN_STRING(LVN_GETINFOTIPW);
#if (_WIN32_IE >= 0x0500)
#  ifndef LVN_INCREMENTALSEARCHA    
#    define LVN_INCREMENTALSEARCHA  (LVN_FIRST-62)
#  endif 
            HANDLE_CASE_RETURN_STRING(LVN_INCREMENTALSEARCHA);

#  ifndef LVN_INCREMENTALSEARCHW
#    define LVN_INCREMENTALSEARCHW  (LVN_FIRST-63)
#  endif 
            HANDLE_CASE_RETURN_STRING(LVN_INCREMENTALSEARCHW);
#endif //_WIN32_IE >= 0x0500 

#if _WIN32_WINNT >= 0x0600
            HANDLE_CASE_RETURN_STRING(LVN_COLUMNDROPDOWN);
            HANDLE_CASE_RETURN_STRING(LVN_COLUMNOVERFLOWCLICK);
#endif // _WIN32_WINNT >= 0x0600
#if (_WIN32_WINNT >= 0x0501)
            HANDLE_CASE_RETURN_STRING(LVN_BEGINSCROLL);
            HANDLE_CASE_RETURN_STRING(LVN_ENDSCROLL);
#endif
#if _WIN32_WINNT >= 0x0600
            HANDLE_CASE_RETURN_STRING(LVN_LINKCLICK);
            HANDLE_CASE_RETURN_STRING(LVN_GETEMPTYMARKUP);
#endif //_WIN32_WINNT >= 0x0600

            HANDLE_CASE_RETURN_STRING(LVN_LAST);

            // Property sheet reserved      (0U-200U) -  (0U-299U) - see prsht.h

            HANDLE_CASE_RETURN_STRING(PSN_SETACTIVE);
            HANDLE_CASE_RETURN_STRING(PSN_KILLACTIVE);  //PSN_VALIDATE
            HANDLE_CASE_RETURN_STRING(PSN_APPLY);
            HANDLE_CASE_RETURN_STRING(PSN_RESET);       //PSN_CANCEL
            HANDLE_CASE_RETURN_STRING(PSN_HELP);
            HANDLE_CASE_RETURN_STRING(PSN_WIZBACK);
            HANDLE_CASE_RETURN_STRING(PSN_WIZNEXT);
            HANDLE_CASE_RETURN_STRING(PSN_WIZFINISH);
            HANDLE_CASE_RETURN_STRING(PSN_QUERYCANCEL);
#if (_WIN32_IE >= 0x0400)
            HANDLE_CASE_RETURN_STRING(PSN_GETOBJECT);
#endif // 0x0400
#if (_WIN32_IE >= 0x0500)
            HANDLE_CASE_RETURN_STRING(PSN_TRANSLATEACCELERATOR);
            HANDLE_CASE_RETURN_STRING(PSN_QUERYINITIALFOCUS);
#endif // 0x0500

            // header
            HANDLE_CASE_RETURN_STRING(HDN_ITEMCHANGINGA);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMCHANGINGW);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMCHANGEDA);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMCHANGEDW);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMCLICKA);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMCLICKW);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMDBLCLICKA);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMDBLCLICKW);
            HANDLE_CASE_RETURN_STRING(HDN_DIVIDERDBLCLICKA);
            HANDLE_CASE_RETURN_STRING(HDN_DIVIDERDBLCLICKW);
            HANDLE_CASE_RETURN_STRING(HDN_BEGINTRACKA);
            HANDLE_CASE_RETURN_STRING(HDN_BEGINTRACKW);
            HANDLE_CASE_RETURN_STRING(HDN_ENDTRACKA);
            HANDLE_CASE_RETURN_STRING(HDN_ENDTRACKW);
            HANDLE_CASE_RETURN_STRING(HDN_TRACKA);
            HANDLE_CASE_RETURN_STRING(HDN_TRACKW);
#if (_WIN32_IE >= 0x0300)
            HANDLE_CASE_RETURN_STRING(HDN_GETDISPINFOA);
            HANDLE_CASE_RETURN_STRING(HDN_GETDISPINFOW);
            HANDLE_CASE_RETURN_STRING(HDN_BEGINDRAG);
            HANDLE_CASE_RETURN_STRING(HDN_ENDDRAG);
#endif //_WIN32_IE
#if (_WIN32_IE >= 0x0500)
            HANDLE_CASE_RETURN_STRING(HDN_FILTERCHANGE);
            HANDLE_CASE_RETURN_STRING(HDN_FILTERBTNCLICK);
#endif //0x0500
#if (_WIN32_IE >= 0x0600)
            HANDLE_CASE_RETURN_STRING(HDN_BEGINFILTEREDIT);
            HANDLE_CASE_RETURN_STRING(HDN_ENDFILTEREDIT);
#endif
#if _WIN32_WINNT >= 0x0600
            HANDLE_CASE_RETURN_STRING(HDN_ITEMSTATEICONCLICK);
            HANDLE_CASE_RETURN_STRING(HDN_ITEMKEYDOWN);
            HANDLE_CASE_RETURN_STRING(HDN_DROPDOWN);
            HANDLE_CASE_RETURN_STRING(HDN_OVERFLOWCLICK);
#endif

            // treeview
            HANDLE_CASE_RETURN_STRING(TVN_SELCHANGINGA);
            HANDLE_CASE_RETURN_STRING(TVN_SELCHANGINGW);
            HANDLE_CASE_RETURN_STRING(TVN_SELCHANGEDA);
            HANDLE_CASE_RETURN_STRING(TVN_SELCHANGEDW);
            HANDLE_CASE_RETURN_STRING(TVN_GETDISPINFOA);
            HANDLE_CASE_RETURN_STRING(TVN_GETDISPINFOW);
            HANDLE_CASE_RETURN_STRING(TVN_SETDISPINFOA);
            HANDLE_CASE_RETURN_STRING(TVN_SETDISPINFOW);
            HANDLE_CASE_RETURN_STRING(TVN_ITEMEXPANDINGA);
            HANDLE_CASE_RETURN_STRING(TVN_ITEMEXPANDINGW);
            HANDLE_CASE_RETURN_STRING(TVN_ITEMEXPANDEDA);
            HANDLE_CASE_RETURN_STRING(TVN_ITEMEXPANDEDW);
            HANDLE_CASE_RETURN_STRING(TVN_BEGINDRAGA);
            HANDLE_CASE_RETURN_STRING(TVN_BEGINDRAGW);
            HANDLE_CASE_RETURN_STRING(TVN_BEGINRDRAGA);
            HANDLE_CASE_RETURN_STRING(TVN_BEGINRDRAGW);
            HANDLE_CASE_RETURN_STRING(TVN_DELETEITEMA);
            HANDLE_CASE_RETURN_STRING(TVN_DELETEITEMW);
            HANDLE_CASE_RETURN_STRING(TVN_BEGINLABELEDITA);
            HANDLE_CASE_RETURN_STRING(TVN_BEGINLABELEDITW);
            HANDLE_CASE_RETURN_STRING(TVN_ENDLABELEDITA);
            HANDLE_CASE_RETURN_STRING(TVN_ENDLABELEDITW);
            HANDLE_CASE_RETURN_STRING(TVN_KEYDOWN);
#if (_WIN32_IE >= 0x0400)
            HANDLE_CASE_RETURN_STRING(TVN_GETINFOTIPA);
            HANDLE_CASE_RETURN_STRING(TVN_GETINFOTIPW);
            HANDLE_CASE_RETURN_STRING(TVN_SINGLEEXPAND);
#endif // 0x400
#if (_WIN32_IE >= 0x0600)
            HANDLE_CASE_RETURN_STRING(TVN_ITEMCHANGINGA);
            HANDLE_CASE_RETURN_STRING(TVN_ITEMCHANGINGW);
            HANDLE_CASE_RETURN_STRING(TVN_ITEMCHANGEDA);
            HANDLE_CASE_RETURN_STRING(TVN_ITEMCHANGEDW);
            HANDLE_CASE_RETURN_STRING(TVN_ASYNCDRAW);
#endif // 0x0600

            //tooltips
            HANDLE_CASE_RETURN_STRING(TTN_GETDISPINFOA);    //== TTN_NEEDTEXTA
            HANDLE_CASE_RETURN_STRING(TTN_GETDISPINFOW);    //== TTN_NEEDTEXTW
            HANDLE_CASE_RETURN_STRING(TTN_SHOW);
            HANDLE_CASE_RETURN_STRING(TTN_POP);
            HANDLE_CASE_RETURN_STRING(TTN_LINKCLICK);

            //tab control
            HANDLE_CASE_RETURN_STRING(TCN_KEYDOWN);
            HANDLE_CASE_RETURN_STRING(TCN_SELCHANGE);
            HANDLE_CASE_RETURN_STRING(TCN_SELCHANGING);
#if (_WIN32_IE >= 0x0400)
            HANDLE_CASE_RETURN_STRING(TCN_GETOBJECT);
#endif      // _WIN32_IE >= 0x0400
#if (_WIN32_IE >= 0x0500)
            HANDLE_CASE_RETURN_STRING(TCN_FOCUSCHANGE);
#endif      // _WIN32_IE >= 0x0500


            // Shell reserved               (0U-580U) -  (0U-589U)

            // common dialog (new)

            // toolbar
            HANDLE_CASE_RETURN_STRING(TBN_GETBUTTONINFOA);
            HANDLE_CASE_RETURN_STRING(TBN_BEGINDRAG);
            HANDLE_CASE_RETURN_STRING(TBN_ENDDRAG);
            HANDLE_CASE_RETURN_STRING(TBN_BEGINADJUST);
            HANDLE_CASE_RETURN_STRING(TBN_ENDADJUST);
            HANDLE_CASE_RETURN_STRING(TBN_RESET);
            HANDLE_CASE_RETURN_STRING(TBN_QUERYINSERT);
            HANDLE_CASE_RETURN_STRING(TBN_QUERYDELETE);
            HANDLE_CASE_RETURN_STRING(TBN_TOOLBARCHANGE);
            HANDLE_CASE_RETURN_STRING(TBN_CUSTHELP);
#if (_WIN32_IE >= 0x0300)
            HANDLE_CASE_RETURN_STRING(TBN_DROPDOWN);
#endif //0x0300
#if (_WIN32_IE >= 0x0400)
            HANDLE_CASE_RETURN_STRING(TBN_GETOBJECT);
#endif //0x0400

            HANDLE_CASE_RETURN_STRING(TBN_HOTITEMCHANGE);
            HANDLE_CASE_RETURN_STRING(TBN_DRAGOUT);
            HANDLE_CASE_RETURN_STRING(TBN_DELETINGBUTTON);
            HANDLE_CASE_RETURN_STRING(TBN_GETDISPINFOA);
            HANDLE_CASE_RETURN_STRING(TBN_GETDISPINFOW);
            HANDLE_CASE_RETURN_STRING(TBN_GETINFOTIPA);
            HANDLE_CASE_RETURN_STRING(TBN_GETINFOTIPW);
            HANDLE_CASE_RETURN_STRING(TBN_GETBUTTONINFOW);
#if (_WIN32_IE >= 0x0500)
            HANDLE_CASE_RETURN_STRING(TBN_RESTORE);
            //HANDLE_CASE_RETURN_STRING(TBN_SAVE);
            HANDLE_CASE_RETURN_STRING(TBN_INITCUSTOMIZE);
#endif // (_WIN32_IE >= 0x0500)

            //#define CDN_FIRST   (0U-601U)
            HANDLE_CASE_RETURN_STRING(CDN_INITDONE);
            HANDLE_CASE_RETURN_STRING(CDN_SELCHANGE);
            HANDLE_CASE_RETURN_STRING(CDN_FOLDERCHANGE);
            HANDLE_CASE_RETURN_STRING(CDN_SHAREVIOLATION);
            HANDLE_CASE_RETURN_STRING(CDN_HELP);
            HANDLE_CASE_RETURN_STRING(CDN_FILEOK);
            HANDLE_CASE_RETURN_STRING(CDN_TYPECHANGE);
            HANDLE_CASE_RETURN_STRING(CDN_INCLUDEITEM);

            // updown
            HANDLE_CASE_RETURN_STRING(UDN_DELTAPOS); //注意：UDN_DELTAPOS 和 TBN_SAVE 的值一样（TBN_XXX 的值超过了 TBN_LAST）

            // datetimepick
            HANDLE_CASE_RETURN_STRING(DTN_USERSTRINGA);
            HANDLE_CASE_RETURN_STRING(DTN_USERSTRINGW);
            HANDLE_CASE_RETURN_STRING(DTN_WMKEYDOWNA);
            HANDLE_CASE_RETURN_STRING(DTN_WMKEYDOWNW);
            HANDLE_CASE_RETURN_STRING(DTN_FORMATA);
            HANDLE_CASE_RETURN_STRING(DTN_FORMATW);
            HANDLE_CASE_RETURN_STRING(DTN_FORMATQUERYA);
            HANDLE_CASE_RETURN_STRING(DTN_FORMATQUERYW);

            // monthcal
            HANDLE_CASE_RETURN_STRING(MCN_SELCHANGE);
            HANDLE_CASE_RETURN_STRING(MCN_GETDAYSTATE);
            HANDLE_CASE_RETURN_STRING(MCN_SELECT);
            //HANDLE_CASE_RETURN_STRING(MCN_VIEWCHANGE);

            // datetimepick2
            HANDLE_CASE_RETURN_STRING(DTN_DATETIMECHANGE);
            HANDLE_CASE_RETURN_STRING(DTN_DROPDOWN);
            HANDLE_CASE_RETURN_STRING(DTN_CLOSEUP);

            // combo box ex
            HANDLE_CASE_RETURN_STRING(CBEN_GETDISPINFOA);
            HANDLE_CASE_RETURN_STRING(CBEN_INSERTITEM);
            HANDLE_CASE_RETURN_STRING(CBEN_DELETEITEM);
            HANDLE_CASE_RETURN_STRING(CBEN_BEGINEDIT);
            HANDLE_CASE_RETURN_STRING(CBEN_ENDEDITA);
            HANDLE_CASE_RETURN_STRING(CBEN_ENDEDITW);
            HANDLE_CASE_RETURN_STRING(CBEN_GETDISPINFOW);
            HANDLE_CASE_RETURN_STRING(CBEN_DRAGBEGINA);
            HANDLE_CASE_RETURN_STRING(CBEN_DRAGBEGINW);

            // rebar
            HANDLE_CASE_RETURN_STRING(RBN_HEIGHTCHANGE);
#if (_WIN32_IE >= 0x0400)
            HANDLE_CASE_RETURN_STRING(RBN_GETOBJECT);
            HANDLE_CASE_RETURN_STRING(RBN_LAYOUTCHANGED);
            HANDLE_CASE_RETURN_STRING(RBN_AUTOSIZE);
            HANDLE_CASE_RETURN_STRING(RBN_BEGINDRAG);
            HANDLE_CASE_RETURN_STRING(RBN_ENDDRAG);
            HANDLE_CASE_RETURN_STRING(RBN_DELETINGBAND);
            HANDLE_CASE_RETURN_STRING(RBN_DELETEDBAND);
            HANDLE_CASE_RETURN_STRING(RBN_CHILDSIZE);
#if (_WIN32_IE >= 0x0500)
            HANDLE_CASE_RETURN_STRING(RBN_CHEVRONPUSHED);
#endif      // _WIN32_IE >= 0x0500
#if (_WIN32_IE >= 0x0600)
            HANDLE_CASE_RETURN_STRING(RBN_SPLITTERDRAG);
#endif      // _WIN32_IE >= 0x0600
#if (_WIN32_IE >= 0x0500)
            HANDLE_CASE_RETURN_STRING(RBN_MINMAX);
#endif      // _WIN32_IE >= 0x0500
#if (_WIN32_WINNT >= 0x0501)
            HANDLE_CASE_RETURN_STRING(RBN_AUTOBREAK);
#endif //0x0501
#endif //0x0400


#if (_WIN32_IE >= 0x0400)
            // internet address
            HANDLE_CASE_RETURN_STRING(IPN_FIELDCHANGED);

            // status bar
            HANDLE_CASE_RETURN_STRING(SBN_SIMPLEMODECHANGE);

            // Pager Control
            HANDLE_CASE_RETURN_STRING(PGN_SCROLL);
            HANDLE_CASE_RETURN_STRING(PGN_CALCSIZE);
            HANDLE_CASE_RETURN_STRING(PGN_HOTITEMCHANGE);
#endif //_WIN32_IE >= 0x0400

#if (_WIN32_IE >= 0x0500)
            //WMN_FIRST
#endif //_WIN32_IE >= 0x0500

#if (_WIN32_WINNT >= 0x0501)
            //BCN_FIRST
#  ifndef NM_GETCUSTOMSPLITRECT
#    define NM_GETCUSTOMSPLITRECT       (BCN_FIRST + 0x0003)
#  endif
            HANDLE_CASE_RETURN_STRING(NM_GETCUSTOMSPLITRECT);
            HANDLE_CASE_RETURN_STRING(BCN_HOTITEMCHANGE);
#endif //_WIN32_WINNT >= 0x0501

#if (_WIN32_WINNT >= 0x600)
            HANDLE_CASE_RETURN_STRING(BCN_DROPDOWN);
#endif // _WIN32_WINNT >= 0x600


#if (_WIN32_WINNT >= 0x0600)
            // trackbar
            HANDLE_CASE_RETURN_STRING(TRBN_THUMBPOSCHANGING);
#endif //_WIN32_WINNT >= 0x0600
        }

        FTLTRACEEX(FTL::tlWarn, TEXT("Unknown Notify Code, %d\n"), nCode);
        return TEXT("Unknown");
    }

    LPCTSTR CFWinUtil::GetCommandNotifyString(HWND hWnd, UINT nCode, LPTSTR pszCommandNotify, int nLength, 
        TranslateWndClassProc pTransProc/* = g_pTranslateWndClassProc*/)
    {
        CHECK_POINTER_RETURN_VALUE_IF_FAIL(pszCommandNotify, NULL);
        BOOL bRet = FALSE;
        HRESULT hr = E_FAIL;

        pszCommandNotify[0] = 0;
        TCHAR szClassName[FTL_MAX_CLASS_NAME_LENGTH] = {0};
        API_VERIFY( 0 != GetClassName(hWnd, szClassName, _countof(szClassName)));
        if (pTransProc)
        {
            TCHAR szNewClassName[FTL_MAX_CLASS_NAME_LENGTH] = {0};
            bRet = (*pTransProc)(szClassName, szNewClassName, _countof(szNewClassName));
            if (bRet)
            {
                FTLTRACEEX(FTL::tlInfo, TEXT("Translate Window Class Name From %s to %s\n"), szClassName, szNewClassName);
                StringCchCopy(szClassName, _countof(szClassName), szNewClassName);
            }
        }
        if (0 == lstrcmpi(szClassName, TEXT("Button")))
        {
            switch(nCode)
            {
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_CLICKED);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_PAINT);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_HILITE);	//BN_PUSHED
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_UNHILITE);	//BN_UNPUSHED
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_DISABLE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_DOUBLECLICKED);	//BN_DBLCLK
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_SETFOCUS);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, BN_KILLFOCUS);
            default:
                break;
            }
        }
        else if (0 == lstrcmpi(szClassName, TEXT("ComboBox")))
        {
            //Combo Box Notification Codes
            switch(nCode)
            {
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_ERRSPACE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_SELCHANGE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_DBLCLK);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_SETFOCUS);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_KILLFOCUS);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_EDITCHANGE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_EDITUPDATE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_DROPDOWN);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_CLOSEUP);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_SELENDOK);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, CBN_SELENDCANCEL);
            default:
                break;
            }
        }
        else if (0 == lstrcmpi(szClassName, TEXT("ListBox")))
        {
            //Combo Box Notification Codes
            switch(nCode)
            {
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, LBN_SELCHANGE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, LBN_DBLCLK);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, LBN_SELCANCEL);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, LBN_SETFOCUS);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, LBN_KILLFOCUS);
            default:
                break;
            }
        }
        else if (0 == lstrcmpi(szClassName, TEXT("Edit")))
        {
            switch(nCode)
            {
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_SETFOCUS);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_KILLFOCUS);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_CHANGE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_UPDATE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_ERRSPACE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_MAXTEXT);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_HSCROLL);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_VSCROLL);
#if(_WIN32_WINNT >= 0x0500)
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_ALIGN_LTR_EC);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_ALIGN_RTL_EC);
#endif /* _WIN32_WINNT >= 0x0500 */
            default:
                break;
            }
        }
        else if(0 == lstrcmpi(szClassName, TEXT("RichEdit20W")))
        {
#ifdef _RICHEDIT_
            switch (nCode)
            {
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_MSGFILTER);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_REQUESTRESIZE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_SELCHANGE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_DROPFILES);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_PROTECTED);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_CORRECTTEXT);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_STOPNOUNDO);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_IMECHANGE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_SAVECLIPBOARD);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_OLEOPFAILED);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_OBJECTPOSITIONS);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_LINK);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_DRAGDROPDONE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_PARAGRAPHEXPANDED);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_PAGECHANGE);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_LOWFIRTF);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_ALIGNLTR);
                HANDLE_CASE_TO_STRING(pszCommandNotify, nLength, EN_ALIGNRTL);
            default:
                //StringCchCopy(pszCommandNotify,nLength,TEXT("Unknown RichEdit Notify"));
                break;
            }
#endif	//_RICHEDIT_
        }
        //Toolbar
        else if(0 == lstrcmpi(szClassName, TEXT("ToolbarWindow"))
            || (0 == lstrcmpi(szClassName, TEXT("ToolbarWindow32")))
            )

            if ( 0 == pszCommandNotify[0] )
            {
                FTLTRACEEX(FTL::tlWarn, TEXT("Warning -- Unknown Command Code %d For Class %s\n"), nCode, szClassName);
                COM_VERIFY(StringCchPrintf(pszCommandNotify, nLength, TEXT("Unknown Command Code %d For Class %s"), nCode, szClassName));
                //FTLASSERT(FALSE);
            }
            return pszCommandNotify;
    }

    LPCTSTR CFWinUtil::GetSysCommandString(UINT nCode)
    {
#ifndef SC_CLICK_CAPTION            //鼠标左键点击标题栏
#  define SC_CLICK_CAPTION 0xF012
#endif 
#ifndef SC_CLICK_CAPTION_ICON       //鼠标左键点击标题栏上的图标
#  define SC_CLICK_CAPTION_ICON  0XF093
#endif 
        switch(nCode)// & 0xFFF0)
        {
            HANDLE_CASE_RETURN_STRING(SC_SIZE);
            HANDLE_CASE_RETURN_STRING(SC_MOVE);
            HANDLE_CASE_RETURN_STRING(SC_MINIMIZE);
            HANDLE_CASE_RETURN_STRING(SC_MAXIMIZE);
            HANDLE_CASE_RETURN_STRING(SC_NEXTWINDOW);
            HANDLE_CASE_RETURN_STRING(SC_PREVWINDOW);
            HANDLE_CASE_RETURN_STRING(SC_CLOSE);
            HANDLE_CASE_RETURN_STRING(SC_VSCROLL);
            HANDLE_CASE_RETURN_STRING(SC_HSCROLL);
            HANDLE_CASE_RETURN_STRING(SC_MOUSEMENU);
            HANDLE_CASE_RETURN_STRING(SC_KEYMENU);
            HANDLE_CASE_RETURN_STRING(SC_ARRANGE);
            HANDLE_CASE_RETURN_STRING(SC_RESTORE);
            HANDLE_CASE_RETURN_STRING(SC_TASKLIST);
            HANDLE_CASE_RETURN_STRING(SC_SCREENSAVE);
            HANDLE_CASE_RETURN_STRING(SC_HOTKEY);

#if(WINVER >= 0x0400)
            HANDLE_CASE_RETURN_STRING(SC_DEFAULT);
            HANDLE_CASE_RETURN_STRING(SC_MONITORPOWER);
            HANDLE_CASE_RETURN_STRING(SC_CONTEXTHELP);
            HANDLE_CASE_RETURN_STRING(SC_SEPARATOR);
#endif /* WINVER >= 0x0400 */

            HANDLE_CASE_RETURN_STRING(SC_CLICK_CAPTION);
            HANDLE_CASE_RETURN_STRING(SC_CLICK_CAPTION_ICON);
        default:
            FTLTRACEEX(FTL::tlWarn, TEXT("Unknown SysCommand, 0x%x\n"), nCode);
            FTLASSERT(FALSE);
            return TEXT("Unknown");
        }
    }

    LPCTSTR CFWinUtil::GetWindowDescriptionInfo(FTL::CFStringFormater& formater, HWND hWnd)
    {
        BOOL bRet = FALSE;
        HRESULT hr = E_FAIL;
        if (::IsWindow(hWnd))
        {
            TCHAR szClass[FTL_MAX_CLASS_NAME_LENGTH] = {0};
            API_VERIFY(0 != GetClassName(hWnd, szClass, _countof(szClass)));

            TCHAR szName[FTL_MAX_CLASS_NAME_LENGTH] = {0};
            API_VERIFY_EXCEPT1(0 != GetWindowText(hWnd, szName, _countof(szName)), ERROR_SUCCESS);

            RECT rcWindow = {0};
            API_VERIFY(GetWindowRect(hWnd, &rcWindow));

            COM_VERIFY(formater.Format(TEXT("0x%x(%d), Class=%s, Name=%s, WinPos=(%d,%d)-(%d,%d) %dx%d"),
                hWnd, hWnd, szClass, szName, 
                rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom,
                rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top));
        }
        else
        {
            COM_VERIFY(formater.Format(TEXT("0x%x(%d) NOT valid Window"), hWnd, hWnd));
        }

        return formater.GetString();
    }

    LPCTSTR CFWinUtil::GetWindowClassString(FTL::CFStringFormater& formater, HWND hWnd,LPCTSTR pszDivide/* = TEXT("|") */)
    {
        FTLASSERT(::IsWindow(hWnd));
        ULONG_PTR clsStyle = ::GetClassLongPtr(hWnd,GCL_STYLE);
        API_ASSERT(clsStyle != 0);
        ULONG_PTR oldClsStyle = clsStyle;
		UNREFERENCED_PARAMETER(oldClsStyle);

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_VREDRAW, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_HREDRAW, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_DBLCLKS, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_OWNDC, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_CLASSDC, pszDivide);

        //* 如子窗口设置了 CS_PARENTDC 属性,它可在其父窗口的显示设备上下文上进行绘制 -- 如 Edit/Button
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_PARENTDC, pszDivide);

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_NOCLOSE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_SAVEBITS, pszDivide);
        //在字节边界上定位窗口的用户区域的位置 -- 有什么用？
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_BYTEALIGNCLIENT, pszDivide);
        //在字节边界上定位窗口的位置 -- 有什么用？
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_BYTEALIGNWINDOW, pszDivide);
        //应用程序全局的窗体类--可以被Exe或Dll注册，对进程内所有模块都有效，通常在提供UI的DLL中注册
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_GLOBALCLASS, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_IME, pszDivide);
#if(_WIN32_WINNT >= 0x0501)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, clsStyle, CS_DROPSHADOW, pszDivide);
#endif /* _WIN32_WINNT >= 0x0501 */

        FTLASSERT( 0 == clsStyle);
        //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, XXXXXXXXX, pszDivide);
        if (0 != clsStyle)
        {
            FTLTRACEEX(FTL::tlWarn, TEXT("%s:Check Class String For HWND(0x%x) Not Complete, total=0x%x, remain=0x%x\n"),
                __FILE__LINE__, hWnd, oldClsStyle, clsStyle);
        }

        return formater.GetString();
    }

    LPCTSTR CFWinUtil::GetWindowStyleString(FTL::CFStringFormater& formater, HWND hWnd,LPCTSTR pszDivide/* = TEXT("|") */)
    {
        BOOL bRet = FALSE;

        FTLASSERT(::IsWindow(hWnd));
        LONG_PTR    lStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
        LONG_PTR    lOldStyle = lStyle;
		UNREFERENCED_PARAMETER(lOldStyle);

        //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_OVERLAPPED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_POPUP, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_CHILDWINDOW, pszDivide); //WS_CHILD
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_MINIMIZE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_VISIBLE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_DISABLED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_CLIPSIBLINGS, pszDivide); //兄弟子窗口互相裁剪(只用于WS_CHILD)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_CLIPCHILDREN, pszDivide); //父窗口中不绘制子窗口(裁剪视频播放窗体)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_MAXIMIZE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_CAPTION, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_BORDER, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_DLGFRAME, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_VSCROLL, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_HSCROLL, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_SYSMENU, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_THICKFRAME, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_GROUP, pszDivide);      //WS_MINIMIZEBOX
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, WS_TABSTOP, pszDivide);    //WS_MAXIMIZEBOX

        //通用的类型都大于 0x00010000L， 各种标准控件特有的 Style 小于 0x00010000L

        TCHAR szClassName[256+1] = {0}; //The maximum length for lpszClassName is 256.
        API_VERIFY(0 != ::GetClassName(hWnd, szClassName, _countof(szClassName)));

        //BUTTON
        if (0 ==  lstrcmpi(szClassName, WC_BUTTON))
        {
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_LEFTTEXT, pszDivide);

            if (0 != (lStyle & BS_TYPEMASK))
            {
#if _WIN32_WINNT >= 0x0600
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_DEFCOMMANDLINK, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_COMMANDLINK, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_DEFSPLITBUTTON, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_SPLITBUTTON, pszDivide);
#endif //_WIN32_WINNT >= 0x0600
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_OWNERDRAW, pszDivide); //按钮的自绘
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_PUSHBOX, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_AUTORADIOBUTTON, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_USERBUTTON, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_GROUPBOX, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_AUTO3STATE, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_3STATE, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_RADIOBUTTON, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_AUTOCHECKBOX, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_CHECKBOX, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_DEFPUSHBUTTON, pszDivide);
            }
            else
            {
                //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_PUSHBUTTON, pszDivide);
                formater.AppendFormat(TEXT("%s%s"), TEXT("BS_PUSHBUTTON"), pszDivide);
            }

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_FLAT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_NOTIFY, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_MULTILINE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_PUSHLIKE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_VCENTER, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_BOTTOM, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_TOP, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_CENTER, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_RIGHT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_LEFT, pszDivide);

            if ( 0 != (lStyle & (BS_ICON|BS_TEXT)))
            {
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_BITMAP, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_ICON, pszDivide);
            }
            else
            {
                //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, BS_TEXT, pszDivide);
                formater.AppendFormat(TEXT("%s%s"), TEXT("BS_TEXT"), pszDivide);
            }
        }

        //Combo Box styles
        if (0 ==  lstrcmp(szClassName, WC_COMBOBOX))
        {
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_DROPDOWNLIST, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_DROPDOWN, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_SIMPLE, pszDivide);

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_OWNERDRAWFIXED, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_OWNERDRAWVARIABLE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_AUTOHSCROLL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_OEMCONVERT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_SORT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_HASSTRINGS, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_NOINTEGRALHEIGHT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_DISABLENOSCROLL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_UPPERCASE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, CBS_LOWERCASE, pszDivide);
        }

#ifdef _RICHEDIT_
        if (0 ==  lstrcmp(szClassName, WC_EDIT)                     //Edit Control Styles
            || 0 == lstrcmp(szClassName, TEXT("RICHEDIT50W")))      //Rich Edit Control Styles
        {
#define ES_ALIGNMASK	(ES_LEFT | ES_CENTER | ES_RIGHT)
            if ( 0 != (lStyle & ES_ALIGNMASK))
            {
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_RIGHT, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_CENTER, pszDivide);
            }
            else
            {
                //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_LEFT, pszDivide);
                formater.AppendFormat(TEXT("%s%s"), TEXT("ES_LEFT"), pszDivide);
            }

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_MULTILINE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_UPPERCASE | ES_NOOLEDRAGDROP, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_LOWERCASE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_PASSWORD, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_AUTOVSCROLL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_AUTOHSCROLL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_NOHIDESEL, pszDivide);

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_OEMCONVERT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_READONLY, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_WANTRETURN, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_NUMBER | ES_DISABLENOSCROLL, pszDivide); //两个的值一样，为什么
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_SUNKEN, pszDivide); 
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_SAVESEL, pszDivide);

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_NOIME, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_VERTICAL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, ES_SELECTIONBAR, pszDivide);
        }
#endif //_RICHEDIT_

        //ListView
        if (0 ==  lstrcmp(szClassName, WC_LISTVIEW))
        {
            if (0 != (lStyle & LVS_TYPEMASK))
            {
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_LIST, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_SMALLICON, pszDivide);
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_REPORT, pszDivide);
            }
            else
            {
                //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_ICON, pszDivide);
                formater.AppendFormat(TEXT("%s%s"), TEXT("LVS_ICON"), pszDivide);
            }

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_SINGLESEL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_SHOWSELALWAYS, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_SORTASCENDING, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_SORTDESCENDING, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_SHAREIMAGELISTS, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_NOLABELWRAP, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_AUTOARRANGE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_EDITLABELS, pszDivide);

            //序列表(虚拟列表)技术 -- 在显示的时候才获取具体的信息，可以大幅减少UI资源的消耗
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_OWNERDATA, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_NOSCROLL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_TYPESTYLEMASK, pszDivide);

            if (0 != (lStyle & LVS_ALIGNMASK ))
            {
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_ALIGNLEFT, pszDivide);
            }
            else
            {
                //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_ALIGNTOP, pszDivide);
                formater.AppendFormat(TEXT("%s%s"), TEXT("LVS_ALIGNTOP"), pszDivide);
            }

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_NOSORTHEADER, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_NOCOLUMNHEADER, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, LVS_OWNERDRAWFIXED, pszDivide);
        }

        //Scroll Bar Styles
        if (0 ==  lstrcmp(szClassName, WC_SCROLLBAR))
        {
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, SBS_SIZEGRIP, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, SBS_SIZEBOX, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, SBS_BOTTOMALIGN|SBS_RIGHTALIGN|SBS_SIZEBOXBOTTOMRIGHTALIGN, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, SBS_TOPALIGN|SBS_LEFTALIGN|SBS_SIZEBOXTOPLEFTALIGN, pszDivide);
            if (0 != (lStyle & SBS_VERT))
            {
                HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, SBS_VERT, pszDivide);
            }
            else
            {
                //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, SBS_HORZ, pszDivide);
                formater.AppendFormat(TEXT("%s%s"), TEXT("SBS_HORZ"), pszDivide);
            }
        }

        //SysHeader32 Box styles
        if (0 ==  lstrcmp(szClassName, WC_HEADER))
        {
            //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_HORZ, pszDivide);
            formater.AppendFormat(TEXT("%s%s"), TEXT("HDS_HORZ"), pszDivide);

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_BUTTONS, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_HOTTRACK, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_HIDDEN, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_DRAGDROP, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_FULLDRAG, pszDivide);
#if (_WIN32_IE >= 0x0500)
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_FILTERBAR, pszDivide);
#endif //_WIN32_IE >= 0x0500

#if (_WIN32_WINNT >= 0x0501)
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_FLAT, pszDivide);
#endif //_WIN32_WINNT >= 0x0501
#if _WIN32_WINNT >= 0x0600
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_CHECKBOXES, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_NOSIZING, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, HDS_OVERFLOW, pszDivide);
#endif
        }

        //Dialog style
        if (0 == lstrcmp(szClassName, _T("#32770")))
        {
            formater.AppendFormat(TEXT("%s%s"), TEXT("Dialog"), pszDivide);

            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_ABSALIGN, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_SYSMODAL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_LOCALEDIT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_SETFONT, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_MODALFRAME, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_NOIDLEMSG, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_SETFOREGROUND, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_3DLOOK, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_FIXEDSYS, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_NOFAILCREATE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_CONTROL, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_CENTER, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_CENTERMOUSE, pszDivide);
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_CONTEXTHELP, pszDivide);
            //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_SHELLFONT, pszDivide);  //(DS_SETFONT | DS_FIXEDSYS)
#if(_WIN32_WCE >= 0x0500)
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, DS_USEPIXELS, pszDivide);
#endif
        }

        FTLASSERT( 0 == lStyle);
        //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lStyle, XXXXXXXXX, pszDivide);
        if (0 != lStyle)
        {
            FTLTRACEEX(FTL::tlWarn, TEXT("%s:Check Style String For \"%s\" Not Complete, total=0x%08x, remain=0x%08x\n"),
                __FILE__LINE__, szClassName, lOldStyle, lStyle);
        }
        return formater.GetString();
    }

    LPCTSTR CFWinUtil::GetWindowExStyleString(FTL::CFStringFormater& formater, HWND hWnd, LPCTSTR pszDivide /* = TEXT */)
    {
        FTLASSERT(::IsWindow(hWnd));
        LONG_PTR    lExStyle = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        LONG_PTR    lOldExStyle = lExStyle;

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_DLGMODALFRAME, pszDivide);
        //当该窗体(Child Window)创建或销毁时不会给父窗体发送 WM_PARENTNOTIFY 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_NOPARENTNOTIFY, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_TOPMOST, pszDivide);
        //可以接收 drag-and-drop 文件(会收到 WM_DROPFILES 消息)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_ACCEPTFILES, pszDivide);
        //透明的，不会掩盖其下方的窗体，鼠标消息会穿透此窗体(典型应用是阴影)，会在其下发的所有窗体都更新完毕后收到 WM_PAINT 消息
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_TRANSPARENT, pszDivide);	
#if(WINVER >= 0x0400)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_MDICHILD, pszDivide);

        //通常用于浮动工具条(floating toolbar) -- 小的标题栏，不出现在任务栏和 Alt+Tab 列表中
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_TOOLWINDOW, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_WINDOWEDGE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_CLIENTEDGE, pszDivide);	//有3D客户区外观，即有一个凹边(sunken edge)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_CONTEXTHELP, pszDivide);
        if (0 != (lExStyle & WS_EX_RIGHT) )
        {
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_RIGHT, pszDivide);
        }
        else
        {
            //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_LEFT, pszDivide);
            formater.AppendFormat(TEXT("%s%s"), TEXT("WS_EX_LEFT"), pszDivide);
        }
        if (0 != (lExStyle & WS_EX_RTLREADING))
        {
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_RTLREADING, pszDivide);
        }
        else
        {
            //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_LTRREADING, pszDivide);
            formater.AppendFormat(TEXT("%s%s"), TEXT("WS_EX_LTRREADING"), pszDivide);
        }
        if (0 != (lExStyle & WS_EX_LEFTSCROLLBAR))
        {
            HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_LEFTSCROLLBAR, pszDivide);
        }
        else
        {
            //HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_RIGHTSCROLLBAR, pszDivide);
            formater.AppendFormat(TEXT("%s%s"), TEXT("WS_EX_RIGHTSCROLLBAR"), pszDivide);
        }
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_CONTROLPARENT, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_STATICEDGE, pszDivide);

        //当激活时，任务条上会出现Top-Level窗体，可用于全屏窗体，保证窗体在最前面
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_APPWINDOW, pszDivide);	
#endif /* WINVER >= 0x0400 */

#if(_WIN32_WINNT >= 0x0500)
        //创建分层窗口
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_LAYERED, pszDivide);
#endif /* _WIN32_WINNT >= 0x0500 */

#if(WINVER >= 0x0500)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_NOINHERITLAYOUT, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_LAYOUTRTL, pszDivide);
#endif /* WINVER >= 0x0500 */

#if(_WIN32_WINNT >= 0x0501)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_COMPOSITED, pszDivide);
#endif /* _WIN32_WINNT >= 0x0501 */

#if(_WIN32_WINNT >= 0x0500)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, lExStyle, WS_EX_NOACTIVATE, pszDivide);
#endif /* _WIN32_WINNT >= 0x0500 */

        FTLASSERT( 0 == lExStyle);
        if (0 != lExStyle)
        {
            FTLTRACEEX(FTL::tlWarn, TEXT("%s: Check ExStyle String Not Complete, total=0x%08x, remain=0x%08x\n"),
                __FILE__LINE__, lOldExStyle, lExStyle);
        }
        return formater.GetString();
    }

    LPCTSTR CFWinUtil::GetWindowPosFlagsString(FTL::CFStringFormater& formater, UINT flags, LPCTSTR pszDivide /* = TEXT */)
    {
        UINT    nOldFlags = flags;

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOSIZE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOMOVE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOZORDER, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOREDRAW, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOACTIVATE, pszDivide);

        //更改新的边框类型(比如 DUILib中去掉 WS_CAPTION 后)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_FRAMECHANGED, pszDivide);

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_SHOWWINDOW, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_HIDEWINDOW, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOCOPYBITS, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOOWNERZORDER, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOSENDCHANGING, pszDivide);


#if(WINVER >= 0x0400)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_DEFERERASE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_ASYNCWINDOWPOS, pszDivide);
#endif /* WINVER >= 0x0400 */

#ifndef SWP_NOCLIENTSIZE 
#  define SWP_NOCLIENTSIZE  0x0800		//Undocumented flags
#endif 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOCLIENTSIZE, pszDivide);

#ifndef SWP_NOCLIENTMOVE
#  define SWP_NOCLIENTMOVE	0x1000		//Undocumented flags
#endif 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_NOCLIENTMOVE, pszDivide);

#ifndef SWP_STATECHANGED  // window state (e.g. minimized, normalized, maximized) is changing or has changed,
#  define SWP_STATECHANGED  0x8000		//Undocumented flags
#endif 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_STATECHANGED, pszDivide);

#ifndef SWP_UNKNOWN_0X01000000
#  define SWP_UNKNOWN_0X01000000  0x01000000	//Undocumented flags
#endif 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_UNKNOWN_0X01000000, pszDivide);

#ifndef SWP_UNKNOWN_0X10000000
#  define SWP_UNKNOWN_0X10000000  0x10000000	//Undocumented flags
#endif 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_UNKNOWN_0X10000000, pszDivide);

#ifndef SWP_UNKNOWN_0X20000000
#  define SWP_UNKNOWN_0X20000000  0x20000000	//Undocumented flags
#endif 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_UNKNOWN_0X20000000, pszDivide);

#ifndef SWP_UNKNOWN_0X40000000
#  define SWP_UNKNOWN_0X40000000  0x40000000	//Undocumented flags
#endif 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, flags, SWP_UNKNOWN_0X40000000, pszDivide);

        FTLASSERT( 0 == flags);
        if (0 != flags)
        {
            FTLTRACEEX(FTL::tlWarn, TEXT("%s: WARNING, Check Set Window Pos String Not Complete, total=0x%08x, remain=0x%08x\n"),
                __FILE__LINE__, nOldFlags, flags);
        }
        return formater.GetString();
    }

    LPCTSTR CFWinUtil::GetOwnerDrawState(FTL::CFStringFormater& formater, UINT itemState, LPCTSTR pszDivide)
    {
        UINT    oldItemState = itemState;

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_SELECTED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_GRAYED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_DISABLED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_CHECKED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_FOCUS, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_DEFAULT, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_COMBOBOXEDIT, pszDivide);
#if(WINVER >= 0x0500)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_HOTLIGHT, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_INACTIVE, pszDivide);
#if(_WIN32_WINNT >= 0x0500)
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_NOACCEL, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemState, ODS_NOFOCUSRECT, pszDivide);
#endif /* _WIN32_WINNT >= 0x0500 */
#endif /* WINVER >= 0x0500 */

        FTLASSERT( 0 == itemState);
        if (0 != itemState)
        {
            FTLTRACEEX(FTL::tlWarn, TEXT("%s: GetOwnerDrawState Not Complete, total=0x%08x, remain=0x%08x\n"),
                __FILE__LINE__, oldItemState, itemState);
        }
        return formater.GetString();
    }


    LPCTSTR CFWinUtil::GetOwnerDrawAction(CFStringFormater& formater, UINT itemAction, LPCTSTR pszDivide)
    {
        //UINT    oldItemAction = itemAction;

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemAction, ODA_DRAWENTIRE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemAction, ODA_SELECT, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, itemAction, ODA_FOCUS, pszDivide);

        FTLASSERT(0 == itemAction);
        return formater.GetString();
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    BOOL CFMenuUtil::DumpMenuInfo(HMENU hMenu, BOOL bDumpChild, int nLevel/* = 0*/)
    {
        BOOL bRet = FALSE;

        FTLASSERT(IsMenu(hMenu));

        MENUINFO menuInfo = { sizeof(menuInfo) };
        menuInfo.fMask = MIM_MAXHEIGHT | MIM_BACKGROUND | MIM_HELPID | MIM_MENUDATA | MIM_STYLE;
        API_VERIFY(::GetMenuInfo(hMenu, &menuInfo));
        if (bRet)
        {
            int nItemCount = ::GetMenuItemCount(hMenu);
            API_ASSERT(nItemCount != -1);

            FTLTRACE(TEXT("itemCount=%d\n"), nItemCount);
            //MenuInfo: Mask=0x%x, dwStyle=0x%x, cyMax=%d, hbrBack=0x%x, helpId=%d, menuData=0x%x\n"),
            //    nItemCount, menuInfo.fMask, menuInfo.dwStyle, menuInfo.cyMax, menuInfo.hbrBack,
            //    menuInfo.dwContextHelpID, menuInfo.dwMenuData);

            if (bDumpChild && nItemCount > 0)
            {
                CFMemAllocator<TCHAR> menuText;

                UINT fCheckMask =  MIIM_STRING | MIIM_SUBMENU; 
                // MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA ; 
                // MIIM_BITMAP | MIIM_FTYPE;
                for (int i = 0; i < nItemCount; i++)
                {
                    API_VERIFY(_GetMenuText(hMenu, i , MF_BYPOSITION, menuText));

                    UINT nState = 0;
                    MENUITEMINFO menuItemInfo = { sizeof(menuItemInfo) };
                    menuItemInfo.fMask = fCheckMask;
                    API_VERIFY(::GetMenuItemInfo(hMenu, i, TRUE, &menuItemInfo));
                    API_VERIFY(-1 != (nState = ::GetMenuState(hMenu, i , MF_BYPOSITION)));
                    if (bRet)
                    {
                        CFStringFormater formaterSpace(nLevel + 1);
                        formaterSpace.AppendFormat(TEXT(""));
                        for (int j = 0; j < nLevel; j++)
                        {
                            formaterSpace.AppendFormat(TEXT(" "));
                        }
                        CFStringFormater InfoFormater, stateFormater;

                        FTLTRACE(TEXT("%sItem[%d]:%s, Info=%s, state=%s\n"), formaterSpace.GetString(), i, menuText.GetMemory(), 
                            GetMenuItemInfoString(InfoFormater, menuItemInfo), 
                            GetMenuStateString(stateFormater, nState));

                        if (menuItemInfo.hSubMenu != NULL)
                        {
                            DumpMenuInfo(menuItemInfo.hSubMenu, bDumpChild, ++nLevel);
                        }
                        else
                        {
                            //((menuItemInfo.fType & MFT_SEPARATOR) == MFT_SEPARATOR)
                            //normal menu or separator

                            //FTL::CFStringFormater formater;
                            //FTLTRACE(TEXT("Item:%s, %s\n"), menuText.GetMemory(), 
                            //    GetMenuItemInfoString(formater, menuItemInfo));
                        }
                    }
                }
            }
        }
        return bRet;
    }

    LPCTSTR CFMenuUtil::GetMenuItemInfoString(FTL::CFStringFormater& formater, const MENUITEMINFO& menuItemInfo)
    {
        CFStringFormater formaterMask;

        formater.Format(_T("Mask=%s"), 
            //menuItemInfo.cbSize, 
            GetMenuItemInfoMaskString(formaterMask, menuItemInfo.fMask));

        if (MIIM_FTYPE == (menuItemInfo.fMask & MIIM_FTYPE))
        {
            CFStringFormater formaterType;
            formater.AppendFormat(TEXT(", Type=0x%x(%s)"), menuItemInfo.fType,
                GetMenuItemInfoTypeString(formaterType, menuItemInfo.fType));
        }
        if (MIIM_STATE == (menuItemInfo.fMask & MIIM_STATE))
        {
            CFStringFormater formaterState;
            formater.AppendFormat(TEXT(", State=0x%x(%s)"), menuItemInfo.fState,
                GetMenuItemInfoTypeString(formaterState, menuItemInfo.fState));
        }
        if (MIIM_ID == (menuItemInfo.fMask & MIIM_ID))
        {
            formater.AppendFormat(TEXT(", Id=0x%x(%d)"), menuItemInfo.wID, menuItemInfo.wID);
        }
        if (MIIM_SUBMENU == (menuItemInfo.fMask & MIIM_SUBMENU))
        {
            formater.AppendFormat(TEXT(", subMenu=0x%x"), menuItemInfo.hSubMenu);
        }
        if (MIIM_CHECKMARKS == (menuItemInfo.fMask & MIIM_CHECKMARKS))
        {
            formater.AppendFormat(TEXT(", check=0x%x/0x%x"), menuItemInfo.hbmpChecked, menuItemInfo.hbmpUnchecked);
        }
        if (MIIM_DATA == (menuItemInfo.fMask & MIIM_DATA))
        {
            formater.AppendFormat(TEXT(", itemData=0x%x"), menuItemInfo.dwItemData);
        }
        if (MIIM_TYPE == (menuItemInfo.fMask & MIIM_TYPE))
        {
            formater.AppendFormat(TEXT(", typeData=0x%x, cch=%d"), menuItemInfo.dwTypeData, menuItemInfo.cch);
        }

        return formater.GetString();
    }

    LPCTSTR CFMenuUtil::GetMenuItemInfoMaskString(FTL::CFStringFormater& formater, UINT fMask, LPCTSTR pszDivide/* = TEXT("|")*/)
    {
        //UINT    oldMask = fMask;

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_STATE, pszDivide);         //fState
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_ID, pszDivide);            //wID
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_SUBMENU, pszDivide);       //hSubMenu
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_CHECKMARKS, pszDivide);    //hbmpChecked + hbmpUnchecked 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_TYPE, pszDivide);          //fType + dwTypeData(老标志,不再使用) 
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_DATA, pszDivide);          //dwItemData

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_STRING, pszDivide);        //dwTypeData
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_BITMAP, pszDivide);        //hbmpItem
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fMask, MIIM_FTYPE, pszDivide);         //fType

        FTLASSERT(0 == fMask);
        return formater.GetString();
    }

    LPCTSTR CFMenuUtil::GetMenuItemInfoTypeString(FTL::CFStringFormater& formater, UINT fType, LPCTSTR pszDivide/* = TEXT("|")*/)
    {
        FTLASSERT((fType & MFT_BITMAP) == 0); //Win9X 后应该已经不使用该标志位
        FTLASSERT((fType & MFT_STRING) == 0); //Win9X 后应该已经不使用该标志位

        //UINT    oldType = fType;
        //注意: MFT_BITMAP, MFT_SEPARATOR, MFT_STRING 不能组合使用

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_STRING, pszDivide);         //Win9X后已被MIIM_STRING代替

        //Win31中使用(dwTypeData低字节是位图句柄), Win9X后已被 MIIM_BITMAP + hbmpItem 代替
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_BITMAP, pszDivide);

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_MENUBARBREAK, pszDivide);   //竖线分割
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_MENUBREAK, pszDivide);      //不被竖线分隔
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_OWNERDRAW, pszDivide);      //自绘菜单项
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_RADIOCHECK, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_SEPARATOR, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_RIGHTORDER, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fType, MFT_RIGHTJUSTIFY, pszDivide);   //只在MenuBar中生效的右对齐

        FTLASSERT(0 == fType);
        return formater.GetString();
    }

    LPCTSTR CFMenuUtil::GetMenuItemInfoStateString(FTL::CFStringFormater& formater, UINT fState, LPCTSTR pszDivide/* = TEXT("|")*/)
    {
        formater.AppendFormat(TEXT(""));
        //UINT    oldState = fState;
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MFS_DISABLED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MFS_CHECKED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MFS_HILITE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MFS_DEFAULT, pszDivide);

        FTLASSERT(0 == fState);
        return formater.GetString();
    }

    LPCTSTR CFMenuUtil::GetMenuStateString(FTL::CFStringFormater& formater, UINT fState, LPCTSTR pszDivide /* = TEXT("|")*/)
    {

#pragma TODO(GetMenuState is error for MF_SEPARATOR)
        ATLASSERT(FALSE); //
        UINT nSubItemCount = HIBYTE(fState);        //
        fState = LOBYTE(fState);
        formater.AppendFormat(TEXT("SubItemCount=%d,"), nSubItemCount);  //当有 MF_SEPARATOR 时, 会误认为是 8 -- 设计有问题

        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_GRAYED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_DISABLED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_CHECKED, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_POPUP, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_MENUBARBREAK, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_MENUBREAK, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_HILITE, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_OWNERDRAW, pszDivide);
        HANDLE_COMBINATION_VALUE_TO_STRING(formater, fState, MF_SEPARATOR, pszDivide);

        FTLASSERT(0 == fState);
        return formater.GetString();
    }

    BOOL CFMenuUtil::_GetMenuText(HMENU hMenu, UINT nIDItem, UINT nFlags, CFMemAllocator<TCHAR>& menuText)
    {
        BOOL bRet = FALSE;

        int nStringLen = ::GetMenuString(hMenu, nIDItem, NULL, 0, nFlags);
        API_VERIFY(nStringLen >= 0);
        if (bRet)
        {
            //MF_SEPARATOR
            API_VERIFY_EXCEPT1(::GetMenuString(hMenu, nIDItem, menuText.GetMemory(nStringLen + 1), nStringLen + 1, nFlags) > 0, ERROR_SUCCESS);
            if (!bRet && ERROR_SUCCESS == GetLastError())
            {
                //TCHAR* pMenuText = menuText.GetMemory(2);
                //lstrcpyn(pMenuText, _T("-"), 1);
                bRet = TRUE;
            }
        }
        else
        {
            //clear old menu text
            TCHAR* pMenuText = menuText.GetMemory(1);
            FTLASSERT(pMenuText);
            if (pMenuText)
            {
                *pMenuText = _T('\0');
            }
        }
        return bRet;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////


    LPCTSTR CFWinHookUtil::GetCBTCodeInfo(CFStringFormater& formater, int nCode, WPARAM wParam, LPARAM lParam)
    {
        //FTLTRACE(TEXT("Enter GetCBTCodeInfo,TickCount=%d, nCode=%d\n"),GetTickCount(), nCode);
        switch(nCode)
        {
            HANDLE_CASE_TO_STRING_FORMATER(formater, HCBT_MOVESIZE);
            HANDLE_CASE_TO_STRING_FORMATER(formater, HCBT_MINMAX);
            HANDLE_CASE_TO_STRING_FORMATER(formater, HCBT_QS);
            HANDLE_CASE_TO_STRING_FORMATER(formater, HCBT_CREATEWND);
            HANDLE_CASE_TO_STRING_FORMATER(formater, HCBT_DESTROYWND);
        case HCBT_ACTIVATE:
            {
                CFStringFormater formaterActivae;
                CFStringFormater formaterActivateStruct;
                formaterActivateStruct.Format(TEXT("%s"), TEXT(""));

                HWND hWndActive = (HWND)wParam;
                CFWinUtil::GetWindowDescriptionInfo(formaterActivae, hWndActive);

                CBTACTIVATESTRUCT * pCBTActivateStruct = (CBTACTIVATESTRUCT*)lParam;
                if (pCBTActivateStruct)
                {
                    CFStringFormater formaterActivaeInStruct;
                    formaterActivateStruct.Format(TEXT("fMouse=%d, hWndActive=%s"),
                        pCBTActivateStruct->fMouse, 
                        CFWinUtil::GetWindowDescriptionInfo(formaterActivaeInStruct, pCBTActivateStruct->hWndActive));
                }
                formater.Format(TEXT("HCBT_ACTIVATE -- Active=%s, Struct=%s"), formaterActivae.GetString(), formaterActivateStruct.GetString());
                break;
            }
            HANDLE_CASE_TO_STRING_FORMATER(formater,HCBT_CLICKSKIPPED);
            HANDLE_CASE_TO_STRING_FORMATER(formater,HCBT_KEYSKIPPED);
            HANDLE_CASE_TO_STRING_FORMATER(formater,HCBT_SYSCOMMAND);
        case HCBT_SETFOCUS:
            {
                HWND hWndGetFocus = (HWND)wParam;
                HWND hWndLostFocus = (HWND)lParam;
                CFStringFormater formaterGetFocus, formaterLostFocus;
                formater.Format(TEXT("HCBT_SETFOCUS -- GetFocus=%s, LostFocus=%s"), 
                    CFWinUtil::GetWindowDescriptionInfo(formaterGetFocus, hWndGetFocus),
                    CFWinUtil::GetWindowDescriptionInfo(formaterLostFocus, hWndLostFocus));
                break;
            }

        default:
            break;
        }
        return formater.GetString();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    __declspec(selectany) HHOOK CFMessageBoxHook::s_hHook = NULL;
    __declspec(selectany) HWND  CFMessageBoxHook::s_ProphWnd = NULL;
    __declspec(selectany) LPCTSTR CFMessageBoxHook::s_pszOKString = NULL;

    CFMessageBoxHook::CFMessageBoxHook(DWORD dwThreadId, LPCTSTR pszOKString)
    {
        BOOL bRet = FALSE;
        CFMessageBoxHook::s_pszOKString = pszOKString;
        s_hHook = ::SetWindowsHookEx(WH_CBT, (HOOKPROC)CFMessageBoxHook::CBTProc
            , NULL, dwThreadId);
        API_VERIFY(NULL != s_hHook);
    }

    CFMessageBoxHook::~CFMessageBoxHook(void)
    {
        if (CFMessageBoxHook::s_ProphWnd)
        {
            RemoveProp(CFMessageBoxHook::s_ProphWnd,PREV_WND_PROC_NAME);
            CFMessageBoxHook::s_ProphWnd = NULL;
        }
        if (CFMessageBoxHook::s_hHook)
        {
            ::UnhookWindowsHookEx(CFMessageBoxHook::s_hHook);
            CFMessageBoxHook::s_hHook = NULL;
        }
    }


    LRESULT CFMessageBoxHook::CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (NULL == CFMessageBoxHook::s_hHook)
            return 0;
        BOOL bRet = FALSE;

        if (nCode == HCBT_CREATEWND){ //HCBT_CREATEWND = 3
            HWND hWnd = (HWND)wParam;
            TCHAR className[MAX_PATH];
            ::GetClassName(hWnd, className, _countof(className));
            if (_tcscmp(className, _T("#32770")) == 0)
            {
                WNDPROC prevWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)CFMessageBoxHook::WndProc);
                API_VERIFY(::SetProp(hWnd, PREV_WND_PROC_NAME, (HANDLE)prevWndProc));
                if (bRet)
                {
                    CFMessageBoxHook::s_ProphWnd = hWnd;
                }
            }
        }
        return ::CallNextHookEx(s_hHook, nCode, wParam, lParam);
    }

    LRESULT CFMessageBoxHook::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        WNDPROC prevWndProc = (WNDPROC)::GetProp(hWnd, PREV_WND_PROC_NAME);
        FTLASSERT(prevWndProc != NULL);

        if (message == WM_INITDIALOG)
        {
            BOOL bRet = FALSE;
            API_VERIFY(SetDlgItemText(hWnd, IDOK, CFMessageBoxHook::s_pszOKString));
        }
        return ::CallWindowProc(prevWndProc, hWnd, message, wParam, lParam);
    }

#if 0
    template <typename ControlT , typename ConverterFun>
    CControlPropertyHandleT<ControlT, ConverterFun>::
        CControlPropertyHandleT(ControlT& control)//, ConverterFun& fun)
        :m_control(control)
        //,m_fun(fun)
    {
    }

    template <typename ControlT , typename ConverterFun>
    INT CControlPropertyHandleT<ControlT, ConverterFun>::AddProperty(INT value)
    {
        INT index = m_control.AddString(ConverterFun(value));
        m_control.SetItemData(value);
        return index;
    }

#endif


}//FTL


#endif //FTL_WINDOW_HPP