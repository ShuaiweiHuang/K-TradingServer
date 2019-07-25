#ifndef INCLUDE_SKOSFORMAT_H_
#define INCLUDE_SKOSFORMAT_H_

struct SK_OS_ORDER
{
	char key[13];          			/* key_no  */
	char trade_type[1];				/* 0:new, 1:cancel */ /* 20161221 add by A97585 */
	char unit_no[4];               	/* ��� */
	char acno[8];               	/* �b�� 7 or 8 */
	char buysell[1];               	/* �R��O�� B:�R�i S:��X */
	char special_order[1];         	/* �S���O�� " " �@��� "S" ���l�� "B" LIMIT ON CLOSE �s�©w�q? */
	char good_no[2];               	/* �ӫ~�N�� HK ? */
	char com_id[6];              	/* �Ѳ��N�� 000000 */
	char qty[14];                  	/* �e�U�Ѽ� S9(09)V9(04) */
	char order_price[15];           /* �e�U���� S9(08)V9(06) */
	char price_mark[1];            	/* �e�U����O�� " ":���� 0:�Q�� 1:���� 2:�^�� */
	char acc_kind[1];              	/* �M��O 1:�~���M�� 2:�x���M�� */
	char agent[2];          		/* �U��ӷ��O ?? */
	char touch_price[15];           /* ���lĲ�o���� S9(08)V9(06) */
	char tif[1];                   	/* ����ɮ� 0:DAY 2:At the Opening 7:At the Closing */
	char expire_date[8];           	/* ���Ĥ��GTD */
	char sub_acno[8];              	/* �l�b�b�� */
	char confirm_sw_area[10];      	/* [0]:�M���ι��O�ڶ����� [1]:����W�L�Q�馬�L10% */
	char currency_mark[1];         	/* ��@���O/�h���O ("Y":�h���O " ":��@���O) */
	char tx_currency1[3];          	/* ��ι��O1 */
	char tx_currency2[3];          	/* ��ι��O2 */
	char tx_currency3[3];          	/* ��ι��O3 */
	char unit_no2[4];              	/* ��� 5270.. */
	char sale_no[4];               	/* ��~���N�� 0011 */
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
