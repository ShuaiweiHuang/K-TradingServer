#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>
#include <assert.h>

#include "CVReadQueueDAO.h"
#include "CVTandemDAOs.h"
#include "CVReadQueueDAOs.h"
#include "CVTIG/CVTandems.h"
#include "../include/CVTIGMessage.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVReadQueueDAO::CCVReadQueueDAO(key_t key, string strService, string strOTSID)
{
	m_kReadKey = key;

	m_pReadQueue = new CCVQueue;
	m_pReadQueue->Create(m_kReadKey);

	m_strService = strService;
	m_strOTSID = strOTSID;
	Start();
}

CCVReadQueueDAO::~CCVReadQueueDAO()
{
	if(m_pReadQueue != NULL)
	{
		delete m_pReadQueue;

		m_pReadQueue = NULL;
	}
}

void* CCVReadQueueDAO::Run()
{
	struct CV_StructTSOrder cv_ts_order;
	unsigned char uncaRecvBuf[BUFSIZE];
	unsigned char uncaTIGMessage[BUFSIZE];
	unsigned char uncaOrderNumber[13];

	struct TCVTIGMessage TIGMessage;

	int nUserDataSize = 0;
	int nPositionOfSeqnoNumber = 0;

	CCVTandemDAO* pTandemDAO = NULL;
	CCVTandemDAOs* pTandemDAOs = CCVTandemDAOs::GetInstance();
	assert(pTandemDAOs);
	CCVReadQueueDAOs* pReadQueueDAOs = CCVReadQueueDAOs::GetInstance();
	assert(pReadQueueDAOs);

	memset(&TIGMessage, 0, sizeof(struct TCVTIGMessage));

	if(m_strService.compare("TS") == 0)
	{
		nUserDataSize = sizeof(struct CV_StructTSOrderReply);

		struct CV_StructTSOrderReply tig_ts_order;

		nPositionOfSeqnoNumber = tig_ts_order.key_id - (char*)&tig_ts_order;
	}
	else
	{
		//todo
	}

	char caLength[5];
	memset(caLength, 0, sizeof(caLength));
	sprintf(caLength, "%04d", sizeof(struct TCVTIGMessage) + nUserDataSize);

	while(m_pReadQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		int nGetMessage = m_pReadQueue->GetMessage(uncaRecvBuf);
		if(nGetMessage > 0)
		{
			if(nGetMessage != sizeof(struct CV_StructTSOrder))
				FprintfStderrLog("RECV_Q_ERROR_LEN", -1, uncaRecvBuf, 0);

			//FprintfStderrLog("RECV_Q", 0, uncaRecvBuf, nGetMessage);
			memcpy(&cv_ts_order, uncaRecvBuf, nGetMessage);
			memset(uncaOrderNumber, 0, sizeof(uncaOrderNumber));
			memcpy(uncaOrderNumber, cv_ts_order.key_id, 13);


			long lSerialNumber = pReadQueueDAOs->GetSerialNumber();
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);

			char caTag[21];
			memset(caTag, 0, sizeof(caTag));

			sprintf(caTag, "%4d%02d%02d%012ld", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, lSerialNumber);
			memcpy(TIGMessage.caTag, caTag, 20);

			memset(uncaTIGMessage, 0, sizeof(uncaTIGMessage));
			if(m_pTandem)
			{
				memcpy(TIGMessage.caServiceId, m_pTandem->GetService(m_nTandemServiceIndex, m_caTandemService)->uncaServiceID, 20);
				++m_nTandemServiceIndex;
			}

			memcpy(uncaTIGMessage, &TIGMessage, sizeof(struct TCVTIGMessage));
			memcpy(uncaTIGMessage + sizeof(struct TCVTIGMessage), uncaRecvBuf, nUserDataSize);
			int nTIGMessageSize = sizeof(struct TCVTIGMessage) + nUserDataSize;

			while(1)
			{
				pTandemDAO = pTandemDAOs->GetAvailableDAO();

				if(pTandemDAO)
				{
					if(pReadQueueDAOs->GetOrderNumberHashStatus(lSerialNumber) == true)
					{
						pReadQueueDAOs->InsertOrderNumberToHash(lSerialNumber, uncaOrderNumber);
						pReadQueueDAOs->SetOrderNumberHashStatus(lSerialNumber, false);
					}
					else
					{
						FprintfStderrLog("GET_HASH_FALSE_ERROR", 0, uncaRecvBuf, nGetMessage);
						break;
					}
					bool bResult= pTandemDAO->SendOrder(uncaRecvBuf, nGetMessage);
					//pTandemDAO->SetInuse(false);

					if(bResult == true)
					{
						//FprintfStderrLog("SEND_TIG", 0, uncaRecvBuf, nGetMessage);
						break;
					}
					else
					{
						FprintfStderrLog("SEND_TIG_ERROR", -1, uncaTIGMessage, nTIGMessageSize);
						perror("SEND_SOCKET_ERROR");
						pReadQueueDAOs->SetOrderNumberHashStatus(lSerialNumber, true);
						continue;
					}
				}
				else
				{
					usleep(100);
					FprintfStderrLog("GET_TANDEMDAO_ERROR", -1, uncaTIGMessage, 0);

					if(!pTandemDAOs->IsDAOsFull())
					{
						pTandemDAOs->TriggerAddAvailableDAOEvent();
					}
					continue;
				}
			}
		}
		else
		{
			//FprintfStderrLog("TG_RECV_Q_ERROR", -1, uncaRecvBuf, 0);
			perror("TG_RECV_Q_ERROR");
			exit(-1);
		}
	}
	return NULL;
}

key_t CCVReadQueueDAO::GetReadKey()
{
	return m_kReadKey;
}
