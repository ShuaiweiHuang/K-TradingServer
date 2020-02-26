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

#include "CVTandemDAO.h"
#include "CVReadQueueDAOs.h"
#include "CVTandemDAOs.h"
#include "CVTIG/CVTandems.h"


using json = nlohmann::json;
using namespace std;

extern string g_TandemEth0;
extern string g_TandemEth1;

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

CCVTandemDAO::CCVTandemDAO(int nTandemDAOID, int nNumberOfWriteQueueDAO, key_t kWriteQueueDAOStartKey, key_t kWriteQueueDAOEndKey, key_t kQueueDAOMonitorKey)
{
	m_pHeartbeat = NULL;
	m_TandemDAOStatus = tsNone;
	m_bInuse = false;
	m_pWriteQueueDAOs = CCVWriteQueueDAOs::GetInstance();

	if(m_pWriteQueueDAOs == NULL)
		m_pWriteQueueDAOs = new CCVWriteQueueDAOs(nNumberOfWriteQueueDAO, kWriteQueueDAOStartKey, kWriteQueueDAOEndKey, kQueueDAOMonitorKey);

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
	m_request_remain = "60";
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
		sleep(1);
	}
	return NULL;
}
TCVTandemDAOStatus CCVTandemDAO::GetStatus()
{
	return m_TandemDAOStatus;
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

bool CCVTandemDAO::SendOrder(const unsigned char* pBuf, int nSize)
{
	return OrderSubmit(pBuf, nSize);
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
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
		}
		curl_global_cleanup();
}


bool CCVTandemDAO::OrderSubmit(const unsigned char* pBuf, int nToSend)
{
	char exname[11];
	struct CV_StructTSOrder cv_ts_order;
	memcpy(&cv_ts_order, pBuf, nToSend);
	sprintf(exname, "%.10s", cv_ts_order.exchange_id);

	if(!strcmp(exname, "BITMEX_T") || !strcmp(exname, "BITMEX"))
	{
		switch(cv_ts_order.trade_type[0])
		{
			case '0':
			case '1':
			case '2':
#ifndef OCOMODE
				return OrderSubmit_Bitmex(cv_ts_order, nToSend, 0);
#else
				if(cv_ts_order.order_type[0] == ORDERREQ) 
					return OrderSubmit_Bitmex(cv_ts_order, nToSend, 0);
				else if(cv_ts_order.order_type[0] == ORDEROCOREQ)
					return OCOSubmit_Bitmex(cv_ts_order, nToSend);
#endif						
				break;
			case '3':
			case '4':
				return OrderModify_Bitmex(cv_ts_order, nToSend);
				break;
				break;
			default:
				FprintfStderrLog("ERROR_TRADE_TYPE", -1, 0, 0);
				break;
	
		}
	}
#if 0
	if(!strcmp(exname, "BINANCE_FT") || !strcmp(exname, "BINANCE") || !strcmp(exname, "BINANCE_F"))
	{
		return OrderSubmit_Binance(cv_ts_order, nToSend);
	}
#endif
	SetInuse(false);
	return true;
}

#if 0
bool CCVTandemDAO::OrderSubmit_Binance(struct CV_StructTSOrder cv_ts_order, int nToSend)
{
	CURLcode res;
	string buysell_str;
	unsigned char * mac = NULL;
	unsigned int mac_length = 0;
	int recvwin = 10000, ret;
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	double expires = (tv.tv_sec) * 1000 ;
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
		buysell_str = "BUY";
	if(cv_ts_order.order_buysell[0] == 'S')
		buysell_str = "SELL";

	switch(cv_ts_order.price_mark[0])
	{
		case '0':
			doprice = atof(oprice) / SCALE_TPYE_1;
			dtprice = atof(tprice) / SCALE_TPYE_1;
			break;
		case '1':
			doprice = atoi(oprice);
			dtprice = atoi(tprice);
			break;
		case '2':
			doprice = atof(oprice) / SCALE_TPYE_2;
			dtprice = atof(tprice) / SCALE_TPYE_2;
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
			dqty = atof(qty) / SCALE_TPYE_2;
			break;
		case '2':
			dqty = atof(qty) / SCALE_TPYE_1;
			break;
		case '3':
		default:
			break;
	}

	CURL *m_curl = curl_easy_init();
	curl_global_init(CURL_GLOBAL_ALL);
	string order_url, order_all_url;
	if(!strcmp(cv_ts_order.exchange_id, "BINANCE_FT"))
	{
		order_url = "https://testnet.binancefuture.com/fapi/v1/order";
		order_all_url = "https://testnet.binancefuture.com/fapi/v1/allOrders";
	}

	if(!strcmp(cv_ts_order.exchange_id, "BINANCE_F"))
	{
		order_url = "https://fapi.binance.com/fapi/v1/order";
		order_all_url = "https://fapi.binance.com/fapi/v1/allOrders";
	}

	switch(cv_ts_order.trade_type[0])
	{
		case '0'://new order
#ifdef DEBUG
			printf("Binance: receive new order\n");
#endif
			sprintf(apikey_str, "X-MBX-APIKEY: %.64s", cv_ts_order.apiKey_order);
			switch(cv_ts_order.order_mark[0])
			{
				case '0'://Market
					sprintf(commandstr, "newClientOrderId=%.13s&symbol=%s&side=%s&quantity=%.3f&type=MARKET&timestamp=%.0lf&recvWindow=%d",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), dqty, expires, recvwin);
					sprintf(encrystr, "%s", commandstr);
					break;
				case '1'://Limit
					sprintf(commandstr, "newClientOrderId=%.13s&symbol=%s&side=%s&quantity=%.3f&price=%.4lf&type=LIMIT&timeInForce=GTC&timestamp=%.0lf&recvWindow=%d",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), dqty, doprice, expires, recvwin);
					sprintf(encrystr, "%s", commandstr);
					break;
				case '4'://stop limit
					sprintf(commandstr, "newClientOrderId=%.13s&symbol=%s&side=%s&quantity=%f&price=%.4lf&stopPrice=%.4lf&type=STOP&timestamp=%.0lf&recvWindow=%d",
					//&text=%.7s|%.30s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), dqty, doprice, dtprice, expires, recvwin);
					sprintf(encrystr, "%s", commandstr);
					break;
				case '2'://Protect
				default:
					FprintfStderrLog("ERROR_ORDER_MARK", -1, 0, 0);
					break;
			}
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_order, strlen(cv_ts_order.apiSecret_order), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());
			break;
		case '1'://delete specific order
