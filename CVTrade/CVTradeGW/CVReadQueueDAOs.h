#ifndef CVREADQUEUEDAOS_H_
#define CVREADQUEUEDAOS_H_

#include <vector>

#include "CVReadQueueDAO.h"
#include "CVCommon/CVSharedMemory.h"

using namespace std;

#define AMOUNT_OF_HASH_SIZE 10000

struct TCVOrderNumberHash//to delete
{
	unsigned char uncaOrderNumber[13];
	bool bAvailable;
};

class CCVReadQueueDAOs: public CCVThread
{
	private:
		CCVReadQueueDAOs();
		static CCVReadQueueDAOs* instance;
		static pthread_mutex_t ms_mtxInstance;

		vector<CCVReadQueueDAO*> m_vReadQueueDAO;

		string m_strService;
		string m_strOTSID;

		struct TCVOrderNumberHash m_OrderNumberHash[AMOUNT_OF_HASH_SIZE];

		CCVSharedMemory* m_pSharedMemoryOfSerialNumber;
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
		static CCVReadQueueDAOs* GetInstance();
		virtual ~CCVReadQueueDAOs();

		void AddDAO(key_t key);
		void RemoveDAO(key_t key);

		CCVReadQueueDAO* GetDAO(key_t key);

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
