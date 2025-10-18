#ifndef FTL_PIPELINE_HPP
#define FTL_PIPELINE_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlPipeline.h"
#endif

namespace FTL
{
    //////////////////////////////////////////////////////////////////////////
    template <typename ELEMENT>
    CFTaskBaseT<ELEMENT>::CFTaskBaseT(HANDLE hStopEvent, HANDLE hContinueEvent)
    : m_hStopEvent(hStopEvent)
    , m_hContinueEvent(hContinueEvent)
    , m_nTaskWeight(0)
    , m_nTotalWeight(0)
    , m_nProgressPriority(0)
    , m_pInputQueue(NULL)
    , m_pOutputQueue(NULL)
    , m_pTaskObserver(NULL)
    , m_pTaskProgressObserver(NULL)
    , m_pTaskThread(NULL)
    , m_pPrevTask(NULL)
    , m_pNextTask(NULL)
    , m_taskErrOpt(eoContinue)
    , m_bAsync(TRUE)
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        FTLASSERT(NULL != m_hStopEvent);
        FTLASSERT(NULL != m_hContinueEvent);
    }

    template <typename ELEMENT>
    CFTaskBaseT<ELEMENT>::~CFTaskBaseT()
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        m_pTaskProgressObserver = NULL;
        m_pTaskObserver = NULL;

        if (m_pTaskThread)
        {
            m_pTaskThread->Wait( INFINITE, TRUE );
            SAFE_DELETE(m_pTaskThread);
        }

        m_hStopEvent = NULL;
        m_hContinueEvent = NULL;
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::SetTaskWeight(INT nTaskWeight)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_nTaskWeight = nTaskWeight;
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::SetProgressPriority(INT nPriority)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_nProgressPriority = nPriority;
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::setTaskObserver(IFTaskObserver<ELEMENT>* pTaskObserver)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_pTaskObserver = pTaskObserver; 
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::setTaskProgressObserver(IFTaskProgressObserver<ELEMENT>* pTaskProgressObserver)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_pTaskProgressObserver = pTaskProgressObserver; 
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::setTotalWeight(INT nTotalWeight)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_nTotalWeight = nTotalWeight;
    }

    template <typename ELEMENT>
    INT CFTaskBaseT<ELEMENT>::GetTaskWeight() const
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        return m_nTaskWeight;
    }

    template <typename ELEMENT>
    INT CFTaskBaseT<ELEMENT>::getTotalWeight() const
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        return m_nTotalWeight;
    }

    template <typename ELEMENT>
    INT CFTaskBaseT<ELEMENT>::GetProgressPriority() const
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        return m_nProgressPriority;
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::setInputQueue(CFProducerConsumerQueue<ELEMENT> * pInputQueue)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_pInputQueue = pInputQueue;
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::setOutputQueue(CFProducerConsumerQueue<ELEMENT> * pOutputQueue)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_pOutputQueue = pOutputQueue;
    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::notifyTaskErrorOperation(ErrorOperation errOpt)
    {
        if (m_pPrevTask)
        {
            m_pPrevTask->notifyTaskErrorOperation(errOpt);
        }
        else
        {
            CFAutoLock<CFLockObject>    locker(&m_lockObject);

            if ((eoContinue == m_taskErrOpt) && (eoContinue != errOpt))
            {
                m_taskErrOpt = errOpt;

                ELEMENT unhandledElement;
                while(m_pInputQueue->RemoveAfterStop(unhandledElement))
                {
                    freeUnhandledInputQueueElement(unhandledElement);
                }

                ELEMENT notifyErrorLastElement;
                notifyErrorLastElement.Type = etNotifyErrorLast;
                m_pInputQueue->Append(notifyErrorLastElement ,INFINITE);
            }
        }

    }

    template <typename ELEMENT>
    VOID CFTaskBaseT<ELEMENT>::notifyProgress(INT progress)
    {
        FTLASSERT(m_pTaskProgressObserver);
        if (m_pTaskProgressObserver)
        {
            m_pTaskProgressObserver->OnProgress(this,m_CurrentElement,progress);
        }
    }
    
    template <typename ELEMENT>
    DWORD CFTaskBaseT<ELEMENT>::onTaskInit(ErrorOperation& /*outErrOpType*/)
    {
        return ERROR_SUCCESS;
    }

    template <typename ELEMENT>
    DWORD CFTaskBaseT<ELEMENT>::onTaskUninit(ErrorOperation& /*outErrOpType*/)
    {
        return ERROR_SUCCESS;
    }

    template <typename ELEMENT>
    INT64 CFTaskBaseT<ELEMENT>::getElementWeightSize(const ELEMENT& element) const
    {
        INT totalWeight = getTotalWeight();
        FTLASSERT( totalWeight > 0);

        INT64 elementWeightSize = element.Size * GetTaskWeight() / totalWeight;
        return elementWeightSize;
    }

    template <typename ELEMENT>
    BOOL CFTaskBaseT<ELEMENT>::doTask(BOOL bAsync)
    {
        FUNCTION_BLOCK_TRACE(0);

        FTLASSERT(m_pTaskObserver);
        FTLASSERT(m_pTaskProgressObserver);
        FTLASSERT(m_pInputQueue);
        FTLASSERT(m_pOutputQueue);
        if (NULL == m_pTaskObserver
            || NULL == m_pTaskProgressObserver
            || NULL == m_pInputQueue
            || NULL == m_pOutputQueue)
        {
            ::SetLastError(ERROR_BAD_ARGUMENTS);
            return FALSE;
        }
        BOOL bRet = FALSE;
        m_bAsync = bAsync;

        if (bAsync)
        {
            FTLASSERT(NULL == m_pTaskThread);  //only can do once
            m_pTaskThread = new FTL::CFThread<FTL::DefaultThreadTraits>(m_hStopEvent, m_hContinueEvent);
            if (!m_pTaskThread)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }

            API_VERIFY(m_pTaskThread->Start(taskThreadProc,this, FALSE));
            API_VERIFY(m_pTaskThread->LowerPriority());
        }
        else
        {
            bRet = innerDoTaskSync();
        }
        return bRet;
    }


    template <typename ELEMENT>
    DWORD CFTaskBaseT<ELEMENT>::taskThreadProc( LPVOID pParam)
    {
        //TaskBase中不调用 onTaskError，只在出错时退出循环等

        FUNCTION_BLOCK_TRACE(0);
        CFTaskBaseT<ELEMENT>* pThis = static_cast<CFTaskBaseT<ELEMENT>*>(pParam);
        FTLASSERT(pThis);

        pThis->m_pTaskObserver->OnTaskBegin(pThis);

        DWORD dwError = ERROR_SUCCESS;

        ErrorOperation errOpType = eoContinue;
        dwError = pThis->onTaskInit(errOpType);
        if ( ERROR_SUCCESS == dwError )
        {
            dwError = pThis->innerDoTaskASync();
            if (ERROR_SUCCESS != dwError)
            {
                //注意，由于 innerDoTaskASync 中调用了 onTaskError，此处就不再调用了
            }

            dwError = pThis->onTaskUninit(errOpType);
            if (dwError != ERROR_SUCCESS)
            {
                //dwLastError = ::GetLastError();
                pThis->m_pTaskObserver->OnTaskError(pThis, NULL, errOpType, dwError );
            }
        }
        else
        {
            pThis->m_pTaskObserver->OnTaskError(pThis, NULL, errOpType, dwError );
        }

        pThis->m_pTaskObserver->OnTaskEnd(pThis);

        return dwError;
    }

    template <typename ELEMENT>
    DWORD CFTaskBaseT<ELEMENT>::innerDoTaskASync()
    {
        FUNCTION_BLOCK_TRACE(0);
        DWORD dwError = ERROR_SUCCESS;
        FTLThreadWaitType waitType = ftwtContinue;

#define USE_AUTO_QUEUE_OPERATOR
#ifdef USE_AUTO_QUEUE_OPERATOR
        BOOL bGotElement = TRUE;
        while (bGotElement)
        {
            ErrorOperation  errOpType = eoContinue;
            {
                CFAutoQueueOperator<ELEMENT> autoQueueOperator(m_pInputQueue, m_CurrentElement, FALSE, INFINITE);
                bGotElement = autoQueueOperator.HaveGotQueue();
                if (bGotElement)
                {
                    if(etNotifyLast == m_CurrentElement.Type || etNotifyErrorLast == m_CurrentElement.Type)
                    {
                        //just append into output queue -- if been stopped, do nothing
                        m_pOutputQueue->Append(m_CurrentElement,INFINITE);
                        break;
                    }


                    if (m_pTaskProgressObserver)
                    {
                        m_pTaskProgressObserver->OnElementBegin(this, m_CurrentElement);
                    }

                    dwError = handleCurrentElement(errOpType);
                    if (m_pTaskProgressObserver)
                    {
                        m_pTaskProgressObserver->OnElementEnd(this, m_CurrentElement);
                    }

                    if (ERROR_SUCCESS != dwError)
                    {
                        FTLTRACEEX(FTL::tlError, TEXT("CFTaskBaseT[%d] handleCurrentElement error=%d, ElementId=%d, errOpType=%d"), 
                            GetTaskId(), dwError, m_CurrentElement.Id, (INT)errOpType);
                        m_pTaskObserver->OnTaskError(this, &m_CurrentElement, errOpType, dwError);
                        if (eoContinue != errOpType)
                        {
                            notifyTaskErrorOperation(errOpType);
                            if (NULL == m_pPrevTask)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            if (eoContinue == errOpType)
            {
                if(bGotElement && m_pOutputQueue->Append(m_CurrentElement,INFINITE) != ftwtContinue)
                {
                    //user stop
                    freeUnpassedCurrentElement();
                    //SetLastError(ERROR_THREAD_WAS_SUSPENDED);
                    break;
                }
            }
        }

#else
        while((waitType = m_pInputQueue->Remove(m_CurrentElement,INFINITE)) == ftwtContinue)
        {
            if(etNotifyLast == m_CurrentElement.Type || etNotifyErrorLast == m_CurrentElement.Type)
            {
                //just append into output queue -- if been stopped, do nothing
                m_pOutputQueue->Append(m_CurrentElement,INFINITE);
                break;
            }

            ErrorOperation  errOpType = eoContinue;

            if (m_pTaskProgressObserver)
            {
                m_pTaskProgressObserver->OnElementBegin(this, m_CurrentElement);
            }
            
            dwError = handleCurrentElement(errOpType);
            if (m_pTaskProgressObserver)
            {
                m_pTaskProgressObserver->OnElementEnd(this, m_CurrentElement);
            }

            if (ERROR_SUCCESS != dwError)
            {
                FTLTRACEEX(FTL::tlError, TEXT("CFTaskBaseT[%d] handleCurrentElement error=%d, ElementId=%d, errOpType=%d"), 
                    GetTaskId(), dwError, m_CurrentElement.Id, (INT)errOpType);
                m_pTaskObserver->OnTaskError(this, &m_CurrentElement, errOpType, dwError);
                if (eoContinue != errOpType)
                {
                    notifyTaskErrorOperation(errOpType);
                    if (NULL == m_pPrevTask)
                    {
                        break;
                    }
                }
            }

            if (eoContinue == errOpType)
            {
                if(m_pOutputQueue->Append(m_CurrentElement,INFINITE) != ftwtContinue)
                {
                    //user stop
                    freeUnpassedCurrentElement();
                    //SetLastError(ERROR_THREAD_WAS_SUSPENDED);
                    break;
                }
            }
        }

#endif
        if (ftwtContinue != waitType)
        {
            dwError = ERROR_OPERATION_ABORTED;
        }

        ELEMENT unhandledElement;
        while(m_pInputQueue->RemoveAfterStop(unhandledElement))
        {
            freeUnhandledInputQueueElement(unhandledElement);
        }
        return dwError;

    }


    template <typename ELEMENT>
    DWORD CFTaskBaseT<ELEMENT>::innerDoTaskSync()
    {
        DWORD dwError = ERROR_SUCCESS;
        FTLThreadWaitType waitType = ftwtError;
        ErrorOperation errOpType = eoContinue;

        do 
        {
            waitType = m_pInputQueue->Remove(m_CurrentElement,INFINITE);
            if (ftwtContinue == waitType) //Get
            {
                if (m_CurrentElement.Type == etNotifyLast || m_CurrentElement.Type == etNotifyErrorLast)
                {
                    //last element,just quit
                    FTLTRACEEX(tlTrace,TEXT("Task %d Get Last Element,will Quit"),GetTaskId());
                    m_pOutputQueue->Append(m_CurrentElement,INFINITE);
                    break;
                }
                dwError = handleCurrentElement(errOpType);
                if (ERROR_SUCCESS != dwError)
                {
                    m_pTaskObserver->OnTaskError(this,&m_CurrentElement,errOpType,dwError);
                    if (eoContinue != errOpType)
                    {
                        notifyTaskErrorOperation(errOpType);
                        if (NULL == m_pPrevTask)
                        {
                            break;
                        }
                    }
                }
                //Because the last task's Output can hold many element, so can't assert (GetElementCount==1)
                FTLASSERT(m_pOutputQueue->GetCapability() > m_pOutputQueue->GetElementCount());
                waitType = m_pOutputQueue->Append(m_CurrentElement,INFINITE);
                if (ftwtContinue != waitType)
                {
                    //Can't Append, User Stop
                    freeUnpassedCurrentElement();
                    m_pTaskObserver->OnTaskStop(this);
                }
            }
            else
            {
                //User Stop, Can't Remove, m_curElement is old value, do nothing
                m_pTaskObserver->OnTaskStop(this);
            }
        } while (false);
        return dwError;
    }


    //////////////////////////////////////////////////////////////////////////
    template <typename ELEMENT>
    CFPipelineBaseT<ELEMENT>::CFPipelineBaseT(HANDLE hStopEvent, HANDLE hContinueEvent, BOOL bAsync /* = TRUE */)
        :m_hStopEvent(hStopEvent)
        ,m_hContinueEvent(hContinueEvent)
        ,m_bAsync(bAsync)
        ,m_pPipelineObserver(NULL)
        ,m_pProgressManager(NULL)
        ,m_iterFillStartedQueue(m_allElements.end())
    {
        FTLASSERT(m_hStopEvent);
        FTLASSERT(m_hContinueEvent);
    }


    template <typename ELEMENT>
    CFPipelineBaseT<ELEMENT>::~CFPipelineBaseT()
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        m_SyncEvents.ClearAllEvent();
        clearAllPipelineInfo();
    }

    template <typename ELEMENT>
    VOID CFPipelineBaseT<ELEMENT>::clearAllPipelineInfo()
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);

        m_allElements.clear();
        for (size_t i = 0; i < m_allTasks.size(); i++)
        {
            SAFE_DELETE(m_allTasks[i]);
        }
        m_allTasks.clear();

        TaskQueueArray::iterator iter;
        for (iter = m_allTaskQueues.begin(); iter != m_allTaskQueues.end(); iter++)
        {
            TaskQueue *pQueue = *iter;
            delete pQueue;
        }
        m_allTaskQueues.clear();
    }

    template <typename ELEMENT>
    VOID CFPipelineBaseT<ELEMENT>::setPipelineObserver(IFPipelineObserver<ELEMENT> * pPipelineObserver)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_pPipelineObserver = pPipelineObserver;
    }

    template <typename ELEMENT>
    VOID CFPipelineBaseT<ELEMENT>::setProgressManager(CFProgressManager<ELEMENT> * pProgressManager)
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_pProgressManager = pProgressManager;
    }

    template <typename ELEMENT>
    DWORD CFPipelineBaseT<ELEMENT>::GetElementCount() const
    {
        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        DWORD dwTotalElementCount = (DWORD)m_allElements.size();
        if (dwTotalElementCount > 0)
        {
            if (m_allElements.rbegin()->Type == etNotifyLast)
            {
                dwTotalElementCount -= 1;
            }
        }
        return dwTotalElementCount;
    }

    template <typename ELEMENT>
    BOOL CFPipelineBaseT<ELEMENT>::addElement(const ELEMENT& element)
    {
        FTLASSERT(getElementHandleScore(element) > 0);

        CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_allElements.push_back(element);
        return TRUE;
    }
    
    template <typename ELEMENT>
    INT64 CFPipelineBaseT<ELEMENT>::calcTotalTaskWeigtSize() 
    {
        INT64 pipelineTotalTaskWeightSize = 0LL;
        INT64 elementTotalTaskWeightSize = 0LL;
        for (PipelineElementsList::iterator iterElement = m_allElements.begin();
            iterElement != m_allElements.end(); ++iterElement)
        {
            if (etNotifyLast == iterElement->Type)
            {
                break;
            }
            elementTotalTaskWeightSize = 0LL;
            for (size_t taskIndex = 0; taskIndex < m_allTasks.size(); taskIndex++)
            {
                elementTotalTaskWeightSize += m_allTasks[taskIndex]->getElementWeightSize(*iterElement);
            }
            (*iterElement).TotalWeightSize = elementTotalTaskWeightSize;
            pipelineTotalTaskWeightSize += elementTotalTaskWeightSize;
        }
        return pipelineTotalTaskWeightSize;
    }

    template <typename ELEMENT>
    DWORD CFPipelineBaseT<ELEMENT>::buildPipeline()
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        FTLASSERT(m_allTasks.empty());
        FTLASSERT(m_allTaskQueues.empty());
        DWORD dwResult = ERROR_SUCCESS;

        dwResult = createPipelineTask();
        if (ERROR_SUCCESS != dwResult)
        {
            return dwResult;
        }

        FTLASSERT(!m_allTasks.empty());

