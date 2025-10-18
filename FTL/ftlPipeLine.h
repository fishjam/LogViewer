///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ftlPipeline.h
/// @brief  Functional Template Library Pipeline Header File.
/// @author fujie
/// @version 0.6 
/// @date 08/20/2009
/// @defgroup ftlPipeline FTL Pipeline function and class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef FTL_PIPELINE_H
#define FTL_PIPELINE_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlPipeline.h requires ftlbase.h to be included first
#endif

#include "ftlThread.h"

#define MAX_PROGRESS     ((INT)(10000))

namespace FTL
{
    /*********************************************************************************************************************
    * 多处理器(单地址空间)
    *   对称多处理器技术(SMP)
    *   多核(CMP,Chip MultiProcessors，芯片内多处理器) -- 为单个物理计算机提供很大吞吐量；尽可能小地产生热量(减少功耗、节省电力)；
    *     集成更多功能在单个芯片上
    * 多计算机(多地址空间)
    *   分布式计算集群(Cluster)
    *
    * 分布式计算：
    *   通过连接性和相互配合促进协作；
    *   通过并行处理改善性能；
    *   通过复制改善可靠性和可用性；
    *   通过模块性改善可伸缩性和可移植性；
    *   通过动态配置和重配置改善可扩展性；
    *   通过资源共享和开发系统提高成本效用
    *
    * 爱立信(Ericsson)开发的Erlang语言，称为并行计算中的佼佼者，是一个带有操作系统的新开发平台，该平台上的并发机制、进程调度、
    *   内存管理、分布式计算、网络通信等功能完全独立于操作系统，原先是用于百万级电信交换机上，是一个开源软件(www.erlang.org)。
    *   但是语法比较古怪，其他很多语言也学习了Erlang的抽象模型，如 Python 的 Candygram，Lisp的ErLisp等。
    *   ★Erlang作者在语法设计上的考量已经超越了对编程习惯、设计模型层面上的取舍，转而上升为不同计算模型对语法结构的决定性影响★
    *     Erlang 是一种函数式语言(Functional language)，函数式语言的变量一旦绑定一个值，就不能更改，因此能具有出众的并发特性和简介的
    *     分布式处理机制。不像普通的命令式语言(Impretive language)
    *   Erlang的特性：
    *     并发性：Erlang的轻量级进程(Light Weight Processes)可以支持极高的并发性，并且在高并发下其内存使用相当少，并发数并不依赖于
    *       宿主操作系统的并发性能限制。使用Erlang实现的Yaws(Web服务器)可以高达80000并发会话连接。
    *       其并发实体是与操作系统的本地线程(Native Thread)无关的 -- 单CPU系统下，Erlang系统的所有进程都在一个本地线程完成并发操作，
    *       进程之间的通信部存在传统的共享变量、锁等概念，所有的同步通信机制都通过消息传递完成。
    *     分布式：一个Erlang虚拟机就是网络上的一个节点，一个Erlang节点能在另外一个节点上生成自己的并发子进程，而子进程说在的节点
    *       可能是另外一台运行不同操作系统的服务器。不同节点之间可以进行极为搞笑而又精确的通信，就像他们运行在一个节点上一样。
    *     鲁棒性：内建多种错误侦测原语，可以通过这些原语来架构高容错能力的系统。如：一个进程可以监视其他进程的状态和活动。
    *     软实时：是一个软实时系统(Soft Real Time)，可以提供毫秒级的响应
    *
    * Concurrency Programming != Multi-Thread Programming，挑战在于Programming Model的改变，如何去实现（包括架构、容错、实时监控等等）这种并行化，如何去调试，如何去测试。
    * 并行编程面临三大挑战：扩展性、精确性和易于编程、维护。
    * 并行编程的分解方式：
    *   1.按任务分解 -- 不同的行为分配给不同的线程，通常用在GUI(如UI线程 和 后台进行计算的工作线程)
    *   2.按数据分解 -- 多个线程对不同的数据集执行同样的操作，标准实现为 完成端口/线程池，对称多处理(SMP)
    *   3.按数据流分解 -- 一个线程的输出是第二个线程的输入，生产者/消费者，标准实现为 流水线
    *
    * 常见的并行编程模式
    *   1.任务级并行编程模式 -- 通常移除任务间的依赖性或使用复制来分离依赖性是必要的;
    *   2.分而治之模式 -- 问题被分成许多并行的子问题。每个子问题被单独解决，很好地处理了负载均衡
    *   3.几何分解模式 -- 基于正在解决问题的数据结构的并行化，每个线程负责操作数据块。适用于诸如热流和声波传播之类的问题。
    *   4.流水线模式 -- 使计算被分割成一系列阶段，每个线程在不同的阶段同时工作。
    *   5.波阵面模式 -- 在处理二维网格中延对角线的数据元素时非常有用
    *   
    **********************************************************************************************************************/

