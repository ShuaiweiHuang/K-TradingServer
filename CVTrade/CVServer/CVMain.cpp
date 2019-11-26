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

void mem_usage(double& vm_usage, double& resident_set);
void ReadQueueDAOConfigFile(string strConfigFileName,string& strService, int& nNumberOfWriteQueueDAO,
			key_t& kQueueDAOWriteStartKey, key_t& kQueueDAOWriteEndKey, key_t& kQueueDAOReadStartKey, key_t& kQueueDAOReadEndKey, key_t& kQueueDAOMonitorKey);

void ReadClientConfigFile(string strConfigFileName, string& strSevice, string& strListenPort, key_t& kSerialNumberSharedMemoryKey);

int main(int argc, char *argv[])
{
	string strService;

	int nNumberOfQueueDAO;
	key_t kQueueDAOWriteStartKey;
	key_t kQueueDAOWriteEndKey;
	key_t kQueueDAOReadStartKey;
	key_t kQueueDAOReadEndKey;
	key_t kQueueDAOMonitorKey;
	double loading[3];
	double vm, rss;

	setbuf(stdout, NULL);
	signal(SIGPIPE,SIG_IGN);

	ReadQueueDAOConfigFile("../ini/CVTrade.ini", strService, nNumberOfQueueDAO, kQueueDAOWriteStartKey, kQueueDAOWriteEndKey, kQueueDAOReadStartKey, kQueueDAOReadEndKey, kQueueDAOMonitorKey);

	CCVQueueDAOs* pQueueDAOs = CCVQueueDAOs::GetInstance();
	assert(pQueueDAOs);

	pQueueDAOs->SetConfiguration(strService, nNumberOfQueueDAO, kQueueDAOWriteStartKey, kQueueDAOWriteEndKey,
								 kQueueDAOReadStartKey, kQueueDAOReadEndKey, kQueueDAOMonitorKey);
	pQueueDAOs->StartUpDAOs();

	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	string strListenPort;
	key_t kSerialNumberSharedMemoryKey;

	ReadClientConfigFile("../ini/CVTrade.ini", strService, strListenPort, kSerialNumberSharedMemoryKey);//todo check
	pClients->SetConfiguration(strService, strListenPort, kSerialNumberSharedMemoryKey);

	while(1)
	{
		sleep(1);
#ifdef MONITOR
		mem_usage(vm, rss);
		cout << "Virtual Memory: " << vm << "\nResident set size: " << rss << endl;
		if(getloadavg(loading, 3) != -1) /*getloadavg is the function used to calculate and obtain the load average*/
		{
			printf("load average : %f , %f , %f\n", loading[0],loading[1],loading[2]);
		}
#endif
		pClients->CheckOnlineClientVector();
		pClients->ClearOfflineClientVector();
	}

	return 0;
}

void ReadQueueDAOConfigFile(string strConfigFileName, string& strService, int& nNumberOfQueueDAO, key_t& kQueueDAOWriteStartKey, key_t& kQueueDAOWriteEndKey, key_t& kQueueDAOReadStartKey, key_t& kQueueDAOReadEndKey, key_t& kQueueDAOMonitorKey)
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
	kQueueDAOMonitorKey = g_key_file_get_integer(keyfile, "Client", "QueueNodeMonitorKey", NULL);
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
	vm_usage = vsize / 1024.0;
	resident_set = rss * page_size_kb;
}
