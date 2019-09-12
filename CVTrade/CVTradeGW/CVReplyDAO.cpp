#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <iconv.h>
#include <curl/curl.h>
#include <algorithm>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include <iomanip>

#include "CVReplyDAO.h"

#define APIKEY "f3-gObpGoi5ECeCjFozXMm4K"
#define APISECRET "i9NmdIydRSa300ZGKP_JHwqnZUpP7S3KB4lf-obHeWgOOOUE"

using json = nlohmann::json;
using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);
static size_t getResponse(char *contents, size_t size, size_t nmemb, void *userp);
static size_t parseHeader(void *ptr, size_t size, size_t nmemb, struct HEADRESP *userdata);

int CSKReplyDAO::HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length) {
	const EVP_MD * engine = EVP_sha256();
	output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key, strlen(key), engine, NULL);
	HMAC_Update(&ctx, (unsigned char*)input, strlen(input));
	HMAC_Final(&ctx, output, &output_length);
	HMAC_CTX_cleanup(&ctx);
	return 0;
}

CSKReplyDAO::CSKReplyDAO(int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey)
{
	m_pHeartbeat = NULL;
	m_TandemDAOStatus = tsNone;
	m_bInuse = false;
	m_pWriteQueueDAOs = CSKWriteQueueDAOs::GetInstance();

	if(m_pWriteQueueDAOs == NULL)
		m_pWriteQueueDAOs = new CSKWriteQueueDAOs(nNumberOfWriteQueueDAO, kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);

	assert(m_pWriteQueueDAOs);

	pthread_mutex_init(&m_MutexLockOnSetStatus, NULL);
	Start();
}

CSKReplyDAO::~CSKReplyDAO()
{
	if(m_pSocket)
	{
		m_pSocket->Disconnect();
		delete m_pSocket;
		m_pSocket = NULL;
	}

	if(m_pHeartbeat)
	{
		delete m_pHeartbeat;
		m_pHeartbeat = NULL;
	}

	if(m_pWriteQueueDAOs)
	{
		delete m_pWriteQueueDAOs;
		m_pWriteQueueDAOs = NULL;
	}
	pthread_mutex_destroy(&m_MutexLockOnSetStatus);
}

void* CSKReplyDAO::Run()
{
	SetStatus(tsServiceOn);
	while(IsTerminated())
	{
		Bitmex_Transaction_Update();
		sleep(1);
	}
	return NULL;
}
TSKTandemDAOStatus CSKReplyDAO::GetStatus()
{
	return m_TandemDAOStatus;
}


void CSKReplyDAO::Bitmex_Transaction_Update()
{
	CURLcode res;
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	int expires = (int)time(NULL)+100, ret;
	char encrystr[256], macoutput[256], execution_str[256], apikey_str[256];
	char qty[10], oprice[10], tprice[10];
	double doprice = 0, dtprice = 0, dqty = 0;
	struct HEADRESP headresponse;
	string response;
	json jtable;

	CURL *m_curl = curl_easy_init();
	curl_global_init(CURL_GLOBAL_ALL);
	string order_url;
	char apiSecret[] = APISECRET;
	sprintf(apikey_str, "api-key: f3-gObpGoi5ECeCjFozXMm4K");
	char commandstr[] = R"({"count":5,"symbol":"XBTUSD","reverse":true})";
	sprintf(encrystr, "GET/api/v1/execution/tradeHistory%d%s", expires, commandstr);
	printf("encrystr = %s\n", encrystr);
	order_url = "https://testnet.bitmex.com/api/v1/execution/tradeHistory";
	char api_secret[48];
	memcpy(api_secret, "i9NmdIydRSa300ZGKP_JHwqnZUpP7S3KB4lf-obHeWgOOOUE", 48);
	HmacEncodeSHA256(apiSecret, 64, encrystr, strlen(encrystr), mac, mac_length);

	for(int i = 0; i < mac_length; i++)
		sprintf(macoutput+i*2, "%02x", (unsigned int)mac[i]);
	printf("signature = %.64s\n", macoutput);

	if(mac)
		free(mac);

	if(m_curl) {
		curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());
		struct curl_slist *http_header;
		http_header = curl_slist_append(http_header, "content-type: application/json");
		http_header = curl_slist_append(http_header, "Accept: application/json");
		http_header = curl_slist_append(http_header, "X-Requested-With: XMLHttpRequest");
		http_header = curl_slist_append(http_header, apikey_str);
		sprintf(execution_str, "api-signature: %.64s", macoutput);
		http_header = curl_slist_append(http_header, execution_str);
		sprintf(execution_str, "api-expires: %d", expires);
		http_header = curl_slist_append(http_header, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, http_header);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, true);
		sprintf(execution_str, "%s", commandstr);
		printf("body: %s\n", execution_str);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(execution_str));
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, parseHeader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &headresponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
		res = curl_easy_perform(m_curl);
		printf("===============\n%s\n==============\n", response.c_str());


		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		curl_slist_free_all(http_header);
		curl_easy_cleanup(m_curl);
	}
	curl_global_cleanup();

}

void CSKReplyDAO::SetStatus(TSKTandemDAOStatus tsStatus)
{
	pthread_mutex_lock(&m_MutexLockOnSetStatus);//lock

	m_TandemDAOStatus = tsStatus;

	pthread_mutex_unlock(&m_MutexLockOnSetStatus);//unlock
}

void CSKReplyDAO::SetInuse(bool bInuse)
{
	m_bInuse = bInuse;
}

bool CSKReplyDAO::IsInuse()
{
	return m_bInuse;
}

static size_t getResponse(char *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static size_t parseHeader(void *ptr, size_t size, size_t nmemb, struct HEADRESP *userdata)
{
	if ( strncmp((char *)ptr, "X-RateLimit-Remaining:", 22) == 0 ) { // get Token
		strtok((char *)ptr, " ");
		(*userdata).remain = (string(strtok(NULL, " \n")));	// token will be stored in userdata
	}
	else if ( strncmp((char *)ptr, "X-RateLimit-Reset", 17) == 0 ) {  // get http response code
		strtok((char *)ptr, " ");
		(*userdata).epoch = (string(strtok(NULL, " \n")));	// http response code
	}
	return nmemb;
}
