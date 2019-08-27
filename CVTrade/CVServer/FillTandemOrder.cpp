#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <assert.h>
#include <sstream>
#include <iostream>

#include "../include/CVType.h"
#include "../include/CVGlobal.h"
#include "CVClients.h"

using namespace std;

bool IsNum(string strData, int nLength)
{
	stringstream sin(strData);
	double d;
	char c;

	string strSpace(" ");
	if(strData.length() != nLength)
		return false;
	if(strData.find(strSpace) != -1)
		return false;
	if(!(sin >> d))
		return false;
	if(sin >> c)
		return false;
	else
		return true;
}

long FillTandemBitcoinOrderFormat(string& strService, char* pIP, map<string, string>& mBranchAccount, union CV_ORDER &ucv, union CV_TS_ORDER &ucvts, bool bIsProxy = false)
{
	char caQty[10];
	char caOrderPrice[10];
	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	memcpy(ucvts.cv_ts_order.client_ip, pIP, IPLEN);
	memcpy(ucvts.cv_ts_order.seq_id, ucv.cv_order.seq_id, 13); 
	printf("ucv.cv_order.seq_id = %.13s\n", ucv.cv_order.seq_id);
//key id
	if(ucv.cv_order.trade_type[0] == '0') {
		pClients->GetSerialNumber(ucvts.cv_ts_order.key_id);
		memcpy(ucv.cv_order.key_id, ucvts.cv_ts_order.key_id, 13); 
		memcpy(ucvts.cv_ts_order.trade_type, ucv.cv_order.trade_type, 1);
	}
	else if(ucv.cv_order.trade_type[0] >= '1' && ucv.cv_order.trade_type[0] <= '3') {
		//todo : check key id valid
		memcpy(ucvts.cv_ts_order.key_id, ucv.cv_order.key_id, 13);
		memcpy(ucvts.cv_ts_order.trade_type, ucv.cv_order.trade_type, 1);
		memcpy(ucvts.cv_ts_order.order_bookno, ucv.cv_order.order_bookno, 36);
	}
	else {
		return TT_ERROR;
	}


//Sub account id and strategy name
	memcpy(ucvts.cv_ts_order.sub_acno_id, ucv.cv_order.sub_acno_id, 7);
	if(!strncmp(ucvts.cv_ts_order.sub_acno_id, "A000012", 7))
	{
	        sprintf(ucvts.cv_ts_order.apiKey_order, "f3-gObpGoi5ECeCjFozXMm4K");
	        sprintf(ucvts.cv_ts_order.apiSecret_order, "i9NmdIydRSa300ZGKP_JHwqnZUpP7S3KB4lf-obHeWgOOOUE");
	        sprintf(ucvts.cv_ts_order.apiKey_cancel, "O-E5T-a0KJDs6AMh5loISqu6");
	        sprintf(ucvts.cv_ts_order.apiSecret_cancel, "4RFDBMdJb8425ZzP61aoT_3sEwF6Q9FqhTo26uXIR3RjBMOP");
	}if(!strncmp(ucvts.cv_ts_order.sub_acno_id, "A000013", 7))
	{
	        sprintf(ucvts.cv_ts_order.apiKey_order, "A9oHum-Pjl590hShf8eXH3Hl");
	        sprintf(ucvts.cv_ts_order.apiSecret_order, "FSgJUpbK4RCUKl8OYUrH3mZnf5KPx9arVy1i89tdIoEa3VsL");
	        sprintf(ucvts.cv_ts_order.apiKey_cancel, "A9oHum-Pjl590hShf8eXH3Hl");
	        sprintf(ucvts.cv_ts_order.apiSecret_cancel, "FSgJUpbK4RCUKl8OYUrH3mZnf5KPx9arVy1i89tdIoEa3VsL");
	}
	memcpy(ucvts.cv_ts_order.strategy_name, ucv.cv_order.strategy_name, 7);

//Agent ID
	if(strcmp(ucvts.cv_ts_order.agent_id, "MC") != 0 &&
	   strcmp(ucvts.cv_ts_order.agent_id, "RS") != 0 &&
	   strcmp(ucvts.cv_ts_order.agent_id, "TV") != 0    )
		memcpy(ucvts.cv_ts_order.agent_id, ucv.cv_order.agent_id, 2);
	else
		return AG_ERROR;

//Broker ID
	memcpy(ucvts.cv_ts_order.broker_id, ucv.cv_order.broker_id, 4);
	memcpy(ucvts.cv_ts_order.exchange_id, ucv.cv_order.exchange_id, 4);
	memcpy(ucvts.cv_ts_order.symbol_name, ucv.cv_order.symbol_name, 10);
	memcpy(ucvts.cv_ts_order.symbol_type, ucv.cv_order.symbol_type, 1);
	memcpy(ucvts.cv_ts_order.symbol_mark, ucv.cv_order.symbol_mark, 1);

	switch(ucv.cv_order.order_offset[0])
	{
		case '0':
		case '1':
		case '2':
			memcpy(ucvts.cv_ts_order.order_offset, ucv.cv_order.order_offset, 1);
			break;
                default:
                        return OO_ERROR;
        }

        switch(ucv.cv_order.order_dayoff[0])
        {
                case 'Y':
                case 'N':
                        memcpy(ucvts.cv_ts_order.order_dayoff, ucv.cv_order.order_dayoff, 1);
                        break;
                default:
                        return OD_ERROR;
        }

//Date & Time
	memcpy(ucvts.cv_ts_order.order_date, ucv.cv_order.order_date, 8);
	memcpy(ucvts.cv_ts_order.order_time, ucv.cv_order.order_time, 8);
#if 0
//Order Type
	switch(ucv.cv_order.order_type[0])
	{
		case 'M':
		case 'L':
		case 'P':
			memcpy(ucvts.cv_ts_order.order_type, ucv.cv_order.order_type, 1);
			break;
		default:
			return OT_ERROR;
	}
#endif
//Order Buy/sell
	switch(ucv.cv_order.order_buysell[0])
	{
		case 'B':
		case 'S':
			memcpy(ucvts.cv_ts_order.order_buysell, ucv.cv_order.order_buysell, 1);
			break;
		default:
			return BS_ERROR;
	}
//Order Condition
	switch(ucv.cv_order.order_cond[0])
	{
		case '0'://ROD
		case '1'://IOC
		case '2'://FOK
			memcpy(ucvts.cv_ts_order.order_cond, ucv.cv_order.order_cond, 1);
			break;
		default:
			return OC_ERROR;
	}
//Order Mark
	switch(ucv.cv_order.order_mark[0])
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
			memcpy(ucvts.cv_ts_order.order_mark, ucv.cv_order.order_mark, 1);
			break;
		default:
			return OM_ERROR;
	}

