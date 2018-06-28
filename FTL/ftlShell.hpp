#ifndef FTL_SHELL_HPP
#define FTL_SHELL_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlShell.h"
#endif

#include "ftlConversion.h"
#pragma comment(lib, "Version.lib")
//#include "ftlComDetect.h"

namespace FTL
{
#define FTL_SHELL_CHANGE_MONITOR_CLASS_NAME     TEXT("FTL_SHELL_CHANGE_MONITOR_CLASS")

#define FILE_SPY_FILTER		( FILE_NOTIFY_CHANGE_DIR_NAME |  FILE_NOTIFY_CHANGE_FILE_NAME | \
    FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE )


    CFShellChangeMonitor::CFShellChangeMonitor()
    {
		m_pChangeObserver = NULL;
        m_hWndNotify = NULL;
        m_uiNotifyMsg = 0;
		m_uiChangeNotifyID = 0;
        m_pShellFolder = NULL;
    }

    CFShellChangeMonitor::~CFShellChangeMonitor()
    {
        Destroy();
    }

    BOOL CFShellChangeMonitor::Create(LPCTSTR pszMonitorPath /* = NULL */, 
		LONG nEvent /* = SHCNE_ALLEVENTS | SHCNE_INTERRUPT */,
		BOOL bRecursive /* = TRUE */)
    {
        BOOL bRet = FALSE;
        HRESULT hr = E_FAIL;
		if (!m_hWndNotify)
		{
			API_VERIFY(_CreateNotifyWinows());
			FTLASSERT(m_hWndNotify);
		}

		if(NULL == m_pShellFolder)
		{
        	COM_VERIFY(SHGetDesktopFolder(&m_pShellFolder));
		}
		
        m_uiNotifyMsg = RegisterWindowMessage(TEXT("FTL_SHELL_CHANGE_MONITOR"));

		LPITEMIDLIST pItemMonitor = NULL;
        if (NULL == pszMonitorPath)
        {
            COM_VERIFY(::SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pItemMonitor)); //CSIDL_DESKTOPDIRECTORY
        }
		else
		{
            COM_VERIFY(CFShellUtil::GetItemIDListFromPath(pszMonitorPath, &pItemMonitor, NULL));
		}
		if (pItemMonitor)
		{
			SHChangeNotifyEntry notifyEntry = {0};
			notifyEntry.fRecursive = bRecursive;
			notifyEntry.pidl = pItemMonitor;

			m_uiChangeNotifyID = ::SHChangeNotifyRegister(m_hWndNotify, 
				SHCNF_IDLIST | SHCNF_TYPE, 
				nEvent,
				m_uiNotifyMsg,   
				1, 
				&notifyEntry);
#pragma TODO(free the pItemMonitor)
			API_VERIFY(m_uiChangeNotifyID != 0);
		}


		//HRESULT hr = E_FAIL;
		//COM_VERIFY(SHGetDesktopFolder(&m_pShellFolder));
        return bRet;
    }

    BOOL CFShellChangeMonitor::Destroy()
    {
		BOOL bRet = TRUE;
		if (m_uiChangeNotifyID)
		{
			API_VERIFY(::SHChangeNotifyDeregister(m_uiChangeNotifyID));
			m_uiChangeNotifyID = 0;
		}
        if(m_hWndNotify)
        {
            API_VERIFY(::DestroyWindow(m_hWndNotify));
            m_hWndNotify = NULL;
        }
		SAFE_RELEASE(m_pShellFolder);
		return bRet;
    }

    BOOL CFShellChangeMonitor::_CreateNotifyWinows()
    {
		FTLASSERT(NULL == m_hWndNotify);

		BOOL bRet = FALSE;
		HINSTANCE hInstance = GetModuleHandle(NULL);

        WNDCLASSEX wndClass = {0};
		wndClass.cbSize = sizeof(WNDCLASSEX);

		wndClass.style			= 0; //CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc	= _MainMonitorWndProc;
		wndClass.cbClsExtra		= 0;
		wndClass.cbWndExtra		= 0;
		wndClass.hInstance		= hInstance;
		wndClass.hIcon			= NULL; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTWNDCLASS));
		wndClass.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wndClass.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
		wndClass.lpszMenuName	= NULL;//MAKEINTRESOURCE(IDC_TESTWNDCLASS);
		wndClass.lpszClassName	= FTL_SHELL_CHANGE_MONITOR_CLASS_NAME;
		wndClass.hIconSm		= NULL; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
		
		ATOM atom = ::RegisterClassEx(&wndClass);
		API_VERIFY(atom != 0);
		if (atom != 0)
		{
			m_hWndNotify = ::CreateWindow(FTL_SHELL_CHANGE_MONITOR_CLASS_NAME, TEXT("FtlShellMonitor"), 
				WS_OVERLAPPED, -2, -2, 1, 1, NULL, NULL, hInstance, NULL);
			API_VERIFY(m_hWndNotify != NULL);
			if (m_hWndNotify)
			{
				::SetWindowLongPtr(m_hWndNotify, GWLP_USERDATA, (LONG_PTR)this);
			}
		}
		
        return bRet;
    }

    LRESULT CALLBACK CFShellChangeMonitor::_MainMonitorWndProc(HWND hwnd,UINT uMsg, 
        WPARAM wParam, LPARAM lParam)
    {
		LONG_PTR nUserData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
		CFShellChangeMonitor* pThis = (CFShellChangeMonitor*)nUserData;

		if (pThis)
		{
			if (wParam && pThis->m_uiNotifyMsg == uMsg)
			{
				LRESULT nResult = 0;
				const LPCITEMIDLIST* pidls = ( LPCITEMIDLIST* )wParam;
				const LONG wEventId = ( LONG )lParam;
                LPCITEMIDLIST pIdlSrc = pidls[0];
				LPCITEMIDLIST pIdlDst = pidls[1];
				nResult = pThis->_HandleMonitorEvent(wEventId, pIdlDst, pIdlSrc);
				return nResult;
			}
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    }
	
	LRESULT CFShellChangeMonitor::_HandleMonitorEvent(LONG wEventId, LPCITEMIDLIST pIdlDst, LPCITEMIDLIST pIdlSrc)
	{
		HRESULT hr = E_FAIL;
        BOOL bRet = FALSE;
#ifdef FTL_DEBUG
		TCHAR szSrcPath[MAX_PATH] = {0};
		TCHAR szDstPath[MAX_PATH] = {0};
		int nSrcIdListSize = 0;
        int nDstIdListSize = 0;

		if (pIdlDst)
		{
            nDstIdListSize = pIdlDst->mkid.cb;
			COM_VERIFY(CFShellUtil::GetItemIdName(pIdlDst, szDstPath, _countof(szDstPath), SHGDN_FORPARSING, m_pShellFolder));

            TCHAR szDstCheck[MAX_PATH] = {0};
            API_VERIFY(SHGetPathFromIDList(pIdlDst, szDstCheck));
            API_VERIFY(lstrcmp(szDstCheck, szDstPath) == 0);
		}
		if (pIdlSrc)
		{
            nSrcIdListSize = pIdlSrc->mkid.cb;
			COM_VERIFY(CFShellUtil::GetItemIdName(pIdlSrc, szSrcPath, _countof(szSrcPath), SHGDN_FORPARSING, m_pShellFolder));

            TCHAR szSrcCheck[MAX_PATH] = {0};
            API_VERIFY(SHGetPathFromIDList(pIdlSrc, szSrcCheck));
            BOOL bSamePath = (lstrcmp(szSrcCheck, szSrcPath) == 0);
            FTLTRACE(TEXT("bSamePath=%d\n"), bSamePath);
        }

        LPITEMIDLIST pItemNew = NULL;
        CFShellUtil::GetItemIDListFromPath(szSrcPath, &pItemNew);
        BOOL isSame = (m_pShellFolder->CompareIDs(0, pIdlSrc, pItemNew) == 0);

		CFStringFormater formaterChangeNotify;
		FTLTRACEEX(tlTrace, TEXT("_HandleMonitorEvent: event=%s(0x%x), srcPath=(cb=%d)%s, dstPath=(cb=%d)%s\n"), 
			CFShellUtil::GetShellChangeNotifyString(wEventId, formaterChangeNotify),
			wEventId, nSrcIdListSize, szSrcPath, nDstIdListSize, szDstPath);
#endif 

		if (m_pChangeObserver)
		{
			switch (wEventId)
			{
			case SHCNE_RENAMEITEM:
				m_pChangeObserver->OnFileRename(pIdlSrc, pIdlDst);
				break;
			case SHCNE_CREATE:
				m_pChangeObserver->OnFileCreate(pIdlDst);
				break;
			case SHCNE_DELETE:
				m_pChangeObserver->OnFileDelete(pIdlDst);
				break;
			case SHCNE_MKDIR:
				m_pChangeObserver->OnDirCreate(pIdlDst);
				break;
			case SHCNE_RMDIR:
				m_pChangeObserver->OnDirDelete(pIdlDst);
				break;
			case SHCNE_MEDIAINSERTED:
				m_pChangeObserver->OnMediaInserted(pIdlDst);
				break;
			case SHCNE_MEDIAREMOVED:
				m_pChangeObserver->OnMediaRemoved(pIdlDst);
				break;
			case SHCNE_DRIVEREMOVED:
				m_pChangeObserver->OnDriveRemoved(pIdlDst);
				break;
			case SHCNE_DRIVEADD:
				m_pChangeObserver->OnDriveAdded(pIdlDst);
				break;
			case SHCNE_NETSHARE:
				m_pChangeObserver->OnNetShare(pIdlDst);
				break;
			case SHCNE_NETUNSHARE:
				m_pChangeObserver->OnNetUnShare(pIdlDst);
				break;
			case SHCNE_ATTRIBUTES:
				m_pChangeObserver->OnChangeAttributes(pIdlDst);
				break;
			case SHCNE_UPDATEDIR:
				m_pChangeObserver->OnDirUpdated(pIdlDst);
				break;
			case SHCNE_UPDATEITEM:
				m_pChangeObserver->OnFileUpdated(pIdlDst);
				break;
			case SHCNE_SERVERDISCONNECT:
				m_pChangeObserver->OnServerDisconnect(pIdlDst);
				break;
			case SHCNE_DRIVEADDGUI:
				m_pChangeObserver->OnDriveAddGUI(pIdlDst);
				break;
			case SHCNE_RENAMEFOLDER:
				m_pChangeObserver->OnDirRename(pIdlSrc, pIdlDst);
				break;
			case SHCNE_FREESPACE:
				m_pChangeObserver->OnFreeSpace(pIdlDst);
				break;
			default:
				FTLTRACEEX(tlWarn, TEXT("Unknown Shell Change Notify Event:0x%x\n"),	wEventId);
				break;
			}
		}
		return 0;
	}

    CDirectoryChangeWatcher::CDirectoryChangeWatcher()
    {
        m_hCompPort = NULL;
    }

    CDirectoryChangeWatcher::~CDirectoryChangeWatcher()
    {

    }

    BOOL CDirectoryChangeWatcher::Create(LPCTSTR pszMonitorPath, BOOL bRecursive /* = TRUE */)
    {
        BOOL bRet = FALSE;
        //获取目录句柄
        API_VERIFY(INVALID_HANDLE_VALUE != (m_DirInfo.hDir = CreateFile( pszMonitorPath, 
             FILE_LIST_DIRECTORY,   //访问(读/写)模式
            FILE_SHARE_READ | FILE_SHARE_WRITE /*| FILE_SHARE_DELETE*/,   //TODO:如果没有 SHARE_DELETE 标志，会导致其他进程无法重命名或者删除这个目录下的文件 ?
            NULL, OPEN_EXISTING, 
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,   // 文件属性
            NULL )));
        if (bRet)
        {
            API_VERIFY(NULL != (m_hCompPort = CreateIoCompletionPort( m_DirInfo.hDir, m_hCompPort, (DWORD)&m_DirInfo, 0 )));
            API_VERIFY(ReadDirectoryChangesW( m_DirInfo.hDir, m_DirInfo.lpBuffer, sizeof(m_DirInfo.lpBuffer),
                TRUE, FILE_SPY_FILTER, &m_DirInfo.dwBufLength, &m_DirInfo.Overlapped, NULL)); // Completion routine	

            if (!bRet)
            {
                SAFE_CLOSE_HANDLE(m_DirInfo.hDir, INVALID_HANDLE_VALUE);
            }
        }
        return bRet;
    }

	//////////////////////////////////////////////////////////////////////////

	LPCTSTR CFShellUtil::GetShellChangeNotifyString(LONG nEvent, 
		CFStringFormater& formater, LPCTSTR pszDivide /* = TEXT("|") */)
	{
		LONG nOldEvent = nEvent;

		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_RENAMEITEM, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_CREATE, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_DELETE, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_MKDIR, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_RMDIR, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_MEDIAINSERTED, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_MEDIAREMOVED, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_DRIVEREMOVED, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_DRIVEADD, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_NETSHARE, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_NETUNSHARE, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_ATTRIBUTES, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_UPDATEDIR, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_UPDATEITEM, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_SERVERDISCONNECT, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_UPDATEIMAGE, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_DRIVEADDGUI, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_RENAMEFOLDER, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_FREESPACE, pszDivide);
#if (_WIN32_IE >= 0x0400)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_EXTENDED_EVENT, pszDivide);
#endif 
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, nEvent, SHCNE_ASSOCCHANGED, pszDivide);

		FTLASSERT( 0 == nEvent);
		if (0 != nEvent)
		{
			FTLTRACEEX(FTL::tlWarn, TEXT("%s: GetShellChangeNotifyString Not Complete, old=0x%x, remain=0x%x\n"),
				__FILE__LINE__, nOldEvent, nEvent);
		}

		return formater.GetString();
	}

	LPCTSTR CFShellUtil::GetShellItemAttributes(SFGAOF gaoFlags, CFStringFormater& formater, LPCTSTR pszDivide /* = TEXT("|") */)
	{
		SFGAOF oldGaoFlags = gaoFlags;
		
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_CANCOPY, pszDivide);	//能拷贝(通常用于设置对应菜单状态)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_CANMOVE, pszDivide);	//能移动
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_CANLINK, pszDivide);	//能创建快捷方式
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_STORAGE, pszDivide);	//可通过 BindToObject 得到 IStorage 接口，实测发现 目录 有该属性(文件没有)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_CANRENAME, pszDivide); //可重命名(只是建议，不保证)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_CANDELETE, pszDivide);	//能删除

		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_HASPROPSHEET, pszDivide);		//有属性页	
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_DROPTARGET, pszDivide);		//可以作为拖拽Target，即能 GetUIObjectOf(IDropTarget) 
		//HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_CAPABILITYMASK, pszDivide);
