#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>


#include "CVTandemDAOs.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKTandemDAOs* CSKTandemDAOs::instance = NULL;
pthread_mutex_t CSKTandemDAOs::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CSKTandemDAOs::CSKTandemDAOs()
{
	m_PEvent = CreateEvent();

	m_nRoundRobinIndexOfTandemDAO = 0;

	pthread_mutex_init(&m_MutexLockOnAddAvailableDAO,NULL);
	pthread_mutex_init(&m_MutexLockOnGetAvailableDAO,NULL);
	pthread_mutex_init(&m_MutexLockOnTriggerAddAvailableDAOEvent,NULL);

	m_nToSetTandemDAOID = 0;
}

CSKTandemDAOs::~CSKTandemDAOs() 
{
	DestroyEvent(m_PEvent);
	pthread_mutex_destroy(&m_MutexLockOnAddAvailableDAO);
	pthread_mutex_destroy(&m_MutexLockOnGetAvailableDAO);
	pthread_mutex_destroy(&m_MutexLockOnTriggerAddAvailableDAOEvent);
}

void CSKTandemDAOs::AddAvailableDAO()
{
	pthread_mutex_lock(&m_MutexLockOnAddAvailableDAO);//lock
	m_nToSetTandemDAOID += 1;
	CSKTandemDAO *pNewDAO = new CSKTandemDAO(m_nToSetTandemDAOID, m_nNumberOfWriteQueueDAO, m_kWriteQueueDAOStartKey, m_kWriteQueueDAOEndKey);

	m_vTandemDAO.push_back(pNewDAO);

	char caID[4];
	sprintf(caID, "%03d", m_nToSetTandemDAOID);
	FprintfStderrLog("ADD_TANDEMDAO", 0, reinterpret_cast<unsigned char*>(caID), 4);

	pthread_mutex_unlock(&m_MutexLockOnAddAvailableDAO);//unlock
}

CSKTandemDAO* CSKTandemDAOs::GetAvailableDAO()
{
	pthread_mutex_lock(&m_MutexLockOnGetAvailableDAO);//lock
	CSKTandemDAO* pTandemDAO = NULL;
	for(int count = 0; count < m_vTandemDAO.size(); count++)
	{
		if(m_vTandemDAO[m_nRoundRobinIndexOfTandemDAO]->IsInuse() == false)
		{
			pTandemDAO = m_vTandemDAO[m_nRoundRobinIndexOfTandemDAO];
			pTandemDAO->SetInuse(true);
			m_nRoundRobinIndexOfTandemDAO ++;
			m_nRoundRobinIndexOfTandemDAO %= m_vTandemDAO.size();
			break;
		}
		m_nRoundRobinIndexOfTandemDAO ++;
		m_nRoundRobinIndexOfTandemDAO %= m_vTandemDAO.size();
	}
	pthread_mutex_unlock(&m_MutexLockOnGetAvailableDAO);//unlock
	return pTandemDAO;
}

bool CSKTandemDAOs::IsDAOsFull()
{
	pthread_mutex_lock(&m_MutexLockOnAddAvailableDAO);
	int tandemDAOsize = m_vTandemDAO.size();
	pthread_mutex_unlock(&m_MutexLockOnAddAvailableDAO);

	return (tandemDAOsize >= m_nMaxNumberOfTandemDAO);
}

void* CSKTandemDAOs::Run()
{
	while(IsTerminated())
	{
		int nResult = WaitForEvent(m_PEvent, 1000);//1(sec)

		if(nResult == WAIT_TIMEOUT)
		{
			bool bAvailableDAO = false;

			for(vector<CSKTandemDAO*>::iterator iter = m_vTandemDAO.begin(); iter != m_vTandemDAO.end(); iter++)
			{
				bAvailableDAO = true;
				break;
			}
			if(!bAvailableDAO && !IsDAOsFull())
			{
				AddAvailableDAO();
			}
		}
		else if(nResult == 0)//trigger
		{
			AddAvailableDAO();
		}
	}
	return NULL;
}

CSKTandemDAOs* CSKTandemDAOs::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CSKTandemDAOs();
			cout << "TandemDAOs One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

void CSKTandemDAOs::TriggerAddAvailableDAOEvent()
{
	pthread_mutex_lock(&m_MutexLockOnTriggerAddAvailableDAOEvent);//lock

	SetEvent(m_PEvent);

	pthread_mutex_unlock(&m_MutexLockOnTriggerAddAvailableDAOEvent);//unlock
}

void CSKTandemDAOs::SetConfiguration(string strService, int nInitialConnection, int nMaximumConnection, int nNumberOfWriteQueueDAO,
					key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey)
{
	m_strService = strService;
	m_nDefaultNumberOfTandemDAO = nInitialConnection;
	m_nMaxNumberOfTandemDAO = nMaximumConnection;

	m_nNumberOfWriteQueueDAO = nNumberOfWriteQueueDAO;
	m_kWriteQueueDAOStartKey = kWriteQueueDAOStartKey;
	m_kWriteQueueDAOEndKey = kWriteQueueDAOEndKey;
}

void CSKTandemDAOs::StartUpDAOs()//todo
{

	for(int i=0 ; i<m_nDefaultNumberOfTandemDAO ; i++)
	{
		if(!IsDAOsFull())
		{
			AddAvailableDAO();
			printf("StartUpDAOs : %d\n", i);
		}
		else
		{
			cout << "DAOs is Full" << endl;
		}
	}

	char caTandemDAOCount[4];
	sprintf(caTandemDAOCount, "%03d", m_vTandemDAO.size());
	FprintfStderrLog("TANDEMDAO_COUNT", 0, reinterpret_cast<unsigned char*>(caTandemDAOCount), 4);
	Start();
}

string& CSKTandemDAOs::GetService()
{
	return m_strService;
}
