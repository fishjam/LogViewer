
#ifndef FTL_SYSTEM_H
#define FTL_SYSTEM_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlsystem.h requires ftlbase.h to be included first
#endif

/*************************************************************************************************************************
* TODO:
*   静态链接Vista中的库，并使用 delayload 的方式防止在WinXp等上无法运行
*   #pragma comment(lib, "dwmapi.lib")
*   #pragma comment(lib, "delayimp.lib")
*   #pragma comment(linker, "/delayload:dwmapi.dll")
*************************************************************************************************************************/

/*************************************************************************************************************************
* Process Hacker -- 开放源码的进程管理器
* 已读例子
*   com\uac(权限分析未看完) -- 动态提升权限的例子(Button_SetElevationRequiredState 设置UAC盾形Shield标记),有个 ElevationManager 可以考虑重用?
*
*************************************************************************************************************************/

#include <Shlwapi.h>    //IShellFolder
#include <float.h>      //_control87
#include <TlHelp32.h>   //CreateToolhelp32Snapshot
#include <list>
#include <functional>
#include <atlstr.h>
#include "ftlFunctional.h"
#include "ftlThread.h"

//控制面板的选项：
//http://msdn.microsoft.com/en-us/library/windows/desktop/cc144191(v=vs.85).aspx
//http://msdn.microsoft.com/en-us/library/windows/desktop/ee330741(v=vs.85).aspx
//https://wiki.mst.edu/deskinst/info/windows/vista/control_panel

/*****************************************************************************************************
* 最佳实践
*   1.机器的唯一标识, 一般可以通过 MachineGuid(HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid)  
*      + MacAddress(参见 CFTLNetTester::dump_allAdaptersInfo) 组合生成字符串的方式
*      
*****************************************************************************************************/

/*****************************************************************************************************
* Session Manager(smss.exe)
*
* 操作系统
*   4个特性：并发性、共享性、虚拟性 和 不确定性。
*   5个管理功能：进程管理、文件管理、存储管理、设备管理 和 作业管理。
* 
* 电源管理(具体宏的定义参见 CFPowerBroadcastMsgInfo )
*   ACPI(Advanced Configuration Management) -- 电源管理规范，由系统而不是BIOS来全面控制电源管理，
*   S0 -- 正常工作状态，所有设备全部供电。
*   S1 -- 通过CPU时钟控制器将CPU关闭之。其他的部件仍然正常工作。该状态也称为POS（Power on Suspend）
*         中央处理器暂停、内存已刷新、整个系统处于低电力模式
*   S2 -- CPU处于停止运作状态，总线时钟也被关闭。其余的设备仍然运转。
*   S3 -- 睡眠/待机(Sleep/StandBy), 操作系统把当前信息储存到内存中，只有内存等几个关键部件通电，可被鼠标或键盘唤醒。
*   S4 -- 休眠(Hibernate), 操作系统把当前信息储存到硬盘中，这时系统主电源关闭。但硬盘仍然带电并可以被唤醒。
*         TODO: 进入挂起状态(Suspend State)后，不能通过键盘或鼠标唤醒计算机，必须通过Power Key唤醒
*   S5 -- 关机，所有设备全部断电。
*   电源管理额消息(对应用程序来说，S3/S4 没有本质区别): 
*     挂起 -- PBT_APMQUERYSUSPEND(WinXP) -> PBT_APMSUSPEND 
*     恢复(顺序不保证): PBT_APMRESUMEAUTOMATIC (恢复时始终会发送，处理时的推荐消息?) -> PBT_APMRESUMESUSPEND(需要用户按键恢复才发送?)
*   电源管理部分的设置:
*     1.控制面板\所有控制面板项\电源选项
*     2.powercfg.exe
*
* 操作系统提供的IPC机制一般分为两类：
*   1.本地IPC -- 共享内存、管道、Unix领域socket、door(?)、信号等；
*   2.远程IPC -- Internet领域socket、Win32命名管道(named pipe)等
*
* 通过性能计数监视程序性能和泄漏：
*   WinXP: 控制面板 -> 管理工具 -> 性能 -> 性能日志和警报 -> 计数器日志 -> 右键 -> 新建日志设置 -> 起名
*   Win7:  控制面板\所有控制面板项\性能信息和工具\高级工具 -> 打开性能监视器 -> 数据收集器集 -> 用户定义 -> 右键 -> 新建 -> 手动创建 -> 创建数据日志(性能计数器)
*          添加: 1.选中Process下的指定计数器, 2.然后选定指定名字的对象实例;
*          日志格式选择 "逗号分隔" -- 即保存为 .csv 格式，方便 Excel 分析
*   Win10: 我的电脑 -> 右键"管理" -> 系统工具\性能\数据收集器集\用户定义\
*   “常规”中“添加计数器” -> "性能对象" 选择 "Process",通常来说为了检查内存、资源泄露，需要增加以下几种：
*     Handle Count -- 句柄数
*     Virtual Bytes -- 
*     Working Set -- 内存使用
*     Virtual Bytes -- 虚拟内存大小
*     Page File Bytes = Private Bytes -- 
*        TODO: Virtual Bytes > Private Bytes = Page File Bytes > Working Set > Working Set Private
*     Privileged Time -- 核心态CPU时间，如果太高，说明程序可能有问题
*     Processor Time / User Time -- 用户态CPU时间(通常需要除以CPU个数才是任务管理器中看到的百分比)
*     Thread Count -- 
*   如果没有内存泄露，应该是在特定区间内波动，如果呈现线性增长趋势，通常说明有泄漏。
*   Excel 中根据csv作图:   
* 
* 映像劫持 -- 运行指定名字的程序时，操作系统就会先启动Debugger指定的进程，并把要执行的程序名和路径传递给它
*   如：替换任务管理器：
*     HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe
*     设置 Debugger 项的值，如 "H:\TOOLS\SYSTEM\PROCESSEXPLORER\PROCEXP.EXE"
*****************************************************************************************************/


