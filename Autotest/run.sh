#NETWORK SETTING
IP=127.0.0.1
PORT=2001

#ACCOUNT SETTING
USERID=keanu.huang
PASSWORD=cryptovix

#RUNNING PARAMETERS
BEGIN=0
END=0
SILENT=0
THREADEXP=0

rm -rf ./Report/*.txt;
mkdir -p ./Report

for ((THREADEXP=$BEGIN; THREADEXP <= $END; THREADEXP++))
do
	#		ID	PASSWORD	ISLOGIN		LOOPNUM		ORDERNUM	THREADEXP	ACCOUNT		TESTPATH	IP	PORT	EXEC_RESULT
	./connect_bm  $USERID	$PASSWORD	1		1		1		$THREADEXP	A000078 	S		$IP     $PORT	;Result[0]=$?;
	#./connect_bn  $USERID	$PASSWORD	1		1		1		$THREADEXP	A000038 	S		$IP     $PORT	;Result[0]=$?;
	#./connect_srv $USERID	$PASSWORD	0		100		0		$THREADEXP	A000012 	S		$IP     $PORT	;Result[0]=$?;
	#./connect_srv $USERID	$PASSWORD	1		10		0		$THREADEXP	A000012 	S		$IP     $PORT	;Result[1]=$?;
	#./connect_srv $USERID	$PASSWORD	1		10		1		$THREADEXP	A000012 	S		$IP     $PORT	;Result[2]=$?;
	#./connect_srv $USERID	$PASSWORD	1		1		100		$THREADEXP	A000012 	S		$IP     $PORT	;Result[3]=$?;
	#./connect_srv $USERID	$PASSWORD	1		50		1		$THREADEXP	A000012 	T		$IP     $PORT	;Result[4]=$?;
	#./connect_srv $USERID	$PASSWORD	1		1		50		$THREADEXP	A000012 	T		$IP     $PORT	;Result[5]=$?;

	for i in {0..5};
	do
		printf "Test item $i with thread exp $THREADEXP [status: " >> ./Report/Result_$THREADEXP.txt
		if [ "${Result[$i]}" == "0" ]; 
		then
			echo "pass]" >> ./Report/Result_$THREADEXP.txt;
		else
			echo "fail]" >> ./Report/Result_$THREADEXP.txt;
		fi
	done
done

NOW=$(date +"%Y%m%d_%H%M%S")
tar zcf Report_$NOW.tar.gz Report
echo "Output Report File: Report_$NOW.tar.gz"
