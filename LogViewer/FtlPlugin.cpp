#include "FtlPlugin.h"

CFtlPlugin::CFtlPlugin(void)
{
}

CFtlPlugin::~CFtlPlugin(void)
{
}

BOOL CFtlPlugin::GetFilterString(LPTSTR pszBuf, LONG* pLen)
{
    HRESULT hr = S_OK;
    COM_VERIFY(StringCchCopy(pszBuf, *pLen, 
        TEXT("iOS Log Files (*.log)|*.log|")
        TEXT("Fast Trace Log Files (*.ftl)|*.ftl")
        ));
    *pLen = _tcslen(pszBuf);
    return TRUE;
}

DWORD CFtlPlugin::GetHandleScore(LPCTSTR pszLogPath)
{
    return 0;
}

BOOL CFtlPlugin::ReadLog(LPCTSTR pszLogPath, LogItemsContainer& logItems)
{
    return FALSE;
}

BOOL CFtlPlugin::WriteLog(LPCTSTR pszLogPath, const LogItemsContainer& logItems)
{
    return FALSE;
}

LONG CFtlPlugin::Release()
{
    delete this;
    return 0;
}


extern "C" LOG_FILTER_API 
ILogFilterBase* GetLogFilter()
{
    return new CFtlPlugin();
}
