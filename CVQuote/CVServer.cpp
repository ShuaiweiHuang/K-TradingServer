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
#include "CVServerManager.h" 
#include "CVWebClientManager.h"
#include "CVCommon/CVClientSocket.h"
#include "CVGlobal.h"
#include "Include/CVTSFormat.h"
#include "Include/CVTFFormat.h"
#include "Include/CVOFFormat.h"
#include "Include/CVOSFormat.h"

#include<iostream>
using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
                             unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKClient::CSKClient(struct TSKClientAddrInfo &ClientAddrInfo)
{
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
	FprintfStderrLog("HEARTBEAT_LOST", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
	SetStatus(csOffline);
}

void CSKClient::OnHeartbeatRequest()
{
	bool bSendAll = SendAll("HEARTBEAT_REQUEST", g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
	if(bSendAll == false)
	{
		FprintfStderrLog("HEARTBEAT_REQUEST_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
	}
}

void CSKClient::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	FprintfStderrLog(pErrorMessage, -1, nData, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID));
}

bool CSKClient::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	int nRecv = 0;
	int nRecved = 0;

	do
	{
		nToRecv -= nRecv;
		nRecv = recv(m_ClientAddrInfo.nSocket, pBuf + nRecved, nToRecv, 0);

		if(nRecv > 0)
		{
			if(m_pHeartbeat)
				m_pHeartbeat->TriggerGetReplyEvent();
			else
				FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID));

			FprintfStderrLog(pWhat, 0, 0, __FILE__, 0, m_uncaLogonID, sizeof(m_uncaLogonID), pBuf + nRecved, nRecv);
			nRecved += nRecv;
		}
		else if(nRecv == 0)
		{
			SetStatus(csOffline);
			FprintfStderrLog("RECV_SK_CLOSE", 0, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
			break;
		}
		else if(nRecv == -1)
		{
			SetStatus(csOffline);
			FprintfStderrLog("RECV_SK_ERROR", -1, errno, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), (unsigned char*)strerror(errno), strlen(strerror(errno)));
			break;
		}
		else
		{
			SetStatus(csOffline);
			FprintfStderrLog("RECV_SK_ELSE_ERROR", -1, errno, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), (unsigned char*)strerror(errno), strlen(strerror(errno)));
			break;
		}
	}
	while(nRecv != nToRecv);

	return nRecv == nToRecv ? true : false;
}

bool CSKClient::SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend)
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
				//FprintfStderrLog(pWhat, 0, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), (unsigned char*)pBuf + nSended, nSend);
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
	shared_ptr<CSKClient> shpClient{ shared_from_this() };

	pServer->SetCallback(shpClient);
	pServer->SetRequestMessage(pRequestMessage, nRequestMessageLength);
	pServer->m_pRequest->TriggerWakeUpEvent();
}

bool CSKClient::SendRequestReply(unsigned char uncaSecondByte, unsigned char* unpRequestReplyMessage, int nRequestReplyMessageLength)
{
	unsigned char* unpSendBuf = new unsigned char[2 + sizeof(struct SK_TS_REPLY)];
	bool bSendAll = false;

	memset(unpSendBuf, 0, 2 + sizeof(struct SK_TS_REPLY));
	unpSendBuf[0] = ESCAPE_BYTE;
	unpSendBuf[1] = uncaSecondByte;
	memcpy(unpSendBuf + 2, unpRequestReplyMessage, sizeof(struct SK_TS_REPLY));

	bSendAll = SendAll("SEND_REQUEST_REPLY", unpSendBuf, 2 + sizeof(struct SK_TS_REPLY));
	delete [] unpSendBuf;

	return bSendAll;
}

bool CSKClient::SendRequestErrorReply(unsigned char uncaSecondByte, unsigned char* pOriginalRequstMessage, int nOriginalRequestMessageLength, const char* pErrorMessage, short nErrorCode)
{
	int nErrorMessageLength;

	switch(uncaSecondByte)
	{
		case TS_ORDER_BYTE:
			nErrorMessageLength = m_nTSReplyMsgLength;
			break;
		case TF_ORDER_BYTE:
			nErrorMessageLength = m_nTFReplyMsgLength;
			break;
		case OF_ORDER_BYTE:
			nErrorMessageLength = m_nOFReplyMsgLength;
			break;
		case OS_ORDER_BYTE:
			nErrorMessageLength = m_nOSReplyMsgLength;
			break;
		default:
			FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaSecondByte, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), pOriginalRequstMessage, nOriginalRequestMessageLength);
	}

	int nReplyMessageLength = m_nOriginalOrderLength + nErrorMessageLength + ORDER_REPLY_ERROR_CODE_LENGTH;
	unsigned char* unpPlainBuf = new unsigned char[nReplyMessageLength];

	memset(unpPlainBuf, 0, nReplyMessageLength);


	union L l;
	memset(&l,0,16);

	l.value = nErrorCode;

	memcpy(unpPlainBuf, pOriginalRequstMessage, nOriginalRequestMessageLength);
	memcpy(unpPlainBuf + nOriginalRequestMessageLength, pErrorMessage, strlen(pErrorMessage));
	memcpy(unpPlainBuf + nOriginalRequestMessageLength + nErrorMessageLength, l.uncaByte, ORDER_REPLY_ERROR_CODE_LENGTH);
	bool bSendAll = SendAll("SEND_REQUEST_ERROR_REPLY", unpPlainBuf, 2 + nReplyMessageLength);

	delete [] unpPlainBuf;

	return bSendAll;
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
