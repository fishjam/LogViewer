#ifndef FTL_CONVERSION_H
#define FTL_CONVERSION_H

#pragma once

#ifndef FTL_BASE_H
#  error ftlConversion.h requires ftlbase.h to be included first
#endif

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