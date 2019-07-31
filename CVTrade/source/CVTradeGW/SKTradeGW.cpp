/*
 * SocketCllent.cpp
 *
 *  Created on: 2015年11月6日
 *      Author: alex
 */
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
#include <assert.h>
#include <stdio.h>
#include <glib.h>

#include "SKTandemDAOs.h"
#include "SKReadQueueDAOs.h"
#include "SKTandem.h"

using namespace std;

#define OTSIDLENTH 12


void ReadTandemDAOConfigFile(string strConfigFileName, string& strService, int& nInitialConnection,
							 int& nMaximumConnection, int& nNumberOfWriteQueueDAO, key_t& kWriteQueueDAOStartKey, 
							 key_t& kWriteQueueDAOEndKey);

void ReadReadQueueDAOConfigFile(string strConfigFileName, string& strOTSID, int& nNumberOfReadQueueDAO,
							    key_t& kReadQueueDAOStartKey, key_t& kReadQueueDAOEndKey, key_t& kTIGNumberSharedMemoryKey);
#ifdef MNTRMSG
#define GWLOGKEY 1114
struct MNTRMSGS g_MNTRMSG;
CSKSharedMemory* g_pMonitorMSGSharedMemory = NULL;
int* g_pMonitorMSGLongArray = NULL;

void UpdateMonitorMSGToSharedMemory()
{
    memcpy(g_pMonitorMSGLongArray, &g_MNTRMSG, sizeof(struct MNTRMSGS));
}
#endif

#ifdef TIMETEST
#define TIMETESTKEY 1111
#define AMOUNT_OF_CLIENT_OBJECT 1000
#define AMOUNT_OF_TIME_POINT 12

CSKSharedMemory* g_pTimeTestSharedMemory = NULL;
long* g_pTimeLogLongArray = NULL;

void InsertTimeLogToSharedMemory(struct timeval *timeval_Start, struct timeval *timeval_End, enum TSKTimePoint tpTimePoint, long lOrderNumber)
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

volatile sig_atomic_t done = 0;

void term(int signum)
{
    done = 1;
}

int main(int argc, char *argv[])
{
	string strService;
	string strOTSID;
	int nInitialConnection;
	int nMaximumConnection;

	int nNumberOfWriteQueueDAO;
	key_t kWriteQueueDAOStartKey;
	key_t kWriteQueueDAOEndKey;

	int nNumberOfReadQueueDAO;
	key_t kReadQueueDAOStartKey;
	key_t kReadQueueDAOEndKey;

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	setbuf(stdout, NULL);

#ifdef MNTRMSG
	memset(&g_MNTRMSG, 0, sizeof(struct MNTRMSGS));
#endif

	ReadTandemDAOConfigFile("APServer.ini", strService, nInitialConnection, nMaximumConnection, nNumberOfWriteQueueDAO,
							kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);//todo check
	CSKTandemDAOs* pTandemDAOs = CSKTandemDAOs::GetInstance();

	if(pTandemDAOs)
	{
		pTandemDAOs->SetConfiguration(strService, nInitialConnection, nMaximumConnection, nNumberOfWriteQueueDAO,
									  kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);
		pTandemDAOs->StartUpDAOs();
	}

	key_t kTIGNumberSharedMemoryKey;

	ReadReadQueueDAOConfigFile("APServer.ini", strOTSID, nNumberOfReadQueueDAO, kReadQueueDAOStartKey, kReadQueueDAOEndKey, kTIGNumberSharedMemoryKey);
	if(strlen(strOTSID.c_str()) > OTSIDLENTH)//try
	{
		cout << "The length of OTSID cannot exceed 12!" << endl;
	}

	CSKReadQueueDAOs* pReadQueueDAOs = CSKReadQueueDAOs::GetInstance();

	if(pReadQueueDAOs)
	{
		pReadQueueDAOs->SetConfiguration(strService, strOTSID, nNumberOfReadQueueDAO, kReadQueueDAOStartKey, kReadQueueDAOEndKey, kTIGNumberSharedMemoryKey);
		pReadQueueDAOs->StartUpDAOs();
	}

#ifdef TIMETEST
	g_pTimeTestSharedMemory = new CSKSharedMemory(TIMETESTKEY, AMOUNT_OF_TIME_POINT * AMOUNT_OF_CLIENT_OBJECT * sizeof(long));
	g_pTimeTestSharedMemory->AttachSharedMemory();
	g_pTimeLogLongArray = (long*)g_pTimeTestSharedMemory->GetSharedMemory();
	for(int i=0 ; i < AMOUNT_OF_TIME_POINT * AMOUNT_OF_CLIENT_OBJECT ; i++)
		g_pTimeLogLongArray[i] = 0;
#endif

#ifdef MNTRMSG
	g_pMonitorMSGSharedMemory = new CSKSharedMemory(GWLOGKEY, sizeof(struct MNTRMSGS));
	g_pMonitorMSGSharedMemory->AttachSharedMemory();
	g_pMonitorMSGLongArray = (int*)g_pMonitorMSGSharedMemory->GetSharedMemory();

	memset(g_pMonitorMSGLongArray, 0, sizeof(struct MNTRMSGS));
#endif

	while(!done)
	{
		sleep(1);
#ifdef MNTRMSG
		pTandemDAOs->UpdateAvailableDAONum();
		UpdateMonitorMSGToSharedMemory();
#ifdef DEBUG
		printf("Gateway: g_MNTRMSG.num_of_thread_Current = %d\n", g_MNTRMSG.num_of_thread_Current);
		printf("Gateway: g_MNTRMSG.num_of_thread_Max = %d\n", g_MNTRMSG.num_of_thread_Max);
		printf("Gateway: g_MNTRMSG.num_of_order_Received = %d\n", g_MNTRMSG.num_of_order_Received);
		printf("Gateway: g_MNTRMSG.num_of_order_Sent = %d\n", g_MNTRMSG.num_of_order_Sent);
		printf("Gateway: g_MNTRMSG.num_of_order_Reply = %d\n", g_MNTRMSG.num_of_order_Reply);
		printf("Gateway: g_MNTRMSG.num_of_SVOF = %d\n", g_MNTRMSG.num_of_SVOF);
		printf("Gateway: g_MNTRMSG.num_of_SVON = %d\n", g_MNTRMSG.num_of_SVON);
		printf("Gateway: g_MNTRMSG.num_of_service_Available = %d\n", g_MNTRMSG.num_of_service_Available);
#endif
#endif
	}
	delete(pTandemDAOs);
	delete(pReadQueueDAOs);

	return 0;
}

