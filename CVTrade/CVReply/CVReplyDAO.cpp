#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <iconv.h>
#include <curl/curl.h>
#include <algorithm>
#include <openssl/hmac.h>

#include "CVReplyDAO.h"
#include "CVReplyDAOs.h"
#include "CVTIG/CVReplys.h"
#include <nlohmann/json.hpp>
#include <iomanip>

using json = nlohmann::json;
using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

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
int CCVReplyDAO::HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length) {
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

CCVReplyDAO::CCVReplyDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey)
{
	m_pHeartbeat = NULL;
	m_TandemDAOStatus = tsNone;
	m_bInuse = false;
	m_pWriteQueueDAOs = CCVWriteQueueDAOs::GetInstance();

	if(m_pWriteQueueDAOs == NULL)
		m_pWriteQueueDAOs = new CCVWriteQueueDAOs(nNumberOfWriteQueueDAO, kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);

	assert(m_pWriteQueueDAOs);

	m_pTandem = NULL;
	CCVReplyDAOs* pTandemDAOs = CCVReplyDAOs::GetInstance();
	assert(pTandemDAOs);
	if(pTandemDAOs->GetService().compare("RP") == 0)
	{
		m_pTandem = CCVReplys::GetInstance()->GetTandemBitmex();
	}
	else
	{
		FprintfStderrLog("create TandemDAO object fail", 0, 0, 0);
	}
	m_nTandemNodeIndex = nTandemDAOID;
	pthread_mutex_init(&m_MutexLockOnSetStatus, NULL);
	sprintf(m_caTandemDAOID, "%03d", nTandemDAOID);
	Start();
}

CCVReplyDAO::~CCVReplyDAO()
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

void* CCVReplyDAO::Run()
{
	SetStatus(tsServiceOn);
	Bitmex_Getkey();
	Binance_Getkey();
	while(IsTerminated())
	{
		for(int i=0 ; i<m_numberOfkey_Bitmex ; i++)
		{
			//Bitmex_Transaction_Update(5, "XBTUSD", m_vApikeyTable_Bitmex[i]);
			//Bitmex_Order_Update(5, "XBTUSD", m_vApikeyTable_Bitmex[i]);
		}
		for(int i=0 ; i<m_numberOfkey_Bitmex ; i++)
		{
			Binance_Transaction_Update(5, "BTCUSDT", m_vApikeyTable_Binance[i]);
			//Binance_Order_Update(5, "XBTUSD", m_vApikeyTable_Binance[i]);
			sleep(1);
		}
	}
	return NULL;
}
TCVReplyDAOStatus CCVReplyDAO::GetStatus()
{
	return m_TandemDAOStatus;
}


void CCVReplyDAO::SetStatus(TCVReplyDAOStatus tsStatus)
{
	pthread_mutex_lock(&m_MutexLockOnSetStatus);//lock

	m_TandemDAOStatus = tsStatus;

	pthread_mutex_unlock(&m_MutexLockOnSetStatus);//unlock
}

void CCVReplyDAO::SetInuse(bool bInuse)
{
	m_bInuse = bInuse;
}

bool CCVReplyDAO::IsInuse()
{
	return m_bInuse;
}

