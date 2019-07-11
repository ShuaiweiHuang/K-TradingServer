#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "CVWebClients.h"
#include "CVGlobal.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
												unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
												unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKServers* CSKServers::instance = NULL;
pthread_mutex_t CSKServers::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CSKServers::CSKServers() { }

CSKServers::~CSKServers() { }

int g_nCount = 0;
void CSKServers::AddFreeServer(enum TSKRequestMarket rmRequestMarket, int nPoolIndex)
{
	try
	{
		pthread_mutex_t* pServerPoolLock = NULL;
		vector<CSKServer*>* pvServerPool = NULL;

		pServerPoolLock = m_vvServerPoolLock.at(rmRequestMarket).at(nPoolIndex);
		pvServerPool = &(m_vvvServerPool.at(rmRequestMarket).at(nPoolIndex));

		if(pServerPoolLock && pvServerPool)
		{
			int nServerIndex= nPoolIndex / m_vServerConfig.at(rmRequestMarket)->nPoolCount;
#ifdef DEBUG
			printf("nServerIndex= %d, nPoolIndex = %d, nPoolCount = %d\n", nServerIndex, nPoolIndex, m_vServerConfig.at(rmRequestMarket)->nPoolCount);
#endif
			CSKServer* pServer = new CSKServer(m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strWeb,
											   m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strQstr,
											   rmRequestMarket,
											   nPoolIndex);
			pServer->SetStatus(ssFree);
			pthread_mutex_lock(pServerPoolLock);//lock
			pvServerPool->push_back(pServer);
			pthread_mutex_unlock(pServerPoolLock);//unlock

			g_nCount++;
			FprintfStderrLog("ADD_A_SERVER", g_nCount, rmRequestMarket);
		}
		else
			FprintfStderrLog("ADD_A_SERVER_ERROR", rmRequestMarket, nPoolIndex);
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}
}

CSKServers* CSKServers::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CSKServers();
			FprintfStderrLog("SERVERS_ONE", 0, 0);
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

CSKServer* CSKServers::GetFreeServerObject(enum TSKRequestMarket rmRequestMarket)
{
	CSKServer* pServer = NULL;

	try
	{
		int nLeastUseServer = 0;
		int nLeastUsePool = 0;
		int nIncrementalServerObjectCount = m_vServerConfig.at(rmRequestMarket)->nIncrementalServerObjectCount;

		nLeastUseServer = GetLeastUseServer(rmRequestMarket);
		nLeastUsePool = GetLeastUsePool(rmRequestMarket, nLeastUseServer);

		vector<CSKServer*>* pvServerPool = &(m_vvvServerPool.at(rmRequestMarket).at(nLeastUsePool));
		pthread_mutex_t* pServerPoolLock = m_vvServerPoolLock.at(rmRequestMarket).at(nLeastUsePool);
		int slp_time = (rand() % 150000)+250000; // interval time of retry.

		for(int i = 0; i < TRY_NEW_CONNECT_TO_SERVER_COUNT; i++)
		{
			pServer = NULL;
			static vector<CSKServer*>::iterator iter_last = pvServerPool->begin();
			pthread_mutex_lock(pServerPoolLock);//lock

			for(vector<CSKServer*>::iterator iter = iter_last ; iter != pvServerPool->end() ; iter++)
			{
				if((*iter)->GetStatus() == ssFree)
				{
					pServer = *iter;
					pServer->SetStatus(ssInuse);
					ChangeInuseServerCount(rmRequestMarket, nLeastUsePool, 1);
					if(iter == pvServerPool->end())
						iter_last = pvServerPool->begin();
					else
						iter_last = iter+1;
					break;
				}
			}

			for(vector<CSKServer*>::iterator iter = pvServerPool->begin(); iter != iter_last; iter++)
			{
				if((*iter)->GetStatus() == ssFree)
				{
					pServer = *iter;
					pServer->SetStatus(ssInuse);
					ChangeInuseServerCount(rmRequestMarket, nLeastUsePool, 1);
					if(iter == pvServerPool->end())
						iter_last = pvServerPool->begin();
					else
						iter_last = iter+1;
					break;
				}
			}
			pthread_mutex_unlock(pServerPoolLock);//unlock

			if(pServer) {

				break;
			}
			else
			{
				for(int i = 0; i < nIncrementalServerObjectCount; i++)
				{
					if(!IsServerPoolFull(rmRequestMarket, pvServerPool)) {
						AddFreeServer(rmRequestMarket, nLeastUsePool);
					}
					else
						break;
				}
			}
			usleep(slp_time);
		}

		if(pServer == NULL && IsServerPoolFull(rmRequestMarket, pvServerPool))  //retry fail & service full
		{
			FprintfStderrLog("SERVER_BUSY_WARNING", -11, 0);
		}

	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}

	return pServer;
}

