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
#include "CVServers.h" 
#include "CVWebClients.h"
#include "CVCommon/CVClientSocket.h"
#include "CVGlobal.h"
#include "Include/CVTSFormat.h"
#include "Include/CVTFFormat.h"
#include "Include/CVOFFormat.h"
#include "Include/CVOSFormat.h"
#include "CVClearVector.h"

#include<iostream>
using namespace std;

#define MAXDATA 1024

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
                             unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKClient::CSKClient(struct TSKClientAddrInfo &ClientAddrInfo)
{
	memset(&m_ClientAddrInfo, 0, sizeof(struct TSKClientAddrInfo));
	memcpy(&m_ClientAddrInfo, &ClientAddrInfo, sizeof(struct TSKClientAddrInfo));

	m_pHeartbeat = NULL;

	m_csClientStatus = csNone;

	m_nOriginalOrderLength = 0;
	struct SK_TS_REPLY sk_ts_reply;
	m_nTSReplyMsgLength = sizeof(sk_ts_reply.reply_msg);
	struct SK_TF_REPLY sk_tf_reply;
	m_nTFReplyMsgLength = sizeof(sk_tf_reply.reply_msg);
	struct SK_OF_REPLY sk_of_reply;
	m_nOFReplyMsgLength = sizeof(sk_of_reply.reply_msg);
	struct SK_OS_REPLY sk_os_reply;
	m_nOSReplyMsgLength = sizeof(sk_os_reply.reply_msg);

	InitialBranchAccountInfo();

	pthread_mutex_init(&m_pmtxClientStatusLock, NULL);

	srand(time(NULL));

	Start();
}

