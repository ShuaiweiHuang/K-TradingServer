#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <glib.h>
#include <assert.h>

#include "CVQueueDAOs.h"
#include "CVClients.h"

using namespace std;

void ReadQueueDAOConfigFile(string strConfigFileName,string& strService, int& nNumberOfWriteQueueDAO, key_t& kQueueDAOWriteStartKey, key_t& kQueueDAOWriteEndKey, key_t& kQueueDAOReadStartKey, key_t& kQueueDAOReadEndKey);

void ReadClientConfigFile(string strConfigFileName, string& strSevice, string& strListenPort, key_t& kSerialNumberSharedMemoryKey);

#ifdef MNTRMSG
#define SRVLOGKEY 1113
struct MNTRMSGS g_MNTRMSG;
CCVSharedMemory* g_pMonitorMSGSharedMemory = NULL;
int* g_pMonitorMSGLongArray = NULL;

void UpdateMonitorMSGToSharedMemory()
{
    memcpy(g_pMonitorMSGLongArray, &g_MNTRMSG, sizeof(struct MNTRMSGS));
}
#endif

#ifdef TIMETEST
#define TIMETESTKEY 1111
CCVSharedMemory* g_pTimeTestSharedMemory = NULL;
long* g_pTimeLogLongArray = NULL;

void InsertTimeLogToSharedMemory(struct timeval *timeval_Start, struct timeval *timeval_End, enum TCVTimePoint tpTimePoint, long lOrderNumber)
{
    long lTime = 0;

    if(timeval_Start == NULL)
        lTime = (timeval_End->tv_sec)*1000000L + timeval_End->tv_usec;
    else
        lTime = (timeval_End->tv_sec-timeval_Start->tv_sec)*1000000L + timeval_End->tv_usec - timeval_Start->tv_usec;

    int nIndex = tpTimePoint + AMOUNT_OF_TIME_POINT * ( lOrderNumber % AMOUNT_OF_CLIENT_OBJECT );

    g_pTimeLogLongArray[nIndex] = lTime;
}

#endif

int main(int argc, char *argv[])
{
	string strService;

	int nNumberOfQueueDAO;
	key_t kQueueDAOWriteStartKey;
	key_t kQueueDAOWriteEndKey;
	key_t kQueueDAOReadStartKey;
	key_t kQueueDAOReadEndKey;

	setbuf(stdout, NULL);
	signal ( SIGPIPE ,  SIG_IGN );

	ReadQueueDAOConfigFile("APServer.ini", strService, nNumberOfQueueDAO, kQueueDAOWriteStartKey, kQueueDAOWriteEndKey, kQueueDAOReadStartKey, kQueueDAOReadEndKey);

	CCVQueueDAOs* pQueueDAOs = CCVQueueDAOs::GetInstance();
	assert(pQueueDAOs);

	pQueueDAOs->SetConfiguration(strService, nNumberOfQueueDAO, kQueueDAOWriteStartKey, kQueueDAOWriteEndKey,
								 kQueueDAOReadStartKey, kQueueDAOReadEndKey);
	pQueueDAOs->StartUpDAOs();

	sleep(1);

	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	string strListenPort;
	key_t kSerialNumberSharedMemoryKey;

	ReadClientConfigFile("APServer.ini", strService, strListenPort, kSerialNumberSharedMemoryKey);//todo check
	pClients->SetConfiguration(strService, strListenPort, kSerialNumberSharedMemoryKey);
#ifdef TIMETEST
	g_pTimeTestSharedMemory = new CCVSharedMemory(TIMETESTKEY, AMOUNT_OF_TIME_POINT * AMOUNT_OF_CLIENT_OBJECT * sizeof(long));
	g_pTimeTestSharedMemory->AttachSharedMemory();
	g_pTimeLogLongArray = (long*)g_pTimeTestSharedMemory->GetSharedMemory();
	for(int i=0 ; i < AMOUNT_OF_TIME_POINT * AMOUNT_OF_CLIENT_OBJECT ; i++)
		g_pTimeLogLongArray[i] = 0;
#endif

#ifdef MNTRMSG
	g_pMonitorMSGSharedMemory = new CCVSharedMemory(SRVLOGKEY, sizeof(struct MNTRMSGS));
	g_pMonitorMSGSharedMemory->AttachSharedMemory();
	g_pMonitorMSGLongArray = (int*)g_pMonitorMSGSharedMemory->GetSharedMemory();
	memset(&g_MNTRMSG, 0, sizeof(struct MNTRMSGS));
#endif

	while(1)
	{
		sleep(1);
		pClients->CheckOnlineClientVector();
		pClients->ClearOfflineClientVector();
#ifdef MNTRMSG
		pClients->FlushLogMessageToFile();
		UpdateMonitorMSGToSharedMemory();
#ifdef DEBUG
		printf("Server: g_MNTRMSG.num_of_thread_Current = %d\n", g_MNTRMSG.num_of_thread_Current);
		printf("Server: g_MNTRMSG.num_of_thread_Max = %d\n", g_MNTRMSG.num_of_thread_Max);
		printf("Server: g_MNTRMSG.num_of_order_Received = %d\n", g_MNTRMSG.num_of_order_Received);
		printf("Server: g_MNTRMSG.num_of_order_Sent = %d\n", g_MNTRMSG.num_of_order_Sent);
		printf("Server: g_MNTRMSG.num_of_order_Reply = %d\n", g_MNTRMSG.num_of_order_Reply);
#endif
#endif
	}

	return 0;
}

void ReadQueueDAOConfigFile(string strConfigFileName, string& strService, int& nNumberOfQueueDAO, key_t& kQueueDAOWriteStartKey, key_t& kQueueDAOWriteEndKey, key_t& kQueueDAOReadStartKey, key_t& kQueueDAOReadEndKey)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;

	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

	strService = g_key_file_get_string(keyfile, "Client", "Service", NULL);
	nNumberOfQueueDAO = g_key_file_get_integer(keyfile, "Client", "NumberOfQueueDAO", NULL);
	kQueueDAOWriteStartKey = g_key_file_get_integer(keyfile, "Client", "QueueDAOWriteStartKey", NULL);
	kQueueDAOWriteEndKey = g_key_file_get_integer(keyfile, "Client", "QueueDAOWriteEndKey", NULL);
	kQueueDAOReadStartKey = g_key_file_get_integer(keyfile, "Client", "QueueDAOReadStartKey", NULL);
	kQueueDAOReadEndKey = g_key_file_get_integer(keyfile, "Client", "QueueDAOReadEndKey", NULL);
}

void ReadClientConfigFile(string strConfigFileName, string& strService, string& strListenPort, key_t& kSerialNumberSharedMemoryKey)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;

	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

	strService = g_key_file_get_string(keyfile, "Client", "Service", NULL);
	strListenPort = g_key_file_get_string(keyfile, "Client", "ListenPort", NULL);
	kSerialNumberSharedMemoryKey = g_key_file_get_integer(keyfile, "Client", "SerialNumberSharedMemoryKey", NULL);
}
