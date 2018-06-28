#ifndef FTL_STRING_HPP
#define FTL_STRING_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlString.h"
#endif

namespace FTL
{
    int ciStringCompare(const std::string& s1, const std::string& s2)
    {
        //效率比使用 mismatch 或 lexicographical_compare 高
        //return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end()).
        return _stricmp(s1.c_str(), s2.c_str());
    }

    BOOL CFStringUtil::IsMatchMask(LPCTSTR pszName, LPCTSTR pszMask, BOOL bCaseSensitive /* = TRUE */)
    {
        if (!pszMask)
        {
            return TRUE;
        }
        //设置比较函数的指针(是否区分大小写)
        typedef int (WINAPI *StrCompareFun)(LPCTSTR, LPCTSTR);
        StrCompareFun pCompareFun = bCaseSensitive ? lstrcmp : lstrcmpi;

        if (!pszName)
        {
            if (pszMask && pszMask[0] && (*pCompareFun)(pszMask, _T("*")))
            {
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }

        while(*pszName && *pszMask)
        {
            switch(*pszMask) 
            {
            case _T('?'):
                {
                    pszName++;
                    pszMask++;
                }
                break;
            case _T('*'):
                {
                    pszMask++;
                    if (!*pszMask)
                    {
                        return TRUE;
                    }
                    do 
                    {
                        if (IsMatchMask(pszName, pszMask, bCaseSensitive))
                        {
                            return TRUE;
                        }
                        pszName++;
                    } while(*pszName);
                    return FALSE;
                }
                break;
            default:
                {
                    if (bCaseSensitive)
                    {
                        //区分大小写比较
                        if (*pszName != *pszMask)
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        //不区分大小写(_totupper 要求的参数是 int, 必须使用临时变量?)
                        TCHAR tmpName = *pszName;
                        TCHAR tmpMask = *pszMask;
                        if (_totupper(tmpName) != _totupper(tmpMask))
                        {
                            return FALSE;
                        }
                    }
                    pszName++;
                    pszMask++;
                }
            }
        }
        return ((!*pszName && !*pszMask) || (_T('*') == *pszMask));
    }

	INT CFStringUtil::DeleteRepeatCharacter(LPCTSTR pszSrc, LPTSTR pszDest, INT nDestSize, TCHAR szChar, INT nMaxRepeatCount, BOOL bResetCount)
	{
		INT nRepeartSplashCount = 0;
		INT nDestCount = 0;
		ATLASSERT(pszSrc);
		ATLASSERT(pszDest);
		if (!pszSrc || !pszDest)
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return INT(-1);
		}

		while(pszSrc && *pszSrc && nDestCount < nDestSize - 1 ) //last assign +  NULL
		{
			if (szChar != *pszSrc)
			{
				*pszDest++ = *pszSrc++;
				if (bResetCount)
				{
					nRepeartSplashCount = 0;
				}
				nDestCount++;
			}
			else
			{
				if (nRepeartSplashCount >= nMaxRepeatCount)
				{
					pszSrc++;
				}
				else
				{
					*pszDest++ = *pszSrc++;
					nDestCount++;
				}
				nRepeartSplashCount++;
			}
		}

		FTLASSERT(nDestCount <= nDestSize - 1);
		*pszDest = NULL; //*pszSrc;

		return nDestCount;
	}
}

#endif //FTL_STRING_HPP
