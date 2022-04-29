#ifndef FTL_LOG_H
#define FTL_LOG_H
#pragma once

#include <atlstr.h>
#include <atlsync.h>

#include "ftlDefine.h"

#if IS_VS_2015_HIGHER
#else
//Mybox 中 VS2010, 必须包含这个
#  include <sys/stat.h>
#  include <share.h>
#endif //IS_VS_2015_HIGHER

/*************************************************************************************************************************
* C++ 的开源日志框架
*   glog(https://github.com/google/glog) <== 谷歌的
*   log4cplus(https://sourceforge.net/projects/log4cplus/)
*   log4cxx(https://logging.apache.org/log4cxx/latest_stable/download.html) <== Apache的 log4j 的C++版本
*     %X{Account} 可通过 MDC 功能记录账号等, log4cxx::MDC::put("Account", "fishjam");  , 在 ConversionPattern 中配置
*   log4cpp(http://sourceforge.net/projects/log4cpp/files) 
*   boost log() <== TODO: 如果编译成动态库的话需要的库太多
*   spdlog(https://github.com/gabime/spdlog) <== 高效多线程日志库, 有 header-only 版本, 也可静态链接.
*      
*************************************************************************************************************************/

/*************************************************************************************************************************
* 已知问题:
*   1.在日志输出时,可能先生成 Seq Number,然后发生线程切换，后面的日志会先写入日志文件. 是否能原子?但效率损失大;
*************************************************************************************************************************/

/*************************************************************************************************************************
* 最佳实践:
*   1.TODO:日志文件名的默认格式为 "<program name>.<hostname>.<user name>.log.<severity level>.<date>.<time>.<pid>"
*     能否更改? 似乎只能通过更改 LogFileObject::Write/LogMessage::Init/LogSink::ToString 等函数?
*     Linux下会通过 链接文件的方式指向最新的文件?
*   2.在 include 文件之前定义 GOOGLE_STRIP_LOG=等级 的宏,可以在编译时去除小于指定等级的日志输出代码.
*   3.默认是根据进程ID是否改变和文件大小是否超过预定值来确定是否需要新建日志文件.
*  
* GLog(https://github.com/google/glog.git)
*   谷歌的开源轻量级日志库,功能:
*     1.命令行的参数定制,控制是否 记录、停止、的条件,可自定义异常处理过程
*     2.日志等级:INFO(0) < WARNING(1) < ERROR(2) < FATAL(3)
*     2.线程安全日志记录方式
*   
*   编译、安装(编译时会将 src/glog 下的 logging.h.in 等文件作为模板，生成到 编译输出目录下 )
*     Win: cmake -G "Visual Studio 14 2015 Win64" -DBUILD_SHARED_LIBS=OFF .. ,  后打开并编译 glog.sln, 
*       -DBUILD_SHARED_LIBS=[ON]|OFF 控制是静态库还是动态库, 默认是动态(glogd.dll/glog.dll)
*     Linux: ./autogen.sh && ./configure && make , 编译后在 libs 目录下生成 libglog.so 和 libglog.a
*   
*   示例(输出的头文件在 build 目录下)
*     编译时需要定义以下宏:
*       GLOG_NO_ABBREVIATED_SEVERITIES         <== 避免与windows.h冲突(ERROR 等),Win上必须使用
*       GOOGLE_GLOG_DLL_DECL=                  <== 该宏表示静态链接
*     代码:
*       #include <glog/logging.h>
* 
*       LOG(INFO) << "Log Information";
*
*   配置参数: 可以在代码中设置或环境变量(GLOG_参数名),也可在启动程序时通过命令行参数配置(--参数名=参数值,需要 gflags 库支持)
*     详情参见 logging.h 中的 DECLARE_bool(xxx) 等方式声明的全局变量:
*     DECLARE_bool(alsologtostderr)[false]  -- 控制是否也输出到 stderr
*     DECLARE_bool(colorlogtostderr)[false] -- 控制shell中是否有颜色
*     DECLARE_bool(logtostderr)[false] -- 控制是否输出到 stderr(而不是输出到文件)
*     DECLARE_bool(stop_logging_if_full_disk)[false] -- 磁盘满了以后是否还继续记录日志
*     DECLARE_int32(max_log_size)[1800] -- 单个日志文件的最大大小(MB)，超过的话自动分隔
*     DECLARE_int32(stderrthreshold)[2|ERROR] -- 大于等于这个级别的日志才打印到标准错误
*     DECLARE_int32(minloglevel)[0|INFO] -- 最小日志输出的等级
*     DECLARE_int32(logbufsecs)[30] -- 使用缓存模式时最长(刷新?)秒数
*     DECLARE_string(log_dir) -- 指定日志输出目录,缺省是依次获取 GOOGLE_LOG_DIR、TEST_TMPDIR 等环境变量,没有的话,就是 ""
*
*   源码分析
*     接口层(namespace google)
*     实现层(namespace base)
*       [c]
*         virtual void Write(bool force_flush, time_t timestamp, const char* message, size_t message_len) = 0;
*       [LogSink]
*       +-[LogFileObject]
*       [LogDestination] <== 里面包含 
*       [LogCleaner] <== 删除指定日期以前的日志?
*************************************************************************************************************************/

