#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <iconv.h>


#include "SKTandemDAO.h"
#include "SKReadQueueDAOs.h"
#include "../include/SKTIGMessage.h"
#include "SKTandemDAOs.h"
#include "../include/TIGTSFormat.h"
#include "../include/TIGTFFormat.h"
#include "../include/TIGOFFormat.h"
#include "../include/TIGOSFormat.h"
#include "SKTIG/SKTandems.h"

using namespace std;

#define TIG_TS_REPLY_MESSAGE_LENGTH 78
#define TIG_TS_REPLY_CODE_LENGTH 4
#define TIG_TS_STATUS_CODE_LENGTH 4

#ifdef MNTRMSG
extern struct MNTRMSGS g_MNTRMSG;
#endif

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

static int IsMessageComplete(unsigned char* pBuf, int nSize)// 0 -> complete, -1 -> incomplete, nLength -> overhead
{
	char caLength[5];//4+\0

	if(nSize < 8)
	{
		return -1;
	}
	else
	{
		memset(caLength, 0, 5);
		memcpy(caLength, pBuf, 4);

		int nLength = atoi(caLength);

		if(nLength == nSize)
		{
			return 0;
		}
		else if(nLength < nSize)
		{
			return nLength;
		}
		else if(nLength > nSize)
		{
			return -1;
		}
	}
}

static void LogTime(char* pOrderNumber, char cSymbol)//nano second
{
	struct timespec end;

	clock_gettime(CLOCK_REALTIME, &end);
	long time = 1000000000 * (end.tv_sec) + end.tv_nsec;

	string filename(pOrderNumber);
	filename = filename + ".txt";

	fstream fp;
	fp.open(filename, ios::out | ios::app);
	fp << time << cSymbol ;
	fp.close();
}

CSKTandemDAO::CSKTandemDAO()
{
	m_pSocket = NULL;
}

CSKTandemDAO::CSKTandemDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey)
{
	m_pSocket = NULL;
	m_pHeartbeat = NULL;

	m_TandemDAOStatus = tsNone;
	m_bInuse = false;

	m_pWriteQueueDAOs = CSKWriteQueueDAOs::GetInstance();

	if(m_pWriteQueueDAOs == NULL)
		m_pWriteQueueDAOs = new CSKWriteQueueDAOs(nNumberOfWriteQueueDAO, kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);

	assert(m_pWriteQueueDAOs);

	m_pTandem = NULL;
	CSKTandemDAOs* pTandemDAOs = CSKTandemDAOs::GetInstance();
	assert(pTandemDAOs);
	if(pTandemDAOs->GetService().compare("TS") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemTaiwanStock();
	}
	else if(pTandemDAOs->GetService().compare("TF") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemTaiwanFuture();
	}
	else if(pTandemDAOs->GetService().compare("OF") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemOverseasFuture();
	}
	else if(pTandemDAOs->GetService().compare("OS") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemOthers();
	}
	else if(pTandemDAOs->GetService().compare("BL") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemOthers();
	}
	else
	{}

	m_nTandemNodeIndex = nTandemDAOID;

	pthread_mutex_init(&m_MutexLockOnSetStatus, NULL);

	m_pSocket = new CSKSocket(this);

	if(m_pTandem)
	{
		m_strHost.assign(reinterpret_cast<const char*>(m_pTandem->GetNode(nTandemDAOID)->uncaIP), 20);
		m_strPort.assign(reinterpret_cast<const char*>(m_pTandem->GetNode(nTandemDAOID)->uncaListenPort), 5);
	}

	if(m_pSocket)
		m_pSocket->Connect(m_strHost, m_strPort); //start

	sprintf(m_caTandemDAOID, "%03d", nTandemDAOID);
}

