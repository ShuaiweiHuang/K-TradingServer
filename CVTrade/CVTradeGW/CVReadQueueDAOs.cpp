#include <cstring>
#include <unistd.h>

#include "CVReadQueueDAOs.h"

#include<iostream>
#include<cstdio>
using namespace std;

CSKReadQueueDAOs* CSKReadQueueDAOs::instance = NULL;
pthread_mutex_t CSKReadQueueDAOs::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CSKReadQueueDAOs::CSKReadQueueDAOs()
{
	for(int i=0;i<AMOUNT_OF_HASH_SIZE;i++)
	{
		m_OrderNumberHash[i].bAvailable = true;
	}

	pthread_mutex_init(&m_MutexLockOnGetDAO,NULL);
	pthread_mutex_init(&m_MutexLockOnSerialNumber,NULL);
}

CSKReadQueueDAOs::~CSKReadQueueDAOs() 
{
	for(vector<CSKReadQueueDAO*>::iterator iter = m_vReadQueueDAO.begin(); iter != m_vReadQueueDAO.end(); iter++)
	{
		delete *iter;
	}

	if(m_pSharedMemoryOfSerialNumber)
	{
		m_pSharedMemoryOfSerialNumber->DetachSharedMemory();
	
		delete m_pSharedMemoryOfSerialNumber;
		m_pSharedMemoryOfSerialNumber = NULL;
	}

	pthread_mutex_destroy(&m_MutexLockOnGetDAO);
	pthread_mutex_destroy(&m_MutexLockOnSerialNumber);
}

void CSKReadQueueDAOs::AddDAO(key_t key)
{
	CSKReadQueueDAO* pNewDAO = new CSKReadQueueDAO(key, m_strService, m_strOTSID);
	m_vReadQueueDAO.push_back(pNewDAO);
}

CSKReadQueueDAO* CSKReadQueueDAOs::GetDAO(key_t key)//not finished
{
	pthread_mutex_lock(&m_MutexLockOnGetDAO);//lock

	pthread_mutex_unlock(&m_MutexLockOnGetDAO);//unlock
	return NULL;
}

void CSKReadQueueDAOs::InsertOrderNumberToHash(long lTIGNumber, unsigned char* pOrderNumber)
{
	int nHashNumber = lTIGNumber % AMOUNT_OF_HASH_SIZE;

	memset(m_OrderNumberHash[nHashNumber].uncaOrderNumber, 0, 13);
	memcpy(m_OrderNumberHash[nHashNumber].uncaOrderNumber, pOrderNumber, 13);
}

bool CSKReadQueueDAOs::GetOrderNumberHashStatus(long lTIGNumber)
{
	int nHashNumber = lTIGNumber % AMOUNT_OF_HASH_SIZE;

	return m_OrderNumberHash[nHashNumber].bAvailable;
}

void CSKReadQueueDAOs::SetOrderNumberHashStatus(long lTIGNumber, bool bStatus)
{
	int nHashNumber = lTIGNumber % AMOUNT_OF_HASH_SIZE;
	m_OrderNumberHash[nHashNumber].bAvailable = bStatus;
}

void CSKReadQueueDAOs::GetOrderNumberFromHash(long lTIGNumber, unsigned char* pBuf)
{
	int nHashNumber = lTIGNumber % AMOUNT_OF_HASH_SIZE;

	memcpy(pBuf, m_OrderNumberHash[nHashNumber].uncaOrderNumber, 13);
}

void CSKReadQueueDAOs::RemoveDAO(key_t key)
{
}

void* CSKReadQueueDAOs::Run()
{
	return NULL;
}

CSKReadQueueDAOs* CSKReadQueueDAOs::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CSKReadQueueDAOs();
			cout << "ReadQueueDAOs One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

void CSKReadQueueDAOs::SetConfiguration(string strService, string strOTSID, int nNumberOfReadQueueDAO, key_t kReadQueueDAOStartKey, key_t kReadQueueDAOEndKey, key_t kTIGNumberSharedMemoryKey)
{
	m_strService = strService;
	m_strOTSID = strOTSID;
	m_nNumberOfReadQueueDAO = nNumberOfReadQueueDAO;
	m_kReadQueueDAOStartKey = kReadQueueDAOStartKey;
	m_kReadQueueDAOEndKey = kReadQueueDAOEndKey;

	m_kTIGNumberSharedMemoryKey = kTIGNumberSharedMemoryKey;
}

void CSKReadQueueDAOs::StartUpDAOs()
{
	m_pSharedMemoryOfSerialNumber = new CSKSharedMemory(m_kTIGNumberSharedMemoryKey, sizeof(long));
	m_pSharedMemoryOfSerialNumber->AttachSharedMemory();
	m_pSerialNumber = (long*)m_pSharedMemoryOfSerialNumber->GetSharedMemory();

	for(key_t kKey = m_kReadQueueDAOStartKey; kKey <= m_kReadQueueDAOEndKey; kKey++)
	{
			//printf("add gw key : %d\n", kKey);
			AddDAO(kKey);
	}
}

long CSKReadQueueDAOs::GetSerialNumber()
{
	pthread_mutex_lock(&m_MutexLockOnSerialNumber);//lock

	(*m_pSerialNumber) += 1;

	pthread_mutex_unlock(&m_MutexLockOnSerialNumber);//unlock

	return *m_pSerialNumber;
}
