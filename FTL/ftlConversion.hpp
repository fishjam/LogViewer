#ifndef FTL_CONVERSION_HPP
#define FTL_CONVERSION_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlConversion.h"
#endif


namespace FTL
{
	CFConversion::CFConversion(UINT CodePage /* = CP_ACP */, DWORD dwFlags /* = 0 */)
		:m_codePage(CodePage)
		,m_dwFlags(dwFlags)
	{
		m_pDefaultChar = NULL;
		//m_bUsedDefaultChar = FALSE;
	}
	CFConversion::~CFConversion()
	{
		SAFE_DELETE_ARRAY(m_pDefaultChar);
	}

	VOID CFConversion::SetDefaultCharForWC2MB(LPSTR pszDefaultChar)
	{
		FTLASSERT(pszDefaultChar != m_pDefaultChar);

		SAFE_DELETE_ARRAY(m_pDefaultChar);
		if (pszDefaultChar)
		{
			int nLength = (int)strlen(pszDefaultChar);
			m_pDefaultChar = new CHAR[nLength + 1];
			if (m_pDefaultChar)
			{
                lstrcpynA(m_pDefaultChar, pszDefaultChar, nLength);
				//strncpy(m_pDefaultChar, pszDefaultChar, nLength);
				m_pDefaultChar[nLength] = NULL;
			}
		}
	}
	LPWSTR CFConversion::UTF8_TO_UTF16(LPCSTR szUTF8 , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
		BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( CP_UTF8, m_dwFlags,  szUTF8, -1, NULL, 0 );
        if (pLength)
        {
            *pLength = wcharsize - 1;
        }
		int size = wcharsize * sizeof( WCHAR );
		API_VERIFY( 0 != MultiByteToWideChar( CP_UTF8, m_dwFlags,  szUTF8, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );
		if (bDetached)
		{
			return (LPWSTR)m_Mem.Detatch();
		}
		return ( LPWSTR )( BYTE* )m_Mem;
	}

	LPSTR  CFConversion::UTF8_TO_MBCS( LPCSTR szUTF8 , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( CP_UTF8, m_dwFlags,  szUTF8, -1, NULL, 0 );
		int size = wcharsize * sizeof( WCHAR );
		API_VERIFY( 0 != MultiByteToWideChar( CP_UTF8, m_dwFlags,  szUTF8, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );

		int charsize = 0;
		API_VERIFY((charsize = WideCharToMultiByte( m_codePage, m_dwFlags, ( LPWSTR )( BYTE* )m_Mem, -1, NULL, 0, 
			m_pDefaultChar, NULL)) != 0);
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		size = charsize * sizeof( char );
		API_VERIFY( 0 != WideCharToMultiByte( m_codePage, m_dwFlags, ( LPWSTR )( BYTE* )m_Mem, -1, 
			( LPSTR )m_Mem2.GetMemory( size ), size, m_pDefaultChar, NULL ) ); // NS
		if (bDetached)
		{
			return (LPSTR)m_Mem2.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem2;
	}

    LPWSTR CFConversion::MBCS_TO_UTF16( LPCSTR szMBCS , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
		BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, NULL, 0 );
        if (pLength)
        {
            *pLength = wcharsize - 1;
        }
		int size = wcharsize * sizeof( WCHAR );
		API_VERIFY(0 != MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );

		if (bDetached)
		{
			return (LPWSTR)m_Mem.Detatch();
		}
		return ( LPWSTR )( BYTE* )m_Mem;
	}

    LPSTR  CFConversion::MBCS_TO_UTF8( LPCSTR szMBCS , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, NULL, 0 );
		int size = wcharsize * sizeof( WCHAR );
		API_VERIFY( MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );

		int charsize = 0;
        API_VERIFY((charsize = WideCharToMultiByte( CP_UTF8, m_dwFlags, ( LPWSTR )m_Mem.GetMemory(), -1, NULL, 0, 
			m_pDefaultChar, NULL)) != 0); //UTF-8时使用非NULL的m_bUsedDefaultChar会失败
		size = charsize * sizeof( char );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		API_VERIFY( WideCharToMultiByte( CP_UTF8, m_dwFlags, ( LPWSTR )( BYTE* )m_Mem, -1, 
			( LPSTR )m_Mem2.GetMemory( size ), size, m_pDefaultChar, NULL ) ); // NS

		if (bDetached)
		{
			return (LPSTR)m_Mem2.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem2;
	}

    LPSTR  CFConversion::UTF16_TO_MBCS( LPCWSTR szUTF16 , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		BOOL bRet = FALSE;
		int charsize = WideCharToMultiByte( m_codePage, m_dwFlags, szUTF16, -1, NULL, 0, NULL, NULL );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		int size = charsize * sizeof( char );
		API_VERIFY(0 != WideCharToMultiByte( m_codePage, m_dwFlags, szUTF16, -1, 
			( LPSTR )m_Mem.GetMemory( size ), size, m_pDefaultChar, NULL ) );

		if (bDetached)
		{
			return (LPSTR)m_Mem.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem;
	}

    LPSTR  CFConversion::WCS_TO_MBCS( const UINT CodePage, LPCWSTR szWcs, INT* pLength /* = NULL */,  BOOL bDetached /* = FALSE */)
	{
		BOOL bRet = FALSE;
		int charsize = WideCharToMultiByte( CodePage, m_dwFlags, szWcs, -1, NULL, 0, NULL, NULL );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		int size = charsize * sizeof( char );
		API_VERIFY( 0 != WideCharToMultiByte( CodePage, m_dwFlags, szWcs, -1, 
			( LPSTR )m_Mem.GetMemory( size ), size, m_pDefaultChar, NULL ) );

		if (bDetached)
		{
			return (LPSTR)m_Mem.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem;
	}

    LPWSTR  CFConversion::MBCS_TO_WCS( const UINT CodePage, LPCSTR szMBCS , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( CodePage, m_dwFlags,  szMBCS, -1, NULL, 0 );
        if (pLength)
        {
            *pLength = wcharsize - 1;
        }
		int size = wcharsize * sizeof( WCHAR );
		API_VERIFY(0 != MultiByteToWideChar( CodePage, m_dwFlags,  szMBCS, -1, ( LPWSTR )m_Mem.GetMemory( size ), size ));

		if (bDetached)
		{
			return (LPWSTR)m_Mem.Detatch();
		}
		return ( LPWSTR )( BYTE* )m_Mem;
	}

    LPSTR  CFConversion::UTF16_TO_UTF8( LPCWSTR szUTF16, INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
		BOOL bRet = FALSE;
		int charsize = WideCharToMultiByte( CP_UTF8, m_dwFlags, szUTF16, -1, NULL, 0, NULL, NULL );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		int size = charsize * sizeof( char );
		API_VERIFY( 0 != WideCharToMultiByte( CP_UTF8, m_dwFlags, szUTF16, -1, 
			( LPSTR )m_Mem.GetMemory( size ), size, NULL, NULL ) );

		if (bDetached)
		{
			return (LPSTR)m_Mem.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem;
	}

#ifdef _UNICODE
    LPCTSTR CFConversion::UTF16_TO_TCHAR( LPCTSTR lp , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
        int nSrc = lstrlen(lp) + 1;
        if (pLength)
        {
            *pLength = nSrc - 1;
        }
		if (bDetached)
		{
			StringCchCopy((LPTSTR)m_Mem.GetMemory(nSrc * sizeof(TCHAR)), nSrc, lp);
			return (LPCTSTR)m_Mem.Detatch();
		}
		else
		{
			return lp;
		}
	}
    LPCWSTR CFConversion::TCHAR_TO_UTF16( LPCTSTR lp, INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
        int nSrc = lstrlen(lp) + 1;
        if (pLength)
        {
            *pLength = nSrc - 1;
        }
        if (bDetached)
		{
			StringCchCopy((LPTSTR)m_Mem.GetMemory(nSrc * sizeof(TCHAR)), nSrc, lp);
			return (LPCTSTR)m_Mem.Detatch();
		}
		else
		{
			return lp;
		}
	}
#else	//Non _UNICODE
    LPCTSTR CFConversion::MBCS_TO_TCHAR( LPCTSTR lp , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
        int nSrc = lstrlen(lp) + 1;
        if (pLength)
        {
            *pLength = nSrc - 1;
        }
		if (bDetached)
		{
			StringCchCopy((LPTSTR)m_Mem.GetMemory(nSrc * sizeof(TCHAR)), nSrc, lp);
			return (LPCTSTR)m_Mem.Detatch();
		}
		else
		{
			return lp;
		}
	}
    LPCTSTR CFConversion::TCHAR_TO_MBCS( LPCTSTR lp , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
        int nSrc = lstrlen(lp) + 1;
        if (pLength)
        {
            *pLength = nSrc - 1;
        }
		if (bDetached)
		{
			StringCchCopy((LPTSTR)m_Mem.GetMemory(nSrc * sizeof(TCHAR)), nSrc, lp);
			return (LPCTSTR)m_Mem.Detatch();
		}
		else
		{
			return lp;
		}
	}
#endif //_UNICODE

    BOOL CFConvUtil::HexFromBinary(__in const BYTE* pBufSrc, __in LONG nSrcLen, 
        __out LPTSTR pBufDest, __inout LONG* pDestCharCount, 
        __in TCHAR chDivision/* = _T('\0') */,
		__in BOOL bCapital /* = FALSE */)
    {
        FTLASSERT(pBufSrc);
        FTLASSERT(pDestCharCount);

        //BOOL bRet = FALSE;

        if (!pBufDest)
        {
			if (pDestCharCount)
			{
				*pDestCharCount = nSrcLen * 2 + 1;
				if (chDivision != _T('\0'))
				{
					*pDestCharCount += (nSrcLen - 1);
				}
			}
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return FALSE;
        }

        static const char s_chHexLowerChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
            'a', 'b', 'c', 'd', 'e', 'f'};
		static const char s_chHexCapitalChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
			'A', 'B', 'C', 'D', 'E', 'F'};

		const char* pHexChars = bCapital ? &s_chHexCapitalChars[0] : &s_chHexLowerChars[0];
        LONG nRead = 0;
        LONG nWritten = 0;
        BYTE ch = 0;
        LPTSTR pWritePos = pBufDest;
        while (nRead < nSrcLen)
        {
            ch = *pBufSrc++;
            nRead++;
            *pWritePos++ = pHexChars[(ch >> 4) & 0x0F];
            *pWritePos++ = pHexChars[ch & 0x0F];
            nWritten += 2;
            if (chDivision != _T('\0'))
            {
                *pWritePos++ = chDivision;
                nWritten ++;
            }
        }
        if (pWritePos != pBufDest)
        {
            *(pWritePos - 1) = NULL;  //set last NULL 
        }

        *pDestCharCount = nWritten;

        return TRUE;
    }

}

#endif //FTL_CONVERSION_HPP