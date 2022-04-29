#ifndef FTL_SHARE_VARIABLE_HPP
#define FTL_SHARE_VARIABLE_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlSharedVariable.h"
#endif

#include <strsafe.h>

namespace FTL
{
    template<typename T>
    CFSharedVariableT<T>::CFSharedVariableT(
        InitializeSharedVariableProc pInitializeProc /* = NULL */,
        FinalizeSharedVariableProc pFinalizeProc /* = NULL */,
        LPCTSTR pszShareName /* = NULL */)
    {
        DWORD dwShareInfoSize = sizeof(T);
		BOOL bRet = FALSE;

        m_pShareValue = NULL;
        m_hMapping = NULL;
        m_bFirstCreate = FALSE;
		m_isLocalValue = FALSE;

        m_pFinalizeProc = pFinalizeProc;

        TCHAR szMapName[MAX_PATH] = { 0 };
        if (NULL == pszShareName)
        {
            TCHAR szModuleFileName[MAX_PATH] = { 0 };
            GetModuleFileName(NULL, szModuleFileName, _countof(szModuleFileName));
            PathRemoveExtension(szModuleFileName);
            LPCTSTR pszNamePos = PathFindFileName(szModuleFileName); // &szModuleFileName[nLength + 1];
            if (pszNamePos)
            {
                StringCchPrintf(szMapName, _countof(szMapName), TEXT("FTLShare_%s"), pszNamePos);
            }
            pszShareName = szMapName;
        }

		//TODO: refactor for functions(need consider memory info)
		ATL::CSecurityDesc sd;
		ATL::CDacl dacl;
		dacl.AddAllowedAce(ATL::Sids::World(), GENERIC_ALL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);
		sd.SetDacl(dacl);

		SECURITY_ATTRIBUTES sa = { 0 };
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = (LPVOID)sd.GetPSECURITY_DESCRIPTOR();
		sa.bInheritHandle = FALSE;

        m_hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
            0, dwShareInfoSize, pszShareName);
        ATLASSERT(m_hMapping != NULL);
        if (m_hMapping)
        {
            m_bFirstCreate = (GetLastError() == 0); //Not ERROR_ALREADY_EXISTS
            m_pShareValue = reinterpret_cast<T*>
                (::MapViewOfFileEx(m_hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, dwShareInfoSize, NULL));
        }
        ATLASSERT(m_pShareValue);
		if (!m_pShareValue)
		{
			m_pShareValue = new T();
			m_isLocalValue = TRUE;
		}

        if (pInitializeProc)
        {
            if (m_pShareValue)
            {
                pInitializeProc(m_bFirstCreate, *m_pShareValue);
            }
        }
    }

    template<typename T>
    CFSharedVariableT<T>::~CFSharedVariableT()
    {
        if (m_pShareValue)
        {
            if (m_pFinalizeProc)
            {
                m_pFinalizeProc(m_bFirstCreate, *m_pShareValue);
            }

			if (m_isLocalValue)
			{
				delete m_pShareValue;
			} else {
				UnmapViewOfFile(m_pShareValue);
			}
            m_pShareValue = NULL;
        }
        if (m_hMapping)
        {
            CloseHandle(m_hMapping);
            m_hMapping = NULL;
        }
    }

    template<typename T>
    T& CFSharedVariableT<T>::GetShareValue()
    {
        ATLASSERT(m_pShareValue);
        return *m_pShareValue;
    }

}
#endif //FTL_SHARE_VARIABLE_HPP