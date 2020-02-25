#ifndef INCLUDE_TIGTSFORMAT_H_
#define INCLUDE_TIGTSFORMAT_H_


#ifdef IPVH_331
struct TIG_TS_ORDER
{
	/* DF-TX-IPVH331 */
	char ctl_id[8];
	char seqno[13];
	char key[13];

	/* AP-MO-SOURCE */
	char agent[2];
	char client_ip[16];
	char keyin_id[6];

	/* AP-MO-ITEMS */
	char function_code[2];  /* 1 buy, 2 sell, 3 reduction, 4 cancel, 5 query, 6 change price */
	char market_mark[1];
	char step_mark[1];
	char unit_no[4];
	char acno[7];
	char sub_acno[7];
	char ota_mark[1];       /* ' ' */
	char ota_order_id[10];  /* ' ' x 10 */
	char buysell[1];
	char com_id[6];
	char qty[9];
	char order_price[9];
	char price_mark[1];
	char order_type[1];
	char price_type[1];
	char time_in_force[1];
	char sale_no[4];

	/* MO-C-KEY */
	char trade_kind[2];
	char tx_date[7];
	char seq_no[9];
	char settl_type[1];
	char filler[5];
	
	/* AP-MO-ORDER-NO */
	/* MO-ORDER-NO */
	char order_no[5];
	char ap_filler[3];
};


struct TIG_TS_REPLY
{
	/*  DF-MO-REPLY-OK */
	char reply_code[4];
	char status_code[4];
	char tx_ymd[7];
	char seqno[13];

	/*  MO-R1-ORDER-NO */
	char order_no[5];
	char filler[1];
	char reply_msg[78];
};

struct TIG_TS_REPLY_ERROR
{
	char reply_code[4];
	char status_code[4];
	char reply_msg[78];
	char filler[26];
};


struct TIG_TS_PREOR_REPLY
{
	/*  EPREOR-R  */
	char ipvh331[156];
	char reply[112];
};

#else
struct TIG_TS_ORDER
{
	char ctl_id[8];
	char seqno[13];
	char key[13];
	char function_code[1];  /* 1 buy, 2 sell, 3 upd, 4 can */
	char acno[7];
	char buysell[1];
	char order_type[1];   	/* 0 general, 1 sublend, 2 subcredit, 3 lend, 4 credit*/
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

#endif //IPVH_331
#endif //INCLUDE_TIGTSFORMAT_H_
