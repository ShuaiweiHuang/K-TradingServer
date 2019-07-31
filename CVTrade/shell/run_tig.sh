#!/bin/sh

SERVICE=$1
MARKET=$2

###  Color set.
COLOR_REST='\e[0m'
COLOR_GREEN='\e[1;32m'
COLOR_RED='\e[1;31m'


### Confirm parameter about $SERVICE and $MARKET.
### If input incorrect than stop.
if [[ $SERVICE != "Trade" && $SERVICE != "Common" ]]; then
	echo -e "${COLOR_RED}Error!! Service Must Be Trade or Common!${COLOR_REST}"
	exit
fi
if [[ $MARKET != "TS" && $MARKET != "TF" && $MARKET != "OF" && $MARKET != "OS" ]]; then
	echo -e "${COLOR_RED}Error!! Market Must Be TS, TF, OF or OS!${COLOR_REST}"
	exit
fi

### Switch into shell script folder.
ABSPATH=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$ABSPATH")
cd $SCRIPTPATH

### Confirm make bin file was exist.
if [ ! -f "copy_server_gw_bin.sh" ]; then
	echo -e "${COLOR_RED}Error!! copy_server_gw_bin.sh not exist.${COLOR_REST}"
	exit
fi

###  Execute "copy_server_gw_bin.sh" 
ERROR_EXIT=2
echo -e "${COLOR_GREEN}Execute copy_server_gw_bin.sh $SERVICE $MARKET ${COLOR_REST}"
sh copy_server_gw_bin.sh $SERVICE $MARKET
if [ $? -eq $ERROR_EXIT ]; then
	echo -e "${COLOR_RED}Error!! Execute copy_server_gw_bin.sh failed.${COLOR_REST}"
	exit
fi

### 1: relative path , 2: absolute path
### Create Log folder in tig folder
USE_PATH=1

if [ $USE_PATH = 1 ]; then
    BIN_MARKET_DIR=../to_run_bin/$MARKET"_"$SERVICE"_""Bin"
elif [ $USE_PATH = 2 ]; then
    BIN_MARKET_DIR=/home1/tig/to_run_bin/$MARKET"_"$SERVICE"_""Bin"
fi

###   Switch into BIN_MARKET_DIR, and create Log dir.
cd $BIN_MARKET_DIR

if [ $USE_PATH = 1 ]; then
    LOGDIR=../../log
elif [ $USE_PATH = 2 ]; then
    LOGDIR=/home1/tig/log
fi

mkdir -p $LOGDIR


#----------- Run Server -----------#
YY=`date +%Y`
MM=`date +%m`
DD=`date +%d`
PSNAME=$MARKET"_"$SERVICE"Server"
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


#----------- Run Gateway -----------# 
PSNAME=$MARKET"_"$SERVICE"GW"

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
