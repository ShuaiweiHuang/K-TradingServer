#include<iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "CVQueueDAOs.h"
#include "CVClient.h"
#include "CVClients.h"
#include "CVNet/CVSocket.h"
#include "CVErrorMessage.h"
#include "../include/CVType.h"
#include "../include/CVGlobal.h"

using namespace std;

typedef long (*FillTandemOrder)(string& strService, char* pIP, map<string, string>& mBranchAccount, union CV_ORDER &cv_order, union CV_TS_ORDER &cv_ts_order, bool bIsProxy);
extern long FillTandemBitcoinOrderFormat(string& strService, char* pIP, map<string, string>& mBranchAccount, union CV_ORDER &cv_order, union CV_TS_ORDER &cv_ts_order, bool bIsProxy);
extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVClient::CCVClient(struct TCVClientAddrInfo &ClientAddrInfo, string strService, bool bIsProxy)
{
	memset(&m_ClientAddrInfo, 0, sizeof(struct TCVClientAddrInfo));
	memcpy(&m_ClientAddrInfo, &ClientAddrInfo, sizeof(struct TCVClientAddrInfo));

	m_ClientStatus = csNone;
	m_strService = strService;
	m_pHeartbeat = NULL;
	pthread_mutex_init(&m_MutexLockOnClientStatus, NULL);
	m_nLengthOfAccountNum = sizeof(struct CV_StructAccnum);
	m_nLengthOfLogonMessage = sizeof(struct CV_StructLogon);
	m_nLengthOfHeartbeatMessage = sizeof(struct CV_StructHeartbeat);
	m_nLengthOfLogoutMessage = sizeof(struct CV_StructLogout);

	if(m_strService.compare("TS") == 0)
	{
		m_nLengthOfOrderMessage = sizeof(struct CV_StructOrder);
	}
	else
	{
		//Keanu : add error message.
	}

	if(bIsProxy)
		m_nLengthOfOrderMessage += (IPLEN);

	m_bIsProxy = bIsProxy;

	Start();
}

