#include <fstream>
#include <cstring>
#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <sys/msg.h>

#include "CVCommon/CVServerSocket.h"
#include "CVWebConn.h"
#include "CVWebConns.h"
#include "CVServiceHandler.h"
#include "CVServer.h"


using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
                             unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVClients* CCVClients::instance = NULL;
pthread_mutex_t CCVClients::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CCVClients::CCVClients()
{
	pthread_mutex_init(&m_pmtxClientVectorLock, NULL);
}

CCVClients::~CCVClients()
{
	if(m_pServerSocket)
	{
		m_pServerSocket->ShutdownServer();

		delete m_pServerSocket;

		m_pServerSocket = NULL;
	}

	m_vClient.clear();//to check

	pthread_mutex_destroy(&m_pmtxClientVectorLock);
}

void* CCVClients::Run()
{
	while(m_pServerSocket->GetStatus() == sssListening)
	{
		struct TCVClientAddrInfo ClientAddrInfo;
		memset(&ClientAddrInfo, 0, sizeof(struct TCVClientAddrInfo));

		ClientAddrInfo.nSocket = m_pServerSocket->Accept(&ClientAddrInfo.ClientAddr);
		socklen_t addr_size = sizeof(ClientAddrInfo.ClientAddr);

		struct sockaddr_in *sin = (struct sockaddr_in *)&ClientAddrInfo.ClientAddr;
		unsigned char *ip = (unsigned char *)&sin->sin_addr.s_addr;
		sprintf(ClientAddrInfo.caIP,"%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);//to do

		FprintfStderrLog("Reply:ACCEPT_CLIENT_IP", 0, 0, NULL, 0, reinterpret_cast<unsigned char*>(ClientAddrInfo.caIP), strlen(ClientAddrInfo.caIP));
		shared_ptr<CCVClient> shpClient = make_shared<CCVClient>(ClientAddrInfo);
		PushBackClientToVector(shpClient);
	}

	return NULL;
}

void CCVClients::OnListening()
{
	//Start();
}

void CCVClients::OnShutdown()
{
}

CCVClients* CCVClients::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CCVClients();
			FprintfStderrLog("CLIENTS_ONE", 0, 0);
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

void CCVClients::SetConfiguration(string& strListenPort, string& strHeartBeatTime, string& strEPIDNum, int& nService)
{
	m_strListenPort = strListenPort;
	m_strHeartBeatTime = strHeartBeatTime;
	m_strEPIDNum = strEPIDNum;
	m_nService = nService;

	try
	{
		m_pServerSocket = new CCVServerSocket(this);
		m_pServerSocket->Listen(m_strListenPort);
	}
	catch (exception& e)
	{
		FprintfStderrLog("NEW_SERVERSOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)e.what(), strlen(e.what()));
	}
}


void CCVClients::CheckClientVector()
{
	vector<shared_ptr<CCVClient> >::iterator iter = m_vClient.begin();
	while(iter != m_vClient.end())
	{
		CCVClient* pClient = (*iter).get();
		if(pClient->GetStatus() == csOffline && (*iter).unique())
		{
			ShutdownClient(pClient->GetClientSocket());
			EraseClientFromVector(iter);
		}
		else
		{
			iter++;
		}
	}
}

void CCVClients::PushBackClientToVector(shared_ptr<CCVClient>& shpClient)
{
	pthread_mutex_lock(&m_pmtxClientVectorLock);

	m_vClient.push_back(move(shpClient));

	pthread_mutex_unlock(&m_pmtxClientVectorLock);
}

void CCVClients::EraseClientFromVector(vector<shared_ptr<CCVClient> >::iterator iter)
{
	pthread_mutex_lock(&m_pmtxClientVectorLock);

	m_vClient.erase(iter);

	pthread_mutex_unlock(&m_pmtxClientVectorLock);
}

void CCVClients::ShutdownClient(int nSocket)
{
	if(m_pServerSocket)
		m_pServerSocket->ShutdownClient(nSocket);
}

bool CCVClients::IsServiceRunning(enum TCVRequestMarket& rmRequestMarket)
{
	return (m_nService & 1<<rmRequestMarket) ? true : false;
}
