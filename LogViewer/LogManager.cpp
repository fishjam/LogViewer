#include "StdAfx.h"
#include "LogManager.h"
#include "LogViewer.h"
#include <ftlFunctional.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

struct LogItemFilter : public std::unary_function<LogItemPointer, bool>
{
public:
    LogItemFilter(CLogManager & logManager)
        : m_LogManager(logManager)
    {
        if (m_LogManager.m_filterType == ftRegex) {
            // 表达式选项 - 忽略大小写
            try
            {
                std::regex_constants::syntax_option_type fl = std::regex_constants::icase;
                m_regularPattern.assign(m_LogManager.m_strLogInfoFilterString, fl);
            }
            catch (const std::tr1::regex_error& e)
            {
                FTL::CFConversion convText, convError;
                FTL::FormatMessageBox(NULL, TEXT("Regex Error"), MB_OK,
                    TEXT("Wrong Regex, reason is %s"), convError.UTF8_TO_TCHAR(e.what()));
            }

        }
    }
    bool operator()(const LogItemPointer& pItem) const
    {
        BOOL bChecked = TRUE;
        
        if (bChecked)
        {
            bChecked = m_LogManager.m_TraceLevelDisplay[pItem->level];
        }

        if (bChecked)
        {
            bChecked = m_LogManager.IsItemMatchLineNumber(pItem->lineNum);
        }
        
        if (bChecked)
        {
            bChecked = m_LogManager.IsItemIdChecked(pItem);// .m_AllLogProcessIdsChecked[pItem->processId];
        }
        if (bChecked)
        {
            if (!m_LogManager.m_strLogInfoFilterString.IsEmpty())
            {
                CAtlString strFilter = m_LogManager.m_strLogInfoFilterString;
                std::tr1::wcmatch regularResults;

                BOOL bFilterByString = TRUE;
                if (m_LogManager.m_filterType == ftAny)
                {
                    bFilterByString = FALSE;
                }

                //bChecked = FTL::CFStringUtil::IsMatchMask(pItem->pszTraceInfo, m_LogManager.m_strLogInfoFilterString, FALSE);
                std::list<CAtlString> tokens;
                FTL::Split(strFilter, TEXT(" "), false, tokens);
                for(std::list<CAtlString>::iterator iter = tokens.begin(); iter != tokens.end(); ++iter){
                    BOOL bMatch = (StrStrI(pItem->pszTraceInfo, *iter) != NULL);
                    switch (m_LogManager.m_filterType)
                    {
                    case ftAll:
                        bFilterByString &= bMatch;
                        break;
                    case ftAny:
                        bFilterByString |= bMatch;
                        break;
                    case ftRegex:
                        bFilterByString &= (BOOL)(std::tr1::regex_match(pItem->pszTraceInfo, regularResults, m_regularPattern));
                        break;
                    case ftNone:
                        bFilterByString &= (!bMatch);
                        break;
                    default:
                        FTLASSERT(FALSE);
                        break;
                    }
                }
                //再根据 m_isIncludeText 的值判断是需要包含文字还是排除文字
                //bChecked = !( bChecked ^ m_LogManager.m_isIncludeText);
                bChecked = bFilterByString;
            }
        }
        return (FALSE != bChecked);
    };
private:
    CLogManager& m_LogManager;
    std::tr1::wregex m_regularPattern;
};

struct LogItemCompare : public std::binary_function<LogItemPointer, LogItemPointer, bool>
{
public:
    LogItemCompare(SortContent* pSortContents)
        :m_pSortContents(pSortContents)
    {
    }
    bool operator()(const LogItemPointer& pItem1, const LogItemPointer& pItem2) const
    {
        LONGLONG result = 0;
        int nTryCount = type_SortCount;
        while (0 == result && nTryCount > 0)
        {
            switch (m_pSortContents[type_SortCount - nTryCount].contentType)
            {
            case type_LineNum:
                result = pItem1->lineNum - pItem2->lineNum;
                break;
            case type_SeqNum:
               result = pItem1->seqNum - pItem2->seqNum;
               break;
            case type_Machine:
                result = pItem1->machine.compare(pItem2->machine);
                break;
            case type_ProcessId:
                result = pItem1->processId.compare(pItem2->processId);
                break;
            case type_ThreadId:
                result = pItem1->threadId.compare(pItem2->threadId);
                break;
            case type_Time:
                {
                    result = pItem1->time - pItem2->time;// stTime.dwHighDateTime - pItem2->stTime.dwHighDateTime;
                    //if(0 == nResult)
                    //{
                    //    nResult = pItem1->stTime.dwLowDateTime - pItem2->stTime.dwLowDateTime;
                    //}
                }
                break;
            case type_ElapseTime:
                result = pItem1->elapseTime - pItem2->elapseTime;
                break;
            case type_ModuleName:
                if(pItem1->pszModuleName && pItem2->pszModuleName){
                    result = _tcscmp(pItem1->pszModuleName,pItem2->pszModuleName);
                }
                break;
            case type_FunName:
                if(pItem1->pszFunName && pItem2->pszFunName){
                    result = _tcscmp(pItem1->pszFunName,pItem2->pszFunName);
                }
                break;
            case type_TraceLevel:
                result = pItem1->level - pItem2->level;
                break;
            case type_FilePos:
                if(pItem1->pszSrcFileName && pItem2->pszSrcFileName){
                    result = _tcscmp(pItem1->pszSrcFileName,pItem2->pszSrcFileName);
                    if (result == 0)
                    {
                        result = pItem1->srcFileline - pItem2->srcFileline;
                    }
                }
                break;
            case type_TraceInfoLen:
                 result = pItem1->traceInfoLen - pItem2->traceInfoLen;
                 break;
            case type_TraceInfo:
                if(pItem1->pszTraceInfo && pItem2->pszTraceInfo){
                    result = _tcscmp(pItem1->pszTraceInfo,pItem2->pszTraceInfo);
                }
                break;
            default:
                FTLASSERT(FALSE);
                break;
            }
            nTryCount--;
        }
        if (m_pSortContents[type_SortCount - nTryCount - 1].bSortAscending)
        {
            return result < 0;
        }
        else
        {
            return result > 0;
        }
    }
private:
    const SortContent* m_pSortContents;
};



CLogManager::CLogManager(void)
{
    //刚开始时，全部设为按照 Sequence 排序
    for (int i = 0; i < type_SortCount; i++)
    {
        m_SortContents[i].contentType = type_LineNum;
        m_SortContents[i].bSortAscending = TRUE;
    }
    for (int i = 0; i < tlEnd; i++)
    {
        m_TraceLevelDisplay[i] = TRUE;
    }
    m_codePage = CP_UTF8;
    m_fileCount = 0;

    //Default is [0 ~ -1], it means all logs
    m_nStartLineNumber = 0;
    m_nEndLineNumber = -1;

    m_nSelectedProcessCount = -1;
    m_nSelectedThreadCount = -1;
    m_filterType = ftAll;
    m_activeLineIndex = INVALID_LINE_INDEX;
    m_activeDisplayIndex = INVALID_LINE_INDEX;
}

CLogManager::~CLogManager(void)
{
    ClearAllLogItems();
}

BOOL CLogManager::ClearAllLogItems()
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    m_nSelectedProcessCount = -1;
    m_nSelectedThreadCount = -1;
    m_activeLineIndex = INVALID_LINE_INDEX;
    m_activeDisplayIndex = INVALID_LINE_INDEX;

    m_threadExecuteTimes.clear();
    m_AllLogItems.clear();
    m_allInitLogItems.clear();
    m_DisplayLogItems.clear();

    m_allMachinePidTidInfos.clear();
    m_filesMap.clear();
	m_userSelectFullPathMap.clear();

    return TRUE;
}

