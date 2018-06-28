#ifndef FTL_CONFIG_HPP
#define FTL_CONFIG_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlConfig.h"
#endif

namespace FTL
{
	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CFConfigIniFile::CFConfigIniFile()
	{
		nMaxSize_All_SectionNames = 1024;
		nMaxSize_A_Section = 1024;
		SetDefaultFilePathName();
	}
	CFConfigIniFile::~CFConfigIniFile()
	{

	}
	CString CFConfigIniFile::GetFilePathName()
	{
		return m_strFileFullName;
	}

	BOOL CFConfigIniFile::SetDefaultFilePathName()
	{
		BOOL bRet = FALSE;
		m_strFileFullName = TEXT("");

		TCHAR exeFullPath[MAX_PATH] = {0};
		API_VERIFY(0 != GetModuleFileName(NULL,exeFullPath,MAX_PATH));
		if (bRet)
		{
			CString strDir(exeFullPath);
			strDir.TrimLeft();
			strDir.TrimRight();

			CString strTemp = strDir.Left(strDir.GetLength() - 3);
			//CString strTemp = strDir.Left(strDir.ReverseFind('.'));
			strDir = strTemp;
			strDir += _T("ini");
			m_strFileFullName = strDir;
		}
		return bRet;
	}

	BOOL CFConfigIniFile::SetFileName(LPCTSTR lpFileName)
	{
		CHECK_POINTER_ISSTRING_PTR_RETURN_VALUE_IF_FAIL(lpFileName, FALSE);

		BOOL bRet = FALSE;
		
		CString strFileName(lpFileName);
		CString strPathName;

		strFileName.TrimLeft();
		strFileName.TrimRight();

		if (strFileName.Find('\\') == -1)
		{
			TCHAR szExeFullPath[MAX_PATH] = {0};

			API_VERIFY(0 != GetModuleFileName(NULL, szExeFullPath, _countof(szExeFullPath)));
			CString strDir(szExeFullPath);
			strDir.TrimLeft();
			strDir.TrimRight();
			int index = strDir.ReverseFind('\\');
			strPathName = strDir.Left(index + 1) + strFileName;
		}
		else{
			strPathName = strFileName;
		}

		CString str;

		if (strPathName.GetLength() > 4)
		{
			str = strPathName.Right(4);
			str.MakeLower();

			if (str == _T(".ini"))
			{
				m_strFileFullName = strPathName;
				return TRUE;
			}
		}

		strPathName += _T(".ini");
		m_strFileFullName = strPathName;

		return bRet;
	}

	BOOL CFConfigIniFile::IsIniFileExist()
	{
		BOOL bRet = FALSE;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA FindFileData = {0};
		hFind = FindFirstFile(m_strFileFullName, &FindFileData);
		if (INVALID_HANDLE_VALUE == hFind)
		{
			bRet = FALSE;
		}
		else
		{
			FindClose(hFind);
			bRet = TRUE;
		}
		return bRet;
	}

	BOOL CFConfigIniFile::EnsureIniFileExist()
	{
		//if (IsIniFileExist())
		// return 1;
		//
		//CFile MyFile;
		//
		//if (!MyFile.Open(m_strFileFullName,CFile::modeCreate))
		// return FALSE;
		//
		//MyFile.Close(); 
		//
		//InitializeForCreate();

		return TRUE;
	}
	BOOL CFConfigIniFile::WriteString(LPCTSTR lpSectionName, 
		LPCTSTR lpKeyName, 
		LPCTSTR lpString)
	{
		return WritePrivateProfileString(lpSectionName,
			lpKeyName,
			lpString,
			m_strFileFullName);
	}
	BOOL CFConfigIniFile::WriteInt(LPCTSTR lpSectionName,
		LPCTSTR lpKeyName,
		int nValue)
	{
		CString strTemp;
		strTemp.Format(_T("%d"),nValue);
		return WritePrivateProfileString(lpSectionName,
			lpKeyName,
			strTemp,
			m_strFileFullName);
	}
	UINT CFConfigIniFile::GetInt(LPCTSTR lpSectionName, 
		LPCTSTR lpKeyName, 
		INT nDefault)
	{
		return GetPrivateProfileInt(lpSectionName,
			lpKeyName,
			nDefault,
			m_strFileFullName);
	}
	DWORD CFConfigIniFile::GetString(LPCTSTR lpSectionName,
		LPCTSTR lpKeyName,
		LPCTSTR lpDefault,
		LPTSTR lpReturnedString,
		DWORD nSize)
	{
		return GetPrivateProfileString(lpSectionName,
			lpKeyName,
			lpDefault,
			lpReturnedString,
			nSize,
			m_strFileFullName);
	}
	DWORD CFConfigIniFile::GetString(LPCTSTR lpSectionName,
		LPCTSTR lpKeyName,
		LPCTSTR lpDefault,
		CString &strReturnedString)
	{

		TCHAR szBuf[256] = {0};
		DWORD len = GetPrivateProfileString(lpSectionName,
			lpKeyName,
			lpDefault,
			szBuf,
			_countof(szBuf),
			m_strFileFullName);
		szBuf[len] = 0;

		strReturnedString.Format(_T("%s"), szBuf); 
		return len;
	}

