#ifndef INCLUDE_CVGLOBAL_H_
#define INCLUDE_CVGLOBAL_H_

#define CONNECT_TCP		1
#define CONNECT_WEBSOCK		2
#define HEARTBEAT_INTERVAL_SEC  30
#define HEARTBEAT_INTERVAL_MIN  300
#define BUFFERSIZE		1024
#define IPLEN			16
#define UIDLEN			10
#define SCALE_VOL_BINANCE	1000000
#define HTBT_COUNT_LIMIT	2
#include "CVCommon/CVSharedMemory.h"

/***** PACKET IDENTIFIER *****/
#define GTA_TAIL_BYTE_1						0x0D
#define GTA_TAIL_BYTE_2						0x0A

#define ESCAPE_BYTE 						0x1b

#define LOGON_BYTE 						0x20
#define HEARTBEAT_BYTE 						0x30

#define EXIT_BYTE 						0xFF

#define ACCOUNT_COUNT_BYTE 					0x40
#define ACCOUNT_DATA_BYTE 					0x41

#define TS_ORDER_BYTE 						0x01
#define TF_ORDER_BYTE 						0x02
#define OF_ORDER_BYTE 						0x03
#define OS_ORDER_BYTE 						0x04

#define TS_COMMON_BYTE 						0x11
#define TF_COMMON_BYTE 						0x12
#define OF_COMMON_BYTE 						0x13
#define OS_COMMON_BYTE 						0x14

/***** ERROR CODE *****/
#define LOGON_FIRST_ERROR 					1099
#define BRANCH_ACCOUNT_ERROR 					1124
#define PROXY_IP_SET_ERROR 					1010
#define PROXY_NOT_PROVIDE_SERVICE_ERROR 			1011
#define PROXY_SERVICE_BUSY_ERROR 				1012

/***** DATA SIZE *****/
#define RSA_KEY_LENGTH						1024*2
#define CIPHER_KEY_LENGTH					128*2
#define INITIALIZATION_VECTOR_LENGTH				16
#define HEARTBEAT_TIME_INTERVAL					30

#define MAX_ACCOUNT_DATA_COUNT 					10
#define ORDER_REPLY_ERROR_CODE_LENGTH	 			2

#define ACCOUNT_ITEM_LENGTH 					128
#define ACCOUNT_DATA_LENGTH 					30

#define TRY_NEW_CONNECT_TO_SERVER_COUNT 			100

/***** MARKET CODE *****/
extern const char* TS_MARKET_CODE;
extern const char* TF_MARKET_CODE;
extern const char* OF_MARKET_CODE;
extern const char* OS_MARKET_CODE;

/***** HEARTBEAT MESSAGE *****/
extern const char* g_pHeartbeatRequestMessage;
extern const char* g_pHeartbeatReplyMessage;

extern unsigned char g_uncaHeartbeatRequestBuf[6];
extern unsigned char g_uncaHeaetbeatReplyBuf[6];

/***** ERROR MESSAGE *****/
extern const char* REPEAT_LOGON_STATUS_CODE;
extern const char* REPEAT_LOGON_ERROR_CODE;

extern char g_caWhat[7][128];
extern char g_caWhatError[7][128];
#endif