BOOL CLogManager::_GetLogItemExportTextFormat(DWORD dwFileds, LogItemPointer& pLogItem, CString& strOutItem) {
    //保存成 FTL Log 的格式
    if (INVALID_SEQ_NUMBER != pLogItem->seqNum)
    {   //完整日志

        if (dwFileds & EXPORT_FIELD_MACHINE)
        {
            if (0 != pLogItem->machine.compare(DEFAULT_LOCAL_MACHINE))
            {
                strOutItem.AppendFormat(TEXT("%s|"), pLogItem->machine.c_str());
            }
        }
        if (dwFileds & EXPORT_FIELD_FILE_POS)
        {
            strOutItem.AppendFormat(TEXT("%s(%d):"), _ConvertNullString(pLogItem->pszSrcFileName), pLogItem->srcFileline);
        }
        if (dwFileds & EXPORT_FIELD_SEQ_NUM)
        {
            strOutItem.AppendFormat(TEXT("%d|"), pLogItem->seqNum);
        }
        if (dwFileds & EXPORT_FIELD_TIME)
        {
            strOutItem.AppendFormat(TEXT("%s|"), FormatDateTime(pLogItem->time, m_logConfig.m_dateTimeType));
        }
        if (dwFileds & EXPORT_FIELD_PID)
        {
            strOutItem.AppendFormat(TEXT("%s|"), pLogItem->processId.c_str());
        }
        if (dwFileds & EXPORT_FIELD_TID)
        {
            strOutItem.AppendFormat(TEXT("%s|"), pLogItem->threadId.c_str());
        }
        if (dwFileds & EXPORT_FIELD_TRACE_LEVEL)
        {
            strOutItem.AppendFormat(TEXT("%s|"), CFLogger::GetLevelName(pLogItem->level));
        }
        if (dwFileds & EXPORT_FIELD_MODULE_NAME)
        {
            strOutItem.AppendFormat(TEXT("%s|"), _ConvertNullString(pLogItem->pszModuleName));
        }
        if (dwFileds & EXPORT_FIELD_TRACE_INFO)
        {
            strOutItem.AppendFormat(TEXT("%s\n"), pLogItem->pszTraceInfo);
        }
    }
    else {
        //不完整日志,因此只输出 traceInfo
        strOutItem.Format(TEXT("%s\n"), pLogItem->pszTraceInfo);
    }
    return TRUE;
}

BOOL CLogManager::_GetLogItemExportJsonFormat(DWORD dwFileds, const LogItemPointer& pLogItem, Json::Value& valRoot) {
    //保存成 FTL Log 的格式
    //if (INVALID_SEQ_NUMBER != pLogItem->seqNum)
    {   //完整日志
        Json::Value valItem;
        FTL::CFConversion conv;
        if (dwFileds & EXPORT_FIELD_MACHINE)
        {
            if (0 != pLogItem->machine.compare(DEFAULT_LOCAL_MACHINE) && !m_logConfig.m_strItemMachine.IsEmpty()){
                valItem[m_logConfig.m_strItemMachine] = conv.TCHAR_TO_UTF8(pLogItem->machine.c_str());
            }
        }
        if (dwFileds & EXPORT_FIELD_FILE_POS)
        {
            if (!m_logConfig.m_strItemFile.IsEmpty()){
                valItem[m_logConfig.m_strItemFile] = conv.TCHAR_TO_UTF8(_ConvertNullString(pLogItem->pszSrcFileName));
            }
            if (!m_logConfig.m_strItemLine.IsEmpty()) {
                valItem[m_logConfig.m_strItemLine] = pLogItem->srcFileline;
            }
        }
        if (dwFileds & EXPORT_FIELD_SEQ_NUM)
        {
            if (!m_logConfig.m_strItemSeqNum.IsEmpty()) {
                valItem[m_logConfig.m_strItemSeqNum] = pLogItem->seqNum;
            }
        }
        if (dwFileds & EXPORT_FIELD_TIME)
        {
            if (!m_logConfig.m_strItemTime.IsEmpty()) {
                valItem[m_logConfig.m_strItemTime] = pLogItem->orgTimeStr;
            }
        }
        if (dwFileds & EXPORT_FIELD_PID)
        {
            if (!m_logConfig.m_strItemPId.IsEmpty()) {
                valItem[m_logConfig.m_strItemPId] =
                    conv.TCHAR_TO_UTF8(pLogItem->processId.c_str());
            }
        }
        if (dwFileds & EXPORT_FIELD_TID)
        {
            if (!m_logConfig.m_strItemTId.IsEmpty()) {
                valItem[m_logConfig.m_strItemTId] =
                    conv.TCHAR_TO_UTF8(pLogItem->threadId.c_str());
            }
        }
        if (dwFileds & EXPORT_FIELD_TRACE_LEVEL)
        {
            if (!m_logConfig.m_strItemLevel.IsEmpty()) {
                valItem[m_logConfig.m_strItemLevel] = pLogItem->orgLevelStr;
            }
        }
        if (dwFileds & EXPORT_FIELD_MODULE_NAME)
        {
            if (!m_logConfig.m_strItemModule.IsEmpty()) {
                valItem[m_logConfig.m_strItemModule] =
                    conv.TCHAR_TO_UTF8(_ConvertNullString(pLogItem->pszModuleName));
            }
        }

        if (dwFileds & EXPORT_FIELD_TRACE_INFO && !m_logConfig.m_strItemLog.IsEmpty())
        {
            valItem[m_logConfig.m_strItemLog] = conv.TCHAR_TO_UTF8(pLogItem->pszTraceInfo);
        }
        valRoot.append(valItem);
    }
    return TRUE;
}

BOOL CLogManager::ExportLogItems(LPCTSTR pszFilePath, DWORD dwFileds /* = EXPORT_FIELD_DEFAULT */, BOOL bAll /* = FALSE */)
{
    BOOL bRet = TRUE;
    CString strFilePath(pszFilePath);
    LogFileType fileType = ft_Text;
    CString strExt = strFilePath.Mid(strFilePath.ReverseFind(_T('.')));
    if (strExt.CompareNoCase(TEXT(".json")) == 0)
    {
        fileType = ft_Json;
    }

    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    LogItemArrayType* pSaveLogItems = bAll ? &m_AllLogItems : &m_DisplayLogItems;

    if (fileType == ft_Json){
        FTL::CFConversion conv;

        Json::Value valRoot;
        for (LogItemArrayIterator iter = pSaveLogItems->begin();
        iter != pSaveLogItems->end() && bRet;
            ++iter)
        {
            _GetLogItemExportJsonFormat(dwFileds, *iter, valRoot);
        }

        Json::StyledStreamWriter styledStreamWriter("\t");  //指定每一行前面的缩进符号，缺省是制表符(\t)
        std::ofstream osFile = std::ofstream(conv.TCHAR_TO_MBCS(pszFilePath));
        styledStreamWriter.write(osFile, valRoot);
    } else {
        FTL::CFUTF8File file;
        API_VERIFY(file.Create(pszFilePath, GENERIC_WRITE | GENERIC_READ, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, TRUNCATE_EXISTING));
        API_VERIFY(file.WriteFileHeader());

        for (LogItemArrayIterator iter = pSaveLogItems->begin();
        iter != pSaveLogItems->end() && bRet;
            ++iter)
        {
            CString strFormat;
            _GetLogItemExportTextFormat(dwFileds, *iter, strFormat);
            API_VERIFY(file.WriteString(strFormat));
        }
        file.Close();
    }

    return bRet;
}

BOOL CLogManager::ReloadLogItems()
{
    BOOL bRet = FALSE;
    CStringArray logFilePaths;
    logFilePaths.Copy(m_logFilePaths);
    SetLogFiles(logFilePaths);

    return bRet;
}

LONG CLogManager::GetLogFileCount() const
{
    return m_fileCount;
}

