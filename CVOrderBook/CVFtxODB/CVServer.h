#ifndef CCVCLIENTS_H_
#define CCVCLIENTS_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <memory>
#include <iostream>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <fstream>

#include "CVCommon/ICVSocketCallback.h"
#include "CVCommon/CVThread.h"
#include "CVCommon/CVServerSocket.h"
#include "CVCommon/CVSharedMemory.h"
#include "CVServiceHandler.h"
#include "CVGlobal.h"

using namespace std;

class CCVClients: public CCVThread, public ICVSocketCallback
{
	private:
		CCVClients();
		virtual ~CCVClients();
		static CCVClients* instance;
		static pthread_mutex_t ms_mtxInstance;

		CCVServerSocket* m_pServerSocket;
		string m_strListenPort;


		int m_nService;

		pthread_mutex_t m_pmtxClientVectorLock;

#ifdef MNTRMSG
		fstream m_fileLogon;
#endif

	protected:
		void* Run();
		void OnListening();
		void OnShutdown();

	public:
		vector<shared_ptr<CCVClient> > m_vClient;
		string m_strHeartBeatTime;
		string m_strEPIDNum;
		static CCVClients* GetInstance();

		void SetConfiguration(string& strListenPort, string& strHeartBeatTime, string& strEPIDNum, int& nService);

		void CheckClientVector();
		void PushBackClientToVector(shared_ptr<CCVClient>& shpClient);
		void EraseClientFromVector(vector<shared_ptr<CCVClient> >::iterator iter);
		void ShutdownClient(int nSocket);
		bool IsServiceRunning(enum TCVRequestMarket& rmRequestMarket);
#ifdef MNTRMSG
		void FlushLogMessageToFile();
#endif
};
#endif
