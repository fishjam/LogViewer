///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ftlbase.h
/// @brief  Fishjam Template Library Base Header File.
/// @author fujie
/// @version 0.6 
/// @date 03/30/2008
/// @defgroup ftlbase ftl base function and class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FTL_BASE_H
#define FTL_BASE_H
#pragma once

#ifndef __cplusplus
#  error FTL requires C++ compilation (use a .cpp suffix)
#endif

#include "ftlDefine.h"
#include "ftlTypes.h"


//在某些情况下(比如 IE插件), XXX_VERIFY 检测到错误时, _CrtDbgReport 无法弹出 assert 对话框
#ifndef USE_MESSAGEBOX_ERROR
#  define USE_MESSAGEBOX_ERROR 0
#endif 

//#if defined USE_THREAD_SAFE_TRACE
#define FTLTRACE    FAST_TRACE
#define FTLTRACEEX  FAST_TRACE_EX
#define FTLTRACEA   FAST_TRACEA
#define FTLTRACEEX  FAST_TRACE_EX
//#endif //USE_THREAD_SAFE_TRACE

//调用方可以重新指定日志输出和断言的机制，如果没有指定，使用默认(日后扩展为FastTrace？)
#if (!defined FTLASSERT)
#  if defined ATLTRACE
#    define FTLASSERT          ATLASSERT
#  elif defined TRACE
#    define FTLASSERT          ASSERT
#else
#   include <Windows.h>
#   include <tchar.h>
#   include <atlconv.h>
#   include <cassert>
#   define FTLASSERT          assert
#  endif
#endif 

// Turn off warnings for /W4
// To resume any of these warning: #pragma warning(default: 4xxx)
// which should be placed after the FTL include files
//#  pragma warning(disable: 4995)  //'%s': name was marked as #pragma deprecated
//#  pragma warning(disable: 4996)  //'%s': This function or variable may be unsafe.

#define STRSAFE_NO_DEPRECATE
#  include <strsafe.h>
//#pragma warning(default: 4995)
//#pragma warning(default: 4996)

#include <set>
#include <vector>
#include <map>
#include <atlstr.h>

namespace FTL
{
    //#if (!defined FTLTRACE) || (!defined FTLASSERT)
    //#  error must define FTLTRACE and  FTLASSERT, or use ATL/MFC
    //#endif 


    //考虑到字节对其的问题，定义为 8 的倍数
    #define MAX_BUFFER_LENGTH				256
	#define STRINGFORMATER_BUFFER_MAX_DEFAULT_TIMES	8

    //禁止拷贝构造和赋值操作符
    #define DISABLE_COPY_AND_ASSIGNMENT(className)  \
    private:\
        className(const className& ref);\
        className& operator = (const className& ref)

    //在每个类中使用该宏，可以使得测试类定义为产品类的友元类 -- 支持访问私有成员变量
    #define ENABLE_UNIT_TEST(cls) friend class cls##Tester; 

    #ifdef _DEBUG
    #  if USE_MESSAGEBOX_ERROR
    #    define DBG_REPORT(_err, e)    FormatMessageBox(::GetActiveWindow(), TEXT("Error Prompt"), MB_OKCANCEL|MB_ICONERROR|MB_DEFBUTTON1, TEXT("%s(0x%x, %d)\n in %s(%d), Debug Now?\n"), _err.GetConvertedInfo(), e, e, TEXT(__FUNCTION__), __LINE__)
    #  else
    #    define DBG_REPORT(_err, e)    _CrtDbgReportW(_CRT_ASSERT, TEXT(__FILE__), __LINE__, NULL, TEXT("%s(0x%x, %d)"), _err.GetConvertedInfo(), e, e)
    #  endif 
    #  define DBG_BREAK     _CrtDbgBreak
    #else
    #  define DBG_REPORT    __noop
    #  define DBG_BREAK     __noop  
    #endif

