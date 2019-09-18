#include <cstring>
#include <endian.h>
#include <cstdio>
#include <cstdlib>

#include "../include/CVType.h"
#include "../include/CVGlobal.h"

void FillBitcoinReplyFormat(union CV_ORDER_REPLY &cv_order_reply, union CV_TS_ORDER_REPLY &cv_ts_order_reply)
{
	memcpy(cv_order_reply.cv_reply.error_code, cv_ts_order_reply.cv_ts_reply.status_code, 4);
	sprintf(cv_order_reply.cv_reply.reply_msg, "%s", cv_ts_order_reply.cv_ts_reply.reply_msg);
	memcpy(cv_order_reply.cv_reply.original.order_bookno, cv_ts_order_reply.cv_ts_reply.bookno, 36);
	memcpy(cv_order_reply.cv_reply.price, cv_ts_order_reply.cv_ts_reply.price, 10);
	memcpy(cv_order_reply.cv_reply.avgPx, cv_ts_order_reply.cv_ts_reply.avgPx, 10);
	memcpy(cv_order_reply.cv_reply.orderQty, cv_ts_order_reply.cv_ts_reply.orderQty, 10);
	memcpy(cv_order_reply.cv_reply.leaveQty, cv_ts_order_reply.cv_ts_reply.leaveQty, 10);
	memcpy(cv_order_reply.cv_reply.cumQty, cv_ts_order_reply.cv_ts_reply.cumQty, 10);
	memcpy(cv_order_reply.cv_reply.transactTime, cv_ts_order_reply.cv_ts_reply.transactTime, 24);
#ifdef DEBUG
	printf("\n===========================================================================\n");
	printf("FillBitcoinReplyFormat bookno: %.36s\n", cv_ts_order_reply.cv_ts_reply.bookno);
	printf("FillBitcoinReplyFormat msg: %s\n", cv_ts_order_reply.cv_ts_reply.reply_msg);
	printf("FillBitcoinReplyFormat status: %.4s\n", cv_ts_order_reply.cv_ts_reply.status_code);
	printf("FillBitcoinReplyFormat price: %.10s\n", cv_ts_order_reply.cv_ts_reply.price);
	printf("FillBitcoinReplyFormat avgPx: %.10s\n", cv_ts_order_reply.cv_ts_reply.avgPx);
	printf("FillBitcoinReplyFormat orderQty: %.10s\n", cv_ts_order_reply.cv_ts_reply.orderQty);
	printf("FillBitcoinReplyFormat leaveQty: %.10s\n", cv_ts_order_reply.cv_ts_reply.leaveQty);
	printf("FillBitcoinReplyFormat cumQty: %.10s\n", cv_ts_order_reply.cv_ts_reply.cumQty);
	printf("FillBitcoinReplyFormat transactTime: %.24s\n", cv_ts_order_reply.cv_ts_reply.transactTime);
	printf("sizeofstructCV_StructOrderReply: %d\n", sizeof(struct CV_StructOrderReply));
	printf("FillBitcoinReplyFormat trade_type: %x\n", cv_order_reply.cv_reply.original.trade_type[0]);
	printf("\n===========================================================================\n");
#endif
	cv_order_reply.cv_reply.header_bit[0] = ESCAPE;
	cv_order_reply.cv_reply.header_bit[1] = ORDERREP;
}
