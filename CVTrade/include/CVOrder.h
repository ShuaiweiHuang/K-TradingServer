#ifndef INCLUDE_CVORDER_H_
#define INCLUDE_CVORDER_H_

#include "CVTSFormat.h"

union CV_ORDER
{
	struct CV_TS_ORDER sk_ts_order;
	char data[256];
};

#endif