VOID CLogManager::GetSelectedCount(LONG& nSelectedProcess, LONG& nSelectedThread)
{
    if (m_allMachinePidTidInfos.empty())
    {
        nSelectedProcess = 0;
        nSelectedThread = 0;
        return;
    }

    if (m_nSelectedProcessCount != -1
        && m_nSelectedThreadCount != -1)
    {
        nSelectedProcess = m_nSelectedProcessCount;
        nSelectedThread = m_nSelectedThreadCount;
    } else {
        nSelectedProcess = 0;
        nSelectedThread = 0;
        {
            CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
            for (MachinePidTidContainer::const_iterator iterMachine = m_allMachinePidTidInfos.begin();
                iterMachine != m_allMachinePidTidInfos.end();
                ++iterMachine)
            {
                const PidTidContainer& pidTidContainer = iterMachine->second;
                for (PidTidContainer::const_iterator iterPid = pidTidContainer.begin();
                    iterPid != pidTidContainer.end();
                    ++iterPid)
                {
                    BOOL isProcessSelected = FALSE;
                    const TidContainer& tidContainer = iterPid->second;
                    for (TidContainer::const_iterator iterTid = tidContainer.begin();
                        iterTid != tidContainer.end();
                        ++iterTid){
                            if(iterTid->second.bChecked){
                                nSelectedThread++;
                                if (FALSE == isProcessSelected)
                                {
                                    isProcessSelected = TRUE;
                                    nSelectedProcess++;
                                }
                            }
                    }
                }
            }
        }
        
        m_nSelectedProcessCount = nSelectedProcess;
        m_nSelectedThreadCount = nSelectedThread;
    }
}

LONG CLogManager::GetDisplayLogItemCount() const
{
    LONG lCount = 0;
    {
        CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
        lCount = (LONG)m_DisplayLogItems.size();
    }
    return lCount;
}

LONG CLogManager::GetTotalLogItemCount() const 
{
    LONG lCount = 0;
    {
        CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
        lCount = (LONG)m_AllLogItems.size();
    }
    return lCount;
}

BOOL CLogManager::IsItemMatchLineNumber(LONG lineNumber){
    LONG checkEndLineNumber = m_nEndLineNumber < 0 ? LONG_MAX : m_nEndLineNumber;
    
    return FTL_INRANGE(m_nStartLineNumber, lineNumber, checkEndLineNumber);
}

BOOL CLogManager::IsItemIdChecked(const LogItemPointer& pItem){
    ID_INFOS& idInfos = m_allMachinePidTidInfos[pItem->machine][pItem->processId][pItem->threadId];
    FTLTRACE(TEXT("machine:%s, pid=%s, tid=%s, checked=%d"), 
        pItem->machine.c_str(), pItem->processId.c_str(), pItem->threadId.c_str(),
        idInfos.bChecked);
    return idInfos.bChecked;
}

void CLogManager::OnlySelectSpecialItems(const MachinePIdTIdTypeList& selectIdTypeList, ONLY_SELECT_TYPE selType) {
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);

    for (MachinePidTidContainer::iterator iterMachine = m_allMachinePidTidInfos.begin();
        iterMachine != m_allMachinePidTidInfos.end();
        ++iterMachine)
    {
        MACHINE_NAME_TYPE curMachine = iterMachine->first;
            PidTidContainer& pidTidContainer = iterMachine->second;

            for (PidTidContainer::iterator iterPid = pidTidContainer.begin();
            iterPid != pidTidContainer.end();
                ++iterPid)
            {
                PROCESS_ID_TYPE curProcess = iterPid->first;
                TidContainer& tidContainer = iterPid->second;

                for (TidContainer::iterator iterTid = tidContainer.begin();
                iterTid != tidContainer.end();
                    ++iterTid) {
                    THREAD_ID_TYPE curThread = iterTid->first;
                    ID_INFOS& rIdInfos = iterTid->second;
                    rIdInfos.bChecked = FALSE;

                    for (MachinePIdTIdTypeList::const_iterator iterIdType = selectIdTypeList.begin();
                    iterIdType != selectIdTypeList.end();
                        ++iterIdType) {
                        const MachinePIdTIdType selectIdType = *iterIdType;

                        BOOL isSameMachine = (curMachine == selectIdType.machine);
                        BOOL isSameProcess = (curProcess == selectIdType.pid);
                        BOOL isSameThread = (curThread == selectIdType.tid);

                        if (ostMachine == selType) {
                            rIdInfos.bChecked |= isSameMachine;
                        }
                        else if (ostProcessId == selType) {
                            rIdInfos.bChecked |= isSameMachine && isSameProcess;
                        }
                        else if (ostThreadId == selType) {
                            rIdInfos.bChecked |= isSameMachine && isSameProcess && isSameThread;
                        }
                    }
                }
            }
    }

    DoFilterLogItems();
}

BOOL CLogManager::SetTraceLevelDisplay(TraceLevel level, BOOL bDisplay)
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    if (m_TraceLevelDisplay[level] != bDisplay)
    {
        m_TraceLevelDisplay[level] = bDisplay;
        DoFilterLogItems();
    }
    return TRUE;
}

void CLogManager::SetDisplayTimeType(DateTimeType dateTimeType)
{
    m_logConfig.m_dateTimeType = dateTimeType;
}

BOOL CLogManager::SetFilterLineNumber(LONG nStartLineNumber, LONG nEndLineNumber){
    m_nStartLineNumber = nStartLineNumber;
    m_nEndLineNumber = nEndLineNumber;
    return TRUE;
}

BOOL CLogManager::SetLogInfoFilterString(LPCTSTR pszFilterString, FilterType filterType)
{
    BOOL bRet = FALSE;
    m_filterType = filterType;
    if (pszFilterString)
    {
        m_strLogInfoFilterString = pszFilterString;
    }
    else
    {
        m_strLogInfoFilterString = TEXT("");
    }
    API_VERIFY(DoFilterLogItems());
    return bRet;
}

BOOL CLogManager::DoFilterLogItems()
{
    LogItemListType displayItems;
    FTL::copy_if(m_AllLogItems.begin(),m_AllLogItems.end(),
        std::back_inserter(displayItems),LogItemFilter(*this));
    {
        CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
        m_activeDisplayIndex = -1;
        m_DisplayLogItems.clear();
        m_DisplayLogItems.reserve(displayItems.size());
        m_DisplayLogItems.assign(displayItems.begin(), displayItems.end());
        SortDisplayItem(m_SortContents[0].contentType,m_SortContents[0].bSortAscending);
    }

    _CalcThreadElpaseTime(m_DisplayLogItems, m_nSelectedProcessCount, m_nSelectedThreadCount);
    //m_nSelectedProcessCount = -1;
    //m_nSelectedThreadCount = -1;
    return TRUE;
}

const LogItemPointer CLogManager::GetDisplayLogItem(LONG index) const
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    LogItemPointer pLogItem;
    if (index >= 0 && index < (LONG)m_DisplayLogItems.size())
    {
        pLogItem = m_DisplayLogItems.at(index);
    }
     
    return pLogItem;
}

BOOL CLogManager::TryReparseRealFileName(CString& strFileName)
{
    if (!m_logConfig.m_strSrcRegular.IsEmpty() && m_logConfig.m_nItemSrcFileEx != INVLIAD_ITEM_MAP)
    {
        try
        {
            CFConversion conv;
            const std::tr1::wregex regularSrcPattern(m_logConfig.m_strSrcRegular);
            std::tr1::wcmatch regularResults;
            bool result = std::tr1::regex_match(strFileName.GetString(), regularResults, regularSrcPattern);
            if (result && (INT)regularResults.size() >= m_logConfig.m_nItemSrcFileEx)
            {
                strFileName = std::wstring(regularResults[m_logConfig.m_nItemSrcFileEx]).c_str();
                return TRUE;
            }
        }
        catch (const std::tr1::regex_error& e)
        {
            FTL::CFConversion convText, convError;
            FTL::FormatMessageBox(NULL, TEXT("Regex for FileSrc Error"), MB_OK,
                TEXT("Text=%s\nWrong Regex, reason is %s"),
                strFileName, convError.UTF8_TO_TCHAR(e.what()));
            return FALSE;
        }
    }

    return FALSE;
}


