#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "CVWebConns.h"
#include "CVWebConn.h"
#include "CVGlobal.h"
#include "CVQueueNodes.h"


using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
				unsigned char* pMessage1 = NULL, int nMessage1Length = 0, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

extern void mem_usage(double& vm_usage, double& resident_set);
extern int ping(const char *adress);

CCVServers* CCVServers::instance = NULL;
pthread_mutex_t CCVServers::ms_mtxInstance = PTHREAD_MUTEX_INITIALIZER;
struct MNTRMSGS g_MNTRMSG;


CCVServers::CCVServers()
{
	unsigned char msg[BUFFERSIZE];
	m_pHeartbeat = NULL;
	memset(&g_MNTRMSG, 0, sizeof(struct MNTRMSGS));

	try
	{
		m_pHeartbeat = new CCVHeartbeat(this);
		m_pHeartbeat->SetTimeInterval(MONITOR_HEARTBEAT_INTERVAL_SEC);
		memset(msg, BUFFERSIZE, 0);
		sprintf((char*)msg, "add heartbeat in reply monitor service.");
		FprintfStderrLog("NEW_HEARTBEAT_CREATE", -1, 0, __FILE__, __LINE__, msg, strlen((char*)msg));
	}
	catch (exception& e)
	{
		FprintfStderrLog("NEW_HEARTBEAT_ERROR", -1, 0, __FILE__, __LINE__, 0, 0, 0, 0);
		return;
	}
}

CCVServers::~CCVServers()
{
	if(m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}
}

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

void CCVServers::OnHeartbeatLost()
{
	FprintfStderrLog("HEARTBEAT LOST", -1, 0, NULL, 0,  NULL, 0);
	exit(-1);
}

void CCVServers::OnHeartbeatRequest()
{
	char caHeartbeatRequestBuf[128];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	double VM_size, RSS_size;
	struct timeval timeval_Start;
	struct timeval timeval_End;
	double loading[3];
	char hostname[128];

	CCVQueueDAO* pQueueDAO = CCVQueueDAOs::GetInstance()->m_QueueDAOMonitor;

	gethostname(hostname, sizeof hostname);
	memset(caHeartbeatRequestBuf, 0, 128);
	mem_usage(VM_size, RSS_size);

	sprintf(caHeartbeatRequestBuf, "{\"Type\":\"Heartbeat\",\"Component\":\"Reply\",\"Hostname\":\"%s\",\"ServerDate\":\"%d%02d%02d\",\"ServerTime\":\"%02d%02d%02d00\"}",
		hostname, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	pQueueDAO->SendData(caHeartbeatRequestBuf, strlen(caHeartbeatRequestBuf));
	sprintf(caHeartbeatRequestBuf, "{\"Type\":\"System\",\"Component\":\"Reply\",\"Hostname\":\"%s\",\"CurrentThread\":\"%d\",\"MaxThread\":\"%d\",\"MemoryUsage\":\"%.0f\"}",
		hostname, g_MNTRMSG.num_of_thread_Current, g_MNTRMSG.num_of_thread_Max, VM_size);
	pQueueDAO->SendData(caHeartbeatRequestBuf, strlen(caHeartbeatRequestBuf));

	gettimeofday (&timeval_Start, NULL) ;
	if(!ping("www.bitmex.com"))
	{
		gethostname(hostname, sizeof hostname);
		gettimeofday (&timeval_End, NULL) ;
		g_MNTRMSG.network_delay_ms = (timeval_End.tv_sec-timeval_Start.tv_sec)*1000000L + timeval_End.tv_usec - timeval_Start.tv_usec;
		g_MNTRMSG.network_delay_ms /= 1000; //ns to ms.
		sprintf(caHeartbeatRequestBuf, "{\"Type\":\"Host\",\"Component\":\"Network\",\"Hostname\":\"%s\",\"Interval\":\"%ld\"}", hostname, g_MNTRMSG.network_delay_ms);
	}
	else
	{
		sprintf(caHeartbeatRequestBuf, "{\"Type\":\"Host\",\"Component\":\"Network\",\"Hostname\":\"%s\",\"Interval\":\"Timeout\"}", hostname);
	}
	pQueueDAO->SendData(caHeartbeatRequestBuf, strlen(caHeartbeatRequestBuf));

	if(getloadavg(loading, 3) != -1) /*getloadavg is the function used to calculate and obtain the load average*/
	{
		gethostname(hostname, sizeof hostname);
		g_MNTRMSG.cpu_loading = (loading[0]*=100)/(sysconf(_SC_NPROCESSORS_ONLN));
		sprintf(caHeartbeatRequestBuf, "{\"Type\":\"Host\",\"Component\":\"CPU\",\"Hostname\":\"%s\",\"Loading\":\"%d\"}", hostname, g_MNTRMSG.cpu_loading);
	}
	pQueueDAO->SendData(caHeartbeatRequestBuf, strlen(caHeartbeatRequestBuf));

	m_pHeartbeat->TriggerGetReplyEvent();
}

void CCVServers::OnHeartbeatError(int nData, const char* pErrorMessage)
{
	exit(-1);
}
