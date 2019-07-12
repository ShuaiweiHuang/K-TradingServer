#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>

#include "CVWebClients.h"
#include "CVWebClient.h"
#include "CVGlobal.h"
#include "Include/CVTSFormat.h"


#ifdef MNTRMSG
extern struct MNTRMSGS g_MNTRMSG;
#endif

#ifdef TIMETEST
extern void InsertTimeLogToSharedMemory(struct timeval *timeval_Start, struct timeval *timeval_End, enum TSKTimePoint tpTimePoint, long lOrderNumber);
#endif

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0, 
                             unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKServer::CSKServer(string strWeb, string strQstr, TSKRequestMarket rmRequestMarket, int nPoolIndex)
{
	m_shpClient = NULL;
	m_pHeartbeat = NULL;
	m_pRequest= NULL;

	m_strWeb = strWeb;
	m_strQstr = strQstr;

	m_ssServerStatus = ssNone;

	m_rmRequestMarket = rmRequestMarket;
	pthread_mutex_init(&m_pmtxServerStatusLock, NULL);

	switch(m_rmRequestMarket)
	{
		case rmBitmex:
			m_nReplyMessageLength = sizeof(struct SK_TS_REPLY);
			m_uncaSecondByte = TS_ORDER_BYTE;

			m_nOriginalOrderLength = sizeof(struct SK_TS_ORDER);

			struct SK_TS_REPLY sk_ts_reply;
			m_nReplyMsgLength = sizeof(sk_ts_reply.reply_msg);
			break;
		default:
			FprintfStderrLog("REQUEST_MARKET_ERROR", -1, m_rmRequestMarket, __FILE__, __LINE__);
			break;
	}

	m_nPoolIndex = nPoolIndex;

	try
	{
		m_pClientSocket = new CSKClientSocket(this);
		m_pClientSocket->Connect( m_strWeb, m_strQstr, CONNECT_WEBSOCK);//start
	}
	catch (exception& e)
	{
		FprintfStderrLog("SERVER_NEW_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)e.what(), strlen(e.what()));
	}
	printf("keanu server init\n");
	Start();
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
	m_pClientSocket->m_cfd.run();

	try
	{
		m_pHeartbeat = new CSKHeartbeat(this);
		m_pHeartbeat->SetTimeInterval(HEARTBEAT_TIME_INTERVAL);
		m_pHeartbeat->Start();

		m_pRequest = new CSKRequest(this);
	}
	catch (exception& e)
	{
		FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)e.what(), strlen(e.what()));
		return NULL;
	}

	CSKServers* pServers = NULL;
	try
	{
		pServers = CSKServers::GetInstance();

		if(pServers == NULL)
			throw "GET_SERVERS_ERROR";
	}
	catch (const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
		return NULL;
	}

	unsigned char uncaRecvBuf[1024];
	unsigned char uncaEscapeBuf[2];

	SetStatus(ssFree);

	while(IsTerminated())
	{
		if(m_ssServerStatus == ssFree || m_ssServerStatus == ssInuse)
		{
			memset(uncaEscapeBuf, 0, sizeof(uncaEscapeBuf));

			bool bRecvAll = RecvAll("RECV_ESC", uncaEscapeBuf, 2);

			if(bRecvAll == false)
			{
				FprintfStderrLog("RECV_ESC_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), uncaEscapeBuf, sizeof(uncaEscapeBuf));
				continue;//ssBreakdown
			}

			if(uncaEscapeBuf[0] != ESCAPE_BYTE)
			{ 
				FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaEscapeBuf[0], __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
				continue;
			}
			else
			{
				int nToRecv = 0;
				int nIndex = 0;

				if(uncaEscapeBuf[1] == HEARTBEAT_BYTE)
				{
					nToRecv = strlen(g_pHeartbeatRequestMessage);
					nIndex = 2;
				}
				else if(uncaEscapeBuf[1] == m_uncaSecondByte)
				{
					nToRecv = m_nReplyMessageLength;
					nIndex = m_rmRequestMarket + 3;
				}
				else
				{
					FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaEscapeBuf[1], __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
					continue;
				}

				memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));

				bRecvAll = RecvAll(g_caWhat[nIndex], uncaRecvBuf, nToRecv);

				if(bRecvAll == false)
				{
					FprintfStderrLog(g_caWhatError[nIndex], -1, 0, __FILE__, __LINE__, uncaRecvBuf, nToRecv);
					continue;
				}
			}

			if(uncaEscapeBuf[1] == HEARTBEAT_BYTE)
			{
				if(memcmp(uncaRecvBuf, g_pHeartbeatRequestMessage, 4) == 0)
				{
					FprintfStderrLog("GET_SERVER_HTBT", 0, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));

					bool bSendAll = SendAll("HEARTBEAT_REPLY", g_uncaHeaetbeatReplyBuf, sizeof(g_uncaHeaetbeatReplyBuf));
					if(bSendAll == false)
					{
						FprintfStderrLog("HEARTBEAT_REPLY_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), g_uncaHeaetbeatReplyBuf, sizeof(g_uncaHeaetbeatReplyBuf));
					}
				}
				else if(memcmp(uncaRecvBuf, g_pHeartbeatReplyMessage, 4) == 0)
				{
					FprintfStderrLog("GET_SERVER_HTBR", 0, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
				}
				else
					FprintfStderrLog("HEARTBEAT_DATA_WRONG_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), uncaRecvBuf, 4);
			}
			else if(uncaEscapeBuf[1] == m_uncaSecondByte)
			{
#ifdef TIMETEST
				struct  timeval timeval_Start;
				struct  timeval timeval_End;
				gettimeofday (&timeval_Start, NULL);
				static int order_count = 0;
#endif

				CSKClient* pClient = m_shpClient.get();
				if(pClient)
				{
						if(memcmp(uncaRecvBuf + m_nOriginalOrderLength, g_pLogonFirstErrorMessage, strlen(g_pLogonFirstErrorMessage)) == 0)
						{
							FprintfStderrLog("PROXY_IP_SET_ERROR", -1, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), uncaRecvBuf, m_nReplyMessageLength);
							memcpy(uncaRecvBuf + m_nOriginalOrderLength, g_pProxyIPSetErrorMessage, strlen(g_pProxyIPSetErrorMessage));

							union L l;
							memset(&l,0,sizeof(L));

							l.value = PROXY_IP_SET_ERROR;
							memcpy(uncaRecvBuf + m_nOriginalOrderLength + m_nReplyMsgLength, l.uncaByte, 2);
						}

						bool bSendRequestReply = pClient->SendRequestReply(m_uncaSecondByte, uncaRecvBuf, m_nReplyMessageLength);
						if(bSendRequestReply == true)
						{
#ifdef TIMETEST
							gettimeofday (&timeval_End, NULL) ;
							InsertTimeLogToSharedMemory(&timeval_Start, &timeval_End, tpProxyToClient, order_count);
							InsertTimeLogToSharedMemory(NULL, &timeval_End, tpProxyProcessEnd, order_count);
							order_count++;
#endif
#ifdef MNTRMSG
							g_MNTRMSG.num_of_order_Reply++;
#endif
						}
						else
							FprintfStderrLog("SEND_REQUEST_REPLY_ERROR", -1, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), uncaRecvBuf, m_nReplyMessageLength);

					m_shpClient = NULL;
				}
				else
					FprintfStderrLog("CLIENT_OBJECT_NULL_ERROR", -1, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), uncaRecvBuf, m_nReplyMessageLength);

				SetStatus(ssFree);

				if(pServers)
				{
					pServers->ChangeInuseServerCount(m_rmRequestMarket, m_nPoolIndex, -1);
#ifdef MNTRMSG
					pServers->UpdateAvailableServerNum(m_rmRequestMarket);
#endif
				}
			}
			else
				FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaEscapeBuf[1], __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));

		}
		else if(m_ssServerStatus == ssNone)
		{
			FprintfStderrLog("SERVER_STATUS_NONE_ERROR", -1, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
		}
		else if(m_ssServerStatus == ssBreakdown)
		{
			SetStatus(ssReconnecting);
			ReconnectSocket();
		}
		else if(m_ssServerStatus == ssReconnecting)
		{
			FprintfStderrLog("SERVER_STATUS_RECONNECTING", 0, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
		}
		else
			FprintfStderrLog("SERVER_STATUS_ERROR", -1, m_ssServerStatus, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
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

void CSKServer::OnDisconnect()
{
	sleep(5);

//	m_pClientSocket->Connect( m_strWeb, m_strQstr, CONNECT_TCP);//start & reset heartbeat
}

void CSKServer::OnData(unsigned char* pBuf, int nSize)
{
}

void CSKServer::OnHeartbeatLost()
{
	if(m_pClientSocket)
	{
		m_pClientSocket->Disconnect();
	}
}

void CSKServer::OnHeartbeatRequest()
{
	bool bSendAll = SendAll("HEARTBEAT_REQUEST", g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
	if(bSendAll == false)
	{
		FprintfStderrLog("HEARTBEAT_REQUEST_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
	}
}

void CSKServer::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	FprintfStderrLog(pErrorMessage, -1, nData, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
}


void CSKServer::OnRequest()
{
#ifdef TIMETEST
	static int request_count = 0;
	struct  timeval timeval_Start;
	struct  timeval timeval_End;
	gettimeofday (&timeval_Start, NULL) ;
#endif
	bool bSendAll = SendAll("SEND_REQUEST", m_uncaRequestMessage, m_nRequestMessageLength);

	if(bSendAll == false)
	{
		FprintfStderrLog("ON_REQUEST_ERROR", -1, 0, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), m_uncaRequestMessage, m_nRequestMessageLength);
		return;
	}
#ifdef TIMETEST
	gettimeofday (&timeval_End, NULL) ;
//	InsertTimeLogToSharedMemory(&timeval_Start, &timeval_End, tpProxyToServer, request_count);
	request_count++;
#endif

#ifdef MNTRMSG
	g_MNTRMSG.num_of_order_Sent++;
#endif

}

void CSKServer::OnRequestError(int nData, const char* pErrorMessage)
{
	FprintfStderrLog(pErrorMessage, -1, nData, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
}

bool CSKServer::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
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
			FprintfStderrLog("SERVER_SOCKET_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
			break;
		}

		if(m_pHeartbeat)
			m_pHeartbeat->TriggerGetReplyEvent();
		else
			FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));

		if(nRecv > 0)
		{
			FprintfStderrLog(pWhat, 0, 0, __FILE__, 0, pBuf + nRecved, nRecv);
			nRecved += nRecv;
		}
		else if(nRecv == 0)//connection closed
		{
			FprintfStderrLog("SERVER_SOCKET_CLOSE", -1, m_rmRequestMarket, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
			SetStatus(ssBreakdown);

			break;
		}
		else if(nRecv == -1)
		{
			FprintfStderrLog("SERVER_SOCKET_ERROR", -1, errno, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)strerror(errno), strlen(strerror(errno)));
			SetStatus(ssBreakdown);

			break;
		}
		else
		{
			SetStatus(ssBreakdown);
			FprintfStderrLog("SERVER_SOCKET_RECV_ELSE_ERROR", -1, errno, NULL, 0, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)strerror(errno), strlen(strerror(errno)));

			break;
		}
	}
	while(nRecv != nToRecv);

	return nRecv == nToRecv ? true : false;
}

