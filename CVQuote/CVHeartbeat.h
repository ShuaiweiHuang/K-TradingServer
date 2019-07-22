#ifndef SKHEARTBEAT_H_
#define SKHEARTBEAT_H_

#ifdef _WIN32
#include <Windows.h>
#endif
#include <string>
#include "CVCommon/CVPevents.h"
#include "CVCommon/CVThread.h"
#include "CVCommon/ISKHeartbeatCallback.h"

using namespace neosmart;
using namespace std;

class ISKHeartbeatCallback;

class CSKHeartbeat: public CSKThread
{
	private:
		int m_nTimeInterval;
		int m_nIdleTime;

		ISKHeartbeatCallback* m_pHeartbeatCallback;
		neosmart_event_t m_PEvent[2];

	protected:
		void* Run();

	public:
		CSKHeartbeat(ISKHeartbeatCallback* pHeartbeatCallback);
		virtual ~CSKHeartbeat();
		void SetTimeInterval(int);
		void TriggerGetReplyEvent();
		void TriggerTerminateEvent();

};
#endif
