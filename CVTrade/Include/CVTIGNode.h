#ifndef INCLUDE_SKTIGNODE_H_
#define INCLUDE_SKTIGNODE_H_

/*
TCPIP Process Name(8Bytes)
Comma   		  (1Byte)
IP     			  (20Bytes)
Comma             (1Byte)
Listen Port       (5Bytes)
Comma             (2Byte)
Comment           (20Bytes)
Comma             (1Byte)
TIG Process Name  (8Bytes)
EOL               (2Bytes)
*/

struct TSKTIGNode{
	unsigned char uncaTCPIPProcessName[8];
	unsigned char uncaComma1[1];
	unsigned char uncaIP[20];
	unsigned char uncaComma2[1];
	unsigned char uncaListenPort[5];
	unsigned char uncaComma3[2];
	unsigned char uncaComment[20];
	unsigned char uncaComma4[1];
	unsigned char uncaTIGProcessName[8];
};

#endif