/*****************************************************************************************************
* 绿色软件(Portable)的制作方法:
*   注意：提取的路径通常是绝对路径，通常需要转换成宏保存，在恢复时再把宏替换成实际路径
*   1.windows优化大师等工具的软件卸载提取，可以分析(原理是什么?)
*   2.反编译安装程序，根据其安装脚本来制作,如：
*     InstallShield,Wise,Inno,Windows Installer(Orca),NSIS,Installer VISE等
*   3.分析安装后的log文件，或卸载时使用的log文件
*   4.安装前后比较系统快照(RegSnap/inctrl5/Advanced Registery Tracer 等，Reg2inf)
*     注册表文件中含有路径信息时，可用Reg2inf将reg转换为inf来实现全自动的路径处理(reg2inf.exe -w -t *.reg *.inf)
*
* PowerShadow(针对OS的虚拟?)
* SVS(针对软件的虚拟?)
*****************************************************************************************************/

/*****************************************************************************************************
* 重启管理器(Restart Manager) -- 可以减少或避免安装或更新程序所需要的系统重启次数
*   RmStartSession
*   RmRegisterResources
*   RmShutdown
*   RmRestart
*****************************************************************************************************/

/*****************************************************************************************************
* 应用程序虚拟化--可以实现在同一操作系统上运行任何版本的虚拟应用程序而不发生冲突
*   VMware ThinApp:
*   Cameyo:
*****************************************************************************************************/
namespace FTL
{
//Class   
    template <typename T> class CFSingletonT;

    class CFRegUtil;
    class CFSystemUtil;
}

namespace FTL
{
    //单实例模式的模版
    template <typename T>
    class CFSingletonT
    {
    private:
        class SelfPtr
        {
        private:
            T*    m_pInstance;
        public:
            SelfPtr() : m_pInstance(NULL) {}
            ~SelfPtr() { delete m_pInstance; m_pInstance = NULL; }
            T* GetPtr() { return m_pInstance; }
            void SetPtr(T* ptr) { delete m_pInstance; m_pInstance = ptr; };
        };
        static SelfPtr s_pSelf;

