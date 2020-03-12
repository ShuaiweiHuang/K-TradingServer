#ifndef INCLUDE_CVREPLY_H_
#define INCLUDE_CVREPLY_H_

#include "CVTSFormat.h"
#include "CVTFFormat.h"
#include "CVOFFormat.h"
#include "CVOSFormat.h"

union CV_REPLY
{
	struct CV_TS_REPLY sk_ts_reply;
	struct CV_TF_REPLY sk_tf_reply;
	struct CV_OF_REPLY sk_of_reply;
	struct CV_OS_REPLY sk_os_reply;
	char data[512];
};

#endif
