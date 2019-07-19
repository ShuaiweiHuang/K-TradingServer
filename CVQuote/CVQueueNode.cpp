#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sys/time.h>
#include <assert.h>

#include "CVQueueNode.h"
#include "CVServer.h"

using namespace std;

CSKQueueDAO::CSKQueueDAO(string strService, key_t kSendKey, key_t kRecvKey)
{
	m_strService = strService;

	m_kSendKey = kSendKey;
	m_kRecvKey = kRecvKey;

	m_pSendQueue = new CSKQueue;
	m_pSendQueue->Create(m_kSendKey);

	m_pRecvQueue = new CSKQueue;
	m_pRecvQueue->Create(m_kRecvKey);

	Start();
}

CSKQueueDAO::~CSKQueueDAO() 
{
	if(m_pSendQueue != NULL) 
	{
		delete m_pSendQueue;
	}

	if(m_pRecvQueue != NULL) 
	{
		delete m_pRecvQueue;
	}
}

void* CSKQueueDAO::Run()
{
	CSKClients* pClients = CSKClients::GetInstance();
	assert(pClients);

	char uncaRecvBuf[BUFSIZE];
	while(m_pRecvQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		int nGetMessage = m_pRecvQueue->GetMessage(uncaRecvBuf);
#ifdef DEBUG
		printf("SERVER: queue data read at key %d\n", m_kRecvKey);
#endif
		if(nGetMessage > 0)
		{
			vector<shared_ptr<CSKClient> >::iterator iter = pClients->m_vClient.begin();
			while(iter != pClients->m_vClient.end())
			{
				CSKClient* pClient = (*iter).get();
				if(pClient->GetStatus() == csOffline && (*iter).unique()) {
					iter++;
					continue;
				}
				pClient->SendAll(NULL, uncaRecvBuf, strlen(uncaRecvBuf));
#ifdef DEBUG
				printf("send msg: %s\n", uncaRecvBuf);
#endif
				iter++;
			}
		}
	}
	return NULL;
}

int CSKQueueDAO::SendData(char* pBuf, int nSize, long lType, int nFlag)
{
	int nResult = -1;
#ifdef DEBUG
	printf("SERVER: queue data send at key %d\n", m_kSendKey);
#endif
	if(m_pSendQueue)
	{
		nResult= m_pSendQueue->SendMessage(pBuf, nSize, lType, nFlag);
	}
	else
	{
		return nResult;
	}

	return nResult;
}

key_t CSKQueueDAO::GetRecvKey()
{
	return m_kRecvKey;
}

key_t CSKQueueDAO::GetSendKey()
{
	return m_kSendKey;
}
