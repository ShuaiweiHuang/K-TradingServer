#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iomanip>

#include "CVQueueDAOs.h"
#include "CVClient.h"
#include "CVClients.h"
#include "CVNet/CVSocket.h"
#include "CVErrorMessage.h"
#include "../include/CVType.h"
#include "../include/CVGlobal.h"

using json = nlohmann::json;
using namespace std;

typedef long (*FillTandemOrder)(string& strService, char* pIP, map<string, string>& mBranchAccount, union CV_ORDER &cv_order, union CV_TS_ORDER &cv_ts_order);
extern long FillTandemBitcoinOrderFormat(string& strService, char* pIP, map<string, string>& mBranchAccount, union CV_ORDER &cv_order, union CV_TS_ORDER &cv_ts_order);
extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVClient::CCVClient(struct TCVClientAddrInfo &ClientAddrInfo, string strService)
{
	memset(&m_ClientAddrInfo, 0, sizeof(struct TCVClientAddrInfo));
	memcpy(&m_ClientAddrInfo, &ClientAddrInfo, sizeof(struct TCVClientAddrInfo));

	m_ClientStatus = csNone;
	m_strService = strService;
	m_pHeartbeat = NULL;
	pthread_mutex_init(&m_MutexLockOnClientStatus, NULL);
	m_nLengthOfAccountNum = sizeof(struct CV_StructAccnum);
	m_nLengthOfLogonMessage = sizeof(struct CV_StructLogon);
	m_nLengthOfLogonReplyMessage = sizeof(struct CV_StructLogonReply);
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
	unsigned char uncaEscapeBuf[2];
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

	int nSizeOfRecvedCVMessage = 0;	
	int nSizeOfCVOrder = 0;
	int nSizeOfTSOrder = 0;
	int nSizeOfRecvSocket = 0;
	int nSizeOfSendSocket = 0;
	int nSizeOfErrorMessage = 0;
	int nTimeIntervals = HEARTBEATVAL;
	int nToRecv; 

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
		nSizeOfErrorMessage = sizeof(struct CV_StructTSOrderReply);
		uncaSendBuf[1] = ORDERREP;
	}

	m_pHeartbeat = new CCVHeartbeat(nTimeIntervals);
	assert(m_pHeartbeat);
	m_pHeartbeat->SetCallback(this);

	SetStatus(csLogoning);

	while(m_ClientStatus != csOffline)
	{
		memset(uncaEscapeBuf, 0, sizeof(uncaEscapeBuf));

		bool bRecvAll = RecvAll(uncaEscapeBuf, 2);

		if(bRecvAll == false)
		{
			FprintfStderrLog("RECV_ESC_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID), uncaEscapeBuf, sizeof(uncaEscapeBuf));
			continue;
		}

		if(uncaEscapeBuf[0] != ESCAPE)
		{
			FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID));
			printf("Error byte = %x\n", uncaEscapeBuf[0]);
			continue;
		}
		else
		{
			nToRecv = 0;
			switch(uncaEscapeBuf[1])
			{
				case LOGREQ:
					nToRecv = sizeof(struct CV_StructLogon)-2;
			printf("\nlogin nToRecv = %d\n", nToRecv);
					break;
				case HEARTBEATREQ:
					nToRecv = sizeof(struct CV_StructHeartbeat)-2;
			printf("\nhbrq nToRecv = %d\n", nToRecv);
					break;
				case HEARTBEATREP:
					nToRecv = sizeof(struct CV_StructHeartbeat)-2;
			printf("\nhbrp nToRecv = %d\n", nToRecv);
					break;
				case ORDERREQ:
					nToRecv = sizeof(struct CV_StructOrder)-2;
			printf("\norder nToRecv = %d\n", nToRecv);
					break;
				case DISCONNMSG:
					SetStatus(csOffline);
			printf("\ndisconnect nToRecv = %d\n", nToRecv);
					break;
				default:
					FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID));
					printf("\nError byte = %x\n", uncaEscapeBuf[1]);
					continue;
			}
			if(m_ClientStatus == csOffline)
				break;

			memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
			bRecvAll = RecvAll(uncaRecvBuf, nToRecv);
			if(bRecvAll == false)
			{
				FprintfStderrLog("RECV_TRAIL_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID), uncaRecvBuf, nToRecv);
				continue;
			}
		}

		memcpy(uncaMessageBuf, uncaEscapeBuf, 2);
		memcpy(uncaMessageBuf+2, uncaRecvBuf, nToRecv);
		nSizeOfRecvedCVMessage = 2 + nToRecv;	
		if(nToRecv > 0)
		{
			if(uncaMessageBuf[1] == LOGREQ)//logon message
			{
				unsigned char uncaSendLogonBuf[MAXDATA];
				
				FprintfStderrLog("RECV_CV_LOGON", 0, uncaMessageBuf, sizeof(struct CV_StructLogon));

				if(m_ClientStatus == csLogoning)
				{
					struct CV_StructLogon logon_type;
					bool bLogon;
					memset(&logon_type, 0, sizeof(struct CV_StructLogon));
					memcpy(&logon_type, uncaMessageBuf, sizeof(struct CV_StructLogon));

					struct CV_StructLogonReply logon_reply;
					memset(&logon_reply, 0, m_nLengthOfLogonReplyMessage);
					memset(m_uncaLogonID, 0, sizeof(m_uncaLogonID));
					memcpy(m_uncaLogonID, logon_type.logon_id, sizeof(logon_type.logon_id));
					bLogon = LogonAuth(logon_type.logon_id, logon_type.password, logon_reply);//logon & get logon reply data
					memset(uncaSendLogonBuf, 0, sizeof(uncaSendLogonBuf));
					logon_reply.header_bit[0] = ESCAPE;
					logon_reply.header_bit[1] = LOGREP;
					uncaSendLogonBuf[0] = ESCAPE;
					uncaSendLogonBuf[1] = LOGREP;
					memcpy(uncaSendLogonBuf, &logon_reply.header_bit[0], m_nLengthOfLogonReplyMessage);
					bool bSendData = SendData(uncaSendLogonBuf, m_nLengthOfLogonReplyMessage);

					if(bSendData == true)
					{
						FprintfStderrLog("SEND_LOGON_REPLY", 0, uncaSendLogonBuf, m_nLengthOfLogonReplyMessage);
					}
					else
					{
						FprintfStderrLog("SEND_LOGON_REPLY_ERROR", -1, uncaSendLogonBuf, m_nLengthOfLogonReplyMessage);
						perror("SEND_SOCKET_ERROR");
					}

					U_ByteSint bytesint;
					memset(&bytesint,0,16);

					if(bLogon)//success
					{
						SetStatus(csOnline);
						m_pHeartbeat->Start();
					}
					else//logon failed
					{
						FprintfStderrLog("LOGON_ON_FAILED", 0, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), sizeof(m_ClientAddrInfo.caIP));
					}
				}
				else if(m_ClientStatus == csOnline)//repeat logon
				{
					struct CV_StructLogonReply logon_reply;
					unsigned char uncaSendRelogBuf[MAXDATA];

					memset(&logon_reply, 0, m_nLengthOfLogonReplyMessage);
					memset(uncaSendRelogBuf, 0, MAXDATA);
					logon_reply.header_bit[0] = ESCAPE;
					logon_reply.header_bit[1] = 0x01;
					memcpy(logon_reply.status_code, "NG", 2);//to do
					memcpy(logon_reply.backup_ip, BACKUP_IP, 15);
					memcpy(logon_reply.error_code, "02", 2);//to do
					sprintf(logon_reply.error_message, "Account has logon.");
					memcpy(uncaSendRelogBuf, &logon_reply.header_bit[0], m_nLengthOfLogonReplyMessage);

					bool bSendData = SendData(uncaSendRelogBuf, m_nLengthOfLogonReplyMessage);
					if(bSendData == true)
					{
						FprintfStderrLog("SEND_REPEAT_LOGON", 0, uncaSendRelogBuf, m_nLengthOfLogonReplyMessage);
					}
					else
					{
						FprintfStderrLog("SEND_REPEAT_LOGON_ERROR", -1, uncaSendRelogBuf, m_nLengthOfLogonReplyMessage);
						perror("SEND_SOCKET_ERROR");
					}
				}
				else
				{
					break;
				}
			}
			else if(uncaMessageBuf[1] == HEARTBEATREQ)//heartbeat message
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
			else if(uncaMessageBuf[1] == HEARTBEATREP)//heartbeat message
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
				FprintfStderrLog("RECV_CV_ORDER", 0, uncaMessageBuf, nSizeOfRecvedCVMessage);

				if(m_ClientStatus == csLogoning)
				{
					struct CV_StructOrderReply replymsg;
					int errorcode = -LG_ERROR;
					memset(&replymsg, 0, sizeof(struct CV_StructOrderReply));
					replymsg.header_bit[0] = 0x1b;
					replymsg.header_bit[1] = ORDERREP;

					memcpy(&replymsg.original, &cv_order, nSizeOfCVOrder);
					sprintf((char*)&replymsg.error_code, "%.4d", errorcode);

					memcpy(&replymsg.reply_msg, pErrorMessage->GetErrorMessage(LG_ERROR),
						strlen(pErrorMessage->GetErrorMessage(LG_ERROR)));

					int nSendData = SendData((unsigned char*)&replymsg, sizeof(struct CV_StructOrderReply));

					if(nSendData)
					{
						FprintfStderrLog("SEND_LOGON_FIRST", 0, (unsigned char*)&replymsg, sizeof(struct CV_StructOrderReply));
					}
					else
					{
						FprintfStderrLog("SEND_LOGON_FIRST_ERROR", -1, (unsigned char*)&replymsg, sizeof(struct CV_StructOrderReply));
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
									m_mBranchAccount, cv_order, cv_ts_order);

					if(lOrderNumber < 0)//error
					{
						int errorcode = -lOrderNumber;
						struct CV_StructOrderReply replymsg;

						memset(&replymsg, 0, sizeof(struct CV_StructOrderReply));
						replymsg.header_bit[0] = 0x1b;
						replymsg.header_bit[1] = ORDERREP;

						memcpy(&replymsg.original, &cv_order, nSizeOfCVOrder);
						sprintf((char*)&replymsg.error_code, "%.4d", errorcode);

						memcpy(&replymsg.reply_msg, pErrorMessage->GetErrorMessage(lOrderNumber),
							strlen(pErrorMessage->GetErrorMessage(lOrderNumber)));

						int nSendData = SendData((unsigned char*)&replymsg, sizeof(struct CV_StructOrderReply));
						if(nSendData)
						{
							FprintfStderrLog("SEND_FILL_TANDEM_ORDER_CODE", -lOrderNumber, (unsigned char*)&replymsg, sizeof(struct CV_StructOrderReply));
						}
						else
						{
							FprintfStderrLog("SEND_FILL_TANDEM_ORDER_CODE_ERROR", lOrderNumber, (unsigned char*)&replymsg, sizeof(struct CV_StructOrderReply));
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
		}//recv tcp message
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

bool CCVClient::RecvAll(unsigned char* pBuf, int nToRecv)
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
				m_pHeartbeat->TriggerGetClientReplyEvent();
			else
				FprintfStderrLog("HEARTBEAT_NULL_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID));

			FprintfStderrLog(NULL, 0, m_uncaLogonID, sizeof(m_uncaLogonID), pBuf + nRecved, nRecv);
			nRecved += nRecv;
		}
		else if(nRecv == 0)
		{
			SetStatus(csOffline);
			FprintfStderrLog("RECV_CV_CLOSE", 0, m_uncaLogonID, sizeof(m_uncaLogonID));
			break;
		}
		else if(nRecv == -1)
		{
			SetStatus(csOffline);
			FprintfStderrLog("RECV_CV_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID), (unsigned char*)strerror(errno), strlen(strerror(errno)));
			break;
		}
		else
		{
			SetStatus(csOffline);
			FprintfStderrLog("RECV_CV_ELSE_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID), (unsigned char*)strerror(errno), strlen(strerror(errno)));
			break;
		}
	}
	while(nRecv != nToRecv);

	return nRecv == nToRecv ? true : false;
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
	memcpy(&cv_order_reply.cv_reply.original, m_mOriginalOrder[nOrderNumber].uncaBuf, nOrderSize);
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
}

bool CCVClient::LogonAuth(char* pID, char* ppassword, struct CV_StructLogonReply &logon_reply)
{
	json jtable;
	json jtable_exname;
	CURLcode res;
	string readBuffer1, readBuffer2, acno, exno, exname;
	CURL *curl = curl_easy_init();

	if(curl) {
		char query_str[512];
		sprintf(query_str, "http://192.168.101.209:19487/mysql?query=select%%20accounting_no,exchange_no%%20from%%20employee,accounting%20where%%20account%20=%20%27%s%%27%%20and%%20password%%20=%%20%%27%s%%27%%20and%%20accounting.trader_no=employee.trader_no", pID, ppassword);
		printf("================\n%s\n===============\n", query_str);
		curl_easy_setopt(curl, CURLOPT_URL, query_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer1);
		res = curl_easy_perform(curl);
		jtable = json::parse(readBuffer1.c_str());

		if(jtable.size() == 0) {
			memcpy(logon_reply.status_code, "NG", 2);//to do
			memcpy(logon_reply.backup_ip, BACKUP_IP, 15);
			memcpy(logon_reply.error_code, "01", 2);
			sprintf(logon_reply.error_message, "login authentication fail");
			return false;
		}

		for(int i=0 ; i<jtable.size() ; i++) {
			readBuffer2 = "";
			acno = to_string(jtable[i]["accounting_no"]);
			exno = to_string(jtable[i]["exchange_no"]);
			acno = acno.substr(1, 7);
			exno = exno.substr(1, 7);
			sprintf(query_str, "http://192.168.101.209:19487/mysql?query=select%%20exchange_name_en%%20from%%20exchange%%20where%%20exchange_no%%20=%%20%%27%s%%27", exno.c_str());
			printf("================\n%s\n===============\n", query_str);
			curl_easy_setopt(curl, CURLOPT_URL, query_str);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer2);
			res = curl_easy_perform(curl);
			printf("%s\n", readBuffer2.c_str());
			jtable_exname = json::parse(readBuffer2.c_str());
			exname = to_string(jtable_exname[0]["exchange_name_en"]);
			exname = exname.substr(1,exname.length()-1);
			m_mBranchAccount.insert(pair<string, string>(acno, exname));
			cout << acno <<", " << exno << ", " << exname << endl;
		}
			memcpy(logon_reply.status_code, "OK", 2);//to do
			memcpy(logon_reply.backup_ip, BACKUP_IP, 15);
			memcpy(logon_reply.error_code, "00", 2);
			sprintf(logon_reply.error_message, "login success");
		curl_easy_cleanup(curl);
		return true;
	}
	memcpy(logon_reply.status_code, "NG", 2);//to do
	memcpy(logon_reply.backup_ip, BACKUP_IP, 15);
	memcpy(logon_reply.error_code, "01", 2);
	sprintf(logon_reply.error_message, "cannot accesss authentication server");
	return false;

}

int CCVClient::GetClientSocket()
{
	return m_ClientAddrInfo.nSocket;
}
