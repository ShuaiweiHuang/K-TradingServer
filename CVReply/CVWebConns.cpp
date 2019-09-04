#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "CVWebConns.h"
#include "CVWebConn.h"
#include "CVGlobal.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
				unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

CCVServers* CCVServers::instance = NULL;
pthread_mutex_t CCVServers::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;

CCVServers::CCVServers() { }

CCVServers::~CCVServers() { }

void CCVServers::AddFreeServer(enum TCVRequestMarket rmRequestMarket, int nServerIndex)
{
	try
	{
		CCVServer* pServer = new CCVServer(m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strWeb,
						   m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strQstr,
						   m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strName,
						   rmRequestMarket);

		printf("[%s] WebSocket URL: %s%s\n",
		m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strName.c_str(),
		m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strWeb.c_str(),
		m_vServerConfig.at(rmRequestMarket)->vServerInfo.at(nServerIndex)->strQstr.c_str());
		m_vServerPool.push_back(pServer);

	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}
}

CCVServers* CCVServers::GetInstance()
{
	if(instance == NULL)
	{
		pthread_mutex_lock(&ms_mtxInstance);//lock

		if(instance == NULL)
		{
			instance = new CCVServers();
			FprintfStderrLog("SERVERS_ONE", 0, 0);
		}

		pthread_mutex_unlock(&ms_mtxInstance);//unlock
	}

	return instance;
}

void CCVServers::SetConfiguration(struct TCVConfig* pstruConfig)
{
	m_vServerConfig.push_back(pstruConfig);
}

void CCVServers::StartUpServers()
{
	try
	{
		printf("Number of Markets : %d\n", rmNum);

		for(int i=0 ; i<rmNum ; i++)
		{
			printf("Number of Server : %d\n", m_vServerConfig.at(i)->nServerCount);
		}

		for(int i=0 ; i<rmNum ; i++)
		{
			for(int j=0 ; j<m_vServerConfig.at(i)->nServerCount ; j++)
			{
				AddFreeServer((TCVRequestMarket)i, j);
			}
		}
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}

}

void CCVServers::RestartUpServers()
{
	vector<CCVServer*>::iterator iter = m_vServerPool.begin();
	while(iter != m_vServerPool.end())
	{
		(*iter)->m_ssServerStatus = ssBreakdown;
		delete (*iter);
		printf("close server: %s\n", (*iter)->m_strWeb.c_str());
		iter++;
	}
	StartUpServers();
}

CCVServer* CCVServers::GetServerByName(string name)
{
	vector<CCVServer*>::iterator iter = m_vServerPool.begin();
	while(iter != m_vServerPool.end())
	{
		if((*iter)->m_strName == name)
		{
			return *iter;
		}
		iter++;
	}
	return NULL;
}

void CCVServers::CheckClientVector()
{
        vector<CCVServer*>::iterator iter = m_vServerPool.begin();
        iter = m_vServerPool.begin();
        while(iter != m_vServerPool.end())
        {
		if((*iter)->m_ssServerStatus == ssBreakdown) {
			RestartUpServers();
			break;
		}
		iter++;
        }
}

