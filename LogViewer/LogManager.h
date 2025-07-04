#pragma once

//#ifndef UNICODE
//#  error Ŀǰֻ֧��Unicode����
//#endif
#include <memory>

#include "..\LogViewerDefine.h"
#include "LogViewerConfig.h"
#include "json.h"

//�ļ�����
enum LogFileType
{
    ft_Text = 0,        //��ͨ�ı���ʽ, ��������ʽ����
    ft_Json             //Json ��ʽ
};

enum LogItemContentType  //��������
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
    type_TraceInfoLen,
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

//ȱʡ����(����Ҫ lineNum)
#define EXPORT_FIELD_DEFAULT        (EXPORT_FIELD_ALL & ~EXPORT_FIELD_LINE_NUM)

// ���������־�ļ�,�����бȽ�(������Ƚ϶��������ͬ����ʱ����־���)
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

typedef std::set<CString>         SameNameFilePathList;
typedef std::shared_ptr<SameNameFilePathList>   SameNameFilePathListPtr;
typedef std::map<CString, SameNameFilePathListPtr, CAtlStringCompareI> FileName2FullPathMap;

//ͳ��ÿһ����־���ֵĴ���
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

    //! bAll Ϊ TRUE ��ʾ�������е���־
    //! bAll Ϊ FALSE ��ʾ������ʾ����־(������ʾ��˳����б���)
    BOOL ExportLogItems(LPCTSTR pszFilePath, DWORD dwFileds = EXPORT_FIELD_DEFAULT, BOOL bAll = FALSE);

    LONG GetDisplayLogItemCount() const;
    LONG GetTotalLogItemCount() const;

    const LogItemPointer GetDisplayLogItem(LONG index) const;
    BOOL TryReparseRealFileName(CString& strFileName);
    CString FormatDateTime(LONGLONG time, DateTimeType dtType);
    CString FormatElapseTime(LONGLONG elapseTime, DateTimeType dtType);

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

    //�Ƿ���ϰ��ļ�������
    BOOL IsFilterFile(CString fileName);

    //�Ƿ�ѡ�� -- ���й���
    BOOL IsItemIdChecked(const LogItemPointer& pItem);

    void OnlySelectSpecialItems(const MachinePIdTIdTypeList& selectIdTypeList, ONLY_SELECT_TYPE selType);

    BOOL SetTraceLevelDisplay(TraceLevel level, BOOL bDisplay);

    BOOL DoFilterLogItems();
    void SetCodepage(UINT codepage){ m_codePage = codepage; }
    void SetDisplayTimeType(DateTimeType dateTimeType);
    BOOL SetFilterLineNumber(LONG nStartLineNumber, LONG nEndLineNumber);
    BOOL SetFilterFileNames(std::set<CString>& filterFiles);
    BOOL ClearFilterFileNames();
    BOOL SetLogInfoFilterString(LPCTSTR pszFilterString, FilterType filterType);
    CLogViewerConfig    m_logConfig;

    BOOL NeedScanSourceFiles() {
        return m_filesMap.empty();
    }
    BOOL ScanSourceFiles(const CStringArray& selectedPaths);
    SameNameFilePathListPtr FindFileFullPath(const CString& strFileName);
	VOID ClearUserFullPathCache();
	VOID SetFullPathForUserCache(const CString& strFileLineCache, const CString& strFullPathUserSelect);
	CString GetFullPathFromUserCache(const CString& strFileLineCache);
    BOOL TryOpenByTool(LPCTSTR pszFileName, int line);

    //�� SeqNumber ��Ϊ -1 ʱ������Ƿ�����©��(���ڼ�� ftl log �Ƿ��ж�ʧ�����)
    //  outMissingLineList �� outRevertLineList �������ȱʧ SeqNumber ʱ����(�Ӷ����� Goto)
    LONG CheckSeqNumber(LogIndexContainer* pOutMissingLineList, LogIndexContainer* pOutReverseLineList, LONG maxCount = 1);

    //ͳ�Ƴ��ִ������� nTop ����־
    LONG GetTopOccurrenceLogs(LONG nTop, UINT nTextLength, LogStatisticsInfos& staticsInfo, LogItemContentType itemType = type_FilePos);

    //��������
    BOOL ParseFileNameAndPos(CString strTraceInfo, CString& outFileName, int& outLine);
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
    std::list<LogItemPointer>   m_allInitLogItems;  //��ʼ��ʱ������ȡLogItem����ֹʹ��vectorʱƵ�������ڴ�
    LogItemArrayType            m_AllLogItems;
    LogItemArrayType            m_DisplayLogItems;

    MachinePidTidContainer      m_allMachinePidTidInfos;
	UserSelectFullPathMap		m_userSelectFullPathMap;
    //ThreadIdContainer           m_AllLogThreadIds;
    //std::map<THREAD_ID_TYPE, BOOL>      m_AllLogThreadIdsChecked;

    //AllProcessIdContainer               m_AllLogProcessIds;
    //std::map<PROCESS_ID_TYPE, BOOL>     m_AllLogProcessIdsChecked;

    typedef  std::map<MachinePIdTIdType, LONGLONG>  ThreadExecuteTimeContainer;  //������+����ID+�߳�ID,��һ�ε�ʱ��ֵ
    ThreadExecuteTimeContainer  m_threadExecuteTimes;

    CString                     m_strLogInfoFilterString;
    FilterType                  m_filterType;
    //BOOL                      m_isIncludeText;
    LONG                        m_fileCount;
    LONG                        m_nStartLineNumber;
    LONG                        m_nEndLineNumber;
    std::set<CString>           m_setFilterFiles;
    LONG                        m_nSelectedProcessCount;
    LONG                        m_nSelectedThreadCount;
    LONG                        m_activeLineIndex;
    LONG                        m_activeDisplayIndex;
    //�����˰��ն�������������� -- ���ж���߳��Ҷ��߳�ID����ʱ������������������־��������
    //�Ƿ�Ӧ����UI�����ӱ�ʾ��
    SortContent                 m_SortContents[type_SortCount];
    BOOL                        m_TraceLevelDisplay[tlEnd];         ///< ����Filter
