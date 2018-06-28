#ifndef FTL_WINDOW_H
#define FTL_WINDOW_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlwindow.h requires ftlbase.h to be included first
#endif

//控制是否允许Dump出ToolTip的消息( WM_USER + 1~60 左右), 一般只有在ToolTip的子类中才使用
#ifndef ENABLE_DUMP_TOOLTIP_MESSAGE
#  define ENABLE_DUMP_TOOLTIP_MESSAGE  0
#endif 

//#include <Wtsapi32.h>
//#pragma comment(lib, "Wtsapi32.lib")
//向导页：http://blog.csdn.net/lk_cool/article/details/4323671

//high-dpi -- http://technet.microsoft.com/zh-cn/library/dd464646 

/*************************************************************************************************************************
* 已读例子
*   winui\fulldrag -- (KBQ121541), 如果绘制比较花费时间，则在 WM_ENTERSIZEMOVE -> WM_EXITSIZEMOVE 时候缓冲，
*     防止频繁变更窗体大小时频繁绘制而闪烁。但是这样会造成窗体变大时出现背景色的空白的Bug(可以考虑通过 CFCanvas 缓存最大窗体的绘图结果 ?)
*   winui\shell\appplatform\DragDropVisuals -- ???, 只能在Vista后的平台运行
*
* 微软未公开的函数列表： http://undoc.airesoft.co.uk/index.php
*
* Win10
*   1.通用化设计 -- 可以非常好地适应桌面及平板类用户, 可在不同平台上获得近乎一样的使用体验
*************************************************************************************************************************/

//窗体中保存自定义数据：可以通过  _SetWindowLongPtr(hDlg, DWLP_USER, (this)); 的方法把this指针传入

//CWndClassInfo & MagnifierLayer::GetWndClassInfo()
//{
//	//
//	// ATL Internals pp 419
//	//
//	static CWndClassInfo wc =
//	{
//		{
//			sizeof( WNDCLASSEX ),
//				0,
//				StartWindowProc,
//				0,
//				0,
//				NULL,
//				NULL,
//				NULL,
//				(HBRUSH)GetStockObject(NULL_BRUSH),
//				NULL,
//				_T( "TestClassInfo"),
//				NULL
//		},
//		NULL,
//		NULL,
//		IDC_ARROW,
//		TRUE,
//		0,
//		_T( "TestClassInfo")
//	};
//
//	return wc;
//}

/******************************************************************************************************
* Monitor -- MonitorFromPoint ，系统有一个 multimon.h 文件
* 
* 窗体类型 -- Child必有父窗口,必无Onwer窗口。Overlapped/Popup可能有Onwer窗口,一般无Parent窗口
*     Combobox的下拉列表框listbox的Parent是NULL(能显示在Dialog外)，但Owner是Combobox的第一个非子窗口Owner(如Dialog，生存期管理)
*   Child   -- WS_CHILD， （子窗口可以是一个重叠窗口,但不能是一个弹出窗口？），注意：Child窗口没有维持其Owner属性(即自动认为非Child的Parent为其Owner)
*              子窗口在父窗口销毁前被销毁,在父窗口隐藏前被隐藏,在父窗口显示后被显示
*   Owner   -- 所有者，进行生存期控制(销毁)，Owner必须是Overlapped或Popup(WS_CHILD不能是Owner窗口)，被拥有的显示在Owner的前面，
*              Onwer最小化时其所拥有的窗口都被隐藏；但隐藏Owner时不会影响拥有者的可见状态(如 A > B > C, 则 A 最小化时B会隐藏，但C仍可见)
*                即： 当Owner窗口隐藏时他的所有Owned窗口不会隐藏。但当Owner最小化是他的Owned窗口会被隐藏。
*              运行时只读( GetWindow(GW_OWNER) )，对Child窗口调用该函数会返回NULL(父窗口即Owner?)，MFC中虽然有CWnd::SetOwner，但只更改了其成员变量 m_hWndOwner
*              CreateWindow函数中的hWndParent若非Child，则也会成为其Owner，否则递归向上找；GetWindowLong(GWL_HWNDPARENT) 得到传入的hWndParent属性
*   Parent  -- 父窗口，控制显示(left/top 等坐标系统)，Parent移动时子窗口也会移动；Parent隐藏时所有子窗口也被隐藏；
*              父窗口最小化时，子窗口也会最小化，但其WS_VISIBLE属性不变；
*              可通过 GetParent/SetParent 获取或动态改变，可设置为NULL(桌面，所有Top-Level窗口的Parent即是NULL)和HWND_MESSAGE(只处理消息的窗体message-only)
*              通常会把通知消息发送给Parent(但Toolbar是发送给Owner)，
*              MSDN中虽然说明了SetParent只能在同一进程中进行，但实际上可以多进程的HWND上调用（如IE多进程模式时多个进程通过IEBarHost组合在一起；但Chrome是主进程显示UI，其他子进程只有线程）
*      将Child设置为Top-Level: SetParent(NULL); ModifyStyle(WS_CHILD, WS_POPUP, 0); ModifyStyleEx(0, WS_EX_TOPMOST | WS_EX_TOOLWINDOW, 0);
*      将Top-Level设置为Child: ModifyStyle(WS_POPUP, WS_CHILD); SetParent(hWndNewParent);
*   Sibling -- 
*   
* 窗体链表
*   Desktop <== 唯一的桌面窗口
*    +- top-level <== 所有非child、父窗口是NULL(desktop)的窗口，可拥有其他top-level或被其他top-level所拥有
*      +- Overlapped <== 重叠窗口，有标题栏和边框，一般用作程序的主窗口，（重叠窗口可以是Child窗口？）
*        +- Child <== 
*      +- Popup <== 带WS_POPUP的窗口，可现实在屏幕任何地方，一般没有父窗口，可以没有标题栏
*
******************************************************************************************************/

