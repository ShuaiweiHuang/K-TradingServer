#ifndef CVHEARTBEAT_H_
#define CVHEARTBEAT_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"
#include "CVCommon/CVThread.h"

using namespace neosmart;
class CCVClient;

class CCVHeartbeat: public CCVThread
{
	private:
		int m_nTimeIntervals;
		int m_nIdleTime;

		CCVClient *m_pClient;
		neosmart_event_t m_PEvent[2];

	protected:
		void* Run();

	public:
		CCVHeartbeat(int nTimeIntervals = 30);
		virtual ~CCVHeartbeat();

		void SetCallback(CCVClient *pClient);

		void TriggerGetClientReplyEvent();
		void Terminate();
};
#endif
