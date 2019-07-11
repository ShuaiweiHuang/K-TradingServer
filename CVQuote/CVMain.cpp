#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <assert.h>
#include <glib.h>
#include <unistd.h>

#include "CVServers.h"
#include "CVWebClients.h"

using namespace std;

void InitialGlobal();

extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
                             unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
                             unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

void ReadConfigFile(string strConfigFileName, string strSection, struct TSKConfig &struConfig);
void ReadClientConfigFile(string strConfigFileName, string& strListenPort, string& strHeartBeatTime, string &strEPID);
#define DECLARE_CONFIG_DATA(CONFIG)\
	struct TSKConfig stru##CONFIG;\
	memset(&stru##CONFIG, 0, sizeof(struct TSKConfig));\
	;

int main()
{
	InitialGlobal();
	DECLARE_CONFIG_DATA(TSConfig);
	DECLARE_CONFIG_DATA(TFConfig);
	DECLARE_CONFIG_DATA(OFConfig);
	DECLARE_CONFIG_DATA(OSConfig);


	setbuf(stdout, NULL);
	signal(SIGPIPE, SIG_IGN );
	srand(time(NULL));

#if 0
	ReadConfigFile("../ini/CVQuote.ini", "BITMEX", struTSConfig);
	CSKServers* pServers = NULL;
	try
	{
		CSKServers* pServers = CSKServers::GetInstance();
		if(pServers == NULL)
			throw "GET_SERVERS_ERROR";

		pServers->SetConfiguration(&struTSConfig);
		pServers->StartUpServers();
	}
	catch (const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
	}
#endif
	string strListenPort, strHeartBeatTime, strEPID;
	ReadClientConfigFile("CVQuote.ini", strListenPort, strHeartBeatTime, strEPID);
	int nService = 0;
	if(struTSConfig.nServerCount > 0)
		nService += 1<<0;

	CSKClients* pClients = NULL;
	try
	{
		pClients = CSKClients::GetInstance();

		if(pClients == NULL)
			throw "GET_CLIENTS_ERROR";

		pClients->SetConfiguration(strListenPort, strHeartBeatTime, nService);
		while(1)
		{
			pClients->CheckClientVector();
			sleep(1);
		}
	}
	catch (const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
	}
	return 0;
}

void ReadConfigFile(string strConfigFileName, string strSection, struct TSKConfig &struConfig)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;

	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

	struConfig.nPoolCount = g_key_file_get_integer(keyfile, strSection.c_str(), "PoolCount", NULL);
	struConfig.nInitailServerObjectCount = g_key_file_get_integer(keyfile, strSection.c_str(), "InitialServerObjectCount", NULL);
	struConfig.nMaximumServerObjectCount = g_key_file_get_integer(keyfile, strSection.c_str(), "MaximumServerObjectCount", NULL);
	struConfig.nIncrementalServerObjectCount = g_key_file_get_integer(keyfile, strSection.c_str(), "IncrementalServerObjectCount", NULL);
	struConfig.nServerCount = g_key_file_get_integer(keyfile, strSection.c_str(), "ServerCount", NULL);//todo->check?

	char caWeb[5];
	char caQstr[7];

	for(int i=0;i<struConfig.nServerCount;i++)
	{
		memset(caWeb, 0, sizeof(caWeb));
		memset(caQstr, 0, sizeof(caQstr));

		sprintf(caWeb, "WEB%02d", i+1);
		sprintf(caQstr, "QSTR%02d", i+1);

		struct TSKServerInfo* pstruServerInfo = new struct TSKServerInfo;//destruct
		pstruServerInfo->strWeb  = g_key_file_get_string(keyfile, strSection.c_str(), caWeb, NULL);
		pstruServerInfo->strQstr = g_key_file_get_string(keyfile, strSection.c_str(), caQstr, NULL);
		printf("Connect web: %s\n", pstruServerInfo->strWeb.c_str());
		printf("Query strnig: %s\n", pstruServerInfo->strQstr.c_str());

		struConfig.vServerInfo.push_back(pstruServerInfo);
	}
}

void ReadClientConfigFile(string strConfigFileName, string& strListenPort, string& strHeartBeatTime, string& strEPID)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));
	strListenPort    = g_key_file_get_string(keyfile, "SERVER", "ListenPort",    NULL);
	strHeartBeatTime = g_key_file_get_string(keyfile, "SERVER", "HeartBeatTime", NULL);
	strEPID          = g_key_file_get_string(keyfile, "SERVER", "EPIDnum",     NULL);
}
