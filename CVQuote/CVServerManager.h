#ifndef CSKCLIENTS_H_
#define CSKCLIENTS_H_

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

#include "CVCommon/ISKSocketCallback.h"
#include "CVCommon/CVThread.h"
#include "CVCommon/CVServerSocket.h"
#include "CVCommon/CVSharedMemory.h"
#include "CVServer.h"
#include "CVGlobal.h"

using namespace std;

class CSKClients: public CSKThread, public ISKSocketCallback
{
	private:
		CSKClients();
		virtual ~CSKClients();
		static CSKClients* instance;
		static pthread_mutex_t ms_mtxInstance;

		CSKServerSocket* m_pServerSocket;
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
		vector<shared_ptr<CSKClient> > m_vClient;
		string m_strHeartBeatTime;
		string m_strEPIDNum;
		static CSKClients* GetInstance();

		void SetConfiguration(string& strListenPort, string& strHeartBeatTime, string& strEPIDNum, int& nService);

		void CheckClientVector();
		void PushBackClientToVector(shared_ptr<CSKClient>& shpClient);
		void EraseClientFromVector(vector<shared_ptr<CSKClient> >::iterator iter);
		void ShutdownClient(int nSocket);
		bool IsServiceRunning(enum TSKRequestMarket& rmRequestMarket);
#ifdef MNTRMSG
		void FlushLogMessageToFile();
#endif
};
#endif