    //c -- error convert class, such as FTL::CFAPIErrorInfo
    //e -- error information, such as value return by GetLastError
    //x -- prompt information, such as call code
    #ifdef FTL_DEBUG
    #  define REPORT_ERROR_INFO(c, e, x) \
         do{ \
             c _err(e);\
             USES_CONVERSION;\
             FAST_TRACE_EX(FTL::tlError, TEXT("%s(%d) :\t Error!!! Reason = 0x%08x(%d,%s),Code:\"%s\" \n"),\
               TEXT(__FILE__),__LINE__, e, e, _err.GetConvertedInfo(),TEXT(#x));\
             (1 != DBG_REPORT(_err, e)) || \
               (DBG_BREAK(), 0);\
         }while(0)
    #else //Not Define FTL_DEBUG
    #  define REPORT_ERROR_INFO(c, e, x) __noop
    #endif //FTL_DEBUG


    //自己构造 HRESULT 错误值 -- MAKE_HRESULT(sev,fac,code)
    //  SEVERITY_XXX -- 严重程度(0:成功；1:错误)
    //  FACILITY_XXX -- 设备信息(错误的类型，如 RPC、SECURITY、WIN32 等)
    //可以通过 HRESULT_SEVERITY、HRESULT_FACILITY、HRESULT_CODE 分别获取对应的值

    #ifdef FTL_DEBUG  //调试版本时，获取并判断 LastError，为了防止 REPORT_ERROR_INFO 对LastError产生影响，需要重新设置
    # define ERROR_RETURN_VERIFY(x)\
        FAST_TRACE_EX(FTL::tlDetail, TEXT("%s(%d) :\t ERROR_RETURN_VERIFY call \"%s\" \n"),TEXT(__FILE__),__LINE__, TEXT(#x));\
        dwRet = (x);\
        if(ERROR_SUCCESS != dwRet)\
        {\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, dwRet, x);\
        }

    # define API_ASSERT(x)\
        FAST_TRACE_EX(FTL::tlDetail, TEXT("%s(%d) :\t API_ASSERT call \"%s\" \n"),TEXT(__FILE__),__LINE__, TEXT(#x));\
        if(FALSE == (x))\
        {\
            FTL::CFLastErrorRecovery lastErrorRecory;\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lastErrorRecory.GetError(), x);\
        }

    //FTLTRACEEX(FTL::tlDetail, TEXT("%s(%d) :\t API_VERIFY Call:\"%s\" \n"),
    //    TEXT(__FILE__),__LINE__, TEXT(#x));
    # define API_VERIFY(x)   \
        SetLastError(0); \
        FTLTRACEEX(FTL::tlDetail, TEXT("%s(%d) :\t API_VERIFY call \"%s\" \n"),TEXT(__FILE__),__LINE__, TEXT(#x));\
        bRet = (x);\
        if(FALSE == bRet)\
        {\
            FTL::CFLastErrorRecovery lastErrorRecory;\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lastErrorRecory.GetError(), x);\
        }

    # define API_VERIFY_EXCEPT1(x,e1)\
        SetLastError(0); \
        FTLTRACEEX(FTL::tlDetail, TEXT("%s(%d) :\t API_VERIFY_EXCEPT1 call \"%s\" \n"),TEXT(__FILE__),__LINE__, TEXT(#x));\
        bRet = (x);\
        if(FALSE == bRet)\
        {\
            FTL::CFLastErrorRecovery lastErrorRecory;\
            if(lastErrorRecory.GetError() != e1)\
            {\
                REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lastErrorRecory.GetError(), x);\
            }\
        }
	# define API_VERIFY_EXCEPT2(x,e1,e2)\
        SetLastError(0); \
        FTLTRACEEX(FTL::tlDetail, TEXT("%s(%d) :\t API_VERIFY_EXCEPT2 call \"%s\" \n"),TEXT(__FILE__),__LINE__, TEXT(#x));\
		bRet = (x);\
		if(FALSE == bRet)\
		{\
            FTL::CFLastErrorRecovery lastErrorRecory;\
			if(lastErrorRecory.GetError() != e1 && lastErrorRecory.GetError() != e2)\
			{\
				REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lastErrorRecory.GetError(),x);\
			}\
		}

    #else //没有定义 FTL_DEBUG 的时候 -- 不进行 GetLastError/SetLastError 的调用
    # define ERROR_RETURN_VERIFY(x) dwRet = (x); 
    # define API_ASSERT(x)  
    # define API_VERIFY(x)   \
        bRet = (x);
    # define API_VERIFY_EXCEPT1(x,e1)\
        bRet = (x);
	# define API_VERIFY_EXCEPT2(x,e1,e2)\
		bRet = (x);
    #endif //FTL_DEBUG

    //如果返回 E_FAIL，并且支持 ISupportErrorInfo 的话，需要取得Rich Error 错误信息
    # define COM_VERIFY(x)   \
        hr = (x);\
        if(S_OK != hr)\
        {\
            REPORT_ERROR_INFO(FTL::CFComErrorInfo, hr,x);\
        }

    # define COM_VERIFY_EXCEPT1(x,h1) \
        hr = (x);\
        if(S_OK != hr && (h1)!= hr)\
        {\
            REPORT_ERROR_INFO(FTL::CFComErrorInfo, hr,x);\
        }

    # define COM_VERIFY_EXCEPT2(x,h1,h2) \
        hr = (x);\
        if(S_OK != hr && (h1)!= hr && (h2) != hr)\
        {\
            REPORT_ERROR_INFO(FTL::CFComErrorInfo, hr,x);\
        }

    # define REG_VERIFY(x)   \
        lRet = (x);\
        if(ERROR_SUCCESS != lRet)\
        {\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lRet,x);\
        }

    # define REG_VERIFY_EXCEPT1(x,ret1)   \
        lRet = (x);\
        if(ERROR_SUCCESS != lRet && (ret1) != lRet )\
        {\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lRet,x);\
        }

    ////////////////////////////////////////////////////////////////////////////////////////

    #ifndef _countof
    # define _countof(arr) (sizeof(arr) / sizeof(arr[0]))
    #endif

    #ifndef tstring
    #  if defined UNICODE 
	#     define tstring std::wstring
    #  else
	#     define tstring std::string
    #  endif 
    #endif


    //! @code SAFE_CLOSE_HANDLE(m_hFile,INVALID_HANDLE_VALUE); 注意 NULL 和 INVALID_HANDLE_VALUE
    #ifndef SAFE_CLOSE_HANDLE
    #  ifdef FTL_DEBUG
    #    define SAFE_CLOSE_HANDLE(h,v) if((v) != (h)) { BOOL oldbRet = bRet; API_VERIFY(::CloseHandle((h))); (h) = (v); bRet = oldbRet; }
    #  else
    #    define SAFE_CLOSE_HANDLE(h,v) if((v) != (h)) { ::CloseHandle((h)); (h) = (v); bRet = bRet; }
    #  endif
    #endif

	#ifndef SAFE_CLOSE_INTERNET_HANDLE
	#  ifdef FTL_DEBUG
	#    define SAFE_CLOSE_INTERNET_HANDLE(h) if(NULL != (h)) { BOOL oldbRet = bRet; API_VERIFY(::InternetCloseHandle((h))); (h) = NULL; bRet = oldbRet; }
	#  else
	#    define SAFE_CLOSE_INTERNET_HANDLE(h) if((NULL) != (h)) { ::InternetCloseHandle((h)); (h) = NULL; bRet = bRet; }
	#  endif
	#endif

    #ifndef SAFE_CLOSE_REG
    #  ifdef FTL_DEBUG
    #    define SAFE_CLOSE_REG(h) if(NULL != (h)) { BOOL oldbRet = bRet; API_VERIFY(ERROR_SUCCESS == ::RegCloseKey((h))); (h) = NULL; bRet = oldbRet; }
    #  else
    #    define SAFE_CLOSE_REG(h) if(NULL != (h)) { ::RegCloseKey((h)); (h) = NULL; }
    #  endif
    #endif

    #ifndef SAFE_DELETE
    #  define SAFE_DELETE(p) if( NULL != (p) ){ delete (p); (p) = NULL; }
    #endif

    #ifndef SAFE_DELETE_ARRAY
    #  define SAFE_DELETE_ARRAY(p) if( NULL != (p) ){ delete [] (p); (p) = NULL; }
    #endif

    #ifndef SAFE_FREE
    #  define SAFE_FREE(p)   if(NULL != (p)) { free((p)); (p) = NULL;}
    #endif

    #ifndef SAFE_LOCAL_FREE
    #  define SAFE_LOCAL_FREE(p) if(NULL != (p)) { ::LocalFree((p)); (p) = NULL; }
    #endif

    #ifndef SAFE_HEAP_FREE
    #  define SAFE_HEAP_FREE(p) if(NULL != (p)) { ::HeapFree(GetProcessHeap(),0,(p)); (p) = NULL; }
    #endif

    #ifndef SAFE_FREE_BSTR
    #  define SAFE_FREE_BSTR(s) if(NULL != (s)){ ::SysFreeString((s)); (s) = NULL; }
    #endif

	#ifndef SAFE_ILFREE
	#  define SAFE_ILFREE(p) if(NULL != (p)){ ::ILFree((p)); (p) = NULL; }
	#endif

    /* 另外一种安全Release的方法， 类似的有 Delete
    template<class T> 
    void Release(T t)
    {
        if( t )
        {
            t->Release();
            t = 0;
        }
    }
    */
    #ifndef SAFE_RELEASE
    #  define SAFE_RELEASE(p)  if( NULL != ((p)) ){ (p)->Release(); (p) = NULL; }
    #endif 

	//CoTaskMemFree == CoGetMalloc (MEMCTX_TASK) + IMalloc::Free
	// http://blogs.msdn.com/b/oldnewthing/archive/2004/07/05/173226.aspx
    #ifndef SAFE_COTASKMEMFREE
    #  define SAFE_COTASKMEMFREE(p) if(NULL != (p)){ ::CoTaskMemFree((p)); (p) = NULL; }
    #endif

    #ifndef SAFE_FREE_LIBRARY
    #  define SAFE_FREE_LIBRARY(p) if(NULL != (p)){ ::FreeLibrary(static_cast<HMODULE>(p)); (p) = NULL; }
    #endif


    ////////////////////////////////////////////////////////////////////////////////////////
    //f is CFStringFormater
    //v is combine value(such as GetStyle return value), 
    //c is check type, such as WS_VISIBLE
	//s is append string, such as Visible
    //d is append string, such as "," or "|"
	#ifndef HANDLE_COMBINATION_VALUE_TO_STRING_EX
	#  define HANDLE_COMBINATION_VALUE_TO_STRING_EX(f, v, c, s, d) \
		if(((v) & (c)) == (c))\
		{\
			if(f.GetStringLength() != 0){ \
				f.AppendFormat(TEXT("%s%s"), d, s);\
			}else{\
				f.AppendFormat(TEXT("%s"), s);\
			}\
			(v) &= ~(c);\
		}
	#  define HANDLE_COMBINATION_VALUE_TO_STRING(f, v, c, d)	HANDLE_COMBINATION_VALUE_TO_STRING_EX(f, v, c, TEXT(#c), d) 
	#endif 

	#ifndef HANDLE_CASE_TO_STRING_EX
	# define HANDLE_CASE_TO_STRING_EX(buf,len,c, v)\
			case (c):\
				StringCchCopy(buf,len,v); \
			break;
	#endif
	# define HANDLE_CASE_TO_STRING(buf,len,c) HANDLE_CASE_TO_STRING_EX(buf, len, c, TEXT(#c))

	#ifndef HANDLE_CASE_TO_STRING_FORMATER
	# define HANDLE_CASE_TO_STRING_FORMATER(f, c)\
		case (c):\
		f.Format(TEXT("%s"), TEXT(#c));\
		break;
	#endif 

	#ifndef HANDLE_CASE_RETURN_STRING_EX
	# define HANDLE_CASE_RETURN_STRING_EX(c, v) \
		case (c):\
		return v;
	#endif 

    #ifndef HANDLE_CASE_RETURN_STRING
    # define HANDLE_CASE_RETURN_STRING(c) HANDLE_CASE_RETURN_STRING_EX(c, TEXT(#c))
    #endif 

    #define FTL_MIN(a,b)                (((a) < (b)) ? (a) : (b))
    #define FTL_MAX(a,b)                (((a) > (b)) ? (a) : (b))
	//如果x在 [a,b] 之间，则是x, 否则会是边沿值
	#define FTL_CLAMP(x, a, b)			(FTL_MIN((b), FTL_MAX((a), (x))))
    #define FTL_ABS(a)		            (((a) < 0) ? -(a) : (a))
    #define FTL_SIGN(a)		            (((a) > 0) ? 1 : (((a) < 0) ? -1 : 0))
    #define FTL_INRANGE(low, Num, High) (((low) <= (Num)) && ((Num) <= (High)))


    #define CHECK_POINTER_RETURN_IF_FAIL(p)    \
        if(NULL == (p))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            return;\
        }

    #define CHECK_POINTER_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p)\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            return r;\
        }

    #define CHECK_POINTER_WRITABLE_RETURN_IF_FAIL(p)    \
        if(NULL == p || ::IsBadWritePtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid writable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, sizeof(DWORD_PTR)));\
            return;\
        }

    #define CHECK_POINTER_WRITABLE_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadWritePtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid writable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, sizeof(DWORD_PTR)));\
            return r;\
        }

    #define CHECK_POINTER_READABLE_RETURN_IF_FAIL(p)    \
        if(NULL == p || ::IsBadReadPtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid readable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, sizeof(DWORD_PTR)));\
            return;\
        }