bool CSKServers::IsServerPoolFull(enum TSKRequestMarket rmRequestMarket, vector<CSKServer*>* pvServerPool)//todo -> reference * index
{
	try//to check
	{
		return m_vServerConfig.at(rmRequestMarket)->nMaximumServerObjectCount > pvServerPool->size() ? false : true;//check
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}
}

void CSKServers::ChangeInuseServerCount(enum TSKRequestMarket rmRequestMarket, int nPoolIndex, int nValue)
{
	try
	{
		pthread_mutex_t* pmtxInuseServerCountLock = NULL;
		int* pInuseServerCount = NULL;

		pmtxInuseServerCountLock = m_vvInuseServerCountLock.at(rmRequestMarket).at(nPoolIndex);
		pInuseServerCount = m_vvInuseServerCount.at(rmRequestMarket).at(nPoolIndex);

		if(pmtxInuseServerCountLock && pInuseServerCount)
		{
			pthread_mutex_lock(pmtxInuseServerCountLock);//lock
			*pInuseServerCount += nValue;
#ifdef DEBUG
			cout << "nPoolIndex=" << nPoolIndex << " -> "<< "pInuseServerCount=" << *pInuseServerCount << endl;
#endif			
			pthread_mutex_unlock(pmtxInuseServerCountLock);//unlock
		}
		else
			FprintfStderrLog("CHANGE_INUSE_SERVER_COUNT_ERROR", rmRequestMarket, nPoolIndex);
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}
}

int CSKServers::GetLeastUseServer(enum TSKRequestMarket rmRequestMarket)
{
	int nServerCount = 0;
	int nPoolCount = 0;
	int nLeastUseServer= 0;
	try
	{
		nServerCount = m_vServerConfig.at(rmRequestMarket)->nServerCount;
		nPoolCount = m_vServerConfig.at(rmRequestMarket)->nPoolCount;

		int nMinimumInuse = 0;
		for(int i=0;i<nPoolCount;i++)
			nMinimumInuse += *m_vvInuseServerCount.at(rmRequestMarket).at(0*nPoolCount+i);

		if(nMinimumInuse == 0)
			return 0;

		for(int i=1;i<nServerCount;i++)
		{
			int nInuse = 0;
			for(int j=0;j<nPoolCount;j++)
			{
				nInuse += *m_vvInuseServerCount.at(rmRequestMarket).at(i*nPoolCount+j);
			}
			if(nInuse == 0)
			{
				nLeastUseServer = i;
				break;
			}
			if(nInuse < nMinimumInuse)
			{
				nMinimumInuse = nInuse;
				nLeastUseServer = i;
			}
		}
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}

	return nLeastUseServer;
}

