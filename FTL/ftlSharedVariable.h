#ifndef FTL_SHARE_VARIABLE_H
#define FTL_SHARE_VARIABLE_H

#pragma once

#include "ftlDefine.h"

namespace FTL
{
    template<typename T>
    class CFSharedVariableT
    {
    public:
        typedef BOOL(CALLBACK* InitializeSharedVariableProc)(BOOL bFirstCreate, T& rValue);
        typedef BOOL(CALLBACK* FinalizeSharedVariableProc)(BOOL bFirstCreate, T& rValue);

        //pszShareName 如果是NULL，会自动根据 Exe的名字+PID 创建进程相关的共享区，这样同一进程中的各个模块能够共享变量
        FTLINLINE CFSharedVariableT(
            InitializeSharedVariableProc pInitializeProc,
            FinalizeSharedVariableProc pFinalizeProc,
            //注意: 因为 FTL 已经使用了 NULL 的 ShareName 方式创建共享变量, 所以要创建用户自己的共享变量时不能使用 NULL
            LPCTSTR pszShareName);
        FTLINLINE ~CFSharedVariableT();

        FTLINLINE T& GetShareValue();
    private:
        HANDLE		m_hMapping;
        BOOL		m_bFirstCreate;
		BOOL		m_isLocalValue;
        T*			m_pShareValue;
        FinalizeSharedVariableProc	m_pFinalizeProc;
    };
}

#endif //FTL_SHARE_VARIABLE_H

#ifndef USE_EXPORT
#  include "ftlSharedVariable.hpp"
#endif 