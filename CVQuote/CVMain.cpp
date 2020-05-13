#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <glib.h>
#include <unistd.h>

#include "CVServer.h"
#include "CVWebConns.h"
#include "CVQueueNodes.h"

using namespace std;

void InitialGlobal();

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
			     unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
			     unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

void ReadConfigFile(string strConfigFileName, string strSection, struct TCVConfig &struConfig);
void ReadClientConfigFile(string strConfigFileName, string& strListenPort, string& strHeartBeatTime, string &strEPIDNum);
void ReadQueueDAOConfigFile(string strConfigFileName,string&, int&, key_t&, key_t&, key_t&, key_t&, key_t&);
#define DECLARE_CONFIG_DATA(CONFIG)\
	struct TCVConfig stru##CONFIG;\
	memset(&stru##CONFIG, 0, sizeof(struct TCVConfig));\
	;

int main()
{
	int nNumberOfQueueDAO;
	key_t kQueueDAOWriteStartKey;
	key_t kQueueDAOWriteEndKey;
	key_t kQueueDAOReadStartKey;
	key_t kQueueDAOReadEndKey;
	key_t kQueueDAOMonitorKey;
	string strService;

	InitialGlobal();
	setbuf(stdout, NULL);
	signal(SIGPIPE, SIG_IGN );
	srand(time(NULL));

	DECLARE_CONFIG_DATA(TSConfig);

	string strListenPort, strHeartBeatTime, strEPIDNum;
	ReadClientConfigFile("../ini/CVQuote.ini", strListenPort, strHeartBeatTime, strEPIDNum);

	int nService = 0;
	printf("Quote program start\n");
	ReadConfigFile("../ini/CVQuote.ini", "EXCHANGE", struTSConfig);

	//Web connection service.
	CCVServers* pServers = NULL;
	try
	{
		pServers = CCVServers::GetInstance();
		if(pServers == NULL)
			throw "GET_WEB_ERROR";

		pServers->SetConfiguration(&struTSConfig);
		pServers->StartUpServers();
	}
	catch (const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
	}
	
	//Queue init.
	ReadQueueDAOConfigFile("../ini/CVQuote.ini", strService, nNumberOfQueueDAO, kQueueDAOWriteStartKey, kQueueDAOWriteEndKey, kQueueDAOReadStartKey, kQueueDAOReadEndKey, kQueueDAOMonitorKey);
	//printf("%d, %d, %d, %d\n", kQueueDAOWriteStartKey, kQueueDAOWriteEndKey, kQueueDAOReadStartKey, kQueueDAOReadEndKey);
	CCVQueueDAOs* pQueueDAOs = CCVQueueDAOs::GetInstance();
	assert(pQueueDAOs);

	pQueueDAOs->SetConfiguration(strService, nNumberOfQueueDAO, kQueueDAOWriteStartKey, kQueueDAOWriteEndKey, kQueueDAOReadStartKey, kQueueDAOReadEndKey, kQueueDAOMonitorKey);
	pQueueDAOs->StartUpDAOs();

	//Server connection service.
	CCVClients* pClients = NULL;
	try
	{
		pClients = CCVClients::GetInstance();
		if(pClients == NULL)
			throw "GET_SERVER_MANAGEMENT_ERROR";

		pClients->SetConfiguration(strListenPort, strHeartBeatTime, strEPIDNum, nService);

		while(1)
		{
			pClients->CheckClientVector();
			pServers->CheckClientVector();
			sleep(1);
		}
	}
	catch (const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
	}

	return 0;
}

void ReadConfigFile(string strConfigFileName, string strSection, struct TCVConfig &struConfig)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;

	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));
	struConfig.nServerCount = g_key_file_get_integer(keyfile, strSection.c_str(), "ServerCount", NULL);//todo->check?
	struConfig.nServerSet = g_key_file_get_integer(keyfile, strSection.c_str(), "ServerSet", NULL);//todo->check?

	char caIP[5];
	char caPort[7];
	char caName[7];

	for(int i=0;i<struConfig.nServerCount;i++)
	{
		memset(caIP, 0, sizeof(caIP));
		memset(caPort, 0, sizeof(caPort));
		memset(caName, 0, sizeof(caName));

		sprintf(caIP, "IP%02d", i+1);
		sprintf(caPort, "PORT%02d", i+1);
		sprintf(caName, "NAME%02d", i+1);

		struct TCVServerInfo* pstruServerInfo = new struct TCVServerInfo;//destructor

		try {
			pstruServerInfo->strHost  = g_key_file_get_string(keyfile, strSection.c_str(), caIP, NULL);
		}
		catch (const char* pErrorMessage)
		{
			FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
		}
		pstruServerInfo->strPara = g_key_file_get_string(keyfile, strSection.c_str(), caPort, NULL);
		pstruServerInfo->strName = g_key_file_get_string(keyfile, strSection.c_str(), caName, NULL);
		printf("Server: %s\n", pstruServerInfo->strName.c_str());
		printf("Connect to: %s", pstruServerInfo->strHost.c_str());
		printf(":%s\n", pstruServerInfo->strPara.c_str());
		struConfig.vServerInfo.push_back(pstruServerInfo);
	}
}

void ReadClientConfigFile(string strConfigFileName, string& strListenPort, string& strHeartBeatTime, string& strEPIDNum)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));
	strEPIDNum       = g_key_file_get_string(keyfile, "SERVER", "EpidNum",     NULL);
	strListenPort    = g_key_file_get_string(keyfile, "SERVER", "ListenPort",    NULL);
	strHeartBeatTime = g_key_file_get_string(keyfile, "SERVER", "HeartBeatTime", NULL);
}

void ReadQueueDAOConfigFile(string strConfigFileName, string& strService, int& nNumberOfQueueDAO, key_t& kQueueDAOWriteStartKey, key_t& kQueueDAOWriteEndKey, key_t& kQueueDAOReadStartKey, key_t& kQueueDAOReadEndKey, key_t& kQueueDAOMonitorKey)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;

	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

	strService = g_key_file_get_string(keyfile, "QUEUE", "Service", NULL);
	nNumberOfQueueDAO = g_key_file_get_integer(keyfile, "QUEUE", "NumberOfQueueDAO", NULL);
	kQueueDAOWriteStartKey = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeWriteStartKey", NULL);
	kQueueDAOWriteEndKey = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeWriteEndKey", NULL);
	kQueueDAOReadStartKey = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeReadStartKey", NULL);
	kQueueDAOReadEndKey = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeReadEndKey", NULL);
	kQueueDAOMonitorKey = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeMonitorKey", NULL);
}

void mem_usage(double& vm_usage, double& resident_set)
{
	vm_usage = 0.0;
	resident_set = 0.0;
	ifstream stat_stream("/proc/self/stat",ios_base::in);
	string pid, comm, state, ppid, pgrp, session, tty_nr;
	string tpgid, flags, minflt, cminflt, majflt, cmajflt;
	string utime, stime, cutime, cstime, priority, nice;
	string O, itrealvalue, starttime;
	unsigned long vsize;
	long rss;
	stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
	>> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
	>> utime >> stime >> cutime >> cstime >> priority >> nice
	>> O >> itrealvalue >> starttime >> vsize >> rss;
	stat_stream.close();
	long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
	vm_usage = vsize / (1024.0*1024.0);
	resident_set = rss * page_size_kb;
}
