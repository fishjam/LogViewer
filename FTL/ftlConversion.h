#ifndef FTL_CONVERSION_H
#define FTL_CONVERSION_H

#pragma once

#include "ftlDefine.h"

/************************************************************************************************************
* CodePage -- 代码页,是Windows为不同的字符编码方案所分配的一个数字编号。
*   常见的有 CP_ACP(当前系统ANSI代码页)、CP_THREAD_ACP(当前线程的ANSI), CP_UTF8 等
* 组合字符 -- 由一个基础字符(如 e)和一个非空字符构成(如 e 头上的重音标记)组合成读音符号(如重音)
* 
* 字符映射时的参数(dwFlags) 
*   MultiByteToWideChar 时的参数 -- MB_PRECOMPOSED 和 MB_COMPOSITE 互斥
*     尚未确认？：dwFlags 为 0 时，会先尝试转换成 预作字符，如果失败，将尝试转换成组合字符。
*     MB_PRECOMPOSED -- 使用预作字符，即由一个基本字符和一个非空字符组成的字符只有一个单一的字符值
*     MB_COMPOSITE() -- 使用组合字符, 由一个基本字符和一个非空字符组成的字符分别有不同的字符值
*     MB_ERR_INVALID_CHARS -- 如遇到无效的输入字符将失败，GetLastError 为 ERROR_NO_UNICODE_TRANSLATION，
*       TODO:此时返回的结果为0。似乎没有办法直接定位无效字符的位置，只能尝试用二分法来查找？
*       如没有设置这个标记，遇到无效字符时(如DBCS串中发现了头字节而没有有效的尾字节)，无效字符将转换为缺省字符。
*     MB_USEGLYPHCHARS -- 使用象形文字替代控制字符
*
*   WideCharToMultiByte时的参数
*     WC_NO_BEST_FIT_CHARS -- 把不能直接转换成相应多字节字符的Unicode字符转换成lpDefaultChar指定的默认字符。
*        如果设置后，把Unicode转换成多字节字符，然后再转换回来，并不一定得到相同的Unicode字符(使用了默认字符)；
*        但如果不设置会如何？
*     WC_COMPOSITECHECK -- 把合成字符转换成预制的字符
*     WC_ERR_INVALID_CHARS -- 若设置了的话，函数遇到无效字符时失败返回(ERROR_NO_UNICODE_TRANSLATION)，
*        否则函数会自动丢弃非法字符。此选项只能用于UTF8
*     WC_DISCARDNS -- 转换时丢弃不占空间的字符，与WC_COMPOSITECHECK一起使用
*     WC_SEPCHARS(默认) -- 转换时产生单独的字符，与WC_COMPOSITECHECK一起使用
*     WC_DEFAULTCHAR -- 转换时使用默认字符代替例外的字符(最常见的是 '?')，与WC_COMPOSITECHECK一起使用
************************************************************************************************************/

namespace FTL
{
#ifndef DEFAULT_MEMALLOCATOR_FIXED_COUNT
#define DEFAULT_MEMALLOCATOR_FIXED_COUNT 32
#endif 

	//! 方便的管理需要分配的临时内存，在类的构造中分配内存，析构中释放
	//! 是否可能生成小的内存碎片？使用Pool优化？
	enum MemoryAllocType
	{
		matNew,             //使用new分配，使用delete释放，为了方便管理，即使只分配一个，也使用数组方式
		matVirtualAlloc,    //使用VirtualAlloc直接在进程的地址空间中保留一快内存(既非堆又非栈)，速度快
		matLocalAlloc,      //使用LocalAlloc分配,LocalFree释放,主要是为了兼容老的程序?
		//matMalloca,         //使用 _malloca 在栈上分配,通过 _freea 释放
	};

