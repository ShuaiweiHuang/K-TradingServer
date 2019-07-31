#ifndef CVCOMMON_CVTHREAD_H_
#define CVCOMMON_CVTHREAD_H_

#include <pthread.h>

class CCVThread 
{
	private:
		pthread_t  m_tid;
		int        m_nRunning;
		int        m_nDetached;

	public:
		CCVThread();
		virtual ~CCVThread();

		int Start();
		int Join();
		int Detach();
		pthread_t Self();

		bool IsTerminated();

		virtual void* Run() = 0;
};
#endif /* CVCOMMON_CVTHREAD_H_ */
