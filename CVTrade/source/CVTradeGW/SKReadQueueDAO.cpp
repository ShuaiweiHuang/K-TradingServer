#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <fstream>
#include <assert.h>

#include "SKReadQueueDAO.h"
#include "SKTandemDAOs.h"
#include "SKReadQueueDAOs.h"
//#include "include/tandem.h"
#include "../include/TIGTSFormat.h"
#include "../include/TIGTFFormat.h"
#include "../include/TIGOFFormat.h"
#include "../include/TIGOSFormat.h"
#include "../include/SKTIGMessage.h"
#include "SKTIG/SKTandems.h"

using namespace std;

#ifdef MNTRMSG
extern struct MNTRMSGS g_MNTRMSG;
#endif

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

static void LogTime(char* pOrderNumber, char cSymbol)//nano second
{
	struct timespec end;

	clock_gettime(CLOCK_REALTIME, &end);
	long time = 1000000000 * (end.tv_sec) + end.tv_nsec;

	string filename(pOrderNumber);
	filename = filename + ".txt";

	fstream fp;
	fp.open(filename, ios::out|ios::app);
	fp << time<< cSymbol ;
	fp.close();
}

CSKReadQueueDAO::CSKReadQueueDAO(key_t key, string strService, string strOTSID)
{
	m_kReadKey = key;

	m_pReadQueue = new CSKQueue;
	m_pReadQueue->Create(m_kReadKey);

	m_strService = strService;
	m_strOTSID = strOTSID;

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
	else if(pTandemDAOs->GetService().compare("TF") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemTaiwanFuture();
		memcpy(m_caTandemService, "FPPW_VHX330A", 12);
	}
	else if(pTandemDAOs->GetService().compare("OF") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemOverseasFuture();
		memcpy(m_caTandemService, "FIPW_FVHX330", 12);
	}
	else if(pTandemDAOs->GetService().compare("OS") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemOthers();
		memcpy(m_caTandemService, "TSMPW_GHS200", 12);
	}
	else
	{}

	srand((unsigned)time(NULL));

	m_nTandemServiceIndex = rand()%11;

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
		nUserDataSize = sizeof(struct TIG_TS_ORDER);

		struct TIG_TS_ORDER tig_ts_order;

		nPositionOfSeqnoNumber = tig_ts_order.seqno - (char*)&tig_ts_order;

#ifdef IPVH_331	//remove redundant code after IPVH 331 version.
		memcpy(TIGMessage.caServiceId, 	"IPVHS331-F1         ", 20);
#else
		memcpy(TIGMessage.caServiceId, 	"IPVHS330            ", 20);
#endif

	}
	else if(m_strService.compare("TF") == 0)
	{
		nUserDataSize = sizeof(struct TIG_TF_ORDER);

		struct TIG_TF_ORDER tig_tf_order;

		nPositionOfSeqnoNumber = tig_tf_order.seqno - (char*)&tig_tf_order;

		memcpy(TIGMessage.caServiceId, 	"FPPW_VHX330A-F1     ", 20);
	}
	else if(m_strService.compare("OF") == 0)
	{
		nUserDataSize = sizeof(struct TIG_OF_ORDER);

		struct TIG_OF_ORDER tig_of_order;

		nPositionOfSeqnoNumber = tig_of_order.seqno - (char*)&tig_of_order;

		memcpy(TIGMessage.caServiceId, 	"FIPW_FVHX330        ", 20);
	}
	else if(m_strService.compare("OS") == 0)
	{
		nUserDataSize = sizeof(struct TIG_OS_ORDER);

		struct TIG_OS_ORDER tig_os_order;

		nPositionOfSeqnoNumber = tig_os_order.seqno - (char*)&tig_os_order;

		memcpy(TIGMessage.caServiceId, 	"TSMPW_GHS200        ", 20);
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
#ifdef DEBUG
		printf("m_kReadKey = %d ", m_kReadKey);
		printf("m_strService = %s ", m_strService.c_str());
		printf("m_strOTSID = %s\n", m_strOTSID.c_str());
#endif

		if(nGetMessage > 0)
		{

#ifdef TIMETEST
			static int order_count = 0;
			struct  timeval timeval_Start;
			struct  timeval timeval_End;
#endif
			FprintfStderrLog("RECV_Q", 0, uncaRecvBuf, nGetMessage);

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

			while(1)
			{
#ifdef MNTRMSG
				pTandemDAOs->UpdateAvailableDAONum();
#endif
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

#ifdef TIMETEST
					static int send_order_count = 0;
					gettimeofday (&timeval_Start, NULL) ;
					gettimeofday (&timeval_End, NULL) ;
				    InsertTimeLogToSharedMemory(NULL, &timeval_End, tpTandemProcessStart, send_order_count);
				    send_order_count++;
#endif

#ifdef MNTRMSG
   					 g_MNTRMSG.num_of_order_Received++;
#endif


					bool bResult= pTandemDAO->SendData(uncaTIGMessage, nTIGMessageSize);
					pTandemDAO->SetInuse(false);

					if(bResult == true)
					{
#ifdef TIMETEST
						gettimeofday (&timeval_End, NULL) ;
						InsertTimeLogToSharedMemory(&timeval_Start, &timeval_End, tpQueueDAOToTandem, order_count);
						order_count++;
#endif
#ifdef MNTRMSG
					    g_MNTRMSG.num_of_order_Sent++;
#endif
						FprintfStderrLog("SEND_TIG", 0, uncaTIGMessage, nTIGMessageSize);
						break;
					}
					else
					{
						FprintfStderrLog("SEND_TIG_ERROR", -1, uncaTIGMessage, nTIGMessageSize);
						perror("SEND_SOCKET_ERROR");
						pReadQueueDAOs->SetOrderNumberHashStatus(lSerialNumber, true);
//						usleep(100000);
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
//					usleep(400000);
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
