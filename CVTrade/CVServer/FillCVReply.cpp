#include <cstring>
#include <endian.h>
#include <cstdio>
#include <cstdlib>

#include "../include/CVType.h"
#include "../include/CVGlobal.h"

void Convert_TigReplyCode_To_CVReplyErrorCode(union CV_ORDER_REPLY &cv_order_reply, union CV_TS_ORDER_REPLY &cv_ts_order_reply)
{
	char caReplyCode[5] = {};
	U_ByteSint bsint;
	memset(&bsint,0,16);
	memcpy(caReplyCode, cv_ts_order_reply.cv_ts_reply.reply_code, 4);
	bsint.value = atoi(caReplyCode);
	memcpy(cv_order_reply.cv_reply.error_code, bsint.uncaByte, 2);
}

void FillBitcoinReplyFormat(union CV_ORDER_REPLY &cv_order_reply, union CV_TS_ORDER_REPLY &cv_ts_order_reply)
{

	Convert_TigReplyCode_To_CVReplyErrorCode(cv_order_reply, cv_ts_order_reply);

	bool IsTandemReplySuccessful = (0 == memcmp(cv_ts_order_reply.cv_ts_reply.reply_code, "0000", 4) );

	if(IsTandemReplySuccessful) {
		memcpy(cv_order_reply.cv_reply.original.order_bookno, cv_ts_order_reply.cv_ts_reply.bookno, 5);
		memcpy(cv_order_reply.cv_reply.reply_msg, cv_ts_order_reply.cv_ts_reply.reply_msg, 78);
	}
	else {
		//reply error message
	}
}