    protected:
        CFSingletonT(){}

    public:
        virtual ~CFSingletonT(){}

        static T* Instance() 
        {
            if( !s_pSelf.GetPtr() )
                s_pSelf.SetPtr(new T());
            return s_pSelf.GetPtr();
        }
        static void Release()
        {
            s_pSelf.SetPtr(NULL);
        }
    }; 

    template <typename T>
    typename CFSingletonT<T>::SelfPtr CFSingletonT<T>::s_pSelf = CFSingletonT<T>::SelfPtr();

    class CFOSInfo : public CFSingletonT<CFOSInfo>
    {
    public:
        enum OSType
        {
            ostLowUnknown,
            //ostWin32,
            ostWin95,
            ostWin98,	//MSDN
            ostWin98_SE,
            ostWinMe,	//MSDN
            ostWinNT3,
            ostWinNT4,
            ostWin2000, 
            ostWinXP,	//MSDN
            ostWin2003,	//MSDN
            ostVista,	//OK
            ostWindows7,//OK
            ostWindows8,//OK
            ostWindows81,
            ostWindows10, //必须使用 RtlGetVersion
            ostHighUnknown
        };//OSType;
    public:
        //_osver,_winmajor,_winminor
        FTLINLINE BOOL IsGreaterWinNT() const;
        FTLINLINE OSType GetOSType() const;
        // if NT without ServicePack, return TRUE but csCSDVersion is ""
        FTLINLINE BOOL GetWinNTServicePackName(LPTSTR pszNTVersion,DWORD nSize) const;
        FTLINLINE DWORD GetNumberOfProcessors() const;
        FTLINLINE BOOL GetPhysicalBytes(DWORDLONG* pAvailablePhysicalBytes, DWORDLONG *pTotalPhysicalBytes) const;
        FTLINLINE BOOL GetVolumeVisibleName(LPCTSTR pszVolume, LPTSTR pszBuf, DWORD bufSize) const;
    public:
        FTLINLINE CFOSInfo();
    private:
        OSVERSIONINFOEX m_OsInfo;
    };


	//SystemParametersInfo -- 查询当前系统的参数
	class SystemParamProperty
	{
	public:
		FTLINLINE SystemParamProperty();
		FTLINLINE ~SystemParamProperty();
		FTLINLINE LPCTSTR GetPropertyString();
	public:
		DWORD_PTR	m_dwUnknown;	//用于未实现的项(如 SPI_LANGDRIVER)
		BOOL	m_bBeep;
		BOOL    m_bKeyboardCues;
		DWORD	m_dwKeyboardSpeed;
		int		m_MouseInfo[3];
		int		m_nBorder;
		int		m_nIconHorizontalSpacing;
		int		m_nIconVerticalSpacing;
		int		m_nScreenSaveTimeout;
	private:
		CFStringFormater	m_strFormater;
		//FTLINLINE void Init();
		FTLINLINE BOOL GetParametersInfo();
	};

	struct VERSIONINFO
	{
		VERSIONINFO()
		{
			dwVer0 = 0;
			dwVer1 = 0;
			dwVer2 = 0;
			dwVer3 = 0;
		}
		bool operator < (const VERSIONINFO & other) const
		{
			COMPARE_MEM_LESS(dwVer0, other);
			COMPARE_MEM_LESS(dwVer1, other);
			COMPARE_MEM_LESS(dwVer2, other);
			COMPARE_MEM_LESS(dwVer3, other);
			return false;
		}
		DWORD dwVer0;
		DWORD dwVer1;
		DWORD dwVer2;
		DWORD dwVer3;
	};

    enum VirtualMachineType
    {
        vmtError = -2,
        vmtUnknown = -1,
        
