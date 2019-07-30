#ifndef INCLUDE_CVTSFORMAT_H_
#define INCLUDE_CVTSFORMAT_H_
//===================================================================================
//=                          證券 格式                                              =
//===================================================================================

#ifdef IPVH_331
struct CV_TS_ORDER
{
	char broker[4];				//	X(04)	證商代號	          BROKER_NO+BRANCH_NO
	char agent[2];				//	X(02)	來源代碼	註一
	char key[13];				//	9(08)	流水號	
	char trade_type[2];			//	9(01)	0:new 1:cancel 2:change amount
	char step_mark[1];			//	9(01)	交易盤別	註三
	char order_type[1];			//	9(01)	委託別		註四
	char tx_date[8];			//	9(08)	委託日期	
	char order_time[9];			//	9(09)	委託時間	微秒
	char acno[7];				//	9(07)	客戶帳號	          IVACNO+' '      *key_2
	char sub_acno[7];    		//  X(15) 子帳號
	char com_id[6];				//	X(06)	股票代號	          STOCK_NO        *key_3
	char qty[9];				//	9(08)	委託數量	註五      QUANTITY        *key_5
	char order_price[9];		//	9(06)V99	委託單價	      PRICE           *key_4
	char buysell[1];			//	X(01)	買賣別	B=買 S=賣   BUY_SELL_CODE   *key_6
	char sale_no[4];		
	char order_no[5];			//	X(05)	委託書編號	        TERM_ID+SEQ_NO  *key_1
	char switch_data[40]; 		//	X(30)	檢核項目強迫記號	註六 2012/5/15新增
	char user_def[128];  		//	X(128)	user define  TERM_ID+SEQ_NO  *key_1
	//【第1~30 BYTE 放  IP】
	//【第 31~43 BYTE放 新單之流水號 <改量/取消才有值,新單放13個0>】
	//【第 44~49 BYTE放賣出庫存不夠, 強制送單時放六位】
	//【第51~64 BYTE 放  timestamp】
	char order_id[10];			//  X(10)
	char price_mark[1];			//  X(01)	/* 20160711 add by A97585 */
	char ota_mark[1];			//  X(01)	/* 20161129 add by A97585 */
	char ota_order_id[10];		//  X(10)	/* 20161129 add by A97585 */
	char keyin_id[6];			//	9(6)	輸入人員員編代碼(電子單000000)
	char price_type[1];			//	X(1)	1. 市價	2.限價
	char order_cond[1];			//	X(1)	0:ROD	3:IOC	4:FOK
	char reserved[32];
};
#else
struct CV_TS_ORDER
{
	//char market[1];
	char broker[4];				//	X(04)	證商代號	          BROKER_NO+BRANCH_NO
	char agent[2];				//	X(02)	來源代碼	註一
	char key[13];				//	9(08)	流水號	
	char trade_type[1];			//	9(01)	0:new 1:cancel 2:change amount
	char step_mark[1];			//	9(01)	交易盤別	註三
	char order_type[1];			//	9(01)	委託別		註四
	char tx_date[8];			//	9(08)	委託日期	
	char order_time[9];			//	9(09)	委託時間	微秒
	char acno[7];				//	9(07)	客戶帳號	          IVACNO+' '      *key_2
	char sub_acno[7];    		//  X(15) 子帳號
	char com_id[6];				//	X(06)	股票代號	          STOCK_NO        *key_3
	char qty[8];				//	9(08)	委託數量	註五      QUANTITY        *key_5
	char order_price[8];		//	9(06)V99	委託單價	      PRICE           *key_4
	char buysell[1];			//	X(01)	買賣別	B=買 S=賣   BUY_SELL_CODE   *key_6
	//char sale_no[6];			//	X(06)	營業員代碼	
	char sale_no[4];		
	char order_no[5];			//	X(05)	委託書編號	        TERM_ID+SEQ_NO  *key_1
	char switch_data[40]; 		//	X(30)	檢核項目強迫記號	註六 2012/5/15新增
	char user_def[128];  		//	X(128)	user define  TERM_ID+SEQ_NO  *key_1
	//【第1~30 BYTE 放  IP】
	//【第 31~43 BYTE放 新單之流水號 <改量/取消才有值,新單放13個0>】
	//【第 44~49 BYTE放賣出庫存不夠, 強制送單時放六位】
	//【第51~64 BYTE 放  timestamp】
	char order_id[10];			//  X(10)   /* 20170105 add by A98585 */
	char price_mark[1];			//  X(01)	/* 20160711 add by A97585 */
	char ota_mark[1];			//  X(01)	/* 20161129 add by A97585 */
	char ota_order_id[10];		//  X(10)	/* 20161129 add by A97585 */
};
#endif
// 註一︰空白:一般
// 註二︰I=新增 C=改量 D=刪單
// 註三︰0=普通 1=鉅額 2=零股 3=定價 4=興櫃 6=拍賣 7=處所議價 8:標購 9:標借
//       (目前只處理 0普通 2零股 3定價 6拍賣) 
// 註四︰0=普通 1=代資 2=代券3=自資 4=自券 5=借券賣 6=借券賣  
// 註五︰整股/定價/拍賣 =單位數(張數)  零股=股數 ; 新單時為委託數量,改量為取消數量

struct CV_TS_REPLY
{
	struct CV_TS_ORDER original;

	char reply_msg[78];
	char error_code[2];
};

#endif
