#ifndef INCLUDE_TIGOSFORMAT_H_
#define INCLUDE_TIGOSFORMAT_H_

struct TIG_OS_ORDER
{
   char ctl_id[8];                	/* GHS200 */
   char seqno[13];          		/* recv_no */
   char key[13];          			/* key_no  */
   /* --- */
   char function_code[2];         	/* ������O 01:�R�i, 02:��X, 04:���� */
   char unit_no[4];               	/* ��� */
   char acno[8];               		/* �b�� 7 or 8 */
   char buysell[1];               	/* �R��O�� B:�R�i S:��X */
   char special_order[1];          	/* �S���O�� " " �@��� "S" ���l�� "B" LIMIT ON CLOSE �s�©w�q? */
   char good_no[2];               	/* �ӫ~�N�� HK ? */
   char com_id[6];              	/* �Ѳ��N�� 000000 */
   char qty[14];                  	/* �e�U�Ѽ� S9(09)V9(04) */
   char order_price[15];            /* �e�U���� S9(08)V9(06) */
   char price_mark[1];            	/* �e�U����O�� " ":���� 0:�Q�� 1:���� 2:�^�� */
   char acc_kind[1];              	/* �M��O 1:�~���M�� 2:�x���M�� */
   char preorder[1];             	/* �w����O�� " ":�L���� 1:�w���� */
   char agent[2];          			/* �U��ӷ��O ?? */
   char step_mark[1];             	/* �L�O */
   char client_ip[16];            	/* �ӷ� IP �� CALLER-ID */
   char touch_price[15];           	/* ���lĲ�o���� S9(08)V9(06) */
   char aon_mark[1];              	/* 'A':AON " ":PARTIAL */
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