namespace FTL
{

// #ifdef _WIN64
// #	define DEFAULT_FTL_LOG_LOCK_OBJECT_NAME	TEXT("Global\\FTL_LOG_LOCK_OBJECT_64")
// #else
// #	define DEFAULT_FTL_LOG_LOCK_OBJECT_NAME	TEXT("Global\\FTL_LOG_LOCK_OBJECT_32")
// #endif 

    enum LogDateTimeFormat {
        ltfTicket,   //GetTicketCount
        ltfDateTime,
        ltfTime
    };

    enum TraceLevel
    {
        //linux的syslog 将日志分成7各等级：info < notice < warning < error < crit(ical)< alert < panic
        tlDetail = 0,
        tlInfo,
        tlTrace,
        tlWarn,
        tlError,
        tlEnd,              //结束标记，用来计算枚举个数，不要直接使用
    };

#define FTL_LOG_FLUSH_MAX_INTERVAL			10000			//如果超过 10秒, 则保证刷新
#define FTL_LOG_SHARE_SIZE          (65536)   //64K
//#define FTL_LOG_SHARE_SIZE          (4096)   //4K

    //下面这种方式可以变相的禁用 buffer,相当于每次都调用底层写函数
    //#define FTL_LOG_SHARE_SIZE          (LOG_MSG_INFO_SIZE + 4)


    struct FTL_LOG_SHARE_META_INFO {
        ULONG volatile msgIndex;
        ULONG volatile lastFlushTime;
        ULONG volatile curLogSize;
        ULONG volatile maxLogSize;
        ULONG volatile maxTotalSize; 
        ULONG volatile nFileIndex;       //0 开始
        ULONG volatile nMsgLength;
        DWORD volatile logTarget;
        //以下部分不会经常变动
        TraceLevel     logThreshold;
        LogDateTimeFormat dateTimeFmt;
        TCHAR szLockMutextName[32];	//
        TCHAR szLogPath[MAX_PATH];  //全路径, 类似 "C:\\user\\applocal\\temp\\"  一类的(一定要有最后的斜线)
        TCHAR szBaseName[MAX_PATH]; //文件基础名, 一般为 "ftl_" 或 空 一类的
    };

//#define LOG_MSG_INFO_SIZE           (sizeof(ULONG)* 5 + (MAX_PATH * sizeof(TCHAR) * 2))
#define LOG_MSG_INFO_SIZE           (sizeof(FTL_LOG_SHARE_META_INFO))
#define LOG_MSG_BUFFER_SIZE         (FTL_LOG_SHARE_SIZE - LOG_MSG_INFO_SIZE)

    struct FTL_LOG_SHARE_INFO {
        FTL_LOG_SHARE_META_INFO metaInfo;
        BYTE szMsgBuffer[LOG_MSG_BUFFER_SIZE];
    };

    //TODO: 存接口基类, 接口方法上应该支持 file/socket/syslog 等
    class CFLogSink
    {
    public:
        FTLINLINE CFLogSink(BOOL isDebug);
        FTLINLINE virtual ~CFLogSink();
        virtual BOOL IsValid() = 0;
        virtual BOOL Open(LPCTSTR pszFilePath) = 0;
        virtual BOOL WriteLogBuffer(LPCVOID pBuffer, LONG nLen, LONG* pWritten) = 0;
		virtual LONG GetLength() = 0;
		virtual BOOL Flush() = 0;
        virtual BOOL Close() = 0;
    protected:
        BOOL m_bDebug;
    };

    class CFLogFileSink : public CFLogSink
    {
    public:
        FTLINLINE CFLogFileSink(BOOL isDebug);
        //TODO: 文件部分, 是否有特有的? 
    };

#if 0
	//使用 WinAPI 的实现: CreateFile + WriteFile 方式
	class CFLogFileSinkAPI : public CFLogFileSink
    {
    public:
        FTLINLINE CFLogFileSinkAPI(BOOL isDebug);
        FTLINLINE ~CFLogFileSinkAPI();