/******************************************************************************************************
* CHAIN_MSG_MAP(__super)
* 
* Vista/Win7
*   Aero向导(CPropertySheetImpl<>, CPropertyPageImpl<>)
*     由属性页变为经典样式的向导:  m_psh.dwFlags |= PSH_WIZARD97; Areo 向导 PSH_AEROWIZARD
*     消息 PSM_SHOWWIZBUTTONS -- 显示或隐藏向导中的标准按钮， 有 PropSheet_ShowWizButtons 辅助宏
*          PSM_ENABLEWIZBUTTONS -- 启用或禁用某个标准按钮，有 PropSheet_EnableWizButtons 辅助宏
*          PSM_SETBUTTONTEXT -- 修改按钮上的文字，有 PropSheet_SetButtonText  辅助宏
* 任务对话框(TaskDialog/TaskDialogIndirect) -- 
*   TASKDIALOGCONFIG::pfCallback 回调函数，用来响应任务对话框所触发的事件
*     通知顺序: TDN_DIALOG_CONSTRUCTED -> TDN_CREATED 
*   消息：TDM_SET_ELEMENT_TEXT -- 设置任务对话框上控件的文本
*         TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE -- 在链接旁显示出UAC盾形图标
*   Flags
*     TDF_USE_COMMAND_LINKS -- 将自定义按钮显示为命令链接(不能控制标准按钮TDCBF_OK_BUTTON等)
*     TDF_SHOW_PROGRESS_BAR -- 显示进度条
*     用TDF_SHOW_MARQUEE_PROGRESS_BAR -- 显示走马灯样式(不停的从左到右)的进度条
*   进度条()
*     TDM_SET_PROGRESS_BAR_RANGE -- 指定进度条的指示范围的消息
*     TDM_SET_PROGRESS_BAR_POS -- 指定进度条在指示范围中的位置
*     TDM_SET_PROGRESS_BAR_STATE -- 改变进度条的状态
*     
* 
* UAC(User Account Control)
*   Button_SetElevationRequiredState --好像无效?
*
* DWM(Desktop Window Manager,窗口管理器) -- 负责组合桌面上的各个窗体, 允许开发者设置某个窗体在于其它窗体组合/重叠时的显示效果，
*   即能用来实现“半透明玻璃(Glass)”特效（允许控制窗体范围内部分区域的透明度)
*     窗体区域(Window Region) -- 指操作系统允许窗体在其中进行绘制的区域，除非切换回Basic主题，否则Vista已不再使用
*     桌面合成(Desktop Composition) -- DWM所提供的一个功能，可以实现诸如玻璃、3D窗口变换等视觉效果，
*       启用时，DWM默认将把窗体的非客户区域以玻璃效果呈现，而客户区域默认为不透明。
*       DwmIsCompositionEnabled -- 判断是否启用了合成效果
*       DwmEnableComposition -- 暂时禁用/启用桌面合成功能，不需要管理员权限？程序退出时自动恢复
*       DwmGetColorizationColor -- 检测到合成效果是半透明的还是不透明的，以及合成颜色
*       DwmEnableBlurBehindWindow -- 让客户区域完全或某部分实现玻璃效果
*       DwmExtendFrameIntoClientArea -- 可让框架(Window Frame)向客户区扩展
*       DwmSetWindowAttribute  -- 设置DWM窗体的属性,如 控制 Flip3D、最小化时的动画效果
*         MARGINS margins={-1}; -- 将框架扩展为整个客户区，即可将整个客户区域和非客户区域作为一个无缝的整体进行显示(如玻璃效果)
*     极光效果(aurora effect) -- 
*     Flip3D(Win+Tab) -- 
*     任务栏缩略图自动同步 -- DwmRegisterThumbnail、DwmUpdateThumbnailProperties
* 
* RGB --  0x00BBGGRR
* Gdiplus::ARGB -- 0xAARRGGBB  <== 注意：颜色顺序和RGB的相反
* 
******************************************************************************************************/

