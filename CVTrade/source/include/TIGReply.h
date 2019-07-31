#ifndef INCLUDE_TIGREPLY_H_
#define INCLUDE_TIGREPLY_H_

#include "TIGTSFormat.h"

union TIG_REPLY
{
	struct TIG_TS_REPLY tig_ts_reply;
	char data[512];
};
union TIG_REPLY_ERROR
{
	struct TIG_TS_REPLY_ERROR tig_ts_reply;
	char data[512];
};

#endif