void ReadTandemDAOConfigFile(string strConfigFileName, string& strService, int& nInitialConnection,
						     int& nMaximumConnection, int& nNumberOfWriteQueueDAO, key_t& kWriteQueueDAOStartKey,
							 key_t& kWriteQueueDAOEndKey)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;

	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

	strService = g_key_file_get_string(keyfile, "Tandem", "Service", NULL);
	nInitialConnection = g_key_file_get_integer(keyfile, "Tandem", "InitialConnection", NULL);
	nMaximumConnection = g_key_file_get_integer(keyfile, "Tandem", "MaximumConnection", NULL);
	nNumberOfWriteQueueDAO = g_key_file_get_integer(keyfile, "Tandem", "NumberOfWriteQueueDAO", NULL);
	kWriteQueueDAOStartKey = g_key_file_get_integer(keyfile, "Tandem", "WriteQueueDAOStartKey", NULL);
	kWriteQueueDAOEndKey = g_key_file_get_integer(keyfile, "Tandem", "WriteQueueDAOEndKey", NULL);
}

void ReadReadQueueDAOConfigFile(string strConfigFileName, string& strOTSID, int& nNumberOfReadQueueDAO, 
								key_t& kReadQueueDAOStartKey, key_t& kReadQueueDAOEndKey, key_t& kTIGNumberSharedMemoryKey)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;

	keyfile = g_key_file_new();
	flags = GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	assert(g_key_file_load_from_file(keyfile, strConfigFileName.c_str(), flags, &error));

	strOTSID = g_key_file_get_string(keyfile, "Tandem", "OTSID", NULL);
	nNumberOfReadQueueDAO = g_key_file_get_integer(keyfile, "Tandem", "NumberOfReadQueueDAO", NULL);
	kReadQueueDAOStartKey = g_key_file_get_integer(keyfile, "Tandem", "ReadQueueDAOStartKey", NULL);
	kReadQueueDAOEndKey = g_key_file_get_integer(keyfile, "Tandem", "ReadQueueDAOEndKey", NULL);

	kTIGNumberSharedMemoryKey = g_key_file_get_integer(keyfile, "Tandem", "TIGNumberSharedMemoryKey", NULL);
}