CCVClient::~CCVClient()
{
	if( m_pHeartbeat)
	{
		m_pHeartbeat->Terminate();
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	pthread_mutex_destroy(&m_MutexLockOnClientStatus);
}

void* CCVClient::Run()
{
	unsigned char uncaRecvBuf[BUFSIZE];
	unsigned char uncaMessageBuf[MAXDATA];
	unsigned char uncaOverheadMessageBuf[BUFSIZE];
	unsigned char uncaOrder[BUFSIZE];
	unsigned char uncaSendBuf[BUFSIZE];
	struct CV_StructHeartbeat HeartbeatRP;
	struct CV_StructHeartbeat HeartbeatRQ;

	memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
	memset(uncaMessageBuf, 0, sizeof(uncaMessageBuf));
	memset(uncaOverheadMessageBuf, 0, sizeof(uncaOverheadMessageBuf));
	memset(uncaSendBuf, 0, sizeof(uncaSendBuf));
	uncaSendBuf[0] = ESCAPE;// for order reply

	HeartbeatRQ.header_bit[0] = ESCAPE;
	HeartbeatRQ.header_bit[1] = HEARTBEATREQ;
	memcpy(HeartbeatRQ.heartbeat_msg, "HBRQ", 4);

	HeartbeatRP.header_bit[0] = ESCAPE;
	HeartbeatRP.header_bit[1] = HEARTBEATREP;
	memcpy(HeartbeatRP.heartbeat_msg, "HBRP", 4);
	

	int nSizeOfRecvedMessage = 0;
	int nSizeOfOverheadMessage = 0;
	int nSizeOfRecvedCVMessage = 0;

	int nSizeOfCVOrder = 0;
	int nSizeOfTSOrder = 0;
	int nSizeOfRecvSocket = 0;
	int nSizeOfSendSocket = 0;
	int nSizeOfErrorMessage = 0;

	FillTandemOrder fpFillTandemOrder = NULL;
	union CV_ORDER cv_order;
	union CV_TS_ORDER cv_ts_order;

	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	CCVErrorMessage* pErrorMessage = new CCVErrorMessage();

	if(m_strService.compare("TS") == 0)
	{
		fpFillTandemOrder = FillTandemBitcoinOrderFormat;
		nSizeOfCVOrder = sizeof(struct CV_StructOrder);
		nSizeOfTSOrder = sizeof(struct CV_StructTSOrder);
		nSizeOfRecvSocket = sizeof(struct CV_StructOrder);
		nSizeOfSendSocket = sizeof(struct CV_StructOrderReply);
		nSizeOfErrorMessage = 78;
		uncaSendBuf[1] = ORDERREP;//
	}

	int nTimeIntervals = HEARTBEATVAL;
	m_pHeartbeat = new CCVHeartbeat(nTimeIntervals);
	assert(m_pHeartbeat);
	m_pHeartbeat->SetCallback(this);

	if(m_bIsProxy == true)
	{
		SetStatus(csOnline);
		m_pHeartbeat->Start();
	}
	else
		SetStatus(csLogoning);

	while(m_ClientStatus != csOffline)
	{
		while(1)//get message
		{
			if(nSizeOfOverheadMessage > 0)
			{
				memset(uncaMessageBuf, 0, sizeof(uncaMessageBuf));
				memcpy(uncaMessageBuf, uncaOverheadMessageBuf, nSizeOfOverheadMessage);
				nSizeOfRecvedMessage = nSizeOfOverheadMessage;
				nSizeOfOverheadMessage = 0;
			}

			int nIsMessageComplete = IsMessageComplete(uncaMessageBuf, nSizeOfRecvedMessage);

			if(nIsMessageComplete == -1)//to recv again
			{
				memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
				int nRecv = recv(m_ClientAddrInfo.nSocket, uncaRecvBuf, sizeof(uncaRecvBuf), 0);

				if(nRecv > 0)
				{
					FprintfStderrLog("RECV_CV", 0, uncaRecvBuf, nRecv);
					m_pHeartbeat->TriggerGetClientReplyEvent();
					memcpy(uncaMessageBuf + nSizeOfRecvedMessage, uncaRecvBuf, nRecv);
					nSizeOfRecvedMessage += nRecv;
				}
				else if(nRecv == 0)
				{
					FprintfStderrLog("RECV_CV_CLOSE", 0, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), sizeof(m_ClientAddrInfo.caIP));
					SetStatus(csOffline);
					nSizeOfRecvedCVMessage = 0;
					break;
				}
				else if(nRecv == -1)
				{
					FprintfStderrLog("RECV_CV_ERROR", -1, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), sizeof(m_ClientAddrInfo.caIP));
					perror("RECV_SOCKET_ERROR");
					SetStatus(csOffline);
					nSizeOfRecvedCVMessage = 0;
					break;
				}
				else
					FprintfStderrLog("RECV_CV_ELSE", -2, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), sizeof(m_ClientAddrInfo.caIP));
			}
			else if(nIsMessageComplete == 0)//success
			{
				nSizeOfRecvedCVMessage = nSizeOfRecvedMessage;
				nSizeOfRecvedMessage = 0;
				break;
			}
			else if(nIsMessageComplete > 0)//overhead
			{
				nSizeOfRecvedCVMessage = nIsMessageComplete;
				nSizeOfOverheadMessage = nSizeOfRecvedMessage - nSizeOfRecvedCVMessage;

				memset(uncaOverheadMessageBuf, 0, sizeof(uncaOverheadMessageBuf));
				memcpy(uncaOverheadMessageBuf, uncaMessageBuf + nSizeOfRecvedCVMessage, nSizeOfOverheadMessage);
				memset(uncaMessageBuf + nIsMessageComplete, 0, sizeof(uncaMessageBuf) - nIsMessageComplete);
				nSizeOfRecvedMessage = 0;
				break;
			}
			else if(nIsMessageComplete == -2)//wrong data
			{
				FprintfStderrLog("RECV_CV_WRONG", -2, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), 
						 sizeof(m_ClientAddrInfo.caIP), uncaMessageBuf, nSizeOfRecvedMessage);

				bool bFindEscByte = false;

				for(int i=0 ; i<nSizeOfRecvedMessage ; i++)
				{
					if(uncaMessageBuf[i] == ESCAPE)
					{
						nSizeOfRecvedMessage -= i;
						memcpy(uncaMessageBuf, uncaMessageBuf + i, nSizeOfRecvedMessage);

						bFindEscByte = true;
						break;
					}
				}

				if(bFindEscByte == false)
				{
					nSizeOfRecvedMessage = 0;
					memset(uncaMessageBuf, 0, sizeof(uncaMessageBuf));
				}
			}
		}//recv tcp message

		if(nSizeOfRecvedCVMessage > 0)
		{
			if(uncaMessageBuf[1] == LOGREQ)//logon message
			{
				unsigned char uncaSendLogonBuf[MAXDATA];
				FprintfStderrLog("RECV_CV_LOGON", 0, uncaMessageBuf, 2 + sizeof(struct CV_StructLogon));

				if(m_ClientStatus == csLogoning)
				{
					struct CV_StructLogon logon_type;
					bool bLogon;
					memset(&logon_type, 0, sizeof(struct CV_StructLogon));
					memcpy(&logon_type, uncaMessageBuf + 2, sizeof(struct CV_StructLogon));

					struct CV_StructLogonReply logon_reply;
					memset(&logon_reply, 0, sizeof(struct CV_StructLogonReply));
//DISABLE LOGON FUNCTION
#if 0
					bLogon = LogonAuth(logon_type.logon_id, logon_type.password, logon_reply);//logon & get logon reply data
#else
					bLogon = true;
					printf("login success.\n");
#endif
					memset(uncaSendLogonBuf, 0, sizeof(uncaSendLogonBuf));

					uncaSendLogonBuf[0] = ESCAPE;
					uncaSendLogonBuf[1] = LOGREP;

					memcpy(uncaSendLogonBuf + 2, &logon_reply, sizeof(struct CV_StructLogonReply));
					bool bSendData = SendData(uncaSendLogonBuf, sizeof(struct CV_StructLogonReply) + 2);

					if(bSendData == true)
					{
#if 0
						FprintfStderrLog("SEND_LOGON_REPLY", 0, uncaSendLogonBuf, sizeof(struct CV_StructLogonReply) + 2);
#endif
					}
					else
					{
						FprintfStderrLog("SEND_LOGON_REPLY_ERROR", -1, uncaSendLogonBuf, sizeof(struct CV_StructLogonReply) + 2);
						perror("SEND_SOCKET_ERROR");
					}

					U_ByteSint bytesint;
					memset(&bytesint,0,16);

					if(bLogon)//success
					{
						SetStatus(csOnline);
						m_pHeartbeat->Start();

//DISABLE GET ACCOUNT
#if 0
						vector<struct AccountMessage> vAccountMessage;
						unsigned char uncaSendAccountBuf[MAXDATA];
						unsigned char uncaAccountMessageBuf[MAXDATA];//todo
						memset(uncaSendAccountBuf, 0, sizeof(uncaSendAccountBuf));
						memset(uncaAccountMessageBuf, 0, sizeof(uncaAccountMessageBuf));
						uncaSendAccountBuf[0] = ESCAPE;
						uncaSendAccountBuf[1] = ACCLISTREP;
						GetAccount(logon_type.logon_id, logon_type.source, logon_type.Version, vAccountMessage);//to do > 1024

						bytesint.value = vAccountMessage.size();

						memcpy(uncaSendAccountBuf + 2, bytesint.uncaByte, 2);
						bool bSendData = SendData(uncaSendAccountBuf, 4);

						if(bSendData == true)
						{
							FprintfStderrLog("SEND_ACCOUNT_COUNT", 0, uncaSendAccountBuf, 4);
						}
						else
						{
							FprintfStderrLog("SEND_ACCOUNT_COUNT_ERROR", -1, uncaSendAccountBuf, 4);
							perror("SEND_SOCKET_ERROR");
						}

						uncaSendAccountBuf[1] = 0x41;
						int nCountOfSendAccountMessage = 0;
						int nIndexOfSendBuf = 2;
						memset(uncaSendAccountBuf + 2, 0, sizeof(uncaSendAccountBuf) - 2);
						for(vector<struct AccountMessage>::iterator iter = vAccountMessage.begin(); iter != vAccountMessage.end(); iter++)
						{
							memcpy(uncaSendAccountBuf + nIndexOfSendBuf, iter->caMessage, 30);
							nCountOfSendAccountMessage += 1;
							nIndexOfSendBuf += 30;

							if(nCountOfSendAccountMessage == 10)
							{
								bool bSendData = SendData(uncaSendAccountBuf, 300 + 2);
								if(bSendData == true)
								{
									FprintfStderrLog("SEND_ACCOUNT_LIST_TAIL", 0, uncaSendAccountBuf, 300 + 2);
								}
								else
								{
									FprintfStderrLog("SEND_ACCOUNT_LIST_TAIL_ERROR", -1, uncaSendAccountBuf, 300 + 2);
									perror("SEND_SOCKET_ERROR");
								}
								nCountOfSendAccountMessage = 0;
								nIndexOfSendBuf = 2;
								memset(uncaSendAccountBuf + 2, 0, sizeof(uncaSendAccountBuf) - 2);
							}
						}
						if(nCountOfSendAccountMessage > 0 && nCountOfSendAccountMessage < 10)
						{
							bSendData = SendData(uncaSendAccountBuf, nCountOfSendAccountMessage* 30 + 2);
							if(bSendData == true)
							{
								FprintfStderrLog("SEND_ACCOUNT_LIST_HEAD", 0, uncaSendAccountBuf, nCountOfSendAccountMessage*30+2);
							}
							else
							{
								FprintfStderrLog("SEND_ACCOUNT_LIST_HEAD_ERROR", -1, uncaSendAccountBuf, nCountOfSendAccountMessage*30+2);
								perror("SEND_SOCKET_ERROR");
							}
						}
						else
						{}
#endif
					}
					else//logon failed
					{
						FprintfStderrLog("LOGON_ON_FAILED", 0, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), sizeof(m_ClientAddrInfo.caIP));
					}
				}
				else if(m_ClientStatus == csOnline)//repeat logon
				{
#if 0
					struct CV_StructLogonReply logon_reply;
					unsigned char uncaSendRelogBuf[MAXDATA];

					memset(&logon_reply, 0, sizeof(struct CV_StructLogonReply));
					memset(uncaSendRelogBuf, 0, MAXDATA);
					uncaSendRelogBuf[0] = ESCAPE;
					uncaSendRelogBuf[1] = 0x01;

					memcpy(logon_reply.status_code, "7160", 4);//to do
					memcpy(logon_reply.error_code, "M716", 4);//to do
					sprintf(logon_reply.error_message_eng, "Account has logon.");
					memcpy(uncaSendRelogBuf + 2, &logon_reply, sizeof(struct CV_StructLogonReply));

					bool bSendData = SendData(uncaSendRelogBuf, sizeof(struct CV_StructLogonReply) + 2);
					if(bSendData == true)
					{
						FprintfStderrLog("SEND_REPEAT_LOGON", 0, uncaSendRelogBuf, sizeof(struct CV_StructLogonReply) + 2);
					}
					else
					{
						FprintfStderrLog("SEND_REPEAT_LOGON_ERROR", -1, uncaSendRelogBuf, sizeof(struct CV_StructLogonReply) + 2);
						perror("SEND_SOCKET_ERROR");
					}
#endif
				}
				else
				{
					//todo
				}
			}
			else if(uncaMessageBuf[1] == HeartbeatRQ.header_bit[1])//heartbeat message
			{
				if(memcmp(uncaMessageBuf + 2, HeartbeatRQ.heartbeat_msg, 4) == 0)
				{
					FprintfStderrLog("RECV_CV_HBRQ", 0, NULL, 0);

					bool bSendData = SendData((unsigned char*)&HeartbeatRP, sizeof(HeartbeatRP));
					if(bSendData == true)
					{
						FprintfStderrLog("SEND_CV_HBRP", 0, (unsigned char*)&HeartbeatRP, sizeof(HeartbeatRP));
					}
					else
					{
						FprintfStderrLog("SEND_CV_HBRP_ERROR", -1, (unsigned char*)&HeartbeatRP, sizeof(HeartbeatRP));
						perror("SEND_SOCKET_ERROR");
					}
				}
				else
				{
					FprintfStderrLog("RECV_CV_HBRQ_ERROR", -1, uncaMessageBuf + 2, 4);
				}
			}
			else if(uncaMessageBuf[1] == HeartbeatRP.header_bit[1])//heartbeat message
			{
				if(memcmp(uncaMessageBuf + 2, HeartbeatRP.heartbeat_msg, 4) == 0)
				{
					FprintfStderrLog("RECV_CV_HBRP", 0, NULL, 0);
				}
				else
				{
					FprintfStderrLog("RECV_CV_HBRP_ERROR", -1, uncaMessageBuf + 2, 4);
				}
			}
			else if(uncaMessageBuf[1] == DISCONNMSG)
			{
				SetStatus(csOffline);
				FprintfStderrLog("RECV_CV_DISCONNECT", 0, 0, 0);
				break;
			}
			else if(uncaMessageBuf[1] == ORDERREQ)
			{
				if(m_bIsProxy)
					nSizeOfRecvedCVMessage -= (IPLEN);//ip
				FprintfStderrLog("RECV_CV_ORDER", 0, uncaMessageBuf, nSizeOfRecvedCVMessage);

				if(m_ClientStatus == csLogoning)
				{
					U_ByteSint bytesint;
					char caErrorMessage[] = "caErrorMessage";
					memset(uncaSendBuf + 2, 0, sizeof(uncaSendBuf) - 2);
					memcpy(uncaSendBuf + 2 ,&cv_order, nSizeOfRecvedCVMessage - 2);
					memcpy(uncaSendBuf + 2 + nSizeOfRecvedCVMessage - 2, caErrorMessage, strlen(caErrorMessage));
					memset(&bytesint,0,16);
					bytesint.value = 1099;
					memcpy(uncaSendBuf + 2 + nSizeOfRecvedCVMessage - 2 + nSizeOfErrorMessage, bytesint.uncaByte, 2);
					bool bSendData = SendData(uncaSendBuf, 2 + nSizeOfRecvedCVMessage - 2 + nSizeOfErrorMessage + 2);

					if(bSendData == true)
					{
						FprintfStderrLog("SEND_LOGON_FIRST", 0, uncaMessageBuf, 2 + nSizeOfRecvedCVMessage - 2 + nSizeOfErrorMessage + 2);
					}
					else
					{
						FprintfStderrLog("SEND_LOGON_FIRST_ERROR", -1, uncaMessageBuf, 2 + nSizeOfRecvedCVMessage - 2 + nSizeOfErrorMessage + 2);
						perror("SEND_SOCKET_ERROR");
					}
					continue;
				}

				CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();

				if(pQueueDAO)
				{
					memset(&cv_order, 0, sizeof(union CV_ORDER));
					memcpy(&cv_order, uncaMessageBuf, nSizeOfCVOrder);
					memset(&cv_ts_order, 0, sizeof(union CV_TS_ORDER));
					long lOrderNumber = 0;
					lOrderNumber = fpFillTandemOrder(m_strService, m_ClientAddrInfo.caIP,
									m_mBranchAccount, cv_order, cv_ts_order, m_bIsProxy ? true : false);
					printf("OrderNumber = %d\n" ,lOrderNumber);

					if(lOrderNumber < 0)//error
					{
						U_ByteSint bytesint;
						memset(&bytesint,0,16);
						memset(uncaSendBuf , 0, sizeof(uncaSendBuf) );
						memcpy(uncaSendBuf ,&cv_order, nSizeOfRecvedCVMessage);
						memcpy(uncaSendBuf + nSizeOfRecvedCVMessage, pErrorMessage->GetErrorMessage(lOrderNumber),
							strlen(pErrorMessage->GetErrorMessage(lOrderNumber)));
						bytesint.value = -lOrderNumber;
						memcpy(uncaSendBuf + nSizeOfRecvedCVMessage + nSizeOfErrorMessage, bytesint.uncaByte, 2);

						int nSendData = SendData(uncaSendBuf, nSizeOfRecvedCVMessage + nSizeOfErrorMessage);

						if(nSendData)
						{
							FprintfStderrLog("SEND_FILL_TANDEM_ORDER_CODE", -lOrderNumber, uncaSendBuf, nSizeOfRecvedCVMessage + nSizeOfErrorMessage + 2);
						}
						else
						{
							FprintfStderrLog("SEND_FILL_TANDEM_ORDER_CODE_ERROR", lOrderNumber, uncaSendBuf, nSizeOfRecvedCVMessage + nSizeOfErrorMessage + 2);
							perror("SEND_SOCKET_ERROR");
						}
						continue;
					}

					try
					{
						if(pClients->GetClientFromHash(lOrderNumber) != NULL)
						{
							throw "Inuse";
						}
						else
						{
							pClients->InsertClientToHash(lOrderNumber, this);
						}
					}
					catch(char const* pExceptionMessage)
					{
						cout << "OrderNumber = " << lOrderNumber << " " << pExceptionMessage << endl;
					}

					memset(uncaOrder, 0, sizeof(uncaOrder));
					memcpy(uncaOrder, &cv_ts_order, nSizeOfTSOrder);
					int nResult = pQueueDAO->SendData(uncaOrder, nSizeOfTSOrder);


					if(nResult == 0)
					{
						FprintfStderrLog("SEND_Q", 0, uncaOrder, nSizeOfTSOrder);
						struct CVOriginalOrder newOriginalOrder;
						memset(newOriginalOrder.uncaBuf, 0, sizeof(newOriginalOrder.uncaBuf));
						memcpy(newOriginalOrder.uncaBuf, &cv_order, nSizeOfCVOrder);
						m_mOriginalOrder.insert(std::pair<long, struct CVOriginalOrder>(lOrderNumber, newOriginalOrder));
					}
					else if(nResult == -1)
					{
						FprintfStderrLog("SEND_Q_ERROR", -1, uncaOrder, nSizeOfTSOrder);
						pClients->RemoveClientFromHash(lOrderNumber);
						perror("SEND_Q_ERROR");
					}
				}
				else
				{
					FprintfStderrLog("GET_QDAO_ERROR", -1, uncaOrder, nSizeOfTSOrder);
				}
			}
		}
	}
	delete pErrorMessage;
	m_pHeartbeat->Terminate();
	pClients->MoveOnlineClientToOfflineVector(this);
	return NULL;
}

