#ifndef FTL_FILE_H
#define FTL_FILE_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlfile.h requires ftlbase.h to be included first
#endif
#include <atlstr.h>

namespace FTL
{
//Class
    class CFStructuredStorageFile;
//Function

}//namespace FTL

#include <WinIoctl.h>
//#include "ftlsystem.h"
#include "ftlThread.h"

namespace FTL
{
	//BYTE -> KB -> MB -> GB -> TB -> PB -> EB -> ZB -> YB
    #define BYTES_PER_KILOBYTE      (1024)
    #define BYTES_PER_MEGABYTE      (BYTES_PER_KILOBYTE * 1000)
    #define BYTES_PER_GIGABYTE      (BYTES_PER_MEGABYTE * 1000)
    #define BYTES_PER_TERABYTE      (BYTES_PER_GIGABYTE * 1000)
    #define BYTES_PER_PETABYTE      (BYTES_PER_TERABYTE * 1000)
	// ? 
	//#define BYTES_PER_ZETATABYTE	(BYTES_PER_PETABYTE * 1000)
    /**************************************************************************************************
    * TODO:
    * 0.正在运行的程序可以被移动，但是不能被删除? 原因? 可能是HANDLE等链接?
    *   延迟文件改名:PendingFileRenameOperations ;
    *   延迟文件删除:PendingFileRenameOperations2 ;
    * 1.替换运行中的文件(重启后删除或替换):把正在使用的文件移动到一个临时目录中去->标记为重启时删除->拷贝新文件
    *   HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\下的PendingFileRenameOperations,为REG_MULTI_SZ格式,
    *   TODO: ControlSet001\Control\Session Manager\ 下也有, 是链接吗?
    *   MoveFileEx(cTempFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)
    *   流程:系统重启->Autochk检查->移动指定文件->
	*
	* CShellFileOpenDialog -- 可以显示手机等上面的文件
    *
    * DeviceIoControl -- 直接给设备驱动发送控制命令执行。
    *   设备通常为 volume/directory/file/stream 等,通过CreateFile创建
    **************************************************************************************************/
    
	/**************************************************************************************************
	* 文本文件：
	*   ANSI[没有标识字符] -- 采用当前语言编码保存(如中文936, 韩文949)，文件头没有标识符，在不同语言的OS下会显示成乱码。
	*   UTF-8 without signature[65001] -- UTF8 编码，文件头没有标识符(记事本不支持)
	*   UTF-8 with signature[65001, 0xEFBBBF] -- UTF8编码，文件头有标识符，英文对应的是 0xXX
	*   Unicode[1200, 0xFFFE] -- Unicode 编码，英文对应的是 0xXX00
	*   Unicode Big-Endian[1201, 0xFEFF] -- Unicode 编码，英文对应的是 0x00XX
	*   UTF-7[65000] -- UTF7编码？文件头没有标识符
    *
    * ShareMode(具体测试参见 FtlDemo 的 FilePage )
    *   CreateFile -- 
    *   ofstream -- 默认以 _SH_DENYNO 方式打开(即允许其他的进行读写)， 会造成逻辑错误, 默认值，可在 open 时指定
    *   fopen -- 默认以 允许写 方式打开(似乎没有可以更改的地方
    * 
    *  ofstream 参数问题 -- http://bbs.chinaunix.net/thread-846808-1-1.html
    *  C标准库的setlocale()用法笔记 -- http://www.cnblogs.com/hnrainll/archive/2011/05/07/2039700.html
    *  Unicode下TRACE中文(_CrtDbgReport: String too long or IO Error) -- http://blog.csdn.net/beyond_cn/article/details/37657693
	**************************************************************************************************/
	enum TextFileEncoding
	{
		tfeError = -1,
		tfeUnknown = 0,

		tfeUTF8,					//0xEF BB BF
		tfeUnicode,					//0xFF FE
		tfeUnicodeBigEndian,		//0xFE FF
		tfeUnicodeLittleEndian,     //0xFF FF
	};

	enum CreateLocalFileFlags{
		clfCreateIfNotExist,
		clfAutoRename,
		clfOverwrite,
	};

