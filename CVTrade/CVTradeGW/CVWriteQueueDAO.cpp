#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>

#include "CVWriteQueueDAO.h"
#include "CVTandemDAOs.h"
#include "CVWriteQueueDAO.h"
#include "../include/CVTIGMessage.h"

#include "CVReadQueueDAOs.h"


using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVWriteQueueDAO::CCVWriteQueueDAO(int nWriteQueueDAOID, key_t key)
{
	sprintf(m_caWriteQueueDAOID, "%03d", nWriteQueueDAOID);

	m_kWriteKey = key;

	m_pWriteQueue = new CCVQueue;
	m_pWriteQueue->Create(m_kWriteKey);

	m_PEvent = CreateEvent();

	m_nReplyMessageSize = 0;

	m_WriteQueueDAOStatus = wsNone;

	pthread_mutex_init(&m_MutexLockOnSetStatus,NULL);

	Start();
}

CCVWriteQueueDAO::~CCVWriteQueueDAO() 
{
	if(m_pWriteQueue != NULL) 
	{
		delete m_pWriteQueue;

		m_pWriteQueue = NULL;
	}

	DestroyEvent(m_PEvent);

	pthread_mutex_destroy(&m_MutexLockOnSetStatus);
}

void* CCVWriteQueueDAO::Run()
{
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
			int nResult = m_pWriteQueue->SendMessage(m_uncaReplyMessage, m_nReplyMessageSize);

			if(nResult == 0)
			{
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

void CCVWriteQueueDAO::SetReplyMessage(unsigned char* pReplyMessage, int nReplyMessageSize)
{
	memset(m_uncaReplyMessage, 0, sizeof(m_uncaReplyMessage));
	memcpy(m_uncaReplyMessage, pReplyMessage, nReplyMessageSize);
	m_nReplyMessageSize = nReplyMessageSize;	
}

void CCVWriteQueueDAO::TriggerWakeUpEvent()
{
	SetEvent(m_PEvent);
}

char* CCVWriteQueueDAO::GetWriteQueueDAOID()
{
	return m_caWriteQueueDAOID;
}

void CCVWriteQueueDAO::SetStatus(TCVWriteQueueDAOStatus wsStatus)
{
	pthread_mutex_lock(&m_MutexLockOnSetStatus);//lock

	m_WriteQueueDAOStatus = wsStatus;

	pthread_mutex_unlock(&m_MutexLockOnSetStatus);//unlock
}

struct timeval& CCVWriteQueueDAO::GetStartTimeval()
{
	return m_timevalStart;
}

unsigned char* CCVWriteQueueDAO::GetReplyMessage()
{
	return m_uncaReplyMessage;
}

TCVWriteQueueDAOStatus CCVWriteQueueDAO::GetStatus()
{
	return m_WriteQueueDAOStatus;
}

int CCVWriteQueueDAO::GetReplyMessageLength()
{
	return m_nReplyMessageSize;	
}
