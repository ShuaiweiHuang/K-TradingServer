#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#define MAXDATA   1024
#define ORDER_LENGTH 400

struct CV_TS_ORDER
{
        char prefix[2];
        char broker[4];
        char agent[2];
        char key[13];
        char trade_type[2];
        char step_mark[1];
        char order_type[1];
        char tx_date[8];
        char order_time[9];
        char acno[7];  
        char sub_acno[7];
        char com_id[6]; 
        char qty[9];   
        char order_price[9]; 
        char buysell[1];    
        char sale_no[4];
        char order_no[5];   
        char switch_data[40]; 
        char user_def[128];
        char order_id[10];
        char price_mark[1];
        char ota_mark[1];
        char ota_order_id[10];
        char keyin_id[6];
        char price_type[1];
        char order_cond[1];
        char reserved[32];
} ts_order;


