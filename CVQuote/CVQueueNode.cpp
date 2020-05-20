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
#define PRICE_POS	3
#define VOLUME_POS	4
#define TC_POS		5
#define EPOCH_POS	8
#define END_POS		9

#define QUEUELOCALSIZE	1000

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

	char uncaRecvBufToken[BUFSIZE];
	char uncaRecvBuf[BUFSIZE];

	while(m_pRecvQueue)
	{
		memset(uncaRecvBufToken, 0, sizeof(uncaRecvBufToken));
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		usleep(500);
		int nGetMessage = m_pRecvQueue->GetMessage(uncaRecvBufToken);
#ifdef DEBUG
		printf("SERVER: queue data read at key %d\n", m_kRecvKey);
#endif
		string hash_string;
		string out_string;
		if(nGetMessage > 0)
		{
			strcpy(uncaRecvBuf, uncaRecvBufToken);

			char *token = strtok(uncaRecvBufToken, ",");
			out_string += token;
			out_string += ",";
			int GTA_index = 0;
			static int tick_count = 0;
			while(token = strtok(NULL, ","))
			{
				GTA_index++;
				if(GTA_index == PRICE_POS || GTA_index == EPOCH_POS || GTA_index == VOLUME_POS)
				{
					hash_string += token;
				}
				if(GTA_index == TC_POS)
				{
					out_string += "TC=";
					out_string += to_string(tick_count);
					tick_count++;
				}
				else
					out_string += token;
				if(GTA_index != END_POS)
					out_string += ",";
			}
			
			if(find(QueueLocal.begin(), QueueLocal.end(), hash_string.c_str()) == QueueLocal.end())
			{
				//printf("%s\n", uncaRecvBuf);
				//printf("%s\n", hash_string.c_str());
				//printf("%s\n", out_string.c_str());
				QueueLocal.push_back(hash_string.c_str());
				vector<shared_ptr<CCVClient> >::iterator iter = pClients->m_vClient.begin();
				while(iter != pClients->m_vClient.end())
				{
					CCVClient* pClient = (*iter).get();
					if(pClient->GetStatus() == csOffline && (*iter).unique()) {
						iter++;
						continue;
					}
					if(pClient->SendAll(NULL, (char*)out_string.c_str(), out_string.length()) != false) {
					//if(pClient->SendAll(NULL, uncaRecvBuf, strlen(uncaRecvBuf)) != false) {
						pClient->m_pHeartbeat->TriggerGetReplyEvent();
					}
					iter++;
				}
			}
			else
			{
				//printf("2. %s\n", hash_string.c_str());
				while(QueueLocal.size() > (QUEUELOCALSIZE / 4 * 3))
				{
					QueueLocal.erase(QueueLocal.begin(), QueueLocal.begin()+250);
				}
				//printf("%d\n", QueueLocal.size());
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
