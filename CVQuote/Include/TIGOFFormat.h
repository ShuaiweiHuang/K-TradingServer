#ifndef INCLUDE_TIGOFFORMAT_H_
#define INCLUDE_TIGOFFORMAT_H_

struct TIG_OF_ORDER_HEAD
{
	char head_reply_code[2];  
	char head_trans_code[2]; 
	char head_function_code[2]; 
	char head_term_id[12];
	char head_broker[7];
	char head_unit_no[3];
	char head_broker_name[10];
	char head_tel_no[2];
	char head_today[4];
};

struct TIG_OF_ORDER
{
	struct TIG_OF_ORDER_HEAD head;
	char server_no[1];              /*@ '1' */
	char broker[7];                 /*@ BROKER ID */
	char unit_no[3];                /*@ UNIT NO */
	char order_no[5];               /*  ' ' */
	char exch_id[7];                /*@ EXCHANGE ID */
	char acno[7];                /*@ INVESTOR ID */
	char open_offset[1];            /*@ 0:新倉 */
	char dayoff[1];                 /*@ ' ':否 'Y':是 */
	char order_type[1];             /*@ 1:市價 2:限價 */
	char com_id1[7];                /*@ 商品代碼1 */
	char settle_month1[6];          /*@ 商品年月1 */
	char strike_price1[10];         /*@ 履約價1   */
	char buysell1[1];               /*  買賣別1   */ 
	char com_id2[7];                /*  商品代碼2 */
	char settle_month2[6];          /*  商品年月2 */
	char strike_price2[10];         /*  履約價2   */
	char buysell2[1];               /*  買賣別2   */ 
	char order_cond[1];             /*@ ORDER CONDITION 
0:ROD 當日有效單(Rest of Day,ROD) 1:GOOD_TILL_CANCEL 長效單(Good-Till-Cancelled,GTC)
2:AT_THE_OPENING 開盤市價單(market-on-openingorder,MOO) 3:IOC 立即成交否則取消單(Immediate or Cancel,IOC)
4:FOK 全部成交否則取消單(Fill or Kill,FOK) 5:GOOD_TILL_GROSSING ??
6:GOOD_TILL_DATE ?? 7:AT_THE_CLOSE 收盤市價單(market-on-closeorder,MOC) */
	char special_order[1];          /*@ SPECIAL ORDER 
1:市價單(MKT) 2:限價單(LMT) 3:停損單(STP) 4:停損限價單(STP LIMIT) 5:收市單(MOC) 6:WITH_OR_WITHOUT 7:LIMIT_OR_BETTER 
8:LIMIT_WITH_OR_WITHOUT 9:ON_BASIS A:ON_CLOSE B:LIMIT_ON_CLOSE C:FOREX_MARKET D:PREVIOUSLY_QUOTED E: PREVIOUSLY_INDICATED 
F:FOREX_LIMIT G:FOREX_SWAP H:FOREX_PREVIOUSLY_QUOTED I:FUNARI J:觸發價(MIT) K:MARKET_WITH_LEFTOVER_AS_LIMIT
L:PREVIOUS_FUND_VALUATION_POINT M:NEXT_FUND_VALUATION_POINT P:PEGGED */
	char qty1[4];             /*@ 委託口數1*/
	char qty2[4];             /*  委託口數2*/
	char order_price[14];           /*@ 委託價(差) 5位小數 */
	char order_price_m[5];          /*  委託價(差)分子 2位小數 */
	char order_price_d[4];          /*  委託價(差)分母 */
	char touch_price[14];           /*  觸發價(差) 5位小數 */
	char touch_price_m[5];          /*  觸發價(差)分子 2位小數 */
	char touch_price_d[4];          /*  觸發價(差)分母  */
	char order_kind[2];             /*  ORDER KIND '0' */
	char upp_sid[4];                /*  0 */
	char exe_upp_sid[4];            /*  0 */
	char sale_no[4];                /*  0 */
	char agent[1];                  /*@ D:策略王 7:HTS */
	char sub_acno[7];               /* 分戶帳號 */
	char no_use_sign1[1];           /* */
	char seqno[13];                 /*@ 網路語音流水號 */
	char no_use_sign2[1];           /* */
	char key[13];                   /*@ 網路語音單號 */
	char tx_date[8];                /*@ 交易日期 */
	char client_ip[16];             /*@ ip address */
	char trade_unit_no[3];             /*@ 交易營業員所屬分公司  */
	char trade_sale_no[4];             /*@ 交易營業員 */
};

struct TIG_OF_REPLY
{
	char reply_msg[80];
	char broker[7];
	char unit_no[3];
	char acno[7];
	char order_no[5];
	char upp_no[10];
	char account_balance[12];
	char var_income[12];
	char usable_margin[12];
	char used_margin[12];
	char offset_margin[12];
	char today_ini_fee[12];
	char old_used_margin[12];
	char used_ini_fee[12];
	char account_equity[12];
	char today_premium[12];
	char primium[12];
	char maintain_rate[9];

	/*char time_log1[8];//timetest
	//char time_log2[20];
	char time_log3[8];
	char time_log4[8];
	char time_log5[8];
	char time_log6[8];
	char time_log7[8];
	char time_log8[8];
	char time_log9[8];
	char time_log10[8];//timetest*/
};

#endif
