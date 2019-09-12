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

CCVClient::CCVClient(struct TCVClientAddrInfo &ClientAddrInfo)
{
	signal(SIGPIPE, SIG_IGN);
	memset(&m_ClientAddrInfo, 0, sizeof(struct TCVClientAddrInfo));
	memcpy(&m_ClientAddrInfo, &ClientAddrInfo, sizeof(struct TCVClientAddrInfo));
	m_pHeartbeat = NULL;
	m_csClientStatus = csNone;
	pthread_mutex_init(&m_pmtxClientStatusLock, NULL);
	srand(time(NULL));
	CCVClients* pClients = NULL;

	try
	{
		pClients = CCVClients::GetInstance();

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
		m_pHeartbeat = new CCVHeartbeat(this);
		m_pHeartbeat->SetTimeInterval( stoi(pClients->m_strHeartBeatTime) );
	}
	catch(exception& e)
	{
		FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}


	Start();
}

CCVClient::~CCVClient() 
{
	if( m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	pthread_mutex_destroy(&m_pmtxClientStatusLock);
}


void* CCVClient::Run()
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
	return NULL;
}

void CCVClient::OnHeartbeatLost()
{
	//no need to implement, since GTA doesn't reply any message.
}

void CCVClient::OnHeartbeatRequest()
{
	char caHeartbeatRequestBuf[128];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	memset(caHeartbeatRequestBuf, 0, 128);

	sprintf(caHeartbeatRequestBuf, "03_ServerDate=%d%02d%02d,ServerTime=%02d%02d%02d00,EPID=%s,\r\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, m_strEPID.c_str());

	bool bSendAll = SendAll("HEARTBEAT_REQUEST", caHeartbeatRequestBuf, sizeof(caHeartbeatRequestBuf));

	if(bSendAll == false)
	{
		FprintfStderrLog("HEARTBEAT_REQUEST_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), g_uncaHeartbeatRequestBuf, sizeof(g_uncaHeartbeatRequestBuf));
	}
	m_pHeartbeat->TriggerGetReplyEvent();
}

void CCVClient::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	exit(-1);
}

bool CCVClient::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	return false;
}

bool CCVClient::SendAll(const char* pWhat, char* pBuf, int nToSend)
{
	int nSend = 0;
	int nSended = 0;
	int retry = 3;

	if(nToSend == 0)
		return true;
	do
	{
		nToSend -= nSend;

		do {
			nSend = send(m_ClientAddrInfo.nSocket, pBuf + nSended, nToSend, 0);

			if(nSend > 0) {
				retry = 3;
				break;
			}
			else {
				FprintfStderrLog("SEND_CV_RETRY", -1, errno, NULL, 0, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), strlen(m_ClientAddrInfo.caIP));
				sleep(1);
			}

		} while(retry--);

		if(nSend <= 0) // socket close or disconnect
		{
			SetStatus(csOffline);
			FprintfStderrLog("SEND_CV_ERROR", -1, errno, NULL, 0, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), strlen(m_ClientAddrInfo.caIP), (unsigned char*)strerror(errno), strlen(strerror(errno)));
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
				FprintfStderrLog("SEND_CV_ELSE_ERROR", -1, errno, NULL, 0, (unsigned char*)pBuf, nToSend, (unsigned char*)strerror(errno), strlen(strerror(errno)));
				break;
			}
		}
	} while(nSend != nToSend);

	return nSend == nToSend ? true : false;
}

void CCVClient::TriggerSendRequestEvent(CCVServer* pServer, unsigned char* pRequestMessage, int nRequestMessageLength)
{
}

bool CCVClient::SendRequestReply(unsigned char uncaSecondByte, unsigned char* unpRequestReplyMessage, int nRequestReplyMessageLength)
{
}

bool CCVClient::SendRequestErrorReply(unsigned char uncaSecondByte, unsigned char* pOriginalRequstMessage, int nOriginalRequestMessageLength, const char* pErrorMessage, short nErrorCode)
{
}

void CCVClient::SetStatus(TCVClientStauts csStatus)
{
	pthread_mutex_lock(&m_pmtxClientStatusLock);//lock

	m_csClientStatus = csStatus;

	pthread_mutex_unlock(&m_pmtxClientStatusLock);//unlock
}

TCVClientStauts CCVClient::GetStatus()
{
	return m_csClientStatus;
}

int CCVClient::GetClientSocket()
{
	return m_ClientAddrInfo.nSocket;
}
