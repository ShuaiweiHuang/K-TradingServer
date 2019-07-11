#ifndef SKSERVERS_H_
#define SKSERVERS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "CVCommon/CVPevents.h"
#include <vector>
#include <string>

#include "CVWebClient.h"

using namespace neosmart;
using namespace std;

struct TSKServerInfo
{
	string strWeb;
	string strQstr;
};

struct TSKConfig
{
	int nPoolCount;
	int nMaximumServerObjectCount;
	int nInitailServerObjectCount;
	int nIncrementalServerObjectCount;

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

		vector<struct TSKConfig*> m_vServerConfig;//TS,TF,OF,OS
		vector<vector<vector<CSKServer*> > > m_vvvServerPool;//[Market][ServerCount*PoolCount][ServerObjectCount]
		vector<vector<pthread_mutex_t*> > m_vvServerPoolLock;//[Market][ServerCount*PoolCount]
		vector<vector<int*> > m_vvInuseServerCount;//[Market][ServerCount*PoolCount]
		vector<vector<pthread_mutex_t*> > m_vvInuseServerCountLock;//[Market][ServerCount*PoolCount]

	protected:
		void AddFreeServer(enum TSKRequestMarket rmRequestMarket, int nPoolIndex);

	public:
#ifdef MNTRMSG
		void UpdateAvailableServerNum(enum TSKRequestMarket rmRequestMarket);
#endif
		static CSKServers* GetInstance();

		CSKServer* GetFreeServerObject(enum TSKRequestMarket rmRequestMarket);
		int GetLeastUseServer(enum TSKRequestMarket rmRequestMarket);
		int GetLeastUsePool(enum TSKRequestMarket rmRequestMarket, int nLeastUseServer);
		bool IsServerPoolFull(enum TSKRequestMarket rmRequestMarket, vector<CSKServer*>* pvServerPool);
		void ChangeInuseServerCount(enum TSKRequestMarket rmRequestMarket, int nPoolIndex, int nValue);

		void SetConfiguration(struct TSKConfig* pstruConfig);
		void StartUpServers();
};
#endif