#ifdef DEBUG
			printf("Binance: receive delete order\n");
#endif
			sprintf(apikey_str, "X-MBX-APIKEY: %.64s", cv_ts_order.apiKey_order);
			sprintf(commandstr, "symbol=%s&orderID=%s&timestamp=%.0lf", cv_ts_order.symbol_name, cv_ts_order.order_bookno, expires);
			sprintf(encrystr, "%s", commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_order, strlen(cv_ts_order.apiSecret_order), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());
			break;
		default :
			FprintfStderrLog("ERROR_TRADE_TYPE", -1, 0, 0);
			break;
	}
#ifdef DEBUG
			printf("\n\nencrystr = %s\n\n", encrystr);
			printf("secret key = %s\n", cv_ts_order.apiSecret_order);
#endif


#if 0	//test
	sprintf(encrystr, "Binance %s %.6s at price %1f.", buysell_str.c_str(), cv_ts_order.symbol_name, doprice);
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
		http_header = curl_slist_append(http_header, apikey_str);
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, http_header);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, true);
		sprintf(execution_str, "%s&signature=%s", commandstr, macoutput);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(execution_str));
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, execution_str);
		curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, parseHeader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &headresponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, getResponse);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
		
		res = curl_easy_perform(m_curl);
#ifdef DEBUG
		printf("apikey_str = %s\n", apikey_str);
		printf("execution_str = %s\n", execution_str);
		printf("\n===================\n%s\n===================\n", response.c_str());
#endif
		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		curl_slist_free_all(http_header);
		curl_easy_cleanup(m_curl);
	}
	curl_global_cleanup();

	//printf("request remain: %s\n", headresponse.remain.c_str());
	m_request_remain = headresponse.remain;
	m_time_limit = headresponse.epoch;
	string text;
	bool jarray;
	switch(cv_ts_order.trade_type[0])
	{
		case '0'://new
		case '1'://old
		case '2':
			time_t tt_time;
			struct tm *tm_time;
			for(int i=0 ; i<response.length() ; i++)
			{
				if(response[i] == '{') {
					response = response.substr(i, response.length()-i);
					FprintfStderrLog("GET_ORDER_REPLY", -1, (unsigned char*)response.c_str() ,response.length());
					jtable = json::parse(response.c_str());
					break;
				}
			}

			memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);


			if(jtable["orderId"].dump() == "null")
			{
				memcpy(m_tandem_reply.status_code, "1001", 4);

				text = jtable["msg"].dump();

				if(text == "null")
					text = jtable["rejectReason"].dump();

				if(text != "null")
					sprintf(m_tandem_reply.reply_msg, "submit fail, error message - [%s]", text.c_str());
				else
					sprintf(m_tandem_reply.reply_msg, "submit fail, error message - [get order ID fail]");
#ifdef DEBUG
				printf("\n\n\ntext = %s\n", text.c_str());
#endif
			}
			else
			{
				memcpy(m_tandem_reply.status_code, "1000", 4);
				memcpy(m_tandem_reply.bookno,	jtable["orderId"].dump().c_str(), jtable["orderId"].dump().length());
				memcpy(m_tandem_reply.price,	jtable["price"].dump().c_str(), jtable["price"].dump().length());
				memcpy(m_tandem_reply.avgPx,	jtable["avgPx"].dump().c_str(), jtable["avgPx"].dump().length());
				memcpy(m_tandem_reply.orderQty,	jtable["origQty"].dump().c_str()+1, jtable["origQty"].dump().length()-2);
				memcpy(m_tandem_reply.cumQty, jtable["cumQty"].dump().c_str(), jtable["cumQty"].dump().length());
				memcpy(m_tandem_reply.transactTime, jtable["updateTime"].dump().c_str(), jtable["updateTime"].dump().length());
				tt_time = atol(m_tandem_reply.transactTime)/1000;
				tm_time  = localtime(&tt_time);
				strftime(m_tandem_reply.transactTime, sizeof(m_tandem_reply.transactTime), "%Y-%m-%d %H:%M:%S", tm_time);
				if(cv_ts_order.trade_type[0] == '0')
					sprintf(m_tandem_reply.reply_msg, "submit order success");
				else
					sprintf(m_tandem_reply.reply_msg, "order cancel success");
#ifdef DEBUG
				printf("==============================\nsubmit order success\n");
				printf("orderID = %s\n=======================\n", m_tandem_reply.bookno);
#endif
				LogOrderReplyDB_Binance(&jtable, &cv_ts_order, (cv_ts_order.trade_type[0] == '0') ? OPT_ADD : OPT_DELETE);
			}
			SetStatus(tsMsgReady);
			break;

		default:
			FprintfStderrLog("ERROR_TRADE_TYPE", -1, 0, 0);
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
#endif

