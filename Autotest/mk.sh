gcc -O2 -w -o connect_srv client_server.c -lssl -lcrypto -lpthread
gcc -O2 -w -o connect_bm client_bitmex.c -lssl -lcrypto -lpthread
gcc -O2 -w -o connect_ftx client_ftx.c -lssl -lcrypto -lpthread
gcc -O2 -w -o connect_bmoco client_bitmex_oco.c -lssl -lcrypto -lpthread
gcc -O2 -w -o connect_bms client_bitmex_ssl.c -lssl -lcrypto -lpthread
gcc -O2 -w -o connect_bn client_binance.c -lssl -lcrypto -lpthread
