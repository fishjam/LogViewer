#pragma once

//#ifndef UNICODE
//#  error 目前只支持Unicode编译
//#endif
#include <memory>

#include "..\LogViewerDefine.h"
#include "LogViewerConfig.h"

enum LogItemContentType  //用于排序
{
    type_LineNum = 0,
    type_SeqNum,
    type_Machine,
    type_ProcessId,
    type_ThreadId,
    type_Time,
    type_ElapseTime,
    type_TraceLevel,
    type_ModuleName,
    type_FunName,
    type_FilePos,
    type_TraceInfo,

    type_SortCount
};

#define EXPORT_FIELD_LINE_NUM       0x0001
#define EXPORT_FIELD_SEQ_NUM        0x0002
#define EXPORT_FIELD_MACHINE        0x0004
#define EXPORT_FIELD_PID            0x0008
#define EXPORT_FIELD_TID            0x0010
#define EXPORT_FIELD_TIME           0x0020
#define EXPORT_FIELD_TRACE_LEVEL    0x0040
#define EXPORT_FIELD_MODULE_NAME    0x0080
#define EXPORT_FIELD_FUN_NAME       0x0100
#define EXPORT_FIELD_FILE_POS       0x0200
#define EXPORT_FIELD_TRACE_INFO     0x0400

#define EXPORT_FIELD_ALL            0xFFFF

//缺省导出(不需要 lineNum)
#define EXPORT_FIELD_DEFAULT        (EXPORT_FIELD_ALL & ~EXPORT_FIELD_LINE_NUM)

// 导出多个日志文件,并进行比较(比如想比较多次运行相同流程时的日志情况)
#define EXPORT_FIELD_COMPARE        \
    (EXPORT_FIELD_TRACE_LEVEL|EXPORT_FIELD_MODULE_NAME|EXPORT_FIELD_FUN_NAME|EXPORT_FIELD_FILE_POS|EXPORT_FIELD_TRACE_INFO)

enum FilterType {
    ftUnknown = -1,
    ftAll,
    ftAny,
    ftNone,
    ftRegex,
};
struct SortContent
{
    LogItemContentType  contentType;
    BOOL                bSortAscending;
};

struct LogOccurrenceInfo {
    CString     strLog;
    LONG        count;
};

struct LogItemFilter;

typedef std::list<CString>         SameNameFilePathList;
typedef std::shared_ptr<SameNameFilePathList>   SameNameFilePathListPtr;
typedef std::map<CString, SameNameFilePathListPtr, CAtlStringCompareI> FileName2FullPathMap;

//统计每一种日志出现的次数
typedef std::list<LogOccurrenceInfo> LogStatisticsInfos;

class CLogManager: public IFileFindCallback
{
    friend struct LogItemFilter;
public:
    //IFileFindCallback
    virtual FileFindResultHandle OnFindFile(LPCTSTR pszFilePath, const WIN32_FIND_DATA& findData, LPVOID pParam);
    virtual FileFindResultHandle OnError(LPCTSTR pszFilePath, DWORD dwError, LPVOID pParam);
public:
    CLogManager(void);
    ~CLogManager(void);
    //LONG GenerateLog(LPCTSTR pszFilePath, LONG logCount);
    CString GetFirstLogFilePath();
    BOOL SetLogFiles(const CStringArray &logFilePaths);
    BOOL ClearAllLogItems();
    BOOL ReloadLogItems();
    LONG GetLogFileCount() const;
    //LONG GetProcessCount() const;
    //LONG GetThreadCount() const;
    VOID GetSelectedCount(LONG& nSelectedProcess, LONG& nSelectedThread);

    //! bAll 为 TRUE 表示保存所有的日志
    //! bAll 为 FALSE 表示保存显示的日志(按照显示的顺序进行保存)
    BOOL ExportLogItems(LPCTSTR pszFilePath, DWORD dwFileds = EXPORT_FIELD_DEFAULT, BOOL bAll = FALSE);

    LONG GetDisplayLogItemCount() const;
    LONG GetTotalLogItemCount() const;

    const LogItemPointer GetDisplayLogItem(LONG index) const;
    BOOL TryReparseRealFileName(CString& strFileName);
    CString FormatDateTime(ULONGLONG time, DateTimeType dtType);

    BOOL DeleteItems(std::set<LONG> delItems);
    void setActiveItemIndex(LONG lineIndex, LONG displayIndex);
    LONG GetActiveLineIndex();
    CString getActiveItemTraceInfo();
   

    MachinePidTidContainer& GetAllMachinePidTidInfos(){
        return m_allMachinePidTidInfos;
    }
    //LONG GetThreadIds(ThreadIdContainer & threadIds) const;
    //LONG GetProcessIds(ProcessIdContainer & processIds) const;
    BOOL SortDisplayItem(LogItemContentType SortContentType, BOOL bSortAscending);
    SortContent GetFirstSortContent() const;

    BOOL IsItemMatchLineNumber(LONG lineNumber);
    //是否选中 -- 进行过滤
    BOOL IsItemIdChecked(const LogItemPointer& pItem);

    void OnlySelectSpecialItems(const MachinePIdTIdType& selectIdType, ONLY_SELECT_TYPE selType);