    #define CHECK_POINTER_READABLE_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadReadPtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid readable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, sizeof(DWORD_PTR)));\
            return r;\
        }

    #define CHECK_POINTER_WRITABLE_DATA_RETURN_IF_FAIL(p,len)    \
        if(NULL == p || ::IsBadWritePtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid %d len writable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return;\
        }

    #define CHECK_POINTER_WRITABLE_DATA_RETURN_VALUE_IF_FAIL(p,len,r)    \
        if(NULL == p || ::IsBadWritePtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid %d len writable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return r;\
        }

    #define CHECK_POINTER_READABLE_DATA_RETURN_IF_FAIL(p,len)    \
        if(NULL == p || ::IsBadReadPtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid %d len readable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            return;\
        }

    #define CHECK_POINTER_READABLE_DATA_RETURN_VALUE_IF_FAIL(p,len,r)    \
        if(NULL == p || ::IsBadReadPtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid %d len readable pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            return r;\
        }

    #define CHECK_POINTER_READ_WRIATE_ABLE_DATA_RETURN_IF_FAIL(p,len)    \
        if(NULL == p || ::IsBadReadPtr(p, len) || ::IsBadWritePtr(p,len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid %d len read/write able pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return;\
        }

    #define CHECK_POINTER_READ_WRIATE_ABLE_DATA_RETURN_VALUE_IF_FAIL(p,len,r)    \
        if(NULL == p || ::IsBadReadPtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid %d len read/write able pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return r;\
        }

    #define CHECK_POINTER_ISSTRING_PTR_RETURN_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadStringPtr(p, INFINITE))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid string pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadStringPtr(p, INFINITE));\
            return;\
        }

    #define CHECK_POINTER_ISSTRING_PTR_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadStringPtr(p, INFINITE))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid string pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadStringPtr(p, INFINITE));\
            return r;\
        }

    #define CHECK_POINTER_CODE_RETURN_IF_FAIL(p)    \
        if(NULL == p || ::IsBadCodePtr(p))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid code pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadCodePtr(p));\
            return;\
        }

    #define CHECK_POINTER_CODE_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadCodePtr(p))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("%s(%d) : Warning!!! %s(0x%p) is not a valid code pointer\n"),TEXT(__FILE__),__LINE__, TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadCodePtr(p));\
            return r;\
        }
}

