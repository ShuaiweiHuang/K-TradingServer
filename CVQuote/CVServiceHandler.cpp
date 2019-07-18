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
	//Start();
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
}

void CSKClient::OnHeartbeatLost()
{
}

void CSKClient::OnHeartbeatRequest()
{
#if 0
	bool bSendAll = SendAll("HEARTBEAT_REQUEST", g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
	if(bSendAll == false)
	{
		FprintfStderrLog("HEARTBEAT_REQUEST_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
	}
#endif
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
