#include "StdAfx.h"
#include "LogViewerConfig.h"

struct ItemMapInfo  
{
    LPCTSTR         pszKeyItem;
    INT*            pIntItemIndex;
    CAtlStringA*    pStrItemIndex;
};

struct LevelMapInfo
{
    LPCTSTR                 pszLevelItem;
    LevelsTextContainer*    pstrLevelText;
};

CLogViewerConfig::CLogViewerConfig(void)
{
    m_nItemSeqNum = INVLIAD_ITEM_MAP;
    m_nItemTime = INVLIAD_ITEM_MAP;
    m_nItemLevel = INVLIAD_ITEM_MAP;
    m_nItemMachine = INVLIAD_ITEM_MAP;
    m_nItemPId = INVLIAD_ITEM_MAP;
    m_nItemTId = INVLIAD_ITEM_MAP;
    m_nItemModule = INVLIAD_ITEM_MAP;
    m_nItemFun = INVLIAD_ITEM_MAP;
    m_nItemFile = INVLIAD_ITEM_MAP;
    m_nItemLine = INVLIAD_ITEM_MAP;
    m_nItemLog = INVLIAD_ITEM_MAP;
    m_nItemRegBody = INVLIAD_ITEM_MAP;

    m_nItemSrcFileEx = INVLIAD_ITEM_MAP;

    m_dateTimeType = dttTimeMilliSecond;  //TODO: �ĳ�Ĭ��ֻ��ʾʱ��(���̫��)
    m_nEnableFullLog = 0;  //if enable this, then can copy full log, but it will use double memory
    m_nMaxLineLength = 4096;
}

CLogViewerConfig::~CLogViewerConfig(void)
{
}

BOOL CLogViewerConfig::LoadConfig(LPCTSTR pszConfigFile)
{
    BOOL bRet = TRUE;
    API_VERIFY(m_config.SetFileName(pszConfigFile));
    if (m_config.IsIniFileExist())
    {
        CString strReadOrder;
        m_config.GetString(SECTION_COMMON, KEY_ORDER, DEFAULT_NULL_VALUE, strReadOrder);
        if (0 == strReadOrder.CompareNoCase(TEXT("DESC")))
        {
            m_readItemOrder = Desc;
        }
        else {
            m_readItemOrder = Asc;  //Ĭ������
        }

        m_config.GetString(SECTION_COMMON, KEY_REGULAR, DEFAULT_NULL_VALUE, m_strLogRegular);
        m_config.GetString(SECTION_COMMON, KEY_REGULAR_2, DEFAULT_NULL_VALUE, m_strLogRegular_2);

        m_config.GetString(SECTION_COMMON, KEY_SOURCE_FILE_EXTS, _T("*.*"), m_strSourceFileExts);
        m_config.GetString(SECTION_COMMON, KEY_OPEN_COMMAND, DEFAULT_NULL_VALUE, m_strOpenCommand);

        m_config.GetString(SECTION_COMMON, KEY_TIMEFORMAT, DEFAULT_NULL_VALUE, m_strTimeFormat);

        m_nEnableFullLog = (INT)m_config.GetInt(SECTION_COMMON, KEY_ENABLE_FULL_LOG, m_nEnableFullLog);
        m_nMaxLineLength = (INT)m_config.GetInt(SECTION_COMMON, MAX_LINE_LENGTH, m_nMaxLineLength);

        m_config.GetString(SECTION_COMMON, KEY_SRC_REGULAR, DEFAULT_NULL_VALUE, m_strSrcRegular);

        CString strSrcFileItem;
        m_config.GetString(SECTION_COMMON, KEY_ITEM_SRC_FILE, DEFAULT_NULL_VALUE, strSrcFileItem);
        if (!strSrcFileItem.IsEmpty())
        {
            m_nItemSrcFileEx = _ConvertItemMapValue(strSrcFileItem);
        }

        _LoadItemMaps();
        _LoadLevelMaps();
    }

    return bRet;
}

FTL::TraceLevel CLogViewerConfig::GetTraceLevelByText(const std::string& strLevel)
{
    FTL::TraceLevel level = tlTrace;
    for (int i = 0; i < _countof(m_strLevelsText); i++)
    {
        if (m_strLevelsText[i].find(strLevel) != m_strLevelsText[i].end())
        {
            level = FTL::TraceLevel(i);
            break;
        }
    }
    return level;
}


