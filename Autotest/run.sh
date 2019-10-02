#NETWORK SETTING
IP=192.168.101.211
PORT=2001

#ACCOUNT SETTING
USERID=keanu.huang
PASSWORD=cryptovix

#RUNNING PARAMETERS
BEGIN=0
END=0
SILENT=0
THREADEXP=0

NOW=$(date +"%Y%m%d_%H%M%S")

rm -rf ./Report/*.txt;
mkdir -p ./Report

#		ID	PASSWORD	ISLOGIN		LOOPNUM		ORDERNUM	THREADEXP	ACCOUNT		TESTPATH	IP	PORT	EXEC_RESULT
#./connect_bm $USERID	$PASSWORD	1		1		1		$THREADEXP	A000012 	S		$IP     $PORT	;Result[0]=$?;
#./connect_bn $USERID	$PASSWORD	1		1		1		$THREADEXP	A000038 	S		$IP     $PORT	;Result[0]=$?;
./connect_srv $USERID	$PASSWORD	0		100		0		$THREADEXP	A000012 	S		$IP     $PORT	;Result[0]=$?;
./connect_srv $USERID	$PASSWORD	1		10		0		$THREADEXP	A000012 	S		$IP     $PORT	;Result[0]=$?;
./connect_srv $USERID	$PASSWORD	1		10		1		$THREADEXP	A000012 	S		$IP     $PORT	;Result[0]=$?;
./connect_srv $USERID	$PASSWORD	1		1		10		$THREADEXP	A000012 	S		$IP     $PORT	;Result[0]=$?;
./connect_srv $USERID	$PASSWORD	1		10		1		$THREADEXP	A000012 	T		$IP     $PORT	;Result[0]=$?;
./connect_srv $USERID	$PASSWORD	1		1		10		$THREADEXP	A000012 	T		$IP     $PORT	;Result[0]=$?;
