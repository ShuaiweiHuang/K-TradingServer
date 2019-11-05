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
#include <algorithm>
#include <openssl/hmac.h>

#include "CVQueueDAOs.h"
#include "CVClient.h"
#include "CVClients.h"
#include "CVNet/CVSocket.h"
#include "CVErrorMessage.h"
#include "../include/CVType.h"
#include "../include/CVGlobal.h"

using json = nlohmann::json;
using namespace std;

typedef long (*FillTandemOrder)(string& strService, char* pUsername, char* pIP, map<string, struct AccountData>& mBranchAccount, union CV_ORDER &cv_order, union CV_TS_ORDER &cv_ts_order);
extern long FillTandemBitcoinOrderFormat(string& strService, char* pUsername, char* pIP, map<string, struct AccountData>& mBranchAccount, union CV_ORDER &cv_order, union CV_TS_ORDER &cv_ts_order);
extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

int CCVClient::HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length)
{
	const EVP_MD * engine = EVP_sha256();
	output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key, strlen(key), engine, NULL);
	HMAC_Update(&ctx, (unsigned char*)input, strlen(input));
	HMAC_Final(&ctx, output, &output_length);
	HMAC_CTX_cleanup(&ctx);
	return 0;
}

CCVClient::CCVClient(struct TCVClientAddrInfo &ClientAddrInfo, string strService)
{
	memset(&m_ClientAddrInfo, 0, sizeof(struct TCVClientAddrInfo));
	memcpy(&m_ClientAddrInfo, &ClientAddrInfo, sizeof(struct TCVClientAddrInfo));

	m_ClientStatus = csNone;
	m_strService = strService;
	m_pHeartbeat = NULL;
	pthread_mutex_init(&m_MutexLockOnClientStatus, NULL);
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
		FprintfStderrLog("SERVICE_TYPE_ERROR", -1, 0, 0, 0, 0);
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
					break;
				case HEARTBEATREQ:
					nToRecv = sizeof(struct CV_StructHeartbeat)-2;
					break;
				case HEARTBEATREP:
					nToRecv = sizeof(struct CV_StructHeartbeat)-2;
					break;
				case ORDERREQ:
					nToRecv = sizeof(struct CV_StructOrder)-2;
					break;
				case DISCONNMSG:
					FprintfStderrLog("RECV_CV_DISCONNECT", 0, 0, 0);
					SetStatus(csOffline);
					break;
				default:
					FprintfStderrLog("ESCAPE_BYTE_ERROR", -1, m_uncaLogonID, sizeof(m_uncaLogonID));
					printf("\nError byte = %x\n", uncaEscapeBuf[1]);
					continue;
			}
			if(m_ClientStatus == csOffline)
				break;

			memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));

			if(nToRecv)
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

		if(nToRecv >= 0)
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
					memcpy(m_username, logon_type.logon_id, 20);
					
					FprintfStderrLog("LOGON_MESSAGE", 0, (unsigned char*)logon_type.logon_id, strlen(logon_type.logon_id),
							(unsigned char*)logon_type.password, strlen(logon_type.password));

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

					if(bLogon)//success
					{
						SetStatus(csOnline);
						ReplyAccountNum();
						LoadRiskControl(logon_type.logon_id);
						m_pHeartbeat->Start();
					}
					else//logon failed
					{
						SetStatus(csOffline);
						FprintfStderrLog("LOGON_ON_FAILED", 0, reinterpret_cast<unsigned char*>(m_ClientAddrInfo.caIP), sizeof(m_ClientAddrInfo.caIP));
						break;
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
			
			else if(uncaMessageBuf[1] == ORDERREQ)
			{
#ifdef DEBUG
				printf("receive ORDERREQ\n\n\n");
#endif
				FprintfStderrLog("RECV_CV_ORDER", 0, uncaMessageBuf, nSizeOfRecvedCVMessage);

				if(m_ClientStatus == csLogoning)
				{
					struct CV_StructOrderReply replymsg;
					int errorcode = -LG_ERROR;
					memset(&replymsg, 0, sizeof(struct CV_StructOrderReply));
					replymsg.header_bit[0] = ESCAPE;
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
#ifdef DEBUG
				printf("get pQueueDAO\n\n\n");
#endif
				CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->GetDAO();

				if(pQueueDAO)
				{
					memset(&cv_order, 0, sizeof(union CV_ORDER));
					memcpy(&cv_order, uncaMessageBuf, nSizeOfCVOrder);
					memset(&cv_ts_order, 0, sizeof(union CV_TS_ORDER));
					long lOrderNumber = 0;

					lOrderNumber = fpFillTandemOrder(m_strService, m_username, m_ClientAddrInfo.caIP,
									m_mBranchAccount, cv_order, cv_ts_order);
#ifdef DEBUG
					printf("lOrderNumber = %ld\n", lOrderNumber);
#endif
					if(lOrderNumber < 0)//error
					{
						int errorcode = ((lOrderNumber >= KI_ERROR) && (lOrderNumber <= TT_ERROR)) ? (-lOrderNumber) : (-KI_ERROR);
						struct CV_StructOrderReply replymsg;

						memset(&replymsg, 0, sizeof(struct CV_StructOrderReply));
						replymsg.header_bit[0] = ESCAPE;
						replymsg.header_bit[1] = ORDERREP;

						memcpy(&replymsg.original, &cv_order, nSizeOfCVOrder);
						sprintf((char*)&replymsg.error_code, "%.4d", errorcode);

						memcpy(&replymsg.reply_msg, pErrorMessage->GetErrorMessage(-errorcode),
							strlen(pErrorMessage->GetErrorMessage(-errorcode)));

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
						//std::map<long, struct CVOriginalOrder>::iterator i = m_mOriginalOrder.find(lOrderNumber);
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
#ifdef DEBUG
	printf("reply client with length %d\n", nToSend);
#endif
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

void CCVClient::LoadRiskControl(char* p_username)
{
	json jtable_query_limit;
	CURLcode res;
	string limit_reply, cum_limit_reply, order_limit_str, strategy_str, accno_str, exchange_str;
	struct AccountData acdata;
	CURL *curl = curl_easy_init();
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	char query_str[512];

	if(!curl)
	{
		FprintfStderrLog("CURL_INIT_FAIL", 0, (unsigned char*)p_username, strlen(p_username));
		return;
	}

	sprintf(query_str, "http://tm1.cryptovix.com.tw:2011/mysql?db=cryptovix_test&query=select%%20acv_risk_control.exchange,acv_risk_control.accounting_no,acv_risk_control.strategy,acv_risk_control.order_limit%%20from%%20acv_risk_control%%20where%%20acv_risk_control.trader_no=(select%%20acv_trader.trader_no%%20from%%20acv_employee,acv_trader%%20where%%20acv_employee.account%%20=%%27%s%%27%%20and%%20acv_trader.emp_no=acv_employee.emp_no)", p_username);

#ifdef DEBUG
	printf("============================\nquery_str:%s\n============================\n", query_str);
#endif
	curl_easy_setopt(curl, CURLOPT_URL, query_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &limit_reply);

	res = curl_easy_perform(curl);
	try {
		jtable_query_limit = json::parse(limit_reply.c_str());

	} catch(...) {
		FprintfStderrLog("JSON_PARSE_FAIL", 0, (unsigned char*)limit_reply.c_str(), limit_reply.length());
	}
 
	if(jtable_query_limit.size() == 0)
	{
		return;
	}

	struct RiskctlData rcdata;
	memset(&rcdata, sizeof(struct RiskctlData), 0);

	for(int i=0 ; i<jtable_query_limit.size() ; i++)
	{
		order_limit_str = jtable_query_limit[i]["order_limit"].dump();
		strategy_str = jtable_query_limit[i]["strategy"].dump();
		strategy_str.erase(remove(strategy_str.begin(), strategy_str.end(), '\"'), strategy_str.end());
		accno_str = jtable_query_limit[i]["accounting_no"].dump();
		accno_str.erase(remove(accno_str.begin(), accno_str.end(), '\"'), accno_str.end());
		exchange_str = jtable_query_limit[i]["exchange"].dump();
		exchange_str.erase(remove(exchange_str.begin(), exchange_str.end(), '\"'), exchange_str.end());
		rcdata.bitmex_limit = atoi(order_limit_str.c_str());
		m_mRiskControl.insert(pair<string, struct RiskctlData>((accno_str+strategy_str), rcdata));

		printf("[%s] %s:%s - %s\n", exchange_str.c_str(), accno_str.c_str(), strategy_str.c_str(), order_limit_str.c_str());
	}

	map<string, struct RiskctlData>::iterator iter;
	for(iter = m_mRiskControl.begin(); iter != m_mRiskControl.end() ; iter++)
		printf("[%s] %s - %d\n", exchange_str.c_str(), iter->first.c_str(), iter->second.bitmex_limit);


	curl_easy_cleanup(curl);
}

void CCVClient::ReplyAccountContents()
{
	char AcclistReplyBuf[MAXDATA];
	map<string, struct AccountData>::iterator iter;
	memset(AcclistReplyBuf, 0, MAXDATA);
	AcclistReplyBuf[0] = ESCAPE;
	AcclistReplyBuf[1] = ACCLISTREP;
	int i = 0, len;

	for(iter = m_mBranchAccount.begin(); iter != m_mBranchAccount.end() ; iter++, i++)
	{
		memcpy(AcclistReplyBuf + 2 + i*(sizeof(struct CV_StructAcclistReply)-2), iter->second.broker_id.c_str(), iter->second.broker_id.length());
		memcpy(AcclistReplyBuf + 2 + i*(sizeof(struct CV_StructAcclistReply)-2)+4, iter->first.c_str(), iter->first.length());
		memcpy(AcclistReplyBuf + 2 + i*(sizeof(struct CV_StructAcclistReply)-2)+11, iter->second.exchange_name.c_str(), iter->second.exchange_name.length());
#ifdef DEBUG
		printf("(%s) (%s), %d\n", iter->second.broker_id.c_str(), 
				AcclistReplyBuf+ 2 + i*(sizeof(struct CV_StructAcclistReply)-2), 2 + i*(sizeof(struct CV_StructAcclistReply)-2));
		printf("(%s) (%s), %d\n", iter->first.c_str(),
				AcclistReplyBuf+ 2 + i*(sizeof(struct CV_StructAcclistReply)-2)+4, 2 + i*(sizeof(struct CV_StructAcclistReply)-2)+4);
		printf("(%s) (%s), %d\n", iter->second.exchange_name.c_str(),
				AcclistReplyBuf+ 2 + i*(sizeof(struct CV_StructAcclistReply)-2)+11, 2 + i*(sizeof(struct CV_StructAcclistReply)-2)+11);
#endif
	}

	len = 2+(i)*(sizeof(struct CV_StructAcclistReply)-2);
#ifdef DEBUG
	printf("account contents len = %d\n", len);
#endif
	int nSendData = SendData((unsigned char*)AcclistReplyBuf, len);

	if(nSendData)
	{
		FprintfStderrLog("SEND_ACCOUNT_LIST", 0, (unsigned char*)AcclistReplyBuf, len);
	}
	else
	{
		FprintfStderrLog("SEND_ACCOUNT_LIST_ERROR", -1, (unsigned char*)AcclistReplyBuf, len);
		perror("SEND_SOCKET_ERROR");
	}
}

void CCVClient::ReplyAccountNum()
{
	struct CV_StructAccnumReply replymsg;
	int num = 0, nSendData;
	char mapsize[3];

	replymsg.header_bit[0] = ESCAPE;
	replymsg.header_bit[1] = ACCNUMREP;

	if(m_ClientStatus == csLogoning)
	{
		replymsg.accnum = 0;
		nSendData = SendData((unsigned char*)&replymsg, sizeof(struct CV_StructAccnumReply));
	}
	else
	{
		replymsg.accnum = m_mBranchAccount.size();
		nSendData = SendData((unsigned char*)&replymsg, sizeof(struct CV_StructAccnumReply));
	}

	if(nSendData)
	{
		FprintfStderrLog("SEND_ACCOUNT_NUM", 0, (unsigned char*)&replymsg, sizeof(struct CV_StructAccnumReply));
	}
	else
	{
		FprintfStderrLog("SEND_ACCOUNT_NUM_ERROR", -1, (unsigned char*)&replymsg, sizeof(struct CV_StructAccnumReply));
		perror("SEND_SOCKET_ERROR");
	}

	if(nSendData)
		ReplyAccountContents();
}

bool CCVClient::LogonAuth(char* p_username, char* p_password, struct CV_StructLogonReply &logon_reply)
{
	json jtable_query_account;
	json jtable_query_exchange;
	CURLcode res;
	string login_query_reply, account_query_reply, acno, exno, brno;
	struct AccountData acdata;
	CURL *curl = curl_easy_init();
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	char macoutput[256];
	char query_str[512];

	if(!curl)
	{
		FprintfStderrLog("CURL_INIT_FAIL", 0, (unsigned char*)p_username, strlen(p_username));
		memcpy(logon_reply.status_code, "NG", 2);
		memcpy(logon_reply.backup_ip, BACKUP_IP, 15);
		memcpy(logon_reply.error_code, "01", 2);
		sprintf(logon_reply.error_message, "cannot accesss authentication server");
		return false;
	}

	sprintf(query_str, "http://tm1.cryptovix.com.tw:2011/mysql?query=select%%20acv_accounting.accounting_no,acv_accounting.broker_no,acv_accounting.exchange_no%%20from%%20acv_accounting%%20where%%20acv_accounting.trader_no=(select%%20acv_trader.trader_no%%20from%%20acv_employee,acv_trader%%20where%%20acv_employee.account%%20=%%27%s%%27%%20and%%20acv_employee.password%%20=%%20%%27%s%%27%%20and%%20acv_trader.emp_no=acv_employee.emp_no)", p_username, p_password);

#ifdef DEBUG
	printf("============================\nquery_str:%s\n============================\n", query_str);
#endif
	curl_easy_setopt(curl, CURLOPT_URL, query_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &login_query_reply);
	res = curl_easy_perform(curl);
	jtable_query_account = json::parse(login_query_reply.c_str());

	if(jtable_query_account.size() == 0)
	{
		memcpy(logon_reply.status_code, "NG", 2);//to do
		memcpy(logon_reply.backup_ip, BACKUP_IP, 15);
		memcpy(logon_reply.error_code, "01", 2);
		sprintf(logon_reply.error_message, "login authentication fail");
		return false;
	}

#ifdef DEBUG
		printf("%s\n", login_query_reply.c_str());
#endif
	for(int i=0 ; i<jtable_query_account.size() ; i++)
	{
		account_query_reply = "";
		acno = jtable_query_account[i]["accounting_no"].dump();
		exno = jtable_query_account[i]["exchange_no"].dump();
		brno = jtable_query_account[i]["broker_no"].dump();
		acno.erase(remove(acno.begin(), acno.end(), '\"'), acno.end());
		exno.erase(remove(exno.begin(), exno.end(), '\"'), exno.end());
		brno.erase(remove(brno.begin(), brno.end(), '\"'), brno.end());

		sprintf(query_str, "http://tm1.cryptovix.com.tw:2011/mysql?query=select%%20exchange_name_en,api_id,api_secret%%20from%%20acv_exchange%%20where%%20exchange_no%%20=%%20%%27%s%%27", exno.c_str());
#ifdef DEBUG
		printf("============================\nquery_str:%s\n============================\n", query_str);
#endif
		curl_easy_setopt(curl, CURLOPT_URL, query_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &account_query_reply);
		res = curl_easy_perform(curl);
		jtable_query_exchange = json::parse(account_query_reply.c_str());
		memset(&acdata, sizeof(struct AccountData), 0);
		acdata.exchange_name = jtable_query_exchange[0]["exchange_name_en"].dump();
		acdata.api_id = jtable_query_exchange[0]["api_id"].dump();
		acdata.api_key = jtable_query_exchange[0]["api_secret"].dump();
		acdata.exchange_name.erase(remove(acdata.exchange_name.begin(), acdata.exchange_name.end(), '\"'), acdata.exchange_name.end());
		acdata.api_id.erase(remove(acdata.api_id.begin(), acdata.api_id.end(), '\"'), acdata.api_id.end());
		acdata.api_key.erase(remove(acdata.api_key.begin(), acdata.api_key.end(), '\"'), acdata.api_key.end());
		acdata.broker_id = brno;
		m_mBranchAccount.insert(pair<string, struct AccountData>(acno, acdata));
#ifdef DEBUG
		printf("%s, %s, %s, %s\n==================\n", acdata.api_id.c_str(), acdata.api_key.c_str(), acdata.exchange_name.c_str(), acdata.broker_id.c_str());
#endif
	}
	
	int expires = (int)time(NULL);
	char expire_str[20];
	sprintf(expire_str, "%d", expires);
	HmacEncodeSHA256(acdata.api_key.c_str(), acdata.api_key.length(), expire_str, strlen(expire_str), mac, mac_length);
	
	for(int i = 0; i < mac_length; i++)
		sprintf(macoutput+i*2, "%02x", (unsigned int)mac[i]);

	if(mac)
		free(mac);

	memcpy(logon_reply.access_token, macoutput, 64);

	sprintf(query_str, "http://tm1.cryptovix.com.tw:2011/mysql?query=update%%20acv_trader%%20set%%20access_token=%%27%.64s%%27where%%20trader_name=%%27%s%%27",
		macoutput, p_username);
	curl_easy_setopt(curl, CURLOPT_URL, query_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &account_query_reply);
	res = curl_easy_perform(curl);

#ifdef DEBUG
	printf("macoutput = %s\n\n\n", macoutput);
	printf("============================\nquery_str:%s\n============================\n", query_str);
#endif
	memcpy(logon_reply.status_code, "OK", 2);//to do
	memcpy(logon_reply.backup_ip, BACKUP_IP, 15);
	memcpy(logon_reply.error_code, "00", 2);
	sprintf(logon_reply.error_message, "login success");
	curl_easy_cleanup(curl);
	return true;


}

int CCVClient::GetClientSocket()
{
	return m_ClientAddrInfo.nSocket;
}