BOOL CLogViewerConfig::_LoadItemMaps()
{
    BOOL bRet = TRUE;

    ItemMapInfo itemMapInfos[] = {
        { KEY_ITEM_SEQNUM, &m_nItemSeqNum, &m_strItemSeqNum },
        { KEY_ITEM_TIME, &m_nItemTime, &m_strItemTime },
        { KEY_ITEM_LEVEL, &m_nItemLevel, &m_strItemLevel },
        { KEY_ITEM_MACHINE, &m_nItemMachine, &m_strItemMachine },
        { KEY_ITEM_PID,	 &m_nItemPId, &m_strItemPId },
        { KEY_ITEM_TID,	 &m_nItemTId, &m_strItemTId },
        { KEY_ITEM_MODULE, &m_nItemModule, &m_strItemModule},
        { KEY_ITEM_FUN, &m_nItemFun, &m_strItemFun},
        { KEY_ITEM_FILE, &m_nItemFile, &m_strItemFile},
        { KEY_ITEM_LINE, &m_nItemLine, &m_strItemLine},
        { KEY_ITEM_LOG, &m_nItemLog, &m_strItemLog},
        { KEY_ITEM_REG_BODY, &m_nItemRegBody, &m_strItemRegBody},
    };

    ItemMapInfo itemMapInfos_2[] = {
		{ KEY_ITEM_FILE_2, &m_nItemFile_2 },
		{ KEY_ITEM_LINE_2, &m_nItemLine_2 },
	};

    CString strItemValue;
    for (int i = 0; i < _countof(itemMapInfos); i++)
    {
        if (m_config.GetString(SECTION_REGMAP, itemMapInfos[i].pszKeyItem, DEFAULT_NULL_VALUE, strItemValue) > 0) {
            // regex
            *itemMapInfos[i].pIntItemIndex = _ConvertItemMapValue(strItemValue);
        }
        if (m_config.GetString(SECTION_JSONMAP, itemMapInfos[i].pszKeyItem, DEFAULT_NULL_VALUE, strItemValue) > 0) {
            // json
            CAtlStringA strV = _ConvertJsonMapValue(strItemValue);
            *itemMapInfos[i].pStrItemIndex = strV;
        }
    }

	for (int i = 0; i < _countof(itemMapInfos_2); i++)
	{
		m_config.GetString(SECTION_REGMAP, itemMapInfos_2[i].pszKeyItem, DEFAULT_NULL_VALUE, strItemValue);
		if (!strItemValue.IsEmpty()) {
			*itemMapInfos_2[i].pIntItemIndex = _ConvertItemMapValue(strItemValue);
		}
	}

    return bRet;
}

BOOL CLogViewerConfig::_LoadLevelMaps()
{
    BOOL bRet = FALSE;

    LevelMapInfo levelMapInfos[] = {
        { KEY_LEVEL_DETAIL, &m_strLevelsText[FTL::tlDetail]},
        { KEY_LEVEL_INFO, &m_strLevelsText[FTL::tlInfo]},
        { KEY_LEVEL_TRACE,	 &m_strLevelsText[FTL::tlTrace]},
        { KEY_LEVEL_WARN, &m_strLevelsText[FTL::tlWarn]},
        { KEY_LEVEL_ERROR, &m_strLevelsText[FTL::tlError]},
    };

    FTL::CFConversion conv;
    //CString strItemValue;
    for (int i = 0; i < _countof(levelMapInfos); i++)
    {
        CString strLevelValue;
        m_config.GetString(SECTION_LEVEL_MAP, levelMapInfos[i].pszLevelItem, DEFAULT_NULL_VALUE, strLevelValue);
        if (!strLevelValue.IsEmpty())
        {
            std::list<CAtlString> strLevelTexts;
            FTL::Split(strLevelValue, TEXT("|"), false, strLevelTexts);
            
            for (std::list<CAtlString>::iterator iter = strLevelTexts.begin();
                iter != strLevelTexts.end();
                ++iter) {
                levelMapInfos[i].pstrLevelText->insert(conv.TCHAR_TO_UTF8(*iter));
            }
        }
    }

    return bRet;
}

INT CLogViewerConfig::_ConvertItemMapValue(const CString& strItemValue){
    INT nItemValue = -1;
    if (!strItemValue.IsEmpty())
    {
        if (strItemValue.GetLength() >= 2 && strItemValue[0]==_T('$'))
        {
            nItemValue = _ttoi((LPCTSTR)strItemValue.Mid(1));
        }
    }
    return nItemValue;
}

CAtlStringA CLogViewerConfig::_ConvertJsonMapValue(const CString& strItemValue) {
    FTL::CFConversion conv;
    if (!strItemValue.IsEmpty())
    {
        if (strItemValue.GetLength() >= 2 && strItemValue[0] == _T('$'))
        {
            CString strSub = strItemValue.Mid(1);
            LPCSTR pszUTF8 = (LPCSTR)conv.TCHAR_TO_UTF8((LPCTSTR)strSub);
            CAtlStringA strUtf8 = pszUTF8;
           
            return strUtf8;
        }
    }
    return CAtlStringA("");
}