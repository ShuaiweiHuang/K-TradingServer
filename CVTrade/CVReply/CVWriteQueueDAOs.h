#ifndef CVWRITEQUEUEDAOS_H_
#define CVWRITEQUEUEDAOS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVPevents.h"
#include "CVWriteQueueDAO.h"
#include "CVCommon/CVThread.h"

#include <vector>

using namespace std;

class CCVReplyDAO;

class CCVWriteQueueDAOs: public CCVThread
{
	private:
		vector<CCVWriteQueueDAO*> m_vWriteQueueDAO;

		static CCVWriteQueueDAOs* instance;

		int m_nWriteQueueDAOCount;
		int m_nWriteQueueDAORoundRobinIndex;

		key_t m_kWriteQueueDAOStartKey;
		key_t m_kWriteQueueDAOEndKey;

		bool m_bAlarm;

		pthread_mutex_t m_MutexLockOnWriteQueueDAO;
		pthread_mutex_t m_MutexLockOnAlarm;

		struct timeval m_timevalEnd;//us
		struct timeval m_timevalStart;//us

		neosmart_event_t m_PEvent;

	protected:
		void* Run();

	public:
		static CCVWriteQueueDAOs* GetInstance();
		CCVWriteQueueDAOs(int nWriteQueueDAOCount, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey);
		virtual ~CCVWriteQueueDAOs();

		CCVWriteQueueDAO* GetAvailableDAO();
		bool IsAllWriteQueueDAOBreakdown();
		void ReConstructWriteQueueDAO();
		void SetAlarm(bool bAlarm);
		bool IsRunningTimeReasonable(CCVWriteQueueDAO* pWriteQueueDAO);
		void ReSendReplyMessage(CCVWriteQueueDAO* pWriteQueueDAO);
};
#endif
