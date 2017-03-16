#include <stdio.h>
#include <sys/types.h>
#include <unistd.h> 
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)
#include "threadpool.h"
class ZRPC_Job:public CJob
{
	private:
		CThreadMutex mutex;
	public:
		void Run(void* jobdata)
		{
			mutex.Lock();
			//gettid()获取线程ID；getpid()获取进程ID
			fprintf(stdout, "Job is set to thread id:[%lu] pid:[%lu]\n", gettid(), getpid());
			mutex.Unlock();
		}
};
	
int main()
{
	int i = 0;
	CThreadManage* manage = new CThreadManage(5);
	ZRPC_Job* job = new ZRPC_Job();
	for(; i < 5; i++) {
		manage->Run(job, NULL);	
	}
	while(1);
	return 0;
}
