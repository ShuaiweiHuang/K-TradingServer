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
if [[ $MARKET != "BITMEX" && $MARKET != "CME" ]]; then
	echo -e "${COLOR_RED}Error!! Market Must Be BITMEX or CME!${COLOR_REST}"
	exit
fi

### Switch into shell script folder.
ABSPATH=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$ABSPATH")
cd $SCRIPTPATH


#----------- By Host Set NUM -----------#
#----------- Take the date of last code and plus on NUM.  -----------#
DateCode=`date +%d`
DateCode=$(($DateCode % 100))
SHIFT_BIT=1000000000
if [[ $HOSTNAME == "pc-keanuhuang-211" ]]; then
    NUM=$(( 1000000000000 + $[DateCode * SHIFT_BIT]))
elif [[ $HOSTNAME == "pc-keanuhuang-119" ]]; then
    NUM=$(( 1100000000000 + $[DateCode * SHIFT_BIT]))
else
    NUM=$(( 9000000000000 + $[DateCode * SHIFT_BIT]))
    echo -e "${COLOR_RED}Host Wrong!!${COLOR_REST}"
fi

#----------- By Service & Market Set KEY -----------#
if [[ $SERVICE == "Trade" ]]; then
	if [[ $MARKET == "BITMEX" ]]; then
		SERIALKEY=11
#		TIGKEY=12	
	elif [[ $MARKET == "CME" ]]; then
		SERIALKEY=21
#		TIGKEY=22	
	fi
elif [[ $SERVICE == "Quote" ]]; then
	if [[ $MARKET == "BITMEX" ]]; then
		SERIALKEY=51
#		TIGKEY=52	
	elif [[ $MARKET == "CME" ]]; then
		SERIALKEY=61
#		TIGKEY=62	
	fi
fi

    BIN_DIR=../bin

$BIN_DIR/CVSetTickNumber $SERIALKEY $NUM
echo set tick number as $NUM