CSKTandemDAO::~CSKTandemDAO()
{
	if(m_pSocket)
	{
		m_pSocket->Disconnect();

		delete m_pSocket;
		m_pSocket = NULL;
	}

	if(m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	if(m_pWriteQueueDAOs)
	{
		delete m_pWriteQueueDAOs;
		m_pWriteQueueDAOs = NULL;
	}

	pthread_mutex_destroy(&m_MutexLockOnSetStatus);
}

void CSKTandemDAO::SendLogout()
{
    const unsigned char LGOT[] = "0008LGOT";

    if(m_pSocket)
        m_pSocket->Send(LGOT, 8);
}


void* CSKTandemDAO::Run()
{
	unsigned char uncaRecvBuf[BUFSIZE];//1024
	unsigned char uncaMessageBuf[2048];
	unsigned char uncaOverheadMessageBuf[BUFSIZE];
	unsigned char uncaUserData[BUFSIZE];

	memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
	memset(uncaMessageBuf, 0, sizeof(uncaMessageBuf));
	memset(uncaOverheadMessageBuf, 0, sizeof(uncaOverheadMessageBuf));

	struct TSKTIGMessage TIGMessage;
	char caTIGNumber[12 + 1]; //12+\0
	long lTIGNumber;

	int nSizeOfRecvedMessage = 0;
	int nSizeOfOverheadMessage = 0;
	int nSizeOfRecvedTIGMessage = 0;
	int nSizeOfSocket = 0;

	memset(uncaOverheadMessageBuf, 0, sizeof(uncaOverheadMessageBuf));

	CSKTandemDAOs* pTandemDAOs = CSKTandemDAOs::GetInstance();
	assert(pTandemDAOs);

	if(pTandemDAOs->GetService().compare("TS") == 0)
	{
		nSizeOfSocket = sizeof(struct TSKTIGMessage) + sizeof(struct TIG_TS_REPLY);
	}
	else if(pTandemDAOs->GetService().compare("TF") == 0)
	{
		nSizeOfSocket = sizeof(struct TSKTIGMessage) + sizeof(struct TIG_TF_REPLY);
	}
	else if(pTandemDAOs->GetService().compare("OF") == 0)
	{
		nSizeOfSocket = sizeof(struct TSKTIGMessage) + sizeof(struct TIG_OF_REPLY);
	}
	else if(pTandemDAOs->GetService().compare("OS") == 0)
	{
		nSizeOfSocket = sizeof(struct TSKTIGMessage) + sizeof(struct TIG_OS_REPLY);
	}

	CSKReadQueueDAOs* pReadQueueDAOs = CSKReadQueueDAOs::GetInstance();
	assert(pReadQueueDAOs);

	SetStatus(tsServiceOff);

	int nTimeIntervals = 30;
	m_pHeartbeat = new CSKHeartbeat(nTimeIntervals);
	assert(m_pHeartbeat);

	m_pHeartbeat->SetCallback(this);
	m_pHeartbeat->Start();

	while(IsTerminated())
	{
		if(m_TandemDAOStatus == tsServiceOff || m_TandemDAOStatus == tsServiceOn)
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

				if(nIsMessageComplete == -1)
				{
					memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
					int nRecv = m_pSocket->Recv(uncaRecvBuf, sizeof(uncaRecvBuf));
					if(nRecv > 0)
					{
						FprintfStderrLog("RECV_TIG", 0, uncaRecvBuf, nRecv);
						m_pHeartbeat->TriggerGetTIGReplyEvent();
						memcpy(uncaMessageBuf + nSizeOfRecvedMessage, uncaRecvBuf, nRecv);
						nSizeOfRecvedMessage += nRecv;
					}
					else if(nRecv == 0)//connection closed
					{
						FprintfStderrLog("RECV_TIG_CLOSE", 0, reinterpret_cast<unsigned char*>(m_caTandemDAOID), sizeof(m_caTandemDAOID));
						m_pHeartbeat->TriggerGetTIGReplyEvent();//reset heartbeat
						SetStatus(tsBreakdown);
						nSizeOfRecvedTIGMessage = -1;
						break;
					}
					else if(nRecv == -1)
					{
						FprintfStderrLog("RECV_TIG_ERROR", -1, reinterpret_cast<unsigned char*>(m_caTandemDAOID), sizeof(m_caTandemDAOID));
						perror("RECV_SOCKET_ERROR");
						m_pHeartbeat->TriggerGetTIGReplyEvent();//reset heartbeat
						SetStatus(tsBreakdown);
						nSizeOfRecvedTIGMessage = -1;
						break;
					}
				}
				else if(nIsMessageComplete == 0)
				{
					nSizeOfRecvedTIGMessage = nSizeOfRecvedMessage;
					nSizeOfRecvedMessage = 0;
					break;
				}
				else if(nIsMessageComplete > 0)
				{
					nSizeOfRecvedTIGMessage = nIsMessageComplete;
					nSizeOfOverheadMessage = nSizeOfRecvedMessage - nSizeOfRecvedTIGMessage;

					memset(uncaOverheadMessageBuf, 0, sizeof(uncaOverheadMessageBuf));
					memcpy(uncaOverheadMessageBuf, uncaMessageBuf + nSizeOfRecvedTIGMessage, nSizeOfOverheadMessage);
					memset(uncaMessageBuf + nIsMessageComplete, 0, sizeof(uncaMessageBuf) - nIsMessageComplete);
					nSizeOfRecvedMessage = 0;
					break;
				}
			}

			if(nSizeOfRecvedTIGMessage > 0)
			{
				fprintf(stderr, "Msg from tandem : ");
				for(int i = 0; i < nSizeOfRecvedTIGMessage; i++)
					fprintf(stderr, "%c", uncaMessageBuf[i]);
				cout << endl;

				if(	memcmp(uncaMessageBuf, "0008HTBT", 8) == 0 ||
					memcmp(uncaMessageBuf, "0008HTBR", 8) == 0 ||
					memcmp(uncaMessageBuf, "0008SVON", 8) == 0 ||
					memcmp(uncaMessageBuf, "0008SVOF", 8) == 0)
				{
					FprintfStderrLog("RECV_TIG_CONTROL_MESSAGE", 0, uncaMessageBuf, 8);
					OnControlMessage(uncaMessageBuf);
				}
				else
				{
					FprintfStderrLog("RECV_TIG_REPLY", 0, uncaMessageBuf, nSizeOfRecvedTIGMessage);

					memset(&TIGMessage, 0, sizeof(TSKTIGMessage));
					memcpy(&TIGMessage, uncaMessageBuf, sizeof(TSKTIGMessage));
					memset(caTIGNumber, 0, 13);
					memcpy(caTIGNumber, TIGMessage.caTag + 8, 12);
					lTIGNumber = atol(caTIGNumber);

					try
					{
						if(pReadQueueDAOs->GetOrderNumberHashStatus(lTIGNumber) == true)
						{
							throw lTIGNumber;
						}
						else
						{
							memset(uncaUserData, 0, sizeof(uncaUserData));
							pReadQueueDAOs->GetOrderNumberFromHash(lTIGNumber, uncaUserData);
							pReadQueueDAOs->SetOrderNumberHashStatus(lTIGNumber, true);
							memcpy(uncaUserData + 13, uncaMessageBuf + sizeof(struct TSKTIGMessage), nSizeOfRecvedTIGMessage - sizeof(struct TSKTIGMessage));
							CSKWriteQueueDAO* pWriteQueueDAO = NULL;
							while(pWriteQueueDAO == NULL)
							{
								if(m_pWriteQueueDAOs)
									pWriteQueueDAO = m_pWriteQueueDAOs->GetAvailableDAO();

								if(pWriteQueueDAO)
								{
									FprintfStderrLog("GET_WRITEQUEUEDAO", 0, reinterpret_cast<unsigned char*>(m_caTandemDAOID), sizeof(m_caTandemDAOID), reinterpret_cast<unsigned char*>(pWriteQueueDAO->GetWriteQueueDAOID()), 4);
									pWriteQueueDAO->SetReplyMessage(uncaUserData, 13 + nSizeOfRecvedTIGMessage - sizeof(struct TSKTIGMessage));
									pWriteQueueDAO->TriggerWakeUpEvent();
#ifdef TIMETEST
									struct timeval timeval_End;
									static int order_count = 0;
									gettimeofday (&timeval_End, NULL) ;
									InsertTimeLogToSharedMemory(NULL, &timeval_End, tpTandemProcessEnd, order_count);
									order_count++;
#endif
								}
								else
								{
									FprintfStderrLog("GET_WRITEQUEUEDAO_NULL_ERROR", -1, reinterpret_cast<unsigned char*>(m_caTandemDAOID), sizeof(m_caTandemDAOID));
									sleep(1);
								}
							}
						}
					}
					catch(long lTIGNumberException)
					{
						string filename = "TIGNumberException.txt";
						fstream fp;
						fp.open(filename, ios::out);
						fp << "TIGNumber= " << lTIGNumberException << endl;
						fp.close();
					}
					catch(...)
					{}
				}
			}
		}
		else if(m_TandemDAOStatus == tsBreakdown)
		{
			SetStatus(tsReconnecting);
			ReconnectSocket();
		}
		else
		{}
	}
	return NULL;
}

