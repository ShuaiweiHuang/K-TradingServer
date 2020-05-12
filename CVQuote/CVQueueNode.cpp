#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sys/time.h>
#include <assert.h>
#include <algorithm>

#include "CVQueueNode.h"
#include "CVServer.h"

#define QUEUELOCALSIZE 100

using namespace std;

vector<string> QueueLocal;
CCVQueueDAO::CCVQueueDAO(string strService, key_t kSendKey, key_t kRecvKey)
{
	m_strService = strService;

	m_kSendKey = kSendKey;
	m_kRecvKey = kRecvKey;

	m_pSendQueue = new CCVQueue;
	m_pSendQueue->Create(m_kSendKey);

	m_pRecvQueue = new CCVQueue;
	m_pRecvQueue->Create(m_kRecvKey);

	Start();
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
	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	char uncaRecvBuf[BUFSIZE];

	while(m_pRecvQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		usleep(500);
		int nGetMessage = m_pRecvQueue->GetMessage(uncaRecvBuf);
#ifdef DEBUG
		printf("SERVER: queue data read at key %d\n", m_kRecvKey);
#endif
		if(nGetMessage > 0)
		{
			//if(find(QueueLocal.begin(), QueueLocal.end(), uncaRecvBuf) == QueueLocal.end())
			{
				//QueueLocal.push_back(uncaRecvBuf);
				//printf("%s\n", uncaRecvBuf);
				vector<shared_ptr<CCVClient> >::iterator iter = pClients->m_vClient.begin();
				while(iter != pClients->m_vClient.end())
				{
					CCVClient* pClient = (*iter).get();
					if(pClient->GetStatus() == csOffline && (*iter).unique()) {
						iter++;
						continue;
					}
					if(pClient->SendAll(NULL, uncaRecvBuf, strlen(uncaRecvBuf)) != false) {
						pClient->m_pHeartbeat->TriggerGetReplyEvent();
					}
					iter++;
				}
			}
			else
			{
				//while(QueueLocal.size() > QUEUELOCALSIZE)
				{
					//QueueLocal.erase(QueueLocal.begin());
				}
			}
		}
		if(nGetMessage < 0)
			exit(-1);
	}
	return NULL;
}

int CCVQueueDAO::SendData(char* pBuf, int nSize, long lType, int nFlag)
{
	int nResult = -1;
#ifdef DEBUG
	printf("SERVER: queue data send at key %d\ndata contents: %s\n", m_kSendKey, pBuf);
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

key_t CCVQueueDAO::GetRecvKey()
{
	return m_kRecvKey;
}

key_t CCVQueueDAO::GetSendKey()
{
	return m_kSendKey;
}
