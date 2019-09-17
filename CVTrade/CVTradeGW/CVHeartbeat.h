#ifndef CVHEARTBEAT_H_
#define CVHEARTBEAT_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVPevents.h"
#include "CVCommon/CVThread.h"

using namespace neosmart;
class CCVTandemDAO;

class CCVHeartbeat: public CCVThread
{
	private:
		int m_nTimeIntervals;
		int m_nIdleTime;

		CCVTandemDAO *m_pTandemDAO;
		neosmart_event_t m_PEvent;

	protected:
		void* Run();

	public:
		CCVHeartbeat(int nTimeIntervals = 30);
		virtual ~CCVHeartbeat();

		void SetCallback(CCVTandemDAO *pTandemDAO);
		void TriggerGetTIGReplyEvent();
};
#endif