        vmtReal,        //真实机器
        vmtVmWare,
        vmtVirtualPC,
        vmtVirtualBox,
    };
    struct PROCESS_INFO{
        DWORD dwSize;
        TCHAR szProcessName[MAX_PATH];
    };
    FTLEXPORT class CFSystemUtil
    {
    public:
		//从版本信息字符串里获取对应的版本号
		FTLINLINE static BOOL GetVersionFromString(VERSIONINFO& ver, LPCTSTR pszVersion);

		//获取当前登录的用户名
		FTLINLINE static BOOL GetCurrentUserID( LPTSTR pszUserName, int iSize );

		FTLINLINE static LPCTSTR GetClipboardFormatString(UINT uFormat, LPTSTR lpszFormatName, int cchMaxCount);

        FTLINLINE static BOOL Is64BitWindows();

        //static BOOL GetLocalizedDateString();
        //static BOOL GetLocalizedTimeString();
        FTLINLINE static BOOL SuspendProcess(DWORD dwProcessID,BOOL fSuspend, DWORD skipedThreadId = ::GetCurrentThreadId());
        
        //使用 ATL 的 CAccessToken 更方便
        //调整进程权限( EnableProcessPrivilege (GetCurrentProcess(),SE_SHUTDOWN_NAME) )
        //（要对一个任意进程（包括系统安全进程和服务进程）进行指定了写相关的访问权的OpenProcess操作，
        //只要当前进程具有SeDeDebug权限就可以了）
        FTLINLINE static BOOL EnableProcessPrivilege (HANDLE hProcess, LPCTSTR lpPrivilegeName = SE_DEBUG_NAME ,BOOL bEnabled = TRUE);

		//判断指定的Module所在进程是否是特殊的进程(如BHO中判断当前进程是IE)
		FTLINLINE static BOOL IsSpecialProcessName(LPCTSTR pszProcessName, HMODULE hModule = NULL);

        //枚举当前所有进程，并根据名字杀掉指定进程,支持 32位程序(如 安装包插件)中杀掉64位程序
        FTLINLINE static BOOL KillProcessByName(LPCTSTR pszProcName, BOOL bAll, DWORD* pKillCount);

        //如果是拷贝位图的话，可以用 CF_BITMAP、CF_DIB 等
        FTLINLINE static BOOL CopyTextToClipboard ( LPCTSTR szMem , HWND hWndOwner = GetClipboardOwner());

		//GetTickCount返回值是整数, 最多49天多就会复位重新从0开始, 微软在Vista后提供了GetTickCount64函数
		//实现在WinXP上也能使用的 GetTickCount64 -- 虽然和真正的相比会差 800 毫秒左右(是否是测试的方法有问题?)
		FTLINLINE static ULONGLONG GetTickCount64();

		//获取第一个指定进程名的PID
		FTLINLINE static DWORD GetPidFromProcessName(LPCTSTR pszProcesName);

        //获取指定进程的父进程PID，如找不到父进程PID，则返回 -1
        //TODO:1.利用了非公开的函数，可能会变
        //     2.获取到的父进程可能已经被关闭，或者其PID已经被重用。有什么方法解决PID重用?
        //     3.尚未测试64位系统
        FTLINLINE static DWORD GetParentProcessId(DWORD dwPID, BOOL bCheckParentExist = TRUE);

        //获取Process相关的信息
		FTLINLINE static BOOL GetProcessInfoByProcessHandle(HANDLE hProcess, PROCESS_INFO* pProcessInfo);
        FTLINLINE static BOOL GetProcessInfoByPid(DWORD dwPid, PROCESS_INFO* pProcessInfo);

        FTLINLINE static BOOL CreateProcessAndWaitAllChild(LPCTSTR pszPath, 
            LPTSTR pszCommandLine = NULL, LPDWORD lpExitCode = NULL);

        //判断当前系统是大端系统还是小端系统(操作系统有没有方式) -- intel的X86是小端，Apple的RISC是大端
        //大端模式 -- 数据的低位保存在内存的高地址中，而数据的高位保存在内存的低地址中
        //  0x12345678 保存为 {低 0x12, 0x34, 0x56, 0x78 高}
        //小端模式 -- 数据的低位保存在内存的低地址中，而数据的高位保存在内存的高地址中
        //  0x12345678 保存为 {低 0x78, 0x56, 0x34, 0x12 高}
        //
        //实际上在 short 中保存 0x0102 就可以了
        //union 
        //{
        //    short s;
        //    char c[sizeof(short)];
        //}un;
        //un.s = 0x0102; //if(1==un.c[0] && 2==un.c[1]){big;}
        FTLINLINE static BOOL IsLittleSystem();

		//TODO:测试是否有 C/C++ 的方式
		//检测是否在虚拟机(Virtual PC/VMWare)上运行 -- http://download.csdn.net/detail/noliver/1380310
        //汇编方式:http://social.msdn.microsoft.com/Forums/vstudio/en-US/ec2b66ec-f1a3-49fb-a8df-329965239284/safeseh-and-assembler-code
		//VB 下：GetObject("winmgmts:\\" & strComputer & "\root\cimv2")
		//  SELECT * FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled = True
		//  Left(objAdapter.Description,2）  ==  VM
#if defined(_M_IX86)
        FTLINLINE static BOOL IsInsideVPC();
        FTLINLINE static BOOL IsInsideVirtualBox();
		FTLINLINE static BOOL IsInsideVMWare();

        //http://www.oschina.net/code/snippet_222150_16772
        FTLINLINE static VirtualMachineType CheckRunningMachineType();
#endif 
        FTLINLINE static BOOL IsRunningOnRemoteDesktop();

        //控制是否阻止由操作系统发起的挂起操作
        FTLINLINE static EXECUTION_STATE EnablePowerSuspend(BOOL bEnabled);

        //把Dos的结束符转换为Unix的结束符(去掉其中的 "\r" )
        FTLINLINE static int DosLineToUnixLine(const char *src, char *dest, int maxlen);
        //把Unix的结束符转换为Dos的结束符(将 "\r" 改成 "\r\n" )
        FTLINLINE static int UnixLineToDosLine(const char *src, char *dest, int maxlen);
    };

