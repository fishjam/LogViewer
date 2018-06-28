
#ifndef FTL_SHELL_H
#define FTL_SHELL_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlShell.h requires ftlbase.h to be included first
#endif

//http://blog.csdn.net/liutaoxwl/article/details/3932219
 
/*****************************************************************************************************
* LPITEMIDLIST(PIDL) -- Shell中用于标识命名空间中的对象(用路径的方式无法表达 虚拟对象), 
*   其本质是一个内存块，其中内容为 SHITEMID 结构列表 + 两个NULL， 可通过 CFShellUtil::GetItemIdListStringValue 查看内部实现
*   系统提供了 ILCreateFromPath、ILClone、ILGetNext、ILCombine 等函数
*   其内存通常必须由用户调用 IMalloc::Free(TODO: 或 ) 释放
*   主要分两类:
*     全限定PIDL(绝对路径,PIDLIST_ABSOLUTE) -- 相对桌面的PIDL列表
*     相对PIDL(相对路径, PCUIDLIST_RELATIVE) -- 相对父目录(IShellFolder)的 PIDL
*   桌面的PIDL就是一个空的 ItemID (即只有两个NULL结束符的 LPITEMIDLIST), 这样才可以保证相对PIDL的一致性
* 
* 实体对象(文件或目录)
* 虚拟对象(网络邻居|控制面板 等) -- 虚拟对象可能在Explorer中显示的是两个对象，但它们可能被存储在同一个磁盘文件中。
* 
* IShellFolder -- Shell操作的核心接口，通常通过 SHGetDesktopFolder 获取桌面对应的接口实例(外壳名称空间的Root),所有Shell命名空间中的文件夹对象都暴露这个接口
*   BindToObject     -- 根据 相对PIDL 得到 子元素(一般是子文件夹, 不处理文件类型 ? )对应的 IShellFolder 接口
*   EnumObjects      -- 枚举文件夹内的子元素，获得相对于当前IShellFolder的满足过滤条件(SHCONTF_xxx)的所有对象的 相对PIDL
*                       当枚举没哟权限的目录(如 C:\System Volume Information )时会失败:
*                       1.有hWnd时(即弹出错误对话框)则返回 HRESULT_FROM_WIN32(ERROR_CANCELLED), 
*                       2.hWnd为NULL时(即不弹出错误对话框)， 则返回实际情况 HRESULT_FROM_WIN32(ERROR_NOT_READY), E_ACCESSDENIED, 
*   GetAttributesOf  -- 根据 相对PIDL 得到 文件或文件夹的指定属性(如是否是文件夹，是否有子元素等 ?), 
*                       虽然 cidl + apidl 参数支持同时获取多个，但在枚举中一般只获取一个(多选时右键属性才同时获取多个的共同属性 ?)
*                       SFGAOF 为输入输出参数，输入时设置想要获取的属性，根据设置参数进行检查，如有指定属性则置位，没有则清除指定属性.
*                       函数返回后和 检查的属性 执行与(&) 操作判断是否有属性。
*                       注意：某些属性(如 SFGAO_HASSUBFOLDER | SFGAO_CONTENTSMASK)可能会造成函数非常慢.
*   GetDisplayNameOf -- 根据 相对PIDL 得到的显示名称(可通过参数设置 全路径名 或 只当前路径名)
*                       gdnFlags -- 主要分为两组参数:
*                       1.SHGDN_NORMAL(值为0 -- 全路径) 或 SHGDN_INFOLDER(值为1 -- 相对于父文件夹) 
*                       2.SHGDN_FOREDITING(in-place编辑时的内容) (F2)
*                         SHGDN_FORADDRESSBAR(文件夹:地址栏的路径; 文件: 文件名, .lnk文件的话不显示扩展名 ) 
*                         SHGDN_FORPARSING -- 通常是内容部使用(如 "控制面板" 会得到 "::{26EE0668-A00A-44D7-9371-BEB064C98683}", 文件的话 SHGDN_NORMAL(全路径), SHGDN_INFOLDER(文件名)
*                       为了获得方便后期分析的内部信息，一般使用 SHGDN_FORPARSING + SHGDN_NORMAL
*   ParseDisplayName -- 根据 全限定文件名 得到 相对PIDL
*
* 辅助函数
*   SHBindToParent -- 根据 全限定PIDL 得到 父文件夹IShellFolder接口 和 相对PIDL
*   SHCreateShellItem -- Windows2000就支持, 从 IShellFolder + PIDL 创建 IShellItem
*   SHCreateItemFromParsingName|SHCreateItemFromIDList -- Vista后的函数，根据文件路径或 PIDL 获得 IShellItem|IShellItemImageFactory 对象
*   ShellBrowser(让用户选择目录) 窗体现在只需要用 SHBrowseForFolder 和 SHGetPathFromIDList 函数即可(新版本的 CFileDialog 似乎也提供了选择目录功能)
*   SHGetDesktopFolder -- 获取桌面对应的 IShellFolder接口()
*   SHGetFileInfo -- 可以获得文件图标等信息，为了节约内存，最好直接使用系统的ImageList返回index(SHGFI_SYSICONINDEX)
*   SHGetFolderPath
*   SHGetIconOverlayIndex -- 根据 全限定文件名 得到 Overlay图标索引
*   SHGetPathFromIDList|SHGetPathFromIDListEx -- 根据 全限定PIDL 得到 文件路径(TODO: 测试后发现有问题，得到的父路径始终是 桌面 )
*   SHGetSpecialFolderPath|SHGetSpecialFolderLocation -- 获取系统中指定类型的目录位置信息(字符串|全限定PIDL)
*   SHParseDisplayName -- 从
* 
* TODO:Overlay图标(类似UAC的盾牌图标?)索引
*****************************************************************************************************/