	const BYTE TEXT_FILE_HEADER_UTF8[]					= { 0xEF, 0xBB, 0xBF };
	const BYTE TEXT_FILE_HEADER_UNICODE[]				= { 0xFF, 0xFE };
	const BYTE TEXT_FILE_HEADER_UNICODE_BIG_ENDIAN[]	= { 0xFE, 0xFF };
	const BYTE TEXT_FILE_HEADER_UNICODE_LITTLE_ENDIAN[] = { 0xFF, 0xFF };
    /**************************************************************************************************
    * IOCP -- 
    * 
	* OVERLAPPED -- 异步I/O是创建高性能可伸缩的应用程序的秘诀，因为它允许单个线程处理来自不同客户机的请求。
	*   注意：
	*   1.不能随便结束发出 I/O 请求的线程
	*     Windows 对异步I/O请求规定了一个限制，即如果线程将一个异步 I/O 请求发送给设备驱动程序，然后终止运行，
	*     那么该 I/O 请求就会丢失，并且在I/O请求运行结束时，没有线程得到这个通知。
	*
    * 共享内存分为两种(注意多线程、进程访问时的同步)：
    *   1.本地共享内存 -- linux 下是 shmget, Windows下是 内存映射文件
	*     ATL 通过 CAtlFileMapping 对文件映射进行了封装
    *   2.分布式共享内存(DSM) -- 在网络上扩展了虚拟内存的概念，通过全局/共享内存中的数据进行透明的进程间通信。
    *     如多台计算机被群集(cluster)为一套逻辑系统，系统中的内存在群集中共享应用程序通过共享内存通信。
    *  
    * CreateFile 可以用来打开各式各样的资源，包括文件(硬盘，软盘，光盘或其他)，串口和并口，Named pipes，console等，
    *   可以在 dwFlagsAndAttributes 变量上设置 FILE_FLAG_OVERLAPPED 属性。
    *   OVERLAPPED结构执行两个功能：
    *     1.识别每一个目前正在进行的overlapped操作；
    *     2.在代码和系统之间提供一个共享区域，参数可以在该区域中双向传递。
    *   GetOverlappedResult
    * 
    *   虽然你要求一个overlapped操作，但它不一定就是overlapped，如果数据已经被放进cache中，或操作系统认为它可以很快地
    *   取得那份数据，那么文件操作就会在ReadFile或WriteFile返回之前完成，而这两个函数将返回TRUE。这种情况下，文件handle
    *   处于激发状态，而对文件的操作可被视为overlaped一样。另外，如果你要求一个文件操作为overlapped，而操作系统把
    *   这个“操作请求”放到队列中等待执行，那么ReadFile和WriteFile都会返回FALSE，这时你必须调用GetLastError并确定
    *   它传回ERROR_IO_PENDING，那意味着“overlapped I/O请求”被放进队列中等待执行
    * 
    * Console 文件
    *   m_hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
    *   if (m_hOutput == INVALID_HANDLE_VALUE) {
    *     API_VERIFY(AllocConsole ());
    *     m_hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
    *   }
    *   SetConsoleTitle (TEXT("ActiveX Debug Output"));  
    *   
    * 通过文件句柄获取文件信息 GetFileInformationByHandle
	*
    * 
	* CreateFile 时如果有 FILE_FLAG_SEQUENTIAL_SCAN， 表示优化Cache，适用于从头到尾顺序访问，不会随机跳
	* GetTempFileName -- 生成指定目录下临时文件的名字，通常用于生成临时文件
	*
	* 读写文件(CFile) 时的性能
	*   1.设置流使用缓冲区：setvbuf(fileLoad.m_pStream, NULL, _IOFBF, 32768);
	*
	**************************************************************************************************/

	/**************************************************************************************************
	* CFileDialog
    *   过滤字符串格式
    *     MFC -- CString strFilter = _T("ImageFile(*.bmp;*.jpg;*.jpeg;*.png)|*.bmp;*.jpg;*.jpeg;*.png|All Files(*.*)|*.*||");
    *     WTL -- CString strFilter = _T("ImageFile(*.bmp;*.jpg;*.jpeg;*.png)|*.bmp;*.jpg;*.jpeg;*.png|All Files(*.*)|*.*||");
    *            strFilter.Replace(TEXT('|'), TEXT('\0')); //注意：如果直接在字符串里面设置 "\0" 是不行的
	*   多选文件:
	*     1.设置 OFN_ALLOWMULTISELECT
	*     2.设置文件名的缓冲区
	*       const int c_cbBuffSize = (MAX_TRANSLATE_FILES_COUNT * (MAX_PATH + 1)) + 1;
	*     	fileDlg.GetOFN().lpstrFile = strMultiPath.GetBuffer(c_cbBuffSize);
	*       fileDlg.GetOFN().nMaxFile = c_cbBuffSize;
	*     3.遍历获取文件
	*       POSITION startPosition = fileDlg.GetStartPosition();
	*       while (startPosition) { CString strPath = fileDlg.GetNextPathName(startPosition); }
	*     4.释放缓冲区资源 strMultiPath.ReleaseBuffer();
	*
	*  初始化路径
	*    TCHAR szDefaultSavePath[MAX_PATH] = {0};
	*    SHGetSpecialFolderPath(NULL, szDefaultSavePath, CSIDL_MYDOCUMENTS , TRUE);
	*    dlg.m_ofn.lpstrInitialDir = szDefaultSavePath;

    **************************************************************************************************/

