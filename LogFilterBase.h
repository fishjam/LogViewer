#ifndef LOG_FILTER_BASE_H
#define LOG_FILTER_BASE_H
#pragma once

#include <Windows.h>


#ifdef LOG_FILTER_EXPORTS
#define LOG_FILTER_API __declspec (dllexport)
#else
#define LOG_FILTER_API __declspec (dllimport)
#endif

#include "LogViewerDefine.h"

#include <ftlbase.h>

class LOG_FILTER_API ILogFilterBase
{
public:
    //返回过滤字符串 "LogViewer Files (*.FTL)|*.FTL|"
    virtual BOOL GetFilterString(LPWSTR pszBuf, LONG* pLen) = 0;

    //根据文件的路径信息(如扩展名)，判断当前Filter是否能处理，返回值越大，说明越能处理，返回0说明不能处理
    virtual DWORD GetHandleScore(LPCWSTR pszLogPath) = 0;

    //
    virtual BOOL ReadLog(LPCWSTR pszLogPath, LogItemsContainer& logItems) = 0;
    virtual BOOL WriteLog(LPCWSTR pszLogPath, const LogItemsContainer& logItems) = 0;

    virtual LONG Release() = 0;
protected:
    virtual ~ILogFilterBase() {};
};

typedef ILogFilterBase* (*GetLogFilterFunc)();

struct LogFilterRelease : public std::unary_function<ILogFilterBase*, void>
{
    void operator()(ILogFilterBase* ptr) const
    {
        ptr->Release();
    }
};

#endif //LOG_FILTER_BASE_H