bool CCVTandemDAO::OrderModify_Bitmex(struct CV_StructTSOrder cv_ts_order, int nToSend)
{
	char chOpt = cv_ts_order.trade_type[0];

	cv_ts_order.trade_type[0] = '1';//modify to delete

	if( OrderSubmit_Bitmex(cv_ts_order, nToSend, MODE_SILENT) ) { //delete order
		cv_ts_order.trade_type[0] = chOpt;
		OrderSubmit_Bitmex(cv_ts_order, nToSend, MODE_NORMAL); //new order
		return true;
	}
	return false;
}
#ifdef OCOMODE
bool CCVTandemDAO::OCOSubmit_Bitmex(struct CV_StructTSOrder cv_ts_order, int nToSend)
{
	OrderSubmit_Bitmex(cv_ts_order, nToSend, MODE_OCO_SILENT);
	memcpy(cv_ts_order.key_id, cv_ts_order.key_id_oco, 13);
	sleep(1);
	OrderSubmit_Bitmex(cv_ts_order, nToSend, MODE_OCO);
}
#endif

bool CCVTandemDAO::OrderSubmit_Bitmex(struct CV_StructTSOrder cv_ts_order, int nToSend, int nMode)
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

#ifdef OCOMODE
	if(nMode == MODE_OCO)
	{
#if 0
		printf("cv_ts_order.order_buysell:%.1s\n", cv_ts_order.order_buysell);
		printf("cv_ts_order.order_bookno:%.36s\n", cv_ts_order.order_bookno);
		printf("cv_ts_order.order_cond:%.1s\n", cv_ts_order.order_cond);
		printf("cv_ts_order.order_mark:%.1s\n", cv_ts_order.order_mark);
		printf("cv_ts_order.trade_type:%.1s\n", cv_ts_order.trade_type);
		printf("cv_ts_order.price_mark:%.1s\n", cv_ts_order.price_mark);
		printf("cv_ts_order.order_price:%.9s\n", cv_ts_order.order_price);
		printf("cv_ts_order.touch_price:%.9s\n", cv_ts_order.touch_price);
		printf("cv_ts_order.qty_mark:%.1s\n", cv_ts_order.qty_mark);
		printf("cv_ts_order.order_qty:%.9s\n", cv_ts_order.order_qty);
		printf("cv_ts_order.key_id:%.13s\n", cv_ts_order.key_id);
#endif
		memcpy(cv_ts_order.order_buysell, cv_ts_order.order_buysell_oco, 1);
		memcpy(cv_ts_order.order_bookno, cv_ts_order.order_bookno_oco, 36);
		memcpy(cv_ts_order.order_cond, cv_ts_order.order_cond_oco, 1);
		memcpy(cv_ts_order.order_mark, cv_ts_order.order_mark_oco, 1);
		memcpy(cv_ts_order.price_mark, cv_ts_order.price_mark_oco, 1);
		memcpy(cv_ts_order.order_price, cv_ts_order.order_price_oco, 9);
		memcpy(cv_ts_order.touch_price, cv_ts_order.touch_price_oco, 9);
		memcpy(cv_ts_order.qty_mark, cv_ts_order.qty_mark_oco, 1);
		memcpy(cv_ts_order.order_qty, cv_ts_order.order_qty_oco, 9);
		memcpy(cv_ts_order.key_id, cv_ts_order.key_id_oco, 13);
#if 0
		printf("cv_ts_order.order_buysell_oco:%.1s\n", cv_ts_order.order_buysell_oco);
		printf("cv_ts_order.order_bookno_oco:%.36s\n", cv_ts_order.order_bookno_oco);
		printf("cv_ts_order.order_cond_oco:%.1s\n", cv_ts_order.order_cond_oco);
		printf("cv_ts_order.order_mark_oco:%.1s\n", cv_ts_order.order_mark_oco);
		printf("cv_ts_order.trade_type:%.1s\n", cv_ts_order.trade_type);
		printf("cv_ts_order.price_mark_oco:%.1s\n", cv_ts_order.price_mark_oco);
		printf("cv_ts_order.order_price_oco:%.9s\n", cv_ts_order.order_price_oco);
		printf("cv_ts_order.touch_price_oco:%.9s\n", cv_ts_order.touch_price_oco);
		printf("cv_ts_order.qty_mark_oco:%.1s\n", cv_ts_order.qty_mark_oco);
		printf("cv_ts_order.order_qty_oco:%.9s\n", cv_ts_order.order_qty_oco);
		printf("cv_ts_order.key_id_oco:%.13s\n", cv_ts_order.key_id_oco);
#endif
	}