/******************************************************************************************************
* CallWindowProc(WNDPROC, HWND, MSG) -- 将消息传递给指定消息处理函数进行处理(一般用于Hook后交由原消息处理函数处理)
* DefWindowProc(HWND, MSG) -- 让Windows的缺省消息处理函数处理消息, 等价于 CallWindowProc(DefWindowProc, HWND, MSG)
* DefDlgProc
* DefFrameProc
* DefMDIChildProc
* WinProc 返回值：
*   0 -- 用户已经处理好消息；如 WM_CLOSE 中返回0则不会关闭，返回 DefWindowProc 才关闭(WM_DESTROY)
* PreTranslateMessage 返回值：
*   TRUE -- 表示消息已经被处理，不会继续发送给后续的消息处理链。
*   FALSE -- 表示消息未被处理，需要进一步处理。但一般是返回 基类的该函数调用结果
*   
* 消息映射
*   MFC: ON_MESSAGE / ON_REGISTERED_MESSAGE
*        ON_NOTIFY_EX -- 如 ON_NOTIFY_EX( TTN_NEEDTEXT, 0, SetTipText )
*   WTL: 
*   TODO: WTL 中的 BEGIN_MSG_MAP_EX 里面定义了 m_bMsgHandled 变量，但没有提前赋值，使用时会是一个随机值
* 
* 创建窗口的顺序：PreCreateWindow -> PreSubclassWindow -> OnGetMinMaxInfo -> OnNcCreate -> OnNcCalcSize -> OnCreate
*   -> OnSize -> OnMove -> OnChildNotify
* 关闭窗口的顺序：OnClose -> OnDestory -> OnNcDestroy -> PostNcDestory
*
* 窗口实例有四个关系窗口的句柄:
*   1.本窗口的 Z-Order 最高子窗口句柄 <== GetTopWindow
*   2.本窗口的下一兄弟窗口句柄 <== GetNextWindow
*   3.本窗口的父窗口的句柄   <== GetParent
*   4.本窗口的所有者窗口句柄 <== GetWindow(GW_OWNER)
*
* SendMessage 后执行体是UI线程(后台线程会等待，直到UI线程执行完毕)
*   如果收到Send消息时，UI线程还在消息处理体中,则★不会立即强制抢断执行★,需要等待执行体结束后，
*   再通过GetMessage★优先★调用执行。
*   WTL的例子程序分析:
*     Send时:_GetMessage -> _DispatchClientMessage -> _InternalCallWinProc
*     Post时:_DispatchMessage -> DispatchMessageWorker -> _InternalCallWinProc
* 
*
* 注意：改变窗体大小时，会连续发送 WM_SIZE + WM_PAINT 消息，擦除背景，容易造成闪烁
* 
* 自绘控件：OWNERDRAW，在创建时处理 WM_MEASUREITEM 消息，改变外观时处理 WM_DRAWITEM 消息
* 
* 创建全屏窗体( WS_POPUP属性, 0,0 ~ CxScreen, CyScreen )：
*   hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE, 0, 0, 
*     GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);
*
* 获取鼠标右键单击的位置，并显示弹出菜单(点击后返回)
*   TrackPopupMenu(hPopMenu, TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN, LOWORD(lParam), HIWORD(lParam) , 0, hwnd, NULL);
*
* 在 FormView 中启用界面更新机制：
*   1.包含 <afxpriv.h>；
*   2.映射处理消息 WM_IDLEUPDATECMDUI (对话框中需要处理 WM_KICKIDLE )
*   3.在实现中调用 UpdateDialogControls
*
* AdjustWindowRectEx -- 根据客户区的大小和窗口样式，计算并调整窗口的完整尺寸,然后MoveWindow进行调整即可
*   RECT rcClient = { 0,0,800,600 };
*   AdjustWindowRectEx( &rcClient, GetWindowStyle(hWnd), GetMenu(hWnd) != NULL, GetWindowExStyle(hWnd));
* MapWindowPoints -- 把相对于一个窗口的坐标空间的一组点映射成相对于另一窗口的坐标空间的一组点
*
* 滚动窗体 -- 依靠更改MapMode、ViewportExt/WindowExt, ViewportOrg/WindowOrg 等实现
*   CScrollView 中有 m_totalDev, m_pageDev 等参数
*   CScrollView 的 SetScrollSize 是逻辑坐标
*
* 客户区拖动
*   方法1(推荐).OnLButtonDown 中，SendMessage ( m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0 ); return 0;
*   方法2.OnLButtonDown 中: m_bMoveWindow = true; m_cpMouseDown = point; SetCapture();
*         OnMouseMove   中: CRect rt; GetWindowRect( &rt );	CSize pos = m_cpMouseDown - point; rt = pos - &rt; MoveWindow( rt );
*         OnLButtonUp   中: if(m_bMoveWindow）{ m_bMoveWindow = false; ReleaseCapture();}
*
* ShowWindow 
*   SW_HIDE -- 隐藏，失活
*   SW_MINIMIZE -- 最小化，失活
*   SW_RESTORE -- 激活并恢复原显示
*   SW_SHOW -- 激活并显示
*   SW_SHOWMAXIMIZED -- 激活并最大化
*   SW_SHOWMINIMIZED -- 激活并图标化
*   SW_SHOWMINNOACTIVE -- 图标化但是不激活
*   SW_SHOWNA -- 显示并保持原激活状态
*   SW_SHOWNOACTIVATE -- 用最近的大小和位置显示并保持原激活状态
*   SW_SHOWNORMAL -- 激活并显示，必要时从最大最小化中恢复
*
* ShowOwnedPopups -- 设置或者删除当前窗口所拥有的所有窗口的WS_VISIBLE属性，然后发送 WM_SHOWWINDOW 消息更新窗口显示
*   用处：如想隐藏Owned窗口但并不想最小化其Owner时 -- 但为什么不直接最小化或ShowWindow(SW_HIDE) ?
*
* 反射消息？ ( 消息 + WM_REFLECT_BASE , 对应的消息映射宏是 ON_WM_XXXX_REFLECT )
*  WM_COMMAND -- 消息的参数中既有发送消息的控件的 ID，又有控件的 HWND，还有通知代码
*  WM_NOTIFY  -- 消息除 WM_COMMAND 的参数之外还有一个指向 NMHDR 数据结构的指针
*  WM_MEASUREITEM -- 
& 
* TODO:
*  MFC中是 ON_NOTIFY_REFLECT 反射 ON_NOTIFY， 如 CTabCtrl中通过 ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange) + ON_NOTIFY_REFLECT(TCN_SELCHANGING, OnSelchanging) 可处理选择事件
*  CTabCtrl 可以自绘界面，可以设置按钮大小()，但不能设置切换按钮的间隔等。
******************************************************************************************************/

/******************************************************************************************************
* ListCtrl -- 列表控件
*   LVS_OWNERDATA -- 序列表技术  //http://download.csdn.net/source/236507
*     处理 LVN_GETDISPINFO 通知消息，填充需要显示的数据
*     处理 NM_CUSTOMDRAW 通知 (父窗体中处理 ?),给 pResult 赋值
*   LVIS_OVERLAYMASK -- 动态获取列表象的Overlay图标(类似TortoiseSVN在Shell中的各种状态图标?),INDEXTOOVERLAYMASK 设置 IShellIconOverlay 获取的索引?
* RichEdit -- CWindowImple<CEditView, CRichEditCtrl>
*   BitBlt 到 HDC 中，称为图片
*   StreamOutRtf -- 到Buffer中持久化
*
* SliderCtrl -- 滑块控件,滑块以创建时所指定的增量移动
*   常见属性、方法:
*     Buddy -- 关联窗口
*     LineSize -- 对应键盘的方向键
*     PageSize -- 对应PageUp、PageDown键
*     Selection-- 选择范围，高亮显示
*     ChannelRect -- 在刻度之上，供Thumb移动的区域
*     ThumbRect -- “拇指”(通过鼠标拖动的那个小方块)的范围
*     Tic/TicFreq -- 获取或设置显示刻度的位置/间隔
*   风格:
*     TBS_AUTOTICKS -- 滑动条具有刻度(?)
*     TBS_NOTICKS -- 滑动条不具有刻度(?)
*     TBS_ENABLESELRANGE -- 允许选择范围
*   要处理滑动条变化的事件，需要父窗体处理 WM_HSCROLL/WM_VSCROLL 消息(因为和水平滚动条公用同一个OnXScroll函数,
*     所以参数中的指针变量被定义为CScrollBar*类型，需要转换为CSliderCtrl*)
*     TRBN_THUMBPOSCHANGING Notify 事件不可以
*     
******************************************************************************************************/

/******************************************************************************************************
* 资源文件： RC -> RES，定义方式通常都是  ID 类型 文件名或资源名    （注意：只能有一个STRINGTABLE，没有名字）
*   CURSOR -- 光标
*   DIALOGEX --
*   ICON --
*   MENU -- DISCARDABLE指示什么？尽量丢弃？
*   STRINGTABLE -- 每个RC文件中只能有一个字符串表，每行字符串不能超过255个字符
*   WAVE -- 声音，使用 PlaySound 播放，SND_PURGE(停止播放)
******************************************************************************************************/