	class CFFileUtil
	{
	public:
        //获取 CreateFile(xxx, dwDesiredAccess, xxx) 中的 dwDesiredAccess 参数
        FTLINLINE static LPCTSTR GetFileDesiredAccessFlagsString(FTL::CFStringFormater& formater, DWORD dwDesiredAccess, LPCTSTR pszDivide = TEXT("|"));

		FTLINLINE static TextFileEncoding GetTextFileEncoding(LPCTSTR pszFilePath);
		FTLINLINE static HANDLE CreateLocalWriteFile(__inout LPTSTR pszFilePath, DWORD dwMaxSize, CreateLocalFileFlags flags);

        //将符合 xxx(数字索引).xxx 的文件名分解成 文件名 + 索引 的方式
        //如果符合规范，则返回TRUE
        FTLINLINE static BOOL SpitFileNameAndIndex(LPCTSTR pszFileName, LPTSTR pszFilePrefix, DWORD dwMaxSize, int& nIndex);

        FTLINLINE static BOOL DumpMemoryToFile(PVOID pBuffer, DWORD dwSize, LPCTSTR pszFilePath);

		//导出指定Handle(文件、目录、Volume句柄等)通过 DeviceIoControl 获取(或设置)的信息
		FTLINLINE static BOOL DumDeviceIoInformation(HANDLE hDevice);
		
	private:
		FTLINLINE static LPCTSTR WINAPI GetStandardDeviceIoInfo(CFStringFormater& formater, LPBYTE pDeviceIoInfo, DWORD dwDeviceInfoLength);
		FTLINLINE static LPCTSTR WINAPI DeviceIoGetNtfsVolumeDataInfo(CFStringFormater& formater, LPBYTE pDeviceIoInfo, DWORD dwDeviceInfoLength);
		FTLINLINE static LPCTSTR WINAPI DeviceIoGetVolumeDiskExtents(CFStringFormater& formater, LPBYTE pDeviceIoInfo, DWORD dwDeviceInfoLength);
		FTLINLINE static LPCTSTR WINAPI DeviceIoGetUsnJournal(CFStringFormater& formater, LPBYTE pDeviceIoInfo, DWORD dwDeviceInfoLength);
		
	};

#if 0
    class CFConsoleFile
    {
    public:
        FTLINLINE CFConsoleFile();
        virtual ~CFConsoleFile();
    };
#endif 

    /****************************************************************************************************
    * 命名管道( Named Pipe )
    *   CreateNamedPipe|CreateFile -- 打开同一个有什么区别? 
    *   FlushFileBuffers
    ****************************************************************************************************/

    //TODO: 是否可以从 CFFile 继承?
    class CFNamedPipe
    {
    public:
        // Attributes
        HANDLE m_hFile;
    public:

        FTLINLINE BOOL Create(LPCTSTR pszName, DWORD dwAccess = GENERIC_WRITE | GENERIC_READ,
            DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
            LPSECURITY_ATTRIBUTES lpSA = NULL,
            DWORD dwCreationDisposition = OPEN_ALWAYS,
            DWORD dwAttributes = FILE_ATTRIBUTE_NORMAL,
            HANDLE hTemplateFile = NULL);

        
    };

    class CFFile
    {
	public:
		enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };
		// Constructors, destructor
		FTLINLINE CFFile();
		FTLINLINE CFFile(HANDLE hFile);
		FTLINLINE virtual ~CFFile();

		// Attributes
		HANDLE m_hFile;

