#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <glib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include "CVServer.h"
#include "CVWebConns.h"
#include "CVQueueNodes.h"

#define PACKETSIZE  64
using namespace std;

void InitialGlobal();

void mem_usage(double& vm_usage, double& resident_set);
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

struct packet
{
		struct icmphdr hdr;
		char msg[PACKETSIZE-sizeof(struct icmphdr)];
};
int main()
{
	int nNumberOfQueueDAO;
	key_t kQueueDAOWriteStartKey;
	key_t kQueueDAOWriteEndKey;
	key_t kQueueDAOReadStartKey;
	key_t kQueueDAOReadEndKey;
	key_t kQueueDAOMonitorKey;
	double vm, rss;

	string strService;

	InitialGlobal();
	setbuf(stdout, NULL);
	signal(SIGPIPE, SIG_IGN );
	srand(time(NULL));

	DECLARE_CONFIG_DATA(TSConfig);

	string strListenPort, strHeartBeatTime, strEPIDNum;
	ReadClientConfigFile("../ini/CVReplyWS.ini", strListenPort, strHeartBeatTime, strEPIDNum);

	int nService = 0;
	printf("Reply program start\n");
	ReadConfigFile("../ini/CVReplyWS.ini", "EXCHANGE", struTSConfig);

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
	ReadQueueDAOConfigFile("../ini/CVReplyWS.ini", strService, nNumberOfQueueDAO, kQueueDAOWriteStartKey, kQueueDAOWriteEndKey, kQueueDAOReadStartKey, kQueueDAOReadEndKey, kQueueDAOMonitorKey);
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

		//Disconnection check.
		while(1)
		{
			pClients->CheckClientVector();
			pServers->CheckClientVector();
#ifdef MONITOR
			mem_usage(vm, rss);
			cout << "Virtual Memory: " << vm << "\nResident set size: " << rss << endl;
#endif
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

	char caWeb[5];
	char caQstr[7];
	char caName[7];
	for(int i=0;i<struConfig.nServerCount;i++)
	{
		memset(caWeb, 0, sizeof(caWeb));
		memset(caQstr, 0, sizeof(caQstr));
		memset(caName, 0, sizeof(caName));

		sprintf(caWeb, "WEB%02d", i+1);
		sprintf(caQstr, "QSTR%02d", i+1);
		sprintf(caName, "NAME%02d", i+1);

		struct TCVServerInfo* pstruServerInfo = new struct TCVServerInfo;//destructor
	try {
		pstruServerInfo->strWeb  = g_key_file_get_string(keyfile, strSection.c_str(), caWeb, NULL);
	}
	catch (const char* pErrorMessage)
	{
		FprintfStderrLog(pErrorMessage, -1, 0, __FILE__, __LINE__);
	}
		pstruServerInfo->strQstr = g_key_file_get_string(keyfile, strSection.c_str(), caQstr, NULL);
		pstruServerInfo->strName = g_key_file_get_string(keyfile, strSection.c_str(), caName, NULL);
		printf("Connect web: %s\n", pstruServerInfo->strWeb.c_str());
		printf("Query strnig: %s\n", pstruServerInfo->strQstr.c_str());
		printf("Exchange: %s\n", pstruServerInfo->strName.c_str());
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
	strEPIDNum	   = g_key_file_get_string(keyfile, "SERVER", "EpidNum",	 NULL);
	strListenPort	= g_key_file_get_string(keyfile, "SERVER", "ListenPort",	NULL);
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

	strService		 = g_key_file_get_string (keyfile, "QUEUE", "Service", NULL);
	nNumberOfQueueDAO	  = g_key_file_get_integer(keyfile, "QUEUE", "NumberOfQueueDAO", NULL);
	kQueueDAOWriteStartKey = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeWriteStartKey", NULL);
	kQueueDAOWriteEndKey   = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeWriteEndKey", NULL);
	kQueueDAOReadStartKey  = g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeReadStartKey", NULL);
	kQueueDAOReadEndKey	= g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeReadEndKey", NULL);
	kQueueDAOMonitorKey	= g_key_file_get_integer(keyfile, "QUEUE", "QueueNodeMonitorKey", NULL);
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


unsigned short checksum(void *b, int len)
{
	unsigned short *buf = (unsigned short *)b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

int ping(const char *adress)
{
	static int cnt=1;
	const int val=255;
	int i, sd;
	struct packet pckt;
	struct sockaddr_in r_addr;
	int len=sizeof(r_addr);
	int loop = 100;
	struct hostent *hname;
	struct sockaddr_in addr_ping,*addr;
	struct protoent *proto = getprotobyname("ICMP");
	hname = gethostbyname(adress);
	bzero(&addr_ping, sizeof(addr_ping));
	addr_ping.sin_family = hname->h_addrtype;
	addr_ping.sin_port = 0;
	addr_ping.sin_addr.s_addr = *(long*)hname->h_addr;
	addr = &addr_ping;
	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
			perror("socket");
			return 1;
	}
	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
	{
			perror("Set TTL option");
			return 1;
	}
	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
	{
			perror("Request nonblocking I/O");
			return 1;
	}
	bzero(&pckt, sizeof(pckt));
	pckt.hdr.type = ICMP_ECHO;

	for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = i+'0';

	pckt.msg[i] = 0;
	pckt.hdr.un.echo.sequence = cnt++;
	pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
	if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
			perror("sendto");
	while ( recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, (socklen_t *)&len) <= 0 && loop)
	{
		loop--;
		usleep(10000);
	}
	if(loop)
		return 0;
	else
		return 1;
}

