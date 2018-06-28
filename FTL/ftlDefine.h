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

	#define     __FILE__LINE__  TEXT(__FILE__) TEXT("(") TEXT(QQUOTE(__LINE__)) TEXT(") :")

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

    //注意，需要区分Dos和Unix
    #define LINE_SEP    CR LF

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
		#define MAKELONGLONG(a, b)	((LONGLONG)(((((LONGLONG)(a)) & 0xFFFFFFFF)) | (((((LONGLONG)(b)) & 0xFFFFFFFF)) << 32)))
		#define HILONG(a) (LONG)	((((LONGLONG)(a)) & 0xFFFFFFFF00000000) >> 32)
		#define LOLONG(a) (LONG)	(((LONGLONG)(a)) & 0xFFFFFFFF)
	#endif 

	enum RecursiveWay			//递归算法的类型
	{
		rwNone,					//不递归
		rwDepthFirst,			//深度优先
		rwBreadthFirst,			//广度优先
	};
}


#endif //FTL_DEFINE_H