#ifndef SFGAO_SYSTEM
#  define SFGAO_SYSTEM         0x00001000     //Windows 7 and later. The specified items are system items.
#endif //SFGAO_SYSTEM
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_SYSTEM, pszDivide);	//具有系统属性的对象, 如 C:\Windows\assembly 等
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_ENCRYPTED, pszDivide);	//被加密了(会使用颜色标记)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_ISSLOW, pszDivide);	//表明通过 IStream 等接口访问时会很慢, 目前尚未测试到
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_GHOSTED, pszDivide);	//TODO: 使用 ghosted icon 显示, 什么意思 ?
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_LINK, pszDivide);		//快捷方式，通常是 .lnk/.url 文件
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_SHARE, pszDivide);		//指定的目录对象是共享的(也就是说遍历时只能遍历一次), 如 C:\Users
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_READONLY, pszDivide);	//只读,目录的话表明不能创建新的子项; 如 家庭组, 网络 等虚拟对象
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_HIDDEN, pszDivide);	//隐藏项，只有当 "Show hidden files and folders" 启用时才会显示
		//HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_DISPLAYATTRMASK, pszDivide);

		//目录对象或者是 文件系统目录 或者 至少有一个后代是文件系统目录, 即 不是纯虚拟对象目录(如 控制面板、回收站, .zip 文件夹), 通常需要在有 SFGAO_FOLDER 属性时才检查 
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_FILESYSANCESTOR, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_FOLDER, pszDivide);		//是否是文件夹(即可以继续遍历子元素), 通常是各种目录、.zip 文件等

		//文件系统中的项(文件、目录),parse 的名字可以认为是文件路径(UNC或磁盘路径) -- 计算机/控制面板等就没有改属性。.zip 有该属性
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_FILESYSTEM, pszDivide);

		//★判断是否有子文件夹(如果有的话，当前文件夹可以继续展开-即前面有加号"+")，注意：使用本参数可能很慢(如在较大的 .zip 中检查是否有子文件夹)
		//注意: 一般也是先要检查 SFGAO_FOLDER; 其值只是"建议"(如 网络磁盘的目录总是返回该值 )
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_HASSUBFOLDER|SFGAO_CONTENTSMASK, pszDivide);

		//输入参数时,指示目录去验证对象是否存在,如 不存在(已删除的文件, 无光盘的光驱, 不能访问的网上邻居 等), GetAttributesOf 会返回错误, 
		//对文件系统目录时, 该参数指示丢弃 已缓存的属性信息(IShellFolder2::GetDetailsEx)
		//本标志永远不会作为结果返回
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_VALIDATE, pszDivide);

		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_REMOVABLE, pszDivide);		//指定对象在可移动设备上，或其自身就是可移动设备(TODO, 哪种?)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_COMPRESSED, pszDivide);	//被压缩,通常也会用颜色标记

		//可以浏览(实测发现 .cab/.htm/.xml/.xlsx/.pptx/.zip 等文件有该属性), 可通过 BindToObject 到 IShellFolder后 CreateViewObject
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_BROWSABLE, pszDivide);

		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_NONENUMERATED, pszDivide);
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_NEWCONTENT, pszDivide);

