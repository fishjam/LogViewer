#pragma once

#define SECTION_COMMON          TEXT("COMMON")
#define SECTION_REGMAP          TEXT("REGMAP")
#define SECTION_JSONMAP         TEXT("JSONMAP")
#define SECTION_LEVEL_MAP       TEXT("LEVELMAP")


#define KEY_ORDER               TEXT("ORDER")
#define VAL_ORDER_ASC           TEXT("ASC")
#define VAL_ORDER_DESC          TEXT("DESC")

#define KEY_REGULAR             TEXT("REGULAR")
#define KEY_REGULAR_2           TEXT("REGULAR_2")
#define KEY_SRC_REGULAR         TEXT("SRC_REGULAR")
#define KEY_ITEM_SRC_FILE       TEXT("ITEM_SRC_FILE")

#define KEY_TIMEFORMAT          TEXT("TIME_FORMAT")
#define KEY_SOURCE_FILE_EXTS    TEXT("SOURCE_FILES_EXT")

//example:
//  GoLand:   OPEN_COMMAND=D:\GoLand\bin\goland64.exe --line $ITEM_LINE $ITEM_FILE
//  EmEditor: OPEN_COMMAND=D:\EmEditor\EmEditor.exe /l $ITEM_LINE $ITEM_FILE
//  VSCode:   OPEN_COMMAND=D:\VSCode\Code.exe --goto $ITEM_FILE:$ITEM_LINE
#define KEY_OPEN_COMMAND        TEXT("OPEN_COMMAND")

#define KEY_ENABLE_FULL_LOG     TEXT("ENABLE_FULL_LOG")

#define MAX_LINE_LENGTH         TEXT("MAX_LINE_LENGTH")

//for REGMAP(regex) or JSONMAP(json)
#define KEY_ITEM_SEQNUM         TEXT("ITEM_SEQNUM")
#define KEY_ITEM_TIME           TEXT("ITEM_TIME")
#define KEY_ITEM_LEVEL          TEXT("ITEM_LEVEL")
#define KEY_ITEM_MACHINE        TEXT("ITEM_MACHINE")
#define KEY_ITEM_PID            TEXT("ITEM_PID")
#define KEY_ITEM_TID            TEXT("ITEM_TID")
#define KEY_ITEM_MODULE         TEXT("ITEM_MODULE")
#define KEY_ITEM_FUN            TEXT("ITEM_FUN")
#define KEY_ITEM_FILE           TEXT("ITEM_FILE")
#define KEY_ITEM_LINE           TEXT("ITEM_LINE")

//json 比较特殊 如果能直接解析出日志体, 那么就设置 ITEM_LOG, 
//  但如果日志体很复杂, 则需要设置 ITEM_REG_BODY, 然后再通过 正则表达式解析.
#define KEY_ITEM_LOG            TEXT("ITEM_LOG")
#define KEY_ITEM_REG_BODY       TEXT("ITEM_REG_BODY")   //TODO: 暂未实现

// for call stack
#define KEY_ITEM_FILE_2           TEXT("ITEM_FILE_2")
#define KEY_ITEM_LINE_2           TEXT("ITEM_LINE_2")

#define KEY_LEVEL_DETAIL        TEXT("LEVEL_DETAIL")
#define KEY_LEVEL_INFO          TEXT("LEVEL_INFO")
#define KEY_LEVEL_TRACE         TEXT("LEVEL_TRACE")
#define KEY_LEVEL_WARN          TEXT("LEVEL_WARN")
#define KEY_LEVEL_ERROR         TEXT("LEVEL_ERROR")

#define DEFAULT_NULL_VALUE      TEXT("")
#define INVLIAD_ITEM_MAP        (-1)

// 读取 日志项的顺序(目前应用于 json )
enum ReadItemOrder {
    Asc,
    Desc,
};

enum DateTimeType{
    dttDateTimeMilliSecond,
    dttDateTimeMicrosecond,
    dttDateTimeLast = dttDateTimeMicrosecond,

    dttTimeMilliSecond,
    dttTimeMicrosecond,

    dttDateTimeNone,   //用户没有设置过的时候
};

typedef std::set<std::string> LevelsTextContainer;

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
    CAtlStringA _ConvertJsonMapValue(const CString& strItemValue);
public:
    CFConfigIniFile m_config;
    ReadItemOrder   m_readItemOrder; //表示日志的顺序(主要涉及到时间戳的计算)
    CString         m_strLogRegular;  //分析正常日志时的正则
	CString         m_strLogRegular_2; //分析 Go/Java 等调用堆栈时的正则

    CString         m_strSrcRegular;  //有可能文件需要正则表达式进行二次解析
    CString         m_strSourceFileExts;
    CString         m_strOpenCommand;
    CString         m_strTimeFormat;
    DateTimeType    m_dateTimeType;

    //if enable this, then can copy full log, but it will use double memory
    INT m_nEnableFullLog;       //是否加载全部日志.
    INT m_nMaxLineLength;       //每行的最大长度,超过的话，就裁剪

    //regex map
    INT m_nItemSeqNum;
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
    INT m_nItemRegBody;  //dummy

    //json map
    CAtlStringA m_strItemSeqNum;
    CAtlStringA m_strItemTime;
    CAtlStringA m_strItemLevel;
    CAtlStringA m_strItemMachine;
    CAtlStringA m_strItemPId;
    CAtlStringA m_strItemTId;
    CAtlStringA m_strItemModule;
    CAtlStringA m_strItemFun;
    CAtlStringA m_strItemFile;
    CAtlStringA m_strItemLine;
    CAtlStringA m_strItemLog;
    CAtlStringA m_strItemRegBody;

	//用于分析 Go/Java 等调用堆栈时的 文件路径+行号
	INT m_nItemFile_2;
	INT m_nItemLine_2;



    //二次分析时中的源文件路径(针对 mybox 这种程序进行的特化)
    INT m_nItemSrcFileEx;

    //支持多个用 "|" 隔开的等级字符串
    LevelsTextContainer m_strLevelsText[FTL::tlEnd];
};