bool CCVReplyDAO::Binance_Getkey()
{
	CURLcode res;
	CURL *curl = curl_easy_init();
	char query_str[512];
	string readBuffer;

	if(curl) {
		sprintf(query_str,"http://tm1.cryptovix.com.tw:19487/mysql?query=select%%20api_id,api_secret%%20from%%20acv_exchange%%20where%%20exchange_name_en%%20=%%20%%27BINANCE_F%%27");
		curl_easy_setopt(curl, CURLOPT_URL, query_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		json jtable_query_exchange = json::parse(readBuffer.c_str());

		m_numberOfkey_Bitmex = jtable_query_exchange.size();
		if(m_numberOfkey_Bitmex == 0)
		{
                        FprintfStderrLog("GET_BINANCE_KEY_ERROR", -1, 0, 0);
		}
		for(int i=0 ; i<m_numberOfkey_Bitmex ; i++)
		{
			struct APIKEY apikeynode;
			apikeynode.api_id = to_string(jtable_query_exchange[i]["api_id"]);
			apikeynode.api_secret = to_string(jtable_query_exchange[i]["api_secret"]);
			apikeynode.api_id = apikeynode.api_id.substr(1, apikeynode.api_id.length()-2);
			apikeynode.api_secret = apikeynode.api_secret.substr(1, apikeynode.api_secret.length()-2);
#ifdef DEBUG
			cout << setw(4) << jtable_query_exchange << endl;
			printf("Binance - %d:apikeynode.api_id(%d) = %s\n", i, apikeynode.api_id.length(), apikeynode.api_id.c_str());
			printf("Binance - %d:apikeynode.api_key(%d) = %s\n", i, apikeynode.api_secret.length(), apikeynode.api_secret.c_str());
#endif
			m_vApikeyTable_Binance.push_back(apikeynode);
		}
	}
	curl_easy_cleanup(curl);
}

bool CCVReplyDAO::Bitmex_Getkey()
{
	CURLcode res;
	CURL *curl = curl_easy_init();
	char query_str[512];
	string readBuffer;

	if(curl) {
		sprintf(query_str, "http://tm1.cryptovix.com.tw:19487/mysql?query=select%%20api_id,api_secret%%20from%%20acv_exchange%%20where%%20exchange_name_en%%20=%%20%%27BITMEX_T%%27");
		curl_easy_setopt(curl, CURLOPT_URL, query_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		json jtable_query_exchange = json::parse(readBuffer.c_str());

		m_numberOfkey_Bitmex = jtable_query_exchange.size();
		if(m_numberOfkey_Bitmex == 0)
		{
                        FprintfStderrLog("GET_BITMEX_KEY_ERROR", -1, 0, 0);
		}
		for(int i=0 ; i<m_numberOfkey_Bitmex ; i++)
		{
			struct APIKEY apikeynode;
			apikeynode.api_id = to_string(jtable_query_exchange[i]["api_id"]);
			apikeynode.api_secret = to_string(jtable_query_exchange[i]["api_secret"]);
			apikeynode.api_id = apikeynode.api_id.substr(1, apikeynode.api_id.length()-2);
			apikeynode.api_secret = apikeynode.api_secret.substr(1, apikeynode.api_secret.length()-2);
#ifdef DEBUG
			cout << setw(4) << jtable_query_exchange << endl;
			printf("Bitmex - %d:apikeynode.api_id(%d) = %s\n", i, apikeynode.api_id.length(), apikeynode.api_id.c_str());
			printf("Bitmex - %d:apikeynode.api_key(%d) = %s\n", i, apikeynode.api_secret.length(), apikeynode.api_secret.c_str());
#endif
			m_vApikeyTable_Bitmex.push_back(apikeynode);
		}
	}
	curl_easy_cleanup(curl);
}

void CCVReplyDAO::Bitmex_Transaction_Update(int count, string symbol, struct APIKEY apikeynode)
{
	CURLcode res;
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	int expires = (int)time(NULL)+100, ret;
	char encrystr[256], macoutput[256], execution_str[256], apikey_str[256], apisecret_str[49];
	char qty[10], oprice[10], tprice[10];
	double doprice = 0, dtprice = 0, dqty = 0;
	struct HEADRESP headresponse;
	string response;
	json jtable;

	struct curl_slist *http_header;
	string order_url;

	char commandstr[] = R"({"count":10,"symbol":"XBTUSD","reverse":true})";

	CURL *m_curl = curl_easy_init();
	curl_global_init(CURL_GLOBAL_ALL);
	if(m_curl) {
		order_url = "https://testnet.bitmex.com/api/v1/execution";
		curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());
		http_header = curl_slist_append(http_header, "content-type: application/json");
		http_header = curl_slist_append(http_header, "Accept: application/json");
		http_header = curl_slist_append(http_header, "X-Requested-With: XMLHttpRequest");

		sprintf(apisecret_str, "%s", apikeynode.api_secret.c_str());
		sprintf(apikey_str, "api-key: %s", apikeynode.api_id.c_str());
		sprintf(encrystr, "GET/api/v1/order%d%s", expires, commandstr);
		HmacEncodeSHA256(apisecret_str, 64, encrystr, strlen(encrystr), mac, mac_length);

		for(int i = 0; i < mac_length; i++)
			sprintf(macoutput+i*2, "%02x", (unsigned int)mac[i]);

		if(mac)
			free(mac);

		http_header = curl_slist_append(http_header, apikey_str);
		sprintf(execution_str, "api-signature: %.64s", macoutput);
		http_header = curl_slist_append(http_header, execution_str);
		sprintf(execution_str, "api-expires: %d", expires);
		http_header = curl_slist_append(http_header, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, http_header);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, true);
		sprintf(execution_str, "%s", commandstr);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(execution_str));
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, parseHeader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &headresponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
		res = curl_easy_perform(m_curl);
		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		curl_slist_free_all(http_header);
		curl_easy_cleanup(m_curl);
		curl_global_cleanup();
#ifdef DEBUG
		printf("apikey_str = %s\n", apikey_str);
		printf("apisec_str = %s\n", apisecret_str);
		printf("encrystr = %s\n", encrystr);
		printf("signature = %.64s\n", macoutput);
		printf("===============\n%s\n==============\n\n\n", response.c_str());
#endif
	}

	m_request_remain = headresponse.remain;
	m_time_limit = headresponse.epoch;
	string text;

	for(int i=0 ; i<response.length() ; i++)
	{
		if(response[i] == '[')
		{
			response = response.substr(i, response.length()-i);
			FprintfStderrLog("GET_ORDER_REPLY", -1, (unsigned char*)response.c_str() ,response.length());
			jtable = json::parse(response.c_str());
			break;
		}
	}

	for(int i=0 ; i<jtable.size() ; i++)
	{
		text = to_string(jtable[i]["error"]);

		printf("\n\n\ntext = %s\n", text.c_str());

		memcpy(m_trade_reply[i].bookno, to_string(jtable[i]["orderID"]).c_str()+1, 36);
#ifdef DEBUG		
		//cout << setw(4) << jtable[i] << endl;
#endif
		if(text != "null")
		{
			memcpy(m_trade_reply[i].status_code, "1001", 4);
			sprintf(m_trade_reply[i].reply_msg, "reply fail, error message:%s", text.c_str());
		}
		else
		{
			memcpy(m_trade_reply[i].status_code, "1000", 4);
			sprintf(m_trade_reply[i].reply_msg, "reply success - [%s]", to_string(jtable[i]["text"]).c_str());
			LogOrderReplyDB_Bitmex(&jtable[i]);
		}
		memcpy(m_trade_reply[i].key_id, to_string(jtable[i]["clOrdID"]).c_str(), 13);
		memcpy(m_trade_reply[i].price, to_string(jtable[i]["price"]).c_str(), 10);
		memcpy(m_trade_reply[i].avgPx, to_string(jtable[i]["avgPx"]).c_str(), 10);
		memcpy(m_trade_reply[i].orderQty, to_string(jtable[i]["orderQty"]).c_str(), 10);
		memcpy(m_trade_reply[i].leaveQty, to_string(jtable[i]["leaveQty"]).c_str(), 10);
		memcpy(m_trade_reply[i].cumQty, to_string(jtable[i]["cumQty"]).c_str(), 10);
		memcpy(m_trade_reply[i].transactTime, to_string(jtable[i]["transactTime"]).c_str()+1, 24);
	}

        CCVWriteQueueDAO* pWriteQueueDAO = NULL;

        while(pWriteQueueDAO == NULL)
        {
                if(m_pWriteQueueDAOs)
                        pWriteQueueDAO = m_pWriteQueueDAOs->GetAvailableDAO();

                if(pWriteQueueDAO)
                {
			for(int i=0 ; i<jtable.size() ; i++)
			{
                        	FprintfStderrLog("GET_WRITEQUEUEDAO", -1, (unsigned char*)&m_trade_reply[i] ,sizeof(m_trade_reply[i]));
	                        pWriteQueueDAO->SetReplyMessage((unsigned char*)&m_trade_reply[i], sizeof(m_trade_reply[i]));
        	                pWriteQueueDAO->TriggerWakeUpEvent();
			}
                        SetStatus(tsServiceOn);
                        SetInuse(false);
                }
                else
                {
                        FprintfStderrLog("GET_WRITEQUEUEDAO_NULL_ERROR", -1, 0, 0);
                        usleep(500000);
                }
        }

        printf("request remain: %s\n", m_request_remain.c_str());
        printf("time limit: %s\n", m_time_limit.c_str());


}