#pragma TODO(check msdn for SFGAO_STREAM)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_CANMONIKER|SFGAO_HASSTORAGE|SFGAO_STREAM, pszDivide);	//支持 BindToObject(IID_IStream)
		HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, SFGAO_STORAGEANCESTOR, pszDivide);	//子对象可通过 IStream or IStorage 接口访问(即其有 SFGAO_STORAGE or SFGAO_STREAM)
		//HANDLE_COMBINATION_VALUE_TO_STRING(formater, gaoFlags, xxxxxx, pszDivide);

		FTLASSERT( 0 == gaoFlags);
		if (0 != gaoFlags)
		{
			FTLTRACEEX(FTL::tlWarn, TEXT("%s: GetShellItemAttributes Not Complete, old=0x%x, remain=0x%x\n"),
				__FILE__LINE__, oldGaoFlags, gaoFlags);
		}
		return formater.GetString();
	}

	LPCTSTR CFShellUtil::GetItemIdListStringValue(LPITEMIDLIST  pItemIdList, CFStringFormater& formater)
	{
		// ITEMIDLIST -- 一或多个 SHITEMID 结构组成的列表(实质是连续的内存块)，以两个 NULL 结束, 每一个item ID 都对应命名空间中的一个对象
		HRESULT hr = E_FAIL;
		BOOL bRet = FALSE;

		LPITEMIDLIST pTmpList = pItemIdList;
		INT nIdListLength = 0;	//计算的长度和 ILGetSize 一致，会包含最后的两个 NULL  
		INT nIdIndex = 0; 
		CFStringFormater tmpFormater;

		if (pTmpList)
		{
			nIdListLength += 2; //最后的两个NULL结束符
		}
		while(pTmpList)
		{
			SHITEMID& itemId = pTmpList->mkid;

			if (0 == itemId.cb)
			{
				break;
			}
			nIdIndex++;
			nIdListLength += itemId.cb;		//ItemID 结构体的长度，包括 cb(USHORT, 2字节)的长度
			
			LONG nStrBinaryCount = 0;

			API_VERIFY_EXCEPT1(CFConvUtil::HexFromBinary(itemId.abID, itemId.cb - sizeof(itemId.cb), NULL, &nStrBinaryCount, _T(' ')), ERROR_INSUFFICIENT_BUFFER);
			CFMemAllocator<TCHAR> bufString(nStrBinaryCount);
			API_VERIFY(CFConvUtil::HexFromBinary(itemId.abID, itemId.cb - sizeof(itemId.cb), bufString.GetMemory(), &nStrBinaryCount, _T(' ')));

			tmpFormater.AppendFormat(_T(",{[%d] %d:%s}"), nIdIndex, itemId.cb, bufString.GetMemory());

			LPITEMIDLIST pCheckItemList = ILGetNext(pTmpList);	//使用系统API获取下一个ItemID开始的List
			pTmpList = LPITEMIDLIST(((BYTE*)pTmpList) + itemId.cb);
			FTLASSERT(pCheckItemList == pTmpList);
		}
		FTLASSERT(NULL == *((BYTE*)pTmpList + 1));	//ITEMIDLIST以两个 NULL 结束，此处验证第二个NULL

		INT nSize = (INT)ILGetSize(pItemIdList);
		FTLASSERT(nSize == (nIdListLength));

		if (pItemIdList)
		{
			COM_VERIFY(formater.Format(_T("%d%s"), nIdListLength, tmpFormater.GetString()));
		}
		else
		{
			COM_VERIFY(formater.Format(TEXT("")));
		}
		
		return formater.GetString();
	}


	HRESULT CFShellUtil::DumpShellFolderInfo(IShellFolder* pShellFolder, HWND hWnd, SHCONTF contFlags, SFGAOF gaoFlags, SHGDNF gdnFlags)
	{
		FUNCTION_BLOCK_TRACE(1);

		//BOOL bRet = FALSE;
		HRESULT hr = E_FAIL;
		CHECK_POINTER_RETURN_VALUE_IF_FAIL(pShellFolder, E_POINTER);

		//枚举文件夹中各元素的 相对PIDL
		CComPtr<IEnumIDList>	spEnumIdList;
		{
			FUNCTION_BLOCK_NAME_TRACE(TEXT("EnumObjects"), 100);
			COM_VERIFY(pShellFolder->EnumObjects(hWnd, contFlags, &spEnumIdList));
		}
		
		if (spEnumIdList)
		{
			LPITEMIDLIST pID = NULL;
			//SFGAOF sof = 0;
			ULONG ulID = 0;
			STRRET strRet = {0};
			//TCHAR szShGotPath[MAX_PATH] = {0};
			TCHAR szGetDisplayName[MAX_PATH] = {0};
			INT nIndex = 0;
			while ( ( spEnumIdList->Next( 1, &pID, &ulID ) == S_OK ) && ulID > 0 )
			{
				nIndex++;
				SFGAOF checkGaoFlags = gaoFlags;
				CFStringFormater gaoFormater;

				{
					FUNCTION_BLOCK_NAME_TRACE(TEXT("GetAttributesOf"), 100);
					COM_VERIFY(pShellFolder->GetAttributesOf( 1, (LPCITEMIDLIST*)&pID, &checkGaoFlags));
					if (SUCCEEDED(hr))
					{
						GetShellItemAttributes(checkGaoFlags, gaoFormater);
					}
				}

				COM_VERIFY(pShellFolder->GetDisplayNameOf( pID, gdnFlags, &strRet ));
				if (SUCCEEDED(hr))
				{
					COM_VERIFY(StrRetToBuf( &strRet, pID, szGetDisplayName, _countof(szGetDisplayName)));
				}
				//FTLASSERT(StrCmpI(szShGotPath, szGetDisplayName) == 0);

				FTLTRACE(TEXT("[%d] szGetDisplayName=%s, Attr=%s\n"), nIndex, szGetDisplayName, gaoFormater.GetString());

				//if (gaoFlags & SFGAO_CANMONIKER)
				//{
				//	CComPtr<IShellFolder> spTmpShellFolder;
				//	COM_VERIFY(pShellFolder->BindToObject(pID, NULL, IID_IShellFolder, (void**)&spTmpShellFolder));
				//	if (spTmpShellFolder)
				//	{
				//		COM_DETECT_INTERFACE_FROM_LIST(spTmpShellFolder);
				//		COM_DETECT_INTERFACE_FROM_REGISTER(spTmpShellFolder);
				//	}
				//}
				
				SAFE_ILFREE(pID);
			}
		}

		return hr;
	}

	HRESULT CFShellUtil::EnumShellFolderObjects(IShellFolder* pShellFolder, HWND hWnd, SHCONTF contFlags, 
		HANDLE_SHELLITEM_PROC pHandleShellItemProc, DWORD_PTR userData, INT nRecursiveLevel)
	{
		FUNCTION_BLOCK_TRACE(100);

		BOOL bRet = FALSE;
		HRESULT hr = E_FAIL;
		//UNREFERENCED_PARAMETER(bRet);

		CHECK_POINTER_RETURN_VALUE_IF_FAIL(pShellFolder, E_POINTER);
		CHECK_POINTER_RETURN_VALUE_IF_FAIL(pHandleShellItemProc, E_POINTER);

		//枚举文件夹中各元素的 相对PIDL
		CComPtr<IMalloc>		spMalloc;
		COM_VERIFY(SHGetMalloc(&spMalloc));

		CComPtr<IEnumIDList>	spEnumIdList;
		{
			FUNCTION_BLOCK_NAME_TRACE(TEXT("EnumObjects"), 100);
			hr = pShellFolder->EnumObjects(hWnd, contFlags, &spEnumIdList);
			FTLASSERT(
				   (S_OK == hr)
				|| (S_FALSE == hr)		// UsersLibraries\Git
				|| (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)    //when hWnd is NOT NULL, and access "C:\System Volume Information"
				|| (HRESULT_FROM_WIN32(ERROR_NOT_READY) == hr)	  //when access CDROM without CD
				|| (E_ACCESSDENIED == hr)						  //when hWnd is NULL and access "C:\System Volume Information" and close the prompt dialog
				);
		}

		if (spEnumIdList && spMalloc)
		{
			HandleShellItemInfo itemInfo;
			itemInfo.userData = userData;
			itemInfo.nRecursiveLevel = nRecursiveLevel;
			BOOL bStop = FALSE;

			HandleShellItemResult	handleResult = hsirContine;
			ULONG ulID = 0;

			while ( !bStop && ( spEnumIdList->Next( 1, &itemInfo.pItemId, &ulID ) == S_OK ) && ulID > 0 )
			{
				BOOL bFreeItem = TRUE;
				handleResult = (*pHandleShellItemProc)(pShellFolder, &itemInfo, bFreeItem);

				//结束
				if (hsirStop == handleResult)
				{
					if (bFreeItem)
					{
						spMalloc->Free(itemInfo.pItemId);
					}
					itemInfo.pItemId = NULL;
					bStop = TRUE;
					break;
				}

				//递归检查
				if ((nRecursiveLevel != 0) && (hsirSkipSubFolder != handleResult))
				{
					SFGAOF gaoFlag = SFGAO_FOLDER;
					COM_VERIFY(pShellFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&itemInfo.pItemId, &gaoFlag));
					if (SFGAO_FOLDER & gaoFlag)
					{
						CComPtr<IShellFolder> spSubShellFolder;
						COM_VERIFY(pShellFolder->BindToObject(itemInfo.pItemId, NULL, IID_IShellFolder, (void**)&spSubShellFolder));
						if (spSubShellFolder)
						{
							 COM_VERIFY(EnumShellFolderObjects(spSubShellFolder, hWnd, contFlags, pHandleShellItemProc, userData, nRecursiveLevel - 1));
							 if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)
							 {
								bStop = TRUE; 
							 }
						}
					}
				}

				if (bFreeItem)
				{
					spMalloc->Free(itemInfo.pItemId);
				}
				itemInfo.pItemId = NULL;
				itemInfo.nIndex++;
			}
		}

		return hr;
	}

	HRESULT CFShellUtil::GetItemIdName( LPCITEMIDLIST  pItemIdList, LPTSTR pFriendlyName, UINT cchBuf, 
		DWORD dwFlags, IShellFolder* pSF )
	{
		HRESULT hr = S_OK;
		STRRET strRet = {0};
		IShellFolder* pShellFolder  = pSF;

		if ( pShellFolder == NULL )
		{
			COM_VERIFY(SHGetDesktopFolder( &pShellFolder ));
			if ( !pShellFolder )
			{
				return hr;
			}
		}
		COM_VERIFY(pShellFolder->GetDisplayNameOf( pItemIdList, dwFlags, &strRet));
		if (SUCCEEDED(hr))
		{
			COM_VERIFY(StrRetToBuf( &strRet, pItemIdList, pFriendlyName, cchBuf)); 
		}

		if ( NULL == pSF )
		{
			pShellFolder->Release();
			pShellFolder = NULL;
		}
		return hr;
	}

    HRESULT CFShellUtil::GetItemIDListFromPath(LPCTSTR szFullPath, LPITEMIDLIST* ppidl, IShellFolder* pSF)
    {
		//TODO: 直接使用 SHParseDisplayName 即可?
        HRESULT hr = E_FAIL;
        IShellFolder* pShellFolder  = pSF;
        if ( pShellFolder == NULL )
        {
            COM_VERIFY(SHGetDesktopFolder( &pShellFolder ));
            if ( !pShellFolder )
            {
                return hr;
            }
        }
        ULONG chEaten = 0;
        DWORD dwAttributes = SFGAO_COMPRESSED;

        OLECHAR olePath[MAX_PATH] = { '\0' };
#ifdef UNICODE
        StringCchCopy( olePath, MAX_PATH, szFullPath );
#else
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szFileName, -1, olePath, MAX_PATH );
#endif

        //LPWSTR pszNPath = (LPWSTR)conv.TCHAR_TO_UTF16(szFullPath);
        COM_VERIFY(pShellFolder->ParseDisplayName(NULL, NULL, olePath, &chEaten, ppidl, &dwAttributes));
        if ( NULL == pSF )
        {
            pShellFolder->Release();
            pShellFolder = NULL;
        }
        return hr;
    }

    HRESULT CFShellUtil::GetFileShellInfo(LPCTSTR pszPath, ShellFileInfo& outInfo)
    {
        HRESULT hr = E_NOTIMPL;
        UNREFERENCED_PARAMETER(pszPath);
        UNREFERENCED_PARAMETER(outInfo);
//#pragma TODO(Use SHGetFileInfo get all info)

		CComPtr<IMalloc> spMalloc;
		CComPtr<IShellFolder> spShellFolder;
		COM_VERIFY(SHGetMalloc(&spMalloc));
		COM_VERIFY(SHGetDesktopFolder(&spShellFolder));
		//
		if (spShellFolder && spMalloc)
		{
              //SHParseDisplayName(pszPath, NULL, &outInfo.pIdl, )
		}
		
        return hr;
    }

	HRESULT CFShellUtil::GetShellIconImageList(__out HIMAGELIST& rSmallIconList, __out HIMAGELIST& rLargeIconList)
	{
		FTLASSERT( NULL == rSmallIconList );
		FTLASSERT( NULL == rLargeIconList );

		HRESULT hr = E_FAIL;

		SHFILEINFO shfi = {0};
		rSmallIconList = (HIMAGELIST) SHGetFileInfo(NULL, 0, &shfi, sizeof(SHFILEINFO),
			SHGFI_ICON | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		rLargeIconList = (HIMAGELIST) SHGetFileInfo(NULL, 0, &shfi, sizeof(SHFILEINFO),
			SHGFI_ICON | SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
		
		if (rSmallIconList != NULL && rLargeIconList != NULL)
		{
			hr = S_OK;
		}
		else
		{
			ImageList_Destroy(rSmallIconList);
			rSmallIconList = NULL;
			ImageList_Destroy(rLargeIconList);
			rLargeIconList = NULL;

			hr = E_FAIL;
		}
		return hr;
	}

	HRESULT CFShellUtil::CreateLink(LPCTSTR szPathObj, LPCTSTR szPathLink, LPCTSTR szDesc, LPCTSTR szIconPath /* = NULL */, int iIcon /* = -1 */)
	{
		HRESULT hr = E_FAIL;
		CComPtr<IShellLink> spShellLink;

		COM_VERIFY(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
			IID_IShellLink, (void**)&spShellLink));
		if (SUCCEEDED(hr)) 
		{
			CComPtr<IPersistFile> spPersistFile;
			COM_VERIFY(spShellLink->QueryInterface(IID_IPersistFile, (void**)&spPersistFile));
			if (SUCCEEDED(hr))
			{
				COM_VERIFY(spShellLink->SetPath(szPathObj));		// 全路径程序名
				COM_VERIFY(spShellLink->SetDescription(szDesc));	// 备注
				//spShellLink->SetArguments();						// 命令行参数
				//spShellLink->SetHotkey();							// 快捷键
				//spShellLink->SetIconLocation();					// 图标
				//spShellLink->SetShowCmd();						// 窗口尺寸
				//spShellLink->SetWorkingDirectory(...);			//设置 EXE 程序的默认工作目录
				if (szIconPath && iIcon >= 0)
				{
					COM_VERIFY(spShellLink->SetIconLocation(szIconPath, iIcon));
				}
                CFConversion conv;
				COM_VERIFY(spPersistFile->Save(conv.TCHAR_TO_UTF16(szPathLink), TRUE));
			}
		}

		return hr;
	}

	HRESULT CFShellUtil::ExecuteOrOpenWithDialog(LPCTSTR pszFile, HWND hWndParent)
	{
		HRESULT hr = S_OK;
		BOOL bRet = FALSE;
		//先使用 ShellExecute 执行，如果有错误不会弹出错误信息
		DWORD dwResult = (DWORD) ::ShellExecute(hWndParent, TEXT("open") , pszFile , NULL, NULL, SW_NORMAL);
		if (dwResult < 32)
		{
			hr = HRESULT_FROM_WIN32(dwResult);

			if (SE_ERR_NOASSOC == dwResult)
			{
				//如果是未建立关联程序的错误
				HMODULE hShell32 = GetModuleHandle(TEXT("Shell32.dll"));
				FTLASSERT(hShell32);

				typedef HRESULT (_stdcall *SHOpenWithDialogProc)(HWND hwndParent, const OPENASINFO* poainfo);
				SHOpenWithDialogProc pShOpenWithDialog = (SHOpenWithDialogProc)GetProcAddress(hShell32, "SHOpenWithDialog");
				if (pShOpenWithDialog)
				{
					//Vista or Win7 above
					OPENASINFO openAsInfo = {0};
					openAsInfo.pcszFile = pszFile;
					openAsInfo.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_EXEC;

					COM_VERIFY_EXCEPT2((*pShOpenWithDialog)(hWndParent, &openAsInfo), S_FALSE, ERROR_CANCELLED);
					//Notice: Does not need call FreeLibrary
				}
				else
				{
					//WinXP
					TCHAR szSysDir[MAX_PATH] = {0};
					GetSystemDirectory(szSysDir, _countof(szSysDir));

					//CPath pathRundll32(szSysDir);
					//pathRundll32.Append(TEXT("Rundll32.exe"));
					TCHAR pathRundll32[MAX_PATH] = {0};
					StringCchCopy(pathRundll32, _countof(pathRundll32), szSysDir);
					PathAppend(pathRundll32, TEXT("Rundll32.exe"));

					//CPath pathRunParam(szSysDir);
					//pathRunParam.Append(TEXT("Shell32.dll,OpenAs_RunDLL"));
					TCHAR pathRunParam[MAX_PATH] = {0};
					StringCchCopy(pathRunParam, _countof(pathRunParam), szSysDir);
					PathAppend(pathRunParam, TEXT("Shell32.dll,OpenAs_RunDLL"));

					CString strCmd;  
					strCmd.Format(_T("%s %s %s"), pathRundll32, pathRunParam, pszFile);
					FTLTRACE(TEXT("ExecuteOpenWithDialog, Cmd=%s\n"), strCmd);
					//MessageBox( strCmd, TEXT("OpenAs_RunDLL in XP"), MB_OK);

//#pragma TODO(CreateProcess cannot work)

					//STARTUPINFO startupInfo = {0};
					//startupInfo.cb = sizeof(startupInfo);

					//PROCESS_INFORMATION processInfo = {0};
					//API_VERIFY(CreateProcess(pathRundll32.m_strPath, (LPTSTR)(LPCTSTR)strCmd, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo));
					//if (bRet)
					//{
					//	CloseHandle(processInfo.hProcess);
					//	CloseHandle(processInfo.hThread);
					//}
					//dwResult = (DWORD)ShellExecute(m_hWnd, TEXT("open"), strCmd, NULL, NULL, SW_SHOWNORMAL);
					//if (dwResult != 0)
					//{
					//	CString strRet;
					//	strRet.Format(TEXT("ErrorReturn:%d"), dwResult);
					//	MessageBox(strRet, TEXT(""), MB_OK);
					//}

					//USES_CONVERSION;
                    //LPSTR pszCmd = T2A(strCmd);
                    CFConversion conv;
					API_VERIFY(::WinExec( conv.TCHAR_TO_MBCS(strCmd), SW_SHOWNORMAL ));
				}
			}
			else
			{
				//other error -- use ShellExecuteEx and show system error message
				SHELLEXECUTEINFO ExecuteInfo= {0};
				ExecuteInfo.cbSize = sizeof(ExecuteInfo);
				ExecuteInfo.fMask = 0;// SEE_MASK_NOCLOSEPROCESS;
				ExecuteInfo.hwnd = hWndParent;
				ExecuteInfo.lpVerb = TEXT("open");
				ExecuteInfo.lpFile =  pszFile;
				ExecuteInfo.nShow = SW_NORMAL;
				API_VERIFY(ShellExecuteEx(&ExecuteInfo));
				if (!bRet)
				{
					hr = HRESULT_FROM_WIN32(GetLastError());
				}
			}
		}

		return hr;
	}

    //TODO: 
    //  http://support.microsoft.com/kb/314853
    //  strParam.Format( _T( "/n, \"%s\"" ), szDirPath ); 或 .Format( _T( "/n, /select, \"%s\"" ), szFilePath );
    //  ::ShellExecute( NULL, _T( "open" ), _T( "explorer.exe" ), strParam, NULL, SW_SHOWNORMAL ); 
	HRESULT CFShellUtil::ExplorerToSpecialFile(LPCTSTR pszFilePath)
	{
		HRESULT hr = E_FAIL;

		PIDLIST_RELATIVE pidl = NULL;
		//ULONG attributes = 0;
		SFGAOF sfgaofIn = 0, sfgaofOut = 0;
        CFConversion conv;
		COM_VERIFY(SHParseDisplayName(conv.TCHAR_TO_UTF16(pszFilePath), NULL, &pidl, sfgaofIn, &sfgaofOut));
		if (SUCCEEDED(hr))
		{
			COM_VERIFY(SHOpenFolderAndSelectItems(pidl, 0, NULL, 0));
			//ILFree(pidl);
			CoTaskMemFree(pidl);
			pidl = NULL;
		}
		return hr;
	}

    HRESULT CFShellUtil::LaunchIE(LPCTSTR szURL, int nCmdShow/* = SW_SHOW */)
    {
        HRESULT hr = S_OK;
        LONG lRet = ERROR_SUCCESS;
        BOOL bRet = FALSE;

        const TCHAR REGKEY_CURVER[]     = _T( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion" );
        const TCHAR REGVAL_PROGFILES[]  = _T( "ProgramFilesDir" ) ;
        const TCHAR IE_DIR_NAME[]       = _T( "Internet Explorer" );
        const TCHAR IE_FILE_NAME[]      = _T( "iexplore.exe" );

        TCHAR szIEPath[MAX_PATH] = {0};

        DWORD dwCount = _countof( szIEPath ) ;
        
        HKEY hKey = NULL;

        REG_VERIFY(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_CURVER, 0, KEY_QUERY_VALUE, &hKey));
        if (ERROR_SUCCESS == lRet)
        {
            DWORD dwType = 0;
            DWORD dwBytes = dwCount * sizeof(TCHAR);
            REG_VERIFY(::RegQueryValueEx(hKey, REGVAL_PROGFILES, NULL, &dwType, (LPBYTE)szIEPath, &dwBytes));
            if (ERROR_SUCCESS == lRet)
            {
                dwCount = dwBytes / sizeof(TCHAR);
                *( szIEPath + dwCount ) = _T( '\0' ) ;
            }
            else{
                dwCount = 0;
            }
            REG_VERIFY(RegCloseKey(hKey));
        }

        API_VERIFY(::PathAppend( szIEPath, IE_DIR_NAME ));
        API_VERIFY(::PathAppend( szIEPath, IE_FILE_NAME ));

        SHELLEXECUTEINFO ExecuteInfo= {0};
        ExecuteInfo.cbSize = sizeof(ExecuteInfo);
        ExecuteInfo.fMask = 0; // SEE_MASK_NOCLOSEPROCESS;
        ExecuteInfo.hwnd = NULL;
        ExecuteInfo.lpVerb = TEXT("open");
        ExecuteInfo.nShow = nCmdShow;
        ExecuteInfo.lpParameters = szURL;
        if ( ::PathFileExists( szIEPath ) )
        {
            ExecuteInfo.lpFile =  szIEPath;
        }
        else{
            ExecuteInfo.lpFile = NULL;
        }
        API_VERIFY(ShellExecuteEx(&ExecuteInfo));
        if (!bRet)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        return hr;
    }

    HRESULT CFShellUtil::GetFileVersionInfo(LPCTSTR pszFilePath, VS_FIXEDFILEINFO *pFileInfo, DWORD dwFlags)
    {
        //定义可以获得版本信息的路径
        static LPCTSTR PATH_VER_FIXEDFILEINFO = TEXT("\\");

        HRESULT hr = E_FAIL;
        BOOL bRet = FALSE;

        CHECK_POINTER_ISSTRING_PTR_RETURN_VALUE_IF_FAIL(pszFilePath,  HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        CHECK_POINTER_WRITABLE_DATA_RETURN_VALUE_IF_FAIL(pFileInfo, sizeof(VS_FIXEDFILEINFO), HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));

        DWORD dwValue = 0 ;
        DWORD dwSize = ::GetFileVersionInfoSizeEx(dwFlags, pszFilePath, &dwValue) ;
        API_VERIFY(0 != dwSize);

        if (dwSize != 0)
        {
            ZeroMemory(pFileInfo, sizeof(*pFileInfo)) ;
            CFMemAllocator<BYTE> dataBuf(dwSize);
            API_VERIFY(::GetFileVersionInfoEx(dwFlags, pszFilePath, 0, dwSize, dataBuf.GetMemory()));
            if (bRet)
            {
                BYTE *pbyBlock = NULL ;
                UINT cbBlock = 0 ;
                API_VERIFY(VerQueryValue(dataBuf.GetMemory(), PATH_VER_FIXEDFILEINFO, reinterpret_cast<void **>(&pbyBlock), &cbBlock));
                FTLASSERT(sizeof(*pFileInfo) == cbBlock);
                if (sizeof(*pFileInfo) == cbBlock)
                {
                    CopyMemory(pFileInfo, pbyBlock, cbBlock);
                    hr = S_OK;
                }
            }
        }
        if (FAILED(hr))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        
        return hr;
    }


    CFDirBrowser::CFDirBrowser(LPCTSTR lpszTitle /* = NULL */, HWND hWndOwner /* = NULL */, LPCTSTR pszInit /* = NULL */, 
        UINT nFlags)
    {
        ZeroMemory(&m_browseInfo, sizeof(m_browseInfo));
        ZeroMemory(m_szPath, sizeof(m_szPath));
        ZeroMemory(m_szInitPath, sizeof(m_szInitPath));
        m_hTreeView = NULL;
        m_bFirstEnsureSelectVisible = TRUE;

        m_browseInfo.lpszTitle = lpszTitle;
        m_browseInfo.hwndOwner = hWndOwner;
        //m_browseInfo.pszDisplayName = m_szPath;
        m_browseInfo.ulFlags = nFlags;
        m_browseInfo.lpfn = DirBrowseCallbackProc;
        m_browseInfo.lParam = (LPARAM)this;
        m_pSelectItemIdList = NULL;
        if (pszInit && PathFileExists(pszInit))
        {
            lstrcpyn(m_szInitPath, pszInit, _countof(m_szInitPath));
        }
     }

	CFDirBrowser::~CFDirBrowser()
	{
		FreeDirResource();
	}

	void CFDirBrowser::FreeDirResource()
	{
		HRESULT hr = E_FAIL;
		if (m_pSelectItemIdList)
		{
			CComPtr<IMalloc> spMalloc;
			COM_VERIFY(SHGetMalloc(&spMalloc));
			if (spMalloc)
			{
				spMalloc->Free(m_pSelectItemIdList);
			}
			else
			{
				CoTaskMemFree(m_pSelectItemIdList);
			}
			m_pSelectItemIdList = NULL;
		}
		m_szPath[0] = NULL;
	}

	BOOL CFDirBrowser::DoModal()
    {
        BOOL bRet = FALSE;
		FreeDirResource();

        m_pSelectItemIdList = SHBrowseForFolder(&m_browseInfo);    //调用显示选择对话框
        if (m_pSelectItemIdList)
        {
			bRet = TRUE;
        }
        return bRet;
    }

	LPCTSTR CFDirBrowser::GetSelectPath()
	{
		BOOL bRet = FALSE;
		if(NULL == m_szPath[0])
		{
			if (NULL != m_pSelectItemIdList)
			{
				API_VERIFY(SHGetPathFromIDList(m_pSelectItemIdList, m_szPath)); //取得文件夹路径到m_szPath里
			}
		}
		return m_szPath;
	}

	LPITEMIDLIST CFDirBrowser::GetSelectPathIdList(BOOL bDetach)
	{
		LPITEMIDLIST pItem = m_pSelectItemIdList;
		if (bDetach)
		{
			m_pSelectItemIdList = NULL;
		}
		return pItem;
	}

    HWND CFDirBrowser::FindTreeViewCtrl(HWND hWnd)
    {
        BOOL bRet = FALSE;
        TCHAR szClassName[FTL_MAX_CLASS_NAME_LENGTH] = {0}; //The maximum length for lpszClassName is 256.
        HWND hWndItem = ::GetWindow(hWnd, GW_CHILD);
        BOOL bFound = FALSE;
        while (hWndItem && !bFound)
        {
            API_VERIFY(::GetClassName(hWndItem, szClassName, _countof(szClassName)) > 0);
            if (bRet)
            {
                if (lstrcmpi(szClassName, WC_TREEVIEW) == 0)
                {
                    //normal style
                    //BrowserDlg -> SysTreeView32
                    bFound = TRUE;
                    break;
                }
                if (lstrcmpi(szClassName, _T("SHBrowseForFolder ShellNameSpace Control")) == 0)
                {
                    //BIF_NEWDIALOGSTYLE
                    //BrowserDlg -> Select Dest Folder(SHBrowseForFolder ShellNameSpace Control) -> SysTreeView32
                    HWND hWndItemInControl = ::GetWindow(hWndItem, GW_CHILD);
                    while (hWndItemInControl)
                    {
                        API_VERIFY(::GetClassName(hWndItemInControl, szClassName, _countof(szClassName)) > 0);
                        if (bRet)
                        {
                            if (lstrcmpi(szClassName, WC_TREEVIEW) == 0)
                            {
                                bFound = TRUE;
                                hWndItem = hWndItemInControl;
                                break;
                            }
                        }
                        hWndItemInControl = ::GetWindow(hWndItemInControl, GW_HWNDNEXT);
                    }
                }
            }
            if (!bFound)
            {
                hWndItem = ::GetWindow(hWndItem, GW_HWNDNEXT);
            }
        }
        return hWndItem;
    }

    BOOL CFDirBrowser::EnsureSelectVisible()
    {
        BOOL bRet = FALSE;
        if (m_hTreeView)
        {
            HTREEITEM hSelected= (HTREEITEM)::SendMessage(m_hTreeView, TVM_GETNEXTITEM, TVGN_CARET, NULL);
            //TreeView_GetSelection(m_hTreeView);
            if (hSelected)
            {
#ifdef _DEBUG
                TCHAR szSelectPath[MAX_PATH] = {0};
                TVITEM item = {0};
                item.hItem = hSelected;
                item.mask = TVIF_TEXT | TVIF_STATE;
                item.pszText = szSelectPath;
                item.cchTextMax = _countof(szSelectPath);
                //TreeView_GetItem(m_hTreeView, &item);
                ::SendMessage(m_hTreeView, TVM_GETITEM, 0, (LPARAM)&item);
                FTLTRACE(TEXT("Select Item Text=%s\n"), szSelectPath);
#endif
                ::SendMessage(m_hTreeView, TVM_ENSUREVISIBLE, 0, (LPARAM)hSelected);
                //TreeView_EnsureVisible(m_hTreeView, hSelected);

                bRet = TRUE;
            }
        }
        return bRet;
    }
    int CFDirBrowser::DirBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {
        //HRESULT hr = E_FAIL;
        BOOL bRet = FALSE;

        //FTLTRACE(TEXT("DirBrowseCallbackProc, uMsg=%d\n"), uMsg);
        CFDirBrowser* pThis = reinterpret_cast<CFDirBrowser*>(lpData);

        switch (uMsg)
        {
        case BFFM_INITIALIZED:
            {
                FTLTRACE(TEXT("DirBrowseCallbackProc - BFFM_INITIALIZED, m_szInitPath=%s\n"), pThis->m_szInitPath);
                if (pThis->m_szInitPath[0])
                {
                    SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pThis->m_szInitPath);
                    pThis->m_hTreeView = pThis->FindTreeViewCtrl(hwnd);
                }
                break;
            }
        case BFFM_SELCHANGED:
            {
                LPITEMIDLIST pItemList = (LPITEMIDLIST)lParam;
                TCHAR szSelPath[MAX_PATH] = {0};
                bRet = SHGetPathFromIDList(pItemList, szSelPath);
                //COM_VERIFY(CFShellUtil::GetItemIdName(pItemList, szSelPath, _countof(szSelPath), SHGDN_FORPARSING));
                if(bRet) //SUCCEEDED(hr))
                {
                    FTLTRACE(TEXT("DirBrowseCallbackProc - BFFM_SELCHANGED, %s\n"), szSelPath);
                    SendMessage(hwnd, BFFM_SETSTATUSTEXT, NULL, (LPARAM)szSelPath);
                }

                if (pThis->m_browseInfo.ulFlags & BIF_NEWDIALOGSTYLE
                    && pThis->m_bFirstEnsureSelectVisible)
                {
                    pThis->m_bFirstEnsureSelectVisible = !pThis->EnsureSelectVisible();
                }
                break;
            }
        case BFFM_VALIDATEFAILED:
            {
                FTLTRACE(TEXT("DirBrowseCallbackProc - BFFM_VALIDATEFAILED, %s\n"), (LPCTSTR)lParam);
                break;
            }
        case BFFM_IUNKNOWN:
            {
                //经测试，至少支持如下接口： IFolderFilter、IFolderFilterSite
                IUnknown *pUnknown = reinterpret_cast<IUnknown*>(lParam);
                UNREFERENCED_PARAMETER(pUnknown);
                FTLTRACE(TEXT("DirBrowseCallbackProc - BFFM_IUNKNOWN, pUnknown=0x%p\n"), pUnknown);

                //COM_DETECT_INTERFACE_FROM_REGISTER(pUnknown);
                break;
            }
        default:
            FTLASSERT(FALSE);
            break;
        }
        return 1;
    }
}

#endif //FTL_SHELL_HPP