#ifndef FTL_LOG_HPP
#define FTL_LOG_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlLog.h"
#endif

#  include <io.h>
#  include <fcntl.h>
#  include <atlsecurity.h>

#include "ftlSharedVariable.h"
#include "ftlConversion.h"
//#include <atlcoll.h>
#include <map>

namespace FTL
{
#if 0
    static void DbgPrint(LPCWSTR format, ...) {
        if (g_DebugMode) {
            const WCHAR *outputString;
            WCHAR *buffer = NULL;
            size_t length;
            va_list argp;

            va_start(argp, format);
            length = _vscwprintf(format, argp) + 1;
            buffer = _malloca(length * sizeof(WCHAR));
            if (buffer) {
                vswprintf_s(buffer, length, format, argp);
                outputString = buffer;
            }
            else {
                outputString = format;
            }
            if (g_UseStdErr)
                fputws(outputString, stderr);
            else
                OutputDebugStringW(outputString);
            if (buffer)
                _freea(buffer);
            va_end(argp);
            if (g_UseStdErr)
                fflush(stderr);
        }
    }
#endif
    CFLogSink::CFLogSink(BOOL isDebug)
    {
        m_bDebug = isDebug;
    }
    CFLogSink::~CFLogSink()
    {

    }

    CFLogFileSink::CFLogFileSink(BOOL isDebug)
        :CFLogSink(isDebug)
    {

    }
#if 0
    CFLogFileSinkAPI::CFLogFileSinkAPI(BOOL isDebug)
        : CFLogFileSink(isDebug)
    {
        m_hLogFile = INVALID_HANDLE_VALUE;
    }

    CFLogFileSinkAPI::~CFLogFileSinkAPI()
    {
        Close();
    }

    BOOL CFLogFileSinkAPI::IsValid()
    {
        return (m_hLogFile != INVALID_HANDLE_VALUE);
    }

