#include <fstream>
#include <cstring>
#include <algorithm>
#include <assert.h>

#include "CVClients.h"

#include<iostream>
using namespace std;

CCVClients* CCVClients::instance = NULL;
pthread_mutex_t CCVClients::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CCVClients::CCVClients() 
{
	for(int i=0;i<AMOUNT_OF_CLIENT_OBJECT_HASH;i++)
	{
		m_ClientObjectHash[i] = NULL;
	}

	pthread_mutex_init(&m_MutexLockOnSerialNumber, NULL);
	pthread_mutex_init(&m_MutexLockOnOnlineClientVector, NULL);
	pthread_mutex_init(&m_MutexLockOnOfflineClientVector, NULL);
#if 1
	ifstream NodeFile("../ini/CVProxy.ini");

	assert(NodeFile);

	while(NodeFile)//EOF
	{
		char* pIP;

		pIP = new char[IPLEN];
		memset(pIP, 0, IPLEN);

		NodeFile.getline(pIP, IPLEN);

		if(strlen(pIP) != 0)
		{
			m_vProxyNode.push_back(pIP);
		}
	}
#endif
}

CCVClients::~CCVClients()
{
	if(m_pServerSocket)
	{
		m_pServerSocket->ShutdownServer();

		delete m_pServerSocket;
		m_pServerSocket = NULL;
	}

	if(m_pSharedMemoryOfSerialNumber)
	{
		m_pSharedMemoryOfSerialNumber->DetachSharedMemory();
	
		delete m_pSharedMemoryOfSerialNumber;
		m_pSharedMemoryOfSerialNumber = NULL;
	}

	if(m_pSharedMemoryOfKeyNumber)
	{
		m_pSharedMemoryOfKeyNumber->DetachSharedMemory();
	
		delete m_pSharedMemoryOfKeyNumber;
		m_pSharedMemoryOfKeyNumber = NULL;
	}

	for(vector<CCVClient*>::iterator iter = m_vOnlineClient.begin(); iter != m_vOnlineClient.end(); iter++)
	{
		delete *iter;
	}

	for(vector<CCVClient*>::iterator iter = m_vOfflineClient.begin(); iter != m_vOfflineClient.end(); iter++)
	{
		delete *iter;
	}

	pthread_mutex_destroy(&m_MutexLockOnSerialNumber);
	pthread_mutex_destroy(&m_MutexLockOnOnlineClientVector);
	pthread_mutex_destroy(&m_MutexLockOnOfflineClientVector);

	for(vector<char*>::iterator iter = m_vProxyNode.begin(); iter != m_vProxyNode.end(); iter++)
	{
		delete *iter;
	}
}

void* CCVClients::Run()
{
	m_pSharedMemoryOfSerialNumber = new CCVSharedMemory(m_kSerialNumberSharedMemoryKey, sizeof(long));
	m_pSharedMemoryOfSerialNumber->AttachSharedMemory();

	m_pSerialNumber = (long*)m_pSharedMemoryOfSerialNumber->GetSharedMemory();

	while(m_pServerSocket->GetStatus() == sssListening)
	{
		struct TCVClientAddrInfo ClientAddrInfo;
		memset(&ClientAddrInfo, 0, sizeof(struct TCVClientAddrInfo));

		ClientAddrInfo.nSocket = m_pServerSocket->Accept(&ClientAddrInfo.ClientAddr);
		socklen_t addr_size = sizeof(ClientAddrInfo.ClientAddr);

		struct sockaddr_in *sin = (struct sockaddr_in *)&ClientAddrInfo.ClientAddr;
		unsigned char *ip = (unsigned char *)&sin->sin_addr.s_addr;
		sprintf(ClientAddrInfo.caIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);//to do


		CCVClient* pNewClient = NULL;
		if(IsProxy(ClientAddrInfo.caIP))
		{
			pNewClient = new CCVClient(ClientAddrInfo, m_strService, true);
			cout << "Accept! Proxy IP = " << ClientAddrInfo.caIP << endl;
		}
		else
		{
			pNewClient = new CCVClient(ClientAddrInfo, m_strService, false);
			cout << "Accept! Client IP = " << ClientAddrInfo.caIP << endl;
		}
		PushBackClientToOnlineVector(pNewClient);
	}
}

void CCVClients::OnListening()
{
	Start();
}

void CCVClients::OnShutdown()
{
}

CCVClient* CCVClients::GetClientFromHash(long lOrderNumber)
{
	int nHashNumber = lOrderNumber % AMOUNT_OF_CLIENT_OBJECT_HASH;

	return m_ClientObjectHash[nHashNumber];
}

void CCVClients::InsertClientToHash(long lOrderNumber, CCVClient* pClient)
{
	int nHashNumber = lOrderNumber % AMOUNT_OF_CLIENT_OBJECT_HASH;
	
	m_ClientObjectHash[nHashNumber] = pClient;
}