    class CFSystemMetricsProperty
    {
    public:
        #define SYSTEM_METRICS_PROPERTY_GET_DEFAULT     0x0000FFFF
    public:
        FTLINLINE CFSystemMetricsProperty();
        FTLINLINE ~CFSystemMetricsProperty();
        FTLINLINE LPCTSTR GetPropertyString(DWORD dwPropertyGet = SYSTEM_METRICS_PROPERTY_GET_DEFAULT);
    public:
        int     m_cxScreen;
        int     m_cyScreen;
        int     m_cxVScroll;
        int     m_cyHScroll;
        int     m_cyCaption;
        int     m_cxBorder;
        int     m_cyBorder;
        int     m_cxDlgFrame;
        int     m_cyDlgFrame;
        int     m_cxHThumb;
        int     m_cyVThumb;
        int     m_cxIcon;
        int     m_cyIcon;
        int     m_cxCursor;
        int     m_cyCursor;
        int     m_cyMenu;
        int     m_cxFullScreen;
        int     m_cyFullScreen;
        int     m_cyKanjiWindow;
        int     m_MousePresent;
        int     m_cyVscroll;
        int     m_cxHscroll;
        int     m_Debug;
        int     m_SwapButton;
        int     m_Reserved1;
        int     m_Reserved2;
        int     m_Reserved3;
        int     m_Reserved4;
        int     m_cxMin;
        int     m_cyMin;
        int     m_cxSize;
        int     m_cySize;
        int     m_cxFrame;
        int     m_cyFrame;
        int     m_cxMinTrack;
        int     m_cyMinTrack;
        int     m_cxDoubleClk;
        int     m_cyDoubleClk;
        int     m_cxIconSpacing;
        int     m_cyIconSpacing;
        int     m_MenuDropAlignment;
        int     m_PenWindows;
        int     m_DBCSEnabled;
        int     m_CMouseButtons;

#if(WINVER >= 0x0400)
        int     m_cxFixedFrame;//           SM_CXDLGFRAME  /* ;win40 name change */
        int     m_cyFixedframe;//           SM_CYDLGFRAME  /* ;win40 name change */
        int     m_cxSizeFrame;//            SM_CXFRAME     /* ;win40 name change */
        int     m_cySizeFrame;//            SM_CYFRAME     /* ;win40 name change */