	BOOL CFConfigIniFile::ModifyInt(LPCTSTR lpSectionName,
		LPCTSTR lpKeyName, 
		int nValue)
	{
		return WriteInt(lpSectionName,lpKeyName,nValue);
	}
	BOOL CFConfigIniFile::ModifyString(LPCTSTR lpSectionName,
		LPCTSTR lpKeyName, 
		LPCTSTR lpString)
	{
		return WriteString(lpSectionName, lpKeyName, lpString);
	}

	BOOL CFConfigIniFile::WriteLongString(LPCTSTR lpValue,
		LPCTSTR lpSectionName,
		LPCTSTR lpKeyName, 
		...)
	{
		// 此函数不允许键名和键值为空的


		va_list pArgList;
		LPCTSTR p;

		if (lpValue == NULL ||
			lpSectionName == NULL ||
			lpKeyName == NULL)
			return FALSE;

		CString strSectionName(lpSectionName);
		CString strKeyName(lpKeyName);

		//BOOL result = FALSE;

		va_start(pArgList,lpKeyName);    /* Initialize variable arguments. */

		do {
			p = va_arg(pArgList,LPCTSTR);
			CString strTempValue1(p);


			if (strTempValue1 == "ListEnd" || 
				strTempValue1 == _T(""))
				break;

			if (!WriteString(strSectionName,strKeyName,strTempValue1))
				return FALSE;

			p = va_arg(pArgList,LPCTSTR);
			CString strTempValue2(p);

			if (strTempValue2 == "ListEnd" || 
				strTempValue2 == _T(""))
				return FALSE;


			strSectionName = strTempValue1;
			strKeyName = strTempValue2;  
		} while(1);

		va_end( pArgList );              /* Reset variable arguments.      */


		if (!WriteString(strSectionName,strKeyName,lpValue))
			return FALSE;

		return TRUE;

	}

	BOOL CFConfigIniFile::DeleteString(CString strSectionName, CString strKeyName)
	{
		return WritePrivateProfileString(strSectionName,strKeyName,NULL,m_strFileFullName);
	}
	BOOL CFConfigIniFile::DeleteSection(CString strSectionName)
	{
		return WritePrivateProfileString(strSectionName,NULL,NULL,m_strFileFullName);

		// return WritePrivateProfileSection(strSectionName,NULL,m_strFileFullName);
		//这种方法会使 section 的标题一并删除

		// return WritePrivateProfileSection(strSectionName,"",m_strFileFullName);
		//这种方法会保留 section 的标题
	}

	BOOL CFConfigIniFile::WriteSection(LPCTSTR lpSectionName, LPCTSTR lpString)
	{
		return WritePrivateProfileSection(lpSectionName,lpString,m_strFileFullName);
	}

	BOOL CFConfigIniFile::WriteStruct(LPCTSTR lpSectionName,
		LPCTSTR lpszKey,
		LPVOID lpStruct,
		UINT uSizeStruct)
	{
		return WritePrivateProfileStruct(lpSectionName,
			lpszKey,
			lpStruct,
			uSizeStruct,
			m_strFileFullName);
	}
	BOOL CFConfigIniFile::DeleteStruct(LPCTSTR lpSectionName,
		LPCTSTR lpszKey)
	{
		return WritePrivateProfileStruct(lpSectionName,lpszKey,NULL,1,m_strFileFullName);
	}