	template <typename T, UINT DefaultFixedCount = DEFAULT_MEMALLOCATOR_FIXED_COUNT, MemoryAllocType allocType = matNew>
	class CFMemAllocator
	{
		//和ATL中的 CTempBuffer 模板类比较
		DISABLE_COPY_AND_ASSIGNMENT(CFMemAllocator);
	public:
		FTLINLINE CFMemAllocator();
		FTLINLINE CFMemAllocator(DWORD nCount, BOOL bAlignment = FALSE);
		FTLINLINE ~CFMemAllocator();
		FTLINLINE T* GetMemory(UINT nMaxSize);
		FTLINLINE T* GetMemory();
		FTLINLINE operator T*()
		{
			if (!m_pMem && m_nCount <= DefaultFixedCount)
			{
				return m_FixedMem;
			}
			return m_pMem;
		}

		FTLINLINE T* Detatch();
		FTLINLINE UINT GetCount() const;
	protected:
		FTLINLINE VOID _Init(DWORD nCount, BOOL bAlignment);
		FTLINLINE UINT _AdjustAlignmentSize(UINT nCount);
		FTLINLINE VOID _FreeMemory();
		FTLINLINE UINT _GetBlockSize(UINT nMaxCount);
	private:
		T*              m_pMem;
		T               m_FixedMem[DefaultFixedCount];
		MemoryAllocType m_allocType;
		UINT            m_nCount;
		BOOL            m_bAlignment;
	};

	//! 字符串格式化，可以根据传入的格式化字符长度自动调整，析构时释放分配的内存
	class CFStringFormater
	{
		DISABLE_COPY_AND_ASSIGNMENT(CFStringFormater);
	public:
		//可以分配的最大内存空间为 : dwInitAllocLength * dwMaxBufferTimes(注意：dwMaxBufferTimes 最好是2的倍数)
		FTLINLINE CFStringFormater(DWORD dwMaxBufferLength = MAX_BUFFER_LENGTH);
		FTLINLINE virtual ~CFStringFormater();
		FTLINLINE BOOL Reset(DWORD dwNewLength = 0);
		FTLINLINE VOID Clear();  //清除内容
		FTLINLINE HRESULT __cdecl Format(LPCTSTR lpszFormat, ...);
		FTLINLINE HRESULT __cdecl FormatV(LPCTSTR lpszFormat, va_list argList);
		FTLINLINE HRESULT __cdecl AppendFormat(LPCTSTR lpszFormat, ...);
		FTLINLINE HRESULT __cdecl AppendFormatV(LPCTSTR lpszFormat, va_list argList);

		//各种支持函数的定义
#if 0
		const CFStringFormater& operator=(const CFStringFormater& src);
		const CFStringFormater& operator=(const TCHAR ch);
		const CFStringFormater& operator=(LPCTSTR pstr);
#  ifdef _UNICODE
		const CFStringFormater& CFStringFormater::operator=(LPCSTR lpStr);
		const CFStringFormater& CFStringFormater::operator+=(LPCSTR lpStr);
#  else
		const CFStringFormater& CFStringFormater::operator=(LPCWSTR lpwStr);
		const CFStringFormater& CFStringFormater::operator+=(LPCWSTR lpwStr);
#  endif
		CFStringFormater operator+(const CFStringFormater& src) const;
		CFStringFormater operator+(LPCTSTR pstr) const;
		const CFStringFormater& operator+=(const CFStringFormater& src);
		const CFStringFormater& operator+=(LPCTSTR pstr);
		const CFStringFormater& operator+=(const TCHAR ch);
		TCHAR operator[] (int nIndex) const;
		bool operator == (LPCTSTR str) const;
		bool operator != (LPCTSTR str) const;
		bool operator <= (LPCTSTR str) const;
		bool operator <  (LPCTSTR str) const;
		bool operator >= (LPCTSTR str) const;
		bool operator >  (LPCTSTR str) const;
#endif 

		FTLINLINE operator LPCTSTR() const
		{
			return m_pBuf;
		}
		FTLINLINE LPCTSTR GetString() const;
		FTLINLINE LPTSTR GetString();
		FTLINLINE LONG  GetStringLength() const;
		FTLINLINE LONG  GetSize() const;
		FTLINLINE LPTSTR Detach();
	protected:
        TCHAR   m_szInitBuf[DEFAULT_BUFFER_LENGTH];
		LPTSTR  m_pBuf;
		DWORD   m_dwTotalBufferLength;
		const DWORD m_dwMaxBufferLength;
        FTLINLINE HRESULT __cdecl _InnerFormatV(LPTSTR pszBuf, DWORD dwBufLength, BOOL bAppend, LPCTSTR lpszFormat, va_list argList);
        FTLINLINE VOID _CheckAndReleaseBuf();
	};

