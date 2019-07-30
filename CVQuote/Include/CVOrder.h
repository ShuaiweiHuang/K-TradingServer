#ifndef INCLUDE_CVORDER_H_
#define INCLUDE_CVORDER_H_

#include "CVTSFormat.h"
#include "CVTFFormat.h"
#include "CVOFFormat.h"
#include "CVOSFormat.h"

union CV_ORDER
{
	struct CV_TS_ORDER sk_ts_order;
	struct CV_TF_ORDER sk_tf_order;
	struct CV_OF_ORDER sk_of_order;
	struct CV_OS_ORDER sk_os_order;
	char data[512];
};

#endif
