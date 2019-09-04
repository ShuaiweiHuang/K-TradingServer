#ifndef CVSERVERS_H_
#define CVSERVERS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"
#include "CVWebConn.h"
#include <vector>
#include <string>

using namespace neosmart;
using namespace std;

struct TCVServerInfo
{
	string strWeb;
	string strQstr;
	string strName;
};

struct TCVConfig
{
	int nServerCount;
	vector<struct TCVServerInfo*> vServerInfo;
};

class CCVServers //public CCVThread
{
	private:
		CCVServers();
		virtual ~CCVServers();
		static CCVServers* instance;
		static pthread_mutex_t ms_mtxInstance;
		int m_alive_check;
		vector<struct TCVConfig*> m_vServerConfig;
		//vector<vector<vector<CCVServer*> > > m_vvvServerPool;
		vector<CCVServer*> m_vServerPool;

	protected:
		void AddFreeServer(enum TCVRequestMarket rmRequestMarket, int nPoolIndex);

	public:
		static CCVServers* GetInstance();
		void SetConfiguration(struct TCVConfig* pstruConfig);
		void StartUpServers();
		void RestartUpServers();
		void CheckClientVector();
		CCVServer* GetServerByName(string name);
};
#endif