		// Operations
		FTLINLINE virtual LONGLONG GetPosition() const;
		FTLINLINE virtual CString GetFileName() const;
		FTLINLINE virtual CString GetFilePath() const;
		FTLINLINE virtual BOOL SetFilePath(CString strNewName);

		FTLINLINE virtual BOOL Create(LPCTSTR pszFileName, 
			DWORD dwAccess = GENERIC_WRITE | GENERIC_READ,
			DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
			LPSECURITY_ATTRIBUTES lpSA = NULL,
			DWORD dwCreationDisposition = CREATE_ALWAYS,
			DWORD dwAttributes = FILE_ATTRIBUTE_NORMAL,
			HANDLE hTemplateFile = NULL
			);

        FTLINLINE BOOL Attach(HANDLE hFile);
        FTLINLINE HANDLE Detach();

		FTLINLINE static BOOL Rename(CString strOldName, CString strNewName);
		FTLINLINE static BOOL Remove(CString strFileName);

		FTLINLINE LONGLONG SeekToEnd();
		FTLINLINE LONGLONG SeekToBegin();

		FTLINLINE virtual CFFile * Duplicate() const;

		FTLINLINE virtual LONGLONG Seek(LONGLONG lOff, UINT nFrom);
		FTLINLINE virtual BOOL SetLength(LONGLONG newLen);
		FTLINLINE virtual LONGLONG GetLength() const;

		FTLINLINE virtual BOOL Read(void* lpBuf, DWORD nCount, DWORD* pdwRead, LPOVERLAPPED lpOverlapped = NULL);
		FTLINLINE virtual BOOL Write(const void* lpBuf, DWORD nCount, DWORD* pdwWritten, LPOVERLAPPED lpOverlapped = NULL);
		FTLINLINE BOOL WriteEndOfLine();

		FTLINLINE virtual BOOL LockRange(DWORD dwPos, DWORD dwCount);
		FTLINLINE virtual BOOL UnlockRange(DWORD dwPos, DWORD dwCount);

		FTLINLINE virtual BOOL Flush();
		FTLINLINE virtual BOOL Close();

		enum BufferCommand { 
			bufferRead, 
			bufferWrite, 
			bufferCommit, 
			bufferCheck, 
			bufferBlocking,
			bufferDirect
		};
		FTLINLINE virtual UINT GetBufferPtr(UINT nCommand, UINT nCount = 0, void** ppBufStart = NULL, void** ppBufMax = NULL);

