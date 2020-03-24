#!/bin/sh

ABSPATH=$(dirname "$0")
BIN_MARKET_DIR=$ABSPATH/../bin/
cd $BIN_MARKET_DIR

LOGDIR=../log

mkdir -p $LOGDIR


YY=`date +%Y`
MM=`date +%m`
DD=`date +%d`

#----------- Run Proxy -----------#
PSNAME=CVFODB

PSNUMBER=`ps -ae | grep $PSNAME | wc -l`
if [ $PSNUMBER -lt 1 ]; then
    while [ $PSNUMBER -lt 1 ]
	do
		./$PSNAME 2>&1 | tee -a $LOGDIR/$PSNAME.$YY$MM$DD.log &

		PSNUMBER=`ps -ae | grep $PSNAME | wc -l`
		if [ $PSNUMBER -ge 1 ];	then
			pid=$(ps -ae | grep $PSNAME | awk '{print $1}')
			echo -e "${COLOR_GREEN}$PSNAME $pid RUNNING${COLOR_REST}"
		fi
		sleep 1
	done
else
	pid=$(ps -ae | grep $PSNAME | awk '{print $1}')
	echo -e "${COLOR_RED}$PSNAME already running,pid=$pid   ${COLOR_REST}"
fi