        FTLINLINE virtual BOOL IsValid();
        FTLINLINE virtual BOOL Open(LPCTSTR pszFilePath);
        FTLINLINE virtual BOOL WriteLogBuffer(LPCVOID pBuffer, LONG nLen, LONG* pWritten);
        FTLINLINE virtual BOOL Flush();
        FTLINLINE virtual BOOL Close();
    private:
        HANDLE m_hLogFile;
    };
#endif //if 0

    // 使用 open + write 的 low-level I/O
    class CFLogFileSinkLowIO : public CFLogFileSink
    {
    public:
        FTLINLINE CFLogFileSinkLowIO(BOOL isDebug);
        FTLINLINE virtual ~CFLogFileSinkLowIO();

        FTLINLINE virtual BOOL IsValid();
        FTLINLINE virtual BOOL Open(LPCTSTR pszFilePath);
        FTLINLINE virtual BOOL WriteLogBuffer(LPCVOID pBuffer, LONG nLen, LONG* pWritten);
		FTLINLINE virtual LONG GetLength();
        FTLINLINE virtual BOOL Flush();
        FTLINLINE virtual BOOL Close();
    private:
        int m_nLogFile;
    };

#if 0
    // 使用 fopen + fwrite 这种 stdio.h 中的缓冲IO
    class CFLogFileSinkStdIO : public CFLogFileSink
    {
    public:
        FTLINLINE CFLogFileSinkStdIO(BOOL isDebug);
        FTLINLINE  virtual ~CFLogFileSinkStdIO();

        FTLINLINE virtual BOOL IsValid();
        FTLINLINE virtual BOOL Open(LPCTSTR pszFilePath);
        FTLINLINE virtual BOOL WriteLogBuffer(LPCVOID pBuffer, LONG nLen, LONG* pWritten);
        FTLINLINE virtual BOOL Flush();
        FTLINLINE virtual BOOL Close();
    protected:
        FILE* m_pLogFile;
    };

    // 使用 open 这种 low-level 打开文件, 然后 fwrite 这种缓冲API写数据(glog 的做法)
    class CFLogFileSinkStdWithLowIO : public CFLogFileSinkStdIO
    {
    public:
        FTLINLINE virtual BOOL Open(LPCTSTR pszFilePath);
        //virtual ULONG GetCurSize();
    };
#endif //if 0

    #define FTL_LOG_TARGET_DEBUG_VIEW		0x1
    #define FTL_LOG_TARGET_LOCAL_FILE		0x2

    #define LOGGER_TRACE_OPTION_LOG_PATH					0x0001
    #define LOGGER_TRACE_OPTION_LOG_THRESHOLD				0x0002
    #define LOGGER_TRACE_OPTION_LOG_TARGET					0x0004
    #define LOGGER_TRACE_OPTION_MAX_LOG_FILE_SIZE			0x0008
    #define LOGGER_TRACE_OPTION_MAX_TOTAL_LOG_SIZE			0x0010

    #define LOGGER_TRACE_OPTION_ALL							0xFFFF

    struct LoggerTraceOptions
    {
        DWORD dwOptionsFlags;	//refer LOGGER_TRACE_OPTION_xxx

        LPCTSTR pszLogPath;
        TraceLevel logThreshold;
        DWORD logTarget;
        ULONG maxLogFileSize;
        ULONG maxTotalLogSize; 
    };

    class CFLogger
    {
        DISABLE_COPY_AND_ASSIGNMENT(CFLogger);
    public:
        FTLINLINE static CFLogger& GetInstance();
        FTLINLINE static LPCWSTR GetBaseNameW(LPCWSTR pszFilePath);
        FTLINLINE static LPCSTR GetBaseNameA(LPCSTR pszFilePath);
        FTLINLINE LPCWSTR GetCurrentTimeStrW(LPWSTR pszTimeBuf, INT nTimeBufCount);
        FTLINLINE LPCSTR GetCurrentTimeStrA(LPSTR pszTimeBuf, INT nTimeBufCount);
    public:

        FTLINLINE CFLogger(FTL_LOG_SHARE_INFO& rLogShareInfo);
        FTLINLINE virtual ~CFLogger();

        FTLINLINE VOID SetTraceOptions(const LoggerTraceOptions& traceOptions);

        FTLINLINE BOOL CheckLevel(TraceLevel level);
        FTLINLINE LPCTSTR GetLevelName(TraceLevel level);
        FTLINLINE ULONG GetNextTraceSequenceNumber();

        FTLINLINE VOID /*__cdecl*/ WriteLogInfoExW(TraceLevel level, const LPCWSTR lpszFormat, ...);
        FTLINLINE VOID /*__cdecl*/ WriteLogInfoExA(TraceLevel level, const LPCSTR lpszFormat, ...);

