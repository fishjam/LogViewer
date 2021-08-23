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
            bChecked = m_LogManager.IsItemMatchSeqNumber(pItem->seqNum);
        }
        
        if (bChecked)
        {
            bChecked = m_LogManager.IsItemIdChecked(pItem);// .m_AllLogProcessIdsChecked[pItem->processId];
        }
        if (bChecked)
        {
            if (!m_LogManager.m_strLogInfoFilterString.IsEmpty())
            {
                BOOL bFilterByString = TRUE;
                if (m_LogManager.m_filterType == ftAny)
                {
                    bFilterByString = FALSE;
                }

                CAtlString strFilter = m_LogManager.m_strLogInfoFilterString;
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
            case type_Sequence:
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
            case type_FileName:
                if(pItem1->pszSrcFileName && pItem2->pszSrcFileName){
                    result = _tcscmp(pItem1->pszSrcFileName,pItem2->pszSrcFileName);
                    if (result == 0)
                    {
                        result = pItem1->srcFileline - pItem2->srcFileline;
                    }
                }
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
        m_SortContents[i].contentType = type_Sequence;
        m_SortContents[i].bSortAscending = TRUE;
    }
    for (int i = 0; i < tlEnd; i++)
    {
        m_TraceLevelDisplay[i] = TRUE;
    }
    m_codePage = CP_UTF8;
    m_fileCount = 0;

    //Default is [0 ~ -1], it means all logs
    m_nStartSeqNumber = 0;
    m_nEndSeqNumber = -1;

    m_nSelectedProcessCount = -1;
    m_nSelectedThreadCount = -1;
    m_filterType = ftAll;
    m_activeItemIndex = -1;
}

CLogManager::~CLogManager(void)
{
    ClearAllLogItems();
}

BOOL CLogManager::ClearAllLogItems()
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    m_activeItemIndex = -1;
    m_threadExecuteTimes.clear();
    m_AllLogItems.clear();
    m_allInitLogItems.clear();
    m_DisplayLogItems.clear();

    m_allMachinePidTidInfos.clear();
    m_filesMap.clear();

    return TRUE;
}