    /**********************************************************************************************************************
    * 使用流水线技术(多道程序设计)的模版框架代码
    *   通过集成成组的抽象类，并定义这些类的协作的标准途径，框架为应用提供了可复用的软件组件
    **********************************************************************************************************************/

#ifndef DEFAULT_QUEUE_COUNT
#  define DEFAULT_QUEUE_COUNT ((DWORD)(1))
#endif 
#pragma message( "PIPELINE  DEFAULT_QUEUE_COUNT = " QQUOTE(DEFAULT_QUEUE_COUNT) )

    struct ElementBase;
    template<typename ELEMENT> class CFTaskBaseT;
    template<typename ELEMENT> class CFPipelineBaseT;
    template<typename ELEMENT> class CFPipelineFactoryT;
    template<typename ELEMENT> class CFProgressManager;

    enum ErrorOperation
    {
        eoContinue,                     //! 忽略当前发生错误的Element，继续处理后续Element -- 比如Element格式错误时
        eoBreakDelay,                   //! 停止当前Element，并等待其他处理中的Element结束 -- 比如目标目录中文件个数到达上限
        eoBreakImmediately,             //! 立即停止处理 -- 比如目标磁盘满时
    };

    enum ElementType
    {
        etNormal,                       //! 普通的Element，正常处理
        etError,                        //! 出现错误的Element，将直接向后传递
        etNotifyLast,                   //! 表示是最后一个空Element，由框架添加，处理完毕后流水线结束
        etNotifyErrorLast               //! 表示出现 eoBreakDelay 后，需要等待流水线结束的空Element，由框架添加和处理
    };

    template <typename ELEMENT>
    class IFProgressObserver
    {
    public:
        virtual ~IFProgressObserver() {};
        virtual VOID OnTotalProgress(double totalPercent, const ELEMENT* pCurElement, double curElementProgress) = 0;
        virtual VOID OnElementProgress(const ELEMENT* pElement, double progress) = 0;
        virtual VOID OnTaskProgress(const CFTaskBaseT<ELEMENT>* pTask, const ELEMENT* pElement, double progress) = 0;
    };

    template <typename ELEMENT>
    class IFStatusObserver
    {
    public:
        virtual ~IFStatusObserver() {};
        virtual VOID OnError(const ELEMENT* pErrElement, ErrorOperation errOpt,  DWORD errCode ) = 0;
        //virtual VOID OnEnd(INT successedCount, INT failedCount ) = 0;
    };
}

namespace FTL
{
    #define INVALID_ELEMENT_ID          ((DWORD)(-1))
    #define INVALID_ELEMENT_SIZE        ((INT64)(-1))

    struct ElementBase
    {
    public:
        explicit ElementBase()
            :Id(INVALID_ELEMENT_ID)
            ,Size(INVALID_ELEMENT_SIZE)
            ,TotalWeightSize(INVALID_ELEMENT_SIZE)
            ,Type(etNormal)
        {
        }
        explicit ElementBase(DWORD id, INT64 size, ElementType type = etNormal)
            :Id(id)
            ,Size(size)
            ,TotalWeightSize(INVALID_ELEMENT_SIZE)
            ,Type(type)
        {
        }
        virtual ~ElementBase()
        {

        }
        ElementBase(const ElementBase &ref)
        {
            *this = ref;
        }
        ElementBase &operator = (const ElementBase &ref)
        {
            if (this != &ref)
            {
                this->Id = ref.Id;
                this->Size = ref.Size;
                this->TotalWeightSize = ref.TotalWeightSize;
                this->Type = ref.Type;
            }
            return *this;
        }
    public:
        //! Element的唯一Id值
        DWORD           Id;
        
        //! Element的大小（如文件大小等）
        INT64           Size;

