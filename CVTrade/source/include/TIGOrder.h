#ifndef INCLUDE_TIGORDER_H_
#define INCLUDE_TIGORDER_H_

#include "TIGTSFormat.h"

union TIG_ORDER
{
	struct TIG_TS_ORDER tig_ts_order;
	char data[512];
};

#endif