	protected:
		CString m_strFileName;	// stores the file name for the current file
    };

    class CFFileAnsiEncoding
    {
    public:
        FTLINLINE static BOOL WriteEncodingString(CFFile* pFile, const CAtlString& strValue, 
            DWORD* pnBytesWritten, LPOVERLAPPED lpOverlapped = NULL);
    };
    class CFFileUTF8Encoding
    {
    public:
        FTLINLINE static BOOL WriteEncodingString(CFFile* pFile, const CAtlString& strValue, 
            DWORD* pnBytesWritten, LPOVERLAPPED lpOverlapped = NULL);
    };

    class CFFileUnicodeEncoding
    {
    public:
        FTLINLINE static BOOL WriteEncodingString(CFFile* pFile, const CAtlString& strValue, 
            DWORD* pnBytesWritten, LPOVERLAPPED lpOverlapped = NULL);
    };

    template <typename TEncoding>
    class CFTextFile : public CFFile
    {
    public:
        typedef CFTextFile< TEncoding > thisClass;
        FTLINLINE CFTextFile(TextFileEncoding fileEncoding);
        FTLINLINE BOOL WriteFileHeader(LPOVERLAPPED lpOverlapped = NULL);
        FTLINLINE BOOL WriteString(const CAtlString&strValue, DWORD* pnBytesWritten = NULL, LPOVERLAPPED lpOverlapped = NULL);
    private:
        TextFileEncoding    m_fileEncoding;
    };

    typedef CFTextFile<CFFileAnsiEncoding>    CFAnsiFile;
    typedef CFTextFile<CFFileUnicodeEncoding> CFUnicodeFile;

    //有 ATLPath 实现了很多功能, 如 RemoveFileSpec(删除路径最后的文件名)
    class CFPath
    {
    public:
		//创建指定路径中的全部目录
		FTLINLINE static BOOL CreateDirTree(LPCTSTR szPath);
        FTLINLINE static BOOL GetRelativePath(LPCTSTR pszFullPath, LPCTSTR pszParentPath, LPTSTR pszRelateivePath, UINT cchMax);

        //将长路径转成短路径(网上拷贝，尚未测试) -- 已有有 GUI 相关的实现
        FTLINLINE static CAtlString GetShortPath(LPCTSTR pszFullPath, long nMaxLength);
    private:
        
    };


    enum FileFindResultHandle
    {
        rhContinue,
        rhSkipDir,  //just use for skip special dir
        rhStop,
    };

    class IFileFindCallback
    {
    public:
        virtual FileFindResultHandle OnFindFile(LPCTSTR pszFilePath, const WIN32_FIND_DATA& findData, LPVOID pParam) = 0;
    };

    //同步查找指定目录 -- 采用了递推方式来避免递归方式的问题
    class CFFileFinder
    {
    public:
        FTLINLINE CFFileFinder();
        FTLINLINE VOID SetCallback(IFileFindCallback* pCallBack, LPVOID pParam);
        FTLINLINE BOOL Find(LPCTSTR pszDirPath, LPCTSTR pszFilter = _T("*.*"), BOOL bRecursive = TRUE);
    protected:
        IFileFindCallback*  m_pCallback;
        LPVOID              m_pParam;
        CAtlString          m_strDirPath;
        CAtlString          m_strFilter;
        typedef std::deque<CAtlString>  FindDirsContainer;      //递归方式时保存需要查找的子目录
        FindDirsContainer  m_FindDirs;
        typedef std::list<CAtlString>   FindFiltersContainer;   //保存通过分号区分开的多种扩展名
        FindFiltersContainer    m_FindFilters;
        FTLINLINE   BOOL _isMatchFilterFile(LPCTSTR pszFileName);
    };


    class ICopyDirCallback
    {
    public:
        enum CallbackType
        {
            cbtPrepareSourceFiles,         //prepare source files
            cbtBegin,
            cbtCopyFile,
            cbtEnd,
            cbtError,
        };
        virtual VOID OnBeginPrepareSourceFiles(LPCTSTR pszSrcDir, LPCTSTR pszDstDir) = 0;
        virtual VOID OnBegin(LONGLONG nTotalSize, LONG nFileCount) = 0;
        virtual VOID OnCopyFile(LPCTSTR pszSrcFile, LPCTSTR pszTargetFile, LONG nIndex, LONGLONG nFileSize, LONGLONG nCopiedSize) = 0;
        virtual VOID OnEnd(BOOL bSuccess, LONGLONG nTotalCopiedSize, LONG nCopiedFileCount) = 0;
        virtual VOID OnError(LPCTSTR pszSrcFile, LPCTSTR pszTargetFile, DWORD dwError) = 0;
    };
    class CFDirectoryCopier : public IFileFindCallback
    {
    public:
        FTLINLINE CFDirectoryCopier();
        FTLINLINE ~CFDirectoryCopier();
        FTLINLINE BOOL SetCallback(ICopyDirCallback* pCallback);
        FTLINLINE BOOL SetCopyEmptyFolder(BOOL bCopyEmptyFolder);
        FTLINLINE BOOL Start(LPCTSTR pszSourcePath, LPCTSTR pszDestPath, 
            LPCTSTR pszFilter = _T("*.*"), 
            BOOL bFailIfExists = FALSE, 
            BOOL bRecursive = TRUE);
        FTLINLINE BOOL IsPaused();
        FTLINLINE BOOL PauseOrResume();
        FTLINLINE BOOL Stop();
        FTLINLINE BOOL WaitToEnd(DWORD dwMilliseconds = INFINITE);
    public:
        FTLINLINE virtual FileFindResultHandle OnFindFile(LPCTSTR pszFilePath, const WIN32_FIND_DATA& findData, LPVOID pParam);
    protected:
        ICopyDirCallback*               m_pCallback;

        LONGLONG                        m_nCurCopyFileSize;
        LONGLONG                        m_nTotalSize;
        LONGLONG                        m_nCopiedFileSize;
        LONGLONG                        m_nCurFileTransferred;

        LONG                            m_nFileCount;
        LONG                            m_nCopyFileIndex;
        LONG                            m_nTotalCopiedFileCount;

        BOOL                            m_bFailIfExists;
        BOOL                            m_bRecursive;
        BOOL                            m_bCopyEmptyFolder;
        BOOL                            m_bCancelForCopyFileEx;

        CAtlString                      m_strCurSrcFilePath;
        CAtlString                      m_strCurDstFilePath;
        CAtlString                      m_strSrcDirPath;
        CAtlString                      m_strDstDirPath;
        CAtlString                      m_strFilter;


        struct SourceFileInfo{
            CAtlString  strFullPath;
            BOOL        isDirectory;
            LONGLONG    nFileSize;
        };
        typedef std::list<SourceFileInfo>   SourceFileInfoContainer;

        SourceFileInfoContainer         m_sourceFiles;
        CFThread<DefaultThreadTraits>   m_threadCopy;

        FTLINLINE VOID   _InitValue();
        FTLINLINE static DWORD __stdcall _CopierThreadProc(LPVOID lpThreadParameter);
        FTLINLINE DWORD _InnerCopierThreadProc();


        FTLINLINE static DWORD CALLBACK _CopyFileProgressRoutine(LARGE_INTEGER TotalFileSize,
            LARGE_INTEGER TotalBytesTransferred,
            LARGE_INTEGER StreamSize,
            LARGE_INTEGER StreamBytesTransferred,
            DWORD dwStreamNumber,
            DWORD dwCallbackReason,
            HANDLE hSourceFile,
            HANDLE hDestinationFile,
            LPVOID lpData
            );

        FTLINLINE VOID _NotifyCallBack(ICopyDirCallback::CallbackType type, DWORD dwError = 0);

        FTLINLINE FTLThreadWaitType _PrepareSourceFiles();
        FTLINLINE FTLThreadWaitType _CopyFiles();
        
    };

    class CFStructuredStorageFile
    {
    public:
        const static DWORD STRUCTURED_STORAGE_FILE_DEFAULT_MODE = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
        FTLINLINE CFStructuredStorageFile();
        FTLINLINE virtual ~CFStructuredStorageFile();
        FTLINLINE HRESULT CreateDocFile(LPCTSTR pszFilePath, DWORD mode = STGM_CREATE | STRUCTURED_STORAGE_FILE_DEFAULT_MODE);
        FTLINLINE HRESULT OpenDocFile(LPCTSTR pszFilePath,DWORD mode = STRUCTURED_STORAGE_FILE_DEFAULT_MODE);
        FTLINLINE void Close();
        FTLINLINE IStorage * Attach(IStorage* pNews);
        FTLINLINE IStorage * Detach();
        // storage-level access:
        FTLINLINE HRESULT CreateStorage(LPCTSTR pName,BOOL bEnter = TRUE, DWORD mode = STRUCTURED_STORAGE_FILE_DEFAULT_MODE);
        FTLINLINE HRESULT EnterStorage(LPCTSTR pName,DWORD mode = STRUCTURED_STORAGE_FILE_DEFAULT_MODE);
        FTLINLINE HRESULT ExitStorage();
        //bool CreateStream(const CString & name, COleStreamFile &sf, DWORD mode = CFile::modeReadWrite | CFile::shareExclusive);
        //bool OpenStream(const CString & name, COleStreamFile &sf, DWORD mode = CFile::modeReadWrite | CFile::shareExclusive);
        FTLINLINE HRESULT CreateStream(LPCTSTR pName,IStream** ppChildStream,DWORD mode = STRUCTURED_STORAGE_FILE_DEFAULT_MODE);
        FTLINLINE HRESULT OpenStream(LPCTSTR pName,IStream** ppChildStream,DWORD mode = STRUCTURED_STORAGE_FILE_DEFAULT_MODE);
        FTLINLINE HRESULT DestroyElement(LPCTSTR pName);
        // status info:
        FTLINLINE IStorage* GetRootStorage() const;
        FTLINLINE IStorage* GetCurrentStorage() const;
        FTLINLINE BOOL IsOpen() const;
        //CString GetPath(const CString & SepChar) const;
        //CString GetFilename() const;
    private:
        struct StorageData
        {
            IStorage *Stg;
            StorageData *ParentStg;
            StorageData()
            {
                Stg = NULL;
                ParentStg = NULL;
            }
        };
        IStorage    *m_pRootStg;
        StorageData *m_pCurrentStg;
    };

    class CFileSystemFinder
    {

    };

}//namespace FTL

#endif //FTL_FILE_H

#ifndef USE_EXPORT
#  include "ftlFile.hpp"
#endif