        //! 计算权重后的WeightSize，用户不设置，框架代码中通过 getElementWeightSize 获取所有Task的权重和，然后设置。
        //  用于计算进度
        INT64           TotalWeightSize;    

        ElementType     Type;
    };
    
    template<typename ELEMENT>
    class IFTaskObserver
    {
    public:
        virtual ~IFTaskObserver() {};
        virtual VOID OnTaskBegin(const CFTaskBaseT<ELEMENT> *pTask) = 0;
        virtual VOID OnTaskStop(const CFTaskBaseT<ELEMENT> *pTask) = 0;
        virtual VOID OnTaskEnd(const CFTaskBaseT<ELEMENT> *pTask) = 0;
        virtual VOID OnTaskError(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT* pErrElement, ErrorOperation errOpt,  DWORD errCode ) = 0; 
    };

    template <typename ELEMENT>
    class IFTaskProgressObserver
    {
    public:
        virtual ~IFTaskProgressObserver() {};
        virtual VOID OnElementBegin(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT& element ) = 0;
        virtual VOID OnElementEnd(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT& element ) = 0;
        virtual VOID OnProgress(const CFTaskBaseT<ELEMENT>* pTask, const ELEMENT& element, INT progress) = 0;
    };

    template <typename ELEMENT>
    class IFPipelineObserver
    {
    public:
        virtual ~IFPipelineObserver() {};
        virtual VOID OnPipelineBegin(const CFPipelineBaseT<ELEMENT> *pPipeline) = 0;
        virtual VOID OnPipelineStop(const CFPipelineBaseT<ELEMENT> *pPipeline) = 0;
        virtual VOID OnPipelineEnd(const CFPipelineBaseT<ELEMENT> *pPipeline) = 0;
        virtual VOID OnPipelineError(const CFPipelineBaseT<ELEMENT> *pPipeline, const ELEMENT* pErrElement, ErrorOperation errOpt,  DWORD errCode ) = 0; 
    };

    template <typename ELEMENT>
    class CFTaskBaseT
    {
        friend class CFPipelineBaseT<ELEMENT>;
        friend class CFProgressManager<ELEMENT>;
    public:
        explicit CFTaskBaseT(HANDLE hStopEvent, HANDLE hContinueEvent);
        virtual ~CFTaskBaseT();
        virtual DWORD GetTaskId() const = 0 ;
    public:
        VOID SetTaskWeight(INT nTaskWeight);
        VOID SetProgressPriority(INT nPriority);
        INT GetTaskWeight() const ;
        INT GetProgressPriority() const ;
    protected:
        virtual DWORD handleCurrentElement(ErrorOperation& outErrOpType) = 0;
        virtual VOID  freeUnhandledInputQueueElement(ELEMENT& unhandledElement) = 0;
        virtual VOID  freeUnpassedCurrentElement() = 0;
    protected:
        virtual DWORD onTaskInit(ErrorOperation& outErrOpType);
        virtual DWORD onTaskUninit(ErrorOperation& outErrOpType);
        virtual INT64 getElementWeightSize(const ELEMENT& element) const;
    protected:
        VOID    notifyProgress(INT progress);
        VOID    setTaskObserver(IFTaskObserver<ELEMENT>* pTaskObserver);
        VOID    setTaskProgressObserver(IFTaskProgressObserver<ELEMENT>* pTaskProgressObserver);
        VOID    setTotalWeight(INT nTotalWeight);
        INT     getTotalWeight() const ;
        VOID    setInputQueue(CFProducerConsumerQueue<ELEMENT> * pInputQueue);
        VOID    setOutputQueue(CFProducerConsumerQueue<ELEMENT> * pOutputQueue);
        VOID    notifyTaskErrorOperation(ErrorOperation errOpt);
        BOOL    doTask(BOOL bAsync);
        DWORD   innerDoTaskASync();  
        DWORD   innerDoTaskSync();
        static DWORD __stdcall  taskThreadProc( LPVOID pParam );
    protected:
        HANDLE                              m_hStopEvent;
        HANDLE                              m_hContinueEvent;
        ELEMENT                             m_CurrentElement;
        INT                                 m_nTaskWeight;
        INT                                 m_nTotalWeight;
        INT                                 m_nProgressPriority;
        BOOL                                m_bAsync;
        CFProducerConsumerQueue<ELEMENT>*    m_pInputQueue;
        CFProducerConsumerQueue<ELEMENT>*    m_pOutputQueue;
        IFTaskObserver<ELEMENT>*            m_pTaskObserver;
        IFTaskProgressObserver<ELEMENT>*    m_pTaskProgressObserver;
        CFThread<FTL::DefaultThreadTraits>* m_pTaskThread;
        ErrorOperation                      m_taskErrOpt;
        CFTaskBaseT<ELEMENT>*               m_pPrevTask;
        CFTaskBaseT<ELEMENT>*               m_pNextTask;
        mutable CFCriticalSection           m_lockObject;
    };

