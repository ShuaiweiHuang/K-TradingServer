#include <cstdio>
#include <cstring>
#include <cerrno> 
#include <sys/socket.h>
#include <unistd.h>

#include "CVHeartbeat.h"
#include "CVClient.h"
#include "CVClients.h"
#include "../include/CVGlobal.h"
#include <iostream>

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVHeartbeat::CCVHeartbeat(int nTimeIntervals)
{
	m_nTimeIntervals = nTimeIntervals;

	m_nIdleTime = 0;

	m_pClient = NULL;

	m_PEvent[0] = CreateEvent();
	m_PEvent[1] = CreateEvent();
}

CCVHeartbeat::~CCVHeartbeat()
{
	DestroyEvent(m_PEvent[0]);
	DestroyEvent(m_PEvent[1]);
}

void* CCVHeartbeat::Run()
{
	unsigned char uncaSendBuf[7];
	memset(uncaSendBuf, 0, 7);

	uncaSendBuf[0] = 0x1b;
	uncaSendBuf[1] = HEARTBEATREQ;
	memcpy(uncaSendBuf + 2, "HBRQ", 4);

	if(m_pClient)
	{
		while(m_pClient->GetStatus() == csLogoning || m_pClient->GetStatus() == csOnline)
		{
			int nIndex = -1;
			int nResult = WaitForMultipleEvents(m_PEvent, 2, false, 5000, nIndex);

			if(nResult != 0 && nResult != WAIT_TIMEOUT) // not timeout and not get event
			{
				FprintfStderrLog("HBRQ_ERROR_IN_WAIT", -1, reinterpret_cast<unsigned char*>(m_pClient->m_ClientAddrInfo.caIP), sizeof(m_pClient->m_ClientAddrInfo.caIP));
			}

			if(nResult == WAIT_TIMEOUT) //Timeout
			{
				m_nIdleTime += 5;

				if(m_nIdleTime == m_nTimeIntervals + 10)
				{
					FprintfStderrLog("HBRQ_CLIENT_HEARTBEAT_LOST", -1,
					reinterpret_cast<unsigned char*>(m_pClient->m_ClientAddrInfo.caIP), sizeof(m_pClient->m_ClientAddrInfo.caIP));

					if(m_pClient)
					{
						m_pClient->SetStatus(csOffline);//end the session
					}
				}
				else if(m_nIdleTime == m_nTimeIntervals || m_nIdleTime == m_nTimeIntervals + 5)
				{
					if(m_pClient)
					{
						FprintfStderrLog("HBRQ_SEND_HBRQ_TO_CLIENT", -1,
						reinterpret_cast<unsigned char*>(m_pClient->m_ClientAddrInfo.caIP), sizeof(m_pClient->m_ClientAddrInfo.caIP));
						m_pClient->SendData(uncaSendBuf,6);//callback to send heartbeat
					}
				}
				else if(m_nIdleTime < m_nTimeIntervals)
				{
					continue;
				}
			}
			else //Get event
			{
				if(nIndex == 0)
				{
					FprintfStderrLog("HBRQ_GET_CLIENT_REPLY", -1,
					reinterpret_cast<unsigned char*>(m_pClient->m_ClientAddrInfo.caIP), sizeof(m_pClient->m_ClientAddrInfo.caIP));
					m_nIdleTime = 0;
				}
				else if(nIndex == 1)
				{
					FprintfStderrLog("HBRQ_TERMINATE", -1,
					reinterpret_cast<unsigned char*>(m_pClient->m_ClientAddrInfo.caIP), sizeof(m_pClient->m_ClientAddrInfo.caIP));
					break;
				}
				else
				{
					FprintfStderrLog("HBRQ_ERROR_INDEX", -1,
					reinterpret_cast<unsigned char*>(m_pClient->m_ClientAddrInfo.caIP), sizeof(m_pClient->m_ClientAddrInfo.caIP));
					break;
				}
			}
		}
	}
	return NULL;
}

void CCVHeartbeat::SetCallback(CCVClient *pClient)
{
	m_pClient = pClient;
}

void CCVHeartbeat::TriggerGetClientReplyEvent()
{
	SetEvent(m_PEvent[0]);
	//neosmart::SetEvent(m_PEvent);
}

void CCVHeartbeat::Terminate()
{
	SetEvent(m_PEvent[1]);
}