void CCVReplyDAO::Binance_Transaction_Update(int count, string symbol, struct APIKEY apikeynode)
{
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
        struct timeval  tv;
        gettimeofday(&tv, NULL);
        double expires = (tv.tv_sec) * 1000 ;
	char encrystr[256], macoutput[256], execution_str[256], apikey_str[256], apisecret_str[49];
	char qty[10], oprice[10], tprice[10];
	double doprice = 0, dtprice = 0, dqty = 0;
	struct HEADRESP headresponse;
	string response;
	json jtable;

	struct curl_slist *http_header;
	string order_url;

	//char commandstr[] = R"({"symbol":"BTCUSDT","limit":500,},)";

	CURL *m_curl = curl_easy_init();

	curl_global_init(CURL_GLOBAL_ALL);

	if(m_curl) {
		order_url = "https://fapi.binance.com/fapi/v1/historicalTrades";
		curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());

		sprintf(apisecret_str, "%.64s", apikeynode.api_secret.c_str());
		sprintf(apikey_str, "X-MBX-APIKEY: %.64s", apikeynode.api_id.c_str());
		sprintf(encrystr, "{\"symbol\":\"BTCUSDT\",\"limit\":\"500\",\"timestamp\":\"%0.1f\",\"recvWindow\":5000", expires);
		HmacEncodeSHA256(apisecret_str, 64, encrystr, strlen(encrystr), mac, mac_length);

		for(int i = 0; i < mac_length; i++)
			sprintf(macoutput+i*2, "%02x", (unsigned int)mac[i]);

		if(mac)
			free(mac);

		http_header = curl_slist_append(http_header, apikey_str);
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, http_header);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, true);
		//sprintf(execution_str, "%s&signature=%s", commandstr, macoutput);
		sprintf(execution_str, "%s,\"signature\":\"%s\"}", encrystr, macoutput);
		printf("execution_str = %s\n", execution_str);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(execution_str));
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, parseHeader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &headresponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
		CURLcode res = curl_easy_perform(m_curl);
		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		curl_slist_free_all(http_header);
		curl_easy_cleanup(m_curl);
		curl_global_cleanup();
