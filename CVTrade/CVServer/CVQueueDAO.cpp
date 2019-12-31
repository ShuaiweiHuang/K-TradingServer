#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sys/time.h>
#include <assert.h>

#include "CVQueueDAO.h"
#include "CVQueueDAOs.h"
#include "CVClients.h"
#include "../include/CVType.h"
#include "../include/CVGlobal.h"

using namespace std;

typedef void (*FillCVReply)(union CV_ORDER_REPLY &cv_order_reply, union CV_TS_ORDER_REPLY	&tig_reply);
extern void FillBitcoinReplyFormat(union CV_ORDER_REPLY &cv_order_reply, union CV_TS_ORDER_REPLY &tig_reply);
extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVQueueDAO::CCVQueueDAO(string strService, key_t kSendKey,key_t kRecvKey)
{
	m_strService = strService;

	m_kSendKey = kSendKey;
	m_kRecvKey = kRecvKey;

	m_pSendQueue = new CCVQueue;
	m_pSendQueue->Create(m_kSendKey);

	m_pRecvQueue = new CCVQueue;
	m_pRecvQueue->Create(m_kRecvKey);

	Start();
}

CCVQueueDAO::~CCVQueueDAO() 
{
	if(m_pSendQueue != NULL) 
	{
		delete m_pSendQueue;
	}

	if(m_pRecvQueue != NULL) 
	{
		delete m_pRecvQueue;
	}
}

