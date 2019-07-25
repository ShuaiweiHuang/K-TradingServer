#ifndef INCLUDE_TIGOSFORMAT_H_
#define INCLUDE_TIGOSFORMAT_H_

struct TIG_OS_ORDER
{
   char ctl_id[8];                	/* GHS200 */
   char seqno[13];          		/* recv_no */
   char key[13];          			/* key_no  */
   /* --- */
   char function_code[2];         	/* 交易類別 01:買進, 02:賣出, 04:取消 */
   char unit_no[4];               	/* 單位 */
   char acno[8];               		/* 帳號 7 or 8 */
   char buysell[1];               	/* 買賣記號 B:買進 S:賣出 */
   char special_order[1];          	/* 特殊單記號 " " 一般單 "S" 停損單 "B" LIMIT ON CLOSE 新舊定義? */
   char good_no[2];               	/* 商品代號 HK ? */
   char com_id[6];              	/* 股票代號 000000 */
   char qty[14];                  	/* 委託股數 S9(09)V9(04) */
   char order_price[15];            /* 委託價格 S9(08)V9(06) */
   char price_mark[1];            	/* 委託價格記號 " ":限價 0:昨收 1:漲停 2:跌停 */
   char acc_kind[1];              	/* 專戶別 1:外幣專戶 2:台幣專戶 */
   char preorder[1];             	/* 預約單記號 " ":盤中單 1:預約單 */
   char agent[2];          			/* 下單來源別 ?? */
   char step_mark[1];             	/* 盤別 */
   char client_ip[16];            	/* 來源 IP 及 CALLER-ID */
   char touch_price[15];           	/* 停損觸發價格 S9(08)V9(06) */
   char aon_mark[1];              	/* 'A':AON " ":PARTIAL */
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
   char filler[24];
   /* --- */
   char s_good_no[2];             	/* HK ??*/
   char s_unit_no[4];             	/* 00000 */
   char order_no[13];               /* recv_no */
};

struct TIG_OS_REPLY
{
	char reply_code[4];
	char status_code[4];
	char tx_ymd[8];
	char seqno[13];
	char good_no[2];
	char unit_no[4];
	char order_no[4];
	char reply_msg[78];
};
struct TIG_OS_REPLY_ERROR
{
	char reply_code[4];
	char status_code[4];
	char reply_msg[78];
};

#endif
