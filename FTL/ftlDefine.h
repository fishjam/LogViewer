//https://gist.githubusercontent.com/zenxds/7579930/raw/f509fbd93eea430eff45a05c1d08c62b032203ec/neverBug.js
//
//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//
//
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//         佛祖镇楼                  BUG辟易
//         佛曰:
//             写字楼里写字间，写字间里程序员；
//             程序人员写程序，又拿程序换酒钱。
//             酒醒只在网上坐，酒醉还来网下眠；
//             酒醉酒醒日复日，网上网下年复年。
//             但愿老死电脑间，不愿鞠躬老板前；
//             奔驰宝马贵者趣，公交自行程序员。
//             别人笑我忒疯癫，我笑自己命太贱；
//             不见满街漂亮妹，哪个归得程序员？

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ftlDefine.h
/// @brief  Fishjam Template Library Define Header File.
/// @author fujie
/// @version 0.6 
/// @date 03/30/2008
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FTL_DEFINE_H
#define FTL_DEFINE_H
#pragma once

#define IS_VS_2010_HIGHER	(_MSC_VER >= 1600)
#define IS_VS_2012_HIGHER	(_MSC_VER >= 1700)
#define IS_VS_2013_HIGHER	(_MSC_VER >= 1800)
#define IS_VS_2015_HIGHER	(_MSC_VER >= 1900)
#define IS_VS_2017_HIGHER   (_MSC_VER >= 1916)

//TODO: 从什么时候开始支持 C++ 11? 是否有版本要求
#define IS_SUPPORT_CPP11    (_MSC_VER >= 1900)

namespace FTL
{
    //使用模版的export关键字，将声明和实现分开 -- 但目前VS2003~VS2008都不支持 export 关键字
    //考虑：是否可以扩展该宏实现静态FTL库？参考 ATL_DLL_ 宏的用法？
    #ifdef USE_EXPORT
    #  define FTLEXPORT  export
    #  define FTLINLINE
    #else
    #  define FTLEXPORT
    #  define FTLINLINE inline
    #endif

        //调试版本下自动定义 FTL_DEBUG，RELEASE 版本下可以手工定义 FTL_DEBUG
    #if defined(DEBUG) || defined(_DEBUG)
    #  ifndef FTL_DEBUG
    #    define FTL_DEBUG
    #  endif
    #endif

    #ifndef QQUOTE
    #  define    QUOTE(x)        #x
    #  define    QQUOTE(y)       QUOTE(y)
    #endif //QQUOTE

    #define MAKE_TYPE_DESC(type) { type, _T(#type) }  
	
	//#define		__FILENAME__ (_tcsrchr(TEXT(__FILE__), TEXT('\\')) ? _tcsrchr(TEXT(__FILE__), TEXT('\\')) + 1 : TEXT(__FILE__))
	#define     __FILE__LINE__  TEXT(__FILE__) TEXT("(") TEXT(QQUOTE(__LINE__)) TEXT(") :")
	//#define     __FILE__LINE_ANSI__  __FILENAME__ "(" QQUOTE(__LINE__) ") :"

    /**********************************************************************************************
    * 宏展开是按字符顺序扫描的
    * JOIN_ONE(var_, __LINE__) 中，首先扫描到JOIN_ONE，因此结果直接生成 var___LINE__, 展开结束
    * JOIN_TWO(var_, __LINE__) 中，首先扫描到JOIN_TWO，展开成中间结果
    *   JOIN_ONE(var_, __LINE__)， 在此次展开中，再看到__LINE__，替换为 JOIN_ONE(var_，11)，
    *   然后再进行第二次展开，生成 var_11  -- 可以根据当前的行号，生成“临时的唯一”变量
    /**********************************************************************************************/
    #define JOIN_ONE(x, y) x ## y
    #define JOIN_TWO(x, y) JOIN_ONE(x, y)
    #define FTL_JOIN(x, y) JOIN_TWO(x, y)
    #define FTL_MAKE_UNIQUE_NAME( prefix )  FTL_JOIN( prefix, __LINE__ )

    //用于在编译器的输出窗口(Output)中输出“TODO”信息，可以双击直接进行定位
    //使用方式：pragma TODO(Fix this later)
    #define TODO(desc) message(__FILE__ "(" QQUOTE(__LINE__) ") : TODO: " #desc)