void CCVClients::RemoveClientFromHash(long lOrderNumber)
{
	int nHashNumber = lOrderNumber % AMOUNT_OF_CLIENT_OBJECT_HASH;
	
	m_ClientObjectHash[nHashNumber] = NULL;
}

CCVClients* CCVClients::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CCVClients();
			cout << "Clients One" << endl;
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}
	return instance;
}

void CCVClients::GetSerialNumber(char* pSerialNumber)//to lock
{
	pthread_mutex_lock(&m_MutexLockOnSerialNumber);//lock

	memset(pSerialNumber, 0, 13);

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	sprintf(pSerialNumber, "%13ld", *m_pSerialNumber);

	(*m_pSerialNumber)++;

	pthread_mutex_unlock(&m_MutexLockOnSerialNumber);//unlock
}


void CCVClients::SetConfiguration(string strService, string strListenPort, key_t kSerialNumberSharedMemoryKey)
{
	m_strService = strService;
	m_strListenPort = strListenPort;
	m_kSerialNumberSharedMemoryKey = kSerialNumberSharedMemoryKey;

	m_pServerSocket = new CCVServerSocket(this);
	m_pServerSocket->Listen(m_strListenPort);
}

void CCVClients::CheckOnlineClientVector()
{
	int index = 0;
	while(index < m_vOnlineClient.size())
	{
		if(m_vOnlineClient[index]->GetStatus() == csOffline)
		{
			MoveOnlineClientToOfflineVector(m_vOnlineClient[index]);
		}
		else
		{
			index++;
		}
	}
}

void CCVClients::MoveOnlineClientToOfflineVector(CCVClient* pClient)
{
	pthread_mutex_lock(&m_MutexLockOnOfflineClientVector);//lock
	
	if(find(m_vOfflineClient.begin(), m_vOfflineClient.end(), pClient) != m_vOfflineClient.end())
	{
		//return;
	}
	else
	{
		m_vOfflineClient.push_back(pClient);
	}

	pthread_mutex_unlock(&m_MutexLockOnOfflineClientVector);//unlock


	pthread_mutex_lock(&m_MutexLockOnOnlineClientVector);//lock

	vector<CCVClient*>::iterator iter = find(m_vOnlineClient.begin(), m_vOnlineClient.end(), pClient);
	if(iter != m_vOnlineClient.end())
	{
		m_vOnlineClient.erase(iter);
	}

	pthread_mutex_unlock(&m_MutexLockOnOnlineClientVector);//unlock
}

void CCVClients::PushBackClientToOnlineVector(CCVClient* pClient)
{
	pthread_mutex_lock(&m_MutexLockOnOnlineClientVector);//lock

	m_vOnlineClient.push_back(pClient);

	pthread_mutex_unlock(&m_MutexLockOnOnlineClientVector);//unlock
}

void CCVClients::ClearOfflineClientVector()
{
	pthread_mutex_lock(&m_MutexLockOnOfflineClientVector);//lock

	for(vector<CCVClient*>::iterator iter = m_vOfflineClient.begin(); iter != m_vOfflineClient.end(); iter++)
	{
		
		int nSocket = (*iter)->GetClientSocket();
		ShutdownClient(nSocket);

		delete *iter;
	}
	
	m_vOfflineClient.clear();

	pthread_mutex_unlock(&m_MutexLockOnOfflineClientVector);//unlock
}

void CCVClients::ShutdownClient(int nSocket)
{
	m_pServerSocket->ShutdownClient(nSocket);
}

bool CCVClients::IsProxy(char* pIP)
{
	bool bIsProxy = false;

	for(vector<char*>::iterator iter = m_vProxyNode.begin(); iter != m_vProxyNode.end(); iter++)
	{
		if(strcmp(*iter, pIP) == 0)
		{
			bIsProxy = true;
			break;
		}
	}

	return bIsProxy;
}

#ifdef MNTRMSG
void CCVClients::FlushLogMessageToFile()
{
	int index = 0;

	while(index < m_vOnlineClient.size())
	{

		while(m_vOnlineClient[index]->m_LogMessageQueue.order_index)
		{
			m_fileOrder << m_vOnlineClient[index]->m_LogMessageQueue.order_msg_len[m_vOnlineClient[index]->m_LogMessageQueue.order_index-1] <<  ","
			            << m_vOnlineClient[index]->m_LogMessageQueue.order_msg_buf[m_vOnlineClient[index]->m_LogMessageQueue.order_index-1] << endl;
			m_vOnlineClient[index]->m_LogMessageQueue.order_index--;
		}

		while(m_vOnlineClient[index]->m_LogMessageQueue.reply_index)
		{
			m_fileReply << m_vOnlineClient[index]->m_LogMessageQueue.reply_msg_len[m_vOnlineClient[index]->m_LogMessageQueue.reply_index-1] <<  ","
			            << m_vOnlineClient[index]->m_LogMessageQueue.reply_msg_buf[m_vOnlineClient[index]->m_LogMessageQueue.reply_index-1] << endl;
			m_vOnlineClient[index]->m_LogMessageQueue.reply_index--;
		}
		index++;
	}
}
#endif