CString CLogManager::FormatDateTime(LONGLONG microSeconds, DateTimeType dtType)
{
    BOOL bRet = FALSE;
    CString strResult;

    //其值是 微秒, 为了计算成 FILETIME 的值(100ns)
    LONGLONG time = microSeconds * 10;
    //把微秒值计算出来
    int microSec = microSeconds % MICROSECOND_PER_SECOND;
    time = time / TIME_RESULT_TO_MILLISECOND * TIME_RESULT_TO_MILLISECOND;  //取整,将毫秒数据清除

    SYSTEMTIME st = { 0 };
    if (dtType <= dttDateTimeLast)
    {
        //带日期的时间
        //FILETIME localFileTime = {0};
        FILETIME tm = { 0 };
        tm.dwHighDateTime = HILONG(time);//(pLogItem->time & 0xFFFFFFFF00000000) >> 32;
        tm.dwLowDateTime = LOLONG(time);// ( pLogItem->time & 0xFFFFFFFF);
        API_VERIFY(FileTimeToSystemTime(&tm, &st));

        //解析显示方式
        CString strFormat = TEXT("");
        int nSecDisplayValue = 0;   
        switch (dtType)
        {
        case dttDateTimeMilliSecond:
            strFormat = TEXT("%4d-%02d-%02d %02d:%02d:%02d.%03d");
            nSecDisplayValue = microSec / 1000;
            break;
        case dttDateTimeMicrosecond: //默认显示成微秒
        default:
            strFormat = TEXT("%4d-%02d-%02d %02d:%02d:%02d.%06d");
            nSecDisplayValue = microSec;
            break;
        }
        strResult.Format(strFormat,
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond, nSecDisplayValue);
    }
    else {
        LONGLONG disTime = time % MIN_TIME_WITH_DAY_INFO;
        LONGLONG tmpTime = disTime / TIME_RESULT_TO_MILLISECOND;
        st.wSecond = tmpTime % 60;
        tmpTime /= 60;
        st.wMinute = tmpTime % 60;
        st.wHour = (WORD)tmpTime / 60;

        CString strFormat = TEXT("");
        int nSecDisplayValue = 0;
        switch (dtType)
        {
        case dttTimeMilliSecond:
            strFormat = TEXT("%02d:%02d:%02d.%03d");
            nSecDisplayValue = microSec / 1000;
            break;
        case dttTimeMicrosecond:
        default:
            strFormat = TEXT("%02d:%02d:%02d.%06d");
            nSecDisplayValue = microSec;
            break;
        }
        strResult.Format(strFormat,
            st.wHour, st.wMinute, st.wSecond, nSecDisplayValue);
    }
    return strResult;
}

CString CLogManager::FormatElapseTime(LONGLONG elapseTime, DateTimeType dtType)
{
    CString strResult;
    switch (dtType)
    {
    case dttDateTimeMilliSecond:
    case dttTimeMilliSecond:
        strResult.Format(TEXT("%.3f s"), (double)elapseTime / MICROSECOND_PER_SECOND);
        break;
    case dttDateTimeMicrosecond:
    case dttTimeMicrosecond:
    default:
        strResult.Format(TEXT("%.6f s"), (double)elapseTime / MICROSECOND_PER_SECOND);
        break;
    }
    return strResult;
}


void CLogManager::setActiveItemIndex(LONG lineIndex, LONG displayIndex){
    m_activeLineIndex = lineIndex;
    m_activeDisplayIndex = displayIndex;
}

BOOL CLogManager::DeleteItems(std::set<LONG> delItems) {
    //std::sort(items.begin(), items.end());
    BOOL bDeleted = FALSE;
    for (LogItemArrayType::iterator iter = m_AllLogItems.begin();
        iter != m_AllLogItems.end(); )
    {
        if (delItems.find((*iter)->lineNum) != delItems.end())
        {
            iter = m_AllLogItems.erase(iter);
            bDeleted = TRUE;
        }
        else 
        {
            ++iter;
        }
    }

    if (bDeleted)
    {
        DoFilterLogItems();
    }
    return bDeleted;
}

LONG CLogManager::GetActiveLineIndex() {
    return m_activeLineIndex;
}

CString CLogManager::getActiveItemTraceInfo(){
    if (m_activeDisplayIndex >= 0 && m_activeDisplayIndex < (LONG)m_DisplayLogItems.size()){
        return m_DisplayLogItems.at(m_activeDisplayIndex)->pszTraceInfo;
    }
    return TEXT("");
}

BOOL CLogManager::SortDisplayItem(LogItemContentType SortContentType, BOOL bSortAscending)
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    if (m_SortContents[0].contentType != SortContentType)
    {
        for (int i = type_SortCount - 1; i> 0 ; i--)
        {
            m_SortContents[i].contentType = m_SortContents[i-1].contentType;
            m_SortContents[i].bSortAscending = m_SortContents[i-1].bSortAscending;
        }
        m_SortContents[0].contentType = SortContentType;
    }
    m_SortContents[0].bSortAscending = bSortAscending;
    sort(m_DisplayLogItems.begin(),m_DisplayLogItems.end(),LogItemCompare(&m_SortContents[0]));
    return TRUE;
}

SortContent CLogManager::GetFirstSortContent() const
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    return m_SortContents[0];
}

// LONG CLogManager::GenerateLog(LPCTSTR pszFilePath, LONG logCount)
// {
//     //FTL::CFAnsiFile file(TextFileEncoding::tfeUnknown);
//     LONG nGenerateCount = 0;
//     return nGenerateCount;
// }

CString CLogManager::GetFirstLogFilePath() 
{
    CString strFirstLogFilePath;
    if (!m_logFilePaths.IsEmpty())
    {
        strFirstLogFilePath = m_logFilePaths[0];
    }
    return strFirstLogFilePath;
}

BOOL CLogManager::SetLogFiles(const CStringArray &logFilePaths)
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    BOOL bRet = FALSE;

    //加载配置文件
    //API_VERIFY(m_logConfig.LoadConfig(TEXT("LogViewer.ini")));
    CWaitCursor waitCursor;

    bRet = logFilePaths.GetSize() > 0;
    if (bRet)
    {
        m_fileCount = (LONG)logFilePaths.GetCount();

        ClearAllLogItems(); //先清除以前的

        INT_PTR logFileCount = logFilePaths.GetCount();
        LONG nStartLineIndex = -1;
        LONG nNewLineIndex = 0;
        for (INT_PTR index = 0; (nStartLineIndex != nNewLineIndex) && index < logFileCount; index++)
        {
            CString strExt = logFilePaths[index].Mid(logFilePaths[index].ReverseFind(_T('.')));
            nStartLineIndex = nNewLineIndex;
            if (0 == strExt.CompareNoCase(TEXT(".json")))
            {
                nNewLineIndex = ReadJsonLogFile(logFilePaths[index], nStartLineIndex);
            }
            else
            {
                nNewLineIndex = ReadTraceLogFile(logFilePaths[index], nStartLineIndex);
            }
            theApp.AddToRecentFileList(logFilePaths[index]);
        }
    }
    if (bRet)
    {
        m_AllLogItems.reserve(m_allInitLogItems.size());
        m_AllLogItems.assign(m_allInitLogItems.begin(),m_allInitLogItems.end());
        m_DisplayLogItems.reserve(m_AllLogItems.size());
        m_DisplayLogItems.assign(m_AllLogItems.begin(),m_AllLogItems.end());
        //copy(m_AllLogItems.begin(),m_AllLogItems.end(),back_ins_itr(m_DisplayLogItems.begin()));

        //按照现在的过滤条件过滤一次
        DoFilterLogItems();
        m_logFilePaths.Copy(logFilePaths);

        AfxGetMainWnd()->SetWindowText(logFilePaths.GetAt(0));
    }
    return bRet;
}

int CLogManager::_ConvertItemInfo(const std::string& srcInfo, LPCTSTR& pszDest, UINT codePage)
{
    int srcLength = (int)srcInfo.length();
    pszDest = new WCHAR[srcLength + 1];
    ZeroMemory((void*)pszDest, sizeof(WCHAR) * (srcLength +1));
    MultiByteToWideChar(codePage, 0, srcInfo.c_str(), -1, (LPWSTR)pszDest, srcLength);
    return srcLength;
}

