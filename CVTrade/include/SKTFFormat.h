#ifndef INCLUDE_SKTFFORMAT_H_
#define INCLUDE_SKTFFORMAT_H_

struct SK_TF_ORDER
{
	char trade_type[1];			/* 0:new, 1:cancel 2:redue amount 3:change price */ /* 20161222 add by A97585 */
	//char head_broker[7];
	//char head_unit_no[3];

	char broker[7];             /* WW-BROKER-ID         ���f�ӥN�X X(7)       */
	char unit_no[3];            /* WW-IB-NO             �����q�O   X(3)       */
	char order_no[5];           /* WW-ORDER-NO          �e�U�ѽs�� X(5)       */
	char acno[7];            	/* WW-INVESTOR-ACNO     ���H�b�� 9(7)       */
	char com_type[1];			/* F:future, O:option */ /* 20170125 add by A97585 */
	char open_offset[1];        /* WW-OPEN-OFFSET-KIND  �s���ܽX   X(1)       */
	char dayoff[1];             /* WW-DAY-TRADE-ID      ��R��     X(1)       */
	char order_type[1];         /* WW-ORDER-TYPE        �e�U����   X(1)       */
	char com_id1[7];            /* WW-COMMODITY-ID1     �ӫ~�N�X1  X(7)       */
	char settle_month1[6];      /* WW-SETTLEMENT-MONTH1 ��Τ��1  9(6)       */
	char strike_price1[9];      /* WW-STRIKE-PRICE1     �i������1  9(6)V9(3)  */
	char buysell1[1];           /* WW-BUY-SELL-KIND1    �R��O��1  X(1)       */
	char com_id2[7];            /* WW-COMMODITY-ID2     �ӫ~�N�X2  X(7)       */
	char settle_month2[6];      /* WW-SETTLEMENT-MONTH2 ��Τ��2  9(6)       */
	char strike_price2[9];      /* WW-STRIKE-PRICE2     �i������2  9(6)V(3)   */
	char buysell2[1];           /* WW-BUY-SELL-KIND2    �R��O��2  X(1)       */
	char order_cond[1];         /* WW-ORDER-COND        �e�U����   X(1)       */
	char special_order[1];      /* WW-SPECIAL-ORDER     ���l��O�� X(1)       */
	char qty1[4];         		/* WW-ORDER-QTY         �e�U�f��   9(4)       */
	//char qty2[4];         		/* WW-ORDER-QTY2        �e�U�f�ƤG 9(4)       */
	char order_price1[10];      /* WW-ORDER-PRICE       �e�U����   S9(5)V9(4) */
	char key[13];               /* WW-INPUT-KEY         �ӷ�����   S9(13)     */
	char sale_no[4];            /* WW-AE-ID             ��~���N�X 9(4)       */
	char agent[1];              /* WW-AP2-IN-KIND       �ӷ�����   X(1)       */
	char sub_acno[7];           /* WW-SUB-ACCOUNT       �l�b��     9(7)       */
	char agent_new[2];          /* WW-AP2-IN-KIND-N     �s�ӷ����� X(2)       */
	char market_date_kind[3];   /* WW-MARKET-DATE-KIND  ��T�ӷ��O X(3)       */
	char order_id[10];			//  X(10)   /* 20170119 add by A98585 */
	char user_def[128];  		//	X(128)	user define  TERM_ID+SEQ_NO  *key_1 /* 20171226 add by A98585 */
};

struct SK_TF_REPLY
{
	struct SK_TF_ORDER original;

	char reply_msg[80];
	char error_code[2];
};

#endif
