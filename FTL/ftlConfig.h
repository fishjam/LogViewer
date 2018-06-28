#ifndef FTL_CONFIG_H
#define FTL_CONFIG_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlConfig.h requires ftlbase.h to be included first
#endif

namespace FTL
{
	//TODO: 建立配置文件的体系
	class CFConfig{
	public:
	};

	class CFConfigRegistry : public CFConfig{

	};

	/****************************************************************************************************************
	* INI 配置文件:
	*   TODO:注释方式?
	* 
	* ExpandEnvironmentStrings  -- 函数用于替换环境变量中的可变字符串
	* ExpandEnvironmentStringsForUser
	****************************************************************************************************************/
	class CFConfigIniFile : public CFConfig
	{
	public:
		FTLINLINE CFConfigIniFile();
		FTLINLINE virtual ~CFConfigIniFile();
		FTLINLINE BOOL    EnsureIniFileExist();

		FTLINLINE CString GetFilePathName();
		FTLINLINE BOOL    SetDefaultFilePathName();
		FTLINLINE BOOL    SetFileName(LPCTSTR lpFileName);
		FTLINLINE BOOL    IsIniFileExist();
	public:
		FTLINLINE DWORD GetKeyNames(CString strSectionName, CSimpleArray<CString> &strArray);

		FTLINLINE UINT GetMaxSize_A_Section();
		FTLINLINE UINT GetMaxSize_All_SectionNames();

		FTLINLINE DWORD GetKeyNamesValues(CString strSectionName,CSimpleArray<CString> &strArray);
		FTLINLINE BOOL  GetStruct(LPCTSTR lpSectionName,LPCTSTR lpKeyName,LPVOID lpStruct,UINT uSizeStruct);
		FTLINLINE DWORD GetSectionNames(CSimpleArray<CString> &strArray );

		FTLINLINE BOOL  DeleteStruct(LPCTSTR lpSectionName, LPCTSTR lpKeyName);
		FTLINLINE BOOL  DeleteSection(CString strSectionName);
		FTLINLINE BOOL  DeleteString(CString strSectionName,CString strKeyName);

		FTLINLINE BOOL  ModifyString(LPCTSTR lpSectionName,LPCTSTR lpKeyName,LPCTSTR lpString);
		FTLINLINE BOOL  ModifyInt(LPCTSTR lpSectionName, LPCTSTR lpKeyName, int nValue);

		FTLINLINE BOOL  WriteStruct(LPCTSTR lpSectionName,LPCTSTR lpKeyName,LPVOID lpStruct,UINT uSizeStruct);
		FTLINLINE BOOL  WriteSection(LPCTSTR lpSectionName, LPCTSTR lpString);
		FTLINLINE BOOL  WriteLongString(LPCTSTR lpValue,LPCTSTR lpSectionName,LPCTSTR lpKeyName,  ...);

		FTLINLINE BOOL  WriteString(LPCTSTR lpSectionName,LPCTSTR lpKeyName,LPCTSTR lpString);
		FTLINLINE BOOL  WriteInt(LPCTSTR lpSectionName, LPCTSTR lpKeyName, int nValue);

		FTLINLINE DWORD GetString(LPCTSTR lpSectionName,LPCTSTR lpKeyName,LPCTSTR lpDefault,CString &strReturnedString);
		FTLINLINE DWORD GetString(LPCTSTR lpSectionName,LPCTSTR lpKeyName,LPCTSTR lpDefault,LPTSTR lpReturnedString,DWORD nSize);
		FTLINLINE UINT  GetInt(LPCTSTR lpSectionName,LPCTSTR lpKeyName,INT nDefault);

		FTLINLINE void    SetMaxSize_All_SectionNames(UINT nSize);
		FTLINLINE void    SetMaxSize_A_Section(UINT nSize );

	protected:
		FTLINLINE void    InitializeForCreate();

		FTLINLINE DWORD   GetSectionNames(LPTSTR lpszReturnBuffer, DWORD nSize);
		FTLINLINE DWORD   GetSection(LPCTSTR lpSectionName,LPTSTR lpReturnedString, DWORD nSize);

	private:
		CString m_strFileFullName;
		UINT    nMaxSize_All_SectionNames;
		UINT    nMaxSize_A_Section;
	};
}
#endif // FTL_CONFIG_H

#ifndef USE_EXPORT
#  include "ftlConfig.hpp"
#endif 