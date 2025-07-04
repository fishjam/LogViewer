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

//json �Ƚ����� �����ֱ�ӽ�������־��, ��ô������ ITEM_LOG, 
//  �������־��ܸ���, ����Ҫ���� ITEM_REG_BODY, Ȼ����ͨ�� ������ʽ����.
#define KEY_ITEM_LOG            TEXT("ITEM_LOG")
#define KEY_ITEM_REG_BODY       TEXT("ITEM_REG_BODY")   //TODO: ��δʵ��

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

// ��ȡ ��־���˳��(ĿǰӦ���� json )
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

    dttDateTimeNone,   //�û�û�����ù���ʱ��
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
    ReadItemOrder   m_readItemOrder; //��ʾ��־��˳��(��Ҫ�漰��ʱ����ļ���)
    CString         m_strLogRegular;  //����������־ʱ������
	CString         m_strLogRegular_2; //���� Go/Java �ȵ��ö�ջʱ������

    CString         m_strSrcRegular;  //�п����ļ���Ҫ������ʽ���ж��ν���
    CString         m_strSourceFileExts;
    CString         m_strOpenCommand;
    CString         m_strTimeFormat;
    DateTimeType    m_dateTimeType;

    //if enable this, then can copy full log, but it will use double memory
    INT m_nEnableFullLog;       //�Ƿ����ȫ����־.
    INT m_nMaxLineLength;       //ÿ�е���󳤶�,�����Ļ����Ͳü�

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

	//���ڷ��� Go/Java �ȵ��ö�ջʱ�� �ļ�·��+�к�
	INT m_nItemFile_2;
	INT m_nItemLine_2;



    //���η���ʱ�е�Դ�ļ�·��(��� mybox ���ֳ�����е��ػ�)
    INT m_nItemSrcFileEx;

    //֧�ֶ���� "|" �����ĵȼ��ַ���
    LevelsTextContainer m_strLevelsText[FTL::tlEnd];
};