/******************************************************************************************************
* 分层窗口(WS_EX_LAYERED) -- 允许控制窗体的透明度。分层窗体提供了两种截然不同的编程模型
*   WS_EX_LAYERED 扩展窗口风格 -- 窗体将具备复合形状、动画、Alpha混合等方面的视觉特效
*     注意：不能用于Child窗体，也不能用于有 CS_OWNDC or CS_CLASSDC 属性的窗体。
*     透明度最大为255，如果要设置透明度 X%, bAlpha = X * 255 / 100;
*   SetLayeredWindowAttributes(简单) -- 允许设置一个RGB颜色(通常是窗体中不会出现的颜色)，然后所有以该颜色绘出的像素都将呈现为透明
*     SetLayeredWindowAttributes( 0, 150, LWA_ALPHA);  //设置透明度为150(窗体整体透明,子控件也透明)
*     SetLayeredWindowAttributes( RGB(240,240,240), 0, LWA_COLORKEY); //窗体整体透明,子控件不透明 
*       设置指定颜色的部分透明(Dialog背景颜色),如要设置其他颜色，需要在 OnCtlColor 或 WM_ERASEBKGND 中指定颜色
*     TODO: 组合使用 LWA_ALPHA 和 LWA_COLORKEY ?
*       ::SetClassLongPtr(m_hWnd, GCL_HBRBACKGROUND, (LONG_PTR)GetStockObject(BLACK_BRUSH));
*       SetLayeredWindowAttributes(m_hWnd, RGB(0xFF, 0, 0), 100, LWA_ALPHA | LWA_COLORKEY); //设置透明度为 100
*   UpdateLayeredWindow(困难) -- 提供一个与设备无关的位图，完整定义屏幕上窗体的整体样式，会将指定的位图完整地保留Alpha通道信息并拷贝到窗体上
*     ::UpdateLayeredWindow( m_hWnd, NULL, &ptDst, &WndSize, dcMem.m_hDC, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA );
*     这种窗体不支持子控件，不支持OnPaint()，但可以通过PNG图片中的Alpha值来 完全控制屏幕上窗体的透明情况
*
*   分层窗口真正实现了两个截然不同的概念：分层和重定向。
*     1. 设置 WS_EX_LAYERED 属性;
*          class ScreenRgnSelector : public CWindowImpl<ScreenRgnSelector, CWindow, 
*              CWinTraits<WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW> >
*     2. 通过 UpdateLayeredWindow 函数来更新分层窗口 -- 需要在位图中绘制出可视区域，并将其与关键色、
*        Alpha混合参数等一起提供给 UpdateLayeredWindow 函数,此时应用程序并不需要响应WM_PAINT或其他绘制消息.
*     或: 使用传统的Win32绘制机制 -- 调用 SetLayeredWindowAttributes 完成对关键色(COLORREF crKey)或阿尔法(BYTE bAlpha)混合参数值的设定,
*         之后系统将开始为分层窗口重定向所有的绘制并自动应用指定的特效
*   半透明窗体:(#define _WIN32_WINNT 0x0501)
*     SetWindowLong(m_hWnd,GWL_EXSTYLE,GetWindowLong(m_hWnd,GWL_EXSTYLE) | WS_EX_LAYERED );
*     //将对话框的窗体(颜色为COLOR_BTNFACE的地方)设为透明并且不再进行点击检测,其他半透明
*     SetLayeredWindowAttributes(GetSysColor(COLOR_BTNFACE),127,LWA_COLORKEY|LWA_ALPHA); 
*     //RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
*   异形窗体 -- 编辑出具有特定KeyColor的图片,使用 SetLayeredWindowAttributes 设置KeyColor
*     通过两个窗体重合且分别使用 SetLayeredXXX(,LWA_COLORKEY) 和 UpdateXXX 的方法来提供异形窗体
* 
*   阴影效果 -- http://www.codeproject.com/Articles/16362/Bring-your-frame-window-a-shadow
*   
*   创建圆形窗体：在Windows 9x下的正确做法是通过 SetWindowRgn 函数指出需要的窗体形状，但是这种处理在频繁更改窗体形状
*     或是在屏幕上拖拽时仍有缺陷存在：前台窗体将要求位于其下的窗体重绘整个区域，这将生过多的消息和计算量。
*     而且使用 SetWindowRgn 只能实现窗体的全透明而无法实现半透明效果。
*  
* 纯消息窗体(Message-Only Window)
*   允许收发消息，但不可见(不能接受键盘和鼠标消息)，没有Z序，不能被枚举，不能接收广播(broadcast)消息，
*   创建方法：在 CreateWindowEx 方法中，指定 hWndParent 参数为 HWND_MESSAGE 常量
*             或 SetParent 方法中，指定 hWndParent 参数为 HWND_MESSAGE 常量 将存在的窗体转换为纯消息窗体
*   找窗体：FindWindowEx(HWND_MESSAGE,xxxx);
*
* MoveWindow
*  WM_WINDOWPOSCHANGING => WM_WINDOWPOSCHANGED => WM_MOVE=> WM_SIZE => WM_NCCALCSIZE
* SetWindowPos -- 改变一个窗口的尺寸，位置和Z序
*   WM_SYNCPAINT(!SWP_DEFERERASE)
*   WM_NCCALCSIZE(SWP_FRAMECHANGED)
*   WM_WINDOWPOSCHANGING(!SWP_NOSENDCHANGING)
*   WM_WINDOWPOSCHANGED 
* BeginDeferWindowPos/DeferWindowPos/EndDeferWindowPos -- 一次性移动多个窗口(参见 GraphChat.sln 中的 ResizeMainWindow)
* SetForegroundWindow
*
* Z序
*   顶端(Topmost) => 顶层(普通) => 子窗口(后创建在上面)
*   BringWindowToTop -- 同类的顶部
******************************************************************************************************/

/******************************************************************************************************
* 模拟按键 Paste ( Ctrl + V )
*   keybd_event(VK_CONTROL, MapVirtualKey(VK_CONTROL, 0), 0, 0);
*   keybd_event('V', MapVirtualKey('V', 0), 0, 0);
*   keybd_event('V', MapVirtualKey('V', 0), KEYEVENTF_KEYUP, 0);
*   keybd_event(VK_CONTROL, MapVirtualKey(VK_CONTROL, 0), KEYEVENTF_KEYUP, 0);
* 鼠标模拟 mouse_event
*
* 鼠标位置检测(HitTest), 系统已经定义了 HTOBJECT、HTCLIENT 等宏
*
* DisableThreadLibraryCalls -- 禁止 DLL 的 DLL_THREAD_ATTACH | DLL_THREAD_DETACH 通知调用(TODO:禁止发还是禁止收?)
*   注意:如果静态链接了CRT的话不能调用该函数,因为CRT需要这些通知才能正常工作
******************************************************************************************************/

