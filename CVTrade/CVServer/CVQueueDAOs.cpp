#include <unistd.h>

#include "CVQueueDAOs.h"

#include<iostream>
using namespace std;

CCVQueueDAOs* CCVQueueDAOs::instance = NULL;
pthread_mutex_t CCVQueueDAOs::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CCVQueueDAOs::CCVQueueDAOs()
{
	pthread_mutex_init(&m_MutexLockOnGetDAO,NULL);
}

CCVQueueDAOs::~CCVQueueDAOs() 
{
	for(vector<CCVQueueDAO*>::iterator iter = m_vQueueDAO.begin(); iter != m_vQueueDAO.end(); iter++)
	{
		delete *iter;
	}

	pthread_mutex_destroy(&m_MutexLockOnGetDAO);
}

void CCVQueueDAOs::AddDAO(key_t kSendKey,key_t kRecvKey)
{

	CCVQueueDAO* pNewDAO = new CCVQueueDAO(m_strService, kSendKey,kRecvKey);
	m_vQueueDAO.push_back(pNewDAO);
}

CCVQueueDAO* CCVQueueDAOs::GetDAO()
{
	pthread_mutex_lock(&m_MutexLockOnGetDAO);//lock

	CCVQueueDAO* pQueueDAO = NULL;
	for(vector<CCVQueueDAO*>::iterator iter = m_vQueueDAO.begin(); iter != m_vQueueDAO.end(); iter++)
	{
		if((*iter)->GetSendKey() == m_kRoundRobinIndexOfQueueDAO)
		{
			pQueueDAO = *iter;
			if(m_kRoundRobinIndexOfQueueDAO < m_kQueueDAOWriteEndKey)
				m_kRoundRobinIndexOfQueueDAO++;
			else
				m_kRoundRobinIndexOfQueueDAO = m_kQueueDAOWriteStartKey;
			break;
		}
	}

	/*for(vector<CCVQueueDAO*>::iterator iter = m_vQueueDAO.begin(); iter != m_vQueueDAO.end(); iter++)
	{
		if((*iter)->GetSendKey() == key //&& (*iter)->GetStatus() == qsIdle)
		{
			pQueueDAO = *iter;
			//pQueueDAO->SetStatus(qsInuse);
			break;
		}
	}*/

	pthread_mutex_unlock(&m_MutexLockOnGetDAO);//unlock
	return pQueueDAO;
}

void CCVQueueDAOs::RemoveDAO(key_t key)
{
}

void* CCVQueueDAOs::Run()
{
	return NULL;
}

CCVQueueDAOs* CCVQueueDAOs::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CCVQueueDAOs();
			cout << "QueueDAOs One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

void CCVQueueDAOs::SetConfiguration(string strService, int nNumberOfQueueDAO, key_t kQueueDAOWriteStartKey, key_t kQueueDAOWriteEndKey, key_t kQueueDAOReadStartKey, key_t kQueueDAOReadEndKey)
{
	m_strService = strService;
	m_nNumberOfQueueDAO = nNumberOfQueueDAO;
	m_kQueueDAOWriteStartKey = kQueueDAOWriteStartKey;
	m_kQueueDAOWriteEndKey = kQueueDAOWriteEndKey;
	m_kQueueDAOReadStartKey = kQueueDAOReadStartKey;
	m_kQueueDAOReadEndKey = kQueueDAOReadEndKey;

	m_kRoundRobinIndexOfQueueDAO = kQueueDAOWriteStartKey;
}

void CCVQueueDAOs::StartUpDAOs()
{
	key_t kWriteKey = m_kQueueDAOWriteStartKey;
	key_t kReadKey = m_kQueueDAOReadStartKey;

	for(kReadKey; kReadKey <= m_kQueueDAOReadEndKey; kWriteKey++, kReadKey++)
	{
		AddDAO(kWriteKey, kReadKey);
	}
}
