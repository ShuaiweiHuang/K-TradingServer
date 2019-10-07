#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>

#include "client.h"

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
		char reserved[97];
} ts_order;


#define MAXTHREAD 1024
#define TIMEDIFF 1
#define CONNRETRY 5
#define RECONNWAIT 3
#define WAITUSERMODE 40


pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;

char	*g_IP_port;
char	*g_IP_addr;
char	*g_username;
char	*g_password;
int 	g_IsLogin;
int	g_Loopnum;
int	g_Ordernum;
int	g_Threadexp;
int	g_threadcount;
char	*g_Account;
char	g_echostage;

int create_socket() {

	int fd;
	int i;
	int length;
	int fdesc;
	int nidata;
	int conn_retry = CONNRETRY;

	struct hostent *hp;
	struct sockaddr_in myaddr;
	struct sockaddr_in servaddr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror ("Error: socket failed!");
		return -1;
	}

	bzero((char *)&myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) <0) {
		perror ("Error: bind failed!");
		return -1;
	}
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(g_IP_port));

	hp = gethostbyname(g_IP_addr);
	bcopy(hp->h_addr_list[0], (caddr_t)&servaddr.sin_addr, hp->h_length);

	if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("Error: connect failed!");
		return -1;
	}

	return fd;
}

void* test_run(void *arg)
{

	SSL_METHOD *method;
	SSL_CTX *ctx = NULL;
	SSL	*ssl = NULL;
	int	server = -1, thread_ret = 0, is_conn = 1, usermode = 0, pressmode;
	int	i, exec_loop, order_loop;
	char	data[MAXDATA];
	char	data1[MAXDATA]; 
	struct	timeval tvs;
	struct	timeval tve;
	int conn_retry = CONNRETRY;
	int account_num;

	srand(time(NULL));

	for(exec_loop = 1 ; (exec_loop <= g_Loopnum) && is_conn ; exec_loop++)
	{
		printf("Start test at loop: (%d/%d)\n", exec_loop, g_Loopnum);
		pthread_mutex_lock(&conn_mutex);
		server = create_socket();
		pthread_mutex_unlock(&conn_mutex);

		if( server == -1 )
		{
			printf("\tError: fail to made the TCP connection.\n");
			thread_ret = -1;
			is_conn = 0;
			break;
		}
		else
		{
			printf("\tOpen connection success.\n");
		}

		conn_retry = CONNRETRY;

		int packs=1, len;

		if(g_IsLogin && is_conn) 
		{
			data[0] = 0x1b;
			data[1] = 0x00;
			memcpy(data+2,	g_username, 20);
			memcpy(data+22, g_password, 30);
			memcpy(data+52, "MC", 2);
			memcpy(data+54, "3.0.0.20", 8);

			if (write(server, data, 64) <= 0) 
			{
				printf("\tLogin: write to server error !");
				thread_ret = -1;
				is_conn = 0;
				break;
			}

			while(packs--)
			{
				if ((len=read(server, data, 192)) <= 0) 
				{
					printf("\tLogin: read from aptrader_server error !");
					thread_ret = -1;
					is_conn = 0;
					break;
				}

				printf("\tLogin reply message: len=%d,%x,%x,%.2s,%.40s,%.2s,%.82s,%.64s\n", len, data[0], data[1], data+2, data+4, data+44, data+46, data+128);

				if ((len=read(server, data, 3)) <= 0) 
				{
					printf("\tLogin: read from aptrader_server error !");
					thread_ret = -1;
					is_conn = 0;
					break;
				}


				if(data[1] == 0x02) {
					account_num = (data[2] );
					printf("\tLogin: account num packet len=%d, %x, %x, %x\n", len, data[0], data[1], data[2]);
				}
				if ((len=read(server, data, 2+account_num*21)) <= 0) 
				{
					perror ("\tLogin: read from aptrader_server error !");
					thread_ret = -1;
					is_conn = 0;
					break;
				}
				int j;
				for(j=0 ; j<account_num ; j++)
					printf("\tLogin: account list packet len=%d, %x, %x, %.4s, %.7s, %.10s\n", len, data[0], data[1], data+2+21*j, data+6+21*j, data+13+21*j);
			}
			if(!is_conn)
				break;

			memset(&ts_order, ' ', sizeof(ts_order));
			ts_order.header_bit[0] = 0x1b;
			ts_order.header_bit[1] = 0x40;
			memcpy(ts_order.sub_acno_id, g_Account, 7);
			memcpy(ts_order.strategy_name, "MACD-1234\0", sizeof(ts_order.strategy_name));
			memcpy(ts_order.agent_id, "MC", 2);
			memcpy(ts_order.broker_id, "9801", 4);
			memcpy(ts_order.exchange_id, "BITMEX_T\0", 10);
			memcpy(ts_order.seq_id, "1234567890123", 13);
			memcpy(ts_order.symbol_name, "XBTUSD\0", 7);
			memcpy(ts_order.symbol_type, "F", 1);
			memcpy(ts_order.symbol_mark, "0", 1);
			memcpy(ts_order.order_offset, "0", 1);
			memcpy(ts_order.order_dayoff, "N", 1);
			memcpy(ts_order.order_date, "20190821", 8);
			memcpy(ts_order.order_time, "17160301", 8);
			memcpy(ts_order.order_buysell, "S", 1);
			memcpy(ts_order.order_cond, "0", 1);//0:ROD
			memcpy(ts_order.order_mark, "1", 1);//0:Market 1:limit 2:protect 3:stop market 4:stop limit
			memcpy(ts_order.trade_type, "0", 1);//0:new 1:delete 2:delete all 3:change qty 4:change price
			memcpy(ts_order.order_bookno, "000000000000000000000000000000000000", 36);
			memcpy(ts_order.price_mark, "0", 1);
			memcpy(ts_order.order_price, "081440000", 9);
			memcpy(ts_order.touch_price, "083440000", 9);
			memcpy(ts_order.qty_mark, "0", 1);
			memcpy(ts_order.order_qty, "000000001", 9);
			memcpy(ts_order.order_kind,"0", 1);
			memset(&ts_order.reserved, ' ', 91);
#if 1
			if(g_echostage == 'S')
				memcpy(ts_order.trade_type, "5", 1);
			if(g_echostage == 'T')
				memcpy(ts_order.order_price, "000000000", 9);
#endif
			for(order_loop=0 ; order_loop<g_Ordernum && is_conn ; order_loop++)
			{
				printf("\t\tSend order %d: %.1s %.9s %s\n", order_loop, ts_order.order_buysell, ts_order.order_price, ts_order.order_mark=='0'?"MARKET":"Limit");
				if (write(server, (&ts_order.header_bit[0]), 256) <= 0) {
					perror("write to server error !");
					is_conn = 0;
					thread_ret = -1;
					break;
				}

				printf("\t\ttesting in order %d\n", order_loop);
				int len;

				len = read(server, data1, 1024);
				if (len < 0) 
				{
					printf("\t\tread from server error !");
					is_conn = 0;
					thread_ret = -1;
					break;
				}
				printf("\t\tread byte = %d,%x,%x,%x,%x,%.13s\n\t\tstatus:%.4s\n\t\tmsg:%s\n", len, data1[0], data1[1], data1[2], data1[3], data1+43, data1+258, data1+336);

#if 0// delete order
				memcpy(ts_order.order_bookno, data1+114, 36);
				memcpy(ts_order.key_id, data1+56, 13);
				memcpy(ts_order.trade_type, "1", 1);//0:new 1:delete 2:delete all 3:change qty 4:change price
				printf("\t\tSend order delete %d: %.1s %.9s %s\n", order_loop, ts_order.order_buysell, ts_order.order_price, ts_order.order_mark=='0'?"MARKET":"Limit");
				if (write(server, (&ts_order.header_bit[0]), 256) <= 0) {
					printf("\t\twrite to server error !");
					is_conn = 0;
					thread_ret = -1;
					break;
				}
				len = read(server, data1, 1024);
				if (len < 0) 
				{
					printf ("\t\tread from server error !");
					is_conn = 0;
					thread_ret = -1;
					break;
				}
				printf("\t\tread success: %d,%x,%x,%x,%x,%.13s\n\t\tstatus:%.4s\n\t\tmsg:%s\n", len, data1[0], data1[1], data1[2], data1[3], data1+43, data1+258, data1+336);
#endif

			}//end loop for order
		}// end login
		data1[0] = 0x1b;
		data1[1] = 0x7f;
		write(server, data1, 2);
		printf("\tSend disconnect message.\n");
		pthread_mutex_lock(&conn_mutex);
		if(server != -1)
		{
			close(server);
			server = -1;
			printf("\tClose connection success.\n");
		}
		pthread_mutex_unlock(&conn_mutex);

	}//end loop of execution

	if(!is_conn) {
		pthread_mutex_lock(&conn_mutex);
		if(server != -1)
			close(server);
		pthread_mutex_unlock(&conn_mutex);

	}
	*(int*)arg = thread_ret;
	pthread_mutex_lock(&count_mutex);
	printf("count = %d\n", g_threadcount);
	g_threadcount--;
	pthread_mutex_unlock(&count_mutex);
}


