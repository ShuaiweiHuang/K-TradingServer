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

#define RPMSGLEN 256

CCVQueueDAO::CCVQueueDAO(string strService, key_t kSendKey, key_t kRecvKey)
{
	m_strService = strService;

	m_kSendKey = kSendKey;
	m_kRecvKey = kRecvKey;

	m_pSendQueue = new CCVQueue;
	m_pSendQueue->Create(m_kSendKey);

	m_pRecvQueue = new CCVQueue;
	m_pRecvQueue->Create(m_kRecvKey);

	//Start();
}

CCVQueueDAO::~CCVQueueDAO() 
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

void* CCVQueueDAO::Run()
{
#if 0
	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	char uncaRecvBuf[BUFSIZE];
	while(m_pRecvQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		sleep(1);
		int nGetMessage = m_pRecvQueue->GetMessage(uncaRecvBuf);
#ifdef DEBUG
		printf("SERVER: queue data read at key %d, size = %d\n", m_kRecvKey, nGetMessage);
#endif
		if(nGetMessage > 0)
		{
			vector<shared_ptr<CCVClient> >::iterator iter = pClients->m_vClient.begin();
			printf("TCP: queue data read at key %d, size = %d\n", m_kRecvKey, nGetMessage);
			while(iter != pClients->m_vClient.end())
			{
				CCVClient* pClient = (*iter).get();
				if(pClient->GetStatus() == csOffline && (*iter).unique()) {
					iter++;
					continue;
				}
				if(pClient->SendAll(NULL, uncaRecvBuf, strlen(uncaRecvBuf)) != false) {
					//pClient->m_pHeartbeat->TriggerGetReplyEvent();
				}
				iter++;
			}
		}
	}
	return NULL;
#endif
}

int CCVQueueDAO::SendData(char* pBuf, int nSize, long lType, int nFlag)
{
	int nResult = -1;
	if(m_pSendQueue)
	{
		nResult= m_pSendQueue->SendMessage(pBuf, RPMSGLEN, lType, nFlag);
#ifdef DEBUG
	printf("SERVER: queue data send at key %d\ndata contents(%d): %s, success:%d\n", m_kSendKey, nSize, pBuf, nResult);
#endif
	}
	else
	{
		return nResult;
	}

	if(nSize > 0)
	{
		CCVClients* pClients = CCVClients::GetInstance();
		assert(pClients);
		vector<shared_ptr<CCVClient> >::iterator iter = pClients->m_vClient.begin();
#ifdef DEBUG
		printf("TCP: queue data read at key %d, size = %d\n", m_kRecvKey, nSize);
#endif
		while(iter != pClients->m_vClient.end())
		{
			CCVClient* pClient = (*iter).get();
			if(pClient->GetStatus() == csOffline && (*iter).unique()) {
				iter++;
				continue;
			}
			if(pClient->SendAll(NULL, pBuf, nSize) != false) {
				pClient->m_pHeartbeat->TriggerGetReplyEvent();
			}
			iter++;
		}
	}
	return nResult;
}

key_t CCVQueueDAO::GetRecvKey()
{
	return m_kRecvKey;
}

key_t CCVQueueDAO::GetSendKey()
{
	return m_kSendKey;
}