#endif
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
			doprice = atof(oprice) / SCALE_TPYE_1;
			dtprice = atof(tprice) / SCALE_TPYE_1;
			break;
		case '1':
			doprice = atoi(oprice);
			dtprice = atoi(tprice);
			break;
		case '2':
			doprice = atof(oprice) / SCALE_TPYE_2;
			dtprice = atof(tprice) / SCALE_TPYE_2;
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
			dqty = atof(qty) / SCALE_TPYE_2;
			break;
		case '2':
			dqty = atof(qty) / SCALE_TPYE_1;
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
		case '3':
		case '4':
#ifdef DEBUG
			printf("Receive new order\n");
#endif
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_order);
			switch(cv_ts_order.order_mark[0])
			{
				case '0'://Market
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&ordType=Market&text=%.7s|%.30s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), atoi(qty),cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, cv_ts_order.username);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '1'://Limit
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&price=%.9f&ordType=Limit&timeInForce=GoodTillCancel&text=%.7s|%.30s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), atoi(qty), doprice,
					cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, cv_ts_order.username);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '3'://stop market
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&stopPx=%.9f&execInst=LastPrice&ordType=Stop&text=%.7s|%.30s|%.20s",
					cv_ts_order.key_id, cv_ts_order.symbol_name, buysell_str.c_str(), atoi(qty), dtprice,
					cv_ts_order.sub_acno_id, cv_ts_order.strategy_name, cv_ts_order.username);
					sprintf(encrystr, "POST/api/v1/order%d%s", expires, commandstr);
					break;
				case '4'://stop limit
					sprintf(commandstr, "clOrdID=%.13s&symbol=%s&side=%s&orderQty=%d&price=%.9f&stopPx=%.9f&execInst=LastPrice&ordType=StopLimit&text=%.7s|%.30s|%.20s",
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
#ifdef DEBUG
			printf("Receive delete order\n");
#endif
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_order);
			sprintf(commandstr, "orderID=%.36s", cv_ts_order.order_bookno);
			sprintf(encrystr, "DELETE/api/v1/order%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_order, strlen(cv_ts_order.apiSecret_order), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(m_curl, CURLOPT_URL, order_url.c_str());
			break;
		case '2'://delete all order
#ifdef DEBUG
			printf("Receive delete order\n");
#endif
			sprintf(apikey_str, "api-key: %s", cv_ts_order.apiKey_order);
			sprintf(commandstr, "symbol=%.6s", cv_ts_order.symbol_name); 
			sprintf(encrystr, "DELETE/api/v1/order/all%d%s", expires, commandstr);
			ret = HmacEncodeSHA256(cv_ts_order.apiSecret_order, strlen(cv_ts_order.apiSecret_order), encrystr, strlen(encrystr), mac, mac_length);
			curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(m_curl, CURLOPT_URL, order_all_url.c_str());
			break;
		default :
			FprintfStderrLog("ERROR_TRADE_TYPE", -1, 0, 0);
			break;
	}


#if 0	//test
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
#if 1
		if(cv_ts_order.sub_acno_id[6] %= INTERFACE_NUM) {
			printf("Account %s submit with Eth:%s\n", cv_ts_order.sub_acno_id, g_TandemEth0.c_str());
			curl_easy_setopt(m_curl, CURLOPT_INTERFACE, g_TandemEth0.c_str());
		}
		else {
			curl_easy_setopt(m_curl, CURLOPT_INTERFACE, g_TandemEth1.c_str());
			printf("Account %s submit with Eth:%s\n", cv_ts_order.sub_acno_id, g_TandemEth1.c_str());
		}
#endif
		if(atoi(m_request_remain.c_str()) < 20 && atoi(m_request_remain.c_str()) > 10) {
			printf("sleep 1 second for delay submit (%s)\n", m_request_remain.c_str());
			sleep(1);
		}
		if(atoi(m_request_remain.c_str()) <= 10) {
			printf("sleep 2 second for delay submit (%s)\n", m_request_remain.c_str());
			sleep(2);
		}

		res = curl_easy_perform(m_curl);