bool CSKServer::SendAll(const char* pWhat, const unsigned char* pBuf, int nToSend)
{
	int nSend = 0;
	int nSended = 0;

	do
	{
		nToSend -= nSend;

		if(m_pClientSocket)
			nSend = m_pClientSocket->Send(pBuf + nSended, nToSend);
		else
		{
			FprintfStderrLog("SERVER_SOCKET_NULL_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID));
			break;
		}

		if(nSend == -1)
		{
			SetStatus(ssBreakdown);
			FprintfStderrLog("SEND_TIG_ERROR", -1, errno, NULL, 0, (unsigned char*)pBuf, nToSend, (unsigned char*)strerror(errno), strlen(strerror(errno)));
			break;
		}

		if(nSend == nToSend)
		{
			FprintfStderrLog(pWhat, 0, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)pBuf + nSended, nSend);
			break;
		}
		else if(nSend < nToSend)
		{
			FprintfStderrLog(pWhat, 0, 0, __FILE__, __LINE__, (unsigned char*)m_caPthread_ID, sizeof(m_caPthread_ID), (unsigned char*)pBuf + nSended, nSend);
			nSended += nSend;
		}
		else
		{
			FprintfStderrLog("SEND_TIG_ELSE_ERROR", -1, errno, NULL, 0, (unsigned char*)pBuf, nToSend, (unsigned char*)strerror(errno), strlen(strerror(errno)));
			break;
		}
	}
	while(nSend != nToSend);

	return nSend == nToSend ? true : false;
}

void CSKServer::ReconnectSocket()
{
	sleep(5);

	if(m_pClientSocket)
	{
		m_pClientSocket->Disconnect();

		m_pClientSocket->Connect( m_strWeb, m_strQstr, CONNECT_TCP);//start & reset heartbeat
	}
}

void CSKServer::SetCallback(shared_ptr<CSKClient>& shpClient)
{
	m_shpClient = shpClient;
}

void CSKServer::SetRequestMessage(unsigned char* pRequestMessage, int nRequestMessageLength)
{
	try
	{
		if(nRequestMessageLength > sizeof(m_uncaRequestMessage))
		{
			throw "REQUEST_DATA_LENGTH_TOO_LARGE";
		}
		memset(m_uncaRequestMessage, 0, sizeof(m_uncaRequestMessage));
		memcpy(m_uncaRequestMessage, pRequestMessage, nRequestMessageLength);

		m_nRequestMessageLength = nRequestMessageLength;
	}
	catch(const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, nRequestMessageLength, NULL, 0, pRequestMessage, nRequestMessageLength);
	}
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
