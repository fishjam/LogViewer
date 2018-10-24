#include "StdAfx.h"
#include "LogManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//函数子
struct LogItemFilter : public std::unary_function<LogItemPointer, bool>
{
public:
    LogItemFilter(CLogManager & logManager)
        : m_LogManager(logManager)
    {
    }
    bool operator()(const LogItemPointer& pItem) const
    {
        BOOL bChecked = m_LogManager.m_TraceLevelDisplay[pItem->level];
		if (bChecked)
		{
			bChecked = m_LogManager.m_AllLogProcessIdsChecked[pItem->processId];
		}
		if (bChecked)
        {
            bChecked = m_LogManager.m_AllLogThreadIdsChecked[pItem->threadId];
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
            case type_ProcessId:
                //result = pItem1->processId - pItem2->processId;
                result = pItem1->processId.compare(pItem2->processId);
                break;
            case type_ThreadId:
                //result = pItem1->threadId - pItem2->threadId;
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
    m_allInitLogItems.clear();
    m_AllLogItems.clear();
    m_allInitLogItems.clear();
    m_DisplayLogItems.clear();

	m_AllLogProcessIds.clear();
	m_AllLogProcessIdsChecked.clear();

	m_AllLogThreadIds.clear();
    m_AllLogThreadIdsChecked.clear();
	//m_logFilePaths.RemoveAll();
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

LONG CLogManager::GetThreadCount() const
{
    LONG lCount;
    {
        CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
        lCount = (LONG)m_AllLogThreadIds.size();
    }
    return lCount;
}

LONG CLogManager::GetSelectedThreadCount() const
{
	LONG lCount = 0;
	{
		CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
		for (std::map<THREAD_ID_TYPE, BOOL>::const_iterator iter = m_AllLogThreadIdsChecked.begin();
			iter != m_AllLogThreadIdsChecked.end();
			iter++)
		{
			if (iter->second)
			{
				lCount++;
			}
		}
	}
	return lCount;
}

LONG CLogManager::GetProcessCount() const
{
	LONG lCount;
	{
		CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
		lCount = (LONG)m_AllLogProcessIds.size();
	}
	return lCount;
}

LONG CLogManager::GetThreadIds(AllThreadIdContainer & allThreadIds) const
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    std::copy(m_AllLogThreadIds.begin(), m_AllLogThreadIds.end(), 
		std::back_inserter(allThreadIds));
    return allThreadIds.size();
}

LONG CLogManager::GetProcessIds(AllProcessIdContainer & allProcessIds) const
{
	CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
	std::copy(m_AllLogProcessIds.begin(), m_AllLogProcessIds.end(), 
		std::back_inserter(allProcessIds));
	return allProcessIds.size();
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

BOOL CLogManager::IsThreadIdChecked(THREAD_ID_TYPE threadId) const
{
    BOOL bChecked = FALSE;
    {
        CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
        std::map<THREAD_ID_TYPE, BOOL>::const_iterator iter = m_AllLogThreadIdsChecked.find(threadId);
        if (iter != m_AllLogThreadIdsChecked.end())
        {
            bChecked = iter->second;
        }
    }
    return bChecked;
}

BOOL CLogManager::IsProcessIdChecked(PROCESS_ID_TYPE processId) const
{
	BOOL bChecked = FALSE;
	{
		CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
		std::map<PROCESS_ID_TYPE, BOOL>::const_iterator iter = m_AllLogProcessIdsChecked.find(processId);
		if (iter != m_AllLogProcessIdsChecked.end())
		{
			bChecked = iter->second;
		}
	}
	return bChecked;
}

BOOL CLogManager::SetThreadIdChecked(THREAD_ID_TYPE threadId, BOOL bChecked)
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
    BOOL bRet = TRUE;

    if (m_AllLogThreadIdsChecked[threadId] != bChecked)
    {
        m_AllLogThreadIdsChecked[threadId] = bChecked;
        DoFilterLogItems();
    }
    return bRet;
}

BOOL CLogManager::SetProcessIdChecked(PROCESS_ID_TYPE processId, BOOL bChecked)
{
	CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
	BOOL bRet = TRUE;

	if (m_AllLogProcessIdsChecked[processId] != bChecked)
	{
		m_AllLogProcessIdsChecked[processId] = bChecked;
		DoFilterLogItems();
	}
	return bRet;
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

void CLogManager::setActiveItemIndex(LONG index){
	m_activeItemIndex = index;
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

LONG CLogManager::GenerateLog(LPCTSTR pszFilePath, LONG logCount)
{
    //FTL::CFAnsiFile file(TextFileEncoding::tfeUnknown);
    LONG nGenerateCount = 0;
    return nGenerateCount;
}

BOOL CLogManager::SetLogFiles(const CStringArray &logFilePaths)
{
    CFAutoLock<CFLockObject>  locker(&m_CsLockObj);
	BOOL bRet = FALSE;

	//加载配置文件
	//API_VERIFY(m_logConfig.LoadConfig(TEXT("LogViewer.ini")));
	CWaitCursor waitCursor;

	bRet = CheckFTLogFiles(logFilePaths);
    if (bRet)
    {
        m_fileCount = logFilePaths.GetCount();

        ClearAllLogItems(); //先清除以前的
        CString strExt = logFilePaths[0].Mid(logFilePaths[0].ReverseFind(_T('.')));

        INT_PTR logFileCount = logFilePaths.GetCount();
        for (INT_PTR index = 0; bRet && index < logFileCount; index++)
        {
            if (0 == strExt.CompareNoCase(TEXT(".ftl")))
            {
                bRet = ReadFTLogFile(logFilePaths[index]);
            }
			else
			{
                bRet = ReadTraceLogFile(logFilePaths[index]);
            }
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

BOOL CLogManager::CheckFTLogFiles(const CStringArray &logFilePaths)
{
    BOOL bRet = FALSE;
    if (logFilePaths.GetSize() > 0)
    {
		bRet = TRUE;
    }
    return bRet;
}

BOOL CLogManager::ReadFTLogFile(LPCTSTR pszFilePath)
{
    BOOL bRet = FALSE;
    CFile file;
    API_VERIFY(file.Open(pszFilePath,CFile::modeRead | CFile::shareDenyWrite |CFile::typeBinary));
    if (bRet)
    {
		LONGLONG llLastTime = 0;
        do 
        {
            if (file.GetLength() < (sizeof(CFFastTrace::FTFILEHEADER) + sizeof(CFFastTrace::FTDATA)))
            {
                FTLASSERT(FALSE); //至少应该写一个FTDATA
                bRet = FALSE;
                break;
            }
            UINT readCount = 0;
            CFFastTrace::FTFILEHEADER fileHeader = {0};
            readCount = file.Read(&fileHeader,sizeof(fileHeader));
            API_VERIFY(readCount == sizeof(fileHeader));
            FTLASSERT(FTFILESIG == fileHeader.dwSig);  //检查是否是 FTLog文件
            if (FTFILESIG != fileHeader.dwSig)
            {
                bRet = FALSE;
                break;
            }
            FTLTRACE(TEXT("ReadFTLogFile,file = %s, ItemCount = %d\n"),pszFilePath,fileHeader.lItemCount);
#if 0
            m_AllLogProcessIds.push_back(fileHeader.lPID);
            m_AllLogProcessIdsChecked[fileHeader.lPID] = TRUE;
			m_AllLogThreadIds.push_back(fileHeader.lTID);
            m_AllLogThreadIdsChecked[fileHeader.lTID] = TRUE;  //默认都是选择的(不过滤)
#endif
            //m_AllFtDatas.resize(m_AllFtDatas.size() + fileHeader.lItemCount);   //提前分配空间
			DWORD dwFtDataSize = sizeof(FTL::CFFastTrace::FTDATA) - sizeof(LPCTSTR);  //不要读最后的字符串指针
            //for (LONG lReadItemCount = 0; lReadItemCount < fileHeader.lItemCount; lReadItemCount++)
			while (bRet)
            {
                LogItemPointer pLogItem(new LogItem);
				pLogItem->size = sizeof(LogItem);

				FTL::CFFastTrace::FTDATA dataItem = {0};
                //读入每一个FTDATA的信息
                readCount = file.Read(&dataItem, dwFtDataSize); 
				API_VERIFY_EXCEPT1(readCount == dwFtDataSize, ERROR_SUCCESS);        //错误处理
				if (readCount != dwFtDataSize)
				{
					if (readCount == 0 && GetLastError() == ERROR_SUCCESS)
					{
						//Read End
						bRet = TRUE;
						break;
					}
					FTLTRACEEX(FTL::tlWarn, TEXT("Read FTL Log Info Fail, want=%d, read=%d\n"),
						dwFtDataSize, readCount);
					break;
				}

				FTLASSERT(dataItem.lSeqNum > 0);				
				FTLASSERT(dataItem.nTraceInfoLen > 0);
				if (dataItem.nTraceInfoLen <= 0)
				{
					FTLTRACEEX(FTL::tlWarn, TEXT("nTraceInfoLen=%d\n"),
						dataItem.nTraceInfoLen);
					bRet = FALSE;
					break;
				}

				pLogItem->seqNum = dataItem.lSeqNum;
				pLogItem->level = dataItem.level;

                //TODO:convert time
				SYSTEMTIME	curSysTime = {0};
				FileTimeToSystemTime(&dataItem.stTime, &curSysTime);

				pLogItem->time = MAKELONGLONG(dataItem.stTime.dwLowDateTime, dataItem.stTime.dwHighDateTime);
				pLogItem->traceInfoLen = dataItem.nTraceInfoLen;
				pLogItem->processId = fileHeader.lPID;
				pLogItem->threadId = fileHeader.lTID;
				
                
                pLogItem->pszTraceInfo = new WCHAR[pLogItem->traceInfoLen]; //保存的结果始终都用 WCHAR 的(ANSI的转换后也会变大)
                ZeroMemory((LPVOID)pLogItem->pszTraceInfo, pLogItem->traceInfoLen * sizeof(WCHAR));
                LPVOID pVoidReadBuf = (LPVOID)pLogItem->pszTraceInfo;  //先设置读取缓存的值
//#ifndef UNICODE 
                //if (fileHeader.bIsUnicode) //如果是非UNICODE编译方式，但 File 是Unicode的，需要重新分配缓存
                //{
                //    pVoidReadBuf = new WCHAR[pLogItem->nTraceInfoLen];
                //    ZeroMemory(pVoidReadBuf, pLogItem->nTraceInfoLen * sizeof(WCHAR));
                //}
//#endif
                DWORD dwCharSize = 0;
                if(fileHeader.bIsUnicode)
                {
                    dwCharSize = sizeof(WCHAR);
                }
                else
                {
                    dwCharSize = sizeof(CHAR);
                }

                //读入FTDATA关联的字符串
                readCount = file.Read(pVoidReadBuf,pLogItem->traceInfoLen * dwCharSize);
                API_VERIFY((LONG)readCount == (LONG)(pLogItem->traceInfoLen * dwCharSize));                //错误处理
				if (!bRet)
				{
					break;
				}
                if (FALSE == fileHeader.bIsUnicode) //File 是非Unicode的，需要转换成Unicode字符串
                {
                    ASSERT(FALSE);
                }

                //FTLTRACE(TEXT("pFTData->pszTraceInfo(%d) = %s"),lReadItemCount,pLogItem->pszTraceInfo);
                //pLogItem->processId = fileHeader.lPID;
                //pLogItem->threadId = fileHeader.lTID;
				if (llLastTime == 0)
				{
					pLogItem->elapseTime = 0;
				}
				else
				{
					pLogItem->elapseTime = pLogItem->time - llLastTime;
				}
				llLastTime = pLogItem->time;
                //加入队列
                m_allInitLogItems.push_back(pLogItem);
            }
        } while (FALSE);
        file.Close();
    }
    return bRet;
}

int CLogManager::_ConvertItemInfo(const std::string& srcInfo, LPCTSTR& pszDest, UINT codePage)
{
    int srcLenth = srcInfo.length();
    pszDest = new WCHAR[srcLenth + 1];
    ZeroMemory((void*)pszDest, sizeof(WCHAR) * (srcLenth +1));
    MultiByteToWideChar(codePage, 0, srcInfo.c_str(), -1, (LPWSTR)pszDest, srcLenth);
    return srcLenth;
}

LogItemPointer CLogManager::ParseRegularTraceLog(std::string& strOneLog, const std::tr1::regex& reg, const LogItemPointer& preLogItem)
{
	std::tr1::smatch regularResults;
	bool result = std::tr1::regex_match(strOneLog, regularResults, reg);
	LogItemPointer pItem(new LogItem);

    _ConvertItemInfo(strOneLog, pItem->pszFullLog, m_codePage);
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
                    st.wMilliseconds = microSecond;
				}
                else if(0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("yyyy-MM-dd HH:mm:ss"))){
                    //2017-06-12 18:21:34
					m_logConfig.m_dateTimeType = dttDateTime;
                    sscanf_s(strTime.c_str(), "%4d-%2d-%2d %2d:%2d:%2d%",
                        &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
                }
                else if(0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("HH:mm:ss"))){
					m_logConfig.m_dateTimeType = dttTime;
                    sscanf_s(strTime.c_str(), "%2d:%2d:%2d%",
                        &st.wHour, &st.wMinute, &st.wSecond);
                }
				else if(0 == m_logConfig.m_strTimeFormat.CompareNoCase(TEXT("HH:mm:ss.SSS"))){
					m_logConfig.m_dateTimeType = dttTime;
					sscanf_s(strTime.c_str(), "%2d:%2d:%2d%*c%3d",
						&st.wHour, &st.wMinute, &st.wSecond, &microSecond);
                    st.wMilliseconds = microSecond;
				}
				else{
					FTLASSERT(FALSE);
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
		if (m_logConfig.m_nItemLevel != INVLIAD_ITEM_MAP)
		{
			std::string strLevel = std::string(regularResults[m_logConfig.m_nItemLevel]);
			//std::replace_if(strLevel.begin(), strLevel.end(), std::bind2nd(std::equal_to<char>(), '\s'), '');
			FTL::Trim(strLevel);
			pItem->level = m_logConfig.GetTraceLevelByText(strLevel);
		}

		if (m_logConfig.m_nItemPId != INVLIAD_ITEM_MAP)
		{
            std::string strPid = std::string(regularResults[m_logConfig.m_nItemPId]);
            pItem->processId = FTL::Trim(strPid);
			//pItem->processId = atoi(std::string(regularResults[m_logConfig.m_nItemPId]).c_str());
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

            while(getline(inFile, strOneLog, '\n'))
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
                LogItemPointer pLogItem = ParseRegularTraceLog(strOneLog, regularPattern, preLogItem);
                preLogItem = pLogItem;
                if (pLogItem)
                {
                    if (processIdsContqainer.find(pLogItem->processId) == processIdsContqainer.end())
                    {
                        processIdsContqainer.insert(pLogItem->processId);
                        m_AllLogProcessIds.push_back(pLogItem->processId);
                        m_AllLogProcessIdsChecked[pLogItem->processId] = TRUE;  //默认都是选择的(不过滤)
                    }

                    //PIdTIdType idType(pLogItem->processId, pLogItem->threadId);
                    if (threadIdsContainer.find(pLogItem->threadId) == threadIdsContainer.end())
                    {
                        threadIdsContainer.insert(pLogItem->threadId);
                        m_AllLogThreadIds.push_back(pLogItem->threadId);
                        m_AllLogThreadIdsChecked[pLogItem->threadId] = TRUE;  //默认都是选择的(不过滤)
                    }

                    PIdTIdType idType(pLogItem->processId, pLogItem->threadId);
                    ThreadExecuteTimeContainer::iterator iter = m_threadExecuteTimes.find(idType);
                    //LONG lNewTime = MAKELONG(pLogItem->stTime.dwHighDateTime, pLogItem->stTime.dwLowDateTime);

                    if (m_threadExecuteTimes.end() == iter )
                    {
                        iter = m_threadExecuteTimes.insert(ThreadExecuteTimeContainer::value_type(idType, pLogItem->time)).first;
                        pLogItem->elapseTime = 0;
                    }
                    else
                    {
                        pLogItem->elapseTime = pLogItem->time - iter->second;
                        iter->second = pLogItem->time;
                    }
#if 0
                    CTime newTime(pLogItem->time);
                    CTime oldTime(iter->second);
                    CTimeSpan diffTime = newTime - oldTime;
                    pLogItem->llElapseTime = diffTime.GetTotalSeconds();
#endif
                    iter->second = pLogItem->time;
                    pLogItem->seqNum = readCount;

                    m_allInitLogItems.push_back(pLogItem);
                }
                readCount++; //放到这个地方来，保证即使一行日志读取失败，SeqNum还是和文件的行数对应
            }
        }catch(const std::tr1::regex_error& /*e*/){
            bRet = FALSE;
            FTL::CFConversion conv;
            FTL::FormatMessageBox(NULL, TEXT("Regex Error"), MB_OK, TEXT("Wrong Regex: \"%s\"\nPlease confirm the REGULAR is valid in %s"), 
                m_logConfig.m_strLogRegular, m_logConfig.m_config.GetFilePathName());
        }

        bRet = TRUE;
    }
    return bRet;
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

		PIdTIdType idType(pLogItem->processId, pLogItem->threadId);
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