#ifdef DEBUG
		printf("apikey_str = %s\n", apikey_str);
		printf("apisec_str = %s\n", apisecret_str);
		printf("encrystr = %s\n", encrystr);
		printf("signature = %.64s\n", macoutput);
		printf("===============\n%s\n==============\n\n\n", response.c_str());
#endif
	}

	m_request_remain = headresponse.remain;
	m_time_limit = headresponse.epoch;
	string text;

	for(int i=0 ; i<response.length() ; i++)
	{
		if(response[i] == '[')
		{
			response = response.substr(i, response.length()-i);
			FprintfStderrLog("GET_ORDER_REPLY", -1, (unsigned char*)response.c_str() ,response.length());
			jtable = json::parse(response.c_str());
			break;
		}
	}

	for(int i=0 ; i<jtable.size() ; i++)
	{
		text = to_string(jtable[i]["error"]);

		printf("\n\n\ntext = %s\n", text.c_str());

		memcpy(m_trade_reply[i].bookno, to_string(jtable[i]["orderID"]).c_str()+1, 36);
#ifdef DEBUG		
		//cout << setw(4) << jtable[i] << endl;
#endif
		if(text != "null")
		{
			memcpy(m_trade_reply[i].status_code, "1001", 4);
			sprintf(m_trade_reply[i].reply_msg, "reply fail, error message:%s", text.c_str());
		}
		else
		{
			memcpy(m_trade_reply[i].status_code, "1000", 4);
			sprintf(m_trade_reply[i].reply_msg, "reply success - [%s]", to_string(jtable[i]["text"]).c_str());
			LogOrderReplyDB_Bitmex(&jtable[i]);
		}
		memcpy(m_trade_reply[i].key_id, to_string(jtable[i]["clOrdID"]).c_str(), 13);
		memcpy(m_trade_reply[i].price, to_string(jtable[i]["price"]).c_str(), 10);
		memcpy(m_trade_reply[i].avgPx, to_string(jtable[i]["avgPx"]).c_str(), 10);
		memcpy(m_trade_reply[i].orderQty, to_string(jtable[i]["orderQty"]).c_str(), 10);
		memcpy(m_trade_reply[i].leaveQty, to_string(jtable[i]["leaveQty"]).c_str(), 10);
		memcpy(m_trade_reply[i].cumQty, to_string(jtable[i]["cumQty"]).c_str(), 10);
		memcpy(m_trade_reply[i].transactTime, to_string(jtable[i]["transactTime"]).c_str()+1, 24);
	}

        CCVWriteQueueDAO* pWriteQueueDAO = NULL;

        while(pWriteQueueDAO == NULL)
        {
                if(m_pWriteQueueDAOs)
                        pWriteQueueDAO = m_pWriteQueueDAOs->GetAvailableDAO();

                if(pWriteQueueDAO)
                {
			for(int i=0 ; i<jtable.size() ; i++)
			{
                        	FprintfStderrLog("GET_WRITEQUEUEDAO", -1, (unsigned char*)&m_trade_reply[i] ,sizeof(m_trade_reply[i]));
	                        pWriteQueueDAO->SetReplyMessage((unsigned char*)&m_trade_reply[i], sizeof(m_trade_reply[i]));
        	                pWriteQueueDAO->TriggerWakeUpEvent();
			}
                        SetStatus(tsServiceOn);
                        SetInuse(false);
                }
                else
                {
                        FprintfStderrLog("GET_WRITEQUEUEDAO_NULL_ERROR", -1, 0, 0);
                        usleep(500000);
                }
        }

        printf("request remain: %s\n", m_request_remain.c_str());
        printf("time limit: %s\n", m_time_limit.c_str());


}



