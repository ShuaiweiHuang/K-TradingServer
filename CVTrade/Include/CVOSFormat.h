#ifndef INCLUDE_SKOSFORMAT_H_
#define INCLUDE_SKOSFORMAT_H_

struct SK_OS_ORDER
{
	char key[13];          			/* key_no  */
	char trade_type[1];				/* 0:new, 1:cancel */ /* 20161221 add by A97585 */
	char unit_no[4];               	/* 單位 */
	char acno[8];               	/* 帳號 7 or 8 */
	char buysell[1];               	/* 買賣記號 B:買進 S:賣出 */
	char special_order[1];         	/* 特殊單記號 " " 一般單 "S" 停損單 "B" LIMIT ON CLOSE 新舊定義? */
	char good_no[2];               	/* 商品代號 HK ? */
	char com_id[6];              	/* 股票代號 000000 */
	char qty[14];                  	/* 委託股數 S9(09)V9(04) */
	char order_price[15];           /* 委託價格 S9(08)V9(06) */
	char price_mark[1];            	/* 委託價格記號 " ":限價 0:昨收 1:漲停 2:跌停 */
	char acc_kind[1];              	/* 專戶別 1:外幣專戶 2:台幣專戶 */
	char agent[2];          		/* 下單來源別 ?? */
	char touch_price[15];           /* 停損觸發價格 S9(08)V9(06) */
	char tif[1];                   	/* 交易時效 0:DAY 2:At the Opening 7:At the Closing */
	char expire_date[8];           	/* 失效日期GTD */
	char sub_acno[8];              	/* 子帳帳號 */
	char confirm_sw_area[10];      	/* [0]:專戶交割幣別款項不足 [1]:價格超過昨日收盤10% */
	char currency_mark[1];         	/* 單一幣別/多幣別 ("Y":多幣別 " ":單一幣別) */
	char tx_currency1[3];          	/* 交割幣別1 */
	char tx_currency2[3];          	/* 交割幣別2 */
	char tx_currency3[3];          	/* 交割幣別3 */
	char unit_no2[4];              	/* 單位 5270.. */
	char sale_no[4];               	/* 營業員代號 0011 */
	char s_good_no[2];             	/* HK ??*/
	char s_unit_no[4];             	/* 00000 */
	char order_no[13];              /* recv_no */
	char order_id[10];				//  X(10)   /* 20170110 add by A98585 */
	char user_def[128];  			//	X(128)	user define  TERM_ID+SEQ_NO  *key_1 /* 20171226 add by A98585 */
};

struct SK_OS_REPLY
{
	struct SK_OS_ORDER original;

	char reply_msg[78];
	char error_code[2];
};

#endif