    template <typename ELEMENT>
    class CFPipelineBaseT : protected IFTaskObserver<ELEMENT>
    {
        friend class CFPipelineFactoryT<ELEMENT>;
    public:
        explicit CFPipelineBaseT(HANDLE hStopEvent, HANDLE hContinueEvent, BOOL bAsync = TRUE);
        virtual ~CFPipelineBaseT();
        virtual  DWORD GetPipelineId() const = 0;
        DWORD    GetElementCount() const;
    protected:
        virtual DWORD   getElementHandleScore(const ELEMENT& element) const = 0;
        virtual DWORD   createPipelineTask() = 0;
        virtual VOID    handleLastQueueElement(const ELEMENT & element) = 0;

        VOID    setProgressManager(CFProgressManager<ELEMENT> * pProgressManager);
        VOID    setPipelineObserver(IFPipelineObserver<ELEMENT> * pPipelineObserver);
        BOOL    addElement(const ELEMENT& element);
        INT64   calcTotalTaskWeigtSize();
        BOOL    innerDoPipelineAsync();
        BOOL    innerDoPipelineSync();
        DWORD   buildPipeline();
        BOOL    fillStartedQueue(BOOL bAll);
        VOID    clearAllPipelineInfo();
        DWORD   run();
    protected:
        virtual VOID OnTaskBegin(const CFTaskBaseT<ELEMENT> *pTask);
        virtual VOID OnTaskStop(const CFTaskBaseT<ELEMENT> *pTask);
        virtual VOID OnTaskError(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT* pErrElement, ErrorOperation errOpt,  DWORD errCode ); 
        virtual VOID OnTaskEnd(const CFTaskBaseT<ELEMENT> *pTask);
    protected:
        BOOL                                        m_bAsync;
        HANDLE                                      m_hStopEvent;
        HANDLE                                      m_hContinueEvent;
        CFProgressManager<ELEMENT>*                 m_pProgressManager;
        IFPipelineObserver<ELEMENT>*                m_pPipelineObserver;
        mutable CFCriticalSection                   m_lockObject;
        CFSyncEventUtility<DWORD>                   m_SyncEvents;
        typedef std::list<ELEMENT>                  PipelineElementsList;
        PipelineElementsList                        m_allElements;
        typename PipelineElementsList::iterator     m_iterFillStartedQueue;

        typedef CFTaskBaseT<ELEMENT>                TaskBaseType;
        typedef std::vector<TaskBaseType*>          TaskArray;
        TaskArray                                   m_allTasks;

        typedef CFProducerConsumerQueue<ELEMENT>     TaskQueue;
        typedef std::vector<TaskQueue *>            TaskQueueArray;
        TaskQueueArray                              m_allTaskQueues;
    protected:
        BOOL    addChildTask(TaskBaseType * pTask);             //! 在当前Task后增加处理后续Element的Task
        //BOOL    addSiblingTask(TaskBaseType * pTask);         //! 在当前Task处增加并行处理当前Element的Task

    };

    template <typename ELEMENT>
    class CFPipelineFactoryT : protected IFPipelineObserver<ELEMENT>
    {
    public:
        CFPipelineFactoryT();
        virtual ~CFPipelineFactoryT();

