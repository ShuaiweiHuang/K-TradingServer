#ifndef SKSERVERS_H_
#define SKSERVERS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"
#include "CVWebConn.h"
#include <vector>
#include <string>

using namespace neosmart;
using namespace std;

struct TSKServerInfo
{
	string strWeb;
	string strQstr;
	string strName;
};

struct TSKConfig
{
	int nServerCount;
	vector<struct TSKServerInfo*> vServerInfo;
};

class CSKServers //public CSKThread
{
	private:
		CSKServers();
		virtual ~CSKServers();
		static CSKServers* instance;
		static pthread_mutex_t ms_mtxInstance;
		int m_alive_check;
		vector<struct TSKConfig*> m_vServerConfig;
		//vector<vector<vector<CSKServer*> > > m_vvvServerPool;
		vector<CSKServer*> m_vServerPool;

	protected:
		void AddFreeServer(enum TSKRequestMarket rmRequestMarket, int nPoolIndex);

	public:
		static CSKServers* GetInstance();
		void SetConfiguration(struct TSKConfig* pstruConfig);
		void StartUpServers();
		void RestartUpServers();
		CSKServer* GetServerByName(string name);
};
#endif
