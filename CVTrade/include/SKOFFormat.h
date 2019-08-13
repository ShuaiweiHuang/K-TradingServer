#ifndef INCLUDE_SKOFFORMAT_H_
#define INCLUDE_SKOFFORMAT_H_
//===================================================================================
//=                          ���� �榡                                              =
//===================================================================================
struct SK_OF_ORDER
{
   	char trade_type[1];				/* 0:new, 1:cancel 2:redue amount */ /* 20161222 add by A97585 */
	//char head_broker[7];
	//char head_unit_no[3];
	char broker[7];                 /*@ BROKER ID */
	char unit_no[3];                /*@ UNIT NO */
	char order_no[5];               /*  ' ' */
	char exch_id[7];                /*@ EXCHANGE ID */
	char acno[7];                	/*@ INVESTOR ID */
	char com_type[1];				/* F:future, O:option */ /* 20170125 add by A97585 */
	char open_offset[1];            /*@ 0:�s�� */
	char dayoff[1];                 /*@ ' ':�_ 'Y':�O */
	char com_id1[7];                /*@ �ӫ~�N�X1 */
	char settle_month1[6];          /*@ �ӫ~�~��1 */
	char strike_price1[10];         /*@ �i����1   */
	char buysell1[1];               /*  �R��O1   */ 
	char com_id2[7];                /*  �ӫ~�N�X2 */
	char settle_month2[6];          /*  �ӫ~�~��2 */
	char strike_price2[10];         /*  �i����2   */
	char buysell2[1];               /*  �R��O2   */ 
	char order_cond[1];             /*@ ORDER CONDITION 
0:ROD ��馳�ĳ�(Rest of Day,ROD) 1:GOOD_TILL_CANCEL ���ĳ�(Good-Till-Cancelled,GTC)
2:AT_THE_OPENING �}�L������(market-on-openingorder,MOO) 3:IOC �ߧY����_�h������(Immediate or Cancel,IOC)
4:FOK ��������_�h������(Fill or Kill,FOK) 5:GOOD_TILL_GROSSING ??
6:GOOD_TILL_DATE ?? 7:AT_THE_CLOSE ���L������(market-on-closeorder,MOC) */
	char special_order[1];          /*@ SPECIAL ORDER 
1:������(MKT) 2:������(LMT) 3:���l��(STP) 4:���l������(STP LIMIT) 5:������(MOC) 6:WITH_OR_WITHOUT 7:LIMIT_OR_BETTER 
8:LIMIT_WITH_OR_WITHOUT 9:ON_BASIS A:ON_CLOSE B:LIMIT_ON_CLOSE C:FOREX_MARKET D:PREVIOUSLY_QUOTED E: PREVIOUSLY_INDICATED 
F:FOREX_LIMIT G:FOREX_SWAP H:FOREX_PREVIOUSLY_QUOTED I:FUNARI J:Ĳ�o��(MIT) K:MARKET_WITH_LEFTOVER_AS_LIMIT
L:PREVIOUS_FUND_VALUATION_POINT M:NEXT_FUND_VALUATION_POINT P:PEGGED */
	char qty1[4];             		/*@ �e�U�f��1*/
	char order_price[14];           /*@ �e�U��(�t) 5��p�� */
	char order_price_m[5];          /*  �e�U��(�t)���l 2��p�� */
	char order_price_d[4];          /*  �e�U��(�t)���� */
	char touch_price[14];           /*  Ĳ�o��(�t) 5��p�� */
	char touch_price_m[5];          /*  Ĳ�o��(�t)���l 2��p�� */
	char touch_price_d[4];          /*  Ĳ�o��(�t)����  */
	char order_kind[2];             /*  ORDER KIND '0' */
	char sale_no[4];                /*  0 */
	char agent[1];                  /*@ D:������ 7:HTS */
	char sub_acno[7];               /* ����b�� */
	char key[13];                   /*@ �����y���渹 */
	char trade_unit_no[3];          /*@ �����~�����ݤ����q  */
	char trade_sale_no[4];          /*@ �����~�� */
	char order_id[10];				//  X(10)   /* 20170120 add by A98585 */
	char user_def[128];  			//	X(128)	user define  TERM_ID+SEQ_NO  *key_1 /* 20171226 add by A98585 */

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

struct SK_OF_REPLY
{
	struct SK_OF_ORDER original;

	char reply_msg[80];
	/*
	   char broker_id[7];
	   char ib_no[3];
	   char investor_acno[7];
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
	*/
	char error_code[2];
};
#endif