	void CFConfigIniFile::InitializeForCreate()
	{
		//WriteString(_T("administrator"),_T("sa"),_T("sa\r\n"));
		//
		//WriteString(_T("user"),"abc","abc");
		//WriteString("user","ab","ab\r\n");
	}

	DWORD CFConfigIniFile::GetSectionNames(LPTSTR lpszReturnBuffer, DWORD nSize)
	{
		return GetPrivateProfileSectionNames(lpszReturnBuffer,nSize,m_strFileFullName);
	}
	DWORD CFConfigIniFile::GetSectionNames(CSimpleArray<CString> &strArray)
	{
		TCHAR *sz = new TCHAR[nMaxSize_All_SectionNames];
		DWORD dw =  GetSectionNames(sz,nMaxSize_All_SectionNames);

		strArray.RemoveAll();

		TCHAR * index = sz;
		do {
			CString str(index);
			if (str.GetLength() < 1)
			{
				delete []sz;
				return dw;
			}
			strArray.Add(str);

			index = index + str.GetLength() + 1;  
		} while(index && (index < sz + nMaxSize_All_SectionNames));
		delete []sz;

		return dw;
	}

	BOOL CFConfigIniFile::GetStruct(LPCTSTR lpSectionName, 
		LPCTSTR lpszKey, 
		LPVOID lpStruct, 
		UINT uSizeStruct)
	{
		return GetPrivateProfileStruct(lpSectionName,
			lpszKey,
			lpStruct,
			uSizeStruct,
			m_strFileFullName);

	}

	DWORD CFConfigIniFile::GetSection(LPCTSTR lpSectionName,
		LPTSTR lpReturnedString, 
		DWORD nSize)
	{
		return GetPrivateProfileSection(lpSectionName,
			lpReturnedString,
			nSize,
			m_strFileFullName);

	}
	DWORD CFConfigIniFile::GetKeyNamesValues(CString strSectionName, CSimpleArray<CString> &strArray)
	{

		TCHAR *sz = new TCHAR[nMaxSize_A_Section];
		DWORD dw =  GetSection(strSectionName,sz,nMaxSize_A_Section);

		TCHAR * index = sz;

		CString strName,strValue;
		int     nPosition = -1;

		while (index && (index < sz + dw))
		{
			CString str(index);

			if (str.GetLength() < 1)
			{
				delete []sz;
				return dw;
			}

			if ((nPosition = str.Find(_T('='))) == -1)
			{
				DeleteString(strSectionName,str);
				//   continue;
			}
			else
			{
				strName = str.Left(nPosition);
				strValue = str.Mid(nPosition + 1);
				/*
				if (strValue.GetLength() < 1)
				{
				IniFile_DeleteString(strSectionName,strName);
				//    continue;
				}
				*/

				strArray.Add(strName);
				strArray.Add(strValue);
			}

			index = index + str.GetLength() + 1;
		}

		delete []sz;
		return dw;
	}
	void CFConfigIniFile::SetMaxSize_A_Section(UINT nSize)
	{
		nMaxSize_A_Section = nSize;
	}
	void CFConfigIniFile::SetMaxSize_All_SectionNames(UINT nSize)
	{
		nMaxSize_All_SectionNames = nSize;
	}
	UINT CFConfigIniFile::GetMaxSize_All_SectionNames()
	{
		return  nMaxSize_All_SectionNames;
	}
	UINT CFConfigIniFile::GetMaxSize_A_Section()
	{
		return nMaxSize_A_Section;
	}
	DWORD CFConfigIniFile::GetKeyNames(CString strSectionName, CSimpleArray<CString> &strArray)
	{

		TCHAR *sz = new TCHAR[nMaxSize_A_Section];
		DWORD dw =  GetSection(strSectionName,sz,nMaxSize_A_Section);

		TCHAR * index = sz;

		CString strName,strValue;
		int     nPosition = -1;

		while (index && (index < sz + dw))
		{
			CString str(index);

			if (str.GetLength() < 1)
			{
				delete []sz;
				return dw;
			}

			if ((nPosition = str.Find(_T('='))) == -1)
			{
				DeleteString(strSectionName,str);
				//   continue;
			}
			else
			{
				strName = str.Left(nPosition);
			}

			strArray.Add(strName);

			index = index + str.GetLength() + 1;
		}

		delete []sz;
		return dw;
	}
}

#endif //FTL_CONFIG_HPP