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
#include "ftlSharedVariable.h"
#include "ftlLog.h"


//在某些情况下(比如 IE插件), XXX_VERIFY 检测到错误时, _CrtDbgReport 无法弹出 assert 对话框
#ifndef USE_MESSAGEBOX_ERROR
#  define USE_MESSAGEBOX_ERROR 0
#endif 

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
    #  define REPORT_ERROR_INFO_EX(c, e, ev, x) \
         do{ \
             c _err(e);\
             USES_CONVERSION;\
             FTLTRACEEX(FTL::tlError, TEXT("Error!!! Reason = 0x%08x(%d,%s),Code:\"%s\" "),\
               (uint32_t)ev, (uint32_t)ev, _err.GetConvertedInfo(),TEXT(#x));\
             (1 != DBG_REPORT(_err, ev)) || \
               (DBG_BREAK(), 0);\
         }while(0)

    #  define REPORT_ERROR_INFO(c, e, x) REPORT_ERROR_INFO_EX(c, e, e, x)
    #else //Not Define FTL_DEBUG
    #  define REPORT_ERROR_INFO_EX(c, e, ev, x) __noop
    #  define REPORT_ERROR_INFO(c, e, x) __noop
#endif //FTL_DEBUG


    //自己构造 HRESULT 错误值 -- MAKE_HRESULT(sev,fac,code)
    //  SEVERITY_XXX -- 严重程度(0:成功；1:错误)
    //  FACILITY_XXX -- 设备信息(错误的类型，如 RPC、SECURITY、WIN32 等)
    //可以通过 HRESULT_SEVERITY、HRESULT_FACILITY、HRESULT_CODE 分别获取对应的值

    #ifdef FTL_DEBUG  //调试版本时，获取并判断 LastError，为了防止 REPORT_ERROR_INFO 对LastError产生影响，需要重新设置
    # define ERROR_RETURN_VERIFY(x)\
        FTLTRACEEX(FTL::tlDetail, TEXT("ERROR_RETURN_VERIFY call \"%s\""),TEXT(#x));\
        dwRet = (x);\
        if(ERROR_SUCCESS != dwRet)\
        {\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, dwRet, x);\
        }

    # define API_ASSERT(x)\
        FTLTRACEEX(FTL::tlDetail, TEXT("API_ASSERT call \"%s\""), TEXT(#x));\
        if(FALSE == (x))\
        {\
            FTL::CFLastErrorRecovery lastErrorRecory;\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lastErrorRecory.GetError(), x);\
        }

    //FTLTRACEEX(FTL::tlDetail, TEXT("%s(%d) :\t API_VERIFY Call:\"%s\" \n"),
    //    TEXT(__FILE__),__LINE__, TEXT(#x));
    # define API_VERIFY(x)   \
        SetLastError(0); \
        FTLTRACEEX(FTL::tlDetail, TEXT("API_VERIFY call \"%s\""), TEXT(#x));\
        bRet = (x);\
        if(FALSE == bRet)\
        {\
            FTL::CFLastErrorRecovery lastErrorRecory;\
            REPORT_ERROR_INFO(FTL::CFAPIErrorInfo, lastErrorRecory.GetError(), x);\
        }

    # define API_VERIFY_EXCEPT1(x,e1)\
        SetLastError(0); \
        FTLTRACEEX(FTL::tlDetail, TEXT("API_VERIFY_EXCEPT1 call \"%s\""), TEXT(#x));\
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
        FTLTRACEEX(FTL::tlDetail, TEXT("API_VERIFY_EXCEPT2 call \"%s\""), TEXT(#x));\
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

	# define COM_VERIFY_RETURN_IF_FAILED(x)   \
        hr = (x);\
        if(S_OK != hr)\
        {\
            REPORT_ERROR_INFO(FTL::CFComErrorInfo, hr,x);\
			return; \
        }

	# define COM_VERIFY_RETURN_VALUE_IF_FAILED(x)   \
        hr = (x);\
        if(S_OK != hr)\
        {\
            REPORT_ERROR_INFO(FTL::CFComErrorInfo, hr,x);\
            return hr; \
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

    #define CHECK_POINTER_RETURN_IF_FAIL(p)    \
        if(NULL == (p))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            return;\
        }

    #define CHECK_POINTER_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p)\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            return r;\
        }

    #define CHECK_POINTER_WRITABLE_RETURN_IF_FAIL(p)    \
        if(NULL == p || ::IsBadWritePtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid writable pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, sizeof(DWORD_PTR)));\
            return;\
        }

    #define CHECK_POINTER_WRITABLE_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadWritePtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid writable pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, sizeof(DWORD_PTR)));\
            return r;\
        }

    #define CHECK_POINTER_READABLE_RETURN_IF_FAIL(p)    \
        if(NULL == p || ::IsBadReadPtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid readable pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, sizeof(DWORD_PTR)));\
            return;\
        }

    #define CHECK_POINTER_READABLE_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadReadPtr(p, sizeof(DWORD_PTR)))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid readable pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, sizeof(DWORD_PTR)));\
            return r;\
        }

    #define CHECK_POINTER_WRITABLE_DATA_RETURN_IF_FAIL(p,len)    \
        if(NULL == p || ::IsBadWritePtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid %d len writable pointer"), TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return;\
        }

    #define CHECK_POINTER_WRITABLE_DATA_RETURN_VALUE_IF_FAIL(p,len,r)    \
        if(NULL == p || ::IsBadWritePtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid %d len writable pointer"), TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return r;\
        }

    #define CHECK_POINTER_READABLE_DATA_RETURN_IF_FAIL(p,len)    \
        if(NULL == p || ::IsBadReadPtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid %d len readable pointer"), TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            return;\
        }

    #define CHECK_POINTER_READABLE_DATA_RETURN_VALUE_IF_FAIL(p,len,r)    \
        if(NULL == p || ::IsBadReadPtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid %d len readable pointer"), TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            return r;\
        }

    #define CHECK_POINTER_READ_WRIATE_ABLE_DATA_RETURN_IF_FAIL(p,len)    \
        if(NULL == p || ::IsBadReadPtr(p, len) || ::IsBadWritePtr(p,len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid %d len read/write able pointer"), TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return;\
        }

    #define CHECK_POINTER_READ_WRIATE_ABLE_DATA_RETURN_VALUE_IF_FAIL(p,len,r)    \
        if(NULL == p || ::IsBadReadPtr(p, len))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid %d len read/write able pointer"), TEXT(#p),p,len);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadReadPtr(p, len));\
            FTLASSERT(FALSE ==::IsBadWritePtr(p, len));\
            return r;\
        }

    #define CHECK_POINTER_ISSTRING_PTR_RETURN_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadStringPtr(p, INFINITE))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid string pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadStringPtr(p, INFINITE));\
            return;\
        }

    #define CHECK_POINTER_ISSTRING_PTR_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadStringPtr(p, INFINITE))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid string pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadStringPtr(p, INFINITE));\
            return r;\
        }

    #define CHECK_POINTER_CODE_RETURN_IF_FAIL(p)    \
        if(NULL == p || ::IsBadCodePtr(p))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid code pointer"), TEXT(#p),p);\
            FTLASSERT(NULL != p);\
            FTLASSERT(FALSE ==::IsBadCodePtr(p));\
            return;\
        }

    #define CHECK_POINTER_CODE_RETURN_VALUE_IF_FAIL(p,r)    \
        if(NULL == p || ::IsBadCodePtr(p))\
        {\
            FTLTRACEEX(FTL::tlWarn, TEXT("Warning!!! %s(0x%p) is not a valid code pointer"), TEXT(#p),p);\
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
    FTLINLINE LPTSTR FtlPathAddBackslash(LPTSTR lpszPath);
    FTLINLINE BOOL   FtlPathAppend(LPTSTR pszPath, LPCTSTR pszMore); //, INT nMaxCount = MAX_PATH
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
    //  注意: pragma data_seg 方式是: 多个Exe公用同一个DLL时，使用DLL中相同的变量(一般是 注入Hook 等场景)
    //  CFSharedVariableT 是 同一个进程的EXE和各个DLL之间共享变量( 默认情况下静态库中定义的全局变量, 在 Exe 和各个 DLL 中有不同的实例,地址不同) 
    //  TODO: 如果是 DLL 中 export 的变量呢? 是否是全局相同的?


// 	struct FTLGlobalShareInfo
// 	{
// 		DWORD	dwTraceTlsIndex;			//FastTrace中保存线程局部储存的index
// 		LONG    nTraceSequenceNumber;		//FastTrace中的序列号
// 		DWORD   dwBlockElapseTlsIndex;		//CFBlockElapse 中保存线程局部存储的Index
// 		LONG	nBlockElapseId;				//CFBlockElapse 中保存Id
// 	};
// 	FTLINLINE BOOL CALLBACK _FtlGlobalShareInfoInitialize(BOOL bFirstCreate, FTLGlobalShareInfo& rShareInfo);
// 	FTLINLINE BOOL CALLBACK _FtlGlobalShareInfoFinalize(BOOL bFirstCreate, FTLGlobalShareInfo& rShareInfo);

	//定义FTL中会用到全局共享变量 -- 可以在使用FTL的 Exe/Dll 之间共享变量
// 	__declspec(selectany)	CFSharedVariableT<FTLGlobalShareInfo>	g_GlobalShareInfo(
// 		_FtlGlobalShareInfoInitialize, 
// 		_FtlGlobalShareInfoFinalize,
// 		NULL);


    FTLEXPORT template<typename TBase, typename INFO_TYPE, LONG bufLen = DEFAULT_BUFFER_LENGTH>
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
        
        //表示是否能查出来错误信息，有些错误码目前没有定义实际的意义(比如 35,40~49 等),
        //为了性能,这个信息只有在调用了 ConvertInfo 以后才能正确设置
        FTLINLINE BOOL isValidErrCode(); 
        FTLINLINE virtual LPCTSTR ConvertInfo();
	protected:
		DWORD	m_LanguageID;
        BOOL    m_isValidErrCode;
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



    // The typedef for the debugging output function.  Note that this matches OutputDebugString.
    typedef VOID (WINAPI *PFNDEBUGOUT)(LPCTSTR lpOutputString) ;

	//注意：这种输出的正则表达式为: ^\[(\d+)\]\[(\d{2}:\d{2}:\d{2}:\d{3})\]\[(\w+)\]\[(\d+)\]:(.*)$
	//提取方式($1-$2-$3-$4-$5): 
	//  $1 -- 表示序列号
	//  $2 -- 表示时间字符串
	//  $3 -- 表示日志等级
	//  $4 -- 表示线程ID
	//  $5 -- 剩下的用户输出日志

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

// #ifdef __cplusplus
//     extern "C"
// #endif
//    void * _ReturnAddress(void);
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
    // 但是使用 FTL_MAKE_UNIQUE_NAME 的话, 又无法正确解析 __FUNCTION__ (会解析成 CFBlockElapse 析构)
#  define FUNCTION_BLOCK_TRACE(elapse) \
    FTL::CFBlockElapse JOIN_TWO(minElapse, __LINE__) (TEXT(__FILE__), __LINE__, TEXT(__FUNCTION__), FTL::TraceDetailNone, _ReturnAddress(), NULL, (elapse))
#  define FUNCTION_BLOCK_NAME_TRACE(blockName,elapse) \
    FTL::CFBlockElapse JOIN_TWO(minElapse, __LINE__) (TEXT(__FILE__), __LINE__, blockName, FTL::TraceDetailNone, _ReturnAddress(), NULL, (elapse))

#  define FUNCTION_BLOCK_TRACE_WITH_THIS(elapse) \
   FTL::CFBlockElapse JOIN_TWO(minElapse, __LINE__) (TEXT(__FILE__), __LINE__, TEXT(__FUNCTION__), FTL::TraceDetailNone, NULL, this, (elapse))

#  define FUNCTION_BLOCK_NAME_TRACE_EX(blockName, detailType, elapse) \
    FTL::CFBlockElapse JOIN_TWO(minElapse, __LINE__) (TEXT(__FILE__), __LINE__, blockName, detailType, _ReturnAddress(), NULL, (elapse))

#  define FUNCTION_BLOCK_MODULE_NAME_TRACE(blockName, elapse) \
    TCHAR szModuleName[MAX_PATH] = {0};\
    GetModuleFileName(NULL, szModuleName, _countof(szModuleName));\
    TCHAR szTrace[MAX_PATH] = {0};\
    StringCchPrintf(szTrace, _countof(szTrace), TEXT("%s in %s(PID=%d)"),  blockName, FtlPathFindFileName(szModuleName), GetCurrentProcessId());\
    FTL::CFBlockElapse JOIN_TWO(minElapse, __LINE__) (TEXT(__FILE__),__LINE__, szTrace, FTL::TraceDetailNone, _ReturnAddress(), NULL, (elapse));

#else
#  define  FUNCTION_BLOCK_TRACE(elapse)                                   __noop
#  define  FUNCTION_BLOCK_NAME_TRACE(blockName,elapse)                    __noop
#  define  FUNCTION_BLOCK_TRACE_WITH_THIS(blockName, elapse)              __noop
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
            LPCTSTR pBlockName, TraceDetailType detailType, LPVOID pReturnAddr, 
            LPVOID pObjectAddr = NULL, DWORD MinElapse = 0);
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
        const LPVOID m_pObjectAddr;
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
