#include "CVThread.h"

#ifdef MNTRMSG
#include "../CVServers.h"
extern struct MNTRMSGS g_MNTRMSG;
pthread_mutex_t mtxThreadCount = PTHREAD_MUTEX_INITIALIZER;
#endif

static void *ThreadExecution(void* pArg)////////////
{
	return ((CSKThread*)pArg)->Run();
}

CSKThread::CSKThread(): m_tid(0), m_nRunning(0), m_nDetached(0)
{
#ifdef MNTRMSG
	pthread_mutex_lock(&mtxThreadCount);
	g_MNTRMSG.num_of_thread_Current++;
	g_MNTRMSG.num_of_thread_Max = (g_MNTRMSG.num_of_thread_Current > g_MNTRMSG.num_of_thread_Max) ? g_MNTRMSG.num_of_thread_Current : g_MNTRMSG.num_of_thread_Max;
	pthread_mutex_unlock(&mtxThreadCount);
#endif
}

CSKThread::~CSKThread()
{
#ifdef MNTRMSG
	pthread_mutex_lock(&mtxThreadCount);
	g_MNTRMSG.num_of_thread_Current--;
	pthread_mutex_unlock(&mtxThreadCount);
#endif
	if ( m_nRunning == 1 && m_nDetached == 0)
	{
		pthread_detach(m_tid);
	}

	if ( m_nRunning == 1)
	{
		pthread_cancel(m_tid);
	}
}

int CSKThread::Start()
{
	int result = pthread_create(&m_tid, NULL, ThreadExecution, this);

	if (result == 0)
	{
		m_nRunning = 1;
	}
	return result;
}

int CSKThread::Join()
{
	int result = -1;

	if ( m_nRunning == 1) 
	{
		result = pthread_join(m_tid, NULL);

		if (result == 0) 
		{
			m_nDetached = 0;
		}
	}
	return result;
}

int CSKThread::Detach()
{
	int result = -1;

	if (m_nRunning == 1 && m_nDetached == 0) 
	{
		result = pthread_detach(m_tid);

		if (result == 0) 
		{
			m_nDetached = 1;
		}
	}
	return result;
}

pthread_t CSKThread::Self()//pthread_self();
{
	return m_tid;
}

bool CSKThread::IsTerminated()
{
	return (m_nRunning==1)? true:false;
}
