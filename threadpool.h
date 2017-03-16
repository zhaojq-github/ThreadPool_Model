#ifndef __ZRPC_THREADPOOL_H__
#define __ZRPC_THREADPOOL_H__
// system header files
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <iostream>
using namespace std;

typedef sem_t Semaphore;
typedef enum
{
	THREAD_IDLE =0,
	THREAD_RUNNING,
	THREAD_SUSPENDED,
	THREAD_TERMINATED,
	THREAD_FINISHED,
	THREAD_DEAD
}ThreadState;

//define thread errocde
#define Error_ThreadSuccess			0
#define Error_ThreadInit				1
#define Error_ThreadCreate			2	
#define Error_ThreadSuspend			3
#define Error_ThreadResume			4	
#define Error_ThreadTerminated	5
#define Error_ThreadExit				6	
#define Error_ThreadSetPriority	7
#define Error_ThreadWakeup			8
#define Error_ThreadYield				9
#define Error_ThreadDetach			10
#define Error_ThreadJoin				11

class CThread {
	private:
		int m_ErrCode;
		Semaphore m_ThreadSemaphore;	//内部信号量
		unsigned long m_ThreadID;
		bool m_Detach;
		bool m_CreateSuspended;
		char* m_ThreadName;
		ThreadState m_ThreadState;
	
	protected:
		void SetErrcode(int errcode) {m_ErrCode = errcode;}
		static void* ThreadFunction(void*);
	
	public:
		CThread();
		CThread(bool createsuspended, bool detach);
		virtual ~CThread();
		
		virtual void Run(void) = 0;
		bool Terminate(void);	//终止线程
		bool Start(void);			//开始执行线程
		void Exit(void);
		bool Wakeup(void);
		bool Detach(void);
		bool Join(void);
		bool Yield(void);
		int Self(void);
		
		int GetThreadID(void) {return m_ThreadID;}
		int GetLastError(void) {return m_ErrCode;}
		
		void SetThreadState(ThreadState state) {m_ThreadState = state;}
		ThreadState GetThreadState(void) {return m_ThreadState;}
		void SetThreadName(char* thrname) {strcpy(m_ThreadName, thrname);}
		char* GetThreadName(void) {return m_ThreadName;}
		bool SetPriority(int priority);
		int GetPriority(void);
		int SetConcurrency(int num);
		int GetConcurrency(void);
};

class CJob {
	private:
		int m_JobNo;
		char* m_JobName;
		CThread* m_pWorkThread;
	
	public:
		CJob(void);
		virtual ~CJob();
		
		int GetJobNo(void) const {return m_JobNo;}
		void SetJobNo(int jobno) {m_JobNo = jobno;}
		char* GetJobName(void) const {return m_JobName;}
		void SetJobName(char* jobname);
		
		CThread* GetWorkThread(void) {return m_pWorkThread;}
		void SetWorkThread(CThread* pWorkThread) {
			m_pWorkThread = pWorkThread;
		}
		bool GetTerminated(void);
		virtual void Run(void* ptr) = 0;
};

//===============================================================
//														CThreadMutex
//用于线程之间的互斥
//===============================================================			
class CThreadMutex {
	friend class CCondition;
  
	protected:
		pthread_mutex_t _mutex;
		pthread_mutexattr_t _mutex_attr;

	public:
		CThreadMutex()
		{ 
			pthread_mutexattr_init( &_mutex_attr );
			pthread_mutex_init( &_mutex, &_mutex_attr );
		}

		~CThreadMutex()
		{ 
			pthread_mutex_destroy( &_mutex );
			pthread_mutexattr_destroy( &_mutex_attr );
		}

		int Lock() { return pthread_mutex_lock( &_mutex ); }
		int Unlock() { return pthread_mutex_unlock( &_mutex ); }
		int Trylock() { return pthread_mutex_trylock( &_mutex ); }
		pthread_mutex_t* GetMutex() { return &_mutex; }
};

//===============================================================
//														CCondition
//条件变量的分装，用于线程之间的同步
//===============================================================		
class CCondition {
	protected:
		pthread_cond_t  _cond;
		CThreadMutex	_mutex;

