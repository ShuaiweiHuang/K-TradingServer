#ifndef CVTANDEMDAOS_H_
#define CVTANDEMDAOS_H_

#include <vector>
#include "CVPevents.h"

#include "CVTandemDAO.h"

using namespace neosmart;


class CCVTandemDAOs: public CCVThread
{
	private:
		CCVTandemDAOs();
		static CCVTandemDAOs* instance;
		static pthread_mutex_t ms_mtxInstance;

		vector<CCVTandemDAO*> m_vTandemDAO;

		string m_strService;

		int m_nDefaultNumberOfTandemDAO;
		int m_nMaxNumberOfTandemDAO;
		int m_nRoundRobinIndexOfTandemDAO;
		int m_nNumberOfWriteQueueDAO;
		key_t m_kWriteQueueDAOStartKey;
		key_t m_kWriteQueueDAOEndKey;
		key_t m_kQueueDAOMonitorKey;
		neosmart_event_t m_PEvent;

		pthread_mutex_t m_MutexLockOnAddAvailableDAO;
		pthread_mutex_t m_MutexLockOnGetAvailableDAO;
		pthread_mutex_t m_MutexLockOnTriggerAddAvailableDAOEvent;

		int m_nToSetTandemDAOID;

	protected:
		void* Run();
		void AddAvailableDAO();

	public:
		static CCVTandemDAOs* GetInstance();
		virtual ~CCVTandemDAOs();

		CCVTandemDAO* GetAvailableDAO();
		bool IsDAOsFull();
		void TriggerAddAvailableDAOEvent();

		void SetConfiguration(string strService, int nInitialConnection, int nMaximumConnection,
					int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey, key_t kQueueDAOMonitorKey);
		void StartUpDAOs();

		string& GetService();
};
#endif
