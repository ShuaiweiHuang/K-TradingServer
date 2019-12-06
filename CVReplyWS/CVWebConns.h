#ifndef CVSERVERS_H_
#define CVSERVERS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"
#include "CVWebConn.h"
#include <vector>
#include <string>
#include "../CVInclude/Monitor.h"

#define PACKETSIZE  64

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

struct MNTRMSGS
{
	int num_of_thread_Current;
	int num_of_thread_Max;
	long network_delay_ms;
	double process_vm_mb;
	int cpu_loading;
};

class CCVServers: public ICVHeartbeatCallback //public CCVThread
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
		CCVHeartbeat* m_pHeartbeat;

	protected:
		void AddFreeServer(enum TCVRequestMarket rmRequestMarket, int nPoolIndex);
		void OnHeartbeatLost();
		void OnHeartbeatRequest();
		void OnHeartbeatError(int nData, const char* pErrorMessage);

	public:
		static CCVServers* GetInstance();
		void SetConfiguration(struct TCVConfig* pstruConfig);
		void StartUpServers();
		void CheckClientVector();
		CCVServer* GetServerByName(string name);
};
#endif
