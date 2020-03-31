#include <cstring>
#include <stdio.h> 
#include <sys/time.h> 
#include <time.h> 

#include "CVGlobal.h"

const char* TS_MARKET_CODE = "TS";
const char* TF_MARKET_CODE = "TF";
const char* OF_MARKET_CODE = "OF";
const char* OS_MARKET_CODE = "OS";

const char* g_pHeartbeatRequestMessage = "HTBT";
const char* g_pHeartbeatReplyMessage = "HTBR";

unsigned char g_uncaHeartbeatRequestBuf[6];
unsigned char g_uncaHeaetbeatReplyBuf[6];

const char* REPEAT_LOGON_STATUS_CODE = "7160";
const char* REPEAT_LOGON_ERROR_CODE = "M716";

char g_caWhat[7][128] = {"RECV_KEY", "RECV_LOGON", "RECV_HEARTBEAT", "RECV_TS",
														 			 "RECV_TF", 
														 			 "RECV_OF", 
														 			 "RECV_OS"};

char g_caWhatError[7][128] = {"RECV_KEY_ERROR", "RECV_LOGON_ERROR", "RECV_HEARTBEAT_ERROR", "RECV_TS_ERROR",
																		  					"RECV_TF_ERROR", 
																		  					"RECV_OF_ERROR", 
																		  					"RECV_OS_ERROR"};


void InitialGlobal()
{
	memset(g_uncaHeartbeatRequestBuf, 0, sizeof(g_uncaHeartbeatRequestBuf));
	memset(g_uncaHeaetbeatReplyBuf, 0, sizeof(g_uncaHeaetbeatReplyBuf));

	g_uncaHeartbeatRequestBuf[0] = g_uncaHeaetbeatReplyBuf[0] = ESCAPE_BYTE;
	g_uncaHeartbeatRequestBuf[1] = g_uncaHeaetbeatReplyBuf[1] = HEARTBEAT_BYTE;

	memcpy(g_uncaHeartbeatRequestBuf+2, g_pHeartbeatRequestMessage, strlen(g_pHeartbeatRequestMessage));
	memcpy(g_uncaHeaetbeatReplyBuf+2, g_pHeartbeatReplyMessage, strlen(g_pHeartbeatReplyMessage));
}
