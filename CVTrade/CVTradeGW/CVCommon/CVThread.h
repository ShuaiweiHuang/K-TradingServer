/*
 * CSKThread.h
 *
 *  Created on: 2015年11月11日
 *      Author: alex
 */
#ifndef SKCOMMON_SKTHREAD_H_
#define SKCOMMON_SKTHREAD_H_

#include <pthread.h>

class CSKThread 
{
	private:
		pthread_t  m_tid;
		int        m_nRunning;
		int        m_nDetached;

	public:
		CSKThread();
		virtual ~CSKThread();

		int Start();
		int Join();
		int Detach();
		pthread_t Self();

		bool IsTerminated();

		virtual void* Run() = 0;
};
#endif /* SKCOMMON_SKTHREAD_H_ */