BOOL CLogManager::SaveLogItems(LPCTSTR pszFilePath, BOOL bAll /* = FALSE */)
{
    BOOL bRet = FALSE;
    srand(GetTickCount());
    CStdioFile file;
    API_VERIFY(file.Open(pszFilePath,CFile::modeWrite | CFile::modeCreate | CFile::typeText));
    if (bRet)
    {
        do 
        {
            CString strFormat;
            CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
            LogItemArrayType* pSaveLogItems = bAll ? &m_AllLogItems : &m_DisplayLogItems;

#if 0
            for (int i = 0; i < 100000; i++)
            {
                strFormat.Format(TEXT("%04d-%02d-%02d %02d:%02d:%02d:%d: (%d|%d) Sau.Utils: INFO: Some Real Infomation\n"),
                    2011,rand()%12+1,
                    rand()%28+1, 
                    rand()%24,
                    rand()%60,rand()%60,
                    rand()%1000000,rand()%20+300000, rand()%30+1090525000);
                file.WriteString(strFormat);
            }
#else
            for (LogItemArrayIterator iter = pSaveLogItems->begin() ; iter != pSaveLogItems->end(); ++iter)
            {
                LogItemPointer pItem = *iter;
                strFormat.Format(TEXT("%08d,%s\n"),pItem->seqNum,pItem->pszTraceInfo);
                file.WriteString(strFormat);
            }
#endif 
        } while (FALSE);
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

BOOL CLogManager::IsItemMatchSeqNumber(LONG seqNumber){
    LONG checkEndSeqNumber = m_nEndSeqNumber < 0 ? LONG_MAX : m_nEndSeqNumber;
    
    return FTL_INRANGE(m_nStartSeqNumber, seqNumber , checkEndSeqNumber);
}

BOOL CLogManager::IsItemIdChecked(const LogItemPointer& pItem){
    ID_INFOS& idInfos = m_allMachinePidTidInfos[pItem->machine][pItem->processId][pItem->threadId];
    return idInfos.bChecked;
}

void CLogManager::OnlySelectSpecialItems(const MachinePIdTIdType& selectIdType, ONLY_SELECT_TYPE selType) {
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);

    for (MachinePidTidContainer::iterator iterMachine = m_allMachinePidTidInfos.begin();
        iterMachine != m_allMachinePidTidInfos.end();
        ++iterMachine)
    {
        MACHINE_NAME_TYPE curMachine = iterMachine->first;
        BOOL isSameMachine = (curMachine == selectIdType.machine);
        PidTidContainer& pidTidContainer = iterMachine->second;

        for (PidTidContainer::iterator iterPid = pidTidContainer.begin();
            iterPid != pidTidContainer.end();
            ++iterPid)
        {
            PROCESS_ID_TYPE curProcess = iterPid->first;
            BOOL isSameProcess = (curProcess == selectIdType.pid);
            TidContainer& tidContainer = iterPid->second;

            for (TidContainer::iterator iterTid = tidContainer.begin();
                iterTid != tidContainer.end();
                ++iterTid) {
                THREAD_ID_TYPE curThread = iterTid->first;
                BOOL isSameThread = (curThread == selectIdType.tid);
                ID_INFOS& rIdInfos = iterTid->second;

                if (ostMachine == selType) {
                    rIdInfos.bChecked = isSameMachine;
                }
                else if (ostProcessId == selType) {
                    rIdInfos.bChecked = isSameMachine && isSameProcess;
                }
                else if (ostThreadId == selType) {
                    rIdInfos.bChecked = isSameMachine && isSameProcess && isSameThread;
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

BOOL CLogManager::SetFilterSeqNumber(LONG nStartSeqNumber, LONG nEndSeqNumber){
    m_nStartSeqNumber = nStartSeqNumber;
    m_nEndSeqNumber = nEndSeqNumber;
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
        m_activeItemIndex = -1;
        m_DisplayLogItems.clear();
        m_DisplayLogItems.reserve(displayItems.size());
        m_DisplayLogItems.assign(displayItems.begin(), displayItems.end());
        SortDisplayItem(m_SortContents[0].contentType,m_SortContents[0].bSortAscending);
    }
    _CalcThreadElpaseTime(m_DisplayLogItems);
    m_nSelectedProcessCount = -1;
    m_nSelectedThreadCount = -1;
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
            if (result && regularResults.size() >= m_logConfig.m_nItemSrcFileEx)
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

void CLogManager::setActiveItemIndex(LONG index){
    m_activeItemIndex = index;
}

BOOL CLogManager::DeleteItems(std::set<LONG> delItems) {
    //std::sort(items.begin(), items.end());
    BOOL bDeleted = FALSE;
    for (LogItemArrayType::iterator iter = m_AllLogItems.begin();
        iter != m_AllLogItems.end(); )
    {
        if (delItems.find((*iter)->seqNum) != delItems.end())
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

CString CLogManager::getActiveItemTraceInfo(){
    if (m_activeItemIndex >= 0 && m_activeItemIndex < (LONG)m_DisplayLogItems.size()){
        return m_DisplayLogItems.at(m_activeItemIndex)->pszTraceInfo;
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
        CString strExt = logFilePaths[0].Mid(logFilePaths[0].ReverseFind(_T('.')));

        INT_PTR logFileCount = logFilePaths.GetCount();
        for (INT_PTR index = 0; bRet && index < logFileCount; index++)
        {
//             if (0 == strExt.CompareNoCase(TEXT(".ftl")))
//             {
//                 bRet = ReadFTLogFile(logFilePaths[index]);
//             }
//             else
            {
                bRet = ReadTraceLogFile(logFilePaths[index]);
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

        m_logFilePaths.Copy(logFilePaths);

        AfxGetMainWnd()->SetWindowText(logFilePaths.GetAt(0));
    }
    return bRet;
}

// BOOL CLogManager::CheckFTLogFiles(const CStringArray &logFilePaths)
// {
//     BOOL bRet = FALSE;
//     if (logFilePaths.GetSize() > 0)
//     {
//         bRet = TRUE;
//     }
//     return bRet;
// }

// BOOL CLogManager::ReadFTLogFile(LPCTSTR pszFilePath)
// {
//     BOOL bRet = FALSE;
//     CFile file;
//     API_VERIFY(file.Open(pszFilePath,CFile::modeRead | CFile::shareDenyWrite |CFile::typeBinary));
//     if (bRet)
//     {
//         LONGLONG llLastTime = 0;
//         do 
//         {
//             if (file.GetLength() < (sizeof(CFFastTrace::FTFILEHEADER) + sizeof(CFFastTrace::FTDATA)))
//             {
//                 FTLASSERT(FALSE); //至少应该写一个FTDATA
//                 bRet = FALSE;
//                 break;
//             }
//             UINT readCount = 0;
//             CFFastTrace::FTFILEHEADER fileHeader = {0};
//             readCount = file.Read(&fileHeader,sizeof(fileHeader));
//             API_VERIFY(readCount == sizeof(fileHeader));
//             FTLASSERT(FTFILESIG == fileHeader.dwSig);  //检查是否是 FTLog文件
//             if (FTFILESIG != fileHeader.dwSig)
//             {
//                 bRet = FALSE;
//                 break;
//             }
//             FTLTRACE(TEXT("ReadFTLogFile,file = %s, ItemCount = %d\n"),pszFilePath,fileHeader.lItemCount);
// #if 0
//             m_AllLogProcessIds.push_back(fileHeader.lPID);
//             m_AllLogProcessIdsChecked[fileHeader.lPID] = TRUE;
//             m_AllLogThreadIds.push_back(fileHeader.lTID);
//             m_AllLogThreadIdsChecked[fileHeader.lTID] = TRUE;  //默认都是选择的(不过滤)
// #endif
//             //m_AllFtDatas.resize(m_AllFtDatas.size() + fileHeader.lItemCount);   //提前分配空间
//             DWORD dwFtDataSize = sizeof(FTL::CFFastTrace::FTDATA) - sizeof(LPCTSTR);  //不要读最后的字符串指针
//             //for (LONG lReadItemCount = 0; lReadItemCount < fileHeader.lItemCount; lReadItemCount++)
//             while (bRet)
//             {
//                 LogItemPointer pLogItem(new LogItem);
//                 pLogItem->size = sizeof(LogItem);
// 
//                 FTL::CFFastTrace::FTDATA dataItem = {0};
//                 //读入每一个FTDATA的信息
//                 readCount = file.Read(&dataItem, dwFtDataSize); 
//                 API_VERIFY_EXCEPT1(readCount == dwFtDataSize, ERROR_SUCCESS);        //错误处理
//                 if (readCount != dwFtDataSize)
//                 {
//                     if (readCount == 0 && GetLastError() == ERROR_SUCCESS)
//                     {
//                         //Read End
//                         bRet = TRUE;
//                         break;
//                     }
//                     FTLTRACEEX(FTL::tlWarn, TEXT("Read FTL Log Info Fail, want=%d, read=%d\n"),
//                         dwFtDataSize, readCount);
//                     break;
//                 }
// 
//                 FTLASSERT(dataItem.lSeqNum > 0);				
//                 FTLASSERT(dataItem.nTraceInfoLen > 0);
//                 if (dataItem.nTraceInfoLen <= 0)
//                 {
//                     FTLTRACEEX(FTL::tlWarn, TEXT("nTraceInfoLen=%d\n"),
//                         dataItem.nTraceInfoLen);
//                     bRet = FALSE;
//                     break;
//                 }
// 
//                 pLogItem->seqNum = dataItem.lSeqNum;
//                 pLogItem->level = dataItem.level;
// 
//                 //TODO:convert time
//                 SYSTEMTIME	curSysTime = {0};
//                 FileTimeToSystemTime(&dataItem.stTime, &curSysTime);
// 
//                 pLogItem->time = MAKELONGLONG(dataItem.stTime.dwLowDateTime, dataItem.stTime.dwHighDateTime);
//                 pLogItem->traceInfoLen = dataItem.nTraceInfoLen;
//                 pLogItem->processId = fileHeader.lPID;
//                 pLogItem->threadId = fileHeader.lTID;
//                 
//                 
//                 pLogItem->pszTraceInfo = new WCHAR[pLogItem->traceInfoLen]; //保存的结果始终都用 WCHAR 的(ANSI的转换后也会变大)
//                 ZeroMemory((LPVOID)pLogItem->pszTraceInfo, pLogItem->traceInfoLen * sizeof(WCHAR));
//                 LPVOID pVoidReadBuf = (LPVOID)pLogItem->pszTraceInfo;  //先设置读取缓存的值
// //#ifndef UNICODE 
//                 //if (fileHeader.bIsUnicode) //如果是非UNICODE编译方式，但 File 是Unicode的，需要重新分配缓存
//                 //{
//                 //    pVoidReadBuf = new WCHAR[pLogItem->nTraceInfoLen];
//                 //    ZeroMemory(pVoidReadBuf, pLogItem->nTraceInfoLen * sizeof(WCHAR));
//                 //}
// //#endif
//                 DWORD dwCharSize = 0;
//                 if(fileHeader.bIsUnicode)
//                 {
//                     dwCharSize = sizeof(WCHAR);
//                 }
//                 else
//                 {
//                     dwCharSize = sizeof(CHAR);
//                 }
// 
//                 //读入FTDATA关联的字符串
//                 readCount = file.Read(pVoidReadBuf,pLogItem->traceInfoLen * dwCharSize);
//                 API_VERIFY((LONG)readCount == (LONG)(pLogItem->traceInfoLen * dwCharSize));                //错误处理
//                 if (!bRet)
//                 {
//                     break;
//                 }
//                 if (FALSE == fileHeader.bIsUnicode) //File 是非Unicode的，需要转换成Unicode字符串
//                 {
//                     ASSERT(FALSE);
//                 }
// 
//                 //FTLTRACE(TEXT("pFTData->pszTraceInfo(%d) = %s"),lReadItemCount,pLogItem->pszTraceInfo);
//                 //pLogItem->processId = fileHeader.lPID;
//                 //pLogItem->threadId = fileHeader.lTID;
//                 if (llLastTime == 0)
//                 {
//                     pLogItem->elapseTime = 0;
//                 }
//                 else
//                 {
//                     pLogItem->elapseTime = pLogItem->time - llLastTime;
//                 }
//                 llLastTime = pLogItem->time;
//                 //加入队列
//                 m_allInitLogItems.push_back(pLogItem);
//             }
//         } while (FALSE);
//         file.Close();
//     }
//     return bRet;
// }

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

BOOL CLogManager::ScanSourceFiles(const CString& strFolderPath)
{
    BOOL bRet = FALSE;
    FTL::CFFileFinder fileFinder;
    fileFinder.SetCallback(this, this);
    API_VERIFY(fileFinder.Find(strFolderPath, m_logConfig.m_strSourceFileExts, TRUE));
    return bRet;
}

SameNameFilePathListPtr CLogManager::FindFileFullPath(CString strFileName) {
    FileName2FullPathMap::iterator iter = m_filesMap.find(strFileName);
    if (iter != m_filesMap.end())  //find
    {
        return iter->second;
    }
    return nullptr;
}

LogItemPointer CLogManager::ParseRegularTraceLog(std::string& strOneLog, const std::tr1::regex& reg, const LogItemPointer& preLogItem)
{
    std::tr1::smatch regularResults;
    bool result = std::tr1::regex_match(strOneLog, regularResults, reg);
    LogItemPointer pItem(new LogItem);

#if ENABLE_COPY_FULL_LOG
    _ConvertItemInfo(strOneLog, pItem->pszFullLog, m_codePage);
#endif

    pItem->level = FTL::tlTrace;
    if (result) //success
    {
        if (m_logConfig.m_nItemTime != INVLIAD_ITEM_MAP)
        {
            std::string strTime = std::string(regularResults[m_logConfig.m_nItemTime]);
            if (!m_logConfig.m_strTimeFormat.IsEmpty())
            {
                SYSTEMTIME st = {0};
                GetLocalTime(&st);
                int microSecond = 0;//, ignore, zooHour = 0, zooMinute = 0;
                if (0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy-MM-dd HH:mm:ss.SSS"))
                    || 0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy-MM-dd HH:mm:ss.SSS"))
                    )
                {
                    m_logConfig.m_dateTimeType = dttDateTime;
                    //2017-06-12 18:21:34.193
                    sscanf_s(strTime.c_str(), "%4d-%2d-%2d%*c%2d:%2d:%2d%*c%3d",
                        &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond, &microSecond);
                    st.wMilliseconds = (WORD)microSecond;
                }
                else if(0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy-MM-dd HH:mm:ss"))){
                    //2017-06-12 18:21:34
                    m_logConfig.m_dateTimeType = dttDateTime;
                    sscanf_s(strTime.c_str(), "%4d-%2d-%2d %2d:%2d:%2d",
                        &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
                }
                else if(0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("HH:mm:ss"))){
                    m_logConfig.m_dateTimeType = dttTime;
                    sscanf_s(strTime.c_str(), "%2d:%2d:%2d",
                        &st.wHour, &st.wMinute, &st.wSecond);
                }
                else if(0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("HH:mm:ss.SSS"))){
                    m_logConfig.m_dateTimeType = dttTime;
                    sscanf_s(strTime.c_str(), "%2d:%2d:%2d%*c%3d",
                        &st.wHour, &st.wMinute, &st.wSecond, &microSecond);
                    st.wMilliseconds = (WORD)microSecond;
                }
                else{
                    FTLASSERT(FALSE);
                }

                if (m_logConfig.m_strDisplayTimeFormat.Find(TEXT("yyyy-MM-dd")) >= 0)
                {
                    m_logConfig.m_dateTimeType = dttDateTime;
                }
                else {
                    m_logConfig.m_dateTimeType = dttTime;
                }

                FILETIME localFileTime = {0};
                SystemTimeToFileTime(&st,&localFileTime);
                pItem->time = *((LONGLONG*)&localFileTime);// ((((st.wHour * 60) + st.wMinute) * 60) + st.wSecond)* 1000 + microSecond;
                if (m_logConfig.m_dateTimeType == dttTime)
                {
                    pItem->time %= MIN_TIME_WITH_DAY_INFO;
                }
            }
        }

        if (m_logConfig.m_nItemLevel != INVLIAD_ITEM_MAP){
            std::string strLevel = std::string(regularResults[m_logConfig.m_nItemLevel]);
            FTL::Trim(strLevel);
            pItem->level = m_logConfig.GetTraceLevelByText(strLevel);
        }
        if (m_logConfig.m_nItemMachine != INVLIAD_ITEM_MAP){
            std::string strMachine = std::string(regularResults[m_logConfig.m_nItemMachine]);
            pItem->machine = FTL::Trim(strMachine);
        }
        if (m_logConfig.m_nItemPId != INVLIAD_ITEM_MAP){
            std::string strPid = std::string(regularResults[m_logConfig.m_nItemPId]);
            pItem->processId = FTL::Trim(strPid);
        }
        if (m_logConfig.m_nItemTId != INVLIAD_ITEM_MAP)
        {
            std::string strTid = std::string(regularResults[m_logConfig.m_nItemTId]);
            pItem->threadId = FTL::Trim(strTid);
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
    }
    if (preLogItem)
    {
        pItem->level = preLogItem->level;
        pItem->machine = preLogItem->machine;
        pItem->processId = preLogItem->processId;
        pItem->threadId = preLogItem->threadId;
        pItem->time = preLogItem->time;
    }
    pItem->traceInfoLen = _ConvertItemInfo(strOneLog, pItem->pszTraceInfo, m_codePage);
    return pItem;
}

BOOL CLogManager::ReadTraceLogFile(LPCTSTR pszFilePath)
{
    BOOL bRet = FALSE;
    CFConversion conv;
    std::ifstream inFile(conv.TCHAR_TO_UTF8(pszFilePath));
    if (inFile.good())
    {
        std::string strOneLog;
        UINT readCount = 1;
        std::set<PROCESS_ID_TYPE> processIdsContqainer;
        std::set<THREAD_ID_TYPE> threadIdsContainer;

        LogItemPointer preLogItem;

        try{
            const std::tr1::regex regularPattern(conv.TCHAR_TO_UTF8(m_logConfig.m_strLogRegular));

            while(getline(inFile, strOneLog))
            {
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
                }
                LogItemPointer pLogItem = ParseRegularTraceLog(strOneLog, regularPattern, preLogItem);
                preLogItem = pLogItem;
                if (pLogItem)
                {
                    _AppendLogItem(pLogItem);
                    pLogItem->seqNum = readCount;
                }
                readCount++; //放到这个地方来，保证即使一行日志读取失败，SeqNum还是和文件的行数对应
            }
        }catch(const std::tr1::regex_error& e){
            bRet = FALSE;
            FTL::CFConversion convText, convError;
            FTL::FormatMessageBox(NULL, TEXT("Regex Error"), MB_OK, 
                TEXT("readCount=%d, Text=%s\nWrong Regex, reason is %s\nPlease confirm the REGULAR is valid in %s"), 
                readCount, convText.UTF8_TO_TCHAR(strOneLog.c_str()), convError.UTF8_TO_TCHAR(e.what()),
                m_logConfig.m_config.GetFilePathName());
        }

        bRet = TRUE;
    }
    return bRet;
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
BOOL CLogManager::_CalcThreadElpaseTime(LogItemArrayType& logItems)
{
    ThreadExecuteTimeContainer threadExecuteTimes;

    for (LogItemArrayIterator iterItem = logItems.begin();
        iterItem != logItems.end(); 
        ++iterItem)
    {
        LogItemPointer& pLogItem = *iterItem;

        MachinePIdTIdType idType(pLogItem->machine, pLogItem->processId, pLogItem->threadId);
        ThreadExecuteTimeContainer::iterator iterElapse = threadExecuteTimes.find(idType);
        if (threadExecuteTimes.end() == iterElapse )
        {
            iterElapse = threadExecuteTimes.insert(ThreadExecuteTimeContainer::value_type(idType, pLogItem->time)).first;
            pLogItem->elapseTime = 0;
        }
        else
        {
            pLogItem->elapseTime = pLogItem->time - iterElapse->second;
            iterElapse->second = pLogItem->time;
        }
    }
    return TRUE;
}

