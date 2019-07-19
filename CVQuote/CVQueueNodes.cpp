#include <unistd.h>

#include "CVQueueNodes.h"

#include<iostream>
using namespace std;

CSKQueueDAOs* CSKQueueDAOs::instance = NULL;

pthread_mutex_t CSKQueueDAOs::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CSKQueueDAOs::CSKQueueDAOs()
{
	pthread_mutex_init(&m_MutexLockOnGetDAO,NULL);
}

CSKQueueDAOs::~CSKQueueDAOs() 
{
	for(vector<CSKQueueDAO*>::iterator iter = m_vQueueDAO.begin(); iter != m_vQueueDAO.end(); iter++)
	{
		delete *iter;
	}

	pthread_mutex_destroy(&m_MutexLockOnGetDAO);
}

void CSKQueueDAOs::AddDAO(key_t kSendKey,key_t kRecvKey)
{
	printf("add queue node with key id = send:%d, recv:%d\n", kSendKey, kRecvKey);
	CSKQueueDAO* pNewDAO = new CSKQueueDAO(m_strService, kSendKey, kRecvKey);
	m_vQueueDAO.push_back(pNewDAO);
}

CSKQueueDAO* CSKQueueDAOs::GetDAO()
{
	pthread_mutex_lock(&m_MutexLockOnGetDAO);//lock

	CSKQueueDAO* pQueueDAO = NULL;

	for(vector<CSKQueueDAO*>::iterator iter = m_vQueueDAO.begin(); iter != m_vQueueDAO.end(); iter++)
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

	pthread_mutex_unlock(&m_MutexLockOnGetDAO);//unlock
	return pQueueDAO;
}

void* CSKQueueDAOs::Run()
{
	return NULL;
}

CSKQueueDAOs* CSKQueueDAOs::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CSKQueueDAOs();
			cout << "QueueDAOs One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

void CSKQueueDAOs::SetConfiguration(string strService, int nNumberOfQueueDAO, key_t kQueueDAOWriteStartKey, key_t kQueueDAOWriteEndKey, key_t kQueueDAOReadStartKey, key_t kQueueDAOReadEndKey)
{
	m_strService = strService;
	m_nNumberOfQueueDAO = nNumberOfQueueDAO;
	m_kQueueDAOWriteStartKey = kQueueDAOWriteStartKey;
	m_kQueueDAOWriteEndKey = kQueueDAOWriteEndKey;
	m_kQueueDAOReadStartKey = kQueueDAOReadStartKey;
	m_kQueueDAOReadEndKey = kQueueDAOReadEndKey;

	m_kRoundRobinIndexOfQueueDAO = kQueueDAOWriteStartKey;
}

void CSKQueueDAOs::StartUpDAOs()
{
	key_t kWriteKey = m_kQueueDAOWriteStartKey;
	key_t kReadKey = m_kQueueDAOReadStartKey;

	for(kReadKey; kReadKey <= m_kQueueDAOReadEndKey; kWriteKey++, kReadKey ++)
	{
		AddDAO(kWriteKey, kReadKey);
	}
}
