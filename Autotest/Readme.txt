
Command parameters setting in shell

1. Network Setting

	config IP & PORT

	Ex.
	IP=127.0.0.1
	PORT=4430

2. Account Setting

	Ex.
	PASSWORD=1111
	USERID=A123456789
	USERACC=9876543

3. Order Request Setting

	LOOPNUM:
		Number of testing.

	BEGIN:
		Number of begin testing thread in 2^N form.
		Ex.
			4 means 2^4 testing threads, i.e, 16 testing threads.
	END:
		Number of last testing thread in 2^N form.
		Ex.
			4 means 2^4 testing threads, i.e, 16 testing threads.

	SILENT:
		0: disable silent mode. In this mode, output messages will dump in monitor.
		1: enable silent mode. In this mode, output messages will dump in report files.

	Ex.
	LOOPNUM=100
	BEGIN=0
	END=5
	SILENT=1

4. Command format

	sslconnet [ID] [PASSWORD] [ISLOGIN] [LOOPNUM] [ORDERNUM] [THREADEXP] [ACCOUNT] [TESTPATH] [IP] [PORT]

	ID			:	User ID
	PASSWORD	: 	Password
	ISLOGIN		:	Config for login procedure.
	LOOPNUM		:	Number of testing loop.
	ORDERNUM	:	Number of testing order within loop.
	THREADEXP	:	Number of thread in exponential format.
	ACCOUNT		:	User account
	TESTPATH	:	T:Tandem / S:Server / O:TWSE
	IP			:	Server IP
	PORT		:	Server PORT

	Ex.
	./sslconnect A123456789 1111 1 100 0 4  9876543 T 127.0.0.1 4430