LPCTSTR CLogManager::_CopyItemInfo(LPCTSTR pszSource){
    LPTSTR pszDest = NULL;
    if (pszSource != NULL){
        size_t srcLength = _tcslen(pszSource);
        pszDest = new WCHAR[srcLength + 1];
        _tcsncpy(pszDest, pszSource, srcLength);
        pszDest[srcLength] = TEXT('\0');
    }
    return pszDest;
}

FileFindResultHandle CLogManager::OnFindFile(LPCTSTR pszFilePath, const WIN32_FIND_DATA& findData, LPVOID pParam)
{
    UNREFERENCED_PARAMETER(pParam);

    BOOL isDirectory = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
    if (!isDirectory)
    {
        CString strFileName = findData.cFileName;
        FileName2FullPathMap::iterator iter = m_filesMap.find(strFileName);
        SameNameFilePathListPtr spSameNameFilePathList;
        if (iter == m_filesMap.end())
        {
            //can not find
            spSameNameFilePathList = std::make_shared<SameNameFilePathList>();
            m_filesMap.insert(FileName2FullPathMap::value_type(strFileName, spSameNameFilePathList));
        }
        else {
            spSameNameFilePathList = iter->second;
        }

        spSameNameFilePathList->push_back(pszFilePath);
    }

    return rhContinue;
}

FileFindResultHandle CLogManager::OnError(LPCTSTR pszFilePath, DWORD dwError, LPVOID pParam)
{
    FTL::CFAPIErrorInfo errInfo(dwError);
    FTLTRACEEX(FTL::tlError, TEXT("err:%d(%s), path=%s"), dwError, errInfo.GetConvertedInfo(), pszFilePath);
    return rhContinue;
}

BOOL CLogManager::ScanSourceFiles(const CStringArray& selectedPaths)
{
    BOOL bRet = TRUE;
    FTL::CFFileFinder fileFinder;
    fileFinder.SetCallback(this, this);

    INT_PTR nCount = selectedPaths.GetCount();
    for (INT_PTR index = 0; bRet && index < nCount; index++)
    {
        API_VERIFY(fileFinder.Find(selectedPaths[index], m_logConfig.m_strSourceFileExts, TRUE));
    }
    return bRet;
}

SameNameFilePathListPtr CLogManager::FindFileFullPath(const CString& strFileName) {
    FileName2FullPathMap::iterator iter = m_filesMap.find(strFileName);
    if (iter != m_filesMap.end())  //find
    {
        return iter->second;
    }
    return nullptr;
}

VOID CLogManager::ClearUserFullPathCache()
{
	m_userSelectFullPathMap.clear();
}

VOID CLogManager::SetFullPathForUserCache(const CString& strFileLineCache, const CString& strFullPathUserSelect)
{
	m_userSelectFullPathMap[strFileLineCache] = strFullPathUserSelect;
}

CString CLogManager::GetFullPathFromUserCache(const CString& strFileLineCache)
{
	UserSelectFullPathMap::iterator iter = m_userSelectFullPathMap.find(strFileLineCache);
	if (iter!= m_userSelectFullPathMap.end())
	{
		return iter->second;
	}
	return TEXT("");
}

BOOL CLogManager::TryOpenByTool(LPCTSTR pszFileName, int line)
{
    BOOL bRet = FALSE;
    if( !m_logConfig.m_strOpenCommand.IsEmpty()) {
        //处理路径有空格的情况
        TCHAR szPath[MAX_PATH] = { 0 };
        StringCchCopy(szPath, _countof(szPath), pszFileName);
        PathQuoteSpaces(szPath);

        CString strCommand = m_logConfig.m_strOpenCommand;
        CString strLine;
        strLine.Format(TEXT("%d"), line);
        strCommand.Replace(TEXT("$ITEM_LINE"), strLine);
        strCommand.Replace(TEXT("$ITEM_FILE"), szPath);

        FTLTRACE(TEXT("try open with command: %s"), strCommand);
        //  goland64.exe --line 42 C:\MyProject\main.go
        STARTUPINFO startupInfo = {0};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo = {0};
        API_VERIFY(::CreateProcess(NULL, (LPTSTR)(LPCTSTR)strCommand, NULL, NULL, FALSE, 0, NULL, NULL, 
            &startupInfo, &processInfo));
        SAFE_CLOSE_HANDLE(processInfo.hProcess, NULL);
        SAFE_CLOSE_HANDLE(processInfo.hThread, NULL);
    }
    return bRet;
}

LONG CLogManager::CheckSeqNumber(LogIndexContainer* pOutMissingLineList, LogIndexContainer* pOutReverseLineList, LONG maxCount)
{
    LONG preSeqNumber = 0;
    LONG findCount = 0;
    LONG index = 0;
    if (pOutMissingLineList)
    {
        pOutMissingLineList->clear();
    }
    if (pOutReverseLineList)
    {
        pOutReverseLineList->clear();
    }

    //TODO: 到底应该比较 m_AllLogItems(只能处理单进程) 还是 m_DisplayLogItems(可以过滤出单进程)
    //m_DisplayLogItems 是按照顺序排列的，因此可以直接比较差值是否为 1
    for (LogItemArrayIterator iter = m_DisplayLogItems.begin(); iter != m_DisplayLogItems.end(); ++iter) 
    {
        LogItemPointer pLogItem = *iter;
        if (pLogItem->seqNum != INVALID_SEQ_NUMBER)
        {
            // 有缺失
            if (pLogItem->seqNum - preSeqNumber > 1)
            {
                BOOL bFoundMissing = FALSE;  // 表示是否能在后面找到(反序了)

#define MAX_CHECK_MISSING_DIFF_COUNT  10
                //前后寻找最多 X 条记录,看是否在后面(本质还是反序), 实测在 dokan 盘应用中,最多超过 100+
                for (int c = -MAX_CHECK_MISSING_DIFF_COUNT; c < MAX_CHECK_MISSING_DIFF_COUNT; c++)
                {
                    int checkIndex = index + c;
                    if (checkIndex >= 0 
                        && checkIndex < m_DisplayLogItems.size() - 1 
                        && (m_DisplayLogItems[checkIndex + 1]->seqNum == preSeqNumber + 1))
                    {
                        FTLTRACE(TEXT("check seq: seq[%d]=%d, curSeq=%d"), index + 1,
                            m_DisplayLogItems[index + 1]->seqNum, pLogItem->seqNum);
                        bFoundMissing = TRUE;
                        break;
                    }
                }
                if (!bFoundMissing)
                {   //在当前记录开始的后面 X 条记录中没有找到缺失的记录, 可以确认丢失.
                    if (pOutMissingLineList)
                    {
                        pOutMissingLineList->push_back(pLogItem->lineNum);
                        findCount++;
                    }
                }
            }
            
            if (pLogItem->seqNum < preSeqNumber) 
            {
                //反序
                if (pOutReverseLineList)
                {
                    pOutReverseLineList->push_back(pLogItem->lineNum);
                    findCount++;
                }
            }
           
            preSeqNumber = pLogItem->seqNum;
            if (maxCount > 0 && findCount >= maxCount)
            {
                break;
            }

        }
        index++;
    }
   
    return findCount;
}

LONG CLogManager::GetTopOccurrenceLogs(LONG nTop, LogStatisticsInfos& staticsInfo, LogItemContentType itemType)
{
    typedef std::map<CString, LONG>     LogStatisticsMap;
    LogStatisticsMap tempLogMap;

    //先统计每一种的次数
    for (LogItemArrayIterator iter = m_DisplayLogItems.begin(); iter != m_DisplayLogItems.end(); ++iter)
    {
        LogItemPointer pLogItem = *iter;
        switch (itemType)
        {
        case type_FilePos:
            {
                if (pLogItem->srcFileline > 0)
                {
                    CString strFileLine;
                    strFileLine.Format(TEXT("%s(%d)"), pLogItem->pszSrcFileName, pLogItem->srcFileline);
                    tempLogMap[strFileLine]++;
                }
                break;
            }
        case type_TraceInfo:
        default:
            tempLogMap[pLogItem->pszTraceInfo]++;
            break;
        }
    }

    //按次数分组, 二叉树,最后的必然最大
    typedef std::multimap<LONG, CString> COUNT_MAP;
    COUNT_MAP countMap;
    
    for (LogStatisticsMap::iterator iter = tempLogMap.begin(); iter != tempLogMap.end(); ++iter)
    {
        countMap.insert(std::pair<LONG, CString>(iter->second, iter->first));
    }
    
    LONG topIndex = 0;
    for (COUNT_MAP::reverse_iterator iter = countMap.rbegin(); 
        topIndex <= nTop && iter != countMap.rend();
            ++iter)
    {
        LogOccurrenceInfo occureInfo;
        occureInfo.strLog = iter->second;
        occureInfo.count = iter->first;
        staticsInfo.push_back(occureInfo);
        ++topIndex;
    }

    return topIndex;
}

