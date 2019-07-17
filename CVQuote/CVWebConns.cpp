#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "CVWebConns.h"
#include "CVGlobal.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
												unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
												unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CSKServers* CSKServers::instance = NULL;
pthread_mutex_t CSKServers::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CSKServers::CSKServers() { }

CSKServers::~CSKServers() { }

void CSKServers::AddFreeServer(enum TSKRequestMarket rmRequestMarket, int nServerIndex)
{
	try
	{
		CSKServer* pServer = new CSKServer(m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strWeb,
						   m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strQstr,
						   m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strName,
						   rmRequestMarket);

		printf("[%s] WebSocket URL: %s%s\n",
		m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strName.c_str(),
		m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strWeb.c_str(),
		m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strQstr.c_str());

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
			m_vvvServerPool.at(i).resize(m_vServerConfig.at(i)->nServerCount);
		}

		printf("Number of Markets : %d\n", rmNum);

		for(int i=0;i<rmNum;i++)
		{
			printf("Number of Server : %d\n", m_vServerConfig.at(i)->nServerCount);
			for(int j=0;j<m_vServerConfig.at(i)->nServerCount;j++)
			{
				AddFreeServer((TSKRequestMarket)i, j);
			}
		}
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}

}
