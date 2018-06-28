#ifndef FTL_STRING_H
#define FTL_STRING_H
#pragma once

#include "ftlDefine.h"
/**************************************************************************************************
*  C运行时本地语言设置(runtime locale setting) -- 如 wcscmp 缺省时仅比较52个不带重音的字母字符，
*  要想使用操作系统的本地设置来比较字符串，必须调用 setlocale(LC_ALL, "");
*  正则表达式 AtlServer 中的 CAtlRegExp
***************************************************************************************************/

namespace FTL
{
    //忽略大小写字符串比较的函数
    FTLINLINE int ciStringCompare(const std::string& s1, const std::string& s2);

    //用于忽略大小写字符串比较的仿函数类 -- 参见 Effective STL 中的条款19
	//示例: 以忽略大小写的方式排序字符串的vector.
	//std::sort(v.begin(),v.end(),CStringCompareI());
    struct CStringCompareI : public std::binary_function<std::string, std::string, bool>
    {
        FTLINLINE bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            return ciStringCompare(lhs, rhs);
        }

    };

	struct CAtlStringCompareI : public std::binary_function<LPCTSTR, LPCTSTR, bool>
	{
		FTLINLINE bool operator()(LPCTSTR& lhs, LPCTSTR& rhs) const
		{
			return StrCmpI(lhs, rhs);
		}
	};

#if 0

	//另外一种忽视大小写的字符串比较类 -- 参见 Effective STL 中的附录 A
	struct lt_str_1 : public std::binary_function<std::string, std::string, bool> {
		struct lt_char {
			const std::ctype<char>& ct;
			lt_char(const std::ctype<char>& c) : ct(c) {}
			bool operator()(char x, char y) const {
				return ct.toupper(x) < ct.toupper(y);
			}
		};
		std::locale loc;
		const std::ctype<char>& ct;
		lt_str_1(const std::locale& L = std::locale::classic())
			: loc(L), ct(std::use facet<std::ctype<char> >(loc)) {}

		bool operator()(const std::string& x, const std::string& y) const{
			return std::lexicographical_compare(x.begin(), x.end(),
				y.begin(), y.end(),
				lt_char(ct));
		}
	};
#endif 

    class CFStringUtil
    {
    public:
        FTLINLINE static BOOL IsMatchMask(LPCTSTR Name, LPCTSTR Mask, BOOL bCaseSensitive = TRUE);
		FTLINLINE static INT  DeleteRepeatCharacter(LPCTSTR pszSrc, LPTSTR pszDest, INT nDestSize, TCHAR szChar, INT nMaxRepeatCount, BOOL bResetCount);
    };
}
#endif //FTL_STRING_H

#ifndef USE_EXPORT
#  include "ftlString.hpp"
#endif