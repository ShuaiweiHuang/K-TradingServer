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
static size_t getResponse(char *contents, size_t size, size_t nmemb, void *userp);
static size_t parseHeader(void *ptr, size_t size, size_t nmemb, struct HEADRESP *userdata);

int CCVTandemDAO::HmacEncodeSHA256( const char * key, unsigned int key_length, const char * input, unsigned int input_length, unsigned char * &output, unsigned int &output_length) {
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

CCVTandemDAO::CCVTandemDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey)
{
	m_pHeartbeat = NULL;
	m_TandemDAOStatus = tsNone;
	m_bInuse = false;
	m_pWriteQueueDAOs = CCVWriteQueueDAOs::GetInstance();

	if(m_pWriteQueueDAOs == NULL)
		m_pWriteQueueDAOs = new CCVWriteQueueDAOs(nNumberOfWriteQueueDAO, kWriteQueueDAOStartKey, kWriteQueueDAOEndKey);

	assert(m_pWriteQueueDAOs);

	m_pTandem = NULL;
	CCVTandemDAOs* pTandemDAOs = CCVTandemDAOs::GetInstance();
	assert(pTandemDAOs);
	if(pTandemDAOs->GetService().compare("TS") == 0)
	{
		m_pTandem = CCVTandems::GetInstance()->GetTandemBitmex();
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

CCVTandemDAO::~CCVTandemDAO()
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

void* CCVTandemDAO::Run()
{
	SetStatus(tsServiceOn);
	while(IsTerminated())
	{
		//Bitmex_Transaction_Update();
		sleep(1);
	}
	return NULL;
}
TCVTandemDAOStatus CCVTandemDAO::GetStatus()
{
	return m_TandemDAOStatus;
}

#define APIKEY "f3-gObpGoi5ECeCjFozXMm4K"
#define APISECRET "i9NmdIydRSa300ZGKP_JHwqnZUpP7S3KB4lf-obHeWgOOOUE"

void CCVTandemDAO::Bitmex_Transaction_Update()
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

void CCVTandemDAO::SetStatus(TCVTandemDAOStatus tsStatus)
{
	pthread_mutex_lock(&m_MutexLockOnSetStatus);//lock

	m_TandemDAOStatus = tsStatus;

	pthread_mutex_unlock(&m_MutexLockOnSetStatus);//unlock
}

void CCVTandemDAO::SetInuse(bool bInuse)
{
	m_bInuse = bInuse;
}

bool CCVTandemDAO::IsInuse()
{
	return m_bInuse;
}

bool CCVTandemDAO::RiskControl()
{
#if 0
	if(limit <10)
		return true;
#endif	
	return false;
}

bool CCVTandemDAO::SendOrder(const unsigned char* pBuf, int nSize)
{
	if(RiskControl())
		return FillRiskMsg(pBuf, nSize);
	else
		return OrderSubmit(pBuf, nSize);
}

bool CCVTandemDAO::FillRiskMsg(const unsigned char* pBuf, int nSize)
{
	struct CV_StructTSOrder cv_ts_order;
	memcpy(&cv_ts_order, pBuf, nSize);
	//add reply message
	memcpy(&m_tandem_reply.original, &cv_ts_order, sizeof(cv_ts_order));
	memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);
	memcpy(m_tandem_reply.status_code, "1002", 4);
	sprintf(m_tandem_reply.reply_msg, "submit fail, due to risk control issue");
	SetStatus(tsMsgReady);
	return true;
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


void CCVTandemDAO::SendNotify(char* pBuf)
{
		CURL *curl;
		CURLcode res;
		curl_global_init(CURL_GLOBAL_ALL);

		curl = curl_easy_init();
		if(curl) {
			struct curl_slist *chunk;
			chunk = curl_slist_append(chunk, "Authorization: Bearer naBftXyXgOX00lJg8Fl78QyKD5hNe6by4PWjJ5sArVU");
			chunk = curl_slist_append(chunk, "Content-Type: application/x-www-form-urlencoded");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
			curl_easy_setopt(curl, CURLOPT_HEADER, true);
			curl_easy_setopt(curl, CURLOPT_URL, "https://notify-api.line.me/api/notify");
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(pBuf));
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pBuf);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();
}


bool CCVTandemDAO::OrderSubmit(const unsigned char* pBuf, int nToSend)
{
	struct CV_StructTSOrder cv_ts_order;
	memcpy(&cv_ts_order, pBuf, nToSend);

	if(!strcmp(cv_ts_order.exchange_id, "BITMEX_T") || !strcmp(cv_ts_order.exchange_id, "BITMEX"))
	{
		return OrderSubmit_Bitmex(cv_ts_order, nToSend);
	}
	if(!strcmp(cv_ts_order.exchange_id, "BINANCE_T") || !strcmp(cv_ts_order.exchange_id, "BINANCE"))
	{
		SetInuse(false);
		return true;
		//return OrderSubmit_Binance(cv_ts_order, nToSend);
	}
	SetInuse(false);
	return true;
}

bool CCVTandemDAO::OrderSubmit_Bitmex(struct CV_StructTSOrder cv_ts_order, int nToSend)
{
	CURLcode res;
	string buysell_str;
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	int expires = (int)time(NULL)+1000, ret;
	char encrystr[256], commandstr[256], macoutput[256], execution_str[256], apikey_str[256];
	char qty[10], oprice[10], tprice[10];
	double doprice = 0, dtprice = 0, dqty = 0;
	struct HEADRESP headresponse;
	string response;
	json jtable;

	memset(commandstr, 0, sizeof(commandstr));
	memset(qty, 0, sizeof(qty));
	memset(oprice, 0, sizeof(oprice));
	memset(tprice, 0, sizeof(tprice));
	memset((void*) &m_tandem_reply, 0 , sizeof(m_tandem_reply));
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
	string order_url, order_all_url;
	if(!strcmp(cv_ts_order.exchange_id, "BITMEX_T"))
	{
		order_url = "https://testnet.bitmex.com/api/v1/order";
		order_all_url = "https://testnet.bitmex.com/api/v1/order/all";
	}
	
	if(!strcmp(cv_ts_order.exchange_id, "BITMEX"))
	{
		order_url = "https://www.bitmex.com/api/v1/order";
		order_all_url = "https://www.bitmex.com/api/v1/order/all";
	}

	switch(cv_ts_order.trade_type[0])
	{
		case '0'://new order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_order);
			switch(cv_ts_order.order_mark[0])
			{
				case '0'://Market
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&ordType=Market&timeInForce=GoodTillCancel&text=%.7s|%.16s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), atoi(qty),cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, cv_ts_order.username);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '1'://Limit
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&price=%.1f&ordType=Limit&timeInForce=GoodTillCancel&text=%.7s|%.16s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), atoi(qty), doprice,
					cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, cv_ts_order.username);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '3'://stop market
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&stopPx=%.1f&ordType=Stop&text=%.7s|%.16s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), atoi(qty), dtprice,
					cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, cv_ts_order.username);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '4'://stop limit
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&price=%.1f&stopPx=%.1f&ordType=StopLimit&text=%.7s|%.16s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), atoi(qty), doprice, dtprice,
					cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, cv_ts_order.username);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '2'://Protect
				default:
					printf("Error order type.\n");
					break;
			}
			printf("encrystr = %s\n", encrystr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_order, strlen(cv_ts_order.apiSecret_order), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());
			break;
		case '1'://delete specific order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_cancel);
			sprintf(commandstr, "orderID=%.36s", cv_ts_order.order_bookno);
			sprintf(encrystr, "DELETE/api/v1/order%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_cancel, strlen(cv_ts_order.apiSecret_cancel), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());
			break;
		case '2'://delete all order
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_cancel);
			sprintf(commandstr, "symbol=%.6s", cv_ts_order.symbol_name); 
			sprintf(encrystr, "DELETE/api/v1/order/all%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_cancel, strlen(cv_ts_order.apiSecret_cancel), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(m_curl, CURLOPT_URL, order_all_url.c_str());
			break;
		case '3'://change qty
		case '4'://change price
		default :
			break;
	}