namespace FTL
{
    template<typename T>
    BOOL IsSameNumber(const T& expected, const T& actual, const T& delta);

	template<typename T>
	void SwapValue(T& value1, T& value2);

    //自定义这几个函数，避免在 FTL 中引入 shlwapi.dll 中的函数(对方可能没有这些依赖)
    FTLINLINE LPTSTR FtlPathFindFileName(LPCTSTR pPath);
    FTLINLINE void   FtlPathRemoveExtension(LPTSTR pszPath);

    FTLEXPORT class CFLastErrorRecovery
    {
    public:
        FTLINLINE CFLastErrorRecovery();
        FTLINLINE ~CFLastErrorRecovery();
        DWORD GetError() const { return m_dwLastError; }
    private:
        DWORD   m_dwLastError;
    };


	//在 Exe 和 DLL 中共享变量的内存区域 -- T 必须是简单类型，能支持 CopyMemory, sizeof 等操作
	template<typename T>
	class CFSharedVariableT
	{
	public:
		typedef BOOL (CALLBACK* InitializeSharedVariableProc)(T& rValue);
		typedef BOOL (CALLBACK* FinalizeSharedVariableProc)(T& rValue);

		//pszShareName 如果是NULL，会自动根据 Exe的名字+PID 创建进程相关的共享区，这样同一进程中的各个模块能够共享变量
		FTLINLINE CFSharedVariableT(InitializeSharedVariableProc pInitializeProc,
			FinalizeSharedVariableProc pFinalizeProc,
			LPCTSTR pszShareName = NULL);
		FTLINLINE ~CFSharedVariableT();

		FTLINLINE T& GetShareValue();
	private:
		HANDLE		m_hMapping;
		BOOL		m_bFirstCreate;
		T*			m_pShareValue;
		FinalizeSharedVariableProc	m_pFinalizeProc;
	};

	struct FTLGlobalShareInfo
	{
		DWORD	dwTraceTlsIndex;			//FastTrace中保存线程局部储存的index
		LONG    nTraceSequenceNumber;		//FastTrace中的序列号
		DWORD   dwBlockElapseTlsIndex;		//CFBlockElapse 中保存线程局部存储的Index
		LONG	nBlockElapseId;				//CFBlockElapse 中保存Id
	};
	FTLINLINE BOOL CALLBACK _FtlGlobalShareInfoInitialize(FTLGlobalShareInfo& rShareInfo);
	FTLINLINE BOOL CALLBACK _FtlGlobalShareInfoFinalize(FTLGlobalShareInfo& rShareInfo);

	//定义FTL中会用到全局共享变量 -- 可以在使用FTL的 Exe/Dll 之间共享变量
	__declspec(selectany)	CFSharedVariableT<FTLGlobalShareInfo>	g_GlobalShareInfo(
		_FtlGlobalShareInfoInitialize, 
		_FtlGlobalShareInfoFinalize,
		NULL);


    FTLEXPORT template<typename TBase, typename INFO_TYPE, LONG bufLen = MAX_BUFFER_LENGTH>
    class CFConvertInfoT
    {
        DISABLE_COPY_AND_ASSIGNMENT(CFConvertInfoT);
    public:
        FTLINLINE explicit CFConvertInfoT(INFO_TYPE info);
        FTLINLINE virtual ~CFConvertInfoT();
        FTLINLINE LPCTSTR GetConvertedInfo();
        FTLINLINE INFO_TYPE GetInfo() const;
        FTLINLINE void SetInfo(INFO_TYPE info);
    public:
        FTLINLINE virtual LPCTSTR ConvertInfo() = 0;
    protected:
        INFO_TYPE   m_Info;
        TCHAR       m_bufInfo[bufLen];
    };

