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

#include "CVTandemDAOs.h"
#include "CVReadQueueDAOs.h"
#include "CVTandem.h"

using namespace std;

#define OTSIDLENTH 12


void ReadTandemDAOConfigFile(string strConfigFileName, string& strService, int& nInitialConnection,
							 int& nMaximumConnection, int& nNumberOfWriteQueueDAO, key_t& kWriteQueueDAOStartKey, 
							 key_t& kWriteQueueDAOEndKey);

void ReadReadQueueDAOConfigFile(string strConfigFileName, string& strOTSID, int& nNumberOfReadQueueDAO,
							    key_t& kReadQueueDAOStartKey, key_t& kReadQueueDAOEndKey, key_t& kTIGNumberSharedMemoryKey);
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

	ReadTandemDAOConfigFile("../ini/CVTrade.ini", strService, nInitialConnection, nMaximumConnection, nNumberOfWriteQueueDAO,
							kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);//todo check

	CSKTandemDAOs* pTandemDAOs = CSKTandemDAOs::GetInstance();

	if(pTandemDAOs)
	{
		pTandemDAOs->SetConfiguration(strService, nInitialConnection, nMaximumConnection, nNumberOfWriteQueueDAO, kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);
		pTandemDAOs->StartUpDAOs();
	}

	key_t kTIGNumberSharedMemoryKey;
	ReadReadQueueDAOConfigFile("../ini/CVTrade.ini", strOTSID, nNumberOfReadQueueDAO, kReadQueueDAOStartKey, kReadQueueDAOEndKey, kTIGNumberSharedMemoryKey);

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

	while(!done)
	{
		sleep(1);
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
