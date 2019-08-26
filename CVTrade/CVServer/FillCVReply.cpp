#include <cstring>
#include <endian.h>
#include <cstdio>
#include <cstdlib>

#include "../include/CVType.h"
#include "../include/CVGlobal.h"

void FillBitcoinReplyFormat(union CV_ORDER_REPLY &cv_order_reply, union CV_TS_ORDER_REPLY &cv_ts_order_reply)
{
	memcpy(cv_order_reply.cv_reply.error_code, cv_ts_order_reply.cv_ts_reply.status_code, 4);
	printf("FillBitcoinReplyFormat: %.4s\n", cv_ts_order_reply.cv_ts_reply.status_code);
	sprintf(cv_order_reply.cv_reply.reply_msg, "%s", cv_ts_order_reply.cv_ts_reply.reply_msg);
	printf("FillBitcoinReplyFormat: %s\n", cv_ts_order_reply.cv_ts_reply.reply_msg);
	memcpy(cv_order_reply.cv_reply.original.order_bookno, cv_ts_order_reply.cv_ts_reply.bookno, 36);
	printf("FillBitcoinReplyFormat: %.4s\n", cv_ts_order_reply.cv_ts_reply.status_code);
	cv_order_reply.cv_reply.header_bit[0] = ESCAPE;
	cv_order_reply.cv_reply.header_bit[1] = ORDERREP;
}
