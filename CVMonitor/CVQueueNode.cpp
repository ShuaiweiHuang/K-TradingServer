#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sys/time.h>
#include <assert.h>
#include <curl/curl.h>

#include "CVQueueNode.h"
#include "CVServer.h"

using namespace std;

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

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
}

int CCVQueueDAO::Webapi_Write(char* buffer, int)
{
	CURLcode res;
	CURL *curl = curl_easy_init();
	char query_str[1024];
	string monitor_reply;

	if(!curl)
	{
		return 0;
	}
	sprintf(query_str, "https://intra.cryptovix.com.tw/receive/tm?data=%s", buffer);
	printf("\n%s\n", query_str);
	curl_easy_setopt(curl, CURLOPT_URL, query_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &monitor_reply);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	res = curl_easy_perform(curl);

	if(res != CURLE_OK) {
		fprintf(stderr, "Monitor ssl curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	}
	else
		printf("%s\n", monitor_reply.c_str());
#if 0
	sprintf(query_str, "http://intra.cryptovix.com.tw:3001/receive/tm?data=%s", buffer);
	printf("\n%s\n", query_str);
	curl_easy_setopt(curl, CURLOPT_URL, query_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &monitor_reply);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	res = curl_easy_perform(curl);

	if(res != CURLE_OK) {
		fprintf(stderr, "Monitor curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	}
	else
		printf("%s\n", monitor_reply.c_str());
#endif
	curl_easy_cleanup(curl);
}

void* CCVQueueDAO::Run()
{
	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	char uncaRecvBuf[BUFSIZE];
	while(m_pRecvQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		usleep(2000);
		int nGetMessage = m_pRecvQueue->GetMessage(uncaRecvBuf);
#ifdef DEBUG
		printf("SERVER: queue data read at key %d\n", m_kRecvKey);
#endif
		if(nGetMessage > 0)
		{
			vector<shared_ptr<CCVClient> >::iterator iter = pClients->m_vClient.begin();
			while(iter != pClients->m_vClient.end())
			{
				CCVClient* pClient = (*iter).get();
				if(pClient->GetStatus() == csOffline && (*iter).unique()) {
					iter++;
					continue;
				}
				if(pClient->SendAll(NULL, uncaRecvBuf, strlen(uncaRecvBuf)) != false) {
					char newline = '\n'; 
					pClient->SendAll(NULL, &newline, 1);
					pClient->m_pHeartbeat->TriggerGetReplyEvent();
				}
				iter++;
			}
			Webapi_Write(uncaRecvBuf, strlen(uncaRecvBuf));
		}
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
