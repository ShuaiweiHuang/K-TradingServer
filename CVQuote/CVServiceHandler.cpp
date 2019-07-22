#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <assert.h>
#include <exception>
#include <unistd.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <fstream>
#include <stdlib.h>

#include "CVServer.h" 
#include "CVServiceHandler.h" 
#include "CVWebConns.h"
#include "CVCommon/CVClientSocket.h"
#include "CVGlobal.h"
#include<iostream>

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
			     unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKClient::CSKClient(struct TSKClientAddrInfo &ClientAddrInfo)
{
	signal(SIGPIPE, SIG_IGN);
	memset(&m_ClientAddrInfo, 0, sizeof(struct TSKClientAddrInfo));
	memcpy(&m_ClientAddrInfo, &ClientAddrInfo, sizeof(struct TSKClientAddrInfo));
	m_pHeartbeat = NULL;
	m_csClientStatus = csNone;
	pthread_mutex_init(&m_pmtxClientStatusLock, NULL);
	srand(time(NULL));
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
	}

	m_strEPID = pClients->m_strEPIDNum;
       try
	{
		m_pHeartbeat = new CSKHeartbeat(this);
		m_pHeartbeat->SetTimeInterval( stoi(pClients->m_strHeartBeatTime) );
	}
	catch(exception& e)
	{
		FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}


	Start();
}

CSKClient::~CSKClient() 
{
	if( m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	pthread_mutex_destroy(&m_pmtxClientStatusLock);
}


void* CSKClient::Run()
{
	unsigned char uncaRecvBuf[BUFFERSIZE];

	while(m_csClientStatus != csOffline)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		int bRecvAll = RecvAll(NULL, uncaRecvBuf, BUFFERSIZE);

		if(bRecvAll) {
			printf("Client received string %s\n", uncaRecvBuf);
		}
		sleep(1);
	}
	if(m_pHeartbeat)
		m_pHeartbeat->TriggerTerminateEvent();

	return NULL;

}

void CSKClient::OnHeartbeatLost()
{

}

void CSKClient::OnHeartbeatRequest()
{
	char caHeaetbeatRequestBuf[128];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	memset(caHeaetbeatRequestBuf, 0, 128);

	sprintf(caHeaetbeatRequestBuf, "03_ServerDate=%d%02d%02d,ServerTime=%02d%02d%02d00,EPID=%s,\r\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, m_strEPID.c_str());

	bool bSendAll = SendAll("HEARTBEAT_REQUEST", caHeaetbeatRequestBuf, sizeof(caHeaetbeatRequestBuf));

	if(bSendAll == false)
	{
		FprintfStderrLog("HEARTBEAT_REQUEST_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
	}
}

void CSKClient::OnHeartbeatError(int nData, const char* pErrorMessage)
{
}

bool CSKClient::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	return false;
}

bool CSKClient::SendAll(const char* pWhat, char* pBuf, int nToSend)
{
	int nSend = 0;
	int nSended = 0;

	do
	{
		nToSend -= nSend;

		nSend = send(m_ClientAddrInfo.nSocket, pBuf + nSended, nToSend, 0);
		if(nSend <= 0) // socket close or disconnect
		{
			SetStatus(csOffline);
			FprintfStderrLog("SEND_SK_ERROR", -1, errno, NULL, 0, (unsigned char*)pBuf, nToSend, (unsigned char*)strerror(errno), strlen(strerror(errno)));
			break;
		}
		else
		{
			if(nSend == nToSend)
			{
#ifdef DEBUG
				FprintfStderrLog(pWhat, 0, 0, __FILE__, __LINE__, (unsigned char*)pBuf + nSended, nSend);
#endif
				break;
			}
			else if(nSend < nToSend)
			{
				FprintfStderrLog(pWhat, -1, 0, __FILE__, __LINE__, (unsigned char*)pBuf + nSended, nSend);
				nSended += nSend;
			}
			else
			{
				FprintfStderrLog("SEND_SK_ELSE_ERROR", -1, errno, NULL, 0, (unsigned char*)pBuf, nToSend, (unsigned char*)strerror(errno), strlen(strerror(errno)));
				break;
			}
		}
	}
	while(nSend != nToSend);

	return nSend == nToSend ? true : false;
}

void CSKClient::TriggerSendRequestEvent(CSKServer* pServer, unsigned char* pRequestMessage, int nRequestMessageLength)
{
}

bool CSKClient::SendRequestReply(unsigned char uncaSecondByte, unsigned char* unpRequestReplyMessage, int nRequestReplyMessageLength)
{
}

bool CSKClient::SendRequestErrorReply(unsigned char uncaSecondByte, unsigned char* pOriginalRequstMessage, int nOriginalRequestMessageLength, const char* pErrorMessage, short nErrorCode)
{
}

void CSKClient::SetStatus(TSKClientStauts csStatus)
{
	pthread_mutex_lock(&m_pmtxClientStatusLock);//lock

	m_csClientStatus = csStatus;

	pthread_mutex_unlock(&m_pmtxClientStatusLock);//unlock
}

TSKClientStauts CSKClient::GetStatus()
{
	return m_csClientStatus;
}

int CSKClient::GetClientSocket()
{
	return m_ClientAddrInfo.nSocket;
}