CSKClient::~CSKClient() 
{
	if( m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	ClearVectorContent(m_vBranchAccountInfo);

	pthread_mutex_destroy(&m_pmtxClientStatusLock);
#ifdef SSLTLS
	SSL_shutdown(m_ssl);
	BIO_free_all(m_bio);
	BIO_free_all(m_accept_bio);
	ERR_free_strings();
#endif
}


void* CSKClient::Run()
{
	unsigned char uncaRecvBuf[MAXDATA];
	unsigned char uncaEscapeBuf[2];
	unsigned char uncaIVBuf[INITIALIZATION_VECTOR_LENGTH];

	SetStatus(csLogoning);
	enum TSKRequestMarket rmRequestMarket = rmNum;
	bool bLogon;

	while(1) {
		usleep(10);
	//	send(m_ClientAddrInfo.nSocket, netmsg, strlen(netmsg), 0);
	}
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
		m_pHeartbeat->SetTimeInterval( stoi(pClients->m_strHeartBeatTime) );
	}
	catch(exception& e)
	{
		FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
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
	while(m_csClientStatus != csOffline)
	{
		bool bSendLogonReply;
		bool bIsServiceRunning;

		memset(uncaEscapeBuf, 0, sizeof(uncaEscapeBuf));

		bool bRecvAll = RecvAll("RECV_ESC", uncaEscapeBuf, 2);

		if(bRecvAll == false)
		{
			FprintfStderrLog("RECV_ESC_ERROR", -1, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), uncaEscapeBuf, sizeof(uncaEscapeBuf));
			continue;
		}

		if(uncaEscapeBuf[0] != ESCAPE_BYTE)
		{
			FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaEscapeBuf[0], __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID));
			continue;
		}
		else
		{
			int nToRecv = 0;
			int nIndex = 0;
			switch(uncaEscapeBuf[1])
			{
				case LOGON_BYTE:
					nToRecv = sizeof(struct TSKLogon);
					nIndex = 1;
					break;
				case HEARTBEAT_BYTE:
					nToRecv = strlen(g_pHeartbeatRequestMessage);
					nIndex = 2;
					break;
				case TS_ORDER_BYTE:
					nToRecv = m_nOriginalOrderLength = sizeof(struct SK_TS_ORDER);
					nIndex = 3;
					break;
				case TF_ORDER_BYTE:
					nToRecv = m_nOriginalOrderLength = sizeof(struct SK_TF_ORDER);
					nIndex = 4;
					break;
				case OF_ORDER_BYTE:
					nToRecv = m_nOriginalOrderLength = sizeof(struct SK_OF_ORDER);
					nIndex = 5;
					break;
				case OS_ORDER_BYTE:
					nToRecv = m_nOriginalOrderLength = sizeof(struct SK_OS_ORDER);
					nIndex = 6;
					break;
				case EXIT_BYTE:
					SetStatus(csOffline);
					break;
				default:
					FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaEscapeBuf[1], __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID));
					continue;
			}

			if(m_csClientStatus == csOffline)
				break;

			memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
			bRecvAll = RecvAll(g_caWhat[nIndex], uncaRecvBuf, nToRecv);
			if(bRecvAll == false)
			{
				FprintfStderrLog(g_caWhatError[nIndex], -1, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRecvBuf, nToRecv);
				continue;
			}
		}

		switch(uncaEscapeBuf[1])
		{
			case LOGON_BYTE:
				if(m_csClientStatus == csLogoning)
				{
					struct TSKLogon struLogon;
					memset(&struLogon, 0, sizeof(struct TSKLogon));
					memcpy(&struLogon, uncaRecvBuf, sizeof(struct TSKLogon));

					struct TSKLogonReply struLogonReply;
					memset(&struLogonReply, 0, sizeof(struct TSKLogonReply));

					memset(m_uncaLogonID, 0, sizeof(m_uncaLogonID));
					memcpy(m_uncaLogonID, struLogon.logon_id, sizeof(m_uncaLogonID));

					bLogon = Logon(struLogon.logon_id, struLogon.password, struLogonReply);	//logon & get logon reply data

#ifdef DEBUG
					bLogon = true;	//skip login check since server is not stable in test environment.
#endif
					bSendLogonReply = SendLogonReply(struLogonReply);

					if(bSendLogonReply == false)
					{
						FprintfStderrLog("SEND_LOGON_REPLY_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
					}

					if(bLogon)//success
					{
						SetStatus(csOnline);
						m_pHeartbeat->Start();

						vector<char*> vAccountData;

						bool bGetAccount = GetAccount(struLogon.logon_id, struLogon.agent, struLogon.version, vAccountData);

						if(bGetAccount)
						{
							bool bSendAccountCount = SendAccountCount(vAccountData.size());
							if(bSendAccountCount == false)
							{
								FprintfStderrLog("SEND_ACCOUNT_COUNT_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
							}

							bool bSendAccoundData = SendAccountData(vAccountData);
							if(bSendAccoundData == false)
							{
								FprintfStderrLog("SEND_ACCOUNT_DATA_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
							}
						}
						else
						{
							FprintfStderrLog("GET_ACCOUNT_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
						}

						ClearVectorContent(vAccountData);
					}
					else//logon failed
					{
						FprintfStderrLog("LOGON_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
						continue;
					}
				}
				else if(m_csClientStatus == csOnline)//repeat logon
				{
					struct TSKLogonReply struLogonReply;
					memset(&struLogonReply, 0, sizeof(struct TSKLogonReply));

					memcpy(struLogonReply.status_code, REPEAT_LOGON_STATUS_CODE, 4);
					memcpy(struLogonReply.error_code, REPEAT_LOGON_ERROR_CODE, 4);

					bSendLogonReply = SendLogonReply(struLogonReply);

					if(bSendLogonReply == false)
					{
						FprintfStderrLog("SEND_LOGON_REPLY_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
					}
				}
				else
				{
					FprintfStderrLog("CLIENT_STATUS_ERROR", -1, m_csClientStatus, __FILE__, __LINE__ , m_uncaLogonID, sizeof(m_uncaLogonID));
				}
				break;

			case HEARTBEAT_BYTE:
				if(memcmp(uncaRecvBuf, g_pHeartbeatRequestMessage, 4) == 0)
				{
					FprintfStderrLog("GET_CLIENT_HTBT", 0, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));

					bool bSendAll = SendAll("HEARTBEAT_REPLY", g_uncaHeaetbeatReplyBuf, sizeof(g_uncaHeaetbeatReplyBuf));
					if(bSendAll == false)
					{
						FprintfStderrLog("HEARTBEAT_REPLY_ERROR", -1, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), g_uncaHeaetbeatReplyBuf, sizeof(g_uncaHeaetbeatReplyBuf));
					}
				}
				else if(memcmp(uncaRecvBuf, g_pHeartbeatReplyMessage, 4) == 0)
				{
					FprintfStderrLog("GET_CLIENT_HTBR", 0, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
				}
				else
					FprintfStderrLog("HEARTBEAT_DATA_WRONG_ERROR", -1, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRecvBuf, 4);
				break;

			case TS_ORDER_BYTE:
			case TF_ORDER_BYTE:
			case OF_ORDER_BYTE:
			case OS_ORDER_BYTE:
				{
#ifdef TIMETEST
					static int order_count = 0;
					struct  timeval timeval_End;
					gettimeofday (&timeval_End, NULL) ;
					InsertTimeLogToSharedMemory(NULL, &timeval_End, tpProxyProcessStart, order_count);
					order_count++;
#endif
#ifdef MNTRMSG
					g_MNTRMSG.num_of_order_Received++;
#endif

					if(uncaEscapeBuf[1] == TS_ORDER_BYTE)
					{
						rmRequestMarket = rmBitmex;
					}
					else
					{
						FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaEscapeBuf[1], __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID));
					}

					if(m_csClientStatus == csLogoning)
					{
						bool bSendRequestErrorReply = SendRequestErrorReply(uncaEscapeBuf[1], uncaRecvBuf, m_nOriginalOrderLength, g_pLogonFirstErrorMessage, LOGON_FIRST_ERROR);

						if(bSendRequestErrorReply == false)
						{
							FprintfStderrLog("SEND_REQUEST_ERROR_REPLY_ERROR", -1, LOGON_FIRST_ERROR, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRecvBuf, m_nOriginalOrderLength);
						}
						continue;
					}
					else if(m_csClientStatus == csOnline)
					{
#ifndef DEBUG //SKIP account check in debug mode.
						bool bCheckBranchAccount = CheckBranchAccount(rmRequestMarket, uncaRecvBuf);

						if(bCheckBranchAccount == false)
						{
							bool bSendRequestErrorReply = SendRequestErrorReply(uncaEscapeBuf[1], uncaRecvBuf, m_nOriginalOrderLength, g_pBranchAccountErrorMessage, BRANCH_ACCOUNT_ERROR);

							if(bSendRequestErrorReply == false)
							{
								FprintfStderrLog("SEND_REQUEST_ERROR_REPLY_ERROR", -1, BRANCH_ACCOUNT_ERROR, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRecvBuf, m_nOriginalOrderLength);
							}
							continue;
						}
#endif

						if(pClients)
							bIsServiceRunning = pClients->IsServiceRunning(rmRequestMarket);
						else
							FprintfStderrLog("CLIENTS_LOST_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));

						if(bIsServiceRunning == false)
						{
							bool bSendRequestErrorReply = SendRequestErrorReply(uncaEscapeBuf[1], uncaRecvBuf, m_nOriginalOrderLength, g_pProxyNotProvideServiceErrorMessage, PROXY_NOT_PROVIDE_SERVICE_ERROR);
						
							if(bSendRequestErrorReply == false)
							{
								FprintfStderrLog("SEND_REQUEST_ERROR_REPLY_ERROR", -1, PROXY_NOT_PROVIDE_SERVICE_ERROR, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRecvBuf, m_nOriginalOrderLength);
							}
							continue;
						}

						unsigned char uncaRequestMessage[MAXDATA];
						memset(uncaRequestMessage, 0, sizeof(uncaRequestMessage));

						memcpy(uncaRequestMessage, uncaEscapeBuf, 2);
						memcpy(uncaRequestMessage + 2, uncaRecvBuf, m_nOriginalOrderLength);
						memcpy(uncaRequestMessage + 2 + m_nOriginalOrderLength, m_ClientAddrInfo.caIP, IPLEN);
#ifdef MNTRMSG
						memcpy(uncaRequestMessage + 2 + m_nOriginalOrderLength+IPLEN, m_userID, UIDLEN);
#endif
						CSKServer* pServer = NULL;

						if(pServers)
						{
							pServer = pServers->GetFreeServerObject(rmRequestMarket);

							if(pServer)
							{
								TriggerSendRequestEvent(pServer, uncaRequestMessage, 2 + m_nOriginalOrderLength + IPLEN + UIDLEN);
							}
							else
							{
								FprintfStderrLog("GET_SERVER_FAILED_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRequestMessage, 2 + m_nOriginalOrderLength + sizeof(m_ClientAddrInfo.caIP));

								bool bSendRequestErrorReply = SendRequestErrorReply(uncaEscapeBuf[1], uncaRecvBuf, m_nOriginalOrderLength, g_pProxyServiceBusyErrorMessage, PROXY_SERVICE_BUSY_ERROR);
								if(bSendRequestErrorReply == false)
								{
									FprintfStderrLog("SEND_REQUEST_ERROR_REPLY_ERROR", -1, PROXY_SERVICE_BUSY_ERROR, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRecvBuf, m_nOriginalOrderLength);
								}
								continue;
							}
						}
						else
							FprintfStderrLog("SERVERS_LOST_ERROR", -1, 0, NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID));
					}
					else
					{
						FprintfStderrLog("CLIENT_STATUS_ERROR", -1, m_csClientStatus, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID));
					}
					break;
				}
			default:
				FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, uncaEscapeBuf[1], __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID));
				continue;
		}
	}

	if(m_pHeartbeat)
		m_pHeartbeat->TriggerTerminateEvent();

	return NULL;
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