//Price Mark
	switch(ucv.cv_order.price_mark[0])
	{
		case '0':
		case '1':
		case '2':
		case '3':
			memcpy(ucvts.cv_ts_order.price_mark, ucv.cv_order.price_mark, 1);
			break;
		default:
			return PM_ERROR;
	}
//Order Price
	memset(caOrderPrice, 0, sizeof(caOrderPrice));
	memcpy(caOrderPrice, ucv.cv_order.order_price, sizeof(ucv.cv_order.order_price));
	string strOrderPrice(caOrderPrice);

	if(!IsNum(strOrderPrice, sizeof(ucv.cv_order.order_price)))
		return PR_ERROR;

	memcpy(ucvts.cv_ts_order.order_price, ucv.cv_order.order_price, 9);

//Touch Price
	memset(caOrderPrice, 0, sizeof(caOrderPrice));
	memcpy(caOrderPrice, ucv.cv_order.touch_price, sizeof(ucv.cv_order.touch_price));
	string strTouchPrice(caOrderPrice);

	if(!IsNum(strTouchPrice, sizeof(ucv.cv_order.touch_price)))
		return TR_ERROR;
	memcpy(ucvts.cv_ts_order.touch_price, ucv.cv_order.touch_price, 9);

//Qty Mark
	switch(ucv.cv_order.price_mark[0])
	{
		case '0':
		case '1':
		case '2':
		case '3':
			memcpy(ucvts.cv_ts_order.qty_mark, ucv.cv_order.qty_mark, 1);
			break;
		default:
			return QM_ERROR;
	}
//Quantity
	memset(caQty, 0, sizeof(caQty));
	memcpy(caQty, ucv.cv_order.order_qty, sizeof(ucv.cv_order.order_qty));
	string strQty(caQty);

	if(!IsNum(strQty, sizeof(ucv.cv_order.order_qty)))
		return QT_ERROR;
	memcpy(ucvts.cv_ts_order.order_qty, ucv.cv_order.order_qty, 9);

//Order Kind
	switch(ucv.cv_order.order_kind[0])
	{
		case '0':
		case '1':
		case '2':
			memcpy(ucvts.cv_ts_order.order_kind, ucv.cv_order.order_kind, 1);
			break;
		default:
			return OK_ERROR;
	}

	char caSeqno[14];
	memset(caSeqno, 0, sizeof(caSeqno));
	memcpy(caSeqno, ucvts.cv_ts_order.key_id, 13);
	return atol(caSeqno);
}
