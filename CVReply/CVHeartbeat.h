#ifndef CVHEARTBEAT_H_
#define CVHEARTBEAT_H_

#ifdef _WIN32
#include <Windows.h>
#endif
#include <string>
#include "CVCommon/CVPevents.h"
#include "CVCommon/CVThread.h"
#include "CVCommon/ICVHeartbeatCallback.h"

using namespace neosmart;
using namespace std;

class ICVHeartbeatCallback;

class CCVHeartbeat: public CCVThread
{
	private:
		int m_nTimeInterval;

		ICVHeartbeatCallback* m_pHeartbeatCallback;
		neosmart_event_t m_PEvent[2];

	protected:
		void* Run();

	public:
		int m_nIdleTime;
		CCVHeartbeat(ICVHeartbeatCallback* pHeartbeatCallback);
		virtual ~CCVHeartbeat();
		void SetTimeInterval(int);
		void TriggerGetReplyEvent();
		void TriggerTerminateEvent();

};
#endif