        FTLINLINE LONG WriteLogBuffer(LPCSTR pszUtf8, INT nLength, BOOL bFlush);

        FTLINLINE BOOL Flush();
        FTLINLINE VOID Close(BOOL bFlush);

    protected:
        FTL_LOG_SHARE_INFO& m_rLogShareInfo;
        BOOL  m_isDebug;
        //LONG    m_nLocalCurLogSize;
        FTLINLINE BOOL _CreateLogFile();
        FTLINLINE BOOL _Rotate(BOOL bFlush, BOOL bIncFileIndex);

        FTLINLINE DWORD _GetCurLogDateInfo(const SYSTEMTIME& st);  //example: 2021090910 , 支持小时
        FTLINLINE BOOL _CleanOldLogs();
    protected:
        ATL::CMutex*     m_pLockObj;
        //FTL::CFMutex	m_LockObj;
        CFLogSink* m_pLogWritter;
        //ULONG	m_maxLogSize;
        //int   m_nLogFile;
        ULONG           m_nCurInstanceIndex;
        volatile LONG   m_nCurFileIndex;
        volatile DWORD  m_dwCurLogDate;		//2021090910 ~ 
        TCHAR   m_szLogFilePath[MAX_PATH];

    private:
        static ULONG     s_LoggerInstanceCount;
        static FTL_LOG_SHARE_INFO*	s_pFtlLogShareInfo;
        static CFLogger	s_Logger;
    };

}
#endif //FTL_LOG_H

#   define FTL_TRACE_EX_W_FULL(_level, _file, _line, fmt, ...) \
    do{ \
        FTL::CFLogger& logger = FTL::CFLogger::GetInstance(); \
        if (logger.CheckLevel(_level)){ \
            WCHAR szTimeStr[32] = {0}; \
            ULONG nNextSeqNumber = logger.GetNextTraceSequenceNumber(); \
            logger.WriteLogInfoExW(_level, L"%s(%d):%u|%s|%04d|%04d|%s|%s|" fmt LOG_LINE_SEP_W, \
                _file, _line, nNextSeqNumber, \
                logger.GetCurrentTimeStrW(szTimeStr, _countof(szTimeStr)), \
                GetCurrentProcessId(), GetCurrentThreadId(), logger.GetLevelName(_level), __FUNCTIONW__, __VA_ARGS__); \
        } \
    }while(0);

#   define FTL_TRACE_EX_A_FULL(_level, _file, _line, fmt, ...) \
    do{ \
        FTL::CFLogger& logger = FTL::CFLogger::GetInstance(); \
        if (logger.CheckLevel(_level)){ \
            CHAR szTimeStr[32] = {0}; \
            ULONG nNextSeqNumber = logger.GetNextTraceSequenceNumber(); \
            logger.WriteLogInfoExA(_level, "%s(%d):%u|%s|%04d|%04d|%s|%s|" fmt LOG_LINE_SEP_A, \
                _file, _line, nNextSeqNumber, \
                logger.GetCurrentTimeStrA(szTimeStr, _countof(szTimeStr)), \
                GetCurrentProcessId(), GetCurrentThreadId(), logger.GetLevelName(_level), __FUNCTION__, __VA_ARGS__); \
        } \
    }while(0);


#   define FTLTRACEEXW(_level, fmt, ...) FTL_TRACE_EX_W_FULL(_level, FTL::CFLogger::GetBaseNameW(TEXT(__FILE__)), __LINE__, fmt, __VA_ARGS__)
#   define FTLTRACEEXA(_level, fmt, ...) FTL_TRACE_EX_A_FULL(_level, FTL::CFLogger::GetBaseNameA(__FILE__), __LINE__, fmt, __VA_ARGS__)

#   define FTLTRACEW(fmt, ...) FTLTRACEEXW(FTL::tlTrace, fmt, __VA_ARGS__)
#   define FTLTRACEA(fmt, ...) FTLTRACEEXA(FTL::tlTrace, fmt, __VA_ARGS__)



#ifdef UNICODE
#define FTLTRACE            FTLTRACEW
#define FTLTRACEEX          FTLTRACEEXW
#define FTLTRACEEX_FULL     FTL_TRACE_EX_W_FULL
#else
#define FTLTRACE            FTLTRACEA
#define FTLTRACEEX          FTLTRACEEXA
#define FTLTRACEEX_FULL     FTL_TRACE_EX_A_FULL

#endif // !UNICODE



#ifndef USE_EXPORT
#  include "ftlLog.hpp"
#endif 