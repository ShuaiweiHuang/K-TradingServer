#ifndef CCVCLIENTS_H_
#define CCVCLIENTS_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <sys/time.h>
#include <fstream>

#include "CVInterface/ICVSocketCallback.h"
#include "CVCommon/CVSharedMemory.h"
#include "CVCommon/CVThread.h"
#include "CVNet/CVServerSocket.h"
#include "CVClient.h"
#include "CVHeartbeat.h"

using namespace std;

#define AMOUNT_OF_CLIENT_OBJECT_HASH 10000

#ifdef TIMETEST

#define AMOUNT_OF_CLIENT_OBJECT 1000
#define AMOUNT_OF_TIME_POINT 12

enum TCVTimePoint
{
	tpClientToQueueDAO = 4,
	tpQueueDAO_Reply_ClinetToProxy,
	tpQueueProcessStart,
	tpQueueProcessEnd
};

extern void InsertTimeLogToSharedMemory(struct timeval *timeval_Start, struct timeval *timeval_End, enum TCVTimePoint tpTimePoint, long lOrderNumber);
#endif

struct MNTRMSGS
{
	int num_of_thread_Current;
	int num_of_thread_Max;
	double process_vm_mb;
	long network_delay_ms;
};

class CCVClients: public CCVThread, public ICVSocketCallback
{
	private:
		CCVClients();
		virtual ~CCVClients();
		static CCVClients* instance;
		static pthread_mutex_t ms_mtxInstance;

		CCVServerSocket* m_pServerSocket;

		CCVSharedMemory* m_pSharedMemoryOfSerialNumber;
		CCVSharedMemory* m_pSharedMemoryOfKeyNumber;

		vector<CCVClient*> m_vOnlineClient;
		vector<CCVClient*> m_vOfflineClient;

		CCVClient* m_ClientObjectHash[AMOUNT_OF_CLIENT_OBJECT_HASH];

		long* m_pSerialNumber;

		string m_strService;
		string m_strListenPort;

		key_t m_kSerialNumberSharedMemoryKey;

		pthread_mutex_t m_MutexLockOnSerialNumber;
		pthread_mutex_t m_MutexLockOnOnlineClientVector;
		pthread_mutex_t m_MutexLockOnOfflineClientVector;
		vector<char*> m_vProxyNode;
	protected:
		void* Run();
		void OnListening();
		void OnShutdown();

	public:
		static CCVClients* GetInstance();
		CCVHeartbeat* m_pHeartbeat;
		CCVClient* GetClientFromHash(long lOrderNumber);
		void InsertClientToHash(long lOrderNumber, CCVClient* pClient);
		void RemoveClientFromHash(long lOrderNumber);

		void GetSerialNumber(char* pSerialNumber);

		void SetConfiguration(string strService, string strListenPort, key_t kSerialNumberSharedMemoryKey);

		void CheckOnlineClientVector();
		void MoveOnlineClientToOfflineVector(CCVClient* pClient);
		void PushBackClientToOnlineVector(CCVClient* pClient);
		void ClearOfflineClientVector();

		void ShutdownClient(int nSocket);

		bool IsProxy(char* pIP);
};
#endif
