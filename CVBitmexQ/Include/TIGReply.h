#ifndef INCLUDE_TIGREPLY_H_
#define INCLUDE_TIGREPLY_H_

#include "TIGTSFormat.h"
#include "TIGTFFormat.h"
#include "TIGOFFormat.h"
#include "TIGOSFormat.h"

union TIG_REPLY
{
	struct TIG_TS_REPLY tig_ts_reply;
	struct TIG_TF_REPLY tig_tf_reply;
	struct TIG_OF_REPLY tig_of_reply;
	struct TIG_OS_REPLY tig_os_reply;
	char data[512];
};
union TIG_REPLY_ERROR
{
	struct TIG_TS_REPLY_ERROR tig_ts_reply;
	struct TIG_TF_REPLY tig_tf_reply;
	struct TIG_OF_REPLY tig_of_reply;
	struct TIG_OS_REPLY_ERROR tig_os_reply;
	char data[512];
};

#endif
