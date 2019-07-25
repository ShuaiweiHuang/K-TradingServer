#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>

#include "CVMCService.h"
#include "CVMCServices.h"
#include "CVGlobal.h"
#include "CVQueueNodes.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0, 
			     unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKServer::CSKServer(string strHost, string strPort, string strName, TSKRequestMarket rmRequestMarket)
{
	m_shpClient = NULL;
	m_pHeartbeat = NULL;
	m_pRequest= NULL;

	m_strHost = strHost;
	m_strPort = strPort;
	m_strName = strName;

	m_ssServerStatus = ssNone;

	m_rmRequestMarket = rmRequestMarket;
	pthread_mutex_init(&m_pmtxServerStatusLock, NULL);

	try
	{
		m_pClientSocket = new CSKClientSocket(this);
		m_pClientSocket->Connect( m_strHost, m_strPort, m_strName, CONNECT_WEBSOCK);//start
	}
	catch (exception& e)
	{
		FprintfStderrLog("SERVER_NEW_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)e.what(), strlen(e.what()));
	}
}

CSKServer::~CSKServer() 
{
	m_shpClient = NULL;

	if(m_pClientSocket)
	{
		m_pClientSocket->Disconnect();

		delete m_pClientSocket;
		m_pClientSocket = NULL;
	}

	if(m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	if(m_pRequest)
	{
		delete m_pRequest;
		m_pRequest = NULL;
	}
	pthread_mutex_destroy(&m_pmtxServerStatusLock);
}

void* CSKServer::Run()
{
	sprintf(m_caPthread_ID, "%020lu", Self());

        CSKClients* pClients = NULL;
        try
        {
                pClients = CSKClients::GetInstance();

                if(pClients == NULL)
                        throw "GET_CLIENTS_ERROR";
        }
        catch(const char* pErrorMessage)
        {
                FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
                return NULL;
        }
        try
        {
                m_pHeartbeat = new CSKHeartbeat(this);
                m_pHeartbeat->SetTimeInterval(HEARTBEAT_INTERVAL_WEB);
                m_pHeartbeat->Start();
		m_heartbeat_count = 0;

        }
        catch (exception& e)
        {
                FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)e.what(), strlen(e.what()));
                return NULL;
        }
        try
        {
		m_pClientSocket->m_cfd.run();
		printf("Websocket disconnect: %s.\n", m_strName.c_str());
		exit(-1);
	}
        catch (exception& e)
        {
                return NULL;
        }


 	return NULL;
}

void CSKServer::OnConnect()
{

	if(m_ssServerStatus == ssNone)
	{
		Start();
	}
	else if(m_ssServerStatus == ssReconnecting)
	{
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

void CSKServer::OnDisconnect()
{
	sleep(5);

	m_pClientSocket->Connect( m_strHost, m_strPort, m_strName, CONNECT_WEBSOCK);//start & reset heartbeat
}

void CSKServer::OnHeartbeatLost()
{
}

void CSKServer::OnHeartbeatRequest()
{
}

void CSKServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
}

void CSKServer::OnData(unsigned char* pBuf, int nSize)
{
}

void CSKServer::OnRequest()
{
}

void CSKServer::OnRequestError(int nData, const char* pErrorMessage)
{
}

bool CSKServer::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	return false;
}

bool CSKServer::SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend)
{
	return false;
}

void CSKServer::ReconnectSocket()
{
	sleep(5);

	if(m_pClientSocket)
	{
		m_pClientSocket->Disconnect();

		m_pClientSocket->Connect( m_strHost, m_strPort, m_strName, CONNECT_TCP);//start & reset heartbeat
	}
}

void CSKServer::SetCallback(shared_ptr<CSKClient>& shpClient)
{
	m_shpClient = shpClient;
}

void CSKServer::SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength)
{
}

void CSKServer::SetStatus(TSKServerStatus ssServerStatus)
{
	pthread_mutex_lock(&m_pmtxServerStatusLock);

	m_ssServerStatus = ssServerStatus;

	pthread_mutex_unlock(&m_pmtxServerStatusLock);
}

TSKServerStatus CSKServer::GetStatus()
{
	return m_ssServerStatus;
}

context_ptr CSKServer::CB_TLS_Init(const char * hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    return ctx;
}

