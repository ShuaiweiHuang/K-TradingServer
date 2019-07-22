#include <cstdio>
#include <cstring>
#include <cerrno> 
#include <sys/socket.h>
#include <unistd.h>
//#include <assert.h>

#include "CVHeartbeat.h"
#include <iostream>

using namespace std;

CSKHeartbeat::CSKHeartbeat(ISKHeartbeatCallback* pHeartbeatCallback)
{
	m_nTimeInterval = 0;
	m_nIdleTime = 0;

	m_pHeartbeatCallback = pHeartbeatCallback;

	m_PEvent[0] = CreateEvent(0,0);
	m_PEvent[1] = CreateEvent(0,0);
	Start();
}

CSKHeartbeat::~CSKHeartbeat()
{
	DestroyEvent(m_PEvent[0]);
	DestroyEvent(m_PEvent[1]);
}

void* CSKHeartbeat::Run()
{
	while(m_pHeartbeatCallback)
	{
		int nIndex = -1;
		int nResult = WaitForMultipleEvents(m_PEvent, 2, false, 10000, nIndex);

		if(nResult != 0 && nResult != WAIT_TIMEOUT)
		{
			if(m_pHeartbeatCallback)
				m_pHeartbeatCallback->OnHeartbeatError(0, "HEARTBEAT_ERROR_IN_WAIT");
			break;
		}

		if(nResult == WAIT_TIMEOUT)
		{
			m_nIdleTime += 10;
			if(m_nIdleTime < m_nTimeInterval)
			{
				continue;
			}
			else if(m_nIdleTime >= m_nTimeInterval)
			{
				if(m_pHeartbeatCallback)
					m_pHeartbeatCallback->OnHeartbeatRequest();
				m_nIdleTime = 0;
			}
			else
			{
				if(m_pHeartbeatCallback)
					m_pHeartbeatCallback->OnHeartbeatError(m_nIdleTime, "HEARTBEAT_IDLETIME_ERROR");
				break;
			}
		}
		else
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

void CSKHeartbeat::SetTimeInterval(int nTimeInterval)
{
	m_nTimeInterval = nTimeInterval;
}

void CSKHeartbeat::TriggerGetReplyEvent()
{
	SetEvent(m_PEvent[0]);
}

void CSKHeartbeat::TriggerTerminateEvent()
{
	SetEvent(m_PEvent[1]);
}