//		printf("\n\n\nexecution_str = %100s\n\n\n", execution_str);
#if 1
		printf("apikey_str = %s\n", apikey_str);
		printf("execution_str = %s\n", execution_str);
		printf("\n===================\n%s\n===================\n", response.c_str());
#endif
#if 1
		if(res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			memcpy(m_tandem_reply.status_code, "1002", 4);
			sprintf(m_tandem_reply.reply_msg, "submit order fail - [%s]", curl_easy_strerror(res));
			memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);
			SetStatus(tsMsgReady);
		}
#endif
		curl_slist_free_all(http_header);
		curl_easy_cleanup(m_curl);
	}
	curl_global_cleanup();

	m_request_remain = headresponse.remain;
	m_time_limit = headresponse.epoch;

	printf("request remain: %s\n", m_request_remain.c_str());
	printf("time limit: %s\n", m_time_limit.c_str());

	string text;
	bool jarray;

	if(res == CURLE_OK)
	{

		switch(cv_ts_order.trade_type[0])
		{
			case '0'://new
			case '3':
			case '4':
			{
				int i;
				for(i=0 ; i<response.length() ; i++)
				{
					if(response[i] == '{') {
						response = response.substr(i, response.length()-i);
						FprintfStderrLog("GET_ORDER_REPLY", -1, (unsigned char*)response.c_str() ,response.length());
						jtable = json::parse(response.c_str());
						break;
					}
				}
				memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);

				if(i == response.length())
				{
					memcpy(m_tandem_reply.status_code, "1003", 4);
					sprintf(m_tandem_reply.reply_msg, "submit order fail - [%s]", response.c_str());
				}
				else
				{
					text = jtable["error"]["message"].dump();


					if(text != "null")
					{
						memcpy(m_tandem_reply.status_code, "1001", 4);
						sprintf(m_tandem_reply.reply_msg, "submit order fail - [%s]", text.c_str());
					}
					else
					{
						memcpy(m_tandem_reply.status_code, "1000", 4);
						if(nMode != MODE_OCO)
							memcpy(m_tandem_reply.bookno, jtable["orderID"].dump().c_str()+1, 36);
						else
							memcpy(m_tandem_reply.bookno_oco, jtable["orderID"].dump().c_str()+1, 36);

						memcpy(m_tandem_reply.price, jtable["price"].dump().c_str(), jtable["price"].dump().length());
						memcpy(m_tandem_reply.avgPx, jtable["avgPx"].dump().c_str(), jtable["avgPx"].dump().length());
						memcpy(m_tandem_reply.orderQty, jtable["orderQty"].dump().c_str(), jtable["orderQty"].dump().length());
						memcpy(m_tandem_reply.lastQty, jtable["lastQty"].dump().c_str(), jtable["lastQty"].dump().length());
						memcpy(m_tandem_reply.cumQty, jtable["cumQty"].dump().c_str(), jtable["cumQty"].dump().length());
						memcpy(m_tandem_reply.transactTime, jtable["transactTime"].dump().c_str()+1, jtable["transactTime"].dump().length()-2);
						if(cv_ts_order.trade_type[0] == '0')
							sprintf(m_tandem_reply.reply_msg, "submit order success - [%s]", jtable["text"].dump().c_str());
						if(cv_ts_order.trade_type[0] == '3')
							sprintf(m_tandem_reply.reply_msg, "change qty success - [%s]", jtable["text"].dump().c_str());
						if(cv_ts_order.trade_type[0] == '4')
							sprintf(m_tandem_reply.reply_msg, "change price success - [%s]", jtable["text"].dump().c_str());
		#ifdef DEBUG
						printf("\n\n\ntext = %s\n", text.c_str());
						printf("==============================\nsubmit order success\n");
						printf("orderID = %s\n=======================\n", m_tandem_reply.bookno);
		#endif
						LogOrderReplyDB_Bitmex(&jtable, &cv_ts_order, OPT_ADD);
					}
				}
				SetStatus(tsMsgReady);
				break;
			}
			case '1'://delete or change
			case '2':
			{
				jarray = false;
				int i;
				for(i=0 ; i<response.length() ; i++)
				{
					if(response[i] == '[' || response[i] == '{') {

						if(response[i] == '[')
							jarray = true;

						response = response.substr(i, response.length()-i);
						FprintfStderrLog("GET_ORDER_REPLY", -1, (unsigned char*)response.c_str() ,response.length());
						jtable = json::parse(response.c_str());
						break;
					}
				}

				memcpy(m_tandem_reply.key_id, cv_ts_order.key_id, 13);

				if(i == response.length())
				{
					memcpy(m_tandem_reply.status_code, "1003", 4);
					sprintf(m_tandem_reply.reply_msg, "delete order fail - [%s]", response.c_str());
				}
				else
				{
					for(int i=0 ; i<jtable.size() ; i++)
					{
						if(jarray)
							text = jtable[i]["error"].dump();
						else
							text = jtable["error"]["message"].dump();


						if(jarray)
							memcpy(m_tandem_reply.bookno, jtable[i]["orderID"].dump().c_str()+1, 36);

						if(text != "null")
						{
							memcpy(m_tandem_reply.status_code, "1001", 4);
							sprintf(m_tandem_reply.reply_msg, "delete order fail - [%s]", text.c_str());
#ifdef DEBUG
							printf("\n\n\ntext = %s\n", text.c_str());
#endif
						}
						else
						{
							memcpy(m_tandem_reply.status_code, "1000", 4);
							sprintf(m_tandem_reply.reply_msg, "delete order success - [%s]", jtable[i]["text"].dump().c_str());
							if(jarray)
								LogOrderReplyDB_Bitmex(&jtable[i], &cv_ts_order, OPT_DELETE);
						}

						if(!jarray)
							break;
#if 0
						memcpy(m_tandem_reply.price,        jtable[i]["price"]).c_str(),          [i]jtable["price"]).length());
						memcpy(m_tandem_reply.avgPx,        jtable[i]["avgPx"]).c_str(),          [i]jtable["avgPx"]).length());
						memcpy(m_tandem_reply.orderQty,     jtable[i]["orderQty"]).c_str(),       [i]jtable["orderQty"]).length());
						memcpy(m_tandem_reply.leaveQty,     jtable[i]["leaveQty"]).c_str(),       [i]jtable["leaveQty"]).length());
						memcpy(m_tandem_reply.cumQty,       jtable[i]["cumQty"]).c_str(),         [i]jtable["cumQty"]).length());
						memcpy(m_tandem_reply.transactTime, jtable[i]["transactTime"]).c_str()+1, jtable[i]["transactTime"]).length()-2);