/******************************************************************************************************
* ToolTip(工具条提示) -- Win32的通用控件，MFC封装为 CToolTipCtrl( 新的扩展:CMFCToolTipCtrl + CTooltipManager)
*   常见的封装方法
*     SetTipTextColor/SetTipBkColor
*     
*   一般用法(自定义CWnd子类中使用):
*     1.添加 CToolTipCtrl m_toolTip 的成员变量;
*     2.在子控件的OnCreate等函数中创建ToolTip控件
*       EnableToolTips(TRUE); 
*       if (m_ToolTip.GetSafeHwnd() == NULL) { m_ToolTip.Create(this); m_ToolTip.Activate(TRUE); }
*     3.设置控件关联的Tip文本: m_ToolTip.AddTool(this, (LPCTSTR)strText);  
*       注意：如果自定义是个父控件，则 AddTool 中可以指定子控件(如 GetDlgItem)的指针; 可为多个控件添加不同的提示信息
*     4.重载 PreTranslateMessage, 并调用 m_toolTip.RelayEvent(pMsg)。
*     5.按需响应 WM_MOUSEHOVER(Update 或 UpdateTipText)、WM_MOUSEMOVE( if(!m_bTrack){ _TrackMouseEvent } )、WM_MOUSELEAVE(m_bTrack=FALSE;) 等消息
*   
*   动态获取信息并显示 -- AddTool(..., LPSTR_TEXTCALLBACK )
*     1.消息映射中增加 ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnToolTipText )
*        BOOL OnToolTipText(UINT id, NMHDR *pTTTStruct, LRESULT *pResult){
*          ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);
*          TOOLTIPTEXT *pToolTipText = (TOOLTIPTEXT *)pTTTStruct;
*          if(pToolTipText->uFlags & TTF_IDISHWND){  //idFrom为HWND
*            UINT nID = ::GetDlgCtrlID((HWND)pNMHDR->idFrom);
*            ....
*            //使工具条提示窗口在最上面
*            ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER); 
*          }
*          返回值TRUE表示设置了Tooltip，返回FALSE表示没有设置?
*        }
* 
*   CListCtrl子类添加多行提示信息方法
*     1.重载 OnToolHitTest 虚函数, 返回唯一的值标识是否变化(如返回 (nItem * 100 + nSubItem)), 返回-1表示不显示ToolTip 
*       if (nFlags & LVHT_ONITEMLABEL){ 
*         ...; pTI->lpszText = LPSTR_TEXTCALLBACK; pTI->uId = (UINT) (nItem * 100 + nSubItem); return pTI->uId;
*       }else { return -1; }
*     2.消息映射中增加 ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, OnToolTipText) 
*       CToolTipCtrl *pToolTip = AfxGetModuleThreadState()->m_pToolTip; 
*   
*   为CListCtrl单元格添加提示信息
*     1.OnMouseMove 中通过 SubItemHitTest 判断鼠标当前所在的位置(行, 列)
*       if((lvhti.iItem != m_nItem) || (lvhti.iSubItem != m_nSubItem)){ 通过和成员变量比较看是否移动到另外的单元格 }
******************************************************************************************************/

#ifndef WM_SYSTIMER
#  define WM_SYSTIMER 0x0118		//UnDocument Message(caret blink) , afxtrace.cpp!_AfxTraceMsg()可以看到定义
#endif 

const UINT DEFAULT_DUMP_FILTER_MESSAGES[] = {
	WM_SETCURSOR,
	
	WM_GETICON,
	WM_NCHITTEST,
	WM_NCMOUSEMOVE,
	WM_NCACTIVATE,

	WM_PAINT,
	WM_ERASEBKGND,
    WM_NCPAINT,

	WM_TIMER,
	WM_SYSTIMER,

	WM_ENTERIDLE,
	WM_MOUSEMOVE,

#ifndef WM_KICKIDLE
#  define WM_KICKIDLE	0x036A
#endif 
	WM_KICKIDLE,
	WM_CTLCOLORMSGBOX,
	WM_CTLCOLOREDIT,
	WM_CTLCOLORLISTBOX,
	WM_CTLCOLORBTN,
	WM_CTLCOLORDLG,
	WM_CTLCOLORSCROLLBAR,
	WM_CTLCOLORSTATIC,
	WM_PRINTCLIENT,

	RegisterWindowMessage(TEXT("WTL_CmdBar_InternalGetBarMsg")),
};

namespace FTL
{
	//注意:
	//1.WTSRegisterSessionNotification 后才能接收 WM_WTSSESSION_CHANGE, MFC 中用宏 ON_WM_WTSSESSION_CHANGE
	//2.RegisterShellHookWindow 后可以接收 WM_SHELLHOOKMESSAGE

	//在Output中Dump出当前接受到的消息，通常第一个参数是 __FILE__LINE__
#ifdef FTL_DEBUG
#  define DUMP_WINDOWS_MSG(pszName, pFilters, nCount, uMsg, wParam, lParam) \
    {\
        BOOL bFilterd = FALSE;\
        if(pFilters) \
        {\
		    for(int i = 0; i < nCount; i++){\
			    if(uMsg == *((UINT*)(pFilters + i))){\
				    bFilterd = TRUE;\
				    break;\
			    }\
		    }\
        }\
        if(!bFilterd)\
        {\
            FTLTRACE(TEXT("%s[%d](%d)%s, wParam=0x%x, lParam=0x%x, Tick=%d\n"),\
                pszName,GetCurrentThreadId(),uMsg, FTL::CFMessageInfo(uMsg, wParam, lParam).GetConvertedInfo(),\
		        wParam, lParam, GetTickCount() );\
        }\
    }
#else
#  define DUMP_WINDOWS_MSG(pszName, filters, nCount, uMsg, wParam, lParam)	__noop;
#endif 

	//通过 RegisterWindowMessage 注册的消息
	//非线程安全
	class CFRegistedMessageInfo
	{
	public:
		FTLINLINE BOOL Init();

