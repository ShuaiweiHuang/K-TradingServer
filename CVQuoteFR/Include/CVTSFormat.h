#ifndef INCLUDE_CVTSFORMAT_H_
#define INCLUDE_CVTSFORMAT_H_
//===================================================================================
//=                          �Ҩ� �榡                                              =
//===================================================================================

#ifdef IPVH_331
struct CV_TS_ORDER
{
	char broker[4];				//	X(04)	�ҰӥN��	          BROKER_NO+BRANCH_NO
	char agent[2];				//	X(02)	�ӷ��N�X	���@
	char key[13];				//	9(08)	�y����	
	char trade_type[2];			//	9(01)	0:new 1:cancel 2:change amount
	char step_mark[1];			//	9(01)	����L�O	���T
	char order_type[1];			//	9(01)	�e�U�O		���|
	char tx_date[8];			//	9(08)	�e�U���	
	char order_time[9];			//	9(09)	�e�U�ɶ�	�L��
	char acno[7];				//	9(07)	�Ȥ�b��	          IVACNO+' '      *key_2
	char sub_acno[7];    		//  X(15) �l�b��
	char com_id[6];				//	X(06)	�Ѳ��N��	          STOCK_NO        *key_3
	char qty[9];				//	9(08)	�e�U�ƶq	����      QUANTITY        *key_5
	char order_price[9];		//	9(06)V99	�e�U���	      PRICE           *key_4
	char buysell[1];			//	X(01)	�R��O	B=�R S=��   BUY_SELL_CODE   *key_6
	char sale_no[4];		
	char order_no[5];			//	X(05)	�e�U�ѽs��	        TERM_ID+SEQ_NO  *key_1
	char switch_data[40]; 		//	X(30)	�ˮֶ��رj���O��	���� 2012/5/15�s�W
	char user_def[128];  		//	X(128)	user define  TERM_ID+SEQ_NO  *key_1
	//�i��1~30 BYTE ��  IP�j
	//�i�� 31~43 BYTE�� �s�椧�y���� <��q/�����~����,�s���13��0>�j
	//�i�� 44~49 BYTE���X�w�s����, �j��e��ɩ񤻦�j
	//�i��51~64 BYTE ��  timestamp�j
	char order_id[10];			//  X(10)
	char price_mark[1];			//  X(01)	/* 20160711 add by A97585 */
	char ota_mark[1];			//  X(01)	/* 20161129 add by A97585 */
	char ota_order_id[10];		//  X(10)	/* 20161129 add by A97585 */
	char keyin_id[6];			//	9(6)	��J�H�����s�N�X(�q�l��000000)
	char price_type[1];			//	X(1)	1. ����	2.����
	char order_cond[1];			//	X(1)	0:ROD	3:IOC	4:FOK
	char reserved[32];
};
#else
struct CV_TS_ORDER
{
	//char market[1];
	char broker[4];				//	X(04)	�ҰӥN��	          BROKER_NO+BRANCH_NO
	char agent[2];				//	X(02)	�ӷ��N�X	���@
	char key[13];				//	9(08)	�y����	
	char trade_type[1];			//	9(01)	0:new 1:cancel 2:change amount
	char step_mark[1];			//	9(01)	����L�O	���T
	char order_type[1];			//	9(01)	�e�U�O		���|
	char tx_date[8];			//	9(08)	�e�U���	
	char order_time[9];			//	9(09)	�e�U�ɶ�	�L��
	char acno[7];				//	9(07)	�Ȥ�b��	          IVACNO+' '      *key_2
	char sub_acno[7];    		//  X(15) �l�b��
	char com_id[6];				//	X(06)	�Ѳ��N��	          STOCK_NO        *key_3
	char qty[8];				//	9(08)	�e�U�ƶq	����      QUANTITY        *key_5
	char order_price[8];		//	9(06)V99	�e�U���	      PRICE           *key_4
	char buysell[1];			//	X(01)	�R��O	B=�R S=��   BUY_SELL_CODE   *key_6
	//char sale_no[6];			//	X(06)	��~���N�X	
	char sale_no[4];		
	char order_no[5];			//	X(05)	�e�U�ѽs��	        TERM_ID+SEQ_NO  *key_1
	char switch_data[40]; 		//	X(30)	�ˮֶ��رj���O��	���� 2012/5/15�s�W
	char user_def[128];  		//	X(128)	user define  TERM_ID+SEQ_NO  *key_1
	//�i��1~30 BYTE ��  IP�j
	//�i�� 31~43 BYTE�� �s�椧�y���� <��q/�����~����,�s���13��0>�j
	//�i�� 44~49 BYTE���X�w�s����, �j��e��ɩ񤻦�j
	//�i��51~64 BYTE ��  timestamp�j
	char order_id[10];			//  X(10)   /* 20170105 add by A98585 */
	char price_mark[1];			//  X(01)	/* 20160711 add by A97585 */
	char ota_mark[1];			//  X(01)	/* 20161129 add by A97585 */
	char ota_order_id[10];		//  X(10)	/* 20161129 add by A97585 */
};
#endif
// ���@�J�ť�:�@��
// ���G�JI=�s�W C=��q D=�R��
// ���T�J0=���q 1=�d�B 2=�s�� 3=�w�� 4=���d 6=��� 7=�B��ĳ�� 8:���� 9:�Э�
//       (�ثe�u�B�z 0���q 2�s�� 3�w�� 6���) 
// ���|�J0=���q 1=�N�� 2=�N��3=�۸� 4=�ۨ� 5=�ɨ�� 6=�ɨ��  
// �����J���/�w��/��� =����(�i��)  �s��=�Ѽ� ; �s��ɬ��e�U�ƶq,��q�������ƶq

struct CV_TS_REPLY
{
	struct CV_TS_ORDER original;

	char reply_msg[78];
	char error_code[2];
};

#endif
