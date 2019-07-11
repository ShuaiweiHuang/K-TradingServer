#ifndef INCLUDE_TIGORDER_H_
#define INCLUDE_TIGORDER_H_

#include "TIGTSFormat.h"
#include "TIGTFFormat.h"
#include "TIGOFFormat.h"
#include "TIGOSFormat.h"

union TIG_ORDER
{
	struct TIG_TS_ORDER tig_ts_order;
	struct TIG_TF_ORDER tig_tf_order;
	struct TIG_OF_ORDER tig_of_order;
	struct TIG_OS_ORDER tig_os_order;
	char data[512];
};

#endif
