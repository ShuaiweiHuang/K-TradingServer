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
SHIFT_BIT=1000000
if [[ $HOSTNAME == "pc-keanuhuang-211" ]]; then
    NUM=$(( 2110000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "pc-keanuhuang-119" ]]; then
    NUM=$(( 1190000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "server-tm" ]]; then
    NUM=$(( 2090000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "server-debug" ]]; then
    NUM=$(( 2140000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "server-tm-aws-01" ]]; then
    NUM=$(( 0010000000000 + $[DateCode * SHIFT_BIT]))
else
    NUM=$(( 0990000000000 + $[DateCode * SHIFT_BIT]))
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