	class CFConversion
	{
	public:
		FTLINLINE CFConversion(UINT CodePage = CP_ACP, DWORD dwFlags = 0);
		FTLINLINE ~CFConversion();
		//FTLINLINE BOOL IsUsedDefaultChar() { return m_bUsedDefaultChar; }
		FTLINLINE VOID SetDefaultCharForWC2MB(LPSTR pszDefaultChar);

		FTLINLINE LPWSTR UTF8_TO_UTF16( LPCSTR szUTF8 , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPSTR  UTF8_TO_MBCS( LPCSTR szUTF8 , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPWSTR MBCS_TO_UTF16( LPCSTR szMBCS , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPSTR  MBCS_TO_UTF8( LPCSTR szMBCS , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPSTR  UTF16_TO_MBCS( LPCWSTR szUTF16 , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPSTR  WCS_TO_MBCS( const UINT CodePage, LPCWSTR szWcs , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE PWSTR  MBCS_TO_WCS( const UINT CodePage, LPCSTR szMBCS , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPSTR  UTF16_TO_UTF8( LPCWSTR szUTF16 , INT* pLength = NULL, BOOL bDetached = FALSE);

#ifdef _UNICODE
#define UTF8_TO_TCHAR	UTF8_TO_UTF16
#define TCHAR_TO_UTF8	UTF16_TO_UTF8
		FTLINLINE LPCTSTR UTF16_TO_TCHAR( LPCTSTR lp , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPCWSTR TCHAR_TO_UTF16( LPCTSTR lp , INT* pLength = NULL, BOOL bDetached = FALSE);

#define MBCS_TO_TCHAR	MBCS_TO_UTF16
#define TCHAR_TO_MBCS	UTF16_TO_MBCS

#else
#define UTF8_TO_TCHAR	UTF8_TO_MBCS
#define UTF16_TO_TCHAR	UTF16_TO_MBCS
#define TCHAR_TO_UTF8	MBCS_TO_UTF8
#define TCHAR_TO_UTF16	MBCS_TO_UTF16

		FTLINLINE LPCTSTR MBCS_TO_TCHAR( LPCTSTR lp , INT* pLength = NULL, BOOL bDetached = FALSE);
		FTLINLINE LPCTSTR TCHAR_TO_MBCS( LPCTSTR lp , INT* pLength = NULL, BOOL bDetached = FALSE);
#endif
	private:
		UINT    m_codePage;
		DWORD	m_dwFlags;

		//WideCharToMultiByte 时使用的参数 -- 注意：CodePage 为 CP_UTF7 和 CP_UTF8 时，这两个参数必须是NULL，否则会失败
		LPSTR   m_pDefaultChar;		//遇到一个不能转换的宽字符，函数便会使用pDefaultChar参数指向的字符
		//BOOL    m_bUsedDefaultChar;	//(UTF-8时会失败)至少有一个字符不能转换为其多字节形式，函数就会把这个变量设为TRUE
		CFMemAllocator<BYTE> m_Mem;
		CFMemAllocator<BYTE> m_Mem2;
	};

	class CFConvUtil
	{
	public:
		//return True or False
		FTLINLINE static LPCTSTR GetVariantBoolString(VARIANT_BOOL varBool);

		//ex. pb = {255,15} -> sz = {"FF,0F"} -- TODO: AtlHexDecode/AtlHexEncode
		FTLINLINE static BOOL HexFromBinary(__in const BYTE* pBufSrc, __in LONG nSrcLen, 
			__out LPTSTR pBufDest, __inout LONG* pDestCharCount, 
			__in TCHAR chDivision = _T('\0'), __in BOOL bCapital = FALSE );
	};
}

#endif //FTL_CONVERSION_H

#ifndef USE_EXPORT
# include "ftlConversion.hpp"
#endif //USE_EXPORT