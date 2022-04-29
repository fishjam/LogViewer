#ifndef FTL_CONVERSION_HPP
#define FTL_CONVERSION_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlConversion.h"
#endif


namespace FTL
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	CFMemAllocator<T, DefaultFixedCount, allocType>::CFMemAllocator()
		:m_pMem(NULL)
		, m_allocType(allocType)
		, m_nCount(DefaultFixedCount)
		, m_bAlignment(FALSE)
	{
		ZeroMemory(m_FixedMem, sizeof(m_FixedMem));
		_Init(DefaultFixedCount, m_bAlignment);
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	CFMemAllocator<T, DefaultFixedCount, allocType>::CFMemAllocator(DWORD nCount, BOOL bAlignment)
		:m_pMem(NULL)
		, m_allocType(allocType)
		, m_nCount(nCount)
		, m_bAlignment(bAlignment)
	{
		if (0 == nCount)
		{
			nCount = 1;
		}
		ZeroMemory(m_FixedMem, sizeof(m_FixedMem));
		_Init(nCount, m_bAlignment);
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	VOID CFMemAllocator<T, DefaultFixedCount, allocType>::_Init(DWORD nCount, BOOL bAlignment)
	{
		if (bAlignment)
		{
			nCount = _AdjustAlignmentSize(nCount);
		}
		if (nCount > DefaultFixedCount)
		{
			m_nCount = nCount;
			switch (allocType)
			{
			case matNew:
				m_pMem = new T[nCount];
				ZeroMemory(m_pMem, sizeof(T) * nCount); //先清除内存，保证没有垃圾数据--是否会造成类指针问题？性能影响？
				break;
			case matVirtualAlloc:
				m_pMem = (T*)VirtualAlloc(NULL, sizeof(T) * nCount, MEM_COMMIT, PAGE_READWRITE);
				break;
			case matLocalAlloc:
				m_pMem = (T*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(T) * nCount);
				break;
			default:
				ATLASSERT(FALSE);
				break;
			}
		}
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	UINT CFMemAllocator<T, DefaultFixedCount, allocType>::_AdjustAlignmentSize(UINT nCount)
	{
		static UINT s_nAlignmentSize[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
			2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, //1048576 = 1024 * 1024
			2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912, 1073741824, //1073741824 = 1024 * 1024 * 1024
			2147483648, UINT_MAX
		};
		INT nIndex = 0;
		while (nCount < s_nAlignmentSize[nIndex] && nIndex < _countof(s_nAlignmentSize))
		{
			nIndex++;

		}
		if (nIndex < _countof(s_nAlignmentSize))
		{
			nCount = s_nAlignmentSize[nIndex];
		}
		return nCount;
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	VOID CFMemAllocator<T, DefaultFixedCount, allocType>::_FreeMemory()
	{
		//BOOL bRet = TRUE;
		if (m_pMem)
		{
			switch (allocType)
			{
			case matNew:
				SAFE_DELETE_ARRAY(m_pMem);
				break;
			case matVirtualAlloc:
				ATLVERIFY(VirtualFree(m_pMem, 0, MEM_RELEASE));
				m_pMem = NULL;
				break;
			case matLocalAlloc:
				LocalFree((HLOCAL)m_pMem);
				m_pMem = NULL;
				break;
			default:
				ATLASSERT(FALSE);
				break;
			}
		}
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	CFMemAllocator<T, DefaultFixedCount, allocType>::~CFMemAllocator()
	{
		_FreeMemory();
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	T* CFMemAllocator<T, DefaultFixedCount, allocType>::Detatch()
	{
		T* pTmpMem = m_pMem;
		m_pMem = NULL;
		m_nCount = DefaultFixedCount;
		return pTmpMem;
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	T* CFMemAllocator<T, DefaultFixedCount, allocType>::GetMemory(UINT nMaxSize)
	{
		if (!m_pMem && nMaxSize <= DefaultFixedCount)
		{
			return m_FixedMem;
		}

		if (m_nCount < nMaxSize || NULL == m_pMem)
		{
			_FreeMemory();
			m_nCount = _GetBlockSize(nMaxSize);
			_Init(m_nCount, m_bAlignment);
		}
		return m_pMem;
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	T* CFMemAllocator<T, DefaultFixedCount, allocType>::GetMemory()
	{
		if (!m_pMem && m_nCount <= DefaultFixedCount)
		{
			return m_FixedMem;
		}
		return m_pMem;
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	UINT CFMemAllocator<T, DefaultFixedCount, allocType>::GetCount() const
	{
		return m_nCount;
	}

	template <typename T, UINT DefaultFixedCount/* = DEFAULT_MEMALLOCATOR_FIXED_COUNT*/, MemoryAllocType allocType  /*= matNew*/>
	UINT CFMemAllocator<T, DefaultFixedCount, allocType>::_GetBlockSize(UINT nMaxCount)
	{
		UINT TempSize = m_nCount;
		while (TempSize < nMaxCount)
		{
			TempSize *= 2;
		}
		return TempSize;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CFStringFormater::CFStringFormater(DWORD dwMaxBufferLength /* = DEFAULT_BUFFER_LENGTH */)
		: m_dwMaxBufferLength(dwMaxBufferLength)
	{
        ZeroMemory(m_szInitBuf, sizeof(m_szInitBuf));
		m_pBuf = m_szInitBuf;
        m_dwTotalBufferLength = _countof(m_szInitBuf);

        ATLASSERT(m_dwMaxBufferLength >= m_dwTotalBufferLength);  //不要传的太小,否则没有意义
	}
	CFStringFormater::~CFStringFormater()
	{
        _CheckAndReleaseBuf();
	}

	BOOL CFStringFormater::Reset(DWORD dwNewLength /* = 0 */)
	{
		ATLASSERT(dwNewLength >= 0);
        if (dwNewLength == m_dwTotalBufferLength)
        {
            //大小一样,清空后返回
            ZeroMemory(m_pBuf, sizeof(TCHAR) * m_dwTotalBufferLength);
            return TRUE;
        }
        
        if (dwNewLength <= _countof(m_szInitBuf))
        {
            //比固定空间小, 恢复成原状
            _CheckAndReleaseBuf();
            ZeroMemory(m_szInitBuf, sizeof(m_szInitBuf));
        }
		else //比固定空间大
		{
            LPTSTR pszBuf = new TCHAR[dwNewLength];
            if (!pszBuf)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            //释放已有内存并重新分配指定大小的
            _CheckAndReleaseBuf();

            ZeroMemory(pszBuf, sizeof(TCHAR) * dwNewLength);
            m_pBuf = pszBuf;
            m_dwTotalBufferLength = dwNewLength;
		}
        return TRUE;
	}

	VOID CFStringFormater::Clear()
	{
		if (m_pBuf)
		{
			//清0,保证没有垃圾数据
			ZeroMemory(m_pBuf, m_dwTotalBufferLength);
		}
	}

    HRESULT CFStringFormater::_InnerFormatV(LPTSTR pszBuf, DWORD dwBufLength, BOOL bAppend, LPCTSTR lpszFormat, va_list argList)
    {
        HRESULT hr = E_FAIL;

        LPTSTR pszDestEnd = NULL;
        size_t cchRemaining = 0;
        DWORD dwFlags = 0;
        //STRSAFE_FILL_ON_FAILURE | STRSAFE_FILL_BEHIND_NULL; //失败时全部填充NULL，成功时在最后填充NULL

        int nOldStringLen = 0;
        if (bAppend)
        {
            nOldStringLen = static_cast<DWORD>(_tcslen(m_pBuf));  //计算并保留原来的长度,否则一旦 printf 以后,再计算长度就不正确了
        }
        ZeroMemory(pszBuf, sizeof(TCHAR) * dwBufLength);
        hr = StringCchVPrintfEx(pszBuf, dwBufLength, &pszDestEnd, &cchRemaining, dwFlags, lpszFormat, argList);
        if (SUCCEEDED(hr))
        {
            // 确保最后的结束符，但实际上已经由 StringCchVPrintfEx 完成
            //m_pBuf[dwLength - 1] = TEXT('\0');
        }
        else if (STRSAFE_E_INSUFFICIENT_BUFFER == hr)
        {
            //内存不足,计算需要的长度, 并分配新的内存
            DWORD dwNeedCount = static_cast<DWORD>(_vscwprintf(lpszFormat, argList) + 1); // include last '\0'
            //ATLASSERT(dwNeedCount > dwBufLength);
            dwNeedCount += nOldStringLen;

            //通过对齐的方式依次二倍扩展.
            DWORD dwLengthAllignment = 1;
            while (dwLengthAllignment < dwNeedCount)
            {
                dwLengthAllignment <<= 1;
            }

            dwNeedCount = FTL_MIN(dwLengthAllignment, m_dwMaxBufferLength);
            if (dwNeedCount > m_dwTotalBufferLength)  //需要的空间比当前的空间大,才需要重新分配内存,否则不再继续处理
            {
                LPTSTR pszNewBuf = new TCHAR[dwNeedCount];
                if (pszNewBuf)
                {
                    ZeroMemory(pszNewBuf, sizeof(TCHAR) * dwNeedCount);
                    if (bAppend && nOldStringLen > 0)
                    {
                        //将原有的字符串拷贝到新的buf中
                        hr = ::StringCchCopyN(pszNewBuf, nOldStringLen + 1, m_pBuf, nOldStringLen);
                        ATLASSERT(SUCCEEDED(hr));
                    }

                    //继续进行 format
                    hr = StringCchVPrintfEx(pszNewBuf + nOldStringLen, dwNeedCount - nOldStringLen, &pszDestEnd, &cchRemaining,
                        dwFlags, lpszFormat, argList);
                    ATLASSERT(SUCCEEDED(hr));

                    if (SUCCEEDED(hr))
                    {
                        _CheckAndReleaseBuf();
                        m_pBuf = pszNewBuf;
                        m_dwTotalBufferLength = dwNeedCount;
                    }
                }
                else
                { //NULL == m_pBuf
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
        }
        else {
            //还会出什么错误? 比如 格式化错误? 那原样返回
            ATLASSERT(FALSE);
        }

        return hr;
    }

    VOID CFStringFormater::_CheckAndReleaseBuf()
    {
        if (m_pBuf && m_pBuf != m_szInitBuf)
        {
            delete[] m_pBuf;
            m_pBuf = m_szInitBuf;
            m_dwTotalBufferLength = _countof(m_szInitBuf);
        }
    }
	HRESULT CFStringFormater::Format(LPCTSTR lpszFormat, ...)
	{
		ATLASSERT(lpszFormat);
		if (NULL == lpszFormat)
		{
			return E_INVALIDARG;
		}

		HRESULT hr = E_FAIL;

		va_list pArgs;
		va_start(pArgs, lpszFormat);
		hr = (FormatV(lpszFormat, pArgs));
		ATLASSERT(SUCCEEDED(hr));
		va_end(pArgs);

		return hr;
	}

	HRESULT CFStringFormater::FormatV(LPCTSTR lpszFormat, va_list argList)
	{
		HRESULT hr = E_FAIL;
        hr = _InnerFormatV(m_pBuf, m_dwTotalBufferLength, FALSE, lpszFormat, argList);
		return hr;
	}

	HRESULT CFStringFormater::AppendFormat(LPCTSTR lpszFormat, ...)
	{
		ATLASSERT(lpszFormat);
		if (NULL == lpszFormat)
		{
			return E_INVALIDARG;
		}
		//CHECK_POINTER_RETURN_VALUE_IF_FAIL(lpszFormat, E_INVALIDARG);
		HRESULT hr = E_FAIL;

		va_list pArgs;
		va_start(pArgs, lpszFormat);
		hr = AppendFormatV(lpszFormat, pArgs);
		ATLASSERT(SUCCEEDED(hr));
		va_end(pArgs);
		return hr;
	}

	HRESULT CFStringFormater::AppendFormatV(LPCTSTR lpszFormat, va_list argList)
	{
		HRESULT hr = E_FAIL;
		DWORD dwOldStringLen = static_cast<DWORD>(_tcslen(m_pBuf));
        hr = _InnerFormatV(m_pBuf + dwOldStringLen, m_dwTotalBufferLength - dwOldStringLen, TRUE, lpszFormat, argList);
        return hr;
	}

	LPCTSTR CFStringFormater::GetString() const
	{
		return m_pBuf;
	}
	LPTSTR CFStringFormater::GetString()
	{
		return m_pBuf;
	}
	LONG CFStringFormater::GetStringLength() const
	{
		return (lstrlen(m_pBuf));
	}
	LONG CFStringFormater::GetSize() const
	{
		return (LONG)(m_dwTotalBufferLength);
	}

	LPTSTR CFStringFormater::Detach()
	{
        HRESULT hr = E_FAIL;
        if (m_pBuf == m_szInitBuf)
        {
            //没有分配过内存, 则分配内存并拷贝字符串, 这样外部可以采用统一的 delete [] 方式释放
            int nLen = (int)lstrlen(m_pBuf);
            LPTSTR pBuf = new TCHAR[nLen + 1];
            if (pBuf)
            {
                hr = StringCchCopy(pBuf, nLen + 1, m_pBuf);
                ATLASSERT(SUCCEEDED(hr));
                m_pBuf = pBuf;
            }
        }

		LPTSTR pBuf = m_pBuf;
        m_pBuf = m_szInitBuf;
        m_dwTotalBufferLength = _countof(m_szInitBuf);
		return pBuf;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
		ATLASSERT(pszDefaultChar != m_pDefaultChar);

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
		bRet = ( 0 != MultiByteToWideChar( CP_UTF8, m_dwFlags,  szUTF8, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );
		ATLASSERT(bRet);
		if (bDetached)
		{
			return (LPWSTR)m_Mem.Detatch();
		}
		return ( LPWSTR )( BYTE* )m_Mem;
	}

	LPSTR  CFConversion::UTF8_TO_MBCS( LPCSTR szUTF8 , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		//BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( CP_UTF8, m_dwFlags,  szUTF8, -1, NULL, 0 );
		int size = wcharsize * sizeof( WCHAR );
		ATLVERIFY( 0 != MultiByteToWideChar( CP_UTF8, m_dwFlags,  szUTF8, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );

		int charsize = 0;
		ATLVERIFY((charsize = WideCharToMultiByte( m_codePage, m_dwFlags, ( LPWSTR )( BYTE* )m_Mem, -1, NULL, 0, 
			m_pDefaultChar, NULL)) != 0);

        if (pLength)
        {
            *pLength = charsize - 1;
        }
		size = charsize * sizeof( char );
		ATLVERIFY( 0 != WideCharToMultiByte( m_codePage, m_dwFlags, ( LPWSTR )( BYTE* )m_Mem, -1, 
			( LPSTR )m_Mem2.GetMemory( size ), size, m_pDefaultChar, NULL ) ); // NS
		if (bDetached)
		{
			return (LPSTR)m_Mem2.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem2;
	}

    LPWSTR CFConversion::MBCS_TO_UTF16( LPCSTR szMBCS , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
		//BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, NULL, 0 );
        if (pLength)
        {
            *pLength = wcharsize - 1;
        }
		int size = wcharsize * sizeof( WCHAR );
		ATLVERIFY(0 != MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );
		if (bDetached)
		{
			return (LPWSTR)m_Mem.Detatch();
		}
		return ( LPWSTR )( BYTE* )m_Mem;
	}

    LPSTR  CFConversion::MBCS_TO_UTF8( LPCSTR szMBCS , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		//BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, NULL, 0 );
		int size = wcharsize * sizeof( WCHAR );
		ATLVERIFY( MultiByteToWideChar( m_codePage, m_dwFlags,  szMBCS, -1, ( LPWSTR )m_Mem.GetMemory( size ), wcharsize ) );

		int charsize = 0;
		ATLVERIFY((charsize = WideCharToMultiByte( CP_UTF8, m_dwFlags, ( LPWSTR )m_Mem.GetMemory(), -1, NULL, 0,
			m_pDefaultChar, NULL)) != 0); //UTF-8时使用非NULL的m_bUsedDefaultChar会失败
		size = charsize * sizeof( char );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		ATLVERIFY( WideCharToMultiByte( CP_UTF8, m_dwFlags, ( LPWSTR )( BYTE* )m_Mem, -1,
			( LPSTR )m_Mem2.GetMemory( size ), size, m_pDefaultChar, NULL ) ); // NS

		if (bDetached)
		{
			return (LPSTR)m_Mem2.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem2;
	}

    LPSTR  CFConversion::UTF16_TO_MBCS( LPCWSTR szUTF16 , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		//BOOL bRet = FALSE;
		int charsize = WideCharToMultiByte( m_codePage, m_dwFlags, szUTF16, -1, NULL, 0, NULL, NULL );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		int size = charsize * sizeof( char );
		ATLVERIFY(0 != WideCharToMultiByte( m_codePage, m_dwFlags, szUTF16, -1,
			( LPSTR )m_Mem.GetMemory( size ), size, m_pDefaultChar, NULL ) );

		if (bDetached)
		{
			return (LPSTR)m_Mem.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem;
	}

    LPSTR  CFConversion::WCS_TO_MBCS( const UINT CodePage, LPCWSTR szWcs, INT* pLength /* = NULL */,  BOOL bDetached /* = FALSE */)
	{
		//BOOL bRet = FALSE;
		int charsize = WideCharToMultiByte( CodePage, m_dwFlags, szWcs, -1, NULL, 0, NULL, NULL );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		int size = charsize * sizeof( char );
		ATLVERIFY( 0 != WideCharToMultiByte( CodePage, m_dwFlags, szWcs, -1,
			( LPSTR )m_Mem.GetMemory( size ), size, m_pDefaultChar, NULL ) );

		if (bDetached)
		{
			return (LPSTR)m_Mem.Detatch();
		}
		return ( LPSTR )( BYTE* )m_Mem;
	}

    LPWSTR  CFConversion::MBCS_TO_WCS( const UINT CodePage, LPCSTR szMBCS , INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */ )
	{
		//BOOL bRet = FALSE;
		int wcharsize = MultiByteToWideChar( CodePage, m_dwFlags,  szMBCS, -1, NULL, 0 );
        if (pLength)
        {
            *pLength = wcharsize - 1;
        }
		int size = wcharsize * sizeof( WCHAR );
		ATLVERIFY(0 != MultiByteToWideChar( CodePage, m_dwFlags,  szMBCS, -1, ( LPWSTR )m_Mem.GetMemory( size ), size ));

		if (bDetached)
		{
			return (LPWSTR)m_Mem.Detatch();
		}
		return ( LPWSTR )( BYTE* )m_Mem;
	}

    LPSTR  CFConversion::UTF16_TO_UTF8( LPCWSTR szUTF16, INT* pLength /* = NULL */, BOOL bDetached /* = FALSE */)
	{
		//BOOL bRet = FALSE;
		int charsize = WideCharToMultiByte( CP_UTF8, m_dwFlags, szUTF16, -1, NULL, 0, NULL, NULL );
        if (pLength)
        {
            *pLength = charsize - 1;
        }
		int size = charsize * sizeof( char );
		ATLVERIFY( 0 != WideCharToMultiByte( CP_UTF8, m_dwFlags, szUTF16, -1,
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

	LPCTSTR CFConvUtil::GetVariantBoolString(VARIANT_BOOL varBool) {
		switch (varBool)
		{
		case VARIANT_FALSE: return TEXT("False");
		case VARIANT_TRUE: return TEXT("True");
		default:
			ATLASSERT(FALSE && L"Unknown VARIANT_BOOL value");
			break;
		}
		return TEXT("Unknown");
	}

    BOOL CFConvUtil::HexFromBinary(__in const BYTE* pBufSrc, __in LONG nSrcLen, 
        __out LPTSTR pBufDest, __inout LONG* pDestCharCount, 
        __in TCHAR chDivision/* = _T('\0') */,
		__in BOOL bCapital /* = FALSE */)
    {
        ATLASSERT(pBufSrc);
        ATLASSERT(pDestCharCount);

        //BOOL bRet = FALSE;

        if (!pBufDest)
        {
			if (pDestCharCount)
			{
				*pDestCharCount = nSrcLen * 2 + 1;
                //额外的分隔符的空间
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