    FTLEXPORT class CFAPIErrorInfo : public CFConvertInfoT<CFAPIErrorInfo,DWORD>
    {
        DISABLE_COPY_AND_ASSIGNMENT(CFAPIErrorInfo);
    public:
        FTLINLINE explicit CFAPIErrorInfo(DWORD dwError);
		FTLINLINE DWORD SetLanguageID(DWORD dwLanguageID);
        FTLINLINE virtual LPCTSTR ConvertInfo();
	protected:
		DWORD	m_LanguageID;
    };

    FTLEXPORT class CFComErrorInfo : public CFConvertInfoT<CFComErrorInfo,HRESULT>
    {
        DISABLE_COPY_AND_ASSIGNMENT(CFComErrorInfo);
    public:
        FTLINLINE explicit CFComErrorInfo(HRESULT hr);
        FTLINLINE virtual LPCTSTR ConvertInfo();
    protected:
        FTLINLINE LPCTSTR GetErrorFacility(HRESULT hr, LPTSTR pszFacility,DWORD dwLength);
    };

	FTLEXPORT class CFWindowDumpInfo: public CFConvertInfoT<CFWindowDumpInfo, HWND>
	{
		DISABLE_COPY_AND_ASSIGNMENT(CFWindowDumpInfo);
	public:
		FTLINLINE explicit CFWindowDumpInfo(HWND hWnd);
		FTLINLINE virtual LPCTSTR ConvertInfo();
	};

    //! 方便的管理需要分配的临时内存，在类的构造中分配内存，析构中释放
    //! 是否可能生成小的内存碎片？使用Pool优化？
    enum MemoryAllocType
    {
        matNew,             //使用new分配，使用delete释放，为了方便管理，即使只分配一个，也使用数组方式
        matVirtualAlloc,    //使用VirtualAlloc直接在进程的地址空间中保留一快内存(既非堆又非栈)，速度快
        matLocalAlloc,      //使用LocalAlloc分配,LocalFree释放,主要是为了兼容老的程序?
        //matMalloca,         //使用_malloca在栈上分配
    };

#ifndef DEFAULT_MEMALLOCATOR_FIXED_COUNT
    #define DEFAULT_MEMALLOCATOR_FIXED_COUNT 32
#endif 
    template <typename T, UINT DefaultFixedCount = DEFAULT_MEMALLOCATOR_FIXED_COUNT, MemoryAllocType allocType  = matNew>
    class CFMemAllocator
    {
        //和ATL中的 CTempBuffer 模板类比较
        DISABLE_COPY_AND_ASSIGNMENT(CFMemAllocator);
    public:
        FTLINLINE CFMemAllocator();
        FTLINLINE CFMemAllocator(DWORD nCount, BOOL bAlignment = FALSE);
        FTLINLINE ~CFMemAllocator();
        FTLINLINE T* GetMemory( UINT nMaxSize );
		FTLINLINE T* GetMemory();
        FTLINLINE operator T*()
        {
            if ( !m_pMem && m_nCount <= DefaultFixedCount )
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
        FTLINLINE CFStringFormater(DWORD dwInitAllocLength = MAX_BUFFER_LENGTH, DWORD dwMaxBufferTimes = STRINGFORMATER_BUFFER_MAX_DEFAULT_TIMES);
        FTLINLINE virtual ~CFStringFormater();
        FTLINLINE BOOL Reset(DWORD dwNewSize = 0);
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
        LPTSTR  m_pBuf;
		//TODO:
		//TCHAR	m_szInitBuf[MAX_BUFFER_LENGTH];
        DWORD   m_dwTotalSpaceSize;
        const DWORD m_dwInitAllocLength;
		const DWORD m_dwMaxBufferTimes;
    };

    // The typedef for the debugging output function.  Note that this matches OutputDebugString.
    typedef VOID (WINAPI *PFNDEBUGOUT)(LPCTSTR lpOutputString) ;

    enum TraceLevel
    {
        //linux的syslog 将日志分成7各等级：info < notice < warning < error < crit(ical)< alert < panic
        tlDetail,
        tlInfo,
        tlTrace ,
        tlWarn,
        tlError,
        tlEnd,              //结束标记，用来计算枚举个数，不要直接使用
    };

    typedef struct FAST_TRACE_OPTIONS
    {
        FAST_TRACE_OPTIONS()
        {
            bWriteThrough   = FALSE;
            bWriteToFile    = FALSE;
            bWriteDetail    = FALSE;
            traceThreshold  = tlTrace;
            pDebugOut       = OutputDebugString;
        }
        BOOL        bWriteToFile;           //是否写入文件，默认情况下关闭
        BOOL        bWriteThrough;          //如果写入文件的话，是否直接写出(效率低，但不会丢掉信息)
        BOOL        bWriteDetail;           //是否输出当时的时间，线程ID，日志等级等，只用于 pDebugOut
        TraceLevel  traceThreshold;         //控制输出的阈值 -- 只有大于等于该阈值的才能输出，默认为 tlTrace
        PFNDEBUGOUT pDebugOut;              //扩展的输出方式，默认为 OutputDebugString;
    } FAST_TRACE_OPTIONS , * LPFAST_TRACE_OPTIONS ;
    

    //默认以 HMODULE_CUSTOM 和 tlTrace 进行输出
#ifdef FTL_DEBUG
#   define FAST_TRACE(fmt, ...)     FAST_TRACE_EX(FTL::tlTrace, fmt, __VA_ARGS__)
//#   define FAST_TRACE      FTL::CFFastTrace::GetInstance().WriteLogInfo
//#   define FAST_TRACE_EX   FTL::CFFastTrace::GetInstance().WriteLogInfoEx
//#   define FAST_TRACEA     FTL::CFFastTrace::GetInstance().WriteLogInfoA

