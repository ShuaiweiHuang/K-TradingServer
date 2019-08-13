#ifndef SKWRITEQUEUEDAOS_H_
#define SKWRITEQUEUEDAOS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVPevents.h"
#include "CVWriteQueueDAO.h"
#include "CVCommon/CVThread.h"

#include <vector>

using namespace std;

class CSKTandemDAO;

class CSKWriteQueueDAOs: public CSKThread
{
	private:
		vector<CSKWriteQueueDAO*> m_vWriteQueueDAO;

		static CSKWriteQueueDAOs* instance;

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
		static CSKWriteQueueDAOs* GetInstance();
		CSKWriteQueueDAOs(int nWriteQueueDAOCount, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey);
		virtual ~CSKWriteQueueDAOs();

		CSKWriteQueueDAO* GetAvailableDAO();
		bool IsAllWriteQueueDAOBreakdown();
		void ReConstructWriteQueueDAO();
		void SetAlarm(bool bAlarm);
		bool IsRunningTimeReasonable(CSKWriteQueueDAO* pWriteQueueDAO);
		void ReSendReplyMessage(CSKWriteQueueDAO* pWriteQueueDAO);
};
#endif