VOID CLogManager::_ConvertTimeStampMsToSystemTime(LONGLONG milliSecond, SYSTEMTIME *pST) {
    time_t nSec = milliSecond / 1000;
    tm *ltm = localtime(&nSec);
    ZeroMemory(pST, sizeof(SYSTEMTIME));
    pST->wYear = 1900 + ltm->tm_year;
    pST->wMonth = 1 + ltm->tm_mon;
    pST->wDay = ltm->tm_mday;
    pST->wHour = ltm->tm_hour;
    pST->wMinute = ltm->tm_min;
    pST->wSecond = ltm->tm_sec;
    pST->wMilliseconds = milliSecond % 1000;
}

LONGLONG CLogManager::_parseTimeString(const std::string& strTime)
{
    BOOL bRet = FALSE;
    if (!m_logConfig.m_strTimeFormat.IsEmpty())
    {
        //TODO: 日期时间解析的代码很挫(纯粹的硬编码,使用比较好的库?)
        // chrono::parse ? 需要 C++20 ?
        // boost::date_time::parse_delimited_time ? 需要引入boost
        //FTLTRACEA("strTime=%s", strTime.c_str());

        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);  //获取年月日等信息,后面才能通过 SystemTimeToFileTime 转换
        st.wMilliseconds = 0;  //其精度只能到毫秒, 因此设置为 0, 通过 microSecond(微秒)保存及转换
        LONGLONG fileTimeValue = 0;

        DateTimeType newDateTimeType = dttDateTimeNone;

        int microSecond = 0; //微秒
        int milliSecond = 0; //毫秒, ignore, zooHour = 0, zooMinute = 0;

        if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("TIMESTAMP_SEC"))) {
            long nSec = std::strtoll(strTime.c_str(), NULL, 10);
            newDateTimeType = dttDateTimeMilliSecond;
            fileTimeValue = nSec * 1000;
            _ConvertTimeStampMsToSystemTime(fileTimeValue, &st);
        } else  if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("TIMESTAMP_MILLSEC"))) {
            newDateTimeType = dttDateTimeMilliSecond;
            LONGLONG nTotalMillSec = std::strtoll(strTime.c_str(), NULL, 10);
            _ConvertTimeStampMsToSystemTime(nTotalMillSec, &st);
        } else if (0 == m_logConfig.m_strTimeFormat.Find(TEXT("yyyy-MM-ddTHH:mm:ss.SSS+0"))
            || 0 == m_logConfig.m_strTimeFormat.Find(TEXT("yyyy-MM-ddTHH:mm:ss.SSS"))
            || 0 == m_logConfig.m_strTimeFormat.Find(TEXT("yyyy/MM/ddTHH:mm:ss.SSS+0"))
            || 0 == m_logConfig.m_strTimeFormat.Find(TEXT("yyyy/MM/ddTHH:mm:ss.SSS"))
            ) //不要最后的 +08:00
        {
            newDateTimeType = dttDateTimeMilliSecond;
            //2022-03-30T11:17:50.380+08:00   <== Nelo 上的时间
            sscanf_s(strTime.c_str(), "%04hu%*c%02hu%*c%02huT%02hu:%02hu:%02hu%*c%3d+%*c",
                &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond, &milliSecond);
            microSecond = milliSecond * 1000;
        }
        else if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy-MM-dd HH:mm:ss.SSSSSS"))
            || 0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy/MM/dd HH:mm:ss.SSSSSS"))
            )
        {
            newDateTimeType = dttDateTimeMicrosecond;
            //2017-06-12 18:21:34.193000
            sscanf_s(strTime.c_str(), "%04hu%*c%02hu%*c%02hu %02hu:%02hu:%02hu%*c%6d",
                &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond, &microSecond);
        }
        else if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy-MM-dd HH:mm:ss.SSS"))
            || 0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy/MM/dd HH:mm:ss.SSS")))
        {
            newDateTimeType = dttDateTimeMilliSecond;
            //2017-06-12 18:21:34.193
            sscanf_s(strTime.c_str(), "%04hu%*c%02hu%*c%02hu %02hu:%02hu:%02hu%*c%3d",
                &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond, &milliSecond);
            microSecond = milliSecond * 1000;
        }
        else if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy-MM-dd HH:mm:ss"))
            || 0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy/MM/dd HH:mm:ss"))) {
            //2017-06-12 18:21:34
            newDateTimeType = dttDateTimeMilliSecond;
            sscanf_s(strTime.c_str(), "%04hu%*c%02hu%*c%02hu %02hu:%02hu:%02hu",
                &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
        }
        else if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("HH:mm:ss"))) {
            newDateTimeType = dttTimeMilliSecond;
            sscanf_s(strTime.c_str(), "%02hu:%02hu:%02hu",
                &st.wHour, &st.wMinute, &st.wSecond);
        }
        else if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("HH:mm:ss.SSS"))) {
            newDateTimeType = dttTimeMilliSecond;
            sscanf_s(strTime.c_str(), "%02hu:%02hu:%02hu%*c%3d",
                &st.wHour, &st.wMinute, &st.wSecond, &milliSecond);
            microSecond = milliSecond * 1000;
        }
        else if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("HH:mm:ss.SSSSSS"))) {
            newDateTimeType = dttTimeMicrosecond;
            sscanf_s(strTime.c_str(), "%02hu:%02hu:%02hu%*c%6d",
                &st.wHour, &st.wMinute, &st.wSecond, &microSecond);
        }
        else {
            FTLASSERT(FALSE);
            //FTL::FormatMessageBox(NULL, TEXT("Error"), 
            //    MB_OK| MB_ICONERROR, TEXT("Not support date time format"));
            //return LogItemPointer(NULL);
        }

        if (dttDateTimeNone == m_logConfig.m_dateTimeType)
        {
            //如果用户没有选择过, 才设置,否则保留用户的选择
            m_logConfig.m_dateTimeType = newDateTimeType;
        }
        FILETIME localFileTime = { 0 };
        API_VERIFY(SystemTimeToFileTime(&st, &localFileTime));

        fileTimeValue = *((LONGLONG*)&localFileTime);  // 表示 100 ns, 通过除以10返回微秒

        return fileTimeValue / 10 + microSecond;
    }
    return 0;
}