    BOOL CFLogFileSinkAPI::Open(LPCTSTR pszFilePath)
    {
        BOOL bRet = FALSE;
        bRet = (INVALID_HANDLE_VALUE != (m_hLogFile = ::CreateFile(pszFilePath,
            FILE_GENERIC_WRITE,
            //GENERIC_WRITE | GENERIC_READ ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)));
        ATLASSERT(bRet);

        //FILE_GENERIC_WRITE vs. GENERIC_WRITE

        if (bRet)
        {
            bRet = ::SetEndOfFile(m_hLogFile);
            ATLASSERT(bRet);
        }

        return bRet;
    }

    BOOL CFLogFileSinkAPI::WriteLogBuffer(LPCVOID pBuffer, LONG nLen, LONG* pWritten)
    {
        BOOL bRet = FALSE;
        DWORD dwWritten = 0;
        bRet = ::WriteFile(m_hLogFile, pBuffer, nLen, &dwWritten, NULL);
        ATLASSERT(bRet);

        if (bRet && pWritten)
        {
            *pWritten = dwWritten;
        }
        return bRet;
    }

    BOOL CFLogFileSinkAPI::Flush()
    {
        BOOL bRet = FALSE;
        if (m_hLogFile)
        {
            bRet = ::FlushFileBuffers(m_hLogFile);
            ATLASSERT(bRet);
        }
        return bRet;
    }

    BOOL CFLogFileSinkAPI::Close()
    {
        BOOL bRet = TRUE;
        if (INVALID_HANDLE_VALUE != m_hLogFile)
        {
            bRet = FlushFileBuffers(m_hLogFile);
            ATLASSERT(bRet);

            ATLVERIFY(CloseHandle(m_hLogFile));
            m_hLogFile = INVALID_HANDLE_VALUE;
        }
        return bRet;
    }
#endif //if 0
    CFLogFileSinkLowIO::CFLogFileSinkLowIO(BOOL isDebug)
        :CFLogFileSink(isDebug)
    {
        m_nLogFile = -1;
    }
    CFLogFileSinkLowIO::~CFLogFileSinkLowIO()
    {
        Close();
    }

    BOOL CFLogFileSinkLowIO::IsValid()
    {
        return (m_nLogFile != -1);
    }

    BOOL CFLogFileSinkLowIO::Open(LPCTSTR pszFilePath)
    {
        //_wfopen
        //_tfsopen()
        BOOL bRet = FALSE;

        //默认是 O_TEXT ? Win 上会自动把 \r 改成 \r\n, 然后造成计算大小时出现偏差
        int oFlags = O_WRONLY | O_BINARY | O_CREAT | O_APPEND;

        int errNo = _tsopen_s(&m_nLogFile, pszFilePath, oFlags, _SH_DENYNO, _S_IREAD | _S_IWRITE);
        ATLASSERT(m_nLogFile > 0);
        if (m_nLogFile > 0)
        {
            bRet = TRUE;
        } else {
            SetLastError(errNo);
        }

        if (m_bDebug)
        {
            ATLTRACE(TEXT("ftl open %s result, m_nLogFile=%d, errNo=%d\n"), pszFilePath, m_nLogFile, errNo);
        }

        return bRet;
    }

    BOOL CFLogFileSinkLowIO::WriteLogBuffer(LPCVOID pBuffer, LONG nLen, LONG* pWritten)
    {
        BOOL bRet = FALSE;
        //在 NFS 等共享磁盘上 flock(fd, LOCK_EX|LOCK_NB) +  lseek(fd, 0, SEEK_END)  + flock(fd, LOCK_UN)
        int nWriten = _write(m_nLogFile, pBuffer, nLen);
        ATLASSERT(nWriten > 0);
        if (nWriten)
        {
            bRet = TRUE;
            if (pWritten)
            {
                *pWritten = nWriten;
            }
        }
        return bRet;
    }

	LONG CFLogFileSinkLowIO::GetLength()
	{
		return (LONG)_filelengthi64(m_nLogFile);
	}

    BOOL CFLogFileSinkLowIO::Flush()
    {
        BOOL bRet = FALSE;
        ATLASSERT(FALSE);  //how to ?
        if (m_nLogFile)
        {
            //fsync()
            //ioctl(fd, IOCTL_COMMAND, args); 
        }
        return bRet;
    }

    BOOL CFLogFileSinkLowIO::Close()
    {
        BOOL bRet = TRUE;
        if (m_nLogFile > 0)
        {
            _close(m_nLogFile);
            m_nLogFile = -1;
        }
        return bRet;
    }

#if 0
    CFLogFileSinkStdIO::CFLogFileSinkStdIO(BOOL isDebug)
        :CFLogFileSink(isDebug)
    {
        m_pLogFile = NULL;
    }
    CFLogFileSinkStdIO::~CFLogFileSinkStdIO()
    {
        Close();
    }

    BOOL CFLogFileSinkStdIO::IsValid()
    {
        return (m_pLogFile != NULL);
    }
    BOOL CFLogFileSinkStdIO::Open(LPCTSTR pszFilePath)
    {
        BOOL bRet = FALSE;
        //_tfopen()

        //打开具有文件共享的流,做好准备以进行后续的共享读写
        ATLASSERT(NULL == m_pLogFile);

        m_pLogFile = _tfsopen(pszFilePath, TEXT("a+"), _SH_DENYNO);
        ATLASSERT(NULL != m_pLogFile);
        if (m_pLogFile)
        {
            bRet = TRUE;
        }
        return bRet;
    }

    BOOL CFLogFileSinkStdIO::WriteLogBuffer(LPCVOID pBuffer, LONG nLen, LONG* pWritten)
    {
        //flock + seek?
        BOOL bRet = FALSE;
        if (m_pLogFile)
        {
            int nWritten = (int)fwrite(pBuffer, 1, nLen, m_pLogFile);
            ATLASSERT(nWritten == nLen);  //TODO: how? disk full ?

            bRet = (nWritten == nLen);
            if (bRet && pWritten)
            {
                *pWritten = nWritten;
            }
        }

        return bRet;
    }

    BOOL CFLogFileSinkStdIO::Flush()
    {
        BOOL bRet = FALSE;
        if (m_pLogFile)
        {
            int nRet = fflush(m_pLogFile);
            bRet = nRet > 0;
        }
        return bRet;
    }
    BOOL CFLogFileSinkStdIO::Close()
    {
        BOOL bRet = FALSE;
        if (m_pLogFile)
        {
            int nRet = fclose(m_pLogFile);
            m_pLogFile = NULL;
        }
        return TRUE;
    }

    BOOL CFLogFileSinkStdWithLowIO::Open(LPCTSTR pszFilePath)
    {
        BOOL bRet = FALSE;
        int oFlags = O_RDWR | O_TEXT | O_CREAT | O_APPEND;

        //int fd = _topen(pszFilePath, oFlags, 0666);
        int fd = 0;
        int errNo = _tsopen_s(&fd, pszFilePath, oFlags, _SH_DENYNO, _S_IREAD | _S_IWRITE);
        ATLASSERT(fd > 0);
        if (fd > 0)
        {
            m_pLogFile = _tfdopen(fd, TEXT("a+"));
            ATLASSERT(m_pLogFile);
            if (m_pLogFile)
            {
                bRet = TRUE;
            }
            else {
                _close(fd);
            }
        }
        return bRet;
    }
#endif //if 0

    struct PROCESS_LOG_INFO
    {
        DWORD PID;
        HANDLE hMutexLock;
        HANDLE hLogFile;
    };

    FTLINLINE BOOL CALLBACK LogShareInitializeProc(BOOL bFirstCreate, FTL_LOG_SHARE_INFO& rValue)
    {

        BOOL bRet = FALSE;
        if (bFirstCreate)
        {
            ZeroMemory(&rValue, sizeof(FTL_LOG_SHARE_INFO));
            rValue.metaInfo.nFileIndex = 0;

#if 1
            //NDrive 中不启用这些代码
            rValue.metaInfo.logThreshold = tlInfo;
#ifdef _DEBUG
            rValue.metaInfo.logTarget = (FTL_LOG_TARGET_DEBUG_VIEW | FTL_LOG_TARGET_LOCAL_FILE);
#else		
            rValue.metaInfo.logTarget = (FTL_LOG_TARGET_LOCAL_FILE);
#endif 
            rValue.metaInfo.dateTimeFmt = ltfTime;
            rValue.metaInfo.maxLogSize = 50 * 1024 * 1024;  //50M
            rValue.metaInfo.maxTotalSize = 200 * 1024 * 1024;	//200M

            TCHAR szLogPath[MAX_PATH] = { 0 }, szModuleName[MAX_PATH] = { 0 };
            ATLVERIFY(GetTempPath(_countof(szLogPath), szLogPath) > 0);

            GetModuleFileName(NULL, szModuleName, _countof(szModuleName));
            PathRemoveExtension(szModuleName);
            LPCTSTR pszModuleFileName = ::PathFindFileName(szModuleName);
            ATLASSERT(NULL != pszModuleFileName);
            StringCchPrintf(rValue.metaInfo.szBaseName, _countof(rValue.metaInfo.szBaseName), TEXT("%s_"), pszModuleFileName);

			StringCchPrintf(rValue.metaInfo.szLockMutextName, _countof(rValue.metaInfo.szLockMutextName), TEXT("%s"), pszModuleFileName);

            FTL::CFStringFormater formater;
            ATLVERIFY(SUCCEEDED(formater.AppendFormat(TEXT("FTL\\")))); //, pszModuleFileName

            ATLVERIFY(PathAppend(szLogPath, formater.GetString()));
            bRet = (CreateDirectory(szLogPath, NULL));
            ATLASSERT(bRet || ERROR_ALREADY_EXISTS == GetLastError());


            StringCchCopy(rValue.metaInfo.szLogPath, _countof(rValue.metaInfo.szLogPath), szLogPath);
            //TEXT("F:\\Fujie\\FJSDK_Export\\Windows\\FTL\\Debug\\"));
            //TEXT("G:\\Fujie\\FJSDK\\Windows\\FTL\\Debug\\"));

#endif 
        }

        ATLTRACE(TEXT("Leave LogShare Initialize Proc, msgIndex=%d, curLogSize=%d\n"),
            rValue.metaInfo.msgIndex, rValue.metaInfo.curLogSize);

        return bRet;
    }

    FTLINLINE BOOL CALLBACK LogShareFinalizeProc(BOOL bFirstCreate, FTL_LOG_SHARE_INFO& rValue)
    {
        BOOL bRet = FALSE;
        ATLTRACE(TEXT("Enter Log Share Finalize Proc, msgIndex=%d, curLogSize=%d\n"),
            rValue.metaInfo.msgIndex, rValue.metaInfo.curLogSize);

        return bRet;
    }

    CFLogger::CFLogger(FTL_LOG_SHARE_INFO& rLogShareInfo)
        :m_rLogShareInfo(rLogShareInfo)
    {
        m_isDebug = FALSE;
        m_nCurInstanceIndex = InterlockedIncrement(&CFLogger::s_LoggerInstanceCount);
        ATL::CSecurityDesc sd;
        ATL::CDacl dacl;
        dacl.AddAllowedAce(ATL::Sids::World(), GENERIC_ALL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);
        sd.SetDacl(dacl);

        SECURITY_ATTRIBUTES sa = { 0 };
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = (LPVOID)sd.GetPSECURITY_DESCRIPTOR();
        sa.bInheritHandle = FALSE;

        m_pLockObj = new ATL::CMutex(&sa, FALSE, rLogShareInfo.metaInfo.szLockMutextName);

        if (m_isDebug)
        {
            ATLTRACE(TEXT("Enter CFLogger::CFLogger, index=%d, total=%d, this=0x%p\n"),
                m_nCurInstanceIndex, s_LoggerInstanceCount, this);
        }

        //m_pLogWritter = new CFLogFileSinkAPI(isDebug);
        m_pLogWritter = new CFLogFileSinkLowIO(m_isDebug);
        //m_pLogWritter = new CFLogFileSinkStdIO(isDebug);
        //m_pLogWritter = new CFLogFileSinkStdWithLowIO(isDebug);
        m_nCurFileIndex = -1;
        m_dwCurLogDate = -1;
        ZeroMemory(m_szLogFilePath, sizeof(m_szLogFilePath));
    }

    CFLogger::~CFLogger()
    {
        InterlockedDecrement(&CFLogger::s_LoggerInstanceCount);
        if (m_isDebug)
        {
            ATLTRACE(TEXT("Enter CFLogger::~CFLogger, index=%d, remain=%d, this=0x%p\n"),
                m_nCurInstanceIndex, s_LoggerInstanceCount, this);
        }

        Close(TRUE);

        if (m_pLogWritter)
        {
            //在 nDrive 项目里面, 由于存在全局变量, 在日志模块释放完以后,全局变量的析构还会再次调用日志模块,
            //因此此处不能 delete, 否则会造成空指针异常, 此处的问题就是可能会有内存泄漏( static 变量, 内存泄漏不高)
            m_pLogWritter->Close();

            delete m_pLogWritter;
            m_pLogWritter = NULL;
        }

        SAFE_DELETE(m_pLockObj);
    }

    VOID CFLogger::SetTraceOptions(const LoggerTraceOptions& traceOptions)
    {
        ATL::CMutexLock locker(*m_pLockObj);

        if ((traceOptions.dwOptionsFlags & LOGGER_TRACE_OPTION_LOG_PATH) && traceOptions.pszLogPath)
        {
            StringCchCopy(m_rLogShareInfo.metaInfo.szLogPath, _countof(m_rLogShareInfo.metaInfo.szLogPath), traceOptions.pszLogPath);
        }
        if ((traceOptions.dwOptionsFlags & LOGGER_TRACE_OPTION_LOG_THRESHOLD))
        {
            m_rLogShareInfo.metaInfo.logThreshold = traceOptions.logThreshold;
        }

        if ((traceOptions.dwOptionsFlags & LOGGER_TRACE_OPTION_LOG_TARGET))
        {
            m_rLogShareInfo.metaInfo.logTarget = traceOptions.logTarget;
        }

        if ((traceOptions.dwOptionsFlags & LOGGER_TRACE_OPTION_MAX_LOG_FILE_SIZE))
        {
            m_rLogShareInfo.metaInfo.maxLogSize = traceOptions.maxLogFileSize;
        }

        if ((traceOptions.dwOptionsFlags & LOGGER_TRACE_OPTION_MAX_TOTAL_LOG_SIZE))
        {
            m_rLogShareInfo.metaInfo.maxTotalSize = traceOptions.maxTotalLogSize;
        }
    }

    BOOL CFLogger::CheckLevel(TraceLevel level)
    {
        BOOL bRet = (level >= m_rLogShareInfo.metaInfo.logThreshold);
        return bRet;
    }

    LPCTSTR CFLogger::GetLevelName(TraceLevel level)
    {
        LPCTSTR pszLevelName = TEXT("U");  //Unknown
        switch (level)
        {
        case tlDetail:
            pszLevelName = TEXT("D");
            break;
        case tlInfo:
            pszLevelName = TEXT("I");
            break;
        case tlTrace:
            pszLevelName = TEXT("T");
            break;
        case tlWarn:
            pszLevelName = TEXT("W");
            break;
        case tlError:
            pszLevelName = TEXT("E");
            break;
        default:
            ATLASSERT(FALSE);
            break;
        }
        return pszLevelName;
    }

    ULONG CFLogger::GetNextTraceSequenceNumber()
    {
        ULONG nNextSeq = InterlockedIncrement(&m_rLogShareInfo.metaInfo.msgIndex);
        return nNextSeq;
    }

    LPCWSTR CFLogger::GetCurrentTimeStrW(LPWSTR pszTimeBuf, INT nTimeBufCount)
    {
        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);
		HRESULT hr = E_FAIL;
		switch (m_rLogShareInfo.metaInfo.dateTimeFmt)
		{
        case ltfTicket:
            hr = StringCchPrintf(pszTimeBuf, nTimeBufCount, L"%d", GetTickCount());
            break;
        case ltfDateTime:
            hr = StringCchPrintfW(pszTimeBuf, nTimeBufCount, L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            break;
        case ltfTime:
        default: //missing value, default is time
            hr = StringCchPrintfW(pszTimeBuf, nTimeBufCount, L"%02d:%02d:%02d.%03d",
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            break;
        }
        ATLASSERT(SUCCEEDED(hr));
        return pszTimeBuf;
    }

    LPCSTR CFLogger::GetCurrentTimeStrA(LPSTR pszTimeBuf, INT nTimeBufCount)
    {
        HRESULT hr = E_FAIL;
        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);
        switch (m_rLogShareInfo.metaInfo.dateTimeFmt)
        {
        case ltfDateTime:
            hr = StringCchPrintfA(pszTimeBuf, nTimeBufCount, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            break;
        case ltfTicket:
            hr = StringCchPrintfA(pszTimeBuf, nTimeBufCount, "%d", GetTickCount());
            break;
        case ltfTime:
        default: //missing value, default is time
            hr = StringCchPrintfA(pszTimeBuf, nTimeBufCount, "%02d:%02d:%02d.%03d",
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            break;
        }

        ATLASSERT(SUCCEEDED(hr));
        return pszTimeBuf;
    }

    VOID /*__cdecl*/ CFLogger::WriteLogInfoExW(TraceLevel level, const LPCWSTR lpszFormat, ...)
    {
        ATL::CAtlStringW strFormater;

        va_list argsp;
        va_start(argsp, lpszFormat);
        strFormater.FormatV(lpszFormat, argsp);
        //strAtl.FormatMessageV(lpszFormat, argsp);
        va_end(argsp);

        if (FTL_LOG_TARGET_DEBUG_VIEW & m_rLogShareInfo.metaInfo.logTarget)
        {
            OutputDebugStringW(strFormater.GetString());
        }
        if (FTL_LOG_TARGET_LOCAL_FILE & m_rLogShareInfo.metaInfo.logTarget)
        {
            FTL::CFConversion conv;
            INT nLength = 0;
            LPCSTR pszUTF8 = conv.UTF16_TO_UTF8(strFormater.GetString(), &nLength);

            //BOOL bFlush = (level >= tlError);
            WriteLogBuffer(pszUTF8, nLength, FALSE);
        }
    }

    VOID /*__cdecl*/ CFLogger::WriteLogInfoExA(TraceLevel level, const LPCSTR lpszFormat, ...)
    {
        ATL::CAtlStringA formater;

        va_list argsp;
        va_start(argsp, lpszFormat);
        formater.FormatV(lpszFormat, argsp);
        //strAtl.AppendFormatV(lpszFormat, argsp);
        va_end(argsp);

        if (FTL_LOG_TARGET_DEBUG_VIEW & m_rLogShareInfo.metaInfo.logTarget)
        {
            OutputDebugStringA(formater.GetString());
        }
        if (FTL_LOG_TARGET_LOCAL_FILE & m_rLogShareInfo.metaInfo.logTarget)
        {
            //BOOL bFlush = (level >= tlError);
            WriteLogBuffer(formater.GetString(), formater.GetLength(), FALSE);
        }
    }


    VOID CFLogger::Close(BOOL bFlush)
    {
        BOOL bRet = FALSE;

        ATL::CMutexLock locker(*m_pLockObj);
        if (m_pLogWritter)
        {
            if (bFlush)
            {
                Flush();
            }
            m_pLogWritter->Close();
        }
        m_nCurFileIndex = -1;
        m_dwCurLogDate = -1;
    }

    BOOL CFLogger::_Rotate(BOOL bFlush, BOOL bIncFileIndex)
    {
        BOOL bRet = FALSE;
        if (m_isDebug)
        {
            ATLTRACE(TEXT("Enter _Rotate, bFlush=%d, bIncFileIndex=%d msgndex=%d, fileIndex=%d, curLogSize=%d\n"),
                bFlush, bIncFileIndex, m_rLogShareInfo.metaInfo.msgIndex,
                m_rLogShareInfo.metaInfo.nFileIndex, m_rLogShareInfo.metaInfo.curLogSize);
        }

        Close(bFlush);
        _CleanOldLogs();

        m_rLogShareInfo.metaInfo.curLogSize = 0;
        if (bIncFileIndex)
        {
            InterlockedIncrement(&m_rLogShareInfo.metaInfo.nFileIndex);
        }
        bRet = (_CreateLogFile());
        ATLASSERT(bRet);
        return bRet;
    }

    FTLINLINE DWORD CFLogger::_GetCurLogDateInfo(const SYSTEMTIME& st)
    {
        DWORD dwCheckLogDate = st.wYear * 10000 + st.wMonth * 100 + st.wDay * 1; // +st.wHour * 1;
        //以下部分用于调试时加速日志的切换
        //DWORD dwCheckLogDate = st.wMonth * 1000000 + st.wDay * 10000 + st.wHour * 100 + st.wMinute * 1;
        return dwCheckLogDate;
    }

    FTLINLINE BOOL CFLogger::_CleanOldLogs()
    {
        HRESULT hr = E_FAIL;
        TCHAR szFindLogFiles[MAX_PATH] = { 0 };
        ULONG nTotalLogSize = 0;

        std::map<ATL::CAtlString, DWORD>  fileSizeMap;

        hr = StringCchPrintf(szFindLogFiles, _countof(szFindLogFiles), TEXT("%s%s") TEXT("*.log"),
            m_rLogShareInfo.metaInfo.szLogPath, m_rLogShareInfo.metaInfo.szBaseName);

        WIN32_FIND_DATA findData = { 0 };
        HANDLE hFindFile = FindFirstFile(szFindLogFiles, &findData);
        if (INVALID_HANDLE_VALUE != hFindFile)
        {
            do
            {
                ATLASSERT(0 == (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)); // file
                if (0 == (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    ATL::CAtlString fullPath;
                    fullPath.Format(TEXT("%s%s"), m_rLogShareInfo.metaInfo.szLogPath, findData.cFileName);
                    //fileSizeMap.SetAt(fullPath, )
                    fileSizeMap[fullPath] = findData.nFileSizeLow;  //small than 4G
                    nTotalLogSize += (ULONG)findData.nFileSizeLow;
                }
            } while (::FindNextFile(hFindFile, &findData));
            ATLVERIFY(FindClose(hFindFile));
            hFindFile = INVALID_HANDLE_VALUE;
        }

        while (nTotalLogSize > m_rLogShareInfo.metaInfo.maxTotalSize && !fileSizeMap.empty())
        {
            ATL::CAtlString strFirstFilePath = fileSizeMap.begin()->first;
            ULONG nFirstFileSize = fileSizeMap.begin()->second;

            if (m_isDebug)
            {
                ATLTRACE(TEXT("cur Total Log Size(%u) > allow maxTotalSize(%u), will remove %s(%d)\n"),
                    nTotalLogSize, m_rLogShareInfo.metaInfo.maxTotalSize, strFirstFilePath, nFirstFileSize);
            }

            ::DeleteFile(strFirstFilePath);	    //有可能会被占用,而造成删除失败, 如果失败则不要断言
            nTotalLogSize -= nFirstFileSize;  //不管是否成功都认为成功,否则删除失败时(如很久不写日志的进程还占用文件),会造成无限循环

            fileSizeMap.erase(fileSizeMap.begin());
        }

        return TRUE;
    }


    LONG CFLogger::WriteLogBuffer(LPCSTR pszUtf8, INT nLength, BOOL bFlush)
    {
        BOOL bRet = FALSE;
        //FTL::CFConversion conv;
        //LPCSTR pszUtf8 = conv.TCHAR_TO_UTF8(pszMessage, &nLength);

        {
            ATL::CMutexLock locker(*m_pLockObj);
            SYSTEMTIME st = { 0 };
            GetLocalTime(&st);

            //当前文件达到单个文件最大(先写, 再关,从而保证其他进程可以检测到这个文件不能再打开继续写)
            BOOL isLarger = (m_rLogShareInfo.metaInfo.maxLogSize > 0 && m_rLogShareInfo.metaInfo.curLogSize + nLength > m_rLogShareInfo.metaInfo.maxLogSize);

            //TODO: 当一个很久都没有写日志的进程再次尝试写时
            BOOL curIsLarger = (m_rLogShareInfo.metaInfo.maxLogSize > 0 && m_rLogShareInfo.metaInfo.curLogSize > m_rLogShareInfo.metaInfo.maxLogSize);

            ///按日期风格检查的话, 已经变化了,需要先关再写
            DWORD dwCheckLogDate = _GetCurLogDateInfo(st);
            BOOL isLogDataTimeChanged = (m_dwCurLogDate != -1 && m_dwCurLogDate != dwCheckLogDate);

            // 整体的文件索引已经变化, 当前正在写的文件索引不同,则需要更改(先关, 再写), 比如一个很久都没有写日志的进程再次尝试写时会出现这种情况
            BOOL hasChangedByOtherProcess = (m_nCurFileIndex != -1 && m_nCurFileIndex != m_rLogShareInfo.metaInfo.nFileIndex);

            BOOL bNeedWriteAfterRotate = !hasChangedByOtherProcess;

            //逻辑分类:
            //  1.正常写;
            //  2.写了以后关闭: 单个文件超过大小限制,写入以后, 其他进程再次打开对应文件, 才可以知道已经切换; 会造成超过阈值的 bug
            //  3.关闭以后再写: 当前文件已经足够大了 或 检测到其他进程已经切换了
            //do
            {
                //日期时间已经变化
                if (isLogDataTimeChanged)
                {
                    if (m_isDebug)
                    {
                        ATLTRACE(TEXT("dataTime changed: %d => %d, reset index from %d to 0\n"),
                            m_dwCurLogDate, dwCheckLogDate, m_rLogShareInfo.metaInfo.nFileIndex);
                    }
                    _Rotate(TRUE, FALSE);
                    m_rLogShareInfo.metaInfo.nFileIndex = 0;
                }
                if (hasChangedByOtherProcess)
                {
                    if (m_isDebug)
                    {
                        ATLTRACE(TEXT("has changed by other process:%d => %d\n"),
                            m_nCurFileIndex, m_rLogShareInfo.metaInfo.nFileIndex);
                    }
                    _Rotate(FALSE, FALSE);
                    //Close(FALSE);  //注意: 已经改变, 不能将缓冲 flush 到当前指向的文件中
                }
                if (curIsLarger) {
                    if (m_isDebug)
                    {
                        ATLTRACE(TEXT("cur file is larger, curLogSize=%d\n"), m_rLogShareInfo.metaInfo.curLogSize);
                    }
                    _Rotate(FALSE, FALSE);
                    //Close(FALSE);  //不能将缓冲 flush 到当前指向的文件中
                }
                //if (bNeedWriteAfterRotate)
                {
                    if (!m_pLogWritter->IsValid())
                    {
                        _CleanOldLogs();
                        bRet = _CreateLogFile();
                        ATLASSERT(bRet);
                    }

                    if (m_pLogWritter->IsValid())
                    {
                        LONG nWrittenLength = 0;
                        if (m_rLogShareInfo.metaInfo.nMsgLength > 0
                            && nLength + m_rLogShareInfo.metaInfo.nMsgLength >= LOG_MSG_BUFFER_SIZE)
                        {
                            //缓冲区满了, 输出
                            bRet = Flush();
                            ATLASSERT(bRet);
                        }

                        //如果缓冲区足够大,则拷贝到缓冲区, 否则直接输出
                        if (nLength < LOG_MSG_BUFFER_SIZE)
                        {
                            memcpy(m_rLogShareInfo.szMsgBuffer + m_rLogShareInfo.metaInfo.nMsgLength,
                                pszUtf8, nLength);
                            m_rLogShareInfo.metaInfo.nMsgLength += nLength;
                            nWrittenLength = nLength;
                            bRet = TRUE;
                        }
                        else {
                            bRet = m_pLogWritter->WriteLogBuffer(pszUtf8, nLength, &nWrittenLength);
                            ATLASSERT(bRet);
                            ATLASSERT(nLength == nWrittenLength);
                        }

                        if (bRet)
                        {
                            InterlockedExchangeAdd((LONG*)&m_rLogShareInfo.metaInfo.curLogSize, nWrittenLength);
                        }
                    }

                    if (isLarger)
                    {
                        if (m_isDebug)
                        {
                            ATLTRACE(TEXT("isLarger, index=%d/%d, curLogSize=%d, nLength=%d, m_rLogShareInfo.nMsgLength=%d\n"),
                                m_nCurFileIndex, m_rLogShareInfo.metaInfo.nFileIndex,
                                m_rLogShareInfo.metaInfo.curLogSize, nLength, m_rLogShareInfo.metaInfo.nMsgLength);
                        }
                        _Rotate(TRUE, TRUE);
                        //Close(TRUE);
                        //API_VERIFY(_CreateLogFile());
                    }
                }
                //} while (hasChangedByOtherProcess);
            }
            //fwrite(pszUtf8, 1, nLength, m_pLogFile);
        }

        DWORD dwCurTicket = GetTickCount();
        if (bFlush ||	//要求强制刷新(比如 tlError)
            (dwCurTicket < m_rLogShareInfo.metaInfo.lastFlushTime  //溢出
                || dwCurTicket >= m_rLogShareInfo.metaInfo.lastFlushTime + FTL_LOG_FLUSH_MAX_INTERVAL)  //超出阈值
            )
        {
            bRet = Flush();
            ATLASSERT(bRet);
        }

        return nLength;
    }

    BOOL CFLogger::Flush()
    {
        BOOL bRet = TRUE;
        ATL::CMutexLock locker(*m_pLockObj);
        ATLASSERT(m_pLogWritter);

        m_rLogShareInfo.metaInfo.lastFlushTime = GetTickCount();

        //ATLTRACE(TEXT("Enter Flush, nMsgLength=%d, isValid=%d\n"), m_rLogShareInfo.metaInfo.nMsgLength, m_pLogWritter->IsValid());
        if (m_rLogShareInfo.metaInfo.nMsgLength > 0 && m_pLogWritter->IsValid())
        {
            //FTLASSERT(FALSE); //如果禁用自定义缓冲区的话,就不会到这里, 因为 nMsgLength 始终是 0

            LONG nWriteBuffer = 0;
            bRet = m_pLogWritter->WriteLogBuffer(m_rLogShareInfo.szMsgBuffer,
                m_rLogShareInfo.metaInfo.nMsgLength, &nWriteBuffer);
            ATLASSERT(bRet);

            if (m_rLogShareInfo.metaInfo.nMsgLength != nWriteBuffer)
            {
                ATLTRACE(TEXT("!!! Flush want write %d, real write %d, err=%d\n"),
                    m_rLogShareInfo.metaInfo.nMsgLength, nWriteBuffer, GetLastError());
            }
            ATLASSERT(m_rLogShareInfo.metaInfo.nMsgLength == nWriteBuffer);

            //disk full?

            //TODO: 实际上只需要设置 nMsgLength 即可,但清零可以保证没有垃圾,方便查看问题
            ZeroMemory(m_rLogShareInfo.szMsgBuffer, LOG_MSG_BUFFER_SIZE);
            m_rLogShareInfo.metaInfo.nMsgLength = 0;
        }


        return bRet;
    }

    BOOL CFLogger::_CreateLogFile()
    {
        BOOL bRet = FALSE;
        HRESULT hr = E_FAIL;
        TCHAR szCheckLogFilePath[MAX_PATH] = { 0 };

        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);
        DWORD dwCheckLogDate = _GetCurLogDateInfo(st);

#define TRY_CREATE_LOG_COUNT  1
        BOOL bCreateNewLogFile = FALSE;
        LONG maxTryCount = 10;
        ULONG nCurFileSize = 0;

        LONG nTryFileIndex = m_rLogShareInfo.metaInfo.nFileIndex;
        do
        {
            if (nTryFileIndex >= 1)
            {
                hr = StringCchPrintf(szCheckLogFilePath, _countof(szCheckLogFilePath), TEXT("%s%s") TEXT("%d_%04u.log"),
                    m_rLogShareInfo.metaInfo.szLogPath, m_rLogShareInfo.metaInfo.szBaseName,
                    dwCheckLogDate, nTryFileIndex);
            } else {
                hr = StringCchPrintf(szCheckLogFilePath, _countof(szCheckLogFilePath), TEXT("%s%s") TEXT("%d.log"),
                    m_rLogShareInfo.metaInfo.szLogPath, m_rLogShareInfo.metaInfo.szBaseName,
                    dwCheckLogDate);
            }

            ATLASSERT(SUCCEEDED(hr));
            {
                nCurFileSize = 0;
                HANDLE hCheckFile = ::CreateFile(szCheckLogFilePath,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
                if (INVALID_HANDLE_VALUE != hCheckFile)
                {
                    LARGE_INTEGER nFileSize = { 0 };
                    if (::GetFileSizeEx(hCheckFile, &nFileSize)) {
                        nCurFileSize = (ULONG)nFileSize.QuadPart;
                    }
                    CloseHandle(hCheckFile);
                    hCheckFile = NULL;
                }
                if (m_isDebug)
                {
                    //注意: 如果多个进程操作的是同一个文件, A 进程写了(设置了内存中的长度,但没有 Flush), B进程无法准确获取到 真实长度. nCurFileSize 可能会小于实际值
                    ATLTRACE(TEXT("check %s, nCurFileSize=%d, m_rLogShareInfo { curLogSize=%d, msgLength=%d maxSize=%d }\n"),
                        szCheckLogFilePath, nCurFileSize, m_rLogShareInfo.metaInfo.curLogSize,
                        m_rLogShareInfo.metaInfo.nMsgLength, m_rLogShareInfo.metaInfo.maxLogSize);
                }

                if (m_rLogShareInfo.metaInfo.maxLogSize > 0 && nCurFileSize >= m_rLogShareInfo.metaInfo.maxLogSize)
                {
                    nTryFileIndex = InterlockedIncrement(&m_rLogShareInfo.metaInfo.nFileIndex);
                    continue;
                }

                bRet = m_pLogWritter->Open(szCheckLogFilePath);
                ATLASSERT(bRet);
                if (bRet)
                {
                    hr = StringCchCopy(m_szLogFilePath, _countof(m_szLogFilePath), szCheckLogFilePath);
                    ATLASSERT(SUCCEEDED(hr));

                    //FTLASSERT(m_rLogShareInfo.curLogSize == 0); // 如果 Close + _CreateLogFile 不是原子操作, 这个断言可能会不满足(其他进程写了)
                    // 出现这个断言, 可以考虑在 425 行的 API_VERIFY(_CreateLogFile());
                    if (0 == m_rLogShareInfo.metaInfo.curLogSize)
                    {
                        m_rLogShareInfo.metaInfo.curLogSize = nCurFileSize; //第一次启动
                    }

                    //m_nLocalCurLogSize = nCurFileSize;
                    //nTryFileIndex = m_rLogShareInfo.nFileIndex;
                    bCreateNewLogFile = TRUE;
                }
                else {
                    nTryFileIndex = InterlockedIncrement(&m_rLogShareInfo.metaInfo.nFileIndex);
                }
            }
        } while (!bCreateNewLogFile && maxTryCount-- > 0);

        if (m_pLogWritter->IsValid())
        {
			if (0 == m_pLogWritter->GetLength()) {
				//刚刚创建,则写入 UTF8 的 BOM 头

				UCHAR bufUTF8ROM[] = { 0xEF , 0xBB , 0xBF };
				LONG nLength = 0;
				bRet = m_pLogWritter->WriteLogBuffer(bufUTF8ROM, sizeof(bufUTF8ROM), &nLength);

				ATLASSERT(bRet);
				ATLASSERT(sizeof(bufUTF8ROM) == nLength);
				ATLASSERT(m_pLogWritter->GetLength() == sizeof(bufUTF8ROM));

				nCurFileSize += sizeof(bufUTF8ROM);
			}
            m_dwCurLogDate = dwCheckLogDate;
            if (m_isDebug)
            {
                ATLTRACE(TEXT("Create log file successful, index=%d, nCurFileSize=%d, m_dwCurLogDate=%d, %s\n"),
                    nTryFileIndex, nCurFileSize, m_dwCurLogDate, szCheckLogFilePath);
            }
            m_nCurFileIndex = nTryFileIndex;
        }
        else {
            ATLTRACE(TEXT("Create log file fail, err=%d, path=%s\n"), GetLastError(), szCheckLogFilePath);
        }

        return bRet;
    }

    __declspec(selectany) FTL::CFSharedVariableT<FTL_LOG_SHARE_INFO> g_GlobalLogShareInfo(
        LogShareInitializeProc, LogShareFinalizeProc, NULL);

    __declspec(selectany) ULONG FTL::CFLogger::s_LoggerInstanceCount = 0L;
    //__declspec(selectany) FTL_LOG_SHARE_INFO*  CFLogger::s_pFtlLogShareInfo = NULL;
    __declspec(selectany) CFLogger CFLogger::s_Logger(g_GlobalLogShareInfo.GetShareValue());


    CFLogger& CFLogger::GetInstance()
    {
        //static CFLogger loggerInstance(g_GlobalLogShareInfo.GetShareValue());
        //return loggerInstance;
        return s_Logger;
    }

    LPCWSTR CFLogger::GetBaseNameW(LPCWSTR pszFilePath)
    {
        LPCWSTR pszBaseName = pszFilePath;
        //return full path for debug, only file for release
#ifndef _DEBUG
        if (pszFilePath)
        {
            pszBaseName = PathFindFileNameW(pszFilePath);
            if (!pszBaseName)
            {
                pszBaseName = pszFilePath;
            }
        }
#endif 
        return pszBaseName;
    }

    LPCSTR CFLogger::GetBaseNameA(LPCSTR pszFilePath)
    {
        LPCSTR pszBaseName = pszFilePath;
        //return full path for debug, only file for release
#ifndef _DEBUG
        if (pszFilePath)
        {
            pszBaseName = PathFindFileNameA(pszFilePath);
            if (!pszBaseName)
            {
                pszBaseName = pszFilePath;
            }
        }
#endif 
        return pszBaseName;
    }

}
#endif //FTL_LOG_HPP