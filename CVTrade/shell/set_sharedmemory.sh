#!/bin/sh

SERVICE=$1
MARKET=$2

###  Color set.
COLOR_REST='\e[0m'
COLOR_GREEN='\e[1;32m'
COLOR_RED='\e[1;31m'


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
if [ ! -f "copy_sharememory_bin.sh" ]; then
	echo -e "${COLOR_RED}Error!! copy_sharememory_bin.sh not exist.${COLOR_REST}"
	exit
fi

###  Execute "copy_server_gw_bin.sh"
ERROR_EXIT=2

echo -e "${COLOR_GREEN}Execute copy_sharememory_bin.sh $SERVICE $MARKET ${COLOR_REST}"
sh copy_sharememory_bin.sh
if [ $? -eq $ERROR_EXIT ]; then
	echo -e "${COLOR_RED}Error!! Execute copy_sharememory_bin.sh failed.${COLOR_REST}"
	exit
fi


#----------- By Host Set NUM -----------#
#----------- Take the date of last code and plus on NUM.  -----------#
DateCode=`date +%m%d`
DateCode=$(echo -e $DateCode | sed -r 's/0*([0-9])/\1/')
echo $DateCode
SHIFT_BIT=1000000
if [[ $HOSTNAME == "pc-keanuhuang-211" ]]; then
    NUM=$(( 2110000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "server-tqdb" ]]; then
    NUM=$(( 2090000000000 + $[DateCode * SHIFT_BIT]))
else
    NUM=$(( 1270000000000 + $[DateCode * SHIFT_BIT]))
    echo -e "${COLOR_RED}Host Wrong!!${COLOR_REST}"
fi

echo $NUM

#----------- By Service & Market Set KEY -----------#
if [[ $SERVICE == "Trade" ]]; then
	if [[ $MARKET == "TS" ]]; then
		SERIALKEY=11
		TIGKEY=12	
	elif [[ $MARKET == "TF" ]]; then
		SERIALKEY=21
		TIGKEY=22	
	elif [[ $MARKET == "OF" ]]; then
		SERIALKEY=31
		TIGKEY=32	
	elif [[ $MARKET == "OS" ]]; then
		SERIALKEY=41
		TIGKEY=42	
	fi
elif [[ $SERVICE == "Common" ]]; then
	if [[ $MARKET == "TS" ]]; then
		SERIALKEY=51
		TIGKEY=52	
	elif [[ $MARKET == "TF" ]]; then
		SERIALKEY=61
		TIGKEY=62	
	elif [[ $MARKET == "OF" ]]; then
		SERIALKEY=71
		TIGKEY=72	
	elif [[ $MARKET == "OS" ]]; then
		SERIALKEY=81
		TIGKEY=82	
	fi
fi

#----------- Run SetSharedmemory -----------#
### 1: relative path , 2: absolute path
### Create Log folder in tig folder
USE_PATH=1
if [ $USE_PATH == 1 ]; then
    BIN_DIR=../to_run_bin/SetSharedMemory_Bin
elif [ $USE_PATH == 2 ]; then
    BIN_DIR=/home1/tig/to_run_bin/SetSharedMemory_Bin
fi


$BIN_DIR/SKSetSerialNumber $SERIALKEY $NUM
$BIN_DIR/SKSetTIGNumber $TIGKEY 0
