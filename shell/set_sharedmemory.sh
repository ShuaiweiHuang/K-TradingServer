#!/bin/sh

SERVICE=$1
MARKET=$2

###  Color set.
COLOR_REST='\e[0m'
COLOR_GREEN='\e[1;32m'
COLOR_RED='\e[1;31m'


if [[ $SERVICE != "Trade" && $SERVICE != "Quote" ]]; then
	echo -e "${COLOR_RED}Error!! Service Must Be Trade or Quote!${COLOR_REST}"
	exit
fi
if [[ $MARKET != "Bitcoin" && $MARKET != "CME" ]]; then
	echo -e "${COLOR_RED}Error!! Market Must Be Bitcoin or CME!${COLOR_REST}"
	exit
fi

### Switch into shell script folder.
ABSPATH=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$ABSPATH")
cd $SCRIPTPATH


#----------- By Host Set NUM -----------#
#----------- Take the date of last code and plus on NUM.  -----------#
DateCode=$(date +"%-m%d")
SHIFT_BIT=100000
if [[ $HOSTNAME == "server-debug" ]]; then
    NUM=$(( 1020000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm0-hq" ]]; then
    NUM=$(( 1020000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm1-hq" ]]; then
    NUM=$(( 1030000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm2-hq" ]]; then
    NUM=$(( 1040000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm0-ir" ]]; then
    NUM=$(( 8000000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm1-ir" ]]; then
    NUM=$(( 8010000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm2-ir" ]]; then
    NUM=$(( 8020000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm1-hk" ]]; then
    NUM=$(( 8030000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm1-hk" ]]; then
    NUM=$(( 8040000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "tm2-hk" ]]; then
    NUM=$(( 8050000000000 + $[DateCode * SHIFT_BIT]))
else
    NUM=$(( 9990000000000 + $[DateCode * SHIFT_BIT]))
    echo -e "${COLOR_RED}Host Wrong!!${COLOR_REST}"
fi

#----------- By Service & Market Set KEY -----------#
if [[ $SERVICE == "Trade" ]]; then
	if [[ $MARKET == "Bitcoin" ]]; then
		SERIALKEY=11
	elif [[ $MARKET == "CME" ]]; then
		SERIALKEY=21
	fi
elif [[ $SERVICE == "Quote" ]]; then
	if [[ $MARKET == "Bitcoin" ]]; then
		SERIALKEY=51
	elif [[ $MARKET == "CME" ]]; then
		SERIALKEY=61
	fi
fi

    BIN_DIR=../bin

$BIN_DIR/CVSetShareNumber $SERIALKEY $NUM
echo set tick number as $NUM at key $SERIALKEY
