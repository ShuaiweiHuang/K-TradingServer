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

#include "client_ssl.h"


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
int		loop_num;
int		order_num;
int		thread_exp;
int		count;
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

	for(exec_loop=0 ; exec_loop < loop_num && is_conn ; exec_loop++) {

#if TIMEDIFF
		gettimeofday (&tvs, NULL) ;
#endif
		pthread_mutex_lock(&conn_mutex);
		server = create_socket();
		pthread_mutex_unlock(&conn_mutex);

		if( server == -1 ) {
			perror("Error: fail to made the TCP connection.\n");
			ret = -1;
			is_conn = 0;
			break;
		}
		method = TLSv1_2_client_method();

		if ( (ctx = SSL_CTX_new(method)) == NULL) {
			perror("Error: Unable to create a new SSL context structure.\n");
			ret = -1;
			is_conn = 0;
			break;
		}

		ssl = SSL_new(ctx);

		if(ssl == NULL) {
			perror("Error: create ssl session fail.\n");
			ret = -1;
			is_conn = 0;
			break;
		}

		SSL_set_fd(ssl, server);

		conn_retry = CONNRETRY;

		if ( SSL_connect(ssl) != 1 ) {
			ret = -1;
			is_conn = 0;
			perror("Error: Could not build a SSL session \n");
			break;
		}
#if TIMEDIFF
		gettimeofday (&tve, NULL) ;
		printf("init. time %.03ld(s):", ((tve.tv_sec-tvs.tv_sec)*1000000L + tve.tv_usec - tvs.tv_usec)/1000000L);
		printf("%.06ld(us)\n",((tve.tv_sec-tvs.tv_sec)*1000000L + tve.tv_usec - tvs.tv_usec)%1000000L);
#endif
		if(is_login && is_conn) {

			data[0] = 0x1b;
			data[1] = 0x20;
			memcpy(data+2,	cp_id, 10);
			memcpy(data+12, cp_pass, 50);
			memcpy(data+62, "D ", 2);
			memcpy(data+64, "3.0.0.20", 10);

#if TIMEDIFF
			gettimeofday (&tvs, NULL) ;
#endif
			if (SSL_write(ssl, data, 74) <= 0) 
			{
				perror("write to server error !");
				ret = -1;
				is_conn = 0;
				break;
			}
			int packs=3, len;

			while(packs--)
			{
				if ((len=SSL_read(ssl, data, MAXDATA)) <= 0) 
				{
					perror ("read from aptrader_server error !");
					ret = -1;
					is_conn = 0;
					break;
				}
				if(data[1] == 0x40) {
					account_num = data[2];
					printf("account num: packet len=%d, %x, %x, %d\n", len, data[0], data[1], account_num);
				}
				if(data[1] == 0x41) {
					printf("account: packet len=%d, %x, %x,\n",	len, data[0], data[1]);
 
					for(i=0 ; i<account_num ; i++)
						printf("%.10s %.10s %.10s\n", data+2+i*30, data+12+i*30, data+22+i*30);
				}
				if(data[1] == 0x20)
					printf("login: len=%d,%x,%x,%.4s,%.4s,%.512s\n", len, data[0], data[1], data+2, data+6, data+10);
			}

			if(!is_conn)
				break;

#if TIMEDIFF
			gettimeofday (&tve, NULL) ;
			printf("login time %.03ld(s):", ((tve.tv_sec-tvs.tv_sec)*1000000L + tve.tv_usec - tvs.tv_usec)/1000000L);
			printf("%.06ld(us)\n",((tve.tv_sec-tvs.tv_sec)*1000000L + tve.tv_usec - tvs.tv_usec)%1000000L);
#endif
			memset(&ts_order, ' ', sizeof(ts_order));

			if(test_path == 'S')
				memcpy(ts_order.trade_type,"00", 2);
			else if(test_path == 'T')
				memcpy(ts_order.trade_type,"04", 2);
			else if(test_path == 'O')
				memcpy(ts_order.trade_type,"01", 2);
			else if(test_path == 'U') {
				usermode = 1;
				memcpy(ts_order.trade_type,"04", 2);
			}
			else if(test_path == 'P') {
				pressmode = 1;
				memcpy(ts_order.trade_type,"04", 2);
			}
			else {
				perror("erorr test path setting\n");
				ret = -1;
				is_conn = 0;
				break;
			}
			ts_order.prefix[0] = 0x1b;
			ts_order.prefix[1] = 0x01;
			memcpy(ts_order.broker,	"9182", 4);
			memcpy(ts_order.agent,	"D ", 2);
			memcpy(ts_order.key,	"0000000000000", 13);
			memcpy(ts_order.step_mark, "0", 1);
			memcpy(ts_order.order_type, "0", 1);
			memcpy(ts_order.tx_date, "20190408", 8);
			memcpy(ts_order.acno, cp_account, 7);
			memcpy(ts_order.com_id, "6005  ", 6);
			memcpy(ts_order.qty, "000000001", 9);
			memcpy(ts_order.order_price, "000090000", 9);
			memcpy(ts_order.order_id, "1234567890", 10);
			memcpy(ts_order.buysell, "B", 1);
			memcpy(ts_order.sale_no, "0001", 4);
			memcpy(ts_order.keyin_id, "000000", 6);
			memcpy(ts_order.price_type,"2", 1);
			memcpy(ts_order.order_cond,"0", 1);
			memset(&ts_order.reserved, ' ', 32);

#if TIMEDIFF
			gettimeofday (&tvs, NULL) ;
#endif
			for(order_loop=0 ; order_loop<order_num && is_conn ; order_loop++)
			{
				printf("testing in order %d\n", order_loop);
				if(usermode)
					sleep(rand()%WAITUSERMODE+1);

				if (SSL_write(ssl, (&ts_order.prefix[0]), 320) <= 0) {
					perror("write to server error !");
					is_conn = 0;
					ret = -1;
					break;
				}

				int len, ntoread = ORDER_LENGTH, nread = 0;
				if(!pressmode)
				{
					while(ntoread){
						len = SSL_read(ssl, data1, ntoread);
						if (len < 0) 
						{
							perror ("read from server error !");
							is_conn = 0;
							ret = -1;
							break;
						}
						ntoread -= len;
						nread += len;
					}
					printf("byte = %d\n", nread);

					if(data1[1] == 0x01)
						printf("order data len=%d, %x, %x, %.318s,\n%.78s\nstatus:%.2c\n", nread, data1[0], data1[1], data1+2, data1+320, data1+398);
				}
			}//end loop for order

			if(pressmode)
			{
				int nread = 0;
				for(order_loop=0 ; order_loop<order_num && is_conn ; order_loop++)
				{
					int ntoread = ORDER_LENGTH;

					while(ntoread){
                        len = SSL_read(ssl, data1, ntoread);
                        if (len < 0)
                        {
                            perror ("read from server error !");
                            is_conn = 0;
                            ret = -1;
                            break;
                        }
                        ntoread -= len;
                        nread += len;
                    }
					printf("order data len=%d, %x, %x, %.318s,\n%.78s\nstatus:%.2c\n", nread, data1[0], data1[1], data1+2, data1+320, data1+398);
				}
                printf("byte = %d\n", nread);
			}
#if TIMEDIFF
			if(order_num) {
				gettimeofday (&tve, NULL) ;
				printf("order time %.03ld(s):", ((tve.tv_sec-tvs.tv_sec)*1000000L + tve.tv_usec - tvs.tv_usec)/1000000L);
				printf("%.06ld(us)\n",((tve.tv_sec-tvs.tv_sec)*1000000L + tve.tv_usec - tvs.tv_usec)%1000000L);
			}
#endif
		}// end login
		if(!is_conn)
			break;

		data1[0] = 0x1b;
		data1[1] = 0xFF;
		SSL_write(ssl, data1, 2);
		pthread_mutex_lock(&conn_mutex);
		if(server != -1) {
			close(server);
			server = -1;
		}
		pthread_mutex_unlock(&conn_mutex);

		if(ctx) {
			SSL_CTX_free(ctx);
			ctx = NULL;
		}
		if(ssl) {
			SSL_free(ssl);
			ssl = NULL;
		}
	}//end loop of execution
	if(!is_conn) {
		pthread_mutex_lock(&conn_mutex);
		if(server != -1)
			close(server);
		pthread_mutex_unlock(&conn_mutex);

		if(ctx)
			SSL_CTX_free(ctx);
		if(ssl)
			SSL_free(ssl);
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
	
	while(count) {
		sleep(1);
	}
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
