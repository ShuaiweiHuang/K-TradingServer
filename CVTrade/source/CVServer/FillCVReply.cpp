#include <cstring>
#include <endian.h>
#include <cstdio>
#include <cstdlib>

#include "../include/CVTSFormat.h"
#include "../include/TIGTSFormat.h"
#include "../include/CVReply.h"
#include "../include/TIGReply.h"

union L
{
   unsigned char uncaByte[16];
   short value;
};

void Convert_TigReplyCode_To_CVReplyErrorCode(union CV_REPLY &sk_reply, union TIG_REPLY &tig_reply)
{
	char caReplyCode[5] = {};
	L l;

	memset(&l,0,16);

	memcpy(caReplyCode,	tig_reply.tig_ts_reply.reply_code, 4);
	l.value = atoi(caReplyCode);
	memcpy(sk_reply.sk_ts_reply.error_code,	l.uncaByte, 2);
}

void FillTaiwanStockReplyFormat(union CV_REPLY &sk_reply, union TIG_REPLY &tig_reply)
{

	Convert_TigReplyCode_To_CVReplyErrorCode(sk_reply, tig_reply);

	bool IsTandemReplySuccessful = (0 == memcmp(tig_reply.tig_ts_reply.reply_code, "0000", 4) );

#if DEBUG
	printf("\n\nreply code : %.4s\n", tig_reply.tig_ts_reply.reply_code);
#endif

	if(IsTandemReplySuccessful)
	{
		memcpy(sk_reply.sk_ts_reply.original.order_no, 	tig_reply.tig_ts_reply.order_no, 5);
		memcpy(sk_reply.sk_ts_reply.reply_msg, 			tig_reply.tig_ts_reply.reply_msg, 78);
#if DEBUG
		printf("reply msg : %.78s\n\n", tig_reply.tig_ts_reply.reply_msg);
#endif
	}
	else
	{

#if DEBUG
		printf("reply msg : %.78s\n\n", tig_reply.tig_ts_reply.tx_ymd);
#endif
		memcpy(sk_reply.sk_ts_reply.reply_msg, 			tig_reply.tig_ts_reply.tx_ymd, 78);
	}
}

void FillTaiwanFuturesReplyFormat(union CV_REPLY &sk_reply, union TIG_REPLY &tig_reply)
{
	memcpy(sk_reply.sk_tf_reply.original.order_no, tig_reply.tig_tf_reply.order_no, 5);
	//memcpy(sk_reply.sk_tf_reply.reply_msg, tig_reply.tig_tf_reply.reply_msg, 80);
	memcpy(sk_reply.sk_tf_reply.reply_msg, tig_reply.tig_tf_reply.reply_msg + 2, 80 - 2);

	/*short nErrorCode;
	memset(&nErrorCode, 0, 2);

	if(__BIG_ENDIAN == __BYTE_ORDER)//Big-Endian
	{
		memcpy(&nErrorCode, tig_reply.tig_tf_reply.reply_msg, 2);
	}
	else if(__LITTLE_ENDIAN == __BYTE_ORDER)//Little-Endian
	{
		char* pPosition = (char*)&nErrorCode;
		memcpy(&nErrorCode, &tig_reply.tig_tf_reply.reply_msg[1], 1);
		memcpy(pPosition + 1, &tig_reply.tig_tf_reply.reply_msg[0], 1);
	}

	char caErrorCode[4];
	memset(caErrorCode, 0, 4);

	sprintf(caErrorCode, "%03d", nErrorCode);
	memcpy(sk_reply.sk_tf_reply.error_code, caErrorCode, 3);*/
	memcpy(sk_reply.sk_tf_reply.error_code, &tig_reply.tig_tf_reply.reply_msg[1], 1);
	memcpy(sk_reply.sk_tf_reply.error_code + 1, &tig_reply.tig_tf_reply.reply_msg[0], 1);
}

void FillOverseasFuturesReplyFormat(union CV_REPLY &sk_reply, union TIG_REPLY &tig_reply)
{
	memcpy(sk_reply.sk_of_reply.original.order_no, tig_reply.tig_of_reply.order_no, 5);//todo
	//memcpy(sk_reply.sk_of_reply.reply_msg, tig_reply.tig_of_reply.reply_msg, 80);
	memcpy(sk_reply.sk_of_reply.reply_msg, tig_reply.tig_of_reply.reply_msg + 2, 80 - 2);

	/*short nErrorCode;
	memset(&nErrorCode, 0, 2);

	if(__BIG_ENDIAN == __BYTE_ORDER)//Big-Endian
	{
		memcpy(&nErrorCode, tig_reply.tig_of_reply.reply_msg, 2);
	}
	else if(__LITTLE_ENDIAN == __BYTE_ORDER)//Little-Endian
	{
		char* pPosition = (char*)&nErrorCode;
		memcpy(&nErrorCode, &tig_reply.tig_of_reply.reply_msg[1], 1);
		memcpy(pPosition + 1, &tig_reply.tig_of_reply.reply_msg[0], 1);
	}

	char caErrorCode[4];
	memset(caErrorCode, 0, 4);

	sprintf(caErrorCode, "%03d", nErrorCode);
	memcpy(sk_reply.sk_of_reply.error_code, caErrorCode, 3);*/
	memcpy(sk_reply.sk_of_reply.error_code, &tig_reply.tig_of_reply.reply_msg[1], 1);
	memcpy(sk_reply.sk_of_reply.error_code + 1, &tig_reply.tig_of_reply.reply_msg[0], 1);
}

void FillOverseasStockReplyFormat(union CV_REPLY &sk_reply, union TIG_REPLY &tig_reply)
{
	Convert_TigReplyCode_To_CVReplyErrorCode(sk_reply, tig_reply);

	bool IsTandemReplySuccessful = (0 == memcmp(tig_reply.tig_os_reply.reply_code, "0000", 4) );

	if(IsTandemReplySuccessful)
	{
		memset(sk_reply.sk_os_reply.original.order_no, 		'0', 9);
		memcpy(sk_reply.sk_os_reply.original.order_no + 9, 	tig_reply.tig_os_reply.order_no, 4);
		memcpy(sk_reply.sk_os_reply.reply_msg, 				tig_reply.tig_os_reply.reply_msg, 78);
	}
	else
	{
		memcpy(sk_reply.sk_os_reply.reply_msg, tig_reply.tig_os_reply.tx_ymd, 78);
	}
}