    //处理虚拟按键(如 VK_ESCAPE )， 或者在 WM_KEYDOWN 中 (GetAsyncKeyState(vk_code) & 0x01) ?
    #define KEY_DOWN(vk_code)   ( (GetAsyncKeyState(vk_code) & 0x8000) ? TRUE  : FALSE ) 
    #define KEY_UP(vk_code)     ( (GetAsyncKeyState(vk_code) & 0x8000) ? FALSE : TRUE  )

	#define FTL_MAX_CLASS_NAME_LENGTH	256

    //换行符(10,A)
    #define CHAR_LF  '\n'
    //回车符(13,D)
    #define CHAR_CR  '\r'

    #define FTL_MIN(a,b)                (((a) < (b)) ? (a) : (b))
    #define FTL_MAX(a,b)                (((a) > (b)) ? (a) : (b))
        //如果x在 [a,b] 之间，则是x, 否则会是边沿值
    #define FTL_CLAMP(x, a, b)			(FTL_MIN((b), FTL_MAX((a), (x))))
    #define FTL_ABS(a)		            (((a) < 0) ? -(a) : (a))
    #define FTL_SIGN(a)		            (((a) > 0) ? 1 : (((a) < 0) ? -1 : 0))
    #define FTL_INRANGE(low, Num, High) (((low) <= (Num)) && ((Num) <= (High)))

    //注意，需要区分Dos和Unix
    #define LINE_SEP    CR LF

#if 1
    //TODO: 适合于 FTL 中增加回车换行的用法
#  define LOG_LINE_SEP_W    L"\r\n"
#  define LOG_LINE_SEP_A    "\r\n"
#else
    //TODO: 适合于 ATLTRACE 一类的用法(需要自己增加)
#  define LOG_LINE_SEP_W    L""
#  define LOG_LINE_SEP_A    ""
#endif


	//禁止拷贝构造和赋值操作符
	#define DISABLE_COPY_AND_ASSIGNMENT(className)  \
		private:\
		 className(const className& ref);\
			className& operator = (const className& ref)


    //定义来查找一个数组中的 past-the-end 位置，使用方式:
    //  std::find(intArray, ARRAY_PAST_THE_END(intArray), 10);
    #define ARRAY_PAST_THE_END(c) ( c + _countof( c ) )

    //用于<重载时比较两个成员的对象变量
    #define COMPARE_OBJ_LESS(f,l,r) \
        if( l.f < r.f ) { return true; } \
        else if( l.f > r.f ) { return false; }
        
    //定义比较成员变量的宏，常用于重载类、结构的 operator < 或 > 时，参数分别为 field , other&
	//注意：在比较完所有需要的变量后需要返回 false( http://support.microsoft.com/kb/949171 )
    #define COMPARE_MEM_LESS(f, o) \
        if( f < o.f ) { return true; }\
        else if( f > o.f ) { return false; }

	#define COMPARE_MEM_BIG(f, o) \
	if( f > o.f ) { return true; }\
		else if( f < o.f ) { return false; }

	#ifndef MAKELONGLONG
		#define MAKELONGLONG(h, l)	( (((((LONGLONG)(h)) & 0xFFFFFFFF)) << 32)) | (LONGLONG)(((((LONGLONG)(l)) & 0xFFFFFFFF)))
		#define HILONG(a) (LONG)	((((LONGLONG)(a)) & 0xFFFFFFFF00000000) >> 32)
		#define LOLONG(a) (LONG)	(((LONGLONG)(a)) & 0xFFFFFFFF)
	#endif 

    #define GUID_LENGTH_WITHOUT_BRACKETS         36
    #define GUID_LENGTH_WITH_BRACKETS            38
    #define GUID_MAX_LENGTH                      40

    //指针长度对齐(32位为4, 64位为8)
    #define BUFFER_ALLIGNMENT                   (sizeof(void*))
    #define DEFAULT_BUFFER_LENGTH               (256)
    #define MAX_BUFFER_LENGTH                   (64 * 1024)
	
    enum RecursiveWay			//递归算法的类型
	{
		rwNone,					//不递归
		rwDepthFirst,			//深度优先
		rwBreadthFirst,			//广度优先
	};

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

}


#endif //FTL_DEFINE_H