#!/bin/sh

SERVICE=$1
MARKET=$2

###  Color set.
COLOR_REST='\e[0m'
COLOR_GREEN='\e[1;32m'
COLOR_RED='\e[1;31m'


### Switch into shell script folder.
ABSPATH=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$ABSPATH")
cd $SCRIPTPATH

### Confirm parameter about $SERVICE and $MARKET.
### If input incorrect than stop.
ERROR_EXIT=2
if [[ $SERVICE != "Trade" && $SERVICE != "Common" ]]; then
	echo -e "${COLOR_RED}Error!! Service Must Be Trade or Common!${COLOR_REST}"
	exit $ERROR_EXIT
fi
if [[ $MARKET != "TS" && $MARKET != "TF" && $MARKET != "OF" && $MARKET != "OS" ]]; then
	echo -e "${COLOR_RED}Error!! Market Must Be TS, TF, OF or OS!${COLOR_REST}"
	exit $ERROR_EXIT
fi

BIN_MARKET_DIR=../to_run_bin/$MARKET"_"$SERVICE"_""Bin"
BIN_DATA_DIR=../execution_data/make_bin

### Confirm make bin file was exist.
if [ ! -f "$BIN_DATA_DIR/SKTradeGW" ]; then
	echo -e "${COLOR_RED}Error!! $BIN_DATA_DIR/SKTradeGW not exist.${COLOR_REST}"
	exit $ERROR_EXIT
fi

if [ ! -f "$BIN_DATA_DIR/SKServer" ]; then
	echo -e "${COLOR_RED}Error!! $BIN_DATA_DIR/SKServer not exist.${COLOR_REST}"
	exit $ERROR_EXIT
fi

### Create binary file folder.
mkdir -p $BIN_MARKET_DIR

### Copy specific bin file into specific binary file folder.

if [[ $SERVICE == "Trade" ]]; then
	if [[ $MARKET == "TS" ]]; then
		cp $BIN_DATA_DIR/SKServer $BIN_MARKET_DIR/TS_TradeServer
		cp $BIN_DATA_DIR/SKTradeGW $BIN_MARKET_DIR/TS_TradeGW
		chmod 755 $BIN_MARKET_DIR/TS_TradeGW
	elif [[ $MARKET == "TF" ]]; then
		cp $BIN_DATA_DIR/SKServer $BIN_MARKET_DIR/TF_TradeServer
		cp $BIN_DATA_DIR/SKTradeGW $BIN_MARKET_DIR/TF_TradeGW
		chmod 755 $BIN_MARKET_DIR/TF_TradeGW
	elif [[ $MARKET == "OF" ]]; then
		cp $BIN_DATA_DIR/SKServer $BIN_MARKET_DIR/OF_TradeServer
		cp $BIN_DATA_DIR/SKTradeGW $BIN_MARKET_DIR/OF_TradeGW
		chmod 755 $BIN_MARKET_DIR/OF_TradeGW
	elif [[ $MARKET == "OS" ]]; then
		cp $BIN_DATA_DIR/SKServer $BIN_MARKET_DIR/OS_TradeServer
		cp $BIN_DATA_DIR/SKTradeGW $BIN_MARKET_DIR/OS_TradeGW
		chmod 755 $BIN_MARKET_DIR/OS_TradeGW
	fi
fi