pthread_mutex_t *ssl_mutex = NULL;

static void ssl_locking_cb (int mode, int type, const char* file, int line)
{
  if (mode & CRYPTO_LOCK)
	pthread_mutex_lock(&ssl_mutex[type]);
  else
	pthread_mutex_unlock(&ssl_mutex[type]);
}

static unsigned long ssl_id_cb (void)
{
  return (unsigned long)pthread_self();
}


int main(int argc, char **argv[])
{
	signal(SIGPIPE, SIG_IGN);
	setbuf(stdout, NULL);
	pthread_t t[MAXTHREAD];
	int ret[MAXTHREAD];
	int i, retval;

	if(argc < 9) {
		printf("./connect_srv USERNAME PASSWORD ISLOGIN LOOPNUM ORDERNUM THREAD_EXP ACCOUNT PATH(S:server, T:tandem)\n");
		return -1;
	}
	g_username	= &argv[1][0];
	g_password	= &argv[2][0];
	g_IsLogin	= atoi(&argv[3][0]);
	g_Loopnum	= atoi(&argv[4][0]);
	g_Ordernum	= atoi(&argv[5][0]);
	g_Threadexp	= atoi(&argv[6][0]);
	g_Account	= argv[7];
	g_echostage	= argv[8][0];
	g_IP_addr 	= &argv[9][0];
	g_IP_port 	= &argv[10][0];

	printf("DEST = %s:%s\n", g_IP_addr, g_IP_port);
	printf("USERNAME=%s, PASSWORD=%s, ISLOGIN=%d, LOOPNUM=%d, ORDERNUM=%d, THREAD_EXP=%d, ACCOUNT=%s, TESTPATH=%c\n",
		g_username, g_password,	g_IsLogin, g_Loopnum, g_Ordernum, g_Threadexp, g_Account, g_echostage);

#ifdef SSLTLS
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	SSL_library_init();

	if ((ssl_mutex = (pthread_mutex_t*)OPENSSL_malloc(sizeof(pthread_mutex_t) * CRYPTO_num_locks())) == NULL)
		printf("malloc() failed.\n");

	for(i=0 ; i < CRYPTO_num_locks(); i++)
			pthread_mutex_init(&ssl_mutex[i], &ret[i]);

	/* Set up locking function */
	CRYPTO_set_locking_callback(ssl_locking_cb);
	CRYPTO_set_id_callback(ssl_id_cb);
#endif

	g_threadcount = 0;
	int thread_num = 1;
	while(g_Threadexp--)
		thread_num*=2;

	for(i=0 ; i < thread_num; i++)
	{
		pthread_mutex_lock(&count_mutex);
		g_threadcount++;
		pthread_mutex_unlock(&count_mutex);

		if(pthread_create(&t[i], NULL, test_run, (void *)&ret[i]) != 0)
		{
			printf("create thread fail.\n");
			exit(-1);
		}
		else
		{
			printf("create thread:%d\n", g_threadcount);
		}
	}
	while(g_threadcount) { sleep(1); }

	retval = 0;
	for(i=0 ; i < thread_num; i++)
	{
		printf("thread return = %d\n", ret[i]);
		if(ret[i] != 0)
		{
			retval = -1;
			break;
		}
	}

#ifdef SSLTLS
	CRYPTO_set_locking_callback(NULL);

	for (i=0; i<CRYPTO_num_locks(); i++)
		pthread_mutex_destroy(&ssl_mutex[i]);

	OPENSSL_free(ssl_mutex);
#endif
	return retval;
}
