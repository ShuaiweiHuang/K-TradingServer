#ifndef SKREADQUEUEDAOS_H_
#define SKREADQUEUEDAOS_H_

#include <vector>

#include "CVReadQueueDAO.h"
#include "CVCommon/CVSharedMemory.h"

using namespace std;

#define AMOUNT_OF_HASH_SIZE 10000

struct TSKOrderNumberHash//to delete
{
	unsigned char uncaOrderNumber[13];
	bool bAvailable;
};

class CSKReadQueueDAOs: public CSKThread
{
	private:
		CSKReadQueueDAOs();
		static CSKReadQueueDAOs* instance;
		static pthread_mutex_t ms_mtxInstance;

		vector<CSKReadQueueDAO*> m_vReadQueueDAO;

		string m_strService;
		string m_strOTSID;

		struct TSKOrderNumberHash m_OrderNumberHash[AMOUNT_OF_HASH_SIZE];

		CSKSharedMemory* m_pSharedMemoryOfSerialNumber;
		long* m_pSerialNumber;
		key_t m_kTIGNumberSharedMemoryKey;

		int m_nNumberOfReadQueueDAO;
		key_t m_kReadQueueDAOStartKey;
		key_t m_kReadQueueDAOEndKey;

		pthread_mutex_t m_MutexLockOnGetDAO;
		pthread_mutex_t m_MutexLockOnSerialNumber;

	protected:
		void* Run();

	public:
		static CSKReadQueueDAOs* GetInstance();
		virtual ~CSKReadQueueDAOs();

		void AddDAO(key_t key);
		void RemoveDAO(key_t key);

		CSKReadQueueDAO* GetDAO(key_t key);

		void InsertOrderNumberToHash(long lTIGNumber, unsigned char* pOrderNumber);
		bool GetOrderNumberHashStatus(long lTIGNumber);
		void SetOrderNumberHashStatus(long lTIGNumber, bool bStatus);
		void GetOrderNumberFromHash(long lTIGNumber, unsigned char* pBuf);

		void SetConfiguration(string strService, string strOTSID, int nNumberOfReadQueueDAO, key_t kReadQueueDAOStartKey, 
							  key_t kReadQueueDAOEndKey, key_t kTIGNumberSharedMemoryKey);
		void StartUpDAOs();

		long GetSerialNumber();
};
#endif
