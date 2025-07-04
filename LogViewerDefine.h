#pragma once

#include <deque>
#include <ftlbase.h>
//#include <ftlThread.h>
#include <list>
#include <ftlSharePtr.h>

#define INVALID_LINE_INDEX          (UINT)(-1)
#define INVALID_SEQ_NUMBER          (LONG)(-1)
//time ���ڵĵ�λ�� FILETIME(100ns)
#define TIME_RESULT_TO_MILLISECOND  (1000 * 1000 * 10)
#define MICROSECOND_PER_SECOND      (1000 * 1000)
#define MIN_TIME_WITH_DAY_INFO      ((LONGLONG)24 * 3600 * TIME_RESULT_TO_MILLISECOND) 
#define DEFAULT_LOCAL_MACHINE       TEXT("local")

// machine => PID => TID configuration
#define MPT_TREE_ROOT				TEXT("All")
#define MPT_TREE_ROOT_PATH			TEXT("/All/")

//�� filter item �б���ͨ���Ҽ�ѡ��ֻ��ʾָ�� pid/tid ����־ => ���� machinePidTid ��ͼ. TODO: �Ƿ��и��õķ�ʽ?
#define VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_PID      1001
#define VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_TID      1002
#define VIEW_UPDATE_HINT_FILTER_BY_FILENAME        1003
#define VIEW_UPDATE_HINT_SELECT_LINE_INDEX         1004

enum ONLY_SELECT_TYPE{
    ostMachine,
    ostProcessId,
    ostThreadId,
};

typedef std::wstring MACHINE_NAME_TYPE;
typedef std::wstring THREAD_ID_TYPE;
typedef std::wstring PROCESS_ID_TYPE;

struct MachinePIdTIdType{
    MachinePIdTIdType() {
        //all default value is "";
    }

    MachinePIdTIdType(MACHINE_NAME_TYPE machine, PROCESS_ID_TYPE pid, THREAD_ID_TYPE tid){
        this->machine = machine;
        this->pid = pid;
        this->tid = tid;
    }
    MACHINE_NAME_TYPE machine;
    PROCESS_ID_TYPE pid;
    THREAD_ID_TYPE  tid;
    bool operator < (const MachinePIdTIdType & other) const{
        int machineCompare = machine.compare(other.machine);
        if (machineCompare < 0) { 
            return true;
        } else if(machineCompare > 0){
            return false;
        }

        int pidCompare = pid.compare(other.pid);
        if( pidCompare < 0){
            return true;
        }else if(pidCompare > 0){
            return false;
        }

        int tidCompare = tid.compare(other.tid);
        if( tidCompare < 0){
            return true;
        }else if(tidCompare > 0){
            return false;
        }
        return false;
    }
};

struct ID_INFOS{
    BOOL        bInited;
    BOOL        bChecked;
    LONGLONG    lastTimeStamp;
    ID_INFOS(){
        bInited = FALSE;
        bChecked = TRUE;
        lastTimeStamp = 0;
    }
};

typedef std::map<THREAD_ID_TYPE, ID_INFOS>      TidContainer;
typedef TidContainer::iterator        TidContainerIter;

typedef std::map<PROCESS_ID_TYPE, TidContainer>     PidTidContainer;
typedef PidTidContainer::iterator             PidTidContainerIter;

typedef std::map<MACHINE_NAME_TYPE, PidTidContainer>    MachinePidTidContainer;
typedef MachinePidTidContainer::iterator          MachinePidTidContainerIter;
typedef MachinePidTidContainer::const_iterator    MachinePidTidContainerConstIter;

// ���ڹ����û�ѡ��Ķ�����¼
typedef std::list<MachinePIdTIdType>    MachinePIdTIdTypeList;

//���ڱ�����е�����Ϣ
typedef std::list<int> LogIndexContainer;
typedef LogIndexContainer::iterator LogIndexContainerIter;

struct LogItem
{
    LONG                size;               //LogItem�Ĵ�С sizeof
    LONG                lineNum;            //�ļ��е��к�
    LONG                seqNum;             //���к�(FTL��ʹ��)
    LONG                moduleNameLen;      //ģ�����ֵĳ���
    LONG                traceInfoLen;       //pszTraceInfo �ĳ��ȣ�Ŀǰ������ pszTraceInfo �ַ�������+1(������β��NULL,���˷ѿռ�)
    LONG                srcFileline;        //��Դ�ļ��е��к�
    LONGLONG            time;                //���� ns(����), ��ֵ /10 �Ժ���� FILETIME ��Ӧ��ֵ(100ns)
    LONGLONG            elapseTime;
    MACHINE_NAME_TYPE   machine;            //������(һ������dsh)
    PROCESS_ID_TYPE     processId;
    THREAD_ID_TYPE      threadId;
    FTL::TraceLevel     level;
    std::string         orgTimeStr;         //����ԭʼ������/ʱ���ַ���,��Ҫ��Ϊ�� json ���ʱʹ��.
    std::string         orgLevelStr;
    //HMODULE           module;
    LPCWSTR             pszFunName;
    LPCWSTR             pszModuleName;
    LPCWSTR             pszTraceInfo;       //�����ַ�����ָ��ʼ����WCHAR������
    LPCTSTR             pszSrcFileName;     //Դ�ļ���·��
    LPCWSTR             pszFullLog;         //������־

    LogItem()
    {
        size = 0;
        lineNum = 0;
        seqNum = INVALID_SEQ_NUMBER;
        moduleNameLen = 0;
        traceInfoLen = 0;
        srcFileline = 0;
        time = 0;
        elapseTime = 0;
        machine = DEFAULT_LOCAL_MACHINE;
        processId = TEXT("0");
        threadId = TEXT("0");
        level = FTL::tlEnd;
        orgTimeStr = "";
		orgLevelStr = "";
        pszFunName = NULL;
        pszModuleName = NULL;
        pszTraceInfo = NULL;
        pszSrcFileName = NULL;
        pszFullLog = NULL;
    }
    ~LogItem()
    {
        SAFE_DELETE_ARRAY(pszFunName);
        SAFE_DELETE_ARRAY(pszModuleName);
        SAFE_DELETE_ARRAY(pszTraceInfo);
        SAFE_DELETE_ARRAY(pszSrcFileName);
        SAFE_DELETE_ARRAY(pszFullLog);
    }
    DISABLE_COPY_AND_ASSIGNMENT(LogItem);
};

typedef CFSharePtr<LogItem>             LogItemPointer;
typedef std::deque< LogItemPointer >    LogItemsContainer;
typedef LogItemsContainer::iterator     LogItemsContainerIter;