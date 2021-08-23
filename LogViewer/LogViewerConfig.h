#pragma once

#define SECTION_COMMON          TEXT("COMMON")
#define SECTION_REGMAP          TEXT("REGMAP")
#define SECTION_LEVEL_MAP       TEXT("LEVELMAP")


#define KEY_REGULAR             TEXT("REGULAR")
#define KEY_SRC_REGULAR         TEXT("SRC_REGULAR")
#define KEY_ITEM_SRC_FILE       TEXT("ITEM_SRC_FILE")

#define KEY_TIMEFORMAT          TEXT("TIME_FORMAT")
#define KEY_DISPLAY_TIMEFORMAT  TEXT("DISPLAY_TIME_FORMAT")
#define KEY_SOURCE_FILE_EXTS    TEXT("SOURCE_FILES_EXT")

#define KEY_ITEM_TIME           TEXT("ITEM_TIME")
#define KEY_ITEM_LEVEL          TEXT("ITEM_LEVEL")
#define KEY_ITEM_MACHINE        TEXT("ITEM_MACHINE")
#define KEY_ITEM_PID            TEXT("ITEM_PID")
#define KEY_ITEM_TID            TEXT("ITEM_TID")
#define KEY_ITEM_MODULE         TEXT("ITEM_MODULE")
#define KEY_ITEM_FUN            TEXT("ITEM_FUN")
#define KEY_ITEM_FILE           TEXT("ITEM_FILE")
#define KEY_ITEM_LINE           TEXT("ITEM_LINE")
#define KEY_ITEM_LOG            TEXT("ITEM_LOG")

#define KEY_LEVEL_DETAIL        TEXT("LEVEL_DETAIL")
#define KEY_LEVEL_INFO          TEXT("LEVEL_INFO")
#define KEY_LEVEL_TRACE         TEXT("LEVEL_TRACE")
#define KEY_LEVEL_WARN          TEXT("LEVEL_WARN")
#define KEY_LEVEL_ERROR         TEXT("LEVEL_ERROR")

#define DEFAULT_NULL_VALUE      TEXT("")
#define INVLIAD_ITEM_MAP        (-1)

enum DateTimeType{
    dttDateTime,
    dttDate,
    dttTime
};

class CLogViewerConfig
{
public:
    CLogViewerConfig(void);
    ~CLogViewerConfig(void);

public:
    BOOL LoadConfig(LPCTSTR pszConfigFile);
    FTL::TraceLevel GetTraceLevelByText(const std::string& strLevel);
private:
    BOOL _LoadItemMaps();
    BOOL _LoadLevelMaps();
    INT	_ConvertItemMapValue(const CString& strItemValue);
public:
    CFConfigIniFile m_config;
    CString         m_strLogRegular;
    CString         m_strSrcRegular;  //有可能文件需要正则表达式进行二次解析
    CString         m_strSourceFileExts;
    CString         m_strTimeFormat;
    CString         m_strDisplayTimeFormat;
    DateTimeType    m_dateTimeType;

    INT m_nItemTime;
    INT m_nItemLevel;
    INT m_nItemMachine;
    INT m_nItemPId;
    INT m_nItemTId;
    INT m_nItemModule;
    INT m_nItemFun;
    INT m_nItemFile;
    INT m_nItemLine;
    INT m_nItemLog;

    //二次分析时中的源文件路径
    INT m_nItemSrcFileEx;

    std::string m_strLevelsText[FTL::tlEnd];
};