    //注意：这种输出的正则表达式为: ^\[(\d+)\]\[(\d{2}:\d{2}:\d{2}:\d{3})\]\[(\w+)\]\[(\d+)\]:(.*)$
    //提取方式($1-$2-$3-$4-$5): 
    //  $1 -- 表示序列号
    //  $2 -- 表示时间字符串
    //  $3 -- 表示日志等级
    //  $4 -- 表示线程ID
    //  $5 -- 剩下的用户输出日志
#   define FAST_TRACE_EX(level, fmt, ...) \
    do{ \
        FTL::CFFastTrace& traceObj = FTL::CFFastTrace::GetInstance(); \
        if (traceObj.CheckLevel(level)){ \
            TCHAR szTimeStr[16] = {0}; \
            LONG& rTraceSequenceNumber = FTL::g_GlobalShareInfo.GetShareValue().nTraceSequenceNumber; \
            LONG nCurSeqNumber = InterlockedIncrement (&rTraceSequenceNumber); \
            if(FTL::CFFastTrace::s_WriteDetail){ \
                traceObj.WriteLogInfoEx(level, TEXT("[%09d][%s][%c][%04d]:") fmt TEXT("\r\n"), \
                    nCurSeqNumber, traceObj.GetCurrentTimeStr(szTimeStr, _countof(szTimeStr)), \
                    *traceObj.GetLevelName(level), GetCurrentThreadId(),__VA_ARGS__); \
            } \
            else{ \
                 traceObj.WriteLogInfoEx(level, TEXT("") fmt TEXT("\r\n"), __VA_ARGS__);\
            } \
        } \
    }while(0);

#   define FAST_TRACEA(fmt, ...) \
    do{ \
        FTL::CFFastTrace& traceObj = FTL::CFFastTrace::GetInstance(); \
        if (traceObj.CheckLevel(FTL::tlTrace)){ \
            TCHAR szTimeStr[16] = {0}; \
            LONG& rTraceSequenceNumber = FTL::g_GlobalShareInfo.GetShareValue().nTraceSequenceNumber; \
            LONG nCurSeqNumber = InterlockedIncrement (&rTraceSequenceNumber); \
            if(FTL::CFFastTrace::s_WriteDetail){ \
                traceObj.WriteLogInfoA("[%09d][%S][%c][%04d]:" fmt "\r\n", \
                    nCurSeqNumber, traceObj.GetCurrentTimeStr(szTimeStr, _countof(szTimeStr)), \
                    *traceObj.GetLevelName(FTL::tlTrace), GetCurrentThreadId(),__VA_ARGS__); \
            }\
            else{ \
                traceObj.WriteLogInfoA(fmt "\r\n", __VA_ARGS__);\
            } \
        }\
    }while(0);

#else
#   define FAST_TRACE      __noop
#   define FAST_TRACE_EX   __noop
#endif

    #define FTFILESIG 'TSAF'
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 高性能的日志跟踪类，将各个线程的日志高性能的输出到单独的目的地(文件)中，查看时再进行统一
    /// 由于日志会大量使用，因此如果使用模版并且 inline 的话，是否会对文件大小、速度造成很大的影响？ 
    /// 也可以使用 #if(DEBUGLEVEL & BASIC_DEBUG)  #endif 的方式控制日志等级
    /// -- 实际代码中最好提取到单独的 DLL、Lib 模块中
    /// TODO
    ///   1.对于已经退出的线程需要进一步的处理 -- 从 m_AllFileWriters 中移除，否则可能越来越大
    ///     CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD...);
    ///   2.增加使用单独的写线程
    
    #define FAST_TRACE_OPTION_APP_NAME  _T("Options")  
    class CFFastTrace
    {
    public:
        typedef struct tagFTFILEHEADER
        {
            DWORD dwSig ;// The signature.  See the FTFILESIG define above.
            BOOL  bIsUnicode;  //表明当前是否是使用Unicode方式写的
            LONG  lItemCount ;// The total number of traces written to the file.
            LONG  lPID ;// The process ID for this file.  (Used to ensure a file matches it's siblings).
            LONG  lTID ;// The thread ID for this file.
        } FTFILEHEADER , * LPFTFILEHEADER ;

        typedef struct tagFTDATA
        {
            LONG       lSeqNum ;// The sequence number for this trace.
            FILETIME    stTime ;
            HMODULE     module;
            TraceLevel  level;
            LONG        nTraceInfoLen; //pszTraceInfo 的长度，目前必须是 pszTraceInfo 字符串长度+1(包括结尾的NULL,不浪费空间)，考虑TCHAR
            LPCTSTR     pszTraceInfo;
        } FTDATA , * LPFTDATA ;
    public:
        FTLINLINE static CFFastTrace& GetInstance();
        static BOOL s_WriteDetail;  //控制是否输出详细信息(通常只在输出到文件时才改为TRUE，输出到Output窗口时为False，则可以双击定位)

        //设置可以进行日志输出的类型(组合项)和等级(大于该等级的输出)
        FTLINLINE BOOL CheckLevel(TraceLevel level);
        FTLINLINE BOOL SetTraceOptions(LPFAST_TRACE_OPTIONS pOptions);
        FTLINLINE BOOL GetTraceOptions(LPFAST_TRACE_OPTIONS pOptions);
		FTLINLINE LPCTSTR GetLevelName(TraceLevel level);
        FTLINLINE LPCTSTR GetCurrentTimeStr(LPTSTR pszTimeBuf, INT nTimeBufCount);
        //默认使用 tlTrace 进行输出

