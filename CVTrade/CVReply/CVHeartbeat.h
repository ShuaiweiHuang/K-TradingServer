#ifndef CVHEARTBEAT_H_
#define CVHEARTBEAT_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVPevents.h"
#include "CVCommon/CVThread.h"

using namespace neosmart;
class CCVReplyDAO;

class CCVHeartbeat: public CCVThread
{
	private:
		int m_nTimeIntervals;
		int m_nIdleTime;

		CCVReplyDAO *m_pTandemDAO;
		neosmart_event_t m_PEvent;

	protected:
		void* Run();

	public:
		CCVHeartbeat(int nTimeIntervals = 30);
		virtual ~CCVHeartbeat();

		void SetCallback(CCVReplyDAO *pTandemDAO);
		void TriggerGetTIGReplyEvent();
};
#endif