        VOID SetProgressObserver(FTL::IFProgressObserver<ELEMENT>* pProgressObserver);
        VOID SetStatusObserver(FTL::IFStatusObserver<ELEMENT>* pStatusObserver);
        BOOL Start();
        BOOL Pause();
        BOOL Resume();
        DWORD Stop(BOOL bWait = FALSE);
        DWORD Wait();
        RunningStatus GetStatus() const;
        LONGLONG   GetElapseTime();
    protected:
        virtual VOID OnPipelineBegin(const CFPipelineBaseT<ELEMENT> *pPipeline);
        virtual VOID OnPipelineStop(const CFPipelineBaseT<ELEMENT> *pPipeline);
        virtual VOID OnPipelineError(const CFPipelineBaseT<ELEMENT> *pPipeline, const ELEMENT* pErrElement, 
            ErrorOperation errOpt,  DWORD errCode );
        virtual VOID OnPipelineEnd(const CFPipelineBaseT<ELEMENT> *pPipeline);
    protected:
        DWORD   addPipelines(CFPipelineBaseT<ELEMENT>* pPipeline);
        BOOL    addElements(ELEMENT& element);
        VOID    handleError(const CFPipelineBaseT<ELEMENT> *pPipeline, 
            const ELEMENT* pErrElement, ErrorOperation errOpt, DWORD errCode);
        VOID   clearAllFactoryInfo();
        virtual DWORD createPipelines() = 0;
        virtual DWORD prepareElements() = 0;
        
    protected:
        HANDLE  m_hStopEvent;
        HANDLE  m_hContinueEvent;
        CFElapseCounter     m_ElapseCounter;
        INT                 m_nPauseCount;
        CFThread<FTL::DefaultThreadTraits>*     m_pFactoryThread;
        FTL::IFStatusObserver<ELEMENT>*         m_pStatusObserver;

        static DWORD __stdcall  factoryThreadProc( LPVOID pParam );
        DWORD   innerFactoryRunProc();

        CFProgressManager<ELEMENT>          m_ProgressManager;

        typedef std::vector<CFPipelineBaseT<ELEMENT>* >  ALL_PIPE_LINE_TYPE;
        ALL_PIPE_LINE_TYPE  m_allPipelines;   

        typedef std::list<ELEMENT> ALL_ELEMENT_LIST;
        ALL_ELEMENT_LIST m_allElements;
    };

    template <typename ELEMENT>
    class CFProgressManager : public IFTaskProgressObserver<ELEMENT>
    {
    public:
        CFProgressManager();
        virtual ~CFProgressManager();
        virtual IFTaskProgressObserver<ELEMENT>* GetTaskProgressObserver();
        virtual VOID OnElementBegin(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT& element );
        virtual VOID OnElementEnd(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT& element );
        virtual VOID OnProgress(const CFTaskBaseT<ELEMENT>* pTask, const ELEMENT& element, INT progress);

        VOID SetProgressObserver(IFProgressObserver<ELEMENT>  * pProgressObserver);
        VOID SetTotalSize(INT64 totalSize);
        VOID SetTotalCount(INT64 totalCount);
        VOID NotifyErrorElementId(DWORD elementId, ErrorOperation errOpt);
        VOID Reset();
    protected:
        VOID calcElementProgress(const CFTaskBaseT<ELEMENT>* pTask, const ELEMENT& element, INT progress);
        VOID calcTotalProgress();
    protected:
        INT64               m_totalSize;
        INT64               m_totalCount;
        INT64               m_pastSize;
        INT64               m_pastCount;
        IFProgressObserver<ELEMENT>*    m_pProgressObserver;
        mutable CFCriticalSection       m_lockObject;

        struct ElementProgressInfo
        {
            ELEMENT element;
            INT64   curPastSize;
            INT64   finishedTaskSize;
            INT     taskProgressPriority;
        };
        typedef std::map<DWORD, ElementProgressInfo> ElementProgressInfoIdMap;
        ElementProgressInfoIdMap  m_handleElementsIdMap;

#ifdef _DEBUG
        //用于防止在一个Task中一个Element处理结束后(到达MAX_PROGRESS)，又来了进度信息的Bug
        //可能还不对？
        typedef struct TaskFinishedElementInfo
        {
            DWORD ElementId;
            DWORD TaskId;
        }TaskFinishedElementInfo;
        typedef std::set<TaskFinishedElementInfo> AllTaskFinishedElementInfo;
        AllTaskFinishedElementInfo    m_allTaskFinishedElements;
#endif
        template<typename T>
        class LessTaskProgressPriority 
        {
        public:
            bool operator()(T& v1, T& v2)
            {
                bool bLess = (v1.second.taskProgressPriority < v2.second.taskProgressPriority);
                return bLess;
            }
        };
    };

}//FTL

#endif //FTL_PIPELINE_H

#ifndef USE_EXPORT
# include "ftlPipeline.hpp"
#endif //USE_EXPORT