bool CSKTandemDAO::SendData(const unsigned char* pBuf, int nSize)
{
	if(GetStatus() == tsServiceOff) //recheck status
		return false;
	return SendAll(pBuf, nSize);
}
bool CSKTandemDAO::SendAll(const unsigned char* pBuf, int nToSend)
{
	int nSend = 0;
	int nSended = 0;

	do
	{
		nToSend -= nSend;

		nSend = m_pSocket->Send(pBuf + nSended, nToSend);

		if(nSend == -1)
		{
			SetStatus(tsBreakdown);
			FprintfStderrLog("TANDEM_OFFLINE", -1, NULL, 0);
			break;
		}

		if(nSend < nToSend)
			nSended += nSend;
		else
			break;
	}
	while(nSend != nToSend);

	return nSend == nToSend ? true : false;
}

void CSKTandemDAO::OnConnect()
{
	if(m_TandemDAOStatus == tsNone)
	{
		Start();
	}
	else if(m_TandemDAOStatus == tsReconnecting)
	{
		SetStatus(tsServiceOff);

		if(m_pHeartbeat)
		{
			m_pHeartbeat->TriggerGetTIGReplyEvent();//reset heartbeat
		}
	}
	else
	{}
}

void CSKTandemDAO::OnDisconnect()
{
	sleep(5);

	if(m_pTandem)
	{
		m_nTandemNodeIndex += 1;
		m_strHost.assign(reinterpret_cast<const char*>(m_pTandem->GetNode(m_nTandemNodeIndex)->uncaIP), 20);
		m_strPort.assign(reinterpret_cast<const char*>(m_pTandem->GetNode(m_nTandemNodeIndex)->uncaListenPort), 5);
	}

	m_pSocket->Connect(m_strHost, m_strPort); //start & reset heartbeat
}