#ifdef FTL_DEBUG
        if (m_bAsync)
        {
            FTLASSERT(m_SyncEvents.GetSyncEventCount() == m_allTasks.size());
        }
        else
        {
            FTLASSERT(m_SyncEvents.GetSyncEventCount() == 0);
        }
#endif
        m_SyncEvents.ResetAllEvent();
        m_allTaskQueues.reserve(m_allTasks.size() + 1);

        //First Queue has no Count limit
        TaskQueue* pStartQueue = new TaskQueue(MAXLONG, 0, m_hStopEvent);
        m_allTaskQueues.push_back(pStartQueue);

        for (size_t i = 0; i < m_allTasks.size(); i++)
        {
            m_allTasks[i]->m_pPrevTask = NULL;
            m_allTasks[i]->m_pNextTask = NULL;

            if (i > 0)
            {
                m_allTaskQueues.push_back(new TaskQueue(DEFAULT_QUEUE_COUNT, 0, m_hStopEvent));
                m_allTasks[i-1]->setOutputQueue(m_allTaskQueues[i]);
                m_allTasks[i]->m_pPrevTask = m_allTasks[i-1];
            }

            if (i < m_allTasks.size() - 1)
            {
                m_allTasks[i]->m_pNextTask = m_allTasks[i+1];
            }

            if (i <  m_allTasks.size())
            {
                m_allTasks[i]->setInputQueue(m_allTaskQueues[i]);
            }
        }

        //Last Queue has no count limit -- 最后一个Queue使用内部的StopEvent，保证Task完成后总是能放入
        TaskQueue* pEndQueue = new TaskQueue(MAXLONG, 0, NULL);
        m_allTaskQueues.push_back(pEndQueue);
        m_allTasks[m_allTasks.size() - 1]->setOutputQueue(m_allTaskQueues[m_allTasks.size()]);

        INT TotalWeight = 0;
        for (size_t i = 0; i < m_allTasks.size(); i++)
        {
            TotalWeight += m_allTasks[i]->GetTaskWeight(); 
        }
        for (size_t i = 0; i < m_allTasks.size(); i++)
        {
            m_allTasks[i]->setTotalWeight(TotalWeight); 
        }

        return dwResult;
    }
    
    template <typename ELEMENT>
    DWORD CFPipelineBaseT<ELEMENT>::run()
    {
        FTLASSERT(m_pPipelineObserver);
        FTLASSERT(m_pProgressManager);
        if (NULL == m_pPipelineObserver
            || NULL == m_pProgressManager)
        {
            return ERROR_BAD_ARGUMENTS;
        }

        FTLTRACEEX(FTL::tlTrace,TEXT("Pipeline [%d] Run, the Element Count is %d:"),
            this->GetPipelineId(),GetElementCount());

        if (m_allElements.empty())
        {
            //if there are no content info in this strategy
            m_pPipelineObserver->OnPipelineBegin(this);
            m_pPipelineObserver->OnPipelineEnd(this);
            return ERROR_SUCCESS;
        }

        DWORD dwResult = ERROR_SUCCESS;
        if (m_bAsync)
        {
            dwResult = innerDoPipelineAsync();
        }   
        else
        {
            dwResult = innerDoPipelineSync();
        }

        ELEMENT element;
        while (m_allTaskQueues[m_allTaskQueues.size() - 1]->RemoveAfterStop(element))
        {
            handleLastQueueElement(element);
        }

        return dwResult;
    }

    template <typename ELEMENT>
    VOID CFPipelineBaseT<ELEMENT>::OnTaskBegin(const CFTaskBaseT<ELEMENT> *pTask)
    {
        if (m_bAsync)
        {
            m_SyncEvents.ResetEvent(pTask->GetTaskId());
        }
        FTLTRACEEX(FTL::tlTrace, TEXT("Task [%d] Begin"), pTask->GetTaskId());
    }

    //virtual VOID OnTaskPause(const CFTaskBaseT *pTask);
    //virtual VOID OnTaskResume(const CFTaskBaseT *pTask);
    template <typename ELEMENT>
    VOID CFPipelineBaseT<ELEMENT>::OnTaskStop(const CFTaskBaseT<ELEMENT> *pTask)
    {
        UNREFERENCED_PARAMETER(pTask);
        FTLTRACEEX(FTL::tlTrace, TEXT("Task [%d] Stop"), 
            pTask->GetTaskId());
    }

    template <typename ELEMENT>
    VOID CFPipelineBaseT<ELEMENT>::OnTaskEnd(const CFTaskBaseT<ELEMENT> *pTask)
    {
        FTLTRACEEX(FTL::tlTrace, TEXT("Task [%d] End"), 
            pTask->GetTaskId());
        if (m_bAsync)
        {
            m_SyncEvents.SetEvent(pTask->GetTaskId());
        }
    }

    template <typename ELEMENT>
    VOID CFPipelineBaseT<ELEMENT>::OnTaskError(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT* pErrElement,
        ErrorOperation errOpt,  DWORD errCode )
    {
        CFAutoLock<CFLockObject> locker(&m_lockObject);
        FTLTRACEEX(FTL::tlError, TEXT("Task [%d] Error, pErrElement Id=%d, errOpt=%d, errCode=%d"), 
            pTask->GetTaskId(), pErrElement ? pErrElement->Id : -1 , errOpt, errCode);
        
        if (!m_bAsync)
        {
            m_iterFillStartedQueue = m_allElements.end();
        }

        if (m_pProgressManager && pErrElement)
        {
            m_pProgressManager->NotifyErrorElementId(pErrElement->Id, errOpt);
        }
        
        if (m_pPipelineObserver)
        {
            m_pPipelineObserver->OnPipelineError(this, pErrElement,errOpt, errCode);
        }
    } 

    template <typename ELEMENT>
    BOOL CFPipelineBaseT<ELEMENT>::fillStartedQueue(BOOL bAll)
    {
        FTLASSERT(m_allElements.rbegin()->Type == etNotifyLast);
        if (bAll)
        {
            for (PipelineElementsList::iterator iterElement = m_allElements.begin();
                iterElement != m_allElements.end(); ++iterElement)
            {
                m_allTaskQueues[0]->Append(*iterElement, INFINITE);
            }
        }
        else
        {
            if (m_iterFillStartedQueue != m_allElements.end())
            {
                m_allTaskQueues[0]->Append(*m_iterFillStartedQueue, INFINITE);
                m_iterFillStartedQueue++;
            }
        }
        BOOL bRet = (m_allTaskQueues[0]->GetElementCount() > 0);
        return bRet;
    }

    template <typename ELEMENT>
    BOOL CFPipelineBaseT<ELEMENT>::innerDoPipelineAsync()
    {
        FTLASSERT(TRUE == m_bAsync);
        BOOL bRet = FALSE;
        //DWORD dwResult = ERROR_SUCCESS;

        m_pPipelineObserver->OnPipelineBegin(this);
        //ErrorOperation  errOptType = eoContinue;
        
        bRet = fillStartedQueue(TRUE);
        if (bRet)
        {
            for (size_t taskIndex = 0; taskIndex < m_allTasks.size(); taskIndex++)
            {
                m_allTasks[taskIndex]->doTask(TRUE);
            }
            m_SyncEvents.WaitAllEvent(INFINITE);
        }

        CFEventChecker eventChecker(m_hStopEvent, m_hContinueEvent);
        if (ftwtContinue != eventChecker.GetWaitType(INFINITE))
        {
            m_pPipelineObserver->OnPipelineStop(this);
        }

        m_pPipelineObserver->OnPipelineEnd(this);
        return bRet;
    }

    template <typename ELEMENT>
    BOOL CFPipelineBaseT<ELEMENT>::innerDoPipelineSync()
    {
        FTLASSERT(!m_bAsync);
        BOOL bRet = FALSE;
        CFEventChecker eventChecker(m_hStopEvent, m_hContinueEvent);

        m_pPipelineObserver->OnPipelineBegin(this);
        ErrorOperation  errOptType = eoContinue;

        m_iterFillStartedQueue = m_allElements.begin();

        bRet = fillStartedQueue(FALSE);
        
        if (bRet)
        {
            DWORD dwResult = ERROR_SUCCESS;
            for (size_t taskIndex = 0; taskIndex < m_allTasks.size(); taskIndex++)
            {
                m_allTasks[taskIndex]->m_pTaskObserver->OnTaskBegin(m_allTasks[taskIndex]);
                dwResult = m_allTasks[taskIndex]->onTaskInit(errOptType);
                if (ERROR_SUCCESS != dwResult)
                {
                    m_allTasks[taskIndex]->m_pTaskObserver->OnTaskError(m_allTasks[taskIndex],NULL,errOptType,dwResult);
                    for (size_t successedTaskIndex = 0; successedTaskIndex < taskIndex; successedTaskIndex++)
                    {
                        m_allTasks[successedTaskIndex]->onTaskUninit(errOptType);
                        m_allTasks[successedTaskIndex]->m_pTaskObserver->OnTaskEnd(m_allTasks[successedTaskIndex]);
                    }
                    break;
                }
            }

            if (ERROR_SUCCESS == dwResult)
            {
                while(m_allTaskQueues[0]->GetElementCount() > 0) 
                {
                    if (ftwtContinue != eventChecker.GetWaitType(INFINITE))
                    {
                        break;
                    }
                    FTLASSERT(m_allTaskQueues[0]->GetElementCount() == 0 || m_allTaskQueues[0]->GetElementCount() == 1);
                    if (m_allTaskQueues[0]->GetElementCount() > 0)
                    {
                        for (size_t taskIndex = 0; taskIndex < m_allTasks.size(); taskIndex++)
                        {
                            bRet = m_allTasks[taskIndex]->doTask(FALSE);
                        }
                        fillStartedQueue(FALSE);
                    }
                }

                for (size_t taskIndex = 0; taskIndex < m_allTasks.size(); taskIndex++)
                {
                    dwResult = m_allTasks[taskIndex]->onTaskUninit(errOptType);
                    m_allTasks[taskIndex]->m_pTaskObserver->OnTaskEnd(m_allTasks[taskIndex]);
                    //this->OnTaskEnd(m_allTasks[taskIndex]);
                }
            }
        }

        if (ftwtContinue != eventChecker.GetWaitType(0))
        {
            m_pPipelineObserver->OnPipelineStop(this);
        }

        m_pPipelineObserver->OnPipelineEnd(this);
        return bRet;
    }

    template <typename ELEMENT>
    BOOL CFPipelineBaseT<ELEMENT>::addChildTask(TaskBaseType * pTask)
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        FTLASSERT( m_pProgressManager );
        FTLASSERT( m_allTasks.size() + 1 < MAXIMUM_WAIT_OBJECTS );

        if (m_allTasks.size() + 1 < MAXIMUM_WAIT_OBJECTS )
        {
            pTask->setTaskObserver(this);
            pTask->setTaskProgressObserver(m_pProgressManager->GetTaskProgressObserver());
            m_allTasks.push_back(pTask);
            if (m_bAsync)
            {
                m_SyncEvents.AddEvent(pTask->GetTaskId());
            }
            return TRUE;
        }
        return FALSE;
    }



    //////////////////////////////////////////////////////////////////////////
    template <typename ELEMENT>
    CFPipelineFactoryT<ELEMENT>::CFPipelineFactoryT()
        :m_hStopEvent(NULL)
        ,m_hContinueEvent(NULL)
        ,m_pFactoryThread(NULL)
        ,m_pStatusObserver(NULL)
        ,m_nPauseCount(0)
    {
        
    }

    template <typename ELEMENT>
    CFPipelineFactoryT<ELEMENT>::~CFPipelineFactoryT()
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        Wait();
        clearAllFactoryInfo();
    }

    template <typename ELEMENT>
    VOID CFPipelineFactoryT<ELEMENT>::clearAllFactoryInfo()
    {
        for (ALL_PIPE_LINE_TYPE::iterator iterPipeLine = m_allPipelines.begin();
            iterPipeLine != m_allPipelines.end(); ++iterPipeLine)
        {
            CFPipelineBaseT<ELEMENT>* pPipeline = *iterPipeLine;
            delete pPipeline;
        }
        m_allPipelines.clear();

        m_allElements.clear();
    }

    template <typename ELEMENT>
    VOID CFPipelineFactoryT<ELEMENT>::OnPipelineBegin(const CFPipelineBaseT<ELEMENT> *pPipeline)
    {
        FTLTRACEEX(FTL::tlTrace, TEXT("Pipeline[%d] Begin"), 
            pPipeline->GetPipelineId());
    }
    //virtual VOID OnPipelinePause(const IFPipeline<ELEMENT> *pPipeline) = 0;
    //virtual VOID OnPipelineResume(const IFPipeline<ELEMENT> *pPipeline) = 0;
    template <typename ELEMENT>
    VOID CFPipelineFactoryT<ELEMENT>::OnPipelineStop(const CFPipelineBaseT<ELEMENT> *pPipeline)
    {
        FTLTRACEEX(FTL::tlTrace, TEXT("Pipeline[%d] Stop"), 
            pPipeline->GetPipelineId());
    }
    template <typename ELEMENT>
    VOID CFPipelineFactoryT<ELEMENT>::OnPipelineError(const CFPipelineBaseT<ELEMENT> *pPipeline, const ELEMENT* pErrElement, 
        ErrorOperation errOpt,  DWORD errCode )
    {
        FTLTRACEEX(FTL::tlError, TEXT("Pipeline[%d] Error,pErrElement Id=%d, errOpt=%d, errCode=%d"), 
            pPipeline->GetPipelineId(), pErrElement ? pErrElement->Id : -1 ,
            errOpt, errCode);
        if (m_pStatusObserver)
        {
            m_pStatusObserver->OnError(pErrElement,errOpt,errCode);
        }
        handleError(pPipeline,pErrElement,errOpt,errCode);
    }

    template <typename ELEMENT>
    VOID CFPipelineFactoryT<ELEMENT>::OnPipelineEnd(const CFPipelineBaseT<ELEMENT> *pPipeline)
    {
        UNREFERENCED_PARAMETER(pPipeline);
        FTLTRACEEX(FTL::tlTrace, TEXT("Pipeline[%d] End"), 
            pPipeline->GetPipelineId());
    }

    template <typename ELEMENT>
    VOID CFPipelineFactoryT<ELEMENT>::SetProgressObserver(FTL::IFProgressObserver<ELEMENT>* pProgressObserver)
    {
        //CFAutoLock<CFLockObject>    locker(&m_lockObject);
        //m_pProgressObserver = pProgressObserver;
        m_ProgressManager.SetProgressObserver(pProgressObserver);
    }

    template <typename ELEMENT>
    VOID CFPipelineFactoryT<ELEMENT>::SetStatusObserver(FTL::IFStatusObserver<ELEMENT>* pStatusObserver)
    {
        //CFAutoLock<CFLockObject>    locker(&m_lockObject);
        m_pStatusObserver = pStatusObserver;
    }

    template <typename ELEMENT>
    BOOL CFPipelineFactoryT<ELEMENT>::Start()
    {
        //FTLASSERT(m_pProgressObserver);
        FTLASSERT(NULL == m_pFactoryThread);
        if (m_pFactoryThread)
        {
            SetLastError(ERROR_ALREADY_INITIALIZED);
            return FALSE;
        }
        m_hStopEvent = ::CreateEvent(NULL,TRUE,FALSE,NULL);
        m_hContinueEvent = ::CreateEvent(NULL,TRUE,TRUE,NULL);
        m_pFactoryThread = new CFThread<FTL::DefaultThreadTraits>(m_hStopEvent, m_hContinueEvent);
        m_pFactoryThread->Start(factoryThreadProc, this, TRUE);
        m_ElapseCounter.Start();
        return TRUE;
    }

    template <typename ELEMENT>
    BOOL CFPipelineFactoryT<ELEMENT>::Pause()
    {
        BOOL bRet = TRUE;
        if (0 == m_nPauseCount)
        {
            API_VERIFY(ResetEvent(m_hContinueEvent));
            m_ElapseCounter.Pause();
        }
        m_nPauseCount++;
        return bRet;
    }

    template <typename ELEMENT>
    BOOL CFPipelineFactoryT<ELEMENT>::Resume()
    {
        FTLASSERT(m_nPauseCount > 0);

        BOOL bRet = TRUE;
        m_nPauseCount--;
        if (0 == m_nPauseCount)
        {
            API_VERIFY(SetEvent(m_hContinueEvent));
            m_ElapseCounter.Resume();
        }
        return bRet;
    }

    template <typename ELEMENT>
    DWORD CFPipelineFactoryT<ELEMENT>::Stop(BOOL bWait /* = FALSE */)
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        DWORD dwResult = ERROR_SUCCESS;
        BOOL bRet = FALSE;
        if (m_hStopEvent)
        {
            API_VERIFY(SetEvent(m_hStopEvent));
            if (bWait)
            {
                dwResult = Wait();
            }
            m_ElapseCounter.Stop();
        }
        return dwResult;
    }

    template <typename ELEMENT>
    DWORD CFPipelineFactoryT<ELEMENT>::Wait()
    {
        FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
        DWORD dwResult = ERROR_SUCCESS;
        if (m_pFactoryThread)
        {
            BOOL bRet = FALSE;
            m_pFactoryThread->Wait(INFINITE, TRUE);
            SAFE_DELETE(m_pFactoryThread);
            SAFE_CLOSE_HANDLE(m_hStopEvent,NULL);
            SAFE_CLOSE_HANDLE(m_hContinueEvent,NULL);
        }

        return dwResult;
    }


     template <typename ELEMENT>
     RunningStatus CFPipelineFactoryT<ELEMENT>::GetStatus() const
     {
         RunningStatus status = rsStopped;
         if (m_hStopEvent && m_hContinueEvent)
         {
             CFEventChecker eventChecker(m_hStopEvent, m_hContinueEvent);
             FTLThreadWaitType waitType = eventChecker.GetWaitType(0);
             switch(waitType)
             {
             case ftwtContinue:
                 status = rsRunning;
                 break;
             case ftwtStop:
                 status = rsStopped;
                 break;
             case ftwtTimeOut:
                 status = rsPaused;
                 break;
             case ftwtError:
             default:
                 status = rsStopped;
                 FTLASSERT(FALSE);
                 break;
             }
         }
         return status;
     }

     template <typename ELEMENT>
     LONGLONG CFPipelineFactoryT<ELEMENT>::GetElapseTime()
     {
        return m_ElapseCounter.GetElapseTime() / NANOSECOND_PER_MILLISECOND;
     }

     template <typename ELEMENT>
     VOID CFPipelineFactoryT<ELEMENT>::handleError(const CFPipelineBaseT<ELEMENT> *pPipeline, 
         const ELEMENT* pErrElement, ErrorOperation errOpt, DWORD errCode)
     {
         //目前这几个变量尚未使用，但可以考虑保存日志？
         UNREFERENCED_PARAMETER(errCode);
         UNREFERENCED_PARAMETER(pErrElement);

         switch(errOpt)
         {
         case eoContinue:
             break;
         case eoBreakDelay:
             if (!pPipeline->m_bAsync)
             {
                Stop(FALSE);
             }
             break;
         case eoBreakImmediately:
         default:
             Stop(FALSE);
             break;
         }
     }

    template <typename ELEMENT>
    DWORD __stdcall  CFPipelineFactoryT<ELEMENT>::factoryThreadProc( LPVOID pParam )
    {
        CFPipelineFactoryT<ELEMENT>* pThis = static_cast< CFPipelineFactoryT<ELEMENT>* >(pParam);
        pThis->innerFactoryRunProc();
        return 0;
    }

    template <typename ELEMENT>
    DWORD CFPipelineFactoryT<ELEMENT>::addPipelines(CFPipelineBaseT<ELEMENT>* pPipeline)
    {
        pPipeline->setProgressManager(&m_ProgressManager);
        pPipeline->setPipelineObserver(this);

        m_allPipelines.push_back(pPipeline);
        return 0;
    }

    template <typename ELEMENT>
    BOOL CFPipelineFactoryT<ELEMENT>::addElements(ELEMENT& element)
    {
        FTLASSERT((element.Type == etNotifyLast) || ((INVALID_ELEMENT_ID != element.Id ) && (INVALID_ELEMENT_SIZE != element.Size)));
        
        if (
            (element.Type == etNotifyLast) 
            || ((INVALID_ELEMENT_ID != element.Id ) && (INVALID_ELEMENT_SIZE != element.Size))
            )
        {
            m_allElements.push_back(element);
            return TRUE;
        }
        
        return FALSE;
    }

    template <typename ELEMENT>
    DWORD CFPipelineFactoryT<ELEMENT>::innerFactoryRunProc()
    {
        FUNCTION_BLOCK_TRACE(0);
        DWORD dwResult = ERROR_SUCCESS;
        clearAllFactoryInfo();

        for (size_t pipelineIndex = 0; pipelineIndex < m_allPipelines.size(); pipelineIndex++)
        {
            m_allPipelines[pipelineIndex]->clearAllPipelineInfo();
        }
        m_allElements.clear();
        m_allPipelines.clear();

        dwResult = createPipelines();
        dwResult = prepareElements();

        DWORD maxScore = 0;
        DWORD curScore = 0;
        size_t maxScorePipelineIndex = 0; 

        for (ALL_ELEMENT_LIST::iterator iterListElement = m_allElements.begin(); iterListElement != m_allElements.end(); ++iterListElement)
        {
            FTLASSERT(iterListElement->Type == etNormal);
            maxScore = 0;
            curScore = 0;
            maxScorePipelineIndex = 0;
            for (size_t pipelineIndex = 0; pipelineIndex < m_allPipelines.size(); pipelineIndex++)
            {
               curScore = m_allPipelines[pipelineIndex]->getElementHandleScore(*iterListElement);
               if (curScore > maxScore)
               {
                   maxScore = curScore;
                   maxScorePipelineIndex = pipelineIndex;
               }
            }

            if (maxScore > 0)
            {
                m_allPipelines[maxScorePipelineIndex]->addElement(*iterListElement);
            }
            else
            {
                FTLTRACEEX(FTL::tlWarn, TEXT("No pipeline can handle this element[Id=%d]"),
                    (*iterListElement).Id);
                FTLASSERT(!TEXT("No pipeline can handle element"));
            }
        }

        INT64 totalElementWeightSize = 0LL;
        INT64 totalElementCount = 0LL;
        for (size_t pipelineIndex = 0; pipelineIndex < m_allPipelines.size(); pipelineIndex++)
        {
            ELEMENT lastElement;
            lastElement.Type = etNotifyLast;
            m_allPipelines[pipelineIndex]->addElement(lastElement);

            m_allPipelines[pipelineIndex]->buildPipeline();
            totalElementWeightSize += m_allPipelines[pipelineIndex]->calcTotalTaskWeigtSize();
            totalElementCount += m_allPipelines[pipelineIndex]->GetElementCount();
        }

        m_ProgressManager.Reset();
        m_ProgressManager.SetTotalSize(totalElementWeightSize);
        m_ProgressManager.SetTotalCount(totalElementCount);
        for (size_t pipelineIndex = 0; pipelineIndex < m_allPipelines.size(); pipelineIndex++)
        {
            m_allPipelines[pipelineIndex]->run();
        }

        return 0;
    }


    //////////////////////////////////////////////////////////////////////////
    
        template <typename ELEMENT>
        CFProgressManager<ELEMENT>::CFProgressManager()
        {
            Reset();
            m_pProgressObserver = NULL;
        }
        template <typename ELEMENT>
        CFProgressManager<ELEMENT>::~CFProgressManager()
        {

        }

        template <typename ELEMENT>
        IFTaskProgressObserver<ELEMENT>* CFProgressManager<ELEMENT>::GetTaskProgressObserver()
        {
            return this;
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::OnElementBegin(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT& element )
        {
            FTLTRACEEX(FTL::tlInfo, TEXT("OnElementBegin, pTaskId[%d], element[%d]"),
                pTask->GetTaskId(), element.Id);
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::OnElementEnd(const CFTaskBaseT<ELEMENT> *pTask, const ELEMENT& element )
        {
            FTLTRACEEX(FTL::tlInfo, TEXT("OnElementEnd, pTaskId[%d], element[%d]"),
                pTask->GetTaskId(), element.Id);
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::OnProgress(const CFTaskBaseT<ELEMENT>* pTask, const ELEMENT& element, INT progress)
        {
            FTLTRACEEX(FTL::tlInfo, TEXT("pTaskId[%d], element[%d], progress=%d"),
                pTask->GetTaskId(), element.Id, progress);

            if (m_pProgressObserver)
            {
                m_pProgressObserver->OnTaskProgress(pTask,&element, progress);
            }

            calcElementProgress(pTask, element, progress);
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::calcElementProgress(const CFTaskBaseT<ELEMENT>* pTask, const ELEMENT& element, INT progress)
        {
            FTLASSERT(progress >= 0);
            FTLASSERT(progress <= MAX_PROGRESS);
            INT64 elementWeightSize = pTask->getElementWeightSize(element);

            CFAutoLock<CFLockObject> locker(&m_lockObject);

            ElementProgressInfoIdMap::iterator iterElement = m_handleElementsIdMap.find(element.Id);
            if (m_handleElementsIdMap.end() == iterElement)
            {

                ElementProgressInfo handleProgressInfo;// = {0};
                handleProgressInfo.element = element;
                handleProgressInfo.curPastSize = progress * elementWeightSize / MAX_PROGRESS;
                handleProgressInfo.taskProgressPriority = pTask->GetProgressPriority();
                if (MAX_PROGRESS == progress)
                {
                    handleProgressInfo.finishedTaskSize = elementWeightSize;
                }
                else
                {
                    handleProgressInfo.finishedTaskSize = 0LL;
                }
                
                iterElement = m_handleElementsIdMap.insert(ElementProgressInfoIdMap::value_type(element.Id, handleProgressInfo)).first;
            }
            else
            {
                iterElement->second.taskProgressPriority = pTask->GetProgressPriority();
                if (MAX_PROGRESS == progress)
                {
                    iterElement->second.finishedTaskSize += elementWeightSize;
                    iterElement->second.curPastSize = iterElement->second.finishedTaskSize;
                }
                else
                {
                    iterElement->second.curPastSize = iterElement->second.finishedTaskSize 
                        + progress * elementWeightSize / MAX_PROGRESS;
                }
            }


            if (m_pProgressObserver)
            {
                m_pProgressObserver->OnElementProgress(&element, 
                    double(MAX_PROGRESS * iterElement->second.curPastSize)/iterElement->second.element.TotalWeightSize);
            }

            calcTotalProgress();
        }


        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::calcTotalProgress()
        {
            CFAutoLock<CFLockObject> locker(&m_lockObject);

            INT64 curTotalPastSize = m_pastSize;

            std::list<DWORD>    finishedElementIdList;
            ElementProgressInfoIdMap::iterator iterElement = m_handleElementsIdMap.begin();
            for ( ; iterElement != m_handleElementsIdMap.end(); ++iterElement)
            {
                if (iterElement->second.element.TotalWeightSize == iterElement->second.finishedTaskSize)
                {
                    //over
                    finishedElementIdList.push_back(iterElement->first);
                    m_pastCount++;
                    m_pastSize+= iterElement->second.element.TotalWeightSize;
                    curTotalPastSize += iterElement->second.element.TotalWeightSize;
                }
                else
                {
                    curTotalPastSize += iterElement->second.curPastSize;
                }
            }

            ELEMENT maxPriorityElement = ELEMENT();
            double maxPriorityElementPercent = 0;
            ElementProgressInfoIdMap::iterator iterMaxPriorityElement = 
                std::max_element(m_handleElementsIdMap.begin(), m_handleElementsIdMap.end(),
                LessTaskProgressPriority<ElementProgressInfoIdMap::value_type>());

            FTLASSERT(iterMaxPriorityElement!= m_handleElementsIdMap.end());

            if (iterMaxPriorityElement!= m_handleElementsIdMap.end())
            {
                maxPriorityElement = iterMaxPriorityElement->second.element;
                maxPriorityElementPercent = (double)MAX_PROGRESS * iterMaxPriorityElement->second.curPastSize 
                    / iterMaxPriorityElement->second.element.TotalWeightSize;
            }

            for (std::list<DWORD>::iterator iterId = finishedElementIdList.begin();
                iterId != finishedElementIdList.end(); ++iterId)
            {
                m_handleElementsIdMap.erase(*iterId);
            }

            
            if (m_pProgressObserver)
            {
                m_pProgressObserver->OnTotalProgress((double)(MAX_PROGRESS * curTotalPastSize)/m_totalSize,
                    &maxPriorityElement, maxPriorityElementPercent);
            }
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::SetProgressObserver(IFProgressObserver<ELEMENT>  * pProgressObserver)
        {
            m_pProgressObserver = pProgressObserver;
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::SetTotalSize(INT64 totalSize)
        {
            m_totalSize = totalSize;
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::SetTotalCount(INT64 totalCount)
        {
            m_totalCount = totalCount;
        }   
        
        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::NotifyErrorElementId(DWORD elementId, ErrorOperation errOpt)
        {
            FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
            CFAutoLock<CFLockObject> locker(&m_lockObject);
            FTLASSERT(errOpt == eoBreakImmediately || errOpt == eoBreakDelay);
            m_handleElementsIdMap.erase(elementId);
        }

        template <typename ELEMENT>
        VOID CFProgressManager<ELEMENT>::Reset()
        {
            m_pastSize = 0LL;
            m_totalSize = 0LL;
            m_pastCount = 0LL;
            m_totalCount = 0LL;
            m_handleElementsIdMap.clear();
        }
}//FTL

#endif //FTL_PIPELINE_HPP