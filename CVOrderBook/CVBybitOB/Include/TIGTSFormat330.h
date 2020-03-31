#ifndef INCLUDE_TIGTSFORMAT_H_
#define INCLUDE_TIGTSFORMAT_H_

struct TIG_TS_ORDER
{
	char ctl_id[8];
	char seqno[13];
	char key[13];
	char function_code[1];  /* 1 buy, 2 sell, 3 upd, 4 can */
	char acno[7];
	char buysell[1];
	char order_type[1];   	/* 0 general, 1 sublend, 2 subcredit, 3 lend 4 credit*/
	char com_id[6];
	char qty[7];
	char order_price[6];
	char sale_no[4];
	char unit_no[4];
	char tx_date[7];
	char market_mark[1];
	char step_mark[1];  	/* 1 stock, 2 few, 3 after, 4 bid */
	char agent[2];
	char order_no[5];
	char price_mark[1];
	char sub_acno[7];
	char preorder[1]; 		/* ' ' normal, 1 preorder */
	char ota_mark[1];   	/* ' ' */
	char ota_order_id[10];  /* ' ' x 10 */
	char client_ip[16];
	char filler[18];
};

struct TIG_TS_REPLY
{
	char reply_code[4];
	char status_code[4];
	char tx_ymd[7];
	char seqno[13];
	char order_no[5];
	char reply_msg[78];
};
struct TIG_TS_REPLY_ERROR
{
	char reply_code[4];
	char status_code[4];
	char reply_msg[78];
};

#endif
