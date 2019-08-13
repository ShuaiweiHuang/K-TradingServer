#define ORDER_URL "https://testnet.bitmex.com/api/v1/order"
#define ORDER_ALL_URL "https://testnet.bitmex.com/api/v1/order/all"

#define MAXDATA		1024

#define ESCAPE		0x1b
#define LOGREQ		0x00
#define LOGREP		0x01
#define ACCNUMREQ	0x02
#define ACCNUMREP	0x03
#define ACCLISTREQ	0x04
#define ACCLISTREP	0x05
#define ORDERREQ	0x80
#define ORDERREP	0x81
#define CONTROLREQ	0xC0
#define CONTROLREP	0xC1
#define HEARTBEATREQ	0xF0
#define HEARTBEATREP	0xF1
#define DISCONNMSG	0xFF

#define TT_ERROR	-1100
#define AG_ERROR	-1101
#define OO_ERROR	-1102
#define OD_ERROR	-1103
#define OT_ERROR	-1104
#define BS_ERROR	-1105
#define OC_ERROR	-1106
#define OM_ERROR	-1107
#define PM_ERROR	-1108
#define PR_ERROR	-1109
#define TR_ERROR	-1110
#define QM_ERROR	-1111
#define QT_ERROR	-1112
#define OK_ERROR	-1113

#define HEARTBEATVAL 	60
union U_ByteSint
{
   unsigned char uncaByte[16];
   short value;
};

