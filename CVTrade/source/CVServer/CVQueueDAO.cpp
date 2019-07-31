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
#include "../include/CVReply.h"
#include "../include/TIGReply.h"

union L//timegood
{
   unsigned char uncaByte[16];
   long value;
};//timegood

using namespace std;

#ifdef MNTRMSG
extern struct MNTRMSGS g_MNTRMSG;
#endif

typedef void (*FillCVReply)(union CV_REPLY &sk_reply, union TIG_REPLY	&tig_reply);

extern void FillTaiwanStockReplyFormat(union CV_REPLY &sk_reply, union TIG_REPLY &tig_reply);

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
	union CV_REPLY sk_reply;
	union TIG_REPLY tig_reply;

	if(m_strService.compare("TS") == 0)
	{
//		fpFillCVReply = FillTaiwanStockReplyFormat;

		nSizeOfOriginalOrder = sizeof(struct CV_TS_ORDER);

		nSizeOfTIGReply = sizeof(struct TIG_TS_REPLY);

		nSizeOfSendSocket = sizeof(struct CV_TS_REPLY) + 2;
		
		uncaSendBuf[1] = 0x01;// for order reply
	}
	else
	{
		//todo
	}

	while(m_pRecvQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		int nGetMessage = m_pRecvQueue->GetMessage(uncaRecvBuf);
#ifdef DEBUG
		printf("SERVER: queue data read at key %d\n", m_kRecvKey);
#endif
		if(nGetMessage > 0)
		{
#ifdef TIMETEST
			struct  timeval timeval_Start;
			struct  timeval timeval_End;
			static int order_count = 0;
			gettimeofday (&timeval_Start, NULL) ;
#endif
			FprintfStderrLog("RECV_Q", 0, uncaRecvBuf, nGetMessage);

			memset(caOrderNumber, 0, sizeof(caOrderNumber));
			memcpy(caOrderNumber, uncaRecvBuf, 13);
			long lOrderNumber = atol(caOrderNumber);


			try
			{
				if(pClients->GetClientFromHash(lOrderNumber) == NULL)
				{
					throw "Lost!";
				}
				else
				{
					pClient = pClients->GetClientFromHash(lOrderNumber);

					if(pClient->GetStatus() == csOnline)//weird if(pClient) not working
					{
						memset(&sk_reply, 0, sizeof(union CV_REPLY));
						pClient->GetOriginalOrder(lOrderNumber, nSizeOfOriginalOrder, sk_reply);
					}
					else 
					{
						FprintfStderrLog("CLIENT_NOT_ONLINE", -1, uncaRecvBuf, nGetMessage);
						pClients->RemoveClientFromHash(lOrderNumber);
						continue;
					}
					memset(&tig_reply, 0, sizeof(union TIG_REPLY));
					memcpy(&tig_reply, uncaRecvBuf + 13, nSizeOfTIGReply);

					fpFillCVReply(sk_reply, tig_reply);

					memset(uncaSendBuf + 2, 0, sizeof(uncaSendBuf) - 2);
					memcpy(uncaSendBuf + 2, &sk_reply, sizeof(union CV_REPLY));

					bool bSendData = false;

					if(pClient->GetStatus() == csOnline)//weird if(pClient) not working
						bSendData = pClient->SendData(uncaSendBuf, nSizeOfSendSocket);
					else
					{
						FprintfStderrLog("CLIENT_NOT_ONLINE", -1, uncaRecvBuf, nGetMessage);
						pClients->RemoveClientFromHash(lOrderNumber);
						continue;
					}
#ifdef MNTRMSG
					struct CV_TS_ORDER reply_oringinal;
					memcpy((void*)&(reply_oringinal), (void*)(uncaSendBuf+2), sizeof(struct CV_TS_ORDER));
					memset(pClient->m_LogMessageQueue.reply_msg_buf[pClient->m_LogMessageQueue.reply_index], 0, BUFFERSIZE);
					sprintf(pClient->m_LogMessageQueue.reply_msg_buf[pClient->m_LogMessageQueue.reply_index], "%.7s,%.13s,%.5s,%hd,%.78s",
							reply_oringinal.acno, reply_oringinal.key,reply_oringinal.order_no,
							*(uncaSendBuf+sizeof(struct CV_TS_ORDER)+2+78),//Status
							uncaSendBuf+sizeof(struct CV_TS_ORDER)+2);//Messasge

					pClient->m_LogMessageQueue.reply_msg_len[pClient->m_LogMessageQueue.reply_index] = strlen(pClient->m_LogMessageQueue.reply_msg_buf[pClient->m_LogMessageQueue.reply_index]);
					pClient->m_LogMessageQueue.reply_index++;
#endif
					if(bSendData == true)
					{
#ifdef TIMETEST
						gettimeofday (&timeval_End, NULL) ;
						InsertTimeLogToSharedMemory(&timeval_Start, &timeval_End, tpQueueDAO_Reply_ClinetToProxy, order_count);
						InsertTimeLogToSharedMemory(NULL, &timeval_End, tpQueueProcessEnd, order_count);
						order_count++;
#endif
#ifdef MNTRMSG
						g_MNTRMSG.num_of_order_Reply++;
#endif

						FprintfStderrLog("SEND_CV_REPLY", 0, uncaSendBuf, nSizeOfSendSocket);
						pClients->RemoveClientFromHash(lOrderNumber);
					}
					else
					{
						FprintfStderrLog("SEND_CV_REPLY_ERROR", -1, uncaSendBuf, nSizeOfSendSocket);
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
#ifdef DEBUG
	printf("SERVER: queue data send at key %d\n", m_kSendKey);
#endif
	if(m_pSendQueue)
	{
		nResult= m_pSendQueue->SendMessage(pBuf, nSize, lType, nFlag);
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