/*****************************************************************************************************
*   SHILCreateFromPath -- 从字符串的PATH中获得 LPITEMIDLIST
*   SHGetPathFromIDList-- 从LPITEMIDLIST中获得字符串的PATH
*
* 获取Shell的图标
*   SHFILEINFO shfi = {0};
*   UINT flags = SHGFI_ICON |SHGFI_SYSICONINDEX|SHGFI_SMALLICON; //SHGFI_LARGEICON
*   HIMAGELIST hi=(HIMAGELIST)SHGetFileInfo(NULL,0,&shfi,sizeof(SHFILEINFO),flags); //CImageList::FromHandle(hi
*
* 使用Shell打开指定文件(LPITEMIDLIST) -- 可以避免参数带空格的问题
*	SHELLEXECUTEINFO ShExecInfo = {0};
*   ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
*   ShExecInfo.fMask = SEE_MASK_IDLIST;
*      SEE_MASK_NOCLOSEPROCESS <== 控制是否通过 hProcess 返回进程的句柄
*   ShExecInfo.lpIDList= pidlItemTarget;
*   ShExecInfo.nShow = SW_MAXIMIZE;
*   ShellExecuteEx(&ShExecInfo);
*
* 获取文件版本信息 https://msdn.microsoft.com/en-us/library/windows/desktop/aa381049(v=vs.85).aspx
*   GetFileVersionInfoSize -> GetFileVersionInfo -> VerQueryValue(lpData, TEXT("\\"), &pFileInfo, (PUINT)&bufLen);
*   HIWORD(MS),LOWORD(MS),wBuild, HIWORD(LS)
*  	VerQueryValue(lpData, _T("\\StringFileInfo\\ProductVersion"), &pvProductVersion, &iProductVersionLen))

*****************************************************************************************************/

