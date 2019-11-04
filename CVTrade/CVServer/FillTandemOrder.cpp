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

long FillTandemBitcoinOrderFormat(string& strService, char* pUsername, char* pIP, map<string, struct AccountData>& mBranchAccount, union CV_ORDER &ucv, union CV_TS_ORDER &ucvts)
{
	char caQty[10];
	char caOrderPrice[10];
	CCVClients* pClients = CCVClients::GetInstance();
	assert(pClients);

	memcpy(ucvts.cv_ts_order.client_ip, pIP, IPLEN);
	memcpy(ucvts.cv_ts_order.seq_id, ucv.cv_order.seq_id, 13); 
	memcpy(ucvts.cv_ts_order.username, pUsername, 20); 
	memcpy(ucvts.cv_ts_order.exchange_id, ucv.cv_order.exchange_id, 10);


//key id
	if(ucv.cv_order.trade_type[0] == '0')
	{
		pClients->GetSerialNumber(ucvts.cv_ts_order.key_id);
		memcpy(ucv.cv_order.key_id, ucvts.cv_ts_order.key_id, 13); 
	}
	else if(ucv.cv_order.trade_type[0] >= '1' && ucv.cv_order.trade_type[0] <= '3')
	{
		//todo : check key id valid
		pClients->GetSerialNumber(ucvts.cv_ts_order.key_id);
		//memcpy(ucvts.cv_ts_order.key_id, ucv.cv_order.key_id, 13);
		memcpy(ucvts.cv_ts_order.order_bookno, ucv.cv_order.order_bookno, 36);
		if(ucv.cv_order.trade_type[0] >= '2') { // delete all
			if(!strcmp(ucvts.cv_ts_order.exchange_id, "BINANCE_F")||!strcmp(ucvts.cv_ts_order.exchange_id, "BINANCE_FT"))
				return TT_ERROR;
		}
		if(ucv.cv_order.trade_type[0] >= '3') {
			if(!strcmp(ucvts.cv_ts_order.exchange_id, "BITMEX"))
				return TT_ERROR;
		}

	}
	else {
		return TT_ERROR;
	}

	memcpy(ucvts.cv_ts_order.trade_type, ucv.cv_order.trade_type, 1);

#ifdef DEBUG	
	printf("ucv.cv_order.seq_id = %.13s\n", ucv.cv_order.seq_id);
	printf("ucv.cv_order.key_id = %.13s\n", ucv.cv_order.key_id);
#endif

//Sub account id and strategy name
	int i = 0, len ;

	memcpy(ucvts.cv_ts_order.sub_acno_id, ucv.cv_order.sub_acno_id, 7);
	bool account_check = false;
	for(map<string, struct AccountData>::iterator iter = mBranchAccount.begin(); iter != mBranchAccount.end() ; iter++)
	{
		if(!memcmp(iter->first.c_str(), ucvts.cv_ts_order.sub_acno_id, iter->first.length()))
		{
			account_check = true;
			memset(ucvts.cv_ts_order.apiKey_order, 0, sizeof(ucvts.cv_ts_order.apiKey_order));
			memset(ucvts.cv_ts_order.apiSecret_order, 0, sizeof(ucvts.cv_ts_order.apiSecret_order));
			memcpy(ucvts.cv_ts_order.apiKey_order, iter->second.api_id.c_str(), iter->second.api_id.length());
			printf("api id (%d): %s\n", iter->second.api_id.length(), iter->second.api_id.c_str());
			memcpy(ucvts.cv_ts_order.apiSecret_order, iter->second.api_key.c_str(), iter->second.api_key.length());
			printf("api key (%d): %s\n", iter->second.api_key.length(), iter->second.api_key.c_str());
			break;
		}
	}

	if(!account_check)
		return AC_ERROR;

	memcpy(ucvts.cv_ts_order.strategy_name, ucv.cv_order.strategy_name, 30);

//Agent ID
	if(strcmp(ucvts.cv_ts_order.agent_id, "MC") != 0 &&
	   strcmp(ucvts.cv_ts_order.agent_id, "RS") != 0 &&
	   strcmp(ucvts.cv_ts_order.agent_id, "TV") != 0    )
		memcpy(ucvts.cv_ts_order.agent_id, ucv.cv_order.agent_id, 2);
	else
		return AG_ERROR;

//Broker ID
	memcpy(ucvts.cv_ts_order.broker_id, ucv.cv_order.broker_id, 4);
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
		case '4':
			memcpy(ucvts.cv_ts_order.order_mark, ucv.cv_order.order_mark, 1);
			break;
		case '3':
			if(!strcmp(ucvts.cv_ts_order.exchange_id, "BINANCE_F"))
				return OM_ERROR;
			memcpy(ucvts.cv_ts_order.order_mark, ucv.cv_order.order_mark, 1);
			break;
		case '2':
			//if(!strcmp(ucvts.cv_ts_order.exchange_id, "BINANCE_F")) // add this if exchange support
				return OM_ERROR;
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
#ifdef DEBUG
	printf("caSeqno = %s\n", caSeqno);
#endif
	return atol(caSeqno);
}