    BOOL SetTraceLevelDisplay(TraceLevel level, BOOL bDisplay);

    BOOL DoFilterLogItems();
    void SetCodepage(UINT codepage){ m_codePage = codepage; }
    BOOL SetFilterLineNumber(LONG nStartLineNumber, LONG nEndLineNumber);
    BOOL SetLogInfoFilterString(LPCTSTR pszFilterString, FilterType filterType);
    CLogViewerConfig    m_logConfig;

    BOOL NeedScanSourceFiles() {
        return m_filesMap.empty();
    }
    BOOL ScanSourceFiles(const CString& strFolderPath);
    SameNameFilePathListPtr FindFileFullPath(const CString& strFileName);
	VOID ClearUserFullPathCache();
	VOID SetFullPathForUserCache(const CString& strFileLineCache, const CString& strFullPathUserSelect);
	CString GetFullPathFromUserCache(const CString& strFileLineCache);


    //当 SeqNumber 不为 -1 时，检查是否有遗漏的(用于检查 ftl log 是否有丢失的情况)
    //  outMissingLineList 和 outRevertLineList 保存的是缺失 SeqNumber 时的行(从而可以 Goto)
    LONG CheckSeqNumber(LogIndexContainer* pOutMissingLineList, LogIndexContainer* pOutReverseLineList, LONG maxCount = 1);

    //统计出现次数最多的 nTop 个日志
    LONG GetTopOccurrenceLogs(LONG nTop, LogStatisticsInfos& staticsInfo, LogItemContentType itemType = type_FilePos);
protected:
    typedef std::vector<LogItemPointer>     LogItemArrayType;
    typedef LogItemArrayType::iterator      LogItemArrayIterator;
    typedef std::list<LogItemPointer>       LogItemListType;
	typedef std::map<CString, CString>		UserSelectFullPathMap;
    //typedef std::back_insert_iterator< LogItemArrayType > back_ins_itr;
    
    FileName2FullPathMap        m_filesMap;

    UINT                        m_codePage;
    mutable CFCriticalSection   m_CsLockObj;
    CStringArray                m_logFilePaths;
    std::list<LogItemPointer>   m_allInitLogItems;  //初始化时用来读取LogItem，防止使用vector时频繁分配内存
    LogItemArrayType            m_AllLogItems;
    LogItemArrayType            m_DisplayLogItems;

    MachinePidTidContainer      m_allMachinePidTidInfos;
	UserSelectFullPathMap		m_userSelectFullPathMap;
    //ThreadIdContainer           m_AllLogThreadIds;
    //std::map<THREAD_ID_TYPE, BOOL>      m_AllLogThreadIdsChecked;

    //AllProcessIdContainer               m_AllLogProcessIds;
    //std::map<PROCESS_ID_TYPE, BOOL>     m_AllLogProcessIdsChecked;

    typedef  std::map<MachinePIdTIdType, LONGLONG>  ThreadExecuteTimeContainer;  //机器名+进程ID+线程ID,上一次的时间值
    ThreadExecuteTimeContainer  m_threadExecuteTimes;

    CString                     m_strLogInfoFilterString;
    FilterType                  m_filterType;
    //BOOL                      m_isIncludeText;
    LONG                        m_fileCount;
    LONG                        m_nStartLineNumber;
    LONG                        m_nEndLineNumber;
    LONG                        m_nSelectedProcessCount;
    LONG                        m_nSelectedThreadCount;
    LONG                        m_activeLineIndex;
    LONG                        m_activeDisplayIndex;
    //增加了按照多个条件进行排序 -- 在有多个线程且对线程ID排序时，不这样做，各个日志可能乱序
    //是否应该在UI上增加标示？
    SortContent                 m_SortContents[type_SortCount];
    BOOL                        m_TraceLevelDisplay[tlEnd];         ///< 过滤Filter
protected:
    //typedef BOOL (__thiscall * ReadLogFileProc)(CLogManager*pThis, LPCTSTR pszFilePath);

    //BOOL CheckFTLogFiles(const CStringArray &logFilePaths);
    //BOOL ReadFTLogFile(LPCTSTR pszFilePath);
    //LogItemPointer ParseFTLLogItem(CString& strLogItem);

    //LogItemPointer ParseTraceLog(std::string& strOneLog);
    BOOL ReadTraceLogFile(LPCTSTR pszFilePath);

    LogItemPointer ParseRegularTraceLog(std::string& strOneLog, const std::tr1::regex& reg, const LogItemPointer& preLogItem);
    BOOL _CalcThreadElpaseTime(LogItemArrayType& logItems, LONG& outProcessCount, LONG& outThreadCount);
    int _ConvertItemInfo(const std::string& srcInfo, LPCTSTR& pszDest, UINT codePage);
    LPCTSTR _CopyItemInfo(LPCTSTR pszSource);
    void _AppendLogItem(LogItemPointer& pLogItem);
    LPCTSTR _ConvertNullString(LPCTSTR pszText) {
        return pszText ? pszText : TEXT("");     //避免在 %s 对 NULL 进行 format 时输出(null) 或 crash
    }
};