int CSKServers::GetLeastUsePool(enum TSKRequestMarket rmRequestMarket, int nLeastUseServer)
{
	int nLeastUsePool = 0;
	try
	{
		int nPoolCount = m_vServerConfig.at(rmRequestMarket)->nPoolCount;
		int nServerCount = m_vServerConfig.at(rmRequestMarket)->nServerCount;
		
		nLeastUsePool = nLeastUseServer *nPoolCount;

		int nMinimumInuse = *m_vvInuseServerCount.at(rmRequestMarket).at(nLeastUseServer*nPoolCount) ;
		if(nMinimumInuse == 0)
			return nLeastUseServer*nPoolCount;

		for(int i = nLeastUseServer * nPoolCount; i < nLeastUseServer * nPoolCount + nPoolCount; i++)
		{
			if(*m_vvInuseServerCount.at(rmRequestMarket).at(i) < nMinimumInuse)
			{
				if(nMinimumInuse == 0)
				{
					nLeastUsePool = i;
					break;
				}

				nMinimumInuse = *m_vvInuseServerCount.at(rmRequestMarket).at(i);
				nLeastUsePool = i;
			}
		}
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}

	return nLeastUsePool;
}

void CSKServers::SetConfiguration(struct TSKConfig* pstruConfig)
{
	m_vServerConfig.push_back(pstruConfig);//TS,TF,OF,OS
}

void CSKServers::StartUpServers()
{
	try
	{
		m_vvvServerPool.resize(rmNum);
		for(int i=0 ; i<rmNum ; i++)
		{
			m_vvvServerPool.at(i).resize(m_vServerConfig.at(i)->nServerCount * m_vServerConfig.at(i)->nPoolCount);
		}

		m_vvServerPoolLock.resize(rmNum);
		m_vvInuseServerCount.resize(rmNum);
		m_vvInuseServerCountLock.resize(rmNum);
		for(int i=0 ; i<rmNum ; i++)
		{
			for(int j=0;j<m_vServerConfig.at(i)->nServerCount * m_vServerConfig.at(i)->nPoolCount;j++)
			{
				pthread_mutex_t* pmtxServerPoolLock = new pthread_mutex_t;
				pthread_mutex_init(pmtxServerPoolLock,NULL);
				m_vvServerPoolLock.at(i).push_back(pmtxServerPoolLock);

				int* pInuseServerCount = new int(0);
				m_vvInuseServerCount.at(i).push_back(pInuseServerCount);

				pthread_mutex_t* pmtxServerCountLock = new pthread_mutex_t;
				pthread_mutex_init(pmtxServerCountLock,NULL);
				m_vvInuseServerCountLock.at(i).push_back(pmtxServerCountLock);
			}
		}


		for(int i=0;i<rmNum;i++)
		{
			for(int j=0;j<m_vServerConfig.at(i)->nServerCount* m_vServerConfig.at(i)->nPoolCount;j++)
			{
				for(int k=0;k<m_vServerConfig.at(i)->nInitailServerObjectCount;k++)
				{
					if(!IsServerPoolFull((TSKRequestMarket)i, &(m_vvvServerPool.at(i).at(j))))
					{
						AddFreeServer((TSKRequestMarket)i, j);
#ifdef DEBUG
						cout << "Market = " << i << ",server count*pool = " << j <<  ",server obj. = "  << k << endl;
#endif
					}
					else
					{
						FprintfStderrLog("POOL_FULL", -1, 0);
					}
				}
#ifdef MNTRMSG
				UpdateAvailableServerNum((TSKRequestMarket)i);
#endif
			}
		}
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}

}

#ifdef MNTRMSG

extern struct MNTRMSGS g_MNTRMSG;

void CSKServers::UpdateAvailableServerNum(enum TSKRequestMarket rmRequestMarket)
{
		int nLeastUseServer = 0;
		int nLeastUsePool = 0;

		nLeastUseServer = GetLeastUseServer(rmRequestMarket);
		nLeastUsePool = GetLeastUsePool(rmRequestMarket, nLeastUseServer);

		vector<CSKServer*>* pvServerPool = &(m_vvvServerPool.at(rmRequestMarket).at(nLeastUsePool));

		g_MNTRMSG.num_of_service_Available = 0;

		for(vector<CSKServer*>::iterator iter = pvServerPool->begin(); iter != pvServerPool->end(); iter++)
		{
			if((*iter)->GetStatus() == ssFree)
			{
				g_MNTRMSG.num_of_service_Available++;
			}
		}
}
#endif
