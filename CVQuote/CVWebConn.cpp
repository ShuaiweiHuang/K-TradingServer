#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>

#include "CVWebConns.h"
#include "CVWebConn.h"
#include "CVGlobal.h"
#include "Include/CVTSFormat.h"
#include "CVQueueNodes.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0, 
			     unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVServer::CCVServer(string strWeb, string strQstr, string strName, TCVRequestMarket rmRequestMarket)
{
	m_shpClient = NULL;
	m_pHeartbeat = NULL;
	m_FTX_enable = false;
	m_strHost = strWeb;
	m_strPara = strQstr;
	m_strName = strName;
	m_ssServerStatus = ssNone;

	m_rmRequestMarket = rmRequestMarket;
	pthread_mutex_init(&m_pmtxServerStatusLock, NULL);

	try
	{
		m_pClientSocket = new CCVClientSocket(this);
		m_pClientSocket->Connect( m_strHost, m_strPara, m_strName, CONNECT_TCP);//start
	}
	catch (exception& e)
	{
		FprintfStderrLog("SERVER_NEW_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)e.what(), strlen(e.what()));
	}
}

CCVServer::~CCVServer() 
{
	m_shpClient = NULL;
	if(m_pClientSocket)
	{
		SetStatus(ssBreakdown);
		delete m_pClientSocket;
		m_pClientSocket = NULL;
	}

	if(m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	pthread_mutex_destroy(&m_pmtxServerStatusLock);
}

void* CCVServer::Run()
{
	sprintf(m_caPthread_ID, "%020lu", Self());
	char uncaRecvBuf[BUFFERSIZE];

	try
	{
		m_pHeartbeat = new CCVHeartbeat(this);
		m_pHeartbeat->SetTimeInterval(HEARTBEAT_TIME_INTERVAL);
		//printf("Set heartbeat timer: %d\n", HEARTBEAT_TIME_INTERVAL);
		m_pHeartbeat->Start();
		m_pRequest = new CCVRequest(this);
	}
	catch (exception& e)
	{
		FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, 
				(unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)e.what(), strlen(e.what()));
		return NULL;
	}

	CCVServers* pServers = NULL;

	try
	{
		pServers = CCVServers::GetInstance();
		if(pServers == NULL)
			throw "GET_SERVERS_ERROR";
	}
	catch (const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
		return NULL;
	}

	SetStatus(ssFree);

	while(IsTerminated())
	{
		if(m_ssServerStatus == ssFree || m_ssServerStatus == ssInuse)
		{
			memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
			bool bRecvAll;
			int char_index = 0;

			pthread_mutex_lock(&m_pmtxServerStatusLock);
			while( char_index < sizeof(uncaRecvBuf))
			{
				bRecvAll = RecvAll("RECV_DATA", (unsigned char*)(uncaRecvBuf + char_index), 1);

				if(bRecvAll == false || uncaRecvBuf[char_index] == '\n')
				{
					break;
				}
				else
				{
					if(m_pHeartbeat)
					{
						m_pHeartbeat->TriggerGetReplyEvent();//reset heartbeat
					}
					else
					{
						FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
					}
				}
				char_index++;
			}
			pthread_mutex_unlock(&m_pmtxServerStatusLock);

			if(bRecvAll == false)
			{
				SetStatus(ssBreakdown);
				while(1) {sleep(1);}//wait for CVServers restart
			}
			//printf("[%s] %s\n", m_strName.c_str(), uncaRecvBuf);
			CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();
			assert(pQueueDAO);
			pQueueDAO->SendData(uncaRecvBuf, strlen(uncaRecvBuf));
		}
	}
	FprintfStderrLog("THREAD_EXIT", -1, 0, m_strName.c_str(), __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
}

void CCVServer::OnConnect()
{
	if(m_ssServerStatus == ssNone)
	{
		Start();
	}
	else if(m_ssServerStatus == ssReconnecting)
	{
		SetStatus(ssFree);
		FprintfStderrLog("RECONNECT_SUCCESS", 0, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));

		if(m_pHeartbeat)
		{
			m_pHeartbeat->TriggerGetReplyEvent();//reset heartbeat
		}
		else
		{
			FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
		}
	}
	else
	{
		FprintfStderrLog("SERVER_STATUS_ERROR", -1, m_ssServerStatus, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
	}
}

void CCVServer::OnDisconnect()
{
	FprintfStderrLog("RECONNECT_HOST", -1, 0, m_strName.c_str(), __LINE__, 0, 0);
	SetStatus(ssBreakdown);
}


void CCVServer::OnHeartbeatLost()
{
	FprintfStderrLog("QUOTE HEARTBEAT LOST", -1, 0, m_strName.c_str(), 0,  NULL, 0);
#ifdef EXIT_VERSION
	exit(-1);
#endif
	SetStatus(ssBreakdown);
}

void CCVServer::OnHeartbeatRequest()
{
}

void CCVServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	FprintfStderrLog("HEARTBEAT ERROR", -1, 0, m_strName.c_str(), m_strName.length(),  NULL, 0);
#ifdef EXIT_VERSION
	exit(-1);
#endif
	SetStatus(ssBreakdown);
}

bool CCVServer::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	int nRecv = 0;
	int nRecved = 0;

	do
	{
		nToRecv -= nRecv;
		if(m_pClientSocket)
			nRecv = m_pClientSocket->Recv(pBuf + nRecved, nToRecv);
		else
		{
			FprintfStderrLog("SERVER_SOCKET_NULL_ERROR", -1, 0, m_strName.c_str(), __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
			break;
		}

		if(nRecv > 0)
		{
			nRecved += nRecv;
		}
		else if(nRecv == 0)//connection closed
		{
			FprintfStderrLog("Quote:SERVER_SOCKET_CLOSE", -1, 0, m_strName.c_str(), 0);
			break;
		}
		else if(nRecv == -1)
		{
			FprintfStderrLog("SERVER_SOCKET_ERROR", -1, errno, m_strName.c_str(), 0 );
			break;
		}
		else
		{
			FprintfStderrLog("SERVER_SOCKET_RECV_ELSE_ERROR", -1, errno, m_strName.c_str(), 0);
			break;
		}
	}
	while(nRecv != nToRecv);
	return (nRecv == nToRecv) ? true : false;
}

bool CCVServer::SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend)
{
	return false;
}

void CCVServer::ReconnectSocket()
{
	SetStatus(ssReconnecting);

	if(m_pClientSocket)
		delete m_pClientSocket;

	m_pClientSocket = new CCVClientSocket(this);
	m_pClientSocket->Connect( m_strHost, m_strPara, m_strName, CONNECT_TCP);//start
	SetStatus(ssNone);
}

void CCVServer::SetCallback(shared_ptr<CCVClient>& shpClient)
{
	m_shpClient = shpClient;
}

void CCVServer::SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength)
{
}

void CCVServer::SetStatus(TCVServerStatus ssServerStatus)
{
	pthread_mutex_lock(&m_pmtxServerStatusLock);

	m_ssServerStatus = ssServerStatus;

	pthread_mutex_unlock(&m_pmtxServerStatusLock);
}

TCVServerStatus CCVServer::GetStatus()
{
	return m_ssServerStatus;
}

context_ptr CCVServer::CB_TLS_Init(const char * hostname, websocketpp::connection_hdl) {
	context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12 );
	return ctx;
}

void CCVServer::OnRequest()
{
}

void CCVServer::OnRequestError(int, const char*)
{
}

void CCVServer::OnData(unsigned char*, int)
{
}
