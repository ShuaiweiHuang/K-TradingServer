#ifndef INCLUDE_CVREPLY_H_
#define INCLUDE_CVREPLY_H_

#include "CVTSFormat.h"

union CV_REPLY
{
	struct CV_TS_REPLY sk_ts_reply;
	char data[512];
};

#endif
