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

#include "CVTandemDAO.h"
#include "CVReadQueueDAOs.h"
#include "CVTandemDAOs.h"
#include "CVTIG/CVTandems.h"
#include <nlohmann/json.hpp>
#include <iomanip>

using json = nlohmann::json;
using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

int CSKTandemDAO::HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length) {
	const EVP_MD * engine = EVP_sha256();
	output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key, strlen(key), engine, NULL);
	HMAC_Update(&ctx, (unsigned char*)input, strlen(input));	// input is OK; &input is WRONG !!!
	HMAC_Final(&ctx, output, &output_length);
	HMAC_CTX_cleanup(&ctx);
	return 0;
}

CSKTandemDAO::CSKTandemDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey)
{
	m_pHeartbeat = NULL;
	m_TandemDAOStatus = tsNone;
	m_bInuse = false;
	m_pWriteQueueDAOs = CSKWriteQueueDAOs::GetInstance();

	if(m_pWriteQueueDAOs == NULL)
		m_pWriteQueueDAOs = new CSKWriteQueueDAOs(nNumberOfWriteQueueDAO, kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);

	assert(m_pWriteQueueDAOs);

	m_pTandem = NULL;
	CSKTandemDAOs* pTandemDAOs = CSKTandemDAOs::GetInstance();
	assert(pTandemDAOs);
	if(pTandemDAOs->GetService().compare("TS") == 0)
	{
		m_pTandem = CSKTandems::GetInstance()->GetTandemBitmex();
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

CSKTandemDAO::~CSKTandemDAO()
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

void* CSKTandemDAO::Run()
{
	SetStatus(tsServiceOn);
	while(IsTerminated())
	{
		if(GetStatus() == tsMsgReady)
		{
			CSKWriteQueueDAO* pWriteQueueDAO = NULL;
			while(pWriteQueueDAO == NULL)
			{
				if(m_pWriteQueueDAOs)
					pWriteQueueDAO = m_pWriteQueueDAOs->GetAvailableDAO();

				if(pWriteQueueDAO)
				{
					FprintfStderrLog("GET_WRITEQUEUEDAO", -1, (unsigned char*)&m_tandemreply ,sizeof(m_tandemreply));
					pWriteQueueDAO->SetReplyMessage((unsigned char*)&m_tandemreply, sizeof(m_tandemreply));
					pWriteQueueDAO->TriggerWakeUpEvent();
					SetStatus(tsServiceOn);
					SetInuse(false);
				}
				else
				{
					FprintfStderrLog("GET_WRITEQUEUEDAO_NULL_ERROR", -1, 0, 0);
					usleep(500);
				}
			}
		}
		else
			usleep(100);
	}
	return NULL;
}
TSKTandemDAOStatus CSKTandemDAO::GetStatus()
{
	return m_TandemDAOStatus;
}

void CSKTandemDAO::SetStatus(TSKTandemDAOStatus tsStatus)
{
	pthread_mutex_lock(&m_MutexLockOnSetStatus);//lock

	m_TandemDAOStatus = tsStatus;

	pthread_mutex_unlock(&m_MutexLockOnSetStatus);//unlock
}

void CSKTandemDAO::SetInuse(bool bInuse)
{
	m_bInuse = bInuse;
}

bool CSKTandemDAO::IsInuse()
{
	return m_bInuse;
}

bool CSKTandemDAO::RiskControl()
{
	return false;
}

bool CSKTandemDAO::SendOrder(const unsigned char* pBuf, int nSize)
{
	if(RiskControl())
		return FillRiskMsg(pBuf, nSize);
	else
		return OrderSubmit(pBuf, nSize);
}

size_t getResponse(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t parseHeader(void *ptr, size_t size, size_t nmemb, struct HEADRESP *userdata)
{
    if ( strncmp((char *)ptr, "X-RateLimit-Remaining:", 22) == 0 ) { // get Token
	strtok((char *)ptr, " ");
	(*userdata).limit = (string(strtok(NULL, " \n")));	// token will be stored in userdata
    }
    else if ( strncmp((char *)ptr, "X-RateLimit-Reset", 17) == 0 ) {  // get http response code
	strtok((char *)ptr, " ");
	(*userdata).epoch = (string(strtok(NULL, " \n")));	// http response code
    }
    return nmemb;
}


bool CSKTandemDAO::OrderSubmit(const unsigned char* pBuf, int nToSend)
{
	struct CV_StructTSOrder cv_ts_order;
	memcpy(&cv_ts_order, pBuf, nToSend);
	CURLcode res;
	string buysell_str;
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	int expires = (int)time(NULL)+1000, ret;
	char encrystr[150], commandstr[150], macoutput[128], post_str[100], apikey_str[150];
	char qty[10], oprice[10], tprice[10];
	double doprice = 0, dtprice = 0, dqty = 0;
	struct HEADRESP headresponse;
	string response;
	json jtable;

	memset(commandstr, 0, sizeof(commandstr));
	memset(qty, 0, sizeof(qty));
	memset(oprice, 0, sizeof(oprice));
	memset(tprice, 0, sizeof(tprice));
	memset((void*) &m_tandemreply, 0 , sizeof(m_tandemreply));
	memcpy(qty, cv_ts_order.order_qty, 9);
	memcpy(oprice, cv_ts_order.order_price, 9);
	memcpy(tprice, cv_ts_order.touch_price, 9);
	if(cv_ts_order.order_buysell[0] == 'B')
		buysell_str = "Buy";
	if(cv_ts_order.order_buysell[0] == 'S')
		buysell_str = "Sell";

	switch(cv_ts_order.price_mark[0])
	{
		case '0':
			doprice = atof(oprice) / 10000;
			dtprice = atof(tprice) / 10000;
			break;
		case '1':
			doprice = atoi(oprice);
			dtprice = atoi(tprice);
			break;
		case '2':
			doprice = atof(oprice) / 1000000000;
			dtprice = atof(tprice) / 1000000000;
			break;
		case '3':
		default:
			break;
	}

	switch(cv_ts_order.qty_mark[0])
	{
		case '0':
			dqty = atoi(qty);
			break;
		case '1':
			dqty = atof(qty) / 1000000000;
			break;
		case '2':
			dqty = atof(qty) / 10000;
			break;
		case '3':
		default:
			break;
	}

	CURL *m_curl = curl_easy_init();
	curl_global_init(CURL_GLOBAL_ALL);

	switch(cv_ts_order.trade_type[0])
	{
		case '0'://new order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_order);
			switch(cv_ts_order.order_mark[0])
			{
				case '0'://Market
					sprintf(commandstr, "clOrdID=%.13s,%.7s,%.16s&symbol=XBTUSD&side=%s&orderQty=%d&ordType=Market&timeInForce=GoodTillCancel",
						cv_ts_order.key_id, cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, buysell_str.c_str(), atoi(qty));
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '1'://Limit
					sprintf(commandstr, "clOrdID=%.13s,%.7s,%.16s&symbol=XBTUSD&side=%s&orderQty=%d&price=%.1f&ordType=Limit&timeInForce=GoodTillCancel",
						cv_ts_order.key_id, cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, buysell_str.c_str(), atoi(qty), doprice);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '3'://stop market
					sprintf(commandstr, "clOrdID=%.13s,%.7s,%.16s&symbol=XBTUSD&side=%s&orderQty=%d&stopPx=%.1f&ordType=Stop",
						cv_ts_order.key_id, cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, buysell_str.c_str(), atoi(qty), dtprice);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '4'://stop limit
					sprintf(commandstr, "clOrdID=%.13s,%.7s,%.16s&symbol=XBTUSD&side=%s&orderQty=%d&price=%.1f&stopPx=%.1f&ordType=StopLimit", 
						cv_ts_order.key_id, cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, buysell_str.c_str(), atoi(qty), doprice, dtprice);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '2'://Protect
				default:
					printf("Error order type.\n");
					break;
			}
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_order, strlen(cv_ts_order.apiSecret_order), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_URL, ORDER_URL);
			break;
		case '1'://delete specific order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_cancel);
			sprintf(commandstr, "orderID=%.36s", cv_ts_order.order_bookno);
			sprintf(encrystr, "DELETE/api/v1/order%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_cancel, strlen(cv_ts_order.apiSecret_cancel), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(m_curl, CURLOPT_URL, ORDER_URL);
			break;
		case '2'://delete all order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_cancel);
			sprintf(commandstr, "symbol=%.6s", cv_ts_order.symbol_name); 
			sprintf(encrystr, "DELETE/api/v1/order/all%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_cancel, strlen(cv_ts_order.apiSecret_cancel), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(m_curl, CURLOPT_URL, ORDER_ALL_URL);
			break;
		case '3'://change qty
			break;
		case '4'://change price
			break;
	}

	printf("encrystr = %s\n", encrystr);

	for(int i = 0; i < mac_length; i++) {
		sprintf(macoutput+i*2, "%02x", (unsigned int)mac[i]);
	}

	if(mac) {
		free(mac);
	}

	if(m_curl) {
		struct curl_slist *http_header;
		http_header = curl_slist_append(http_header, "Content-Type: application/x-www-form-urlencoded");
		http_header = curl_slist_append(http_header, "Accept: application/json");
		http_header = curl_slist_append(http_header, "X-Requested-With: XMLHttpRequest");
		http_header = curl_slist_append(http_header, apikey_str);
		sprintf(post_str, "api-signature: %.64s", macoutput);
		http_header = curl_slist_append(http_header, post_str);
		sprintf(post_str, "api-expires: %d", expires);
		http_header = curl_slist_append(http_header, post_str);
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, http_header);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, true);
		sprintf(post_str, "%s", commandstr);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(post_str));
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, post_str);
		curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, parseHeader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &headresponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
		res = curl_easy_perform(m_curl);
		printf("\n===================\n%s\n===================\n", response.c_str());

		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		curl_slist_free_all(http_header);
		curl_easy_cleanup(m_curl);
	}

	curl_global_cleanup();

	m_requestlimit = headresponse.limit;
	m_timelimit = headresponse.epoch;

	char notation;
	for(int i=0 ; i<response.length() ; i++)
	{
		if(response[i] == '{' || response[i] == '[') {
			notation = response[i];
			if(response[i] == '{') {
				jtable = json::parse(&(response[i]));
				break;
			}
			break;
		}
	}
	string text = to_string(jtable["error"]["message"]);

	if(text != "null")
	{
		memcpy(&m_tandemreply.original, &cv_ts_order, sizeof(cv_ts_order));
		memcpy(m_tandemreply.key_id, cv_ts_order.key_id, 13);
		memcpy(m_tandemreply.status_code, "1001", 4);
		sprintf(m_tandemreply.reply_msg, "submit fail, error message:%s", text.c_str());
		SetStatus(tsMsgReady);
	}
	else
	{
		memcpy(m_tandemreply.status_code, "1000", 4);
		memcpy(&m_tandemreply.original, &cv_ts_order, sizeof(cv_ts_order));
		memcpy(m_tandemreply.key_id, cv_ts_order.key_id, 13);
		if(notation != '[') {
			string orderbookNo = to_string(jtable["orderID"]);
			memcpy(m_tandemreply.bookno, orderbookNo.c_str()+1, 36);
			sprintf(m_tandemreply.reply_msg, "submit success, orderID(BookNo):%.36s", m_tandemreply.bookno);
		}
		SetStatus(tsMsgReady);
	}
//	printf("m_tandemreply.trade_type = %c\n", cv_ts_order.trade_type[0]);
	return true;
}