	public:
		CCondition() { pthread_cond_init( &_cond, NULL ); }
		~CCondition() { pthread_cond_destroy( &_cond ); }

		void Wait(){ pthread_cond_wait(&_cond, _mutex.GetMutex()); }
		void Signal(){ pthread_cond_signal( &_cond );	}
		void Broadcast(){ pthread_cond_broadcast( &_cond ); }
};

class CThreadPool;
class CWorkerThread:public CThread {
	private:
		CThreadPool* m_ThreadPool;
		CJob* m_Job;
		void* m_JobData;
		CThreadMutex m_VarMutex;
		bool m_IsEnd;
	
	public:
		CCondition m_JobCond;
		CThreadMutex m_WorkMutex;
		CWorkerThread();
		virtual ~CWorkerThread();
		
		void Run();
		void SetJob(CJob* job, void* jobdata);
		CJob* GetJob(void) {return m_Job;}
		void SetThreadPool(CThreadPool* threadpool);
		CThreadPool* GetThreadPool(void) {return m_ThreadPool;}
};

class CThreadPool {
	friend class CWorkerThread;
	private:
		unsigned int m_MaxNum; 		//当前线程池中所允许并发存在的线程的最大数目
		unsigned int m_AvailLow;	//当前线程池中所允许存在的空闲线程的最小数目
		unsigned int m_AvailHigh;	//当前线程池中所允许存在的空闲线程的最大数目
		unsigned int m_AvailNum;	//当前线程池中实际存在的空闲线程数目
		unsigned int m_InitNum;		//初始创建时线程池中的线程个数
	
	protected:
		CWorkerThread* GetIdleThread(void);
		void AppendToIdleList(CWorkerThread* jobthread);
		void MoveToBusyList(CWorkerThread* idlethread);
		void MoveToIdleList(CWorkerThread* busythread);
		void DeleteIdleThread(int num);
		void CreateIdleThread(int num);
	
	public:
		CThreadMutex m_BusyMutex;	//when visit busy list,use m_BusyMutex to lock and unlock
		CThreadMutex m_IdleMutex;	//when visit idle list,use m_IdleMutex to lock and unlock
		//CThreadMutex	m_JobMutex;
		CThreadMutex m_VarMutex;
		
		CCondition m_BusyCond;		//m_BusyCond is used to sync busy thread list
		CCondition m_IdleCond;		//m_IdleCond is used to sync idle thread list
		//CCondition m_IdleJobCond;
		CCondition m_MaxNumCond;
		
		vector<CWorkerThread*> m_ThreadList;
		vector<CWorkerThread*> m_BusyList;
		vector<CWorkerThread*> m_IdleList;
		//vector<CJob*> m_JobList;   
		
		CThreadPool();
		CThreadPool(int initnum);
		virtual ~CThreadPool();
		
		void SetMaxNum(int maxnum) {m_MaxNum = maxnum;}
		unsigned int GetMaxNum(void) {return m_MaxNum;}
		void SetAvailLowNum(int minnum) {m_AvailLow = minnum;}
		unsigned int GetAvailLowNum(void) {return m_AvailLow;}
		void SetAvailHighNum(int highnum) {m_AvailHigh = highnum;}
		unsigned int GetAvailHighNum(void) {return m_AvailHigh;}
		unsigned int GetActualAvailNum(void) {return m_AvailNum;}
		unsigned int GetAllNum(void) {return m_ThreadList.size();}
		unsigned int GetBusyNum(void) {return m_BusyList.size();}
		void SetInitNum(int initnum) {m_InitNum = initnum;}
		unsigned int GetInitNum(void) {return m_InitNum;}
		
		void TerminateAll(void);
		void Run(CJob* job, void* jobdata);
};

class CThreadManage {
	private:
		CThreadPool* m_Poll;	//指向实际的线程池
		int m_NumOfThread;		//初始允许创建的并发的线程个数

	public:
		CThreadManage();
		CThreadManage(int num);
		virtual ~CThreadManage();
		
		void SetParallelNum(int num);
		void Run(CJob* job, void* jobdata);
		void TerminateAll(void);
};
#endif  // __ZRPC_THREADPOOL_H__
