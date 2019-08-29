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
        char strategy_name[16];
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

char	*cp_port;
char	*cp_ip;
char	*cp_id;
char	*cp_pass;
int 	is_login;
int	loop_num;
int	order_num;
int	thread_exp;
int	count;
char	*cp_account;
char	test_path;

int create_socket() {

	int fd;      /* fd into transport provider */
	int i;      /* loops through user name */
	int length;     /* length of message */
	int fdesc;     /* file description */
	int nidata;     /* the number of file data */
	int conn_retry = CONNRETRY;

	struct hostent *hp;   /* holds IP address of server */
	struct sockaddr_in myaddr;   /* address that client uses */
	struct sockaddr_in servaddr; /* the server's full addr */

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
	servaddr.sin_port = htons(atoi(cp_port));

	hp = gethostbyname(cp_ip);
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
	SSL		*ssl = NULL;
	int		server = -1, ret = 0, is_conn = 1, usermode = 0, pressmode;
	int		i, exec_loop, order_loop;
	char	data[MAXDATA];
	char	data1[MAXDATA]; 
	struct	timeval tvs;
	struct	timeval tve;
	int conn_retry = CONNRETRY;
	int account_num;

	srand(time(NULL));

	for(exec_loop=0 ; (exec_loop < 1) && is_conn ; exec_loop++)
	{

		pthread_mutex_lock(&conn_mutex);
		server = create_socket();
		pthread_mutex_unlock(&conn_mutex);

		if( server == -1 ) {
			perror("Error: fail to made the TCP connection.\n");
			ret = -1;
			is_conn = 0;
			break;
		}

		conn_retry = CONNRETRY;

		if(is_login && is_conn) {
#if 1
			data[0] = 0x1b;
			data[1] = 0x00;
			memcpy(data+2,	cp_id, 20);
			memcpy(data+22, cp_pass, 30);
			memcpy(data+52, "MC", 2);
			memcpy(data+54, "3.0.0.20", 8);

			if (write(server, data, 64) <= 0) 
			{
				perror("write to server error !");
				ret = -1;
				is_conn = 0;
				break;
			}
			int packs=1, len;

			while(packs--)
			{
				if ((len=read(server, data, MAXDATA)) <= 0) 
				{
					perror ("read from aptrader_server error !");
					ret = -1;
					is_conn = 0;
					break;
				}
				if(data[1] == 0x03) {
					account_num = data[2];
					printf("account num: packet len=%d, %x, %x, %d\n", len, data[0], data[1], account_num);
				}
				if(data[1] == 0x05) {
					printf("account: packet len=%d, %x, %x,\n",	len, data[0], data[1]);
 
					for(i=0 ; i<account_num ; i++)
						printf("%.10s %.10s %.10s\n", data+2+i*30, data+12+i*30, data+22+i*30);
				}
				if(data[1] == 0x01)
					printf("login: len=%d,%x,%x,%.2s,%.40s,%.2s,%.82s\n", len, data[0], data[1], data+2, data+4, data+44, data+46);
			}
#endif
#if 0
			data[0] = 0x1b;
			data[1] = 0x06;
			memcpy(data+2,	"HBRQ", 4);
			if (write(server, data, 6) <= 0) 
			{
				perror("write to server error !");
				ret = -1;
				is_conn = 0;
				break;
			}
			packs = 1;
                        while(packs--)
                        {
                                if ((len=read(server, data, 6)) <= 0)
                                {
                                        perror ("read from aptrader_server error !");
                                        ret = -1;
                                        is_conn = 0;
                                        break;
                                }
                                if(data[1] == 0x07)
                                        printf("HTBT: len=%d,%x,%x,%.4s\n", len, data[0], data[1], data+2);
				else
                                        printf("HTBTERROR: len=%d,%x,%x,%.4s\n", len, data[0], data[1], data+2);
					
                        }
#endif
			if(!is_conn)
				break;

			memset(&ts_order, ' ', sizeof(ts_order));
			ts_order.header_bit[0] = 0x1b;
			ts_order.header_bit[1] = 0x40;
			memcpy(ts_order.sub_acno_id, cp_account, 7);
			memcpy(ts_order.strategy_name, "MACD123", 7);
			memcpy(ts_order.agent_id, "MC", 2);
			memcpy(ts_order.broker_id, "9801", 4);
			memcpy(ts_order.exchange_id, "BITMEX\0", 7);
			memcpy(ts_order.seq_id, "9487943123456", 13);
			memcpy(ts_order.symbol_name, "XBTUSD\0", 7);
			memcpy(ts_order.symbol_type, "F", 1);
			memcpy(ts_order.symbol_mark, "0", 1);
			memcpy(ts_order.order_offset, "0", 1);
			memcpy(ts_order.order_dayoff, "N", 1);
			memcpy(ts_order.order_date, "20190821", 8);
			memcpy(ts_order.order_time, "17160301", 8);
			memcpy(ts_order.order_buysell, "B", 1);
			memcpy(ts_order.order_cond, "0", 1);//0:ROD
			memcpy(ts_order.order_mark, "1", 1);//0:Market 1:limit 2:protect 3:stop market 4:stop limit
			memcpy(ts_order.trade_type, "0", 1);//0:new 1:delete 2:delete all 3:change qty 4:change price
			memcpy(ts_order.order_bookno, "000000000000000000000000000000000000", 36);
			memcpy(ts_order.price_mark, "0", 1);
			memcpy(ts_order.order_price, "100005000", 9);
			memcpy(ts_order.touch_price, "106000000", 9);
			memcpy(ts_order.qty_mark, "0", 1);
			memcpy(ts_order.order_qty, "000000001", 9);
			memcpy(ts_order.order_kind,"0", 1);
			memset(&ts_order.reserved, ' ', 91);
			for(order_loop=0 ; order_loop<order_num && is_conn ; order_loop++)
			{
				printf("send order %d: %.1s %.9s %s\n", order_loop, ts_order.order_buysell, ts_order.order_price, ts_order.order_mark=='0'?"MARKET":"Limit");
				if (write(server, (&ts_order.header_bit[0]), 256) <= 0) {
					perror("write to server error !");
					is_conn = 0;
					ret = -1;
					break;
				}

				printf("testing in order %d\n", order_loop);
				int len;

				len = read(server, data1, 1024);
				if (len < 0) 
				{
					printf("keanu read error\n");
					perror ("read from server error !");
					is_conn = 0;
					ret = -1;
					break;
				}
				printf("keanu read success\n");
				printf("read byte = %d,%x,%x,%x,%x,%.13s\nstatus:%.4s\nmsg:%.250s\n", len, data1[0], data1[1], data1[2], data1[3], data1+43, data1+258, data1+262);
#if 1
				memcpy(ts_order.order_bookno, data1+100, 36);
				memcpy(ts_order.trade_type, "1", 1);//0:new 1:delete 2:delete all 3:change qty 4:change price
				printf("send order %d: %.1s %.9s %s\n", order_loop, ts_order.order_buysell, ts_order.order_price, ts_order.order_mark=='0'?"MARKET":"Limit");
				if (write(server, (&ts_order.header_bit[0]), 256) <= 0) {
					perror("write to server error !");
					is_conn = 0;
					ret = -1;
					break;
				}
				len = read(server, data1, 1024);
				if (len < 0) 
				{
					printf("keanu read error\n");
					perror ("read from server error !");
					is_conn = 0;
					ret = -1;
					break;
				}
				printf("keanu read success\n");
				printf("read byte = %d,%x,%x,%x,%x,%.13s\nstatus:%.4s\nmsg:%.250s\n", len, data1[0], data1[1], data1[2], data1[3], data1+43, data1+258, data1+262);
#endif

			}//end loop for order
		}// end login
#if 0
		data1[0] = 0x1b;
		data1[1] = 0x7F;
		write(server, data1, 2);
#endif
		pthread_mutex_lock(&conn_mutex);
		if(server != -1) {
			close(server);
			server = -1;
		}
		pthread_mutex_unlock(&conn_mutex);

	}//end loop of execution
	if(!is_conn) {
		pthread_mutex_lock(&conn_mutex);
		if(server != -1)
			close(server);
		pthread_mutex_unlock(&conn_mutex);

	}
	*(int*)arg = ret;
	pthread_mutex_lock(&count_mutex);
	count--;
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
	//arguments

	if(argc < 9) {
		printf("sslconnect ID PASSWORD ISLOGIN(0/1) LOOPNUM ORDERNUM FORKEXP ACCOUNT PATH(S:server, T:tandem)\n");
		return -1;
	}
	cp_id		= &argv[1][0];
	cp_pass		= &argv[2][0];
	is_login	= atoi(&argv[3][0]);
	loop_num	= atoi(&argv[4][0]);
	order_num	= atoi(&argv[5][0]);
	thread_exp	= atoi(&argv[6][0]);
	cp_account	= argv[7];
	test_path	= argv[8][0];
	cp_ip 		= &argv[9][0];
	cp_port 	= &argv[10][0];

	printf("DEST = %s:%s\n", cp_ip, cp_port);
	printf("ID=%s, PASSWORD=%s, ISLOGIN=%d, LOOPNUM=%d, ORDERNUM=%d, FORKEXP=%d, ACCOUNT=%s, TESTPATH=%c\n",
			cp_id, cp_pass, 	is_login, 	loop_num, 	order_num, 	thread_exp, 	cp_account, test_path);

	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	SSL_library_init();
	int i, r;
	pthread_t t[MAXTHREAD];
	int ret[MAXTHREAD];
    if ((ssl_mutex = (pthread_mutex_t*)OPENSSL_malloc(sizeof(pthread_mutex_t) * CRYPTO_num_locks())) == NULL)
        printf("malloc() failed.\n");

    for(i=0 ; i < CRYPTO_num_locks(); i++)
            pthread_mutex_init(&ssl_mutex[i], &ret[i]);
    /* Set up locking function */
    CRYPTO_set_locking_callback(ssl_locking_cb);
    CRYPTO_set_id_callback(ssl_id_cb);

	count = 0;
	int thread_num = 1;
	while(thread_exp--)
		thread_num*=2;

	for(i=0 ; i < thread_num; i++) {

	    pthread_mutex_lock(&count_mutex);
		count++;
	    pthread_mutex_unlock(&count_mutex);

		if(pthread_create(&t[i], NULL, test_run, (void *)&ret[i]) != 0) {
			printf("create thread fail.\n");
			exit(-1);
		}
		else
			printf("create thread:%d\n", count);
	}
	
	while(count) { sleep(1); }

	r = 0;

	for(i=0 ; i < thread_num; i++) {

		if(ret[i] != 0) {
			r=-1;
			break;
		}
	}
    CRYPTO_set_locking_callback(NULL);
    for (i=0; i<CRYPTO_num_locks(); i++)
        pthread_mutex_destroy(&ssl_mutex[i]);
    OPENSSL_free(ssl_mutex);

	return r;
}