#endif
					}
				}
				SetStatus(tsMsgReady);
				break;
			}
			default:
				break;

		}//switch
	}//if(res != CURLE_OK)

	if( nMode != MODE_SILENT && nMode != MODE_OCO_SILENT) {

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
				usleep(1000);
			}
		}
	}
	return true;
}

bool CCVTandemDAO::LogOrderReplyDB_Bitmex(json* jtable, struct CV_StructTSOrder* cv_ts_order, int option)
{
	char insert_str[MAXDATA], delete_str[MAXDATA];

	string response, exchange_data[30];

	exchange_data[0] = (*jtable)["account"].dump();
	exchange_data[0] = exchange_data[0].substr(0, exchange_data[0].length());

	exchange_data[1] = (*jtable)["orderID"].dump();
	exchange_data[1] = exchange_data[1].substr(1, exchange_data[1].length()-2);

	exchange_data[2] = (*jtable)["symbol"].dump();
	exchange_data[2] = exchange_data[2].substr(1, exchange_data[2].length()-2);

	exchange_data[3] = (*jtable)["side"].dump();
	exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length()-2);

	exchange_data[4] = (*jtable)["price"].dump();
	exchange_data[4] = exchange_data[4].substr(0, exchange_data[4].length());

	exchange_data[5] = (*jtable)["orderQty"].dump();
	exchange_data[5] = exchange_data[5].substr(0, exchange_data[5].length());

	exchange_data[6] = (*jtable)["ordType"].dump();
	exchange_data[6] = exchange_data[6].substr(1, exchange_data[6].length()-2);

	exchange_data[7] = (*jtable)["ordStatus"].dump();
	exchange_data[7] = exchange_data[7].substr(1, exchange_data[7].length()-2);

	exchange_data[8] = (*jtable)["transactTime"].dump();
	exchange_data[8] = exchange_data[8].substr(1, 19);

	exchange_data[9] = (*jtable)["stopPx"].dump();
	exchange_data[9] = exchange_data[9].substr(0, exchange_data[9].length());

	exchange_data[10] = (*jtable)["avgPx"].dump();
	exchange_data[10] = exchange_data[10].substr(0, exchange_data[10].length());

	exchange_data[11] = (*jtable)["cumQty"].dump();
	exchange_data[11] = exchange_data[11].substr(0, exchange_data[11].length());

	exchange_data[12] = (*jtable)["leavesQty"].dump();
	exchange_data[12] = exchange_data[12].substr(0, exchange_data[12].length());

	exchange_data[13] = (*jtable)["currency"].dump();
	exchange_data[13] = exchange_data[13].substr(1, exchange_data[13].length()-2);

	exchange_data[14] = (*jtable)["settlCurrency"].dump();
	exchange_data[14] = exchange_data[14].substr(1, exchange_data[14].length()-2);

	exchange_data[15] = (*jtable)["clOrdID"].dump();
	exchange_data[15] = exchange_data[15].substr(1, exchange_data[15].length()-2);

	exchange_data[16] = (*jtable)["text"].dump();
	exchange_data[16] = exchange_data[16].substr(1, exchange_data[16].length()-2);

	if(option == OPT_ADD) {
#ifdef AWSCODE
		sprintf(insert_str, "https://127.0.0.1:2012/mysql/?query=insert%%20into%%20bitmex_order_history%%20set%%20exchange=%27BITMEX%27,account=%%27%s%%27,order_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,order_time=%%27%s%%27,match_qty=%%27%s%%27,remaining_qty=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27,update_user=USER(),source_ip=%%27%.15s%%27,agent_id=%%27%.2s%%27,seq_id=%%27%.13s%%27", exchange_data[0].c_str(), exchange_data[1].c_str(), exchange_data[2].c_str(), exchange_data[3].c_str(), exchange_data[5].c_str(), exchange_data[6].c_str(), exchange_data[7].c_str(), exchange_data[8].c_str(), exchange_data[11].c_str(), exchange_data[12].c_str(), exchange_data[13].c_str(), exchange_data[14].c_str(), exchange_data[15].c_str(), exchange_data[16].c_str(), cv_ts_order->client_ip, cv_ts_order->agent_id, cv_ts_order->seq_id);
#else
		sprintf(insert_str, "http://tm1.cryptovix.com.tw:2011/mysql?db=cryptovix&query=insert%%20into%%20bitmex_order_history%%20set%%20exchange=%27BITMEX%27,account=%%27%s%%27,order_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,order_time=%%27%s%%27,match_qty=%%27%s%%27,remaining_qty=%%27%s%%27,quote_currency=%%27%s%%27,settlement_currency=%%27%s%%27,serial_no=%%27%s%%27,remark=%%27%s%%27,update_user=USER()", exchange_data[0].c_str(), exchange_data[1].c_str(), exchange_data[2].c_str(), exchange_data[3].c_str(), exchange_data[5].c_str(), exchange_data[6].c_str(), exchange_data[7].c_str(), exchange_data[8].c_str(), exchange_data[11].c_str(), exchange_data[12].c_str(), exchange_data[13].c_str(), exchange_data[14].c_str(), exchange_data[15].c_str(), exchange_data[16].c_str());
#endif
		if(exchange_data[4] != "null")
			sprintf(insert_str, "%s,order_price=%%27%s%%27", insert_str, exchange_data[4].c_str());
		if(exchange_data[9] != "null")
			sprintf(insert_str, "%s,stop_price=%%27%s%%27", insert_str, exchange_data[9].c_str());
		if(exchange_data[10] != "null")
			sprintf(insert_str, "%s,match_price=%%27%s%%27", insert_str, exchange_data[10].c_str());
	}
	if(option == OPT_DELETE) {
#ifdef AWSCODE
		sprintf(insert_str,"https://127.0.0.1:2012/mysql/?query=update%%20bitmex_order_history%%20set%%20order_status=%%27%s%%27,match_qty=%%27%s%%27,remaining_qty=%%27%s%%27,remark=%%27%s%%27", exchange_data[7].c_str(), exchange_data[11].c_str(), exchange_data[12].c_str(), exchange_data[16].c_str());
#else
		sprintf(insert_str,"http://tm1.cryptovix.com.tw:2011/mysql?db=cryptovix&query=update%%20bitmex_order_history%%20set%%20order_status=%%27%s%%27,match_qty=%%27%s%%27,remaining_qty=%%27%s%%27,remark=%%27%s%%27", exchange_data[7].c_str(), exchange_data[11].c_str(), exchange_data[12].c_str(), exchange_data[16].c_str());
#endif
		sprintf(insert_str, "%s%%20where%%20order_no=%%27%s%%27", insert_str, exchange_data[1].c_str());
	}

	for(int i=0 ; i<strlen(insert_str) ; i++)
	{
		if(insert_str[i] == ' ')
			insert_str[i] = '+';
	}
	printf("%s\n", insert_str);
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, insert_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	printf("================response\n%s\n===============\n", response.c_str());
	return true;
}

