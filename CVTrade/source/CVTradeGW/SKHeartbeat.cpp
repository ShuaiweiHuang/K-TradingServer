#include <cstdio>
#include <cstring>
#include <cerrno> 
#include <sys/socket.h>
#include <unistd.h>
#include "SKHeartbeat.h"
#include "SKTandemDAO.h"
#include <iostream>

using namespace std;

class CSKTandemDAO;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKHeartbeat::CSKHeartbeat(int nTimeIntervals)
{
	m_nTimeIntervals = nTimeIntervals;

	m_nIdleTime = 0;

	m_pTandemDAO= NULL;

	m_PEvent = CreateEvent();
}

CSKHeartbeat::~CSKHeartbeat()
{
	DestroyEvent(m_PEvent);
}

void* CSKHeartbeat::Run()
{
	unsigned char uncaBuf[] = "0008HTBT";

	if(m_pTandemDAO)
	{
		while(m_pTandemDAO->GetStatus() != tsNone)
		{
			int result = WaitForEvent(m_PEvent,5000);

			if(result == WAIT_TIMEOUT)
			{
				m_nIdleTime += 5;

				if(m_nIdleTime == m_nTimeIntervals + 5 && m_pTandemDAO != NULL)
				{
					FprintfStderrLog("TANDEM_HEARTBEAT_LOST", -1, reinterpret_cast<unsigned char*>(m_pTandemDAO->m_caTandemDAOID), 4);
					m_pTandemDAO->CloseSocket();
					m_nIdleTime = 0;
				}
				else if(m_nIdleTime == m_nTimeIntervals && m_pTandemDAO != NULL)
				{
					FprintfStderrLog("TANDEM_HEARTBEAT_REQUEST", 0, reinterpret_cast<unsigned char*>(m_pTandemDAO->m_caTandemDAOID), 4);
					int nSendData = m_pTandemDAO->SendData(uncaBuf,8);//callback to send heartbeat
				}
				else if(m_nIdleTime < m_nTimeIntervals)
				{
					continue;
				}
			}
			else if(result == 0)
			{
				FprintfStderrLog("TANDEM_HEARTBEAT_GET_REPLY", 0, reinterpret_cast<unsigned char*>(m_pTandemDAO->m_caTandemDAOID), 4);
				m_nIdleTime = 0;
			}
			else
			{
				FprintfStderrLog("TANDEM_HEARTBEAT_ERROR_IN_WAIT", 0, reinterpret_cast<unsigned char*>(m_pTandemDAO->m_caTandemDAOID), 4);
			}
		}
	}
	return NULL;
}

void CSKHeartbeat::SetCallback(CSKTandemDAO *pTandemDAO)
{
	m_pTandemDAO = pTandemDAO;
}

void CSKHeartbeat::TriggerGetTIGReplyEvent()
{
	SetEvent(m_PEvent);
	//neosmart::SetEvent(m_PEvent);
}
