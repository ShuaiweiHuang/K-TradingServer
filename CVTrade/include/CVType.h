struct CV_StructLogonReply
{
	char header_bit[2];
	char status_code[2];
	char backup_ip[40];
	char error_code[2];
	char error_message[82];
	char access_token[64];
};
struct CV_StructLogout
{
	char header_bit[2];
};

struct CV_StructAccnumReply
{
	char header_bit[2];
	char accnum;
};

struct CV_StructAcclistReply
{
	char header_bit[2];
	char acno[4];
	char sub_acno[7];
	char exchange_name[10];
};

struct CV_StructOrder
{
	char header_bit[2];
	char sub_acno_id[7];
	char strategy_name[30];
	char agent_id[2];
	char broker_id[4];
	char exchange_id[10];
	char seq_id[13];
	char key_id[13];
	char symbol_name[10];
	char symbol_type[1];
	char symbol_mark[1];
	char order_offset[1];
	char order_dayoff[1];
	char order_date[8];
	char order_time[8];
	char order_buysell[1];
	char order_bookno[36];
	char order_cond[1];
	char order_mark[1];
	char trade_type[1];
	char price_mark[1];
	char order_price[9];
	char touch_price[9];
	char qty_mark[1];
	char order_qty[9];
	char order_kind[2];
	char reserved[74];
};

struct CV_StructOrderReply
{
	char header_bit[2];
	struct CV_StructOrder original;
	char error_code[4];
	char price[10];
	char avgPx[10];
	char orderQty[10];
	char lastQty[10];
	char cumQty[10];
	char transactTime[24];	
	char reply_msg[688];
};

struct CV_StructConfig
{
	char header_bit[2];
	char config_item[1];
	char config_string[13];
};

struct CV_StructConfigReply
{
	char header_bit[2];
	struct CV_StructConfig oringinal;
	char reply_msg[14];
};

struct CV_StructHeartbeat
{
	char header_bit[2];
	char heartbeat_msg[4];
};

struct CV_StructHeartbeatReply
{
	char header_bit[2];
	char heartbeat_msg[4];
};

struct CV_StructTSOrder
{
	char order_type[1];
	char client_ip[16];
	char username[20];
	char sub_acno_id[7];
	char strategy_name[30];
	char agent_id[2];
	char broker_id[4];
	char exchange_id[10];
	char seq_id[13];
	char key_id[13];
	char symbol_name[10];
	char symbol_type[1];
	char symbol_mark[1];
	char order_offset[1];
	char order_dayoff[1];
	char order_date[8];
	char order_time[8];
	char order_buysell[1];
	char order_bookno[36];
	char order_cond[1];
	char order_mark[1];
	char trade_type[1];
	char price_mark[1];
	char order_price[9];
	char touch_price[9];
	char qty_mark[1];
	char order_qty[9];
	char order_kind[2];
	char apiKey_order[65];
	char apiSecret_order[65];
	char account[20];
};

struct CV_StructTSOrderReply
{
	char status_code[4];
	char key_id[13];
	char bookno[36];
	char price[10];
	char avgPx[10];
	char orderQty[10];
	char lastQty[10];
	char cumQty[10];
	char transactTime[24];	
	char reply_msg[641];
};

union CV_ORDER
{
	struct CV_StructOrder cv_order;
	char data[256];
};
union CV_ORDER_REPLY
{
	struct CV_StructOrderReply cv_reply;
	char data[1024];
};

union CV_TS_ORDER
{
	struct CV_StructTSOrder cv_ts_order;
	char data[256];
};

union CV_TS_ORDER_REPLY
{
	struct CV_StructTSOrderReply cv_ts_reply;
	char data[1024];
};