void* CCVQueueDAO::Run()
{
	unsigned char uncaRecvBuf[BUFSIZE];
	unsigned char uncaSendBuf[BUFSIZE];
	char caOrderNumber[14];// 13 + \0

	memset(uncaSendBuf, 0, sizeof(uncaSendBuf));
	uncaSendBuf[0] = 0x1b;// for order reply

	int nSizeOfOriginalOrder;
	int nSizeOfTIGReply;
	int nSizeOfSendSocket;

	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);
	CCVClient* pClient = NULL;

	FillCVReply fpFillCVReply = NULL;
	union CV_ORDER_REPLY cv_order_reply;
	union CV_TS_ORDER_REPLY tig_reply;

	if(m_strService.compare("TS") == 0)
	{
		fpFillCVReply = FillBitcoinReplyFormat;

		nSizeOfOriginalOrder = sizeof(struct CV_StructOrder);

		nSizeOfTIGReply = sizeof(struct CV_StructTSOrderReply);

		nSizeOfSendSocket = sizeof(struct CV_StructOrderReply);
		
		uncaSendBuf[1] = ORDERREP;// for order reply
	}
	else
	{
		FprintfStderrLog("SERVICE NAME ERROR", -1, 0, 0);
	}

	while(m_pRecvQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		int nGetMessage = m_pRecvQueue->GetMessage(uncaRecvBuf);
		if(nGetMessage > 0)
		{
			FprintfStderrLog("RECV_Q", 0, uncaRecvBuf, nGetMessage);

			memset(caOrderNumber, 0, sizeof(caOrderNumber));
			memcpy(caOrderNumber, uncaRecvBuf+4, 13);
			long lOrderNumber = atol(caOrderNumber);

#ifdef DEBUG
			printf("\nreceive queue %d data with length = %d\n\n", m_kRecvKey, nSizeOfSendSocket);
#endif
			try
			{
				

				if(pClients->GetClientFromHash(lOrderNumber) == NULL)
				{
					throw "Lost!";
				}
				else if(pClients->hostname_check_num != (lOrderNumber)/10000000000)
				{
					throw "Hostname not match!";
				}
				else
				{	
					pClient = pClients->GetClientFromHash(lOrderNumber);

					if(pClient->GetStatus() == csOnline)
					{
						memset(&cv_order_reply, 0, sizeof(union CV_ORDER_REPLY));
						pClient->GetOriginalOrder(lOrderNumber, nSizeOfOriginalOrder, cv_order_reply);
					}
					else 
					{
						FprintfStderrLog("CLIENT_NOT_ONLINE", -1, uncaRecvBuf, nGetMessage);
						pClients->RemoveClientFromHash(lOrderNumber);
						continue;
					}
					memset(&tig_reply, 0, sizeof(union CV_TS_ORDER_REPLY));
					memcpy(&tig_reply, uncaRecvBuf, nSizeOfTIGReply);

					fpFillCVReply(cv_order_reply, tig_reply);
					memcpy(uncaSendBuf , &cv_order_reply, sizeof(union CV_ORDER_REPLY));
#if 1
					//Risk control check
					string strRiskKey(cv_order_reply.cv_reply.original.sub_acno_id);
					printf("\n\nReply strRiskKey = %s\n\n", strRiskKey.c_str());

					map<string, struct RiskctlData>::iterator iter;

					iter = pClient->m_mRiskControl.find(strRiskKey);
					char Qty[10], Status[5];
					memset(Qty, 0, 10);
					memset(Status, 0, 5);
					memcpy(Qty, cv_order_reply.cv_reply.original.order_qty, 9);
					memcpy(Status, cv_order_reply.cv_reply.error_code, 4);
					int order_qty = atoi(Qty);
					printf("\n\n\n%s\n\n\n", Status);
					if(strcmp(Status, "1000") == 0) {

						if(cv_order_reply.cv_reply.original.trade_type[0] == '1')//delete order success
							pClient->m_bitmex_side_limit_current += ((cv_order_reply.cv_reply.original.order_buysell[0] == 'B') ? -(order_qty) : order_qty);

						//if(cv_order_reply.cv_reply.original.trade_type[0] == '0')//submit order success
						//	pClient->m_bitmex_side_limit_current += ((cv_order_reply.cv_reply.original.order_buysell[0] == 'S') ? -(order_qty) : order_qty);

					}
					else{
						if(cv_order_reply.cv_reply.original.trade_type[0] == '0')//submit order success
							pClient->m_bitmex_side_limit_current += ((cv_order_reply.cv_reply.original.order_buysell[0] == 'B') ? -(order_qty) : order_qty);
#if 1
						if(cv_order_reply.cv_reply.original.trade_type[0] == '0') {
							if(pClient->m_order_index > 0) {
								pClient->m_order_index--;
							}
							else {
								pClient->m_order_index = MAX_TIME_LIMIT-1;
							}
							pClient->m_order_timestamp[pClient->m_order_index] = 0;
						}
#endif							
					}
					printf("order_qty = %d, order_limit = %d\nside_limit = %d, side_limit_current = %d\ntime_limit = %d, time_limit_current = %d\n",
						order_qty, iter->second.bitmex_limit, iter->second.bitmex_side_limit, pClient->m_bitmex_side_limit_current, 
						iter->second.bitmex_time_limit, pClient->m_bitmex_time_limit_current);

#endif

					bool bSendData = false;

					if(pClient->GetStatus() == csOnline)//weird if(pClient) not working
					{
						bSendData = pClient->SendData(uncaSendBuf, nSizeOfSendSocket);
					}
					else
					{
						FprintfStderrLog("CLIENT_NOT_ONLINE", -1, uncaRecvBuf, nGetMessage);
						pClients->RemoveClientFromHash(lOrderNumber);
						continue;
					}
					if(bSendData == true)
					{
						FprintfStderrLog("SEND_CV_ORDER_REPLY", 0, uncaSendBuf, nSizeOfSendSocket);
						//pClients->RemoveClientFromHash(lOrderNumber);
					}
					else
					{
						FprintfStderrLog("SEND_CV_ORDER_REPLY_ERROR", -1, uncaSendBuf, nSizeOfSendSocket);
						perror("SEND_SOCKET_ERROR");
						throw "Send Data Error!";
					}
				}
			}
			catch(char const* pExceptionMessage)
			{
				cout << "OrderNumber: " << lOrderNumber << " " << pExceptionMessage << endl;
				continue;//to do
			}
		}
		else
		{
			FprintfStderrLog("RECV_Q_ERROR", -1, uncaRecvBuf, 0);
			perror("RECV_Q_ERROR");
			//todo
		}
	}
	return NULL;
}

int CCVQueueDAO::SendData(const unsigned char* pBuf, int nSize, long lType, int nFlag)
{
	int nResult = -1;
	if(m_pSendQueue)
	{
		//printf("Key = %d\n", m_kSendKey);
		nResult= m_pSendQueue->SendMessage(pBuf, nSize, lType, nFlag);
		//printf("return nResult = %d\n", nResult);
	}
	else
	{
		FprintfStderrLog("SEND_Q_ERROR", -1, NULL, 0);
		return nResult;
	}

	return nResult;
}

key_t CCVQueueDAO::GetRecvKey()
{
	return m_kRecvKey;
}

key_t CCVQueueDAO::GetSendKey()
{
	return m_kSendKey;
}