		FTLINLINE explicit CFRegistedMessageInfo();
		FTLINLINE virtual LPCTSTR GetMessageInfo(UINT msg, WPARAM wParam, LPARAM lParam);
	private:
		BOOL m_bInited;
		TCHAR m_bufInfo[128];

		UINT RWM_ATL_CREATE_OBJECT;		//ATL.CAtlAutoThreadModuleT.CreateInstance 创建COM对象时使用(内部使用的套间线程同步?)
		UINT RWM_ATLGETCONTROL;			//AtlAxGetControl 获取Control的IUnknown接口(返回值)
		UINT RWM_ATLGETHOST;			//AtlAxGetHost 获取Host的IUnknown接口(返回值）
		UINT RWM_COLOROKSTRING;         //_T("commdlg_ColorOK")
		UINT RWM_COMMDLG_FIND;
		UINT RWM_FILEOKSTRING;
		UINT RWM_FINDMSGSTRING;         //_T("commdlg_FindReplace")
		UINT RWM_LBSELCHSTRING;

		//zmouse.h 中处理 MSWheel 的三个消息(有一个 MouseZ 窗体)
		UINT RWM_MSH_MOUSEWHEEL;		//MouseWheel
		UINT RWM_MSH_WHEELSUPPORT;		//3DSupport
		UINT RWM_MSH_SCROLL_LINES;		//ScrollLines

		UINT RWM_HELPMSGSTRING;
		UINT RWM_HTML_GETOBJECT;		//从IE窗体中获取对应的IHTMLDocument2接口
		UINT RWM_SETRGBSTRING;          //_T("commdlg_ColorOK")
		UINT RWM_SHAREVISTRING;
		UINT WM_SHELLHOOKMESSAGE;		//RegisterShellHookWindow 后接收
		UINT RWM_TASKBARBUTTONCREATED;	//任务栏重新创建?(没有确认，可以用于初始化Vista的 ITaskbarList3 接口?)
		UINT RWM_TASKBARCREATED;        //Explorer 崩溃重启后会发这个消息，App可以重新加入通知区域(Vista后需要ChangeWindowMessageFilter)

        UINT RWM_DRAGLISTMSGSTRING;
        
        //WTL
        UINT RWM_WTL_CMDBAR_INTERNALAUTOPOPUPMSG;   // _T("WTL_CmdBar_InternalAutoPopupMsg")
        UINT RWM_WTL_CMDBAR_INTERNALGETBARMSG;      // _T("WTL_CmdBar_InternalGetBarMsg")
        UINT RWM_WTL_GETEXTERIORPAGETITLEFONT;      // _T("GetExteriorPageTitleFont_531AF056-B8BE-4c4c-B786-AC608DF0DF12")
        UINT RWM_WTL_GETBULLETFONT;                 // _T("GetBulletFont_AD347D08-8F65-45ef-982E-6352E8218AD5")

        //WM_DEVICECHANGE; //这个消息需要通过 RegisterDeviceNotification 注册才能获得?
	};

    //! 将消息( WM_XXX )转换为易读的格式，类似于 ",wm"
    FTLEXPORT class CFMessageInfo : public CFConvertInfoT<CFMessageInfo,UINT>
    {
    public:
        FTLINLINE explicit CFMessageInfo(UINT msg, WPARAM wParam, LPARAM lParam);
        FTLINLINE virtual LPCTSTR ConvertInfo();
    public:
        WPARAM m_wParam;
        LPARAM m_lParam;

	private:
		static CFRegistedMessageInfo	s_RegistedMessageInfo;
    };

    #define BEGIN_WINDOW_RESIZE_MAP(thisClass) \
        static const _WindowResizeMap* GetWindowResizeMap() \
        { \
            static const _WindowResizeMap theMap[] = \
            {

    #define END_WINDOW_RESIZE_MAP() \
                { (DWORD)(-1), 0 }, \
            }; \
            return theMap; \
        }

    #define WINDOW_RESIZE_CONTROL(id, flags) \
                { id, flags },

    //开始定义一个组，之后定义的Control的位置将以Group前的一个Control的位置为标准
    #define BEGIN_WINDOW_RESIZE_GROUP() \
                { -1, _WINSZ_BEGIN_GROUP },

    #define END_WINDOW_RESIZE_GROUP() \
                { -1, _WINSZ_END_GROUP },


    FTLEXPORT template <typename T>
    class CFMFCDlgAutoSizeTraits
    {
    public:
        static HWND GetWinHwndProxy(T* pThis)
        {
            return pThis->GetSafeHwnd();
        }
        static DWORD GetStatusBarCtrlID()
        {
            return AFX_IDW_STATUS_BAR;
        }
        //static LRESULT OnSizeProxy(T* pThis,UINT nType, int cx, int cy);
    };

    FTLEXPORT template <typename T>
    class CFWTLDlgAutoSizeTraits
    {
    public:
        static HWND GetWinHwndProxy(T* pThis)
        {
            return pThis->m_hWnd;
        }
        static DWORD GetStatusBarCtrlID()
        {
            return ATL_IDW_STATUS_BAR;
        }
        //static void OnSizeProxy(UINT nType, CSize size);
    };

    //! 从WTL的 CDialogResize 拷贝并改造的自动调整窗体控件位置、大小的类，支持MFC、WTL的Dialog
    //! TODO:支持普通的Windows，如 View；增加窗体位置保存、恢复等功能（扩展WTL）
    //! Bug:是否有 TWindowAutoSizeTraits 的必要？
    //! 使用方法：继承列表中增加 public FTL::CFWindowAutoSize<CMyDlg,FTL::CFWTLDlgAutoSizeTraits<CMyDlg> >
    //!   然后在 OnInitDialog 中调用 InitAutoSizeInfo，在 WM_SIZE 处理函数中调用 AutoResizeUpdateLayout，处理 WM_GETMINMAX...
    //! 窗体如果具有 WS_CLIPCHILDREN 属性，可以防止出现闪烁？但实际测试时发现Child时无法正确绘制
    FTLEXPORT template <typename T, typename TWindowAutoSizeTraits >
    class CFWindowAutoSize
    {
    public:
        // Data declarations and members
        enum
        {
            WINSZ_SIZE_X        = 0x00000001,   //自动横向扩展、伸缩，保证左、右边界不变
            WINSZ_SIZE_Y        = 0x00000002,   //自动纵向扩展、伸缩，保证上、下边界不变
            WINSZ_MOVE_X        = 0x00000004,   //自动横向移动，保证右边界不变
            WINSZ_MOVE_Y        = 0x00000008,   //自动纵向移动，保证下边界不变
            WINSZ_CENTER_X      = 0x00000010,   //自动横向移动，保证左、右边界不变
            WINSZ_CENTER_Y      = 0x00000020,   //自动纵向移动，保证上、下边界不变
            WINSZ_REPAINT       = 0x00000040,   //Resize后强制重绘，通常没有必要 