		FTLINLINE void /*__cdecl*/ WriteLogInfo(const LPCTSTR lpszFormat,...);
		FTLINLINE void /*__cdecl*/ WriteLogInfoEx(TraceLevel level, const LPCTSTR lpszFormat,...);
		FTLINLINE void /*__cdecl*/ WriteLogInfoA(const LPCSTR lpszFormat,...);
	protected:
        class CFTFileWriter// : public CFTargetWriter<HANDLE>
        {
        public:
            FTLINLINE CFTFileWriter();
            FTLINLINE ~CFTFileWriter();
            FTLINLINE BOOL Initialize ( HANDLE hFile , DWORD dwTID );
            FTLINLINE BOOL WriteData ( LPFTDATA pData ) ;
            FTLINLINE BOOL WriteLog ( LPCTSTR pszLog ) ;
            FTLINLINE BOOL Flush ();
            FTLINLINE BOOL Close();
			FTLINLINE bool operator < (const CFTFileWriter & other) const;
        protected:
            HANDLE m_hFile ;    // The actual file handle.
            LONG   m_lWrites;   // The total number of writes to the file.
            DWORD  m_dwTID ;    // The thread this file wraps.
            FTLINLINE BOOL WriteHeader () ;
        };
    protected:
		//BOOL					m_bAllocTls;
        FAST_TRACE_OPTIONS      m_Options;
        CRITICAL_SECTION        m_CsLock;               //在更改Option等时进行互斥
        TCHAR                   m_szLogFilePrefix[MAX_PATH];
        typedef std::set<CFTFileWriter*>            AllFileWriterArrayType;
        typedef AllFileWriterArrayType::iterator    AllFileWriterArrayIterator;
        typedef std::pair<AllFileWriterArrayIterator , bool > AllFileWriterArrayPair;
        AllFileWriterArrayType  m_AllFileWriters;

        //FAST_TRACE_MODULE_INFO  m_AllTraceModuleInfos[MAX_MODULE_INFO_COUNT];
        //LONG                    m_CurrentModuleCount;
        //typedef std::map<HMODULE,FAST_TRACE_MODULE_INFO>    AllTraceModuleInfoType;
        //typedef std::pair<AllTraceModuleInfoType::iterator , bool > AllTraceModuleInfoPair;
        //AllTraceModuleInfoType   m_AllTraceModuleInfos;

        FTLINLINE CFTFileWriter* GetCurrentThreadFile();
        FTLINLINE HANDLE CreateRawThreadFile( DWORD dwTID );
        FTLINLINE BOOL BuildThreadFileName( LPTSTR szFileNameBuff , size_t iBuffLen , DWORD dwTID );
        FTLINLINE BOOL AddFileToArray(CFTFileWriter* pFileWriter);
        FTLINLINE BOOL RemoveFileFromArray(CFTFileWriter* pFileWriter);
        FTLINLINE BOOL CloseAllFileWriters();
        FTLINLINE void InternalWriteLogData(TraceLevel level, LPCTSTR pStrInfo);
        FTLINLINE BOOL MakeSurePathExist(LPCTSTR pszPath);
        FTLINLINE INT _GetTraceOption(LPCTSTR pszKeyName, INT nDefault, LPCTSTR lpFileName);
    private:
        FTLINLINE CFFastTrace(LPCTSTR pszSubPath, HMODULE hModule);
        FTLINLINE ~CFFastTrace();
        FTLINLINE BOOL _GetConfiguration(LPCTSTR pszConfigPath);
    };

    //高性能的计时器，能用于计算花费的时间(如计算拷贝时的速度估计) -- 支持暂停/继续
    #define NANOSECOND_PER_MILLISECOND  (1000 * 1000)
    #define MILLISECOND_PER_SECOND      (1000)
    #define NANOSECOND_PER_SECOND       (NANOSECOND_PER_MILLISECOND * MILLISECOND_PER_SECOND)

    enum RunningStatus
    {
        rsStopped,
        rsRunning,
        rsPaused
    };

    class CFElapseCounter
    {
        DISABLE_COPY_AND_ASSIGNMENT(CFElapseCounter);
    public:
        FTLINLINE CFElapseCounter();
        FTLINLINE ~CFElapseCounter();
		FTLINLINE BOOL Reset();
        FTLINLINE BOOL Start();
        FTLINLINE BOOL Pause();
        FTLINLINE BOOL Resume();
        FTLINLINE BOOL Stop();
        FTLINLINE RunningStatus GetStatus() const;

        //! 运行状态，获取当前时间到开始的时间
        //! 暂停状态，获取暂停结束时间到开始的时间
        //! 停止状态，获取结束到开始的时间
        FTLINLINE LONGLONG GetElapseTime(); //返回单位是纳秒(NS) 10^-9
    private:
        LARGE_INTEGER   m_Frequency;
        LARGE_INTEGER   m_StartTime;
        LARGE_INTEGER   m_PauseTime;
        LARGE_INTEGER   m_StopTime;
        RunningStatus   m_Status;
    };

#ifdef __cplusplus
    extern "C"
#endif
    void * _ReturnAddress(void);
#   pragma intrinsic(_ReturnAddress)

#ifndef DEFAULT_BLOCK_TRACE_THRESHOLD    //默认的追踪阈值为100毫秒
#  define DEFAULT_BLOCK_TRACE_THRESHOLD  (100)
#endif 

#ifndef MAX_TRACE_INDICATE_LEVEL        //默认最多追踪50层
#  define MAX_TRACE_INDICATE_LEVEL    (50)
#endif 

#pragma message( "  MAX_TRACE_INDICATE_LEVEL = " QQUOTE(MAX_TRACE_INDICATE_LEVEL) )

    enum TraceDetailType{
        TraceDetailNone = 0,
        TraceDetailExeName,
        TraceDetailModuleName,
    };
#ifdef FTL_DEBUG
    //KB118816
    
