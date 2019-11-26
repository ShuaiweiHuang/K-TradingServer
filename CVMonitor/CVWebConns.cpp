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
		printf("address = %x\n", pServer);
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

			for(int j=0 ; j<m_vServerConfig.at(i)->nServerCount ; j++)
			{
				printf("AddFreeServer\n");
				AddFreeServer((TCVRequestMarket)i, j);
			}
		}
#ifdef DEBUG
		printf("vector size = %d\n", m_vServerPool.size());
#endif
	}
	catch(const out_of_range& e)
	{
		FprintfStderrLog("OUT_OF_RANGE_ERROR", -1, 0, __FILE__, __LINE__, (unsigned char*)e.what(), strlen(e.what()));
	}

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
	for(int i=0 ; i<m_vServerPool.size() ; i++)
	{
		if((m_vServerPool[i])->m_ssServerStatus == ssBreakdown)
		{
			printf("[%s]: break down\n", (m_vServerPool[i])->m_strName.c_str());

			for(int j=0 ; j<m_vServerConfig.at(0)->nServerCount ; j++)
			{
				if(m_vServerConfig.at(0)->vServerInfo.at(j)->strName == (m_vServerPool[i])->m_strName)
				{
					CCVServer* pServer = new CCVServer(
							m_vServerConfig.at(0)->vServerInfo.at(j)->strWeb,
                                                        m_vServerConfig.at(0)->vServerInfo.at(j)->strQstr,
                                                        m_vServerConfig.at(0)->vServerInfo.at(j)->strName,
                                                        rmBitmex);

			                printf("[Reconnect] %s: WebSocket URL: %s%s\n",
			                m_vServerConfig.at(0)->vServerInfo.at(j)->strName.c_str(),
			                m_vServerConfig.at(0)->vServerInfo.at(j)->strWeb.c_str(),
			                m_vServerConfig.at(0)->vServerInfo.at(j)->strQstr.c_str());
					delete(m_vServerPool[i]);
			                m_vServerPool[i] = pServer;
				}
			}
		}
        }
}

