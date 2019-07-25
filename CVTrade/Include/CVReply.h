#ifndef INCLUDE_SKREPLY_H_
#define INCLUDE_SKREPLY_H_

#include "CVTSFormat.h"
#include "CVTFFormat.h"
#include "CVOFFormat.h"
#include "CVOSFormat.h"

union SK_REPLY
{
	struct SK_TS_REPLY sk_ts_reply;
	struct SK_TF_REPLY sk_tf_reply;
	struct SK_OF_REPLY sk_of_reply;
	struct SK_OS_REPLY sk_os_reply;
	char data[512];
};

#endif