LogItemPointer CLogManager::ParseRegularTraceLog(std::string& strOneLog, const std::tr1::regex& reg, const std::tr1::regex& reg2, const LogItemPointer& preLogItem)
{
    BOOL bRet = FALSE;
    std::tr1::smatch regularResults;
    bool result = std::tr1::regex_match(strOneLog, regularResults, reg);
    LogItemPointer pItem(new LogItem);
    FTL::CFConversion conv;

#if ENABLE_COPY_FULL_LOG
    _ConvertItemInfo(strOneLog, pItem->pszFullLog, m_codePage);
#endif

    pItem->level = FTL::tlTrace;
    if (result) //success
    {
        if (m_logConfig.m_nItemTime != INVLIAD_ITEM_MAP)
        {
            std::string strTime = FTL::Trim(std::string(regularResults[m_logConfig.m_nItemTime]));
            pItem->time = _parseTimeString(strTime);
            pItem->orgTimeStr = strTime;
        }

        if (m_logConfig.m_nItemSeqNum != INVLIAD_ITEM_MAP) {
            pItem->seqNum = atoi(std::string(regularResults[m_logConfig.m_nItemSeqNum]).c_str());
        }
        if (m_logConfig.m_nItemLevel != INVLIAD_ITEM_MAP){
            std::string strLevel = std::string(regularResults[m_logConfig.m_nItemLevel]);
            FTL::Trim(strLevel);
            pItem->level = m_logConfig.GetTraceLevelByText(strLevel);
            pItem->orgLevelStr = strLevel;
        }
        if (m_logConfig.m_nItemMachine != INVLIAD_ITEM_MAP){
            std::string strMachine = std::string(regularResults[m_logConfig.m_nItemMachine]);
            pItem->machine = conv.UTF8_TO_TCHAR(FTL::Trim(strMachine).c_str());
        }
        if (m_logConfig.m_nItemPId != INVLIAD_ITEM_MAP){
            std::string strPid = std::string(regularResults[m_logConfig.m_nItemPId]);
            pItem->processId = conv.UTF8_TO_TCHAR(FTL::Trim(strPid).c_str());
        }
        if (m_logConfig.m_nItemTId != INVLIAD_ITEM_MAP)
        {
            std::string strTid = std::string(regularResults[m_logConfig.m_nItemTId]);
            pItem->threadId = conv.UTF8_TO_TCHAR(FTL::Trim(strTid).c_str());
        }
        if (m_logConfig.m_nItemModule != INVLIAD_ITEM_MAP)
        {
            pItem->moduleNameLen = _ConvertItemInfo(std::string(regularResults[m_logConfig.m_nItemModule]), pItem->pszModuleName, m_codePage);
        }
        if (m_logConfig.m_nItemFun != INVLIAD_ITEM_MAP)
        {
            _ConvertItemInfo(std::string(regularResults[m_logConfig.m_nItemFun]), pItem->pszFunName, m_codePage);
        }
        if (m_logConfig.m_nItemFile != INVLIAD_ITEM_MAP)
        {
            _ConvertItemInfo(std::string(regularResults[m_logConfig.m_nItemFile]), pItem->pszSrcFileName, m_codePage);
        }
        if (m_logConfig.m_nItemLine != INVLIAD_ITEM_MAP)
        {
            pItem->srcFileline = atoi(std::string(regularResults[m_logConfig.m_nItemLine]).c_str());
        }
        if (m_logConfig.m_nItemLog != INVLIAD_ITEM_MAP)
        {
            pItem->traceInfoLen = _ConvertItemInfo(std::string(regularResults[m_logConfig.m_nItemLog]), pItem->pszTraceInfo, m_codePage);
        }

        return pItem;
	} else {
		//如果缺省的正则表达式失败, 则尝试用 REGULAR2 的字符串进行解析
		bool result = std::tr1::regex_match(strOneLog, regularResults, reg2);
		if (result)
		{
			if (m_logConfig.m_nItemFile_2 != INVLIAD_ITEM_MAP)
			{
				_ConvertItemInfo(std::string(regularResults[m_logConfig.m_nItemFile_2]), pItem->pszSrcFileName, m_codePage);
			}
			if (m_logConfig.m_nItemLine_2 != INVLIAD_ITEM_MAP)
			{
				pItem->srcFileline = atoi(std::string(regularResults[m_logConfig.m_nItemLine_2]).c_str());
			}
		}
	}

    //不匹配正则表达式的情况,尽量重用上一条记录的信息
    if (preLogItem)
    {
        pItem->level = preLogItem->level;
        pItem->seqNum = -1;
        pItem->machine = preLogItem->machine;
        pItem->processId = preLogItem->processId;
        pItem->threadId = preLogItem->threadId;
        pItem->time = preLogItem->time;

        //int leftBraPos = strOneLog.find('('); //左括号
        //int rightBraPos = strOneLog.find(')');//右括号

        //如果文字中有左右括号,则可能存在源码信息, 则不要重用上一条记录的文件名和行号
        //if (leftBraPos != std::string::npos && rightBraPos != std::string::npos)
        //如果没有特意设置 源码分析的正则表达式, 才重用上一条记录的文件名和行号
        //if (m_logConfig.m_strSrcRegular.IsEmpty() || m_logConfig.m_nItemSrcFileEx == INVLIAD_ITEM_MAP)
        {
            if (NULL == pItem->pszSrcFileName && preLogItem->pszSrcFileName && preLogItem->srcFileline > 0)
            {
				//在没有通过 REGULAR_2 找到 文件路径/行号的前提下, 才重用以前的信息
                pItem->srcFileline = preLogItem->srcFileline;
                pItem->pszSrcFileName = _CopyItemInfo(preLogItem->pszSrcFileName);
            }
        }
    }

    pItem->traceInfoLen = _ConvertItemInfo(strOneLog, pItem->pszTraceInfo, m_codePage);
    return pItem;
}

LogItemPointer CLogManager::ParseJsonLogItem(const Json::Value& valItem, const LogItemPointer& preLogItem) {
    BOOL bRet = FALSE;
    LogItemPointer pItem(new LogItem);
    FTL::CFConversion conv;

    if (!m_logConfig.m_strItemSeqNum.IsEmpty()) {
        pItem->seqNum = valItem[m_logConfig.m_strItemSeqNum].asInt();
    }
    if (!m_logConfig.m_strItemMachine.IsEmpty()) {
        pItem->machine = conv.UTF8_TO_TCHAR(valItem[m_logConfig.m_strItemMachine].asString().c_str());
    }
    if (!m_logConfig.m_strItemTime.IsEmpty()) {
        std::string timeStr = valItem[m_logConfig.m_strItemTime].asString();
        pItem->time = _parseTimeString(timeStr);
        pItem->orgTimeStr = timeStr;
    }
    if (!m_logConfig.m_strItemLevel.IsEmpty()) {
        std::string strLevel = valItem[m_logConfig.m_strItemLevel].asString();
        pItem->level = m_logConfig.GetTraceLevelByText(strLevel);
        pItem->orgLevelStr = strLevel;
    }
    if (!m_logConfig.m_strItemPId.IsEmpty()) {
        pItem->processId = conv.UTF8_TO_TCHAR(valItem[m_logConfig.m_strItemPId].asString().c_str());
    }    
    if (!m_logConfig.m_strItemTId.IsEmpty()) {
        pItem->threadId = conv.UTF8_TO_TCHAR(valItem[m_logConfig.m_strItemTId].asString().c_str());
    }    
    if (!m_logConfig.m_strItemModule.IsEmpty()) {
        pItem->moduleNameLen = _ConvertItemInfo(valItem[m_logConfig.m_strItemModule].asString(), pItem->pszModuleName, m_codePage);
    }    
    if (!m_logConfig.m_strItemFun.IsEmpty()) {
        _ConvertItemInfo(valItem[m_logConfig.m_strItemFun].asString(), pItem->pszFunName, m_codePage);
    }    
    if (!m_logConfig.m_strItemLine.IsEmpty()) {
        pItem->lineNum = valItem[m_logConfig.m_strItemLine].asInt();
    }
    if (!m_logConfig.m_strItemLog.IsEmpty()) {
        pItem->traceInfoLen = _ConvertItemInfo(valItem[m_logConfig.m_strItemLog].asString(), pItem->pszTraceInfo, m_codePage);
    }
    
//     if(!m_logConfig.m_strItemRegBody.IsEmpty() && !m_logConfig.m_strLogRegular.IsEmpty()) {
//         CAtlStringA logBody = valItem[m_logConfig.m_strItemRegBody].asString();
// 
//         const std::tr1::regex regularPattern(conv.TCHAR_TO_UTF8(m_logConfig.m_strLogRegular));
//         const std::tr1::regex regularPattern2(conv.TCHAR_TO_UTF8(m_logConfig.m_strLogRegular_2));  //如果字符串为空,也没有问题
// 
//         pItem = ParseRegularTraceLog(logBody, regularPattern, regularPattern2, pItem);
//     }

    return pItem;
}

