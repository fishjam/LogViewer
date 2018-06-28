#pragma once

#include "../LogFilterBase.h"

class CFtlPlugin : public ILogFilterBase
{
public:
    CFtlPlugin(void);
    virtual ~CFtlPlugin(void);
public:
    virtual BOOL GetFilterString(LPWSTR pszBuf, LONG* pLen);
    virtual DWORD GetHandleScore(LPCWSTR pszLogPath);
    virtual BOOL ReadLog(LPCWSTR pszLogPath, LogItemsContainer& logItems);
    virtual BOOL WriteLog(LPCWSTR pszLogPath, const LogItemsContainer& logItems);
    virtual LONG Release();
};