bool CCVClient::SendData(const unsigned char* pBuf, int nSize)
{
	return SendAll(pBuf, nSize);
}

bool CCVClient::SendAll(const unsigned char* pBuf, int nToSend)
{
	int nSend = 0;
	int nSended = 0;

	do
	{
		nToSend -= nSend;

		nSend = send(m_ClientAddrInfo.nSocket, pBuf + nSended, nToSend, 0);

		if(nSend == -1)
		{
			break;
		}

		if(nSend < nToSend)
		{
			nSended += nSend;
		}
		else
		{
			break;
		}
	}
	while(nSend != nToSend);

	return nSend == nToSend ? true : false;
}

void CCVClient::SetStatus(TCVClientStauts csStatus)
{
	pthread_mutex_lock(&m_MutexLockOnClientStatus);//lock

	m_ClientStatus = csStatus;

	pthread_mutex_unlock(&m_MutexLockOnClientStatus);//unlock
}

TCVClientStauts CCVClient::GetStatus()
{
	return m_ClientStatus;
}

void CCVClient::GetOriginalOrder(long nOrderNumber, int nOrderSize, union CV_ORDER_REPLY &cv_order_reply)
{	
	memcpy(&cv_order_reply, m_mOriginalOrder[nOrderNumber].uncaBuf, nOrderSize);
}