/*****************************************************************************************************
* TODO:
*   1.区别? SHChangeNotifyRegister 可以监控桌面等? ReadDirectoryChangesW 只能监控目录等 ?
*   2.性能问题: 似乎在 不同目录间移动上千个文件 时可能导致丢失一部分(或者很多)通知 -- 和 处理通知的速度有关
* 文件系统变化监控
*   1.SHChangeNotifyRegister -- 通过窗口消息实现，把指定的窗口添加到系统的消息监视链中，接收并处理来自文件系统或者Shell的通知消息。
*     注意: a.WinXP 中无法报告所有文件变更，Vista后才可以处理全部变更; b.存在性能问题; 
*   2.FindFirstChangeNotification  -- 创建一个更改通知句柄并设置初始更改通知过滤条件，然后进行Wait
*     注意: a.没有给出变更文件的信息;
*   3.ReadDirectoryChangesW -- 
*   4.实现ICopyHook接口的Shell扩展对象，并注册到 HKCR\Directory\shellex\CopyHookHandlers 下
*   5.变更日志(Change Journal), 可以跟踪每一个变更的细节，即使程序没有运行
*     https://msdn.microsoft.com/en-us/library/aa363803.aspx
*   6.文件系统过滤驱动 -- Sysinternals 的 FileMon 就使用了这种技术. 
*     https://msdn.microsoft.com/en-us/library/ff548202.aspx
* 注册表系统变化监控
*   1.RegNotifyChangeKeyValue
*****************************************************************************************************/

namespace FTL
{
	
