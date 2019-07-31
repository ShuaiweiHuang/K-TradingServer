#ifndef SKREADQUEUEDAOS_H_
#define SKREADQUEUEDAOS_H_

#include <vector>

#include "SKReadQueueDAO.h"
#include "SKCommon/SKSharedMemory.h"

using namespace std;

#define AMOUNT_OF_HASH_SIZE 10000

#ifdef MNTRMSG
struct MNTRMSGS
{
    int num_of_thread_Current;
    int num_of_thread_Max;
    int num_of_order_Received;
    int num_of_order_Sent;
    int num_of_order_Reply;
	int num_of_SVOF;
	int num_of_SVON;
    int num_of_service_Available;
};
#endif

#ifdef TIMETEST
enum TSKTimePoint
{
	tpQueueDAOToTandem = 8,
	tpTandemToQueueDAO,
	tpTandemProcessStart,
	tpTandemProcessEnd
};

extern void InsertTimeLogToSharedMemory(struct timeval *timeval_Start, struct timeval *timeval_End, enum TSKTimePoint tpTimePoint, long lOrderNumber);
#endif


struct TTestTime//timetest
{
	char time_log1[8];
	char time_log3[8];
	char time_log4[8];
	char time_log5[8];
	char time_log6[8];
	char time_log7[8];
	char time_log8[8];
	char time_log9[8];
	char time_log10[8];
};

struct TSKOrderNumberHash//to delete
{
	unsigned char uncaOrderNumber[13];
	bool bAvailable;
	struct TTestTime test_time;
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
		TTestTime& GetOrderNumberHashTime(long lSerialNumber);//timetest

		void SetConfiguration(string strService, string strOTSID, int nNumberOfReadQueueDAO, key_t kReadQueueDAOStartKey, 
							  key_t kReadQueueDAOEndKey, key_t kTIGNumberSharedMemoryKey);
		void StartUpDAOs();

		long GetSerialNumber();
};
#endif