#if 1//test
	sprintf(encrystr, "message=%s:%.6s:%1f", buysell_str.c_str(), cv_ts_order.symbol_name, doprice);
	SendNotify(encrystr);
#endif
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
		sprintf(execution_str, "api-signature: %.64s", macoutput);
		http_header = curl_slist_append(http_header, execution_str);
		sprintf(execution_str, "api-expires: %d", expires);
		http_header = curl_slist_append(http_header, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, http_header);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, true);
		sprintf(execution_str, "%s", commandstr);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(execution_str));
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, parseHeader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &headresponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
		res = curl_easy_perform(m_curl);
		//printf("\n===================\n%s\n===================\n", response.c_str());
		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		curl_slist_free_all(http_header);
		curl_easy_cleanup(m_curl);
	}
	curl_global_cleanup();

	m_request_remain = headresponse.remain;
	m_time_limit = headresponse.epoch;
	string text;
	switch(cv_ts_order.trade_type[0])
	{
		case '0'://new
			for(int i=0 ; i<response.length() ; i++)
			{
				if(response[i] == '{') {
					response = response.substr(i, response.length()-i);
					FprintfStderrLog("GET_ORDER_REPLY", -1, (unsigned char*)response.c_str() ,response.length());
					jtable = json::parse(response.c_str());
					break;
				}
			}
			text = to_string(jtable["error"]["message"]);

			if(text != "null")
			{
				memcpy(&m_tandem_reply.original, &cv_ts_order, sizeof(cv_ts_order));
				memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);
				memcpy(m_tandem_reply.status_code, "1001", 4);
				sprintf(m_tandem_reply.reply_msg, "submit fail, error message:%s", text.c_str());
			}
			else
			{
				memcpy(m_tandem_reply.status_code, "1000", 4);
				memcpy(&m_tandem_reply.original, &cv_ts_order, sizeof(cv_ts_order));
				memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);
				string orderbookNo = to_string(jtable["orderID"]);
				memcpy(m_tandem_reply.bookno, orderbookNo.c_str()+1, 36);
				sprintf(m_tandem_reply.reply_msg, "submit success, orderID(BookNo):%.36s", m_tandem_reply.bookno);
				LogOrderReplyDB_Bitmex(&jtable, OPT_ADD);
			}
			SetStatus(tsMsgReady);
			break;
		case '1'://old
		case '2':
			for(int i=0 ; i<response.length() ; i++)
			{
				if(response[i] == '[') {
					response = response.substr(i, response.length()-i);
					FprintfStderrLog("GET_ORDER_REPLY", -1, (unsigned char*)response.c_str() ,response.length());
					jtable = json::parse(response.c_str());
					break;
				}
			}

			for(int i=0 ; i<jtable.size() ; i++)
			{
				text = to_string(jtable[i]["error"]);

				if(text != "null")
				{
					memcpy(&m_tandem_reply.original, &cv_ts_order, sizeof(cv_ts_order));
					memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);
					memcpy(m_tandem_reply.status_code, "1001", 4);
					sprintf(m_tandem_reply.reply_msg, "submit fail, error message:%s", text.c_str());
				}
				else
				{
					memcpy(m_tandem_reply.status_code, "1000", 4);
					memcpy(&m_tandem_reply.original, &cv_ts_order, sizeof(cv_ts_order));
					memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);
					string orderbookNo = to_string(jtable[i]["orderID"]);
					memcpy(m_tandem_reply.bookno, orderbookNo.c_str()+1, 36);
					sprintf(m_tandem_reply.reply_msg, "submit success, orderID(BookNo):%.36s", m_tandem_reply.bookno);
					LogOrderReplyDB_Bitmex(&jtable[i], OPT_DELETE);
				}
			}
			SetStatus(tsMsgReady);
			break;
		default:
			break;

	}

	CCVWriteQueueDAO* pWriteQueueDAO = NULL;
	while(pWriteQueueDAO == NULL)
	{
		if(m_pWriteQueueDAOs)
			pWriteQueueDAO = m_pWriteQueueDAOs->GetAvailableDAO();

		if(pWriteQueueDAO)
		{
			FprintfStderrLog("GET_WRITEQUEUEDAO", -1, (unsigned char*)&m_tandem_reply ,sizeof(m_tandem_reply));
			pWriteQueueDAO->SetReplyMessage((unsigned char*)&m_tandem_reply, sizeof(m_tandem_reply));
			pWriteQueueDAO->TriggerWakeUpEvent();
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

	return true;
}

bool CCVTandemDAO::LogOrderReplyDB_Bitmex(json* jtable, int option)
{
	char insert_str[MAXDATA], delete_str[MAXDATA];

	string response, bitmex_data[30];

	string orderID, clOrdID, account, symbol, side, orderQty, price, ordStatus, transactTime, stopPx, avgPx, leavesQty, currency, settlCurrency, text;

	bitmex_data[0] = to_string((*jtable)["account"]);
	bitmex_data[0] = bitmex_data[0].substr(0, bitmex_data[0].length());

	bitmex_data[1] = to_string((*jtable)["orderID"]);
	bitmex_data[1] = bitmex_data[1].substr(1, bitmex_data[1].length()-2);

	bitmex_data[2] = to_string((*jtable)["symbol"]);
	bitmex_data[2] = bitmex_data[2].substr(1, bitmex_data[2].length()-2);

	bitmex_data[3] = to_string((*jtable)["side"]);
	bitmex_data[3] = bitmex_data[3].substr(1, bitmex_data[3].length()-2);

	bitmex_data[4] = to_string((*jtable)["price"]);
	bitmex_data[4] = bitmex_data[4].substr(0, bitmex_data[4].length());

	bitmex_data[5] = to_string((*jtable)["orderQty"]);
	bitmex_data[5] = bitmex_data[5].substr(0, bitmex_data[5].length());

	bitmex_data[6] = to_string((*jtable)["ordType"]);
	bitmex_data[6] = bitmex_data[6].substr(1, bitmex_data[6].length()-2);

	bitmex_data[7] = to_string((*jtable)["ordStatus"]);
	bitmex_data[7] = bitmex_data[7].substr(1, bitmex_data[7].length()-2);

	bitmex_data[8] = to_string((*jtable)["transactTime"]);
	bitmex_data[8] = bitmex_data[8].substr(1, 19);

	bitmex_data[9] = to_string((*jtable)["stopPx"]);
	bitmex_data[9] = bitmex_data[9].substr(0, bitmex_data[9].length());

	bitmex_data[10] = to_string((*jtable)["avgPx"]);
	bitmex_data[10] = bitmex_data[10].substr(0, bitmex_data[10].length());

	bitmex_data[11] = to_string((*jtable)["cumQty"]);
	bitmex_data[11] = bitmex_data[11].substr(0, bitmex_data[11].length());

	bitmex_data[12] = to_string((*jtable)["leavesQty"]);
	bitmex_data[12] = bitmex_data[12].substr(0, bitmex_data[12].length());

	bitmex_data[13] = to_string((*jtable)["currency"]);
	bitmex_data[13] = bitmex_data[13].substr(1, bitmex_data[13].length()-2);

	bitmex_data[14] = to_string((*jtable)["settlCurrency"]);
	bitmex_data[14] = bitmex_data[14].substr(1, bitmex_data[14].length()-2);

	bitmex_data[15] = to_string((*jtable)["clOrdID"]);
	bitmex_data[15] = bitmex_data[15].substr(1, bitmex_data[15].length()-2);

	bitmex_data[16] = to_string((*jtable)["text"]);
	bitmex_data[16] = bitmex_data[16].substr(1, bitmex_data[16].length()-2);


	//printf("\n\n\n%s\n\n\n", bitmex_data[1].c_str());
	if(option == OPT_ADD) {
		sprintf(insert_str, "http://192.168.101.209:19487/mysql?db=Cryptovix_test&query=insert%%20into%%20bitmex_order_history%%20set%%20exchange=%27BITMEX%27,account=%%27%s%%27,order_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,order_time=%%27%s%%27,match_qty=%%27%s%%27,remaining_qty=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27", bitmex_data[0].c_str(), bitmex_data[1].c_str(), bitmex_data[2].c_str(), bitmex_data[3].c_str(), bitmex_data[5].c_str(), bitmex_data[6].c_str(), bitmex_data[7].c_str(), bitmex_data[8].c_str(), bitmex_data[11].c_str(), bitmex_data[12].c_str(), bitmex_data[13].c_str(), bitmex_data[14].c_str(), bitmex_data[15].c_str(), bitmex_data[16].c_str());
		if(bitmex_data[4] != "null")
			sprintf(insert_str, "%s,order_price=%%27%s%%27", insert_str, bitmex_data[4].c_str());
		if(bitmex_data[9] != "null")
			sprintf(insert_str, "%s,stop_price=%%27%s%%27", insert_str, bitmex_data[9].c_str());
		if(bitmex_data[10] != "null")
			sprintf(insert_str, "%s,match_price=%%27%s%%27", insert_str, bitmex_data[10].c_str());
	}

	if(option == OPT_DELETE) {
//		sprintf(delete_str, "http://192.168.101.209:19487/mysql?db=Cryptovix_test&query=delete%%20from%%20bitmex_order_history%20where%%20order_no%%20=%%20%%27%s%%27", bitmex_data[1].c_str());
		sprintf(insert_str, "http://192.168.101.209:19487/mysql?db=Cryptovix_test&query=update%%20bitmex_order_history%%20set%%20exchange=%27BITMEX%27,account=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,order_time=%%27%s%%27,match_qty=%%27%s%%27,remaining_qty=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27", bitmex_data[0].c_str(), bitmex_data[2].c_str(), bitmex_data[3].c_str(), bitmex_data[5].c_str(), bitmex_data[6].c_str(), bitmex_data[7].c_str(), bitmex_data[8].c_str(), bitmex_data[11].c_str(), bitmex_data[12].c_str(), bitmex_data[13].c_str(), bitmex_data[14].c_str(), bitmex_data[15].c_str(), bitmex_data[16].c_str());
		if(bitmex_data[4] != "null")
			sprintf(insert_str, "%s,order_price=%%27%s%%27", insert_str, bitmex_data[4].c_str());
		if(bitmex_data[9] != "null")
			sprintf(insert_str, "%s,stop_price=%%27%s%%27", insert_str, bitmex_data[9].c_str());
		if(bitmex_data[10] != "null")
			sprintf(insert_str, "%s,match_price=%%27%s%%27", insert_str, bitmex_data[10].c_str());
		sprintf(insert_str, "%s%%20where order_no=%%27%s%%27", insert_str, bitmex_data[1].c_str());
	}

	for(int i=0 ; i<strlen(insert_str) ; i++)
	{
		if(insert_str[i] == ' ')
			insert_str[i] = '+';
	}

	for(int i=0 ; i<strlen(delete_str) ; i++)
	{
		if(delete_str[i] == ' ')
			delete_str[i] = '+';
	}

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, delete_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_perform(curl);
	curl_easy_setopt(curl, CURLOPT_URL, insert_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	//printf("================response\n%s\n===============\n", response.c_str());
	return true;
}
