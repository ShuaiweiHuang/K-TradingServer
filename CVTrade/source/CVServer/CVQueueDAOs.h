#ifndef CVQUEUEDAOS_H_
#define CVQUEUEDAOS_H_

#include <vector>
#include <string>

#include "CVQueueDAO.h"

using namespace std;

class CCVQueueDAOs: public CCVThread
{
	private:
		CCVQueueDAOs();
		virtual ~CCVQueueDAOs();
		static CCVQueueDAOs* instance;
		static pthread_mutex_t ms_mtxInstance;

		vector<CCVQueueDAO*> m_vQueueDAO;

		int m_nNumberOfQueueDAO;
		key_t m_kQueueDAOWriteStartKey;
		key_t m_kQueueDAOWriteEndKey;
		key_t m_kQueueDAOReadStartKey;
		key_t m_kQueueDAOReadEndKey;

		key_t m_kRoundRobinIndexOfQueueDAO;

		string m_strService;

		pthread_mutex_t m_MutexLockOnGetDAO;

	protected:
		void* Run();

	public:
		static CCVQueueDAOs* GetInstance();

		void AddDAO(key_t kSendKey,key_t kRecvKey);
		void RemoveDAO(key_t key);//Sendkey

		CCVQueueDAO* GetDAO();//SendKey

		void SetConfiguration(string strService, int nNumberOfQueueDAO, key_t kQueueDAOWriteStartKey, key_t kQueueDAOWriteEndKey,
						      key_t kQueueDAOReadStartKey, key_t kQueueDAOReadEndKey);
		void StartUpDAOs();
};
#endif