    class IShellChangeObserver
    {
    public:
		virtual void OnFileRename( LPCITEMIDLIST /* pIdl_Src */, LPCITEMIDLIST /* pIdl_Dst */ ) {};
		virtual void OnFileCreate( LPCITEMIDLIST /* pIdl */ ){};
		virtual void OnFileDelete( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnFileUpdated( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnFreeSpace(LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnDirRename( LPCITEMIDLIST /* pIdl_Src */, LPCITEMIDLIST /* pIdl_Dst */ ){
            FTLTRACE(TEXT("OnDirRename: \n"));
        };

		virtual void OnDirCreate( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnDirDelete( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnDirUpdated( LPCITEMIDLIST /* pIdl */  ){};
		virtual void OnMediaInserted( LPCITEMIDLIST /* pIdl */  ){};
		virtual void OnMediaRemoved( LPCITEMIDLIST /* pIdl */  ){};
		virtual void OnNetShare(LPCITEMIDLIST /* pIdl */ ) {};
		virtual void OnNetUnShare(LPCITEMIDLIST /* pIdl */ ) {};
		virtual void OnDriveAdded( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnDriveAddGUI( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnDriveRemoved( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnChangeAttributes( LPCITEMIDLIST /* pIdl */  ) {};
		virtual void OnServerDisconnect(LPCITEMIDLIST /* pIdl */  ) {};
    };

	//使用 SHChangeNotifyRegister 检测文件系统文件变化通知
    FTLEXPORT class CFShellChangeMonitor
    {
    public:
        FTLINLINE CFShellChangeMonitor();
        FTLINLINE ~CFShellChangeMonitor();
		FTLINLINE BOOL SetChangeObserver(IShellChangeObserver* pChangeObserver)
		{
			m_pChangeObserver = pChangeObserver;
			return TRUE;
		}
        FTLINLINE BOOL Create(LPCTSTR pszMonitorPath = NULL, 
			LONG nEvent = SHCNE_ALLEVENTS | SHCNE_INTERRUPT,
			BOOL bRecursive = TRUE);
        FTLINLINE BOOL Destroy();
    private:
		IShellChangeObserver*	m_pChangeObserver;
        HWND m_hWndNotify;
        UINT m_uiNotifyMsg;
		ULONG m_uiChangeNotifyID;
        IShellFolder*  m_pShellFolder;
        FTLINLINE BOOL _CreateNotifyWinows();
        FTLINLINE static LRESULT CALLBACK _MainMonitorWndProc(HWND hwnd,UINT uMsg, WPARAM wParam,LPARAM lParam);
		FTLINLINE LRESULT _HandleMonitorEvent(LONG wEventId, LPCITEMIDLIST pIdlDst, LPCITEMIDLIST pIdlSrc);

        //FTLINLINE BOOL _RegisterMonitor()   
    };

    #define MAX_BUFFER					4096 * 8
    typedef struct _DIRECTORY_INFO
    {
        HANDLE		hDir;
        TCHAR		lpszDirName[MAX_PATH];
        CHAR		lpBuffer[MAX_BUFFER];
        DWORD		dwBufLength;
        OVERLAPPED	Overlapped;
    }DIRECTORY_INFO, *LPDIRECTORY_INFO;

    //使用 ReadDirectoryChangesW 监控 -- TODO: nDrive 中，尚未完全移植
    FTLEXPORT class CDirectoryChangeWatcher
    {
    public:
        FTLINLINE CDirectoryChangeWatcher();
        FTLINLINE ~CDirectoryChangeWatcher();

        FTLINLINE BOOL Create(LPCTSTR pszMonitorPath = NULL, 
            BOOL bRecursive = TRUE);
    private:
        DIRECTORY_INFO m_DirInfo;
        HANDLE m_hCompPort;
    };

    struct ShellFileInfo
    {
        //PIDLIST_ABSOLUTE    pIdl;
    };

	enum HandleShellItemResult{
		hsirContine,
		hsirSkipSubFolder,
		hsirStop,
	};
	struct HandleShellItemInfo{
		LPITEMIDLIST	pItemId;
		DWORD_PTR		userData;
		INT				nRecursiveLevel;
		INT				nIndex;

		HandleShellItemInfo()
		{
			pItemId = NULL;
			userData = NULL;
			nRecursiveLevel = 0;
			nIndex = 0;
		}
		~HandleShellItemInfo()
		{
			FTLASSERT(NULL == pItemId);		//don't forget free
		}
	};

	typedef HandleShellItemResult (CALLBACK* HANDLE_SHELLITEM_PROC)(__in IShellFolder* pShellFolder, __in const HandleShellItemInfo* const pItemInfo, __out BOOL& bFreeItem);

    FTLEXPORT class CFShellUtil
    {
    public:
		//获取 SHCNE_XXX( 如 SHCNE_RENAMEITEM ) 等对应的字符串信息
		FTLINLINE static LPCTSTR GetShellChangeNotifyString(LONG nEvent, CFStringFormater& formater, LPCTSTR pszDivide = TEXT("|"));

		FTLINLINE static LPCTSTR GetShellItemAttributes(SFGAOF gaoFlags, CFStringFormater& formater, LPCTSTR pszDivide = TEXT("|"));

		//获取 LPCITEMIDLIST 的字符串表达方式，用于调试
		FTLINLINE static LPCTSTR GetItemIdListStringValue(LPITEMIDLIST  pItemIdList, CFStringFormater& formater);

		FTLINLINE static HRESULT DumpShellFolderInfo(IShellFolder* pShellFolder, HWND hWnd, SHCONTF contFlags,
			SFGAOF gaoFlags, SHGDNF gdnFlags);

		//遍历 IShellFolder 的子元素, nRecursiveLevel 为 0 表示不递归， < 0 表示一直递归, > 0 表示递归层数
		//遍历时需要注意的目录: 
		//  1. %USERPROFILE%\Searches(FOLDERID_SavedSearches) -- 非常多(如何 过滤 ?), 有 SFGAO_STORAGEANCESTOR 但没有 SFGAO_STORAGE
		FTLINLINE static HRESULT EnumShellFolderObjects(IShellFolder* pShellFolder, HWND hWnd, SHCONTF contFlags, 
			HANDLE_SHELLITEM_PROC pHandleShellItemProc, DWORD_PTR userData, INT nRecursiveLevel = 0);

		//获取 pItemIdList 对应的字符串
		FTLINLINE static HRESULT GetItemIdName(  LPCITEMIDLIST  pItemIdList, LPTSTR pFriendlyName, UINT cchBuf, 
			DWORD dwFlags = SHGDN_FORPARSING, IShellFolder* pSF = NULL );

        //获取路径对应的 LPITEMIDLIST
        FTLINLINE static HRESULT GetItemIDListFromPath( LPCTSTR szFullPath, LPITEMIDLIST* ppidl, IShellFolder* pSF = NULL);

        //TODO: 系统已经提供了 SHGetFileInfo 函数
        FTLINLINE static HRESULT GetFileShellInfo(LPCTSTR pszPath, ShellFileInfo& outInfo);

		//获取Shell的系统图标列表，之后可以通过 GetListCtrl().SetImageList(CImageList::FromHandle(hi),LVSIL_SMALL 或 LVSIL_NORMAL) 的方式使用
		//获取文件信息时，可以获取到其在系统图标中的s索引 ?
		FTLINLINE static HRESULT GetShellIconImageList(__out HIMAGELIST& rSmallIconList, __out HIMAGELIST& rLargeIconList);

		//CLSID_ShellLink 组件有 IShellLink(提供快捷方式的参数读写功能)、IPersistFile(提供快捷方式持续性文件的读写功能)等接口。
		FTLINLINE static HRESULT CreateLink(LPCTSTR szPathObj, LPCTSTR szPathLink, LPCTSTR szDesc, LPCTSTR szIconPath = NULL, int iIcon = -1);

		//使用ShellExecute的方法打开文件执行，但如果没有建立连接，则弹出"OpenWith"的对话框
		//  TODO: 是否有其他标准的函数来完成该功能
		FTLINLINE static HRESULT ExecuteOrOpenWithDialog(LPCTSTR pszFile, HWND hWndParent);

		FTLINLINE static HRESULT ExplorerToSpecialFile(LPCTSTR pszFilePath);

        FTLINLINE static HRESULT LaunchIE(LPCTSTR szURL, int nCmdShow = SW_SHOW);

        //TODO: 获取文件的版本信息 -- 版本信息里面有很多内容(比如iexplore.exe有3K多的内容), 
        //  采用树结构进行保存各分享信息(如 \\VarFileInfo\\Translation ), 参见 PATH_VER_FIXEDFILEINFO 定义
        FTLINLINE static HRESULT GetFileVersionInfo(LPCTSTR pszFilePath, VS_FIXEDFILEINFO *pFileInfo, 
            DWORD dwFlags = FILE_VER_GET_LOCALISED | FILE_VER_GET_NEUTRAL);

    };


    //BUG -- Win7 下，当有 BIF_NEWDIALOGSTYLE 时，无法自动定位显示 pszInit 的目录
    //https://connect.microsoft.com/VisualStudio/feedback/details/518103/bffm-setselection-does-not-work-with-shbrowseforfolder-on-windows-7
    //atldlgs.h 中有 CFolderDialog -- CFolderDialog dlg ( hWnd, NULL, BIF_USENEWUI | BIF_RETURNONLYFSDIRS ); dlg.SetInitialFolder( szDefaultPath ); if (IDOK ==dlg.DoModal() ){ ... }
    class CFDirBrowser
    {
    public:
        FTLINLINE CFDirBrowser(LPCTSTR lpszTitle = NULL, HWND hWndOwner = NULL, LPCTSTR pszInit = NULL, 
            UINT nFlags = BIF_NEWDIALOGSTYLE | 
            BIF_EDITBOX |
            BIF_STATUSTEXT |        //可包含状态区域，通过发送消息可以设置其文本
            BIF_RETURNONLYFSDIRS |  //仅返回文件系统的目录，选中“我的电脑”等时确认按钮为禁用状态
            BIF_BROWSEINCLUDEURLS 
            );
		FTLINLINE virtual ~CFDirBrowser();
        FTLINLINE BOOL DoModal();
        FTLINLINE LPCTSTR GetSelectPath();
		FTLINLINE LPITEMIDLIST GetSelectPathIdList(BOOL bDetach);
        BROWSEINFO  m_browseInfo;
    protected:
        TCHAR       m_szPath[MAX_PATH];
        TCHAR       m_szInitPath[MAX_PATH];
		LPITEMIDLIST m_pSelectItemIdList;
    protected:
        FTLINLINE static int CALLBACK DirBrowseCallbackProc(HWND hwnd, UINT uMsg,LPARAM lParam, LPARAM lpData);
        HWND        m_hTreeView;
        BOOL        m_bFirstEnsureSelectVisible;
        FTLINLINE HWND FindTreeViewCtrl(HWND hWnd);
        FTLINLINE BOOL EnsureSelectVisible();
		FTLINLINE void FreeDirResource();
    };
}

#endif //FTL_SHELL_H

#ifndef USE_EXPORT
#  include "ftlShell.hpp"
#endif