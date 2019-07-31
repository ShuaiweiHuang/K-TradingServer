#ifndef INCLUDE_CVTSFORMAT_H_
#define INCLUDE_CVTSFORMAT_H_

struct CV_TS_ORDER
{
	char control_bit[2];
	char order_id[10];
	char sub_acno_id[7];
	char stategy_name[16];
	char agent_id[2];
	char broken_id[4];
	char exchange_id[3];
	char seq_id[13];
	char symbol_name[10];
	char symbol_type[1];
	char symbol_mark[1];
	char order_offset[1];
	char order_dayoff[1];
	char order_date[8];
	char order_time[8];
	char order_type[1];
	char order_butsell[1];
	char order_bookno[36];
	char order_cond[1];
	char order_mark[1];
	char trade_type[1];
	char prick_mark[1];
	char order_price[9];
	char touch_price[9];
	char qty_mark[1];
	char order_qty[9];
	char order_kind[2];
	char reserverd[97];
};

struct CV_TS_REPLY
{
	char control_bit[2];
	struct CV_TS_ORDER original;
	char error_code[2];
	char reply_msg[252];
};

#endif