void CSKTandemDAO::OnData(unsigned char* pBuf, int nSize)
{
}

void CSKTandemDAO::OnControlMessage(const unsigned char* pBuf)
{
	if(memcmp(pBuf, "0008HTBT", 8) == 0)
	{
		unsigned char uncaBuf[] = "0008HTBR";
		SendData(uncaBuf, 8);
	}
	else if(memcmp(pBuf, "0008HTBR", 8) == 0)
	{
	}
	else if(memcmp(pBuf, "0008SVON", 8) == 0)
	{
#ifdef MNTRMSG
		g_MNTRMSG.num_of_SVON;
#endif
		SetStatus(tsServiceOn);
	}
	else if(memcmp(pBuf, "0008SVOF", 8) == 0)
	{
#ifdef MNTRMSG
		g_MNTRMSG.num_of_SVOF;
#endif
		SetStatus(tsServiceOff);
	}
}

TSKTandemDAOStatus CSKTandemDAO::GetStatus()
{
	return m_TandemDAOStatus;
}
void CSKTandemDAO::SetStatus(TSKTandemDAOStatus tsStatus)
{
	pthread_mutex_lock(&m_MutexLockOnSetStatus);//lock

	m_TandemDAOStatus = tsStatus;

	pthread_mutex_unlock(&m_MutexLockOnSetStatus);//unlock
}

void CSKTandemDAO::ReconnectSocket()
{
	sleep(5);
	if(m_pSocket)
	{
		m_pSocket->Disconnect();

		if(m_pTandem)
		{
			m_nTandemNodeIndex += 1;
			m_strHost.assign(reinterpret_cast<const char*>(m_pTandem->GetNode(m_nTandemNodeIndex)->uncaIP), 20);
			m_strPort.assign(reinterpret_cast<const char*>(m_pTandem->GetNode(m_nTandemNodeIndex)->uncaListenPort), 5);
		}

		m_pSocket->Connect(m_strHost, m_strPort); //start & reset heartbeat
	}
}

void CSKTandemDAO::CloseSocket()
{
	if(m_pSocket)
	{
		m_pSocket->Disconnect();
	}
}

void CSKTandemDAO::SetInuse(bool bInuse)
{
	m_bInuse = bInuse;
}

bool CSKTandemDAO::IsInuse()
{
	return m_bInuse;
}
char* CSKTandemDAO::GetReplyMsgOffsetPointer(const unsigned char *pMessageBuf)
{

	char *pSrcReplyMsg = NULL;
	int iOffsetTIGMessage = 0;
	int reply_msg_pointer_offset = 0;

	iOffsetTIGMessage = sizeof(TSKTIGMessage);
	bool IsTandemOrderSuccessful = (memcmp(pMessageBuf + iOffsetTIGMessage, "0000", TIG_TS_REPLY_CODE_LENGTH) == 0);

	if(IsTandemOrderSuccessful)
	{
		reply_msg_pointer_offset = iOffsetTIGMessage + sizeof(TIG_TS_REPLY) - TIG_TS_REPLY_MESSAGE_LENGTH;
	}
	else
	{
		reply_msg_pointer_offset = iOffsetTIGMessage + TIG_TS_REPLY_CODE_LENGTH + TIG_TS_STATUS_CODE_LENGTH;
	}

	pSrcReplyMsg = (char *)(pMessageBuf + reply_msg_pointer_offset);

	return pSrcReplyMsg;
}