            // internal use only
            _WINSZ_BEGIN_GROUP  = 0x00001000,
            _WINSZ_END_GROUP    = 0x00002000,
            _WINSZ_GRIPPER      = 0x00004000    //内部使用，表明是拖拽的Gripper，只能有一个(内部设置)
        };
        struct _WindowResizeMap
        {
            DWORD   m_nCtlID;
            DWORD   m_dwResizeFlags;
        };
        struct _WindowResizeData
        {
            int     m_nCtlID;
            //! 0x   7   6   |    5   4   |    3   2   1   0    |
            //!      未用(0) |   Group数  |      WINSZ 标志     |
            DWORD   m_dwResizeFlags; 
            RECT    m_rect;

            int GetGroupCount() const
            {
                return (int)LOBYTE(HIWORD(m_dwResizeFlags));
            }

            void SetGroupCount(int nCount)
            {
                FTLASSERT(nCount > 0 && nCount < 256);
                FTLASSERT(( m_dwResizeFlags & 0x00FFFFFF ) == m_dwResizeFlags); //目前最高的一个字节没有内容
                DWORD dwCount = (DWORD)MAKELONG(0, MAKEWORD(nCount, 0));
                m_dwResizeFlags &= 0xFF00FFFF;
                m_dwResizeFlags |= dwCount;
            }

            bool operator ==(const _WindowResizeData& r) const
            { 
                return (m_nCtlID == r.m_nCtlID && m_dwResizeFlags == r.m_dwResizeFlags); 
            }
        };
        typedef std::vector<_WindowResizeData> WindowResizeDataArray;
        WindowResizeDataArray   m_allResizeData;
        SIZE    m_sizeWindow;
        POINT   m_ptMinTrackSize;
        BOOL    m_bGripper;
    public:
        FTLINLINE CFWindowAutoSize();
        FTLINLINE BOOL InitAutoSizeInfo(BOOL bAddGripper = TRUE, BOOL bUseMinTrackSize = TRUE);
        FTLINLINE BOOL AutoResizeUpdateLayout(int cxWidth, int cyHeight);
    protected:
        FTLINLINE BOOL AutoPositionControl(int cxWidth, int cyHeight, RECT& rectGroup, _WindowResizeData& data, bool bGroup, 
            _WindowResizeData* pDataPrev = NULL);
    };

    //管理显示器的信息(如双显)
	//  使用如下的API(multimon.h，需要定义 COMPILE_MULTIMON_STUBS 宏?)： MonitorFromWindow/MonitorFromPoint/EnumDisplayMonitors 等
    class CFMonitorManager
    {
    public:
		
    private:
        //static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    };

	enum ColorType
	{
		ctCtrlColor,
		ctSysColor,
	};
	
    //TODO:使用方法
    //  1.客户端程序定义一个 TranslateWndClassProc 类型的全局函数
    //  2.
	typedef BOOL (CALLBACK* TranslateWndClassProc)(LPCTSTR pszOriClassName, LPTSTR pszNewClass, UINT nLength);
	__declspec(selectany) TranslateWndClassProc g_pTranslateWndClassProc = NULL;

    void FTLINLINE SetTranslateWndClassProc(TranslateWndClassProc pNewProc){
        g_pTranslateWndClassProc = pNewProc;
    }

    FTLEXPORT class CFWinUtil
    {
    public:
		//无Caption的窗体(如DUI::ListDemo)要实现大小改变、标题栏拖动时需要相应 WM_NCHITTEST 消息并返回对应的位置值
		//pPtClient -- ScreenToClient(鼠标在客户区中的坐标)
		//prcClient -- GetClientRect(因为是无Caption窗体，最大化、最小化、关闭等按钮都是在客户区中自绘的);
		//prcCaption -- 自绘的Cpation的范围(TODO:怎么排除 关闭等按钮的区域， 用Region?)
		//bZoomed -- IsZoomed
		FTLINLINE static LRESULT CalcNcHitTestPostion(LPPOINT pPtClient, LPCRECT prcClient, LPCRECT prcCaption, BOOL bZoomed);

		//查找指定ProcessId的主窗口ID
		FTLINLINE static HWND GetProcessMainWindow(DWORD dwProcessId);

        //激活并将指定窗体放在最前方 -- 解决在状态栏闪烁但不到前台的Bug(win98/2000?)
        //常用于二重启动时将窗体显示在前台
		//注意：Windows 98/2000 doesn't want to foreground a window when some other window has keyboard focus
        FTLINLINE static BOOL ActiveAndForegroundWindow(HWND hWnd);

        //隐藏任务栏
        FTLINLINE static BOOL HideTaskBar(BOOL bHide);

        //全屏窗口：WS_POPUP|WS_VISIBLE 属性(不能是 WS_OVERLAPPEDWINDOW ), 0,0 ~ CxScreen, CyScreen
        FTLINLINE static BOOL SetWindowFullScreen(HWND hWnd,BOOL isFullScreen, BOOL &oldZoomedState);
        FTLINLINE static LPCDLGTEMPLATE LoadDialog(HMODULE hModuleRes, LPCTSTR szResource, HINSTANCE * phInstFoundIn);

		//获取 GetSysColor 时颜色索引对应的字符串
		FTLINLINE static LPCTSTR GetColorString(ColorType clrType, int nColorIndex);

        //WM_HSCROLL 或 WM_VSCROLL 的通知码
        FTLINLINE static LPCTSTR GetScrollBarCodeString(UINT  nSBCode);

		//获取按键对应的 VK_
		FTLINLINE static LPCTSTR GetVirtualKeyString(int nVirtKey);

        //获取 WM_NOTIFY 消息 Code 对应的字符串信息
        FTLINLINE static LPCTSTR GetNotifyCodeString(HWND hWnd, UINT nCode, LPTSTR pszCommandNotify, int nLength, 
			TranslateWndClassProc pTransProc = g_pTranslateWndClassProc);

        //获取 WM_COMMAND 消息的 notifyCode
        FTLINLINE static LPCTSTR GetCommandNotifyString(HWND hWnd, UINT nCode, LPTSTR pszCommandNotify, int nLength, 
			TranslateWndClassProc pTransProc = g_pTranslateWndClassProc);

        FTLINLINE static LPCTSTR GetSysCommandString(UINT nCode);

		//获取窗体的类型、名字、位置、大小等最基本的信息
		FTLINLINE static LPCTSTR GetWindowDescriptionInfo(FTL::CFStringFormater& formater, HWND hWnd);
		//获取 Windows 窗体属性对应的字符串信息 
        FTLINLINE static LPCTSTR GetWindowClassString(FTL::CFStringFormater& formater, HWND hWnd, LPCTSTR pszDivide = TEXT("|"));
        FTLINLINE static LPCTSTR GetWindowStyleString(FTL::CFStringFormater& formater, HWND hWnd, LPCTSTR pszDivide = TEXT("|"));
        FTLINLINE static LPCTSTR GetWindowExStyleString(FTL::CFStringFormater& formater, HWND hWnd, LPCTSTR pszDivide = TEXT("|"));
		FTLINLINE static LPCTSTR GetWindowPosFlagsString(FTL::CFStringFormater& formater, UINT flags, LPCTSTR pszDivide = TEXT("|"));

		FTLINLINE static LPCTSTR GetOwnerDrawState(FTL::CFStringFormater& formater, UINT itemState, LPCTSTR pszDivide = TEXT("|"));
        FTLINLINE static LPCTSTR GetOwnerDrawAction(FTL::CFStringFormater& formater, UINT itemAction, LPCTSTR pszDivide = TEXT("|"));

        //窗体居中放置(ATL有源码？) -- TODO: 未测试
        FTLINLINE static BOOL CenterWindow(HWND hWndCenter , BOOL bCurrMonitor); //HWND hWndParent, 
    };//CFWinUtil

