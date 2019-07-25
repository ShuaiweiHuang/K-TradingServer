#ifndef INCLUDE_SKORDER_H_
#define INCLUDE_SKORDER_H_

#include "SKTSFormat.h"
#include "SKTFFormat.h"
#include "SKOFFormat.h"
#include "SKOSFormat.h"

union SK_ORDER
{
	struct SK_TS_ORDER sk_ts_order;
	struct SK_TF_ORDER sk_tf_order;
	struct SK_OF_ORDER sk_of_order;
	struct SK_OS_ORDER sk_os_order;
	char data[512];
};

#endif