    //CFBlockElapse JOIN_TWO(elapse,__LINE__) (TEXT(__FILE__),__LINE__,TEXT(__FUNCTION__),FTL::_ReturnAddress(),(elapse))
    // #pragma TODO(此处的写法有问题，无法根据行号生成唯一变量 -- "JOIN_TWO" 不支持带参数的构造)
#  define FUNCTION_BLOCK_TRACE(elapse) \
    FTL::CFBlockElapse FTL_MAKE_UNIQUE_NAME(minElapse) (TEXT(__FILE__), __LINE__, TEXT(__FUNCTION__), FTL::TraceDetailNone, FTL::_ReturnAddress(),(elapse))
#  define FUNCTION_BLOCK_NAME_TRACE(blockName,elapse) \
    FTL::CFBlockElapse FTL_MAKE_UNIQUE_NAME(minElapse) (TEXT(__FILE__), __LINE__, blockName, FTL::TraceDetailNone, FTL::_ReturnAddress(), (elapse))

#  define FUNCTION_BLOCK_NAME_TRACE_EX(blockName, detailType, elapse) \
    FTL::CFBlockElapse FTL_MAKE_UNIQUE_NAME(minElapse) (TEXT(__FILE__), __LINE__, blockName, detailType, FTL::_ReturnAddress(), (elapse))

#  define FUNCTION_BLOCK_MODULE_NAME_TRACE(blockName, elapse) \
    TCHAR szModuleName[MAX_PATH] = {0};\
    GetModuleFileName(NULL, szModuleName, _countof(szModuleName));\
    TCHAR szTrace[MAX_PATH] = {0};\
    StringCchPrintf(szTrace, _countof(szTrace), TEXT("%s in %s(PID=%d)"),  blockName, FtlPathFindFileName(szModuleName), GetCurrentProcessId());\
    FTL::CFBlockElapse FTL_MAKE_UNIQUE_NAME(minElapse) (TEXT(__FILE__),__LINE__, szTrace, FTL::TraceDetailNone, FTL::_ReturnAddress(), (elapse));

#else
#  define  FUNCTION_BLOCK_TRACE(elapse)                                   __noop
#  define  FUNCTION_BLOCK_NAME_TRACE(blockName,elapse)                    __noop
#  define  FUNCTION_BLOCK_NAME_TRACE_EX(blockName, detailType, elapse)    __noop
#  define  FUNCTION_BLOCK_MODULE_NAME_TRACE(blockName, minElapse)         __noop
#endif


#ifdef FTL_DEBUG        //由于BlockElapse会消耗大量的时间，因此，只在Debug状态下启用
    //! 跟踪程序的运行逻辑和效率
    //! @code FUNCTION_BLOCK_TRACE(1000) -- 指定的代码块需要在1000毫秒中执行完，否则打印日志
    //! 由于每个线程都有自己的 s_Indent/s_bufIndicate(静态全局线程局部变量)，因此其操作方法是线程安全的
    //! Bug：
    //!   1.如果是 EXE + DLL 的方式，则线程同时调用Exe + Dll 时，会认为是不同的线程。
    //!   2.如果 DLL 是使用__declspec(thread) 分配线程本地存储区，客户端应用程序必须隐式链接到 DLL。
    //!     如果客户端应用程序显式链接到 DLL，对 LoadLibrary 的调用将不会成功加载此 DLL，出现access violation (AV)错误。 
    //!     http://support.microsoft.com/kb/118816/en-us
    //!     通过 Dumpbin.exe 查看“Thread Storage Directory”是否为0，可以知道DLL是否有 静态全局线程局部变量。
    //!   怎么解决？用TLS+FileMap？
    //!   解决方式：使用TLS先解决KB118816的问题，由于在第一次时分配，最后一次时释放，因此需要在线程
    //!     开始的时候调用一次 FUNCTION_BLOCK_TRACE(0)，保证在线程结束前indent都大于0。
    class CFBlockElapse
    {
    public:
        //使用毫秒作为判断是否超时
        FTLINLINE CFBlockElapse(LPCTSTR pszFileName,DWORD line, 
            LPCTSTR pBlockName, TraceDetailType detailType, LPVOID pReturnAddr, DWORD MinElapse = 0);
		FTLINLINE void SetLeaveLine(DWORD line);
        FTLINLINE ~CFBlockElapse(void);
    private:
        struct BlockElapseInfo
        {
            LONG   indent;
            TCHAR  bufIndicate[MAX_TRACE_INDICATE_LEVEL + 1];//增加最后的NULL所占的空间
            TCHAR  szDetailName[MAX_PATH];
        };
        //static DWORD  s_dwTLSIndex;
		//static LONG   s_lElapseId;
        const TCHAR* m_pszFileName;
        const TCHAR* m_pszBlkName;
        const TraceDetailType m_traceDetailType;
        const LPVOID m_pReturnAdr;
        const DWORD  m_Line;
        const DWORD m_MinElapse;
		LONG  m_nElapseId;
        DWORD m_StartTime;
    };
#endif //FTL_DEBUG

    //注意：目前用于SideBySide，注意：本类不是线程安全的
#if 0
    class CModulesHolder
    {
    public:
        FTLINLINE CModulesHolder();
        FTLINLINE ~CModulesHolder();
        FTLINLINE void Erase(HMODULE hModule);
        FTLINLINE bool Find(HMODULE hModule);
        FTLINLINE bool Insert(HMODULE hModule);
    private:
        typedef std::set<HMODULE>   ModeulesHolderType;
        typedef ModeulesHolderType::iterator ModeulesHolderIterator;
        typedef std::pair<ModeulesHolderIterator , bool > ModeulesHolderPair;
        std::set<HMODULE> m_allModules;
    };
#endif

    //Function
    //显示Message前获取当前程序的活动窗体
    //HWND hWnd = (HWND)(AfxGetApp()->m_pMainWnd) ;
    //if ( FALSE == ::IsWindowVisible ( hWnd ) )
    //{
    //    hWnd = ::GetActiveWindow ( ) ;
    //}
    FTLINLINE int __cdecl FormatMessageBox(HWND hWnd, LPCTSTR lpCaption, UINT uType, LPCTSTR lpszFormat, ...);

} //namespace FTL

#endif// FTL_BASE_H

#ifndef USE_EXPORT
#  include "ftlbase.hpp"
#endif