bool CSKClient::Logon(char* pID, char* pPasswd, struct TSKLogonReply &struLogonReply)
{	
	CSKClientSocket* pClientSocket = NULL;
	try
	{
		pClientSocket = new CSKClientSocket();
		pClientSocket->Connect("pass.capital.com.tw", "80", CONNECT_TCP);
	}
	catch(exception& e)
	{
		FprintfStderrLog("NEW_LOGON_CLIENT_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)e.what(), strlen(e.what()));
		return false;
	}

	char caBuf[MAXDATA];
	unsigned char uncaRecvBuf[MAXDATA];
	int nToSend = 0;
	int nSended = 0;

	memset(caBuf, 0, MAXDATA);
	memset(uncaRecvBuf, 0, MAXDATA);

	char pass_ip[50];
	memset(pass_ip, 0, 50);
	sprintf(pass_ip, "pass-sk.capital.com.tw");

	char cust_id[11];
	memset(cust_id, 0,11);
	memcpy(cust_id, pID, UIDLEN);

	char en_id_pass[50];
	memset(en_id_pass, 0, 50);
	memcpy(en_id_pass, pPasswd, 50);

	sprintf(caBuf,"GET http://%s/PasswordGW/Gateway.asp?IDNO=%s&Func=2&PswdType=0&Password=%s&IP=%s HTTP/1.0\n\n",
			pass_ip, cust_id, en_id_pass, m_ClientAddrInfo.caIP);


	nToSend = strlen(caBuf);
	nSended = 0;
	while(nToSend > 0)
	{
		int nSend = pClientSocket->Send(reinterpret_cast<unsigned char*>(caBuf) + nSended, nToSend);

		if(nSend == nToSend)
			break;
		else if(nSend < nToSend)
		{
			nToSend -= nSend;
			nSended += nSend;
		}
		else if(nSend == -1)
		{
			FprintfStderrLog("LOGON_SOCKET_SEND_ERROR", -1, errno, NULL, 0, (unsigned char*)strerror(errno), strlen(strerror(errno)));

			if(pClientSocket)
			{
				delete pClientSocket;
				pClientSocket = NULL;
			}

			return false;
		}
	}

	int nRecv = pClientSocket->Recv(uncaRecvBuf,MAXDATA);
	if(nRecv == -1)
	{
		FprintfStderrLog("LOGON_SOCKET_RECV_ERROR", -1, errno, NULL, 0, (unsigned char*)strerror(errno), strlen(strerror(errno)));
		return false;
	}

	//dump http reply message.
	cout << uncaRecvBuf << endl;

	if(pClientSocket)
	{
		delete pClientSocket;
		pClientSocket = NULL;
	}

	/*************************************************************************************************/

	char caRecvBuf[MAXDATA];

	char* pFirstToken = NULL;
	char* pToken = NULL;

	char caItem[30][512];
	int nItemCount;

	char castatus_code[5], caHttpMessage[512];
	memset(caRecvBuf, 0, MAXDATA);
	memset(caItem, 0, sizeof(caItem));
	memset(castatus_code, 0, 5);
	memset(caHttpMessage, 0, 512);
	memcpy(caRecvBuf, uncaRecvBuf, sizeof(caRecvBuf));

	pFirstToken = strstr(caRecvBuf, "200 OK");
	if(pFirstToken == NULL)
	{
		memcpy(struLogonReply.error_code, "M999", 4);
		sprintf(struLogonReply.error_message, "Check PASS return error#");
		return false;
	}

	pFirstToken = strstr(caRecvBuf, "\r\n\r\n");

	if(pFirstToken == NULL)
	{
		//todo: error web reply format.
	}
	else
	{
		pToken = strtok(pFirstToken, ",");
		nItemCount = 0;
		while(1)
		{
			if(pToken == NULL)
				break;
			strcpy(caItem[nItemCount], pToken);
			nItemCount++;
			pToken = strtok(NULL, ",");
		}

		pToken = strtok(caItem[0], "=");
		pToken = strtok(NULL, "=");
		sprintf(castatus_code, "%04d", atol(pToken));
		for(int i=1;i<nItemCount;i++)
		{
			if(strncmp(caItem[i], "Msg", 3) == 0)
			{
				pToken = strtok(caItem[i], "=");
				pToken = strtok(NULL, "=");
				sprintf(caHttpMessage, "%s", pToken);
			}
		}

		if(strncmp(castatus_code, "0000", 4) == 0 || strncmp(castatus_code, "0001", 4) == 0)
				//strncmp(castatus_code, "7995", 4) == 0 )
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M000", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			//SetStatus(csOnline);
#ifdef MNTRMSG
			memset(m_userID, 0, UIDLEN);
			memcpy(m_userID, pID, UIDLEN);
#endif
			return true;
		}
		else if (strncmp(castatus_code, "7997", 4) == 0)
		{ /* first login */
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M151", 4);
			sprintf(struLogonReply.error_message, "±K½X¿ù»~#");
			return false;
		}
		else if (strncmp(castatus_code, "7996", 4) == 0)
		{ /* first login */
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M155", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else if(strncmp(castatus_code, "7993", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M156", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			//SetStatus(csOnline);
			return true;
		}
		else if(strncmp(castatus_code, "7992", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M156", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			//SetStatus(csOnline);
			return true;
		}
		else if (strncmp(castatus_code, "7998", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M152", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else if (strncmp(castatus_code, "1999", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M999", 4);
			sprintf(struLogonReply.error_message, "M999 pass server busy#");
			return false;
		}
		else if (strncmp(castatus_code, "8992", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M153", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else if (strncmp(castatus_code, "8994", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M153", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else if (strncmp(castatus_code, "8995", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M153", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else if (strncmp(castatus_code, "8996", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M153", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else if (strncmp(castatus_code, "8997", 4) == 0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M153", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else if (strncmp(castatus_code, "8998", 4)==0)
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M153", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
		else
		{
			memcpy(struLogonReply.status_code, castatus_code, 4);
			memcpy(struLogonReply.error_code, "M153", 4);
			memcpy(struLogonReply.error_message, caHttpMessage, sizeof(struLogonReply.error_message));
			return false;
		}
	}
}

bool CSKClient::GetAccount(char* pID, char* pAgent, char* pVersion, vector<char*> &vAccountData)
{
	CSKClientSocket* pClientSocket = NULL;
	unsigned char uncaRecvBuf[MAXDATA];
	char caSendBuf[MAXDATA];
	char caItem[10][128];
	int nToSend = 0, nSended = 0, nRecv = 0, nRecved = 0, nIndex = 0, nItemCount = 0;
	vector<struct TSKAccountRecvBuf> vAccountRecvBuf;
	char* pch = NULL;
	char* caRecvBuf = NULL;

	try
	{
		pClientSocket = new CSKClientSocket();
		pClientSocket->Connect("aptrade.capital.com.tw", "4000", CONNECT_TCP);
	}
	catch(exception& e)
	{
		FprintfStderrLog("NEW_GET_ACCOUNT_CLIENT_SOCKET_ERROR", -1, 0, NULL, 0, (unsigned char*)e.what(), strlen(e.what()));
		return false;
	}

	memset(caSendBuf, 0, sizeof(caSendBuf));
	sprintf(caSendBuf, "B real_no_pass 01000 %10.10s %c %c 0 0 0 0##", pID, pAgent, pVersion);
	nToSend = strlen(caSendBuf);
	nSended = 0;
	while(nToSend > 0)
	{
		int nSend = pClientSocket->Send(reinterpret_cast<unsigned char*>(caSendBuf) + nSended, nToSend);

		if(nSend == nToSend)
			break;

		else if(nSend < nToSend)
		{
			nToSend -= nSend;
			nSended += nSend;
		}

		else if(nSend == -1)
		{
			FprintfStderrLog("GET_ACCOUNT_SOCKET_SEND_ERROR", -1, errno, NULL, 0,(unsigned char*)strerror(errno), strlen(strerror(errno)));

			if(pClientSocket)
			{
				delete pClientSocket;
				pClientSocket = NULL;
			}

			return false;
		}
	}

	nRecv = 0;
	nRecved = 0;
	do
	{
		struct TSKAccountRecvBuf struAccountRecvBuf;
		memset(&struAccountRecvBuf, 0, sizeof(struct TSKAccountRecvBuf));

		nRecv = pClientSocket->Recv(struAccountRecvBuf.uncaRecvBuf, sizeof(struAccountRecvBuf.uncaRecvBuf));
		if(nRecv > 0)
		{
			struAccountRecvBuf.nRecved = nRecv;
			vAccountRecvBuf.push_back(struAccountRecvBuf);
			nRecved += nRecv;
		}
		else if(nRecv == -1)
		{
			FprintfStderrLog("GET_ACCOUNT_SOCKET_RECV_ERROR", -1, errno, NULL, 0, (unsigned char*)strerror(errno), strlen(strerror(errno)));

			if(pClientSocket)
			{
				delete pClientSocket;
				pClientSocket = NULL;
			}

			return false;
		}
	}
	while(nRecv != 0);

	caRecvBuf = new char[nRecved+1];

	memset(caRecvBuf, 0, nRecved+1);

	nIndex = 0;

	for(vector<struct TSKAccountRecvBuf>::iterator iter = vAccountRecvBuf.begin(); iter != vAccountRecvBuf.end(); iter++)
	{
		memcpy(caRecvBuf + nIndex, iter->uncaRecvBuf, iter->nRecved);
		nIndex += iter->nRecved;
	}
	vAccountRecvBuf.clear();

	if(pClientSocket)
	{
		delete pClientSocket;
		pClientSocket = NULL;
	}

	vector<char*> vAccountItem;
	
	pch = strtok(caRecvBuf, "#");
	while(pch != NULL)//todo test
	{
		printf("%s\n",pch);

		//struct TSKAccountItem struAccountItem;
		char* pAccountItem = new char[ACCOUNT_ITEM_LENGTH];
		memset(pAccountItem, 0, ACCOUNT_ITEM_LENGTH);

		strcpy(pAccountItem, pch);
		vAccountItem.push_back(pAccountItem);

		pch = strtok(NULL, "#");
	}
	delete [] caRecvBuf;

	nItemCount = 0;
	for(vector<char*>::iterator iter = vAccountItem.begin(); iter != vAccountItem.end(); iter++)
	{
		memset(caItem, 0, sizeof(caItem));

		pch = strtok(*iter, ",");
		while(pch != NULL)
		{
			strcpy(caItem[nItemCount], pch);
			nItemCount++;
			pch = strtok(NULL, ",");
		}
		delete *iter;
		*iter = NULL;
		nItemCount = 0;

		//struct TSKAccountData struAccountData;
		char* pAccountData = new char[ACCOUNT_DATA_LENGTH];
		memset(pAccountData, 0, ACCOUNT_DATA_LENGTH);

		string strBranch(caItem[2]);
		memcpy(pAccountData, caItem[2], 10);

		string strAccount(caItem[3]);
		memcpy(pAccountData + 10, caItem[3], 10);
		if(memcmp(caItem[0], "OS", 2) == 0)
		{
			if(strlen(caItem[3]) == 7)
			{
				strAccount.insert(0, 1, '0');
				memcpy(pAccountData + 11, pAccountData + 10, 7);
				memset(pAccountData + 10, '0', 1);
			}
		}

		string strSubAccount(caItem[5]);
		memcpy(pAccountData + 20, caItem[5], 10);
		if(memcmp(caItem[0], "OS", 2) == 0)
		{
			if(strlen(caItem[5]) == 7)
			{
				strSubAccount.insert(0, 1, '0');
				memcpy(pAccountData + 21, pAccountData + 20, 7);
				memset(pAccountData + 20, '0', 1);
			}
		}

		vAccountData.push_back(pAccountData);

		string strMarket(caItem[0]);

		string strMarketBranchAccount = strMarket + strBranch + strAccount + strSubAccount;
		m_mMarketBranchAccount.insert(std::pair<string, string>(strMarketBranchAccount, strMarketBranchAccount));
	}

	ClearVectorContent(vAccountItem);

	return true;
}

bool CSKClient::RecvAll(const char* pWhat, unsigned char* pBuf, int nToRecv)
{
	int nRecv = 0;
	int nRecved = 0;

	do
	{
		nToRecv -= nRecv;
#ifdef SSLTLS
		nRecv = SSL_read(m_ssl, pBuf + nRecved, nToRecv);
#else
		nRecv = recv(m_ClientAddrInfo.nSocket, pBuf + nRecved, nToRecv, 0);
#endif

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

#ifdef SSLTLS
		nSend = SSL_write(m_ssl, pBuf + nSended, nToSend);
#else
		nSend = send(m_ClientAddrInfo.nSocket, pBuf + nSended, nToSend, 0);
#endif
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
				FprintfStderrLog(pWhat, 0, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), (unsigned char*)pBuf + nSended, nSend);
				break;
			}
			else if(nSend < nToSend)
			{
				FprintfStderrLog(pWhat, -1, 0, __FILE__, __LINE__, m_uncaLogonID, sizeof(m_uncaLogonID), (unsigned char*)pBuf + nSended, nSend);
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

void CSKClient::InitialBranchAccountInfo()
{
	struct SK_TS_ORDER sk_ts_order;
	struct SK_TF_ORDER sk_tf_order;
	struct SK_OF_ORDER sk_of_order;
	struct SK_OS_ORDER sk_os_order;

	struct TSKBranchAccountInfo* pstruTSBranchAccountInfo = new struct TSKBranchAccountInfo;
	struct TSKBranchAccountInfo* pstruTFBranchAccountInfo = new struct TSKBranchAccountInfo;
	struct TSKBranchAccountInfo* pstruOFBranchAccountInfo = new struct TSKBranchAccountInfo;
	struct TSKBranchAccountInfo* pstruOSBranchAccountInfo = new struct TSKBranchAccountInfo;

	m_vBranchAccountInfo.push_back(pstruTSBranchAccountInfo);
	m_vBranchAccountInfo.push_back(pstruTFBranchAccountInfo);
	m_vBranchAccountInfo.push_back(pstruOFBranchAccountInfo);
	m_vBranchAccountInfo.push_back(pstruOSBranchAccountInfo);

	memset(pstruTSBranchAccountInfo, 0, sizeof(struct TSKBranchAccountInfo));
	memset(pstruTFBranchAccountInfo, 0, sizeof(struct TSKBranchAccountInfo));
	memset(pstruOFBranchAccountInfo, 0, sizeof(struct TSKBranchAccountInfo));
	memset(pstruOSBranchAccountInfo, 0, sizeof(struct TSKBranchAccountInfo));

	pstruTSBranchAccountInfo->pMarket = TS_MARKET_CODE;
	pstruTFBranchAccountInfo->pMarket = TF_MARKET_CODE;
	pstruOFBranchAccountInfo->pMarket = OF_MARKET_CODE;
	pstruOSBranchAccountInfo->pMarket = OS_MARKET_CODE;

	pstruTSBranchAccountInfo->nBranchPosition 		= sk_ts_order.broker - (char*)&sk_ts_order;
	pstruTSBranchAccountInfo->nAccountPosition 		= sk_ts_order.acno - (char*)&sk_ts_order;
	pstruTSBranchAccountInfo->nSubAccountPosition 	= sk_ts_order.sub_acno - (char*)&sk_ts_order;
	pstruTSBranchAccountInfo->nBranchLength 	= sizeof(sk_ts_order.broker);
	pstruTSBranchAccountInfo->nAccountLength 	= sizeof(sk_ts_order.acno);
	pstruTSBranchAccountInfo->nSubAccountLength = sizeof(sk_ts_order.sub_acno);

	pstruTFBranchAccountInfo->nBranchPosition 		= sk_tf_order.unit_no - (char*)&sk_tf_order;
	pstruTFBranchAccountInfo->nAccountPosition 		= sk_tf_order.acno - (char*)&sk_tf_order;
	pstruTFBranchAccountInfo->nSubAccountPosition 	= sk_tf_order.sub_acno - (char*)&sk_tf_order;
	pstruTFBranchAccountInfo->nBranchLength 	= sizeof(sk_tf_order.unit_no);
	pstruTFBranchAccountInfo->nAccountLength 	= sizeof(sk_tf_order.acno);
	pstruTFBranchAccountInfo->nSubAccountLength = sizeof(sk_tf_order.sub_acno);

	pstruOFBranchAccountInfo->nBranchPosition 		= sk_of_order.unit_no - (char*)&sk_of_order;
	pstruOFBranchAccountInfo->nAccountPosition 		= sk_of_order.acno - (char*)&sk_of_order;
	pstruOFBranchAccountInfo->nSubAccountPosition 	= sk_of_order.sub_acno - (char*)&sk_of_order;
	pstruOFBranchAccountInfo->nBranchLength 	= sizeof(sk_of_order.unit_no);
	pstruOFBranchAccountInfo->nAccountLength 	= sizeof(sk_of_order.acno);
	pstruOFBranchAccountInfo->nSubAccountLength = sizeof(sk_of_order.sub_acno);

	pstruOSBranchAccountInfo->nBranchPosition 		= sk_os_order.unit_no2- (char*)&sk_os_order;
	pstruOSBranchAccountInfo->nAccountPosition 		= sk_os_order.acno - (char*)&sk_os_order;
	pstruOSBranchAccountInfo->nSubAccountPosition 	= sk_os_order.sub_acno - (char*)&sk_os_order;
	pstruOSBranchAccountInfo->nBranchLength 	= sizeof(sk_os_order.unit_no2);
	pstruOSBranchAccountInfo->nAccountLength 	= sizeof(sk_os_order.acno);
	pstruOSBranchAccountInfo->nSubAccountLength = sizeof(sk_os_order.sub_acno);
}

bool CSKClient::CheckBranchAccount(TSKRequestMarket rmRequestMarket, unsigned char* pRequestMessage)
{
	try
	{
		if(rmRequestMarket == rmBitmex)
		{
			char caBranchAccount[4+8+8+1];
			memset(caBranchAccount, 0, sizeof(caBranchAccount));

			struct TSKBranchAccountInfo* pstruBranchAccountInfo = m_vBranchAccountInfo.at(rmRequestMarket);
			memcpy(caBranchAccount, pRequestMessage + pstruBranchAccountInfo->nBranchPosition, pstruBranchAccountInfo->nBranchLength);
			memcpy(caBranchAccount + pstruBranchAccountInfo->nBranchLength, pRequestMessage + pstruBranchAccountInfo->nAccountPosition, pstruBranchAccountInfo->nAccountLength);
			memcpy(caBranchAccount + pstruBranchAccountInfo->nBranchLength + pstruBranchAccountInfo->nAccountLength, pRequestMessage + pstruBranchAccountInfo->nSubAccountPosition, pstruBranchAccountInfo->nSubAccountLength);

			string strBranchAccount(caBranchAccount);
			string strMarket(pstruBranchAccountInfo->pMarket);
			string strMarketBranchAccount = strMarket + strBranchAccount;

			if(m_mMarketBranchAccount.count(strMarketBranchAccount) <= 0)
				return false;
				//return -1124;
			else
				return true;
		}
		else
			FprintfStderrLog("REQUEST_MARKET_ERROR", -1, rmRequestMarket, __FILE__, __LINE__);
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}
}


void CSKClient::TriggerSendRequestEvent(CSKServer* pServer, unsigned char* pRequestMessage, int nRequestMessageLength)
{
	shared_ptr<CSKClient> shpClient{ shared_from_this() };

	pServer->SetCallback(shpClient);
	pServer->SetRequestMessage(pRequestMessage, nRequestMessageLength);
	pServer->m_pRequest->TriggerWakeUpEvent();
}

bool CSKClient::SendLogonReply(struct TSKLogonReply &struLogonReply)
{
	unsigned char* unpSendBuf = new unsigned char[2 + sizeof(struct TSKLogonReply)];
	memset(unpSendBuf, 0, 2 + sizeof(struct TSKLogonReply));

	unpSendBuf[0] = ESCAPE_BYTE;
	unpSendBuf[1] = LOGON_BYTE;
	
	memcpy(unpSendBuf + 2, &struLogonReply, sizeof(struct TSKLogonReply));

	bool bSendAll = SendAll("SEND_LOGON_REPLY", unpSendBuf, 2 + sizeof(struct TSKLogonReply));

	delete [] unpSendBuf;

	return bSendAll;
}

bool CSKClient::SendAccountCount(short sCount)
{
	unsigned char uncaSendBuf[2 + 2];
	memset(uncaSendBuf, 0, sizeof(uncaSendBuf));

	union L l;
	memset(&l,0,16);

	uncaSendBuf[0] = ESCAPE_BYTE;
	uncaSendBuf[1] = ACCOUNT_COUNT_BYTE;
	
	l.value = sCount;
	memcpy(uncaSendBuf + 2, l.uncaByte, 2);

	bool bSendAll = SendAll("SEND_ACCOUNT_COUNT", uncaSendBuf, 2 + 2);

	return bSendAll;
}

bool CSKClient::SendAccountData(vector<char*> &vAccountData)//to test
{
	unsigned char uncaSendBuf[2 + ACCOUNT_DATA_LENGTH*10];
	memset(uncaSendBuf, 0, sizeof(uncaSendBuf));

	uncaSendBuf[0] = ESCAPE_BYTE;
	uncaSendBuf[1] = ACCOUNT_DATA_BYTE;

	int nRemainCountOfAccountData = vAccountData.size();
	int nCountOfToSendAccountData = 0;
	int nIndexOfBuf = 2;
	for(vector<char*>::iterator iter = vAccountData.begin(); iter != vAccountData.end(); iter++)
	{
		memcpy(uncaSendBuf + nIndexOfBuf, *iter, ACCOUNT_DATA_LENGTH);
		nCountOfToSendAccountData += 1;
		nIndexOfBuf += 30;
		nRemainCountOfAccountData -= 1;

		if(nCountOfToSendAccountData == MAX_ACCOUNT_DATA_COUNT || nRemainCountOfAccountData == 0)
		{
			bool bSendAll = SendAll("SEND_ACCOUNT_DATA", uncaSendBuf, 2 + nCountOfToSendAccountData * ACCOUNT_DATA_LENGTH);
			if(bSendAll == false)
			{
				return false;
			}

			nCountOfToSendAccountData = 0;
			nIndexOfBuf = 2;
			memset(uncaSendBuf + 2, 0, sizeof(uncaSendBuf) - 2);
		}
	}
	return true;
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

#ifdef MNTRMSG
	if(bSendAll)
		g_MNTRMSG.num_of_order_Reply++;
#endif
	return bSendAll;
}

#ifdef SSLTLS
int password_cb(char *buf, int size, int rwflag, void *password)
{
	strncpy(buf, (char *)(password), size);
	buf[size - 1] = '\0';
	return strlen(buf);
}

void CSKClient::init_openssl()
{
	SSL_load_error_strings();		//registers the error strings for all libssl functions.
	ERR_load_crypto_strings();		//registers the error strings for all libcrypto functions.
	SSL_library_init();
	SSL_CTX *ctx = SSL_CTX_new(TLSv1_2_server_method());
	if (ctx == NULL) {
		printf("errored; unable to load context.\n");
		ERR_print_errors_fp(stderr);
	}
	EVP_PKEY* pkey = generatePrivateKey();
	X509* x509 = generateCertificate(pkey);
	SSL_CTX_use_certificate(ctx, x509);
	//sets the default password callback called when loading/storing a PEM certificate with encryption.
	SSL_CTX_set_default_passwd_cb(ctx, password_cb);
	SSL_CTX_use_PrivateKey(ctx, pkey);
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
	m_ssl = SSL_new(ctx);
}


EVP_PKEY *CSKClient::generatePrivateKey()
{
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
	EVP_PKEY_keygen_init(pctx);
	EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048);
	EVP_PKEY_keygen(pctx, &pkey);
	return pkey;
}

X509 *CSKClient::generateCertificate(EVP_PKEY *pkey)
{
	X509 *x509 = X509_new();
	X509_set_version(x509, 2);
	ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);
	X509_gmtime_adj(X509_get_notBefore(x509), 0);
	X509_gmtime_adj(X509_get_notAfter(x509), (long)60*60*24*365);
	X509_set_pubkey(x509, pkey);

	X509_NAME *name = X509_get_subject_name(x509);
	X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char*)"TW", -1, -1, 0);
	X509_set_issuer_name(x509, name);
	X509_sign(x509, pkey, EVP_md5());
	return x509;
}


#endif

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