    FTLEXPORT class CFMenuUtil
    {
    public:
        FTLINLINE static BOOL DumpMenuInfo(HMENU hMenu, BOOL bDumpChild, int nLevel = 0);

        FTLINLINE static LPCTSTR GetMenuItemInfoString(FTL::CFStringFormater& formater, const MENUITEMINFO& menuItemInfo);
        FTLINLINE static LPCTSTR GetMenuItemInfoMaskString(FTL::CFStringFormater& formater, UINT fMask, LPCTSTR pszDivide = TEXT("|"));
        FTLINLINE static LPCTSTR GetMenuItemInfoTypeString(FTL::CFStringFormater& formater, UINT fType, LPCTSTR pszDivide = TEXT("|"));
        FTLINLINE static LPCTSTR GetMenuItemInfoStateString(FTL::CFStringFormater& formater, UINT fState, LPCTSTR pszDivide = TEXT("|"));
        FTLINLINE static LPCTSTR GetMenuStateString(FTL::CFStringFormater& formater, UINT fState, LPCTSTR pszDivide = TEXT("|"));

    private:
        FTLINLINE static BOOL _GetMenuText(HMENU hMenu, UINT nIDItem, UINT nFlags, CFMemAllocator<TCHAR>& menuText);
    };

	//SetWindowHook 时的一些辅助方法
	FTLEXPORT class CFWinHookUtil
	{
	public:
		FTLINLINE static LPCTSTR GetCBTCodeInfo(CFStringFormater& formater, int nCode, WPARAM wParam, LPARAM lParam);
	};

    template <typename T>
    class CFUIAdapterForWin
    {
    public:
        //typedef T     parent_class;

        //virtual bool PostNotification(XfNotificationPtr pNotify) = 0;
        virtual void UpdateUI() = 0;

        //static bool PostNotificationProxy(XfNotificationPtr pNotify, void* data)
        //{
        //    T* pThis = static_cast<T*>(data);
        //    return pThis->PostNotification(pNotify);
        //}
        static void UpdateUIProxy(void* data)
        {
            T* pThis = static_cast<T*>(data);
            pThis->UpdateUI();
        }
    };

    //对MessageBox进行Hook，可以更改按钮的文本，目前没有更改按钮的大小
    //! 用法：CFMessageBoxHook(m_hWnd, TEXT("MyClose"));
    //        ::MessageBox(...);
    #define PREV_WND_PROC_NAME  TEXT("PrevWndProc")
    class CFMessageBoxHook
    {
    public:
         //更改从线程弹出的对话框上的OK按钮文字
        FTLINLINE CFMessageBoxHook(DWORD dwThreadId, LPCTSTR pszOKString);
        FTLINLINE ~CFMessageBoxHook(void);
    private:
        FTLINLINE static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
        FTLINLINE static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    private:
        static HHOOK    s_hHook;
        static HWND     s_ProphWnd;
        static LPCTSTR  s_pszOKString;
    };

    class CFLockUpdate     
    { 
    public: 
        CFLockUpdate(HWND hwnd, BOOL bHighPrivilege = FALSE)
            :m_bHighPrivilege(bHighPrivilege)
            ,m_hwnd(hwnd) 
          { 
              if(m_hwnd==NULL)return; 
              if(m_bHighPrivilege) 
                  LockWindowUpdate(m_hwnd); 
              else 
                  SendMessage(m_hwnd,WM_SETREDRAW,FALSE,0); 
          } 
          ~CFLockUpdate() 
          { 
              if(m_hwnd==NULL)
              {
                  return; 
              }
              if(m_bHighPrivilege) 
                  LockWindowUpdate(NULL); 
              else 
                  SendMessage(m_hwnd,WM_SETREDRAW,TRUE,0); 
          } 
    private: 
        BOOL   m_bHighPrivilege; 
        HWND   m_hwnd; 
    };

#if 0
    template <typename ControlT , typename ConverterFun>
    class CControlPropertyHandleT
    {
    public:
        CControlPropertyHandleT(ControlT& control);//, ConverterFun& fun);
        FTLINLINE INT AddProperty(INT value);
    private:
        ControlT&       m_control;
        //ConverterFun&   m_fun;
    };
#endif

}//namespace FTL

#endif //FTL_WINDOW_H

#ifndef USE_EXPORT
#  include "ftlwindow.hpp"
#endif 