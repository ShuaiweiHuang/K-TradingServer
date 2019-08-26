#include <cstring>
#include <unistd.h>
#include <sys/time.h>

#include "CVWriteQueueDAOs.h"
#include "CVTandemDAO.h"

#include<iostream>
#include<cstdio>

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKWriteQueueDAOs* CSKWriteQueueDAOs::instance = NULL;

CSKWriteQueueDAOs* CSKWriteQueueDAOs::GetInstance()
{
	return instance;
}

CSKWriteQueueDAOs::CSKWriteQueueDAOs(int nWriteQueueDAOCount, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey)
{
	key_t kKey = kWriteQueueDAOStartKey;
	for(int i=0;i<nWriteQueueDAOCount;i++)
	{
		CSKWriteQueueDAO* pNewDAO = new CSKWriteQueueDAO(i+1, kKey);
		m_vWriteQueueDAO.push_back(pNewDAO);

		if(kKey < kWriteQueueDAOEndKey)
		{
			kKey++;
		}
		else if(kKey == kWriteQueueDAOEndKey)
		{
			kKey = kWriteQueueDAOStartKey;
		}
		else
			FprintfStderrLog("WRITE_QDAOS_KEY_ERROR", kKey, NULL, 0);
	}

	m_nWriteQueueDAOCount = nWriteQueueDAOCount;
	m_nWriteQueueDAORoundRobinIndex = 0;

	m_kWriteQueueDAOStartKey = kWriteQueueDAOStartKey;
	m_kWriteQueueDAOEndKey = kWriteQueueDAOEndKey;

	m_bAlarm = false;

	pthread_mutex_init(&m_MutexLockOnWriteQueueDAO,NULL);
	pthread_mutex_init(&m_MutexLockOnAlarm,NULL);

	m_PEvent = CreateEvent();
	printf("WriteQueueDAOs One\n");
	instance = this;
	Start();
}

CSKWriteQueueDAOs::~CSKWriteQueueDAOs()
{
	for(vector<CSKWriteQueueDAO*>::iterator iter = m_vWriteQueueDAO.begin(); iter != m_vWriteQueueDAO.end(); iter++)
	{
		if(*iter)
			delete *iter;

		*iter = NULL;
	}
	m_vWriteQueueDAO.clear();

	pthread_mutex_destroy(&m_MutexLockOnWriteQueueDAO);
	pthread_mutex_destroy(&m_MutexLockOnAlarm);

	DestroyEvent(m_PEvent);
}

void* CSKWriteQueueDAOs::Run()
{
	while(IsTerminated())
	{
		int nResult = WaitForEvent(m_PEvent, 500);
		if(nResult == WAIT_TIMEOUT)
		{
			for(vector<CSKWriteQueueDAO*>::iterator iter = m_vWriteQueueDAO.begin(); iter != m_vWriteQueueDAO.end(); iter++)
			{
				switch((*iter)->GetStatus())
				{
					case wsNone:
						FprintfStderrLog("WRITE_QDAO_STATUS_NONE_ERROR", -1, reinterpret_cast<unsigned char*>((*iter)->GetWriteQueueDAOID()), 4);
						break;

					case wsReadyForLooping:
					case wsReadyForRunning:
						break;

					case wsRunning:
						if(IsRunningTimeReasonable(*iter) == false)
						{
							(*iter)->SetStatus(wsBreakdown);
							FprintfStderrLog("RUNNING_TIME_ILLEGAL_ERROR", -1, reinterpret_cast<unsigned char*>((*iter)->GetWriteQueueDAOID()), 4);
							cout << "EndTime 	= " << m_timevalEnd.tv_sec*1000000 + m_timevalEnd.tv_usec << endl;
							cout << "StartTime 	= " << m_timevalStart.tv_sec*1000000 + m_timevalStart.tv_usec << endl;;
							ReSendReplyMessage(*iter);
						}
						break;

					case wsSendQFailed:
						FprintfStderrLog("WRITE_QDAO_STATUS_FAILED_ERROR", -1, reinterpret_cast<unsigned char*>((*iter)->GetWriteQueueDAOID()), 4);
						ReSendReplyMessage(*iter);
						break;
					case wsSendQSuccessful:
						if(m_bAlarm == true)
						{
							SetAlarm(false);
							FprintfStderrLog("TURN_ALARM_OFF", 0, reinterpret_cast<unsigned char*>((*iter)->GetWriteQueueDAOID()), 4);
						}
						break;
					case wsBreakdown:
						FprintfStderrLog("WRITE_QDAO_STATUS_BREAKDOWN_ERROR", -1, reinterpret_cast<unsigned char*>((*iter)->GetWriteQueueDAOID()), 4);
						break;
					default:
						FprintfStderrLog("WRITE_QDAO_STATUS_DEFAULT_ERROR", -1, reinterpret_cast<unsigned char*>((*iter)->GetWriteQueueDAOID()), 4);
						break;
				}
			}
		}
		else if(nResult == 0)
		{
			FprintfStderrLog("TRIGGERED_ERROR", -1, NULL, 0);
		}
		else if(nResult != 0)
		{
			FprintfStderrLog("WAIT_ERROR", -1, NULL, 0);
		}
		else
		{
			FprintfStderrLog("PEVENT_ERROR", -1, NULL, 0);
		}
	}
	return NULL;
}

CSKWriteQueueDAO* CSKWriteQueueDAOs::GetAvailableDAO()
{
	CSKWriteQueueDAO* pWriteQueueDAO = NULL;
	pthread_mutex_lock(&m_MutexLockOnWriteQueueDAO);
	for(int nCount = 0; nCount < m_vWriteQueueDAO.size(); nCount++)
	{
		m_nWriteQueueDAORoundRobinIndex ++;
		m_nWriteQueueDAORoundRobinIndex %= m_vWriteQueueDAO.size();

		if(m_vWriteQueueDAO[m_nWriteQueueDAORoundRobinIndex])
		{
			if(m_vWriteQueueDAO[m_nWriteQueueDAORoundRobinIndex]->GetStatus() == wsReadyForLooping 	||
			   m_vWriteQueueDAO[m_nWriteQueueDAORoundRobinIndex]->GetStatus() == wsSendQFailed 	||
			   m_vWriteQueueDAO[m_nWriteQueueDAORoundRobinIndex]->GetStatus() == wsSendQSuccessful)
			{
				pWriteQueueDAO = m_vWriteQueueDAO[m_nWriteQueueDAORoundRobinIndex];
				pWriteQueueDAO->SetStatus(wsReadyForRunning);
				break;
			}
			else
				continue;
		}
		else
		{
			FprintfStderrLog("WRITE_QDAO_NULL_ERROR", -1, NULL, 0);
		}
	}

	if(pWriteQueueDAO == NULL)
	{
		if(IsAllWriteQueueDAOBreakdown())
		{
			if(m_bAlarm == true)
				FprintfStderrLog("ALL_WRITE_QDAO_DOWN_ALARM", -111, NULL, 0);
			else
			{
				ReConstructWriteQueueDAO();
				FprintfStderrLog("ALL_WRITE_QDAO_DOWN_WARNING", -11, NULL, 0);
				SetAlarm(true);
			}
		}
		else
		{
			FprintfStderrLog("GETWRITE_QDAO_ERROR_WAIT_A_AVAILABLE_WRITE_QDAO", -1, NULL, 0);
		}
	}

	pthread_mutex_unlock(&m_MutexLockOnWriteQueueDAO);//unlock
	return pWriteQueueDAO;
}

bool CSKWriteQueueDAOs::IsAllWriteQueueDAOBreakdown()
{
	bool bIsAllWriteQueueDAOBreakdown = true;

	for(vector<CSKWriteQueueDAO*>::iterator iter = m_vWriteQueueDAO.begin(); iter != m_vWriteQueueDAO.end(); iter++)
	{
		if((*iter)->GetStatus() != 	wsBreakdown)
		{
			bIsAllWriteQueueDAOBreakdown = false;
			break;
		}
	}

	return bIsAllWriteQueueDAOBreakdown;
}

void CSKWriteQueueDAOs::ReConstructWriteQueueDAO()
{
	pthread_mutex_lock(&m_MutexLockOnWriteQueueDAO);//lock

	for(vector<CSKWriteQueueDAO*>::iterator iter = m_vWriteQueueDAO.begin(); iter != m_vWriteQueueDAO.end(); iter++)
	{
		if(*iter)
			delete *iter;

		*iter = NULL;
	}
	m_vWriteQueueDAO.clear();

	key_t kKey = m_kWriteQueueDAOStartKey;
	for(int i=0;i<m_nWriteQueueDAOCount;i++)
	{
		CSKWriteQueueDAO* pNewDAO = new CSKWriteQueueDAO(i+1, kKey);
		m_vWriteQueueDAO.push_back(pNewDAO);

		if(kKey < m_kWriteQueueDAOEndKey)
		{
			kKey++;
		}
		else if(kKey == m_kWriteQueueDAOEndKey)
		{
			kKey = m_kWriteQueueDAOStartKey;
		}
		else
			FprintfStderrLog("WRITE_QDAOS_KEY_ERROR", kKey, NULL, 0);
	}

	pthread_mutex_unlock(&m_MutexLockOnWriteQueueDAO);//unlock
}

void CSKWriteQueueDAOs::SetAlarm(bool bAlarm)
{
	pthread_mutex_lock(&m_MutexLockOnAlarm);//lock

	m_bAlarm = bAlarm;

	pthread_mutex_unlock(&m_MutexLockOnAlarm);//unlock
}

bool CSKWriteQueueDAOs::IsRunningTimeReasonable(CSKWriteQueueDAO* pWriteQueueDAO)
{
	bool bIsRunningTimeReasonable = false;

	gettimeofday(&m_timevalEnd, NULL);

	if(pWriteQueueDAO)
	{
		memset(&m_timevalStart, 0, sizeof(struct timeval));
		memcpy(&m_timevalStart, &pWriteQueueDAO->GetStartTimeval(), sizeof(struct timeval));

		if((m_timevalEnd.tv_sec - m_timevalStart.tv_sec) == 0)//less than 1(sec) is reasonable
			bIsRunningTimeReasonable = true;
		else if((m_timevalEnd.tv_sec - m_timevalStart.tv_sec) == 1)
			m_timevalEnd.tv_usec > m_timevalStart.tv_usec ? bIsRunningTimeReasonable = false : bIsRunningTimeReasonable = true;
		else
			bIsRunningTimeReasonable = false;
	}
	else
		FprintfStderrLog("IS_RUNNINH_TIME_REASONABLE_WRITE_QDAO_NULL_ERROR", -1, NULL, 0);

	return bIsRunningTimeReasonable;
}

void CSKWriteQueueDAOs::ReSendReplyMessage(CSKWriteQueueDAO* pWriteQueueDAO)
{
	while(pWriteQueueDAO)
	{
		CSKWriteQueueDAO* pAvailableWriteQueueDAO = GetAvailableDAO();
		if(pAvailableWriteQueueDAO)
		{
			pAvailableWriteQueueDAO->SetReplyMessage(pWriteQueueDAO->GetReplyMessage(), pWriteQueueDAO->GetReplyMessageLength());
			pAvailableWriteQueueDAO->TriggerWakeUpEvent();
			break;
		}
		else
		{
			FprintfStderrLog("RECEND_GET_WRITE_QDAO_NULL_ERROR", -1, NULL, 0);
			usleep(200);
		}
	}
}
