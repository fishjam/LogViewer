#include "StdAfx.h"
#include "VsIdeHandler.h"

CVsIdeHandler::CVsIdeHandler(void)
{
}

CVsIdeHandler::~CVsIdeHandler(void)
{
}

BOOL CVsIdeHandler::HadSelectedActiveIDE()
{
    return (NULL != m_pActiveIEnvDTE);
}

HRESULT CVsIdeHandler::StartFindStudio()
{   
    FTLASSERT(NULL == m_pIRunningObjectTable);
    FTLASSERT(NULL == m_pEnumMoniker);

    HRESULT hr = E_FAIL;
    COM_VERIFY(::GetRunningObjectTable( 0, &m_pIRunningObjectTable ));
    if (SUCCEEDED(hr))
    {
        COM_VERIFY(m_pIRunningObjectTable->EnumRunning( &m_pEnumMoniker ));
        if (FAILED(hr))
        {
            m_pIRunningObjectTable.Release();
        }
    }
    return hr;
}

HRESULT CVsIdeHandler::FindNextStudio(StudioInfo* pInfo)
{
    FTLASSERT(NULL != m_pEnumMoniker);
    USES_CONVERSION;
    HRESULT hr = E_FAIL;
    while (TRUE)
    {
        ULONG celtFetched;
        CComPtr< IMoniker > pIMoniker;
        COM_VERIFY_EXCEPT1(m_pEnumMoniker->Next( 1, &pIMoniker, &celtFetched ),S_FALSE);
        if (S_OK != hr)
        {
            return hr;
        }
        CComPtr< IBindCtx > pIBindCtx; 
        COM_VERIFY(::CreateBindCtx( NULL, &pIBindCtx )); 

        LPOLESTR pszDisplayName;
        COM_VERIFY(pIMoniker->GetDisplayName( pIBindCtx, NULL, &pszDisplayName ));
        TRACE( "Moniker %s\n", W2T( pszDisplayName ) );
        CString strDisplayName( pszDisplayName );
        ::CoTaskMemFree(pszDisplayName);
        //Find Studio
        if ( strDisplayName.Right( 4 ) == _T(".sln") 
            || strDisplayName.Find( _T("VisualStudio.DTE") ) >= 0 )
        {
            CComPtr<IUnknown> pIUnknown;
            COM_VERIFY(m_pIRunningObjectTable->GetObject( pIMoniker, &pIUnknown ));
            CComPtr< EnvDTE::_DTE > pIEnvDTE;
            pIUnknown->QueryInterface(__uuidof(pIEnvDTE), (void**)&pIEnvDTE );
            if( pIEnvDTE )
            {
                pInfo->pStudioIDE = pIUnknown.Detach();
                pInfo->strDisplayName = strDisplayName;
                break;
            }
        }
    }
    return hr;
}

HRESULT CVsIdeHandler::CloseFind()
{
    FTLASSERT(m_pEnumMoniker);
    m_pEnumMoniker.Release();
    m_pIRunningObjectTable.Release();
    return S_OK;
}

HRESULT CVsIdeHandler::SetActiveIDE(IUnknown* pUnknown)
{
    FTLASSERT(pUnknown);
    HRESULT hr = E_FAIL;
    CComPtr< EnvDTE::_DTE > pTempIEnvDTE;
    COM_VERIFY(pUnknown->QueryInterface(&pTempIEnvDTE));
    if (SUCCEEDED(hr))
    {
        m_pActiveIEnvDTE.Release();
        m_pActiveIEnvDTE = pTempIEnvDTE;
    }
    return hr;
}

HRESULT CVsIdeHandler::GoToLineInSourceCode(LPCTSTR pszFileName,int line)
{
    HRESULT hr = E_FAIL;
    if (m_pActiveIEnvDTE)
    {
        CComPtr< EnvDTE::Documents > pIDocuments;
        COM_VERIFY(m_pActiveIEnvDTE->get_Documents( &pIDocuments ));
        if ( !SUCCEEDED( hr ) )
        {
            return hr;
        }
        CComPtr< EnvDTE::Document > pIDocument;
        CComBSTR bstrFileName( pszFileName );
        CComVariant type=_T("Text");
        CComVariant read=_T("False");
        COM_VERIFY(pIDocuments->Open( bstrFileName, 
            type.bstrVal,
            read.bVal, 
            &pIDocument ));
        if ( !SUCCEEDED( hr ) )
        {
            return hr;
        }
        FTLASSERT( pIDocument.p );

        CComPtr< IDispatch > pIDispatch;
        hr = pIDocument->get_Selection( &pIDispatch );
        if ( !SUCCEEDED( hr ) )
            return hr;

        CComPtr< EnvDTE::TextSelection > pITextSelection;
        pIDispatch->QueryInterface( &pITextSelection );

        FTLASSERT( pITextSelection.p );

        hr = pITextSelection->GotoLine( line, TRUE );
        if ( !SUCCEEDED( hr ) )
            return hr;
        CComPtr<EnvDTE::Window> pMainWindow;
        hr = m_pActiveIEnvDTE->get_MainWindow(&pMainWindow);
        if (SUCCEEDED(hr))
        {
            pMainWindow->Activate();
        }
    }
    return hr;
}

HRESULT CVsIdeHandler::GoToFunctionLocation(LPCTSTR pszFunName)
{
    HRESULT hr = E_FAIL;
    if (m_pActiveIEnvDTE)
    {
    }
    return hr;
}