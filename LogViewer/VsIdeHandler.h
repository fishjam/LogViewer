#pragma once

#include <initguid.h>

#pragma warning( disable : 4278 )
#pragma warning( disable : 4146 )
#if _MSC_VER == 1200        //VC6
#  error NotSurpport
#elif _MSC_VER == 1310      //VS2003
#import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("7.0") lcid("0") raw_interfaces_only named_guids
#elif _MSC_VER == 1400      //VS2005
#  import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("8.0") lcid("0") raw_interfaces_only named_guids
#elif _MSC_VER == 1500      //VS2008
#  import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("9.0") lcid("0") raw_interfaces_only named_guids
#elif _MSC_VER == 1600      //VS2010
#  import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("10.0") lcid("0") raw_interfaces_only named_guids
#elif _MSC_VER == 1700      //VS2012
#  import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("11.0") lcid("0") raw_interfaces_only named_guids
#elif _MSC_VER == 1800      //VS2013
#  import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("12.0") lcid("0") raw_interfaces_only named_guids
#elif _MSC_VER == 1900      //VS2015
#  import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("13.0") lcid("0") raw_interfaces_only named_guids
#endif

#pragma warning( default : 4146 )
#pragma warning( default : 4278 )

struct StudioInfo
{
    CString     strDisplayName;
    IUnknown*   pStudioIDE;  
};
class CVsIdeHandler
{
public:
    CVsIdeHandler(void);
    ~CVsIdeHandler(void);
    HRESULT StartFindStudio();
    HRESULT FindNextStudio(StudioInfo* pInfo);
    HRESULT CloseFind();
    HRESULT GoToLineInSourceCode(LPCTSTR pszFileName,int line);
    HRESULT SetActiveIDE(IUnknown* pUnknown);
    BOOL    HadSelectedActiveIDE();
    VOID    ClearActiveIDE();
protected:
    CComPtr< IRunningObjectTable > m_pIRunningObjectTable;
    CComPtr< IEnumMoniker >   m_pEnumMoniker;
    CComPtr< EnvDTE::_DTE > m_pActiveIEnvDTE;
};
