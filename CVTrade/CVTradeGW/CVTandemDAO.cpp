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

#include "../include/CVType.h"
#include "../include/CVGlobal.h"
#include "CVTandemDAO.h"
#include "CVReadQueueDAOs.h"
#include "CVTandemDAOs.h"
#include "CVTIG/CVTandems.h"

using namespace std;

extern void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0);

int HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length) {
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
	return NULL;
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


bool CSKTandemDAO::SendData(const unsigned char* pBuf, int nSize)
{
	return SendAll(pBuf, nSize);
}

bool CSKTandemDAO::SendAll(const unsigned char* pBuf, int nToSend)
{
printf("size of CVTS = %d\n", sizeof(struct CV_StructTSOrder));

	struct CV_StructTSOrder cv_ts_order;
	memcpy(&cv_ts_order, pBuf, nToSend);
#if 0
	printf("ip = %.16s\n", cv_ts_order.client_ip);
	printf("order id = %.10s\n", cv_ts_order.order_id);
	printf("sub_acno_id = %.7s\n", cv_ts_order.sub_acno_id);
	printf("strategy_name = %.10s\n", cv_ts_order.strategy_name);
	printf("agent_id = %.2s\n", cv_ts_order.agent_id);
	printf("broker_id = %.4s\n", cv_ts_order.broker_id);
	printf("exchange_id = %10.s\n", cv_ts_order.exchange_id);
	printf("seq_id = %.13s\n", cv_ts_order.seq_id);
	printf("symbol_name = %.10s\n", cv_ts_order.symbol_name);
	printf("symbol_type = %.1s\n", cv_ts_order.symbol_type);
	printf("symbol_mark = %.1s\n", cv_ts_order.symbol_mark);
	printf("order_offset = %.1s\n", cv_ts_order.order_offset);
	printf("order_dayoff = %.1s\n", cv_ts_order.order_dayoff);
	printf("order_date = %.8s\n", cv_ts_order.order_date);
	printf("order_time = %.8s\n", cv_ts_order.order_time);
	printf("order_type = %.1s\n", cv_ts_order.order_type);
	printf("order_buysell = %.1s\n", cv_ts_order.order_buysell);
	printf("order_bookno = %.36s\n", cv_ts_order.order_bookno);
	printf("order_cond = %.1s\n", cv_ts_order.order_cond);
	printf("order_mark = %.1s\n", cv_ts_order.order_mark);
	printf("trade_type = %.1s\n", cv_ts_order.trade_type);
	printf("price_mark = %.1s\n", cv_ts_order.price_mark);
	printf("order_price = %.9s\n", cv_ts_order.order_price);
	printf("touch_price = %.9s\n", cv_ts_order.touch_price);
	printf("qty_mark = %.1s\n", cv_ts_order.qty_mark);
	printf("order_qty = %.9s\n", cv_ts_order.order_qty);
	printf("order_kind = %.2s\n", cv_ts_order.order_kind);
#endif
#if 0
	char apikey_order[24];
	char apikey_cancel[24];
        char secretkey_order[48];
        char secretkey_cancel[48];
#endif
	CURL *curl;
	CURLcode res;
	string buysell_str;
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	int expires = (int)time(NULL)+1000;
	char encrystr[1024];
	char commandstr[1024];
	char macoutput[64];
	char post_str[1024];
	char apikey_str[1024];
	char qty[10];
	char oprice[10];
	char tprice[10];
	double doprice = 0;
	double dtprice = 0;
	double dqty = 0;
	int ret;

	memset(commandstr, 0, sizeof(commandstr));
	memset(qty, 0, sizeof(qty));
	memset(oprice, 0, sizeof(oprice));
	memset(tprice, 0, sizeof(tprice));
	memcpy(qty, cv_ts_order.order_qty, 9);
	memcpy(oprice, cv_ts_order.order_price, 9);
	memcpy(tprice, cv_ts_order.touch_price, 9);

	if(cv_ts_order.order_buysell[0] == 'B')
		buysell_str = "Buy";
	else if(cv_ts_order.order_buysell[0] == 'S')
		buysell_str = "Sell";
	else
		printf("Error buy/sell notation.\n");

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

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	switch(cv_ts_order.trade_type[0])
	{
		case '0'://new order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_order);
			switch(cv_ts_order.order_mark[0])
			{
				case '0'://Market
					sprintf(commandstr, "symbol=XBTUSD&side=%s&orderQty=%d&ordType=Market&timeInForce=GoodTillCancel", buysell_str.c_str(), atoi(qty));
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '1'://Limit
					sprintf(commandstr, "symbol=XBTUSD&side=%s&orderQty=%d&price=%.1f&ordType=Limit&timeInForce=GoodTillCancel", buysell_str.c_str(), atoi(qty), doprice);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '2'://Protect
					printf("Error order type.\n");
					break;
				case '3'://stop market
					sprintf(commandstr, "symbol=XBTUSD&side=%s&orderQty=%d&stopPx=%.1f&ordType=Stop", buysell_str.c_str(), atoi(qty), dtprice);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '4'://stop limit
					sprintf(commandstr, "symbol=XBTUSD&side=%s&orderQty=%d&price=%.1f&stopPx=%.1f&ordType=StopLimit", 
						buysell_str.c_str(), atoi(qty), doprice, dtprice);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				default:
					printf("Error order type.\n");
					break;
			}
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_order, strlen(cv_ts_order.apiSecret_order), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(curl, CURLOPT_URL, ORDER_URL);
			break;
		case '1'://delete specific order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_cancel);
			sprintf(commandstr, "orderID=%.36s", cv_ts_order.order_bookno);
			sprintf(encrystr, "DELETE/api/v1/order%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_cancel, strlen(cv_ts_order.apiSecret_cancel), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(curl, CURLOPT_URL, ORDER_URL);
			break;
		case '2'://delete all order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_cancel);
			sprintf(commandstr, "symbol=%.6s", cv_ts_order.symbol_name); 
			sprintf(encrystr, "DELETE/api/v1/order/all%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_cancel, strlen(cv_ts_order.apiSecret_cancel), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(curl, CURLOPT_URL, ORDER_ALL_URL);
			break;
		case '3'://change qty
			break;
		case '4'://change price
			break;
	}
	printf("Command str = %s\n", commandstr);

	if(0 == ret) {
		cout << "Algorithm HMAC encode succeeded!" << endl;
	}
	else {
		cout << "Algorithm HMAC encode failed!" << endl;
		return -1;
	}

	for(int i = 0; i < mac_length; i++) {
		sprintf(macoutput+i*2, "%02x", (unsigned int)mac[i]);
	}
	cout << endl;

	if(curl) {
		printf("apikey_str = %s\n", apikey_str);
		struct curl_slist *chunk = curl_slist_append(chunk, "Content-Type: application/x-www-form-urlencoded");
		chunk = curl_slist_append(chunk, "Accept: application/json");
		chunk = curl_slist_append(chunk, "X-Requested-With: XMLHttpRequest");
		chunk = curl_slist_append(chunk, apikey_str);
		sprintf(post_str, "api-signature: %s", macoutput);
		chunk = curl_slist_append(chunk, post_str);
		sprintf(post_str, "api-expires: %d", expires);
		chunk = curl_slist_append(chunk, post_str);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl, CURLOPT_HEADER, true);
		sprintf(post_str, "%s", commandstr);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_str));
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_str);

		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		curl_easy_cleanup(curl);
	}
	if(mac) {
		free(mac);
	}


	curl_global_cleanup();

	return true;
}


