#include <cstdio>
#include <cstring>
#include <cerrno> 
#include <sys/socket.h>
#include <unistd.h>

#include "CVHeartbeat.h"
#include <iostream>

using namespace std;

CCVHeartbeat::CCVHeartbeat(ICVHeartbeatCallback* pHeartbeatCallback)
{
	m_nTimeInterval = 0;
	m_nIdleTime = 0;

	m_pHeartbeatCallback = pHeartbeatCallback;

	m_PEvent[0] = CreateEvent(0,0);
	m_PEvent[1] = CreateEvent(0,0);
	Start();
}

CCVHeartbeat::~CCVHeartbeat()
{
	TriggerTerminateEvent();
	DestroyEvent(m_PEvent[0]);
	DestroyEvent(m_PEvent[1]);

}

void* CCVHeartbeat::Run()
{
	while(m_pHeartbeatCallback)
	{
		int nIndex = -1;
		int nResult = WaitForMultipleEvents(m_PEvent, 2, false, 5000, nIndex);

		if(nResult != 0 && nResult != WAIT_TIMEOUT)
		{
			if(m_pHeartbeatCallback)
				m_pHeartbeatCallback->OnHeartbeatError(0, "HEARTBEAT_ERROR_IN_WAIT");
			break;
		}

		if(nResult == WAIT_TIMEOUT)
		{
			m_nIdleTime += 5;

			if(m_nIdleTime <= m_nTimeInterval)
			{
				continue;
			}
			else if(m_nIdleTime >= m_nTimeInterval && m_nIdleTime <= m_nTimeInterval+10)
			{
				if(m_pHeartbeatCallback)
					m_pHeartbeatCallback->OnHeartbeatRequest();
				continue;
			}
			else
			{
				if(m_pHeartbeatCallback)
					m_pHeartbeatCallback->OnHeartbeatLost();
				break;
			}
		}
		else // Get event
		{
			if(nIndex == 0)//get client reply
			{
				m_nIdleTime = 0;
			}
			else if(nIndex == 1)//terminate heartbeat
			{
				if(m_pHeartbeatCallback)
					m_pHeartbeatCallback->OnHeartbeatError(0, "HEARTBEAT_TERMINATE");
				break;
			}
			else
			{
				if(m_pHeartbeatCallback)
					m_pHeartbeatCallback->OnHeartbeatError(nIndex, "HEARTBEAT_UNEXPECTED_INDEX_ERROR");
				break;
			}
		}
	}
	return NULL;
}

void CCVHeartbeat::SetTimeInterval(int nTimeInterval)
{
	m_nTimeInterval = nTimeInterval;
}

void CCVHeartbeat::TriggerGetReplyEvent()
{
	SetEvent(m_PEvent[0]);
}

void CCVHeartbeat::TriggerTerminateEvent()
{
	SetEvent(m_PEvent[1]);
}