bool CCVTandemDAO::LogOrderReplyDB_Binance(json* jtable, struct CV_StructTSOrder* cv_ts_order, int option)
{
	char insert_str[MAXDATA], delete_str[MAXDATA];

	string response, exchange_data[30];

	exchange_data[0] = (*jtable)["orderId"].dump();
	exchange_data[0] = exchange_data[0].substr(0, exchange_data[0].length());

	exchange_data[1] = (*jtable)["symbol"].dump();
	exchange_data[1] = exchange_data[1].substr(1, exchange_data[1].length()-2);

	exchange_data[2] = (*jtable)["accountId"].dump();
	exchange_data[2] = exchange_data[2].substr(0, exchange_data[2].length());

	exchange_data[3] = (*jtable)["status"].dump();
	exchange_data[3] = exchange_data[3].substr(1, exchange_data[3].length()-2);

	exchange_data[4] = (*jtable)["clientOrderId"].dump();
	exchange_data[4] = exchange_data[4].substr(1, exchange_data[4].length()-2);

	exchange_data[5] = (*jtable)["price"].dump();
	exchange_data[5] = exchange_data[5].substr(1, exchange_data[5].length()-2);

	exchange_data[6] = (*jtable)["origQty"].dump();
	exchange_data[6] = exchange_data[6].substr(1, exchange_data[6].length()-2);

	exchange_data[7] = (*jtable)["executedQty"].dump();
	exchange_data[7] = exchange_data[7].substr(1, exchange_data[7].length()-2);

	exchange_data[8] = (*jtable)["cumQty"].dump();
	exchange_data[8] = exchange_data[8].substr(1, exchange_data[8].length()-2);

	exchange_data[9] = (*jtable)["cumQuote"].dump();
	exchange_data[9] = exchange_data[9].substr(1, exchange_data[9].length()-2);

	exchange_data[10] = (*jtable)["timeInForce"].dump();
	exchange_data[10] = exchange_data[10].substr(1, exchange_data[10].length()-2);

	exchange_data[11] = (*jtable)["type"].dump();
	exchange_data[11] = exchange_data[11].substr(1, exchange_data[11].length()-2);

	exchange_data[12] = (*jtable)["reduceOnly"].dump();
	exchange_data[12] = exchange_data[12].substr(0, exchange_data[12].length());

	exchange_data[13] = (*jtable)["side"].dump();
	exchange_data[13] = exchange_data[13].substr(1, exchange_data[13].length()-2);

	exchange_data[14] = (*jtable)["stopPrice"].dump();
	exchange_data[14] = exchange_data[14].substr(1, exchange_data[14].length()-2);

	exchange_data[15] = (*jtable)["updateTime"].dump();
	exchange_data[15] = exchange_data[15].substr(0, exchange_data[15].length());

#if 1//log local time
	time_t tt_time = atol(exchange_data[15].c_str())/1000;
	char time_str[30];
	struct tm *tm_time  = localtime(&tt_time);
	strftime(time_str, 30, "%Y-%m-%d %H:%M:%S", tm_time);
#endif

#ifdef AWSCODE
		sprintf(insert_str, "https://127.0.0.1:2012/mysql/?query=insert%%20into%%20binance_order_history%%20set%%20exchange=%27BINANCE%27,account=%%27%s%%27,order_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,order_time=%%27%.19s%%27,match_qty=%%27%s%%27,serial_no=%%27%s%%27,stop_price=%%27%s%%27,order_price=%%27%s%%27,accounting_no=%%27%.7s%%27,strategy=%%27%.30s%%27,trader=%%27%.20s%%27,update_user=USER()",
		exchange_data[2].c_str(), exchange_data[0].c_str(), exchange_data[1].c_str(), exchange_data[13].c_str(),
		exchange_data[6].c_str(), exchange_data[11].c_str(), exchange_data[3].c_str(), time_str,
		exchange_data[7].c_str(), exchange_data[4].c_str(), exchange_data[14].c_str(), exchange_data[5].c_str(),
		cv_ts_order->sub_acno_id, cv_ts_order->strategy_name, cv_ts_order->username);
#else
		sprintf(insert_str, "http://tm1.cryptovix.com.tw:2011/mysql?db=cryptovix&query=insert%%20into%%20binance_order_history%%20set%%20exchange=%27BINANCE%27,account=%%27%s%%27,order_no=%%27%s%%27,symbol=%%27%s%%27,side=%%27%s%%27,order_qty=%%27%s%%27,order_type=%%27%s%%27,order_status=%%27%s%%27,order_time=%%27%.19s%%27,match_qty=%%27%s%%27,serial_no=%%27%s%%27,stop_price=%%27%s%%27,order_price=%%27%s%%27,accounting_no=%%27%.7s%%27,strategy=%%27%.30s%%27,trader=%%27%.20s%%27,update_user=USER()",
		exchange_data[2].c_str(), exchange_data[0].c_str(), exchange_data[1].c_str(), exchange_data[13].c_str(),
		exchange_data[6].c_str(), exchange_data[11].c_str(), exchange_data[3].c_str(), time_str,
		exchange_data[7].c_str(), exchange_data[4].c_str(), exchange_data[14].c_str(), exchange_data[5].c_str(),
		cv_ts_order->sub_acno_id, cv_ts_order->strategy_name, cv_ts_order->username);

#endif

	if(option == OPT_DELETE) {
#ifdef AWSCODE
		sprintf(insert_str, "https://127.0.0.1:2012/mysql/?query=update%%20binance_order_history%%20set%%20order_status=%%27%s%%27,match_qty=%%27%s%%27", exchange_data[3].c_str(), exchange_data[7].c_str());
#else
		sprintf(insert_str, "http://tm1.cryptovix.com.tw:2011/mysql?db=cryptovix&query=update%%20binance_order_history%%20set%%20order_status=%%27%s%%27,match_qty=%%27%s%%27", exchange_data[3].c_str(), exchange_data[7].c_str());
#endif
		sprintf(insert_str, "%s%%20where%%20order_no=%%27%s%%27", insert_str, exchange_data[0].c_str());
	}

	for(int i=0 ; i<strlen(insert_str) ; i++)
	{
		if(insert_str[i] == ' ')
			insert_str[i] = '+';
	}

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, insert_str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	printf("================insert_str\n%s\n==============", insert_str);
#ifdef DEBUG
	printf("================insert_str\n%s\n==============", insert_str);
	printf("================response\n%s\n===============\n", response.c_str());
#endif
	return true;
}