bool CCVReplyDAO::LogOrderReplyDB_Bitmex(json* jtable)
{
	char insert_str[MAXDATA], delete_str[MAXDATA];

	string response, bitmex_data[30];

	string orderID, clOrdID, account, symbol, side, orderQty, price, ordStatus, transactTime, stopPx, avgPx, leavesQty, currency, settlCurrency, text;

	bitmex_data[0] = to_string((*jtable)["account"]);
	bitmex_data[0] = bitmex_data[0].substr(0, bitmex_data[0].length());

	bitmex_data[1] = to_string((*jtable)["execID"]);
	bitmex_data[1] = bitmex_data[1].substr(1, bitmex_data[1].length()-2);

	bitmex_data[2] = to_string((*jtable)["symbol"]);
	bitmex_data[2] = bitmex_data[2].substr(1, bitmex_data[2].length()-2);

	bitmex_data[3] = to_string((*jtable)["side"]);
	bitmex_data[3] = bitmex_data[3].substr(1, bitmex_data[3].length()-2);

	bitmex_data[4] = to_string((*jtable)["lastPx"]);
	bitmex_data[4] = bitmex_data[4].substr(0, bitmex_data[4].length());

	bitmex_data[5] = to_string((*jtable)["lastQty"]);
	bitmex_data[5] = bitmex_data[5].substr(0, bitmex_data[5].length());

	bitmex_data[6] = to_string((*jtable)["cumQty"]);
	bitmex_data[6] = bitmex_data[6].substr(0, bitmex_data[6].length());

	bitmex_data[7] = to_string((*jtable)["leavesQty"]);
	bitmex_data[7] = bitmex_data[7].substr(0, bitmex_data[7].length());

	bitmex_data[8] = to_string((*jtable)["execType"]);
	bitmex_data[8] = bitmex_data[8].substr(1, bitmex_data[8].length()-2);

	bitmex_data[9] = to_string((*jtable)["transactTime"]);
	bitmex_data[9] = bitmex_data[9].substr(1, 19);

	bitmex_data[10] = to_string((*jtable)["commission"]);
	bitmex_data[10] = bitmex_data[10].substr(0, bitmex_data[10].length());

	bitmex_data[11] = to_string((*jtable)["execComm"]);
	bitmex_data[11] = bitmex_data[11].substr(0, bitmex_data[11].length());

	bitmex_data[12] = to_string((*jtable)["orderID"]);
	bitmex_data[12] = bitmex_data[12].substr(1, bitmex_data[12].length()-2);

	bitmex_data[13] = to_string((*jtable)["price"]);
	bitmex_data[13] = bitmex_data[13].substr(0, bitmex_data[13].length());

	bitmex_data[14] = to_string((*jtable)["orderQty"]);
	bitmex_data[14] = bitmex_data[14].substr(0, bitmex_data[14].length());

	bitmex_data[15] = to_string((*jtable)["ordType"]);
	bitmex_data[15] = bitmex_data[15].substr(1, bitmex_data[15].length()-2);

	bitmex_data[16] = to_string((*jtable)["ordStatus"]);
	bitmex_data[16] = bitmex_data[16].substr(1, bitmex_data[16].length()-2);

	bitmex_data[17] = to_string((*jtable)["currency"]);
	bitmex_data[17] = bitmex_data[17].substr(1, bitmex_data[17].length()-2);

	bitmex_data[18] = to_string((*jtable)["settlCurrency"]);
	bitmex_data[18] = bitmex_data[18].substr(1, bitmex_data[18].length()-2);

	bitmex_data[19] = to_string((*jtable)["clOrdID"]);
	bitmex_data[19] = bitmex_data[19].substr(1, bitmex_data[19].length()-2);

	bitmex_data[20] = to_string((*jtable)["text"]);
	bitmex_data[20] = bitmex_data[20].substr(1, bitmex_data[20].length()-2);

	sprintf(insert_str, "http://tm1.cryptovix.com.tw:19487/mysql?db=Cryptovix_test&query=insert%%20into%%20bitmex_match_history%%20set%%20exchange=%27BITMEX%27,account=%%27%s%%27,match_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,match_qty=%%27%s%%27,match_cum_qty=%%27%s%%27,remaining_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,commission=%%27%s%%27,order_no=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27", bitmex_data[0].c_str(), bitmex_data[1].c_str(), bitmex_data[2].c_str(), bitmex_data[3].c_str(), bitmex_data[5].c_str(), bitmex_data[6].c_str(), bitmex_data[7].c_str(), bitmex_data[8].c_str(), bitmex_data[9].c_str(), bitmex_data[11].c_str(), bitmex_data[12].c_str(), bitmex_data[14].c_str(), bitmex_data[15].c_str(), bitmex_data[16].c_str(), bitmex_data[17].c_str(), bitmex_data[18].c_str(), bitmex_data[19].c_str(), bitmex_data[20].c_str());

	if(bitmex_data[4] != "null")
		sprintf(insert_str, "%s,match_price=%%27%s%%27", insert_str, bitmex_data[4].c_str());
	if(bitmex_data[10] != "null")
		sprintf(insert_str, "%s,commission_rate=%%27%s%%27", insert_str, bitmex_data[10].c_str());
	if(bitmex_data[13] != "null")
		sprintf(insert_str, "%s,order_price=%%27%s%%27", insert_str, bitmex_data[13].c_str());
	sprintf(insert_str, "%s,insert_user=%%27trade.server%%27,update_user=%%27trade.server%%27", insert_str);

	sprintf(delete_str, "http://tm1.cryptovix.com.tw:19487/mysql?db=Cryptovix_test&query=update%%20bitmex_match_history%%20set%%20exchange=%27BITMEX%27,account=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,match_qty=%%27%s%%27,match_cum_qty=%%27%s%%27,remaining_qty=%%27%s%%27,match_type=%%27%s%%27,match_time=%%27%s%%27,commission=%%27%s%%27,order_no=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27", bitmex_data[0].c_str(), bitmex_data[2].c_str(), bitmex_data[3].c_str(), bitmex_data[5].c_str(), bitmex_data[6].c_str(), bitmex_data[7].c_str(), bitmex_data[8].c_str(), bitmex_data[9].c_str(), bitmex_data[11].c_str(), bitmex_data[12].c_str(), bitmex_data[14].c_str(), bitmex_data[15].c_str(), bitmex_data[16].c_str(), bitmex_data[17].c_str(), bitmex_data[18].c_str(), bitmex_data[19].c_str(), bitmex_data[20].c_str());

	if(bitmex_data[4] != "null")
		sprintf(delete_str, "%s,match_price=%%27%s%%27", delete_str, bitmex_data[4].c_str());
	if(bitmex_data[10] != "null")
		sprintf(delete_str, "%s,commission_rate=%%27%s%%27", delete_str, bitmex_data[10].c_str());
	if(bitmex_data[13] != "null")
		sprintf(delete_str, "%s,order_price=%%27%s%%27", delete_str, bitmex_data[13].c_str());
		sprintf(delete_str, "%s,insert_user=%%27trade.server%%27,update_user=%%27trade.server%%27", delete_str);
		sprintf(delete_str, "%s%%20where%%20match_no=%%27%s%%27", delete_str, bitmex_data[1].c_str());
	printf("=============\n%s\n=============\n", delete_str);
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, delete_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_perform(curl);
	printf("=============\n%s\n=============\n", insert_str);
	curl_easy_setopt(curl, CURLOPT_URL, insert_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return true;
}
