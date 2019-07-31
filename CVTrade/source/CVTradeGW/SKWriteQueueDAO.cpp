#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>

#include "SKWriteQueueDAO.h"
#include "SKTandemDAOs.h"
#include "SKWriteQueueDAO.h"
#include "../include/SKTIGMessage.h"

#include "SKReadQueueDAOs.h"//timetest
#include "../include/TIGOFFormat.h"//timetest

#ifdef MNTRMSG
extern struct MNTRMSGS g_MNTRMSG;
#endif

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

static void LogTime(char* pOrderNumber, char cSymbol)//nano second
{
	struct timespec end;

	clock_gettime(CLOCK_REALTIME, &end);
	long time = 1000000000 * (end.tv_sec) + end.tv_nsec; 

	//string filename = to_string(nOrderNumber) + ".txt";
	string filename(pOrderNumber);
	filename = filename + ".txt";

	fstream fp;
	fp.open(filename, ios::out|ios::app);
	fp << time<< cSymbol ;
	fp.close();
}

CSKWriteQueueDAO::CSKWriteQueueDAO(int nWriteQueueDAOID, key_t key)
{
	sprintf(m_caWriteQueueDAOID, "%03d", nWriteQueueDAOID);

	m_kWriteKey = key;

	m_pWriteQueue = new CSKQueue;
	m_pWriteQueue->Create(m_kWriteKey);

	m_PEvent = CreateEvent();

	m_nReplyMessageSize = 0;

	m_WriteQueueDAOStatus = wsNone;

	pthread_mutex_init(&m_MutexLockOnSetStatus,NULL);

	Start();
}

CSKWriteQueueDAO::~CSKWriteQueueDAO() 
{
	if(m_pWriteQueue != NULL) 
	{
		delete m_pWriteQueue;

		m_pWriteQueue = NULL;
	}

	DestroyEvent(m_PEvent);

	pthread_mutex_destroy(&m_MutexLockOnSetStatus);
}

void* CSKWriteQueueDAO::Run()
{
#ifdef TIMETEST
	struct  timeval timeval_Start;
	struct  timeval timeval_End;
	static int write_order_count = 0;
#endif

	SetStatus(wsReadyForLooping);
	while(m_pWriteQueue)
	{
		int nResult = WaitForEvent(m_PEvent, -1);

		gettimeofday(&m_timevalStart, NULL);

		SetStatus(wsRunning);

		if(nResult == WAIT_TIMEOUT)
		{
			cout << "Wait Timeout!" << endl;
		}
		else if(nResult == 0)
		{
#ifdef TIMETEST
			gettimeofday (&timeval_Start, NULL) ;
#endif
			int nResult = m_pWriteQueue->SendMessage(m_uncaReplyMessage, m_nReplyMessageSize);

			if(nResult == 0)
			{
#ifdef TIMETEST
	            gettimeofday (&timeval_End, NULL) ;
                InsertTimeLogToSharedMemory(&timeval_Start, &timeval_End, tpTandemToQueueDAO, write_order_count);
                write_order_count++;
#endif
#ifdef MNTRMSG
			    g_MNTRMSG.num_of_order_Reply++;
#endif
				FprintfStderrLog("SEND_Q", 0, m_uncaReplyMessage, m_nReplyMessageSize);
				SetStatus(wsSendQSuccessful);
			}
			else if(nResult == -1)
			{
				FprintfStderrLog("SEND_Q_ERROR", -1, reinterpret_cast<unsigned char*>(m_caWriteQueueDAOID), sizeof(m_caWriteQueueDAOID), m_uncaReplyMessage, m_nReplyMessageSize);
				perror("SEND_Q_ERROR");
				SetStatus(wsSendQFailed);
			}
		}
		else if(nResult != 0)
		{
			cout << "Error in wait!" << endl;
		}
		else
		{
			cout << "Else Error!" << endl;
		}
	}

	return NULL;
}

void CSKWriteQueueDAO::SetReplyMessage(unsigned char* pReplyMessage, int nReplyMessageSize)
{
	memset(m_uncaReplyMessage, 0, sizeof(m_uncaReplyMessage));
	memcpy(m_uncaReplyMessage, pReplyMessage, nReplyMessageSize);

	m_nReplyMessageSize = nReplyMessageSize;	
}

void CSKWriteQueueDAO::TriggerWakeUpEvent()
{
	SetEvent(m_PEvent);
}

/*void CSKWriteQueueDAO::SetIsAvailableStatus(bool bAvailableStatus)
{
	m_bAvailable = bAvailableStatus;
}*/

char* CSKWriteQueueDAO::GetWriteQueueDAOID()
{
	return m_caWriteQueueDAOID;
}

void CSKWriteQueueDAO::SetStatus(TSKWriteQueueDAOStatus wsStatus)
{
	pthread_mutex_lock(&m_MutexLockOnSetStatus);//lock

	m_WriteQueueDAOStatus = wsStatus;

	pthread_mutex_unlock(&m_MutexLockOnSetStatus);//unlock
}

struct timeval& CSKWriteQueueDAO::GetStartTimeval()
{
	return m_timevalStart;
}

unsigned char* CSKWriteQueueDAO::GetReplyMessage()
{
	return m_uncaReplyMessage;
}

TSKWriteQueueDAOStatus CSKWriteQueueDAO::GetStatus()
{
	return m_WriteQueueDAOStatus;
}

int CSKWriteQueueDAO::GetReplyMessageLength()
{
	return m_nReplyMessageSize;	
}
