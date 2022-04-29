#include "StdAfx.h"
#include "LogViewerConfig.h"

struct ItemMapInfo  
{
    LPCTSTR pszKeyItem;
    INT*	pItemIndex;
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

    m_nItemSrcFileEx = INVLIAD_ITEM_MAP;

    m_dateTimeType = dttDateTime;
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
        m_config.GetString(SECTION_COMMON, KEY_REGULAR, DEFAULT_NULL_VALUE, m_strLogRegular);
        m_config.GetString(SECTION_COMMON, KEY_SOURCE_FILE_EXTS, _T("*.*"), m_strSourceFileExts);
        m_config.GetString(SECTION_COMMON, KEY_TIMEFORMAT, DEFAULT_NULL_VALUE, m_strTimeFormat);
        m_config.GetString(SECTION_COMMON, KEY_DISPLAY_TIMEFORMAT, m_strTimeFormat, m_strDisplayTimeFormat);
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
        { KEY_ITEM_SEQNUM, &m_nItemSeqNum },
        { KEY_ITEM_TIME, &m_nItemTime },
        { KEY_ITEM_LEVEL, &m_nItemLevel},
        { KEY_ITEM_MACHINE, &m_nItemMachine},
        { KEY_ITEM_PID,	 &m_nItemPId},
        { KEY_ITEM_TID,	 &m_nItemTId},
        { KEY_ITEM_MODULE, &m_nItemModule},
        { KEY_ITEM_FUN, &m_nItemFun},
        { KEY_ITEM_FILE, &m_nItemFile},
        { KEY_ITEM_LINE, &m_nItemLine},
        { KEY_ITEM_LOG, &m_nItemLog},
    };

    CString strItemValue;
    for (int i = 0; i < _countof(itemMapInfos); i++)
    {
        m_config.GetString(SECTION_REGMAP, itemMapInfos[i].pszKeyItem, DEFAULT_NULL_VALUE, strItemValue);
        *itemMapInfos[i].pItemIndex = _ConvertItemMapValue(strItemValue);
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