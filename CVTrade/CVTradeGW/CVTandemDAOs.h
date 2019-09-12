#ifndef SKTANDEMDAOS_H_
#define SKTANDEMDAOS_H_

#include <vector>
#include "CVPevents.h"

#include "CVTandemDAO.h"

using namespace neosmart;


class CSKTandemDAOs: public CSKThread
{
	private:
		CSKTandemDAOs();
		static CSKTandemDAOs* instance;
		static pthread_mutex_t ms_mtxInstance;

		vector<CSKTandemDAO*> m_vTandemDAO;

		string m_strService;

		int m_nDefaultNumberOfTandemDAO;
		int m_nMaxNumberOfTandemDAO;
		int m_nRoundRobinIndexOfTandemDAO;
		int m_nNumberOfWriteQueueDAO;
		key_t m_kWriteQueueDAOStartKey;
		key_t m_kWriteQueueDAOEndKey;

		neosmart_event_t m_PEvent;

		pthread_mutex_t m_MutexLockOnAddAvailableDAO;
		pthread_mutex_t m_MutexLockOnGetAvailableDAO;
		pthread_mutex_t m_MutexLockOnTriggerAddAvailableDAOEvent;

		int m_nToSetTandemDAOID;

	protected:
		void* Run();
		void AddAvailableDAO();

	public:
		static CSKTandemDAOs* GetInstance();
		virtual ~CSKTandemDAOs();

		CSKTandemDAO* GetAvailableDAO();
		bool IsDAOsFull();
		void TriggerAddAvailableDAOEvent();

		void SetConfiguration(string strService, int nInitialConnection, int nMaximumConnection,
							  int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey);
		void StartUpDAOs();

		string& GetService();
};
#endif