        int     m_Secure;
        int     m_cxEdge;
        int     m_cyEdge;
        int     m_cxMinSpacing;
        int     m_cyMinSpacing;
        int     m_cxSMIcon;
        int     m_cySMIcon;
        int     m_cySMCaption;
        int     m_cxSMSize;
        int     m_cySMSize;
        int     m_cxMenuSize;
        int     m_cyMenuSize;
        int     m_Arrange;
        int     m_cxMinimized;
        int     m_cyMinimized;
        int     m_cxMaxTrack;
        int     m_cyMaxTrack;
        int     m_cxMaximized;
        int     m_cyMaximized;
        int     m_Network;
        int     m_CleanBoot;            //how the system is started
        int     m_cxDrag;
        int     m_cyDrag;

#endif /* WINVER >= 0x0400 */

        int     m_ShowSounds;

#if(WINVER >= 0x0400)
        int     m_cxMenuCheck;
        int     m_cyMenuCheck;
        int     m_SlowMachine;          //low-end (slow) processor
        int     m_MideaStenabled;       //enabled for Hebrew and Arabic languages
#endif /* WINVER >= 0x0400 */

#if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
        int     m_MouseWheelPresent;
#endif

#if(WINVER >= 0x0500)
        int     m_XVirtualScreen;
        int     m_YVirtualScreen;
        int     m_cxVirtualScreen;
        int     m_cyVirtualScreen;
        int     m_CMonitors;
        int     m_SameDisplayFormat;
#endif /* WINVER >= 0x0500 */

#if(_WIN32_WINNT >= 0x0500)
        int     m_IMMEnabled;
#endif /* _WIN32_WINNT >= 0x0500 */

#if(_WIN32_WINNT >= 0x0501)
        int     m_cxFocusBorder;
        int     m_cyFocusBorder;
#endif /* _WIN32_WINNT >= 0x0501 */

#if(_WIN32_WINNT >= 0x0501)
        int     m_TabletPc;             //Windows XP Tablet PC edition
        int     m_MediaCenter;          //Windows XP Media Center Edition
        int     m_Starter;              //Windows XP Starter Edition
        int     m_Serverr2;             //Windows Server 2003 R2
#endif /* _WIN32_WINNT >= 0x0501 */

#if(_WIN32_WINNT >= 0x0600)
        int     m_MouseHorizontalWheelPresent;
        int     m_cxPaddedBorder;
#endif /* _WIN32_WINNT >= 0x0600 */

        int     m_CMetrics;

#if(WINVER >= 0x0500)
        int     m_RemoteSession;
#  if(_WIN32_WINNT >= 0x0501)
        int     m_ShuttingDown;
#  endif /* _WIN32_WINNT >= 0x0501 */
#  if(WINVER >= 0x0501)
        int     m_RemoteControl;
#  endif /* WINVER >= 0x0501 */
#  if(WINVER >= 0x0501)
        int     m_CaretBlinkingEnabled;
#  endif /* WINVER >= 0x0501 */
#endif /* WINVER >= 0x0500 */

    private:
        CFStringFormater	m_strFormater;
        DWORD   m_dwOldGetProperty;
    };

    class CFSecurityUtil
    {
        //创建一个任何人都可以使用的Mutex -- ATL中已有函数？

        //SECURITY_DESCRIPTOR sd;
        //BOOL ret = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        //ret = SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE);
        //SECURITY_ATTRIBUTES sa;
        //sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        //sa.lpSecurityDescriptor = &sd;
        //sa.bInheritHandle = FALSE;
        //::CreateMutex(&sa, FALSE, NULL);
    };

