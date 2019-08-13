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
#include "../include/CVTIGMessage.h"
#include "../include/CVType.h"
#include "CVTIG/CVTandems.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKReadQueueDAO::CSKReadQueueDAO(key_t key, string strService, string strOTSID)
{
	m_kReadKey = key;

	m_pReadQueue = new CSKQueue;
	m_pReadQueue->Create(m_kReadKey);

	m_strService = strService;
	m_strOTSID = strOTSID;

#if 0
	memset(m_caTandemService, 0, sizeof(m_caTandemService));
	m_pTandem = NULL;
	CSKTandemDAOs* pTandemDAOs = CSKTandemDAOs::GetInstance();
	assert(pTandemDAOs);
	if(pTandemDAOs->GetService().compare("TS") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemTaiwanStock();

#ifdef IPVH_331
		memcpy(m_caTandemService, "IPVHS331-F1", 11);
#else
		memcpy(m_caTandemService, "IPVHS330", 8);
#endif

	}
	else
	{}
	srand((unsigned)time(NULL));

	m_nTandemServiceIndex = rand()%11;

#endif
	Start();
}

CSKReadQueueDAO::~CSKReadQueueDAO()
{
	if(m_pReadQueue != NULL)
	{
		delete m_pReadQueue;

		m_pReadQueue = NULL;
	}
}

void* CSKReadQueueDAO::Run()
{
	struct CV_StructTSOrder cv_ts_order;
	unsigned char uncaRecvBuf[BUFSIZE];
	unsigned char uncaTIGMessage[BUFSIZE];
	unsigned char uncaOrderNumber[13];

	struct TSKTIGMessage TIGMessage;

	int nUserDataSize = 0;
	int nPositionOfSeqnoNumber = 0;

	CSKTandemDAO* pTandemDAO = NULL;
	CSKTandemDAOs* pTandemDAOs = CSKTandemDAOs::GetInstance();
	assert(pTandemDAOs);
	CSKReadQueueDAOs* pReadQueueDAOs = CSKReadQueueDAOs::GetInstance();
	assert(pReadQueueDAOs);

	memset(&TIGMessage, 0, sizeof(struct TSKTIGMessage));

	if(m_strService.compare("TS") == 0)
	{
		nUserDataSize = sizeof(struct CV_StructTSOrderReply);

		struct CV_StructTSOrderReply tig_ts_order;

		nPositionOfSeqnoNumber = tig_ts_order.seq_id - (char*)&tig_ts_order;

		memcpy(TIGMessage.caServiceId, 	"IPVHS331-F1         ", 20);
	}
	else
	{
		//todo
	}

	char caLength[5];
	memset(caLength, 0, sizeof(caLength));
	sprintf(caLength, "%04d", sizeof(struct TSKTIGMessage) + nUserDataSize);
	memcpy(TIGMessage.caLength, caLength, 4);

	memcpy(TIGMessage.caType, "DARQ", 4);

	memcpy(TIGMessage.caOTSId, m_strOTSID.c_str(), strlen(m_strOTSID.c_str()));//not greater than 12
	memset(TIGMessage.caOTSId + strlen(m_strOTSID.c_str()), 32, 12 - strlen(m_strOTSID.c_str()));//set space

	memcpy(TIGMessage.caError, "00000000", 8);

	while(m_pReadQueue)
	{
		memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
		int nGetMessage = m_pReadQueue->GetMessage(uncaRecvBuf);
		if(nGetMessage > 0)
		{
			if(nGetMessage != sizeof(struct CV_StructTSOrder))
				FprintfStderrLog("RECV_Q_ERROR_LEN", -1, uncaRecvBuf, 0);

			FprintfStderrLog("RECV_Q", 0, uncaRecvBuf, nGetMessage);
			memcpy(&cv_ts_order, uncaRecvBuf, nGetMessage);
			memset(uncaOrderNumber, 0, sizeof(uncaOrderNumber));
			memcpy(uncaOrderNumber, uncaRecvBuf + nPositionOfSeqnoNumber, 13);

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

			memcpy(uncaTIGMessage, &TIGMessage, sizeof(struct TSKTIGMessage));
			memcpy(uncaTIGMessage + sizeof(struct TSKTIGMessage), uncaRecvBuf, nUserDataSize);
			int nTIGMessageSize = sizeof(struct TSKTIGMessage) + nUserDataSize;

			printf("%d:uncaRecvBuf = %s\n", nGetMessage, uncaRecvBuf);

			while(1)
			{
				sleep(1);
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
					bool bResult= pTandemDAO->SendData(uncaRecvBuf, nGetMessage);
					pTandemDAO->SetInuse(false);

					if(bResult == true)
					{
						FprintfStderrLog("SEND_TIG", 0, uncaRecvBuf, nGetMessage);
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
			FprintfStderrLog("RECV_Q_ERROR", -1, uncaRecvBuf, 0);
			perror("RECV_Q_ERROR");
		}
	}
	return NULL;
}

key_t CSKReadQueueDAO::GetReadKey()
{
	return m_kReadKey;
}
