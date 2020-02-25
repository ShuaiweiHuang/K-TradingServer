#ifndef INCLUDE_TIGTFFORMAT_H_
#define INCLUDE_TIGTFFORMAT_H_

struct TIG_TF_ORDER_HEAD
{
   char head_reply_code[2];
   char head_trans_code[2];
   char head_function_code[2];
   char head_term_id[12];
   char head_broker[7];
   char head_unit_no[3];
   char head_broker_name[10];
   char head_tel_no[2];
   char head_today[4];
};

struct TIG_TF_ORDER
{
    struct TIG_TF_ORDER_HEAD head;

    char server_no[1];          /* WW-SERVER-NO         服務代碼   X(1)       */
    char broker[7];             /* WW-BROKER-ID         期貨商代碼 X(7)       */
    char unit_no[3];            /* WW-IB-NO             分公司別   X(3)       */
    char order_no[5];           /* WW-ORDER-NO          委託書編號 X(5)       */
    char exch_id[7];            /* WW-EXCH-ID           期交所代碼 X(7)       */
    char acno[7];            /* WW-INVESTOR-ACNO     投資人帳號 9(7)       */
    char open_offset[1];        /* WW-OPEN-OFFSET-KIND  新平倉碼   X(1)       */
    char dayoff[1];             /* WW-DAY-TRADE-ID      當沖單     X(1)       */
    char order_type[1];         /* WW-ORDER-TYPE        委託種類   X(1)       */
    char com_id1[7];            /* WW-COMMODITY-ID1     商品代碼1  X(7)       */
    char settle_month1[6];      /* WW-SETTLEMENT-MONTH1 交割月份1  9(6)       */
    char strike_price1[9];      /* WW-STRIKE-PRICE1     履約價格1  9(6)V9(3)  */
    char buysell1[1];           /* WW-BUY-SELL-KIND1    買賣記號1  X(1)       */
    char com_id2[7];            /* WW-COMMODITY-ID2     商品代碼2  X(7)       */
    char settle_month2[6];      /* WW-SETTLEMENT-MONTH2 交割月份2  9(6)       */
    char strike_price2[9];      /* WW-STRIKE-PRICE2     履約價格2  9(6)V(3)   */
    char buysell2[1];           /* WW-BUY-SELL-KIND2    買賣記號2  X(1)       */
    char order_cond[1];         /* WW-ORDER-COND        委託條件   X(1)       */
    char special_order[1];      /* WW-SPECIAL-ORDER     停損單記號 X(1)       */
    char qty1[4];         /* WW-ORDER-QTY         委託口數   9(4)       */
    char qty2[4];         /* WW-ORDER-QTY2        委託口數二 9(4)       */
    char order_price1[10];      /* WW-ORDER-PRICE       委託價格   S9(5)V9(4) */
    char order_price2[10];      /* WW-ORDER-PRICE2      委託價格二 S9(5)V9(4) */
    char touch_price[9];        /* WW-TOUCH-PRICE       觸發價格   9(5)V9(4)  */
    char no_use_sign1[1];
    char seqno[13];             /* WW-INPUT-SEQNO       來源流水號 S9(13)     */
    char no_use_sign2[1];
    char key[13];               /* WW-INPUT-KEY         來源索引   S9(13)     */
    char sale_no[4];            /* WW-AE-ID             營業員代碼 9(4)       */
    char agent[1];              /* WW-AP2-IN-KIND       來源種類   X(1)       */
    char sub_acno[7];           /* WW-SUB-ACCOUNT       子帳號     9(7)       */
    char tx_date[8];            /* WW-TX-DATE           交易日期   9(8)       */
    char client_ip[16];         /* WW-SOURCE-IP         IP         X(16)      */
    char price_flag[1];         /* WW-PRICE-FLAG        價格旗標   X(1)       */
    char agent_new[2];          /* WW-AP2-IN-KIND-N     新來源種類 X(2)       */
    char market_date_kind[3];   /* WW-MARKET-DATE-KIND  資訊來源別 X(3)       */
    char filler[20];            /* WW-FILLER                       X(20)      */
};

struct TIG_TF_REPLY
{
	char reply_msg[80];
	char broker[7];
	char unit_no[3];
	char acno[7];
	char order_no[5];
};

#endif