protected:
    //typedef BOOL (__thiscall * ReadLogFileProc)(CLogManager*pThis, LPCTSTR pszFilePath);

    //BOOL CheckFTLogFiles(const CStringArray &logFilePaths);
    //BOOL ReadFTLogFile(LPCTSTR pszFilePath);
    //LogItemPointer ParseFTLLogItem(CString& strLogItem);

    //LogItemPointer ParseTraceLog(std::string& strOneLog);
    LONG ReadTraceLogFile(LPCTSTR pszFilePath, LONG startLineNum);
    LONG ReadJsonLogFile(LPCTSTR pszFilePath, LONG startLineNum);
    LONG ReadNdJsonLogFile(LPCTSTR pszFilePath, LONG startLineNum);

    LogItemPointer ParseRegularTraceLog(std::string& strOneLog, const std::tr1::regex& reg, const std::tr1::regex& reg2, const LogItemPointer& preLogItem);
    LogItemPointer ParseJsonLogItem(const Json::Value& valItem, const LogItemPointer& preLogItem);

    LONGLONG _parseTimeString(const std::string& strTime);
    VOID _ConvertTimeStampMsToSystemTime(LONGLONG milliSecond, SYSTEMTIME *pST);

    BOOL _CalcThreadElpaseTime(LogItemArrayType& logItems, LONG& outProcessCount, LONG& outThreadCount);
    int _ConvertItemInfo(const std::string& srcInfo, LPCTSTR& pszDest, UINT codePage);
    LPCTSTR _CopyItemInfo(LPCTSTR pszSource);
    void _AppendLogItem(LogItemPointer& pLogItem);
    LPCTSTR _ConvertNullString(LPCTSTR pszText) {
        return pszText ? pszText : TEXT("");     //������ %s �� NULL ���� format ʱ���(null) �� crash
    }
    BOOL _GetLogItemExportTextFormat(DWORD dwFileds, LogItemPointer& pLogItem, CString& strOutItem);
    BOOL _GetLogItemExportJsonFormat(DWORD dwFileds, const LogItemPointer& pLogItem, Json::Value& valRoot);
};