bool CCVClient::LogonAuth(char* pID, char* ppassword, struct CV_StructLogonReply &logon_reply)
{
	CCVSocket* pSocket = new CCVSocket();
	pSocket->Connect("pass.cryptovix.com.tw", "80");

	char caBuf[MAXDATA];
	unsigned char uncaSendBuf[MAXDATA];
	unsigned char uncaRecvBuf[MAXDATA];

	memset(caBuf, 0, MAXDATA);
	memset(uncaSendBuf, 0, MAXDATA);
	memset(uncaRecvBuf, 0, MAXDATA);

	char pass_ip[50];
	memset(pass_ip, 0, 50);
	sprintf(pass_ip, "pass.cryptovix.com.tw");

	char cust_id[11];
	memset(cust_id, 0,11);
	memcpy(cust_id, pID, 10);

	char en_id_pass[50];
	memset(en_id_pass, 0, 50);
	memcpy(en_id_pass, ppassword, 50);

	sprintf(caBuf,"GET http://%s/PasswordGW/Gateway.asp?IDNO=%s&Func=2&PswdType=0&Password=%s&IP=%s HTTP/1.0\n\n",
			pass_ip, cust_id, en_id_pass, m_ClientAddrInfo.caIP);

	memcpy(uncaSendBuf, caBuf ,MAXDATA);

	pSocket->Send(uncaSendBuf, strlen(caBuf));

	pSocket->Recv(uncaRecvBuf,MAXDATA);
	cout << uncaRecvBuf << endl;
	delete pSocket;


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
		memcpy(logon_reply.error_code, "M999", 4);
		sprintf(logon_reply.error_message_eng, "Check PASS return error#");
		return false;
	}

	pFirstToken = strstr(caRecvBuf, "\r\n\r\n");

	if(pFirstToken == NULL)
	{
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
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M000", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return true;
		}
		else if (strncmp(castatus_code, "7997", 4) == 0)
		{ /* first login */ 
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M151", 4);
			sprintf(logon_reply.error_message_eng, "±K½X¿ù»~#");
			return false;
		}
		else if (strncmp(castatus_code, "7996", 4) == 0)
		{ /* first login */
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M155", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else if(strncmp(castatus_code, "7993", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M156", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			//SetStatus(csOnline);
			return true;
		}
		else if(strncmp(castatus_code, "7992", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M156", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			//SetStatus(csOnline);
			return true;
		}
		else if (strncmp(castatus_code, "7998", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M152", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else if (strncmp(castatus_code, "1999", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M999", 4);
			sprintf(logon_reply.error_message_eng, "M999 pass server busy#");
			return false;
		}
		else if (strncmp(castatus_code, "8992", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M153", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else if (strncmp(castatus_code, "8994", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M153", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else if (strncmp(castatus_code, "8995", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M153", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else if (strncmp(castatus_code, "8996", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M153", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else if (strncmp(castatus_code, "8997", 4) == 0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M153", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else if (strncmp(castatus_code, "8998", 4)==0)
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M153", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
		else
		{
			memcpy(logon_reply.status_code, castatus_code, 4);
			memcpy(logon_reply.error_code, "M153", 4);
			memcpy(logon_reply.error_message_eng, caHttpMessage, sizeof(logon_reply.error_message_eng));
			return false;
		}
	}
}

void CCVClient::GetAccount(char* pID, char* psource, char* pVersion, vector<struct AccountMessage> &vAccountMessage)
{
	CCVSocket* pSocket = new CCVSocket();
	pSocket->Connect("aptrade.capital.com.tw", "4000");

	unsigned char uncaSendBuf[MAXDATA];
	char caSendBuf[MAXDATA];
	unsigned char uncaRecvBuf[MAXDATA];
	vector<struct AccountRecvBuf> vAccountRecvBuf;
	char* caRecvBuf = NULL;

	sprintf(caSendBuf, "B real_no_pass 01000 %10.10s %c %c 0 0 0 0##", pID, psource, pVersion);
	memset(uncaSendBuf, 0, sizeof(uncaSendBuf));
	memcpy(uncaSendBuf, caSendBuf, MAXDATA);
	pSocket->Send(uncaSendBuf, strlen(caSendBuf));


	int nRecv = 0;
	int nRecved = 0;
	do
	{
		struct AccountRecvBuf account_recv_buf;
		memset(&account_recv_buf, 0, sizeof(struct AccountRecvBuf));

		nRecv = pSocket->Recv(account_recv_buf.uncaRecvBuf, MAXDATA);
		if(nRecv > 0)
		{
			account_recv_buf.nRecved = nRecv;
			vAccountRecvBuf.push_back(account_recv_buf);
			nRecved += nRecv;
		}
	}
	while(nRecv != 0);

	caRecvBuf = new char[nRecved];

	memset(caRecvBuf, 0, nRecved);

	int nIndex = 0;
	for(vector<struct AccountRecvBuf>::iterator iter = vAccountRecvBuf.begin(); iter != vAccountRecvBuf.end(); iter++)
	{
		memcpy(caRecvBuf + nIndex, iter->uncaRecvBuf, iter->nRecved);
		nIndex += iter->nRecved;
	}
	delete pSocket;

	vector<struct AccountItem> vAccountItem;
	
	char* pch = NULL;
	pch = strtok(caRecvBuf, "#");
	while(pch != NULL)
	{
		fprintf(stderr, "%s\n",pch);
		if(strstr(pch, m_strService.c_str()))
		{
			struct AccountItem account_item;
			memset(&account_item, 0, sizeof(struct AccountItem));

			strcpy(account_item.caItem, pch);
			vAccountItem.push_back(account_item);
		}
		pch = strtok(NULL, "#");
	}
	delete [] caRecvBuf;

	char caItem[10][128];
	int nItemCount = 0;
	for(vector<struct AccountItem>::iterator iter = vAccountItem.begin(); iter != vAccountItem.end(); iter++)
	{
		memset(caItem, 0, sizeof(caItem));

		pch = strtok(iter->caItem, ",");
		while(pch != NULL)
		{
			strcpy(caItem[nItemCount], pch);
			nItemCount++;
			pch = strtok(NULL, ",");
		}
		nItemCount = 0;

		struct AccountMessage account_message;
		memset(&account_message, 0, sizeof(struct AccountMessage));

		string strBranch(caItem[2]);
		memcpy(account_message.caMessage, caItem[2], 10);

		string strAccount(caItem[3]);
		memcpy(account_message.caMessage + 10, caItem[3], 10);
		if(m_strService.compare("OS") == 0)
		{
			if(strlen(caItem[3]) == 7)
			{
				strAccount.insert(0, 1, '0');
				memcpy(account_message.caMessage + 11, account_message.caMessage + 10, 7);
				memset(account_message.caMessage + 10, '0', 1);
			}
		}

		string strSubAccount(caItem[5]);
		memcpy(account_message.caMessage + 20, caItem[5], 10);
		if(m_strService.compare("OS") == 0)
		{
			if(strlen(caItem[5]) == 7)
			{
				strSubAccount.insert(0, 1, '0');
				memcpy(account_message.caMessage + 21, account_message.caMessage + 20, 7);
				memset(account_message.caMessage + 20, '0', 1);
			}
		}

		vAccountMessage.push_back(account_message);

		string strBranchAccount = m_strService + strBranch + strAccount + strSubAccount;
		m_mBranchAccount.insert(std::pair<string, string>(strBranchAccount, strBranchAccount));
	}
}

int CCVClient::GetClientSocket()
{
	return m_ClientAddrInfo.nSocket;
}

// 0 -> complete, -1 -> incomplete, nLength -> overhead, -2 -> wrong data
int CCVClient::IsMessageComplete(unsigned char* pBuf, int nRecvedSize)
{
	if(nRecvedSize == 0)
		return -1;

	//Logon
	if(pBuf[1] == LOGREQ) {
		if(nRecvedSize == m_nLengthOfLogonMessage)
			return 0;
		else if(nRecvedSize < m_nLengthOfLogonMessage)
			return -1;
		else if(nRecvedSize > m_nLengthOfLogonMessage)
			return m_nLengthOfLogonMessage;
	}
	//Get Account Number
	else if(pBuf[1] == ACCNUMREQ || pBuf[1] == ACCLISTREQ) {
		if(nRecvedSize == m_nLengthOfAccountNum)
			return 0;
		else if(nRecvedSize < m_nLengthOfAccountNum)
			return -1;
		else if(nRecvedSize > m_nLengthOfAccountNum)
			return m_nLengthOfAccountNum;
	}
	// Heart Beat
	else if(pBuf[1] == HEARTBEATREQ || pBuf[1] == HEARTBEATREP) {
		if(nRecvedSize == m_nLengthOfHeartbeatMessage)
			return 0;
		else if(nRecvedSize < m_nLengthOfHeartbeatMessage)
			return -1;
		else if(nRecvedSize > m_nLengthOfHeartbeatMessage)
			return m_nLengthOfHeartbeatMessage;
	}
	//Order
	else if(pBuf[1] == ORDERREQ) {
		if(nRecvedSize == m_nLengthOfOrderMessage)
			return 0;
		else if(nRecvedSize < m_nLengthOfOrderMessage)
			return -1;
		else if(nRecvedSize > m_nLengthOfOrderMessage)
			return m_nLengthOfOrderMessage;
	}//DISCONNMSG
	else if(pBuf[1] == DISCONNMSG) {
		if(nRecvedSize == m_nLengthOfLogoutMessage)
			return 0;
		else if(nRecvedSize < m_nLengthOfLogoutMessage)
			return -1;
		else if(nRecvedSize > m_nLengthOfLogoutMessage)
			return m_nLengthOfLogoutMessage;
	}
	else {
		FprintfStderrLog("RECV_ERROR_HEADER", 0, pBuf+1, 1);
		printf("Error header:%x\n", pBuf[1]);
		exit(-1);
		return -2;
	}
}
