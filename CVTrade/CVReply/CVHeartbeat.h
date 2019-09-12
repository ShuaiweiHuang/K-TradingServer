#ifndef SKHEARTBEAT_H_
#define SKHEARTBEAT_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVPevents.h"
#include "CVCommon/CVThread.h"

using namespace neosmart;
class CSKTandemDAO;

class CSKHeartbeat: public CSKThread
{
	private:
		int m_nTimeIntervals;
		int m_nIdleTime;

		CSKTandemDAO *m_pTandemDAO;
		neosmart_event_t m_PEvent;

	protected:
		void* Run();

	public:
		CSKHeartbeat(int nTimeIntervals = 30);
		virtual ~CSKHeartbeat();

		void SetCallback(CSKTandemDAO *pTandemDAO);
		void TriggerGetTIGReplyEvent();
};
#endif