LONG CLogManager::ReadTraceLogFile(LPCTSTR pszFilePath, LONG startLineNum)
{
    BOOL bRet = FALSE;
    CFConversion conv;
    LONG curLineNum = startLineNum;
	//std::wifstream winFile(pszFilePath);
    std::ifstream inFile(conv.TCHAR_TO_MBCS(pszFilePath), std::ios::binary);
    if(inFile.good())
	//if (winFile.good())
    {
		//std::wstring wstrOneLog;
        std::string strOneLog;
        UINT readCount = 1;
        //std::set<PROCESS_ID_TYPE> processIdsContqainer;
        //std::set<THREAD_ID_TYPE> threadIdsContainer;

        LogItemPointer preLogItem;
        DWORD dwStartTime = GetTickCount();

        try{
			const std::tr1::regex regularPattern(conv.TCHAR_TO_UTF8(m_logConfig.m_strLogRegular));

			const std::tr1::regex regularPattern2(conv.TCHAR_TO_UTF8(m_logConfig.m_strLogRegular_2));  //如果字符串为空,也没有问题

			//while (getline(winFile, wstrOneLog))
            while(getline(inFile, strOneLog))
            {
				//strOneLog = conv.TCHAR_TO_UTF8(wstrOneLog.c_str());
                if (readCount == 1 && strOneLog.length() > 3)
                {
                    //判断第一行的 UTF8 文件头
                    unsigned char c1 = strOneLog.at(0);
                    unsigned char c2 = strOneLog.at(1);
                    unsigned char c3 = strOneLog.at(2);

                    if (c1 == 0xEF && c2 == 0xBB && c3 == 0xBF)
                    {
                        strOneLog.erase(0, 3);
                    }
                }
                if (!strOneLog.empty())
                {
                    //去除最后的 "\r 或 \n", 避免正则表达式解析失败.
                    bool needRemove = false;
                    do 
                    {
                        unsigned char cLast = strOneLog.at(strOneLog.length() - 1);
                        needRemove = cLast == '\r' || cLast == '\n';
                        if (needRemove)
                        {
                            strOneLog.erase(strOneLog.length() - 1);
                        }
                    } while (needRemove && !strOneLog.empty());

                    //如果长度太长(比如 600K+ ),正则表达式会失败(内存不足)
                    if (strOneLog.length() > m_logConfig.m_nMaxLineLength)
                    {
                        strOneLog = strOneLog.substr(0, m_logConfig.m_nMaxLineLength);
                    }
                }
				//FTLTRACE(TEXT("parse [%d], str=%s"), readCount, conv.UTF8_TO_TCHAR(strOneLog.c_str()));
                LogItemPointer pLogItem = ParseRegularTraceLog(strOneLog, regularPattern, regularPattern2, preLogItem);
                preLogItem = pLogItem;
                curLineNum++;  
                if (pLogItem)
                {
                    pLogItem->lineNum = curLineNum;
                    _AppendLogItem(pLogItem);
                }
                readCount++; 

#define LOG_INTERVAL_COUNT       10000
                if (0 == (readCount % LOG_INTERVAL_COUNT))
                {
                    //每读取一万记录, 输出一次日志
                    DWORD dwNow = GetTickCount();
                    DWORD dwElapseTime = dwNow - dwStartTime;
                    FTLTRACE(TEXT("count=%d, parse %d log, use %d ms"), readCount, LOG_INTERVAL_COUNT, dwElapseTime);
                    dwStartTime = dwNow;
                }
            }
            bRet = TRUE;
        }catch(const std::tr1::regex_error& e){
            bRet = FALSE;
            FTL::CFConversion convText, convError;
            FTL::FormatMessageBox(NULL, TEXT("Regex Error"), MB_OK, 
                TEXT("readCount=%d, Text=%s\nWrong Regex, reason is %s\nPlease confirm the REGULAR is valid in %s"), 
                readCount, convText.UTF8_TO_TCHAR(strOneLog.c_str()), convError.UTF8_TO_TCHAR(e.what()),
                m_logConfig.m_config.GetFilePathName());
        }
    }
    return curLineNum;
}

LONG CLogManager::ReadJsonLogFile(LPCTSTR pszFilePath, LONG startLineNum) 
{
    BOOL bRet = FALSE;
    LONG curLineNum = startLineNum;
    CFConversion conv;
    //std::wifstream winFile(pszFilePath);
    std::ifstream inFile(conv.TCHAR_TO_MBCS(pszFilePath), std::ios::binary);
    if (inFile.good())
    {
        Json::Value valRoot;
        Json::Reader reader;
        if (reader.parse(inFile, valRoot)) {
            FTLTRACE(TEXT("read json %s, valRoot.size=%d"), pszFilePath, valRoot.size());
            LogItemPointer preLogItem;
            int itemCount = valRoot.size();
            for (int i = 0; i < itemCount; i++) {
                //FTLTRACE(TEXT("parse [%d], str=%s"), readCount, conv.UTF8_TO_TCHAR(strOneLog.c_str()));
                int readIndex = i;
                if (m_logConfig.m_readItemOrder == Desc)
                {
                    readIndex = itemCount - i - 1;
                }
                LogItemPointer pLogItem = ParseJsonLogItem(valRoot[readIndex], preLogItem);
                preLogItem = pLogItem;
                if (pLogItem)
                {
                    curLineNum++;
                    pLogItem->lineNum = curLineNum;
                    _AppendLogItem(pLogItem);
                }
            }
            bRet = TRUE;
        }
    }

    return curLineNum;
}

void CLogManager::_AppendLogItem(LogItemPointer& pLogItem){
    //MachinePIdTIdType idType(pLogItem->machine, pLogItem->processId, pLogItem->threadId);
    ID_INFOS &idInfos = m_allMachinePidTidInfos[pLogItem->machine][pLogItem->processId][pLogItem->threadId];
    if (!idInfos.bInited){ //第一次
        pLogItem->elapseTime = 0;
        idInfos.lastTimeStamp = pLogItem->time;
        idInfos.bInited = TRUE;
    } else {
        pLogItem->elapseTime = pLogItem->time - idInfos.lastTimeStamp;
        idInfos.lastTimeStamp = pLogItem->time;
    }

    m_allInitLogItems.push_back(pLogItem);
}

//花费时间 改为计算显示出来的同线程时间差
BOOL CLogManager::_CalcThreadElpaseTime(LogItemArrayType& logItems, LONG& outProcessCount, LONG& outThreadCount)
{
    ThreadExecuteTimeContainer threadExecuteTimes;
    
    std::set<MachinePIdTIdType> processCalcContainer;

    outProcessCount = 0;
    outThreadCount = 0;

    for (LogItemArrayIterator iterItem = logItems.begin();
        iterItem != logItems.end(); 
        ++iterItem)
    {
        LogItemPointer& pLogItem = *iterItem;

        //计算每一个的线程的时间
        MachinePIdTIdType idType(pLogItem->machine, pLogItem->processId, pLogItem->threadId);
        ThreadExecuteTimeContainer::iterator iterElapse = threadExecuteTimes.find(idType);
        if (threadExecuteTimes.end() == iterElapse )
        {
            iterElapse = threadExecuteTimes.insert(ThreadExecuteTimeContainer::value_type(idType, pLogItem->time)).first;
            pLogItem->elapseTime = 0;
            outThreadCount++;
        }
        else
        {
            pLogItem->elapseTime = pLogItem->time - iterElapse->second;
            iterElapse->second = pLogItem->time;
        }

        //判断是否是新的进程(将线程 id 统一为 空,即可查找(TODO: 将 machine + processId 合并成一个字符串,可以快速计算个数?
        MachinePIdTIdType processCountType(pLogItem->machine, pLogItem->processId, TEXT(""));
        if (processCalcContainer.end() == processCalcContainer.find(processCountType))
        {
            processCalcContainer.insert(processCountType);
        }
    }

    //线程数
    outProcessCount = (LONG)processCalcContainer.size();
    
    return TRUE;
}