    class CFTempFpuRcReset  //FPU的四舍五入设定 -- 能提高有效位数？
    {
    public:
        CFTempFpuRcReset()
            :m_oldVal(_control87(0, 0))
        {
            //_control87(_EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT,  _MCW_EM);
            //_control87(_PC_53, _MCW_PC);

            //_control87(_RC_CHOP, _MCW_RC);
            _control87(_RC_NEAR, _MCW_RC);

        }
        ~CFTempFpuRcReset()
        {
            _control87(m_oldVal, _MCW_RC);
        }
    private:
        //禁止拷贝构造和赋值函数
        CFTempFpuRcReset& operator= (const CFTempFpuRcReset&);
        CFTempFpuRcReset(CFTempFpuRcReset&);
        //! 保存以前的值
        const unsigned int m_oldVal;
    };

    //构造函数是无参数的，有参数的如何办？
    //两个参数:
    //  一个是插件的指针类型，一个是插件指针的释放方式，如采用 delete 或其对应的 Release 等
    template <typename T, typename F = FTL::ObjectDeleter<T*> >
    class CFPluginMgrT
    {
    public:
        FTLINLINE BOOL Init(LPCTSTR pszPluginPath, LPCSTR pszProcName, LPCTSTR pszExtName= TEXT("*.dll"));
        FTLINLINE void UnInit();
        FTLINLINE BOOL LoadPlugin(LPCTSTR pszePlugPath, LPCSTR pszProcName);
        FTLINLINE ~CFPluginMgrT();
    public:
        typedef std::list<T*>       PluginContainer;
        typedef std::list<HMODULE>  ModuleContainer;
        PluginContainer         m_plugins;
        ModuleContainer         m_modules;
    private:
        typedef T* (* FunGetInstance )();
    };

//    class CFStringUtil
//    {
//    public:
//        static HRESULT ConvertString2LONG(LPCTSTR pszValue, BOOL bIsHex, LONG* pValue)
//        {
//            CHECK_POINTER_WRITABLE_DATA_RETURN_VALUE_IF_FAIL(pValue,sizeof(LONG), E_POINTER);
//            UNREFERENCED_PARAMETER(pszValue);
//            UNREFERENCED_PARAMETER(bIsHex);
//            UNREFERENCED_PARAMETER(pValue);
//#if 0
//            int nIndex;
//            int nPower = 0;
//            int nPowerDec = 1;
//            int nDigit = 0;
//            *pValue = 0;
//
//            StringCchLength(pszValue, MAX_PATH, (size_t *)&nIndex);
//            nIndex--;
//
//            // skip whitespace
//            while( nIndex >= 0 )
//            {
//                if( pszValue[nIndex] != TEXT(' ') && 
//                    ( pszValue[nIndex] != TEXT('L') || pszValue[nIndex] != TEXT('l')))
//                    break;
//                nIndex--;
//            }
//
//            bool bFoundDigit = false;
//            while( nIndex >= 0 )
//            {
//                // Convert to uppercase
//                if( pszValue[nIndex] >= 'a' && pszValue[nIndex] <= 'z' )
//                    pszValue[nIndex] += 'A' - 'a';
//
//                if( bIsHex && pszValue[nIndex] >= 'A' && pszValue[nIndex] <= 'F' )
//                    nDigit = pszValue[nIndex] - 'A' + 10;
//                else if( pszValue[nIndex] >= '0' && pszValue[nIndex] <= '9' )
//                    nDigit = pszValue[nIndex] - '0';
//                else if( pszValue[nIndex] == '-' )
//                {
//                    *pValue = -*pValue;
//                    break;
//                }
//                else
//                {
//                    // break if we've found a number, but don't break otherwise
//                    // This will skip any random letters in the front & end of the string
//                    if( bFoundDigit )
//                    {
//                        break;
//                    }
//                    else
//                    {
//                        nIndex--;
//                        continue;
//                    }
//                }
//
//                bFoundDigit = true;
//                if( bIsHex )
//                    *pValue += ( nDigit << (nPower*4) );
//                else
//                    *pValue += ( nDigit * nPowerDec );
//
//                nPowerDec *= 10;
//                nIndex--;
//                nPower++;
//            }
//#endif 
//            return E_NOTIMPL;
//        }
//    };

}

#endif //FTL_SYSTEM_H

#ifndef USE_EXPORT
#  include "ftlSystem.hpp"
#endif