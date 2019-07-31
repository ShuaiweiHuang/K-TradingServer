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
if [[ $SERVICE != "Trade" && $SERVICE != "Common" ]]; then
	echo -e "${COLOR_RED}Error!! Service Must Be Trade or Common!${COLOR_REST}"
	exit
fi
if [[ $MARKET != "TS" && $MARKET != "TF" && $MARKET != "OF" && $MARKET != "OS" ]]; then
	echo -e "${COLOR_RED}Error!! Market Must Be TS, TF, OF or OS!${COLOR_REST}"
	exit
fi

BIN_MARKET_DIR=../to_run_bin/$MARKET"_"$SERVICE"_""Bin"
INI_DIR=../execution_data/ini

### Use $HOSTNAME to set OTSID in Ini file.
if [[ $HOSTNAME == "srvtradegw00-tp" || $HOSTNAME == "DEV-TRADEGW01" || $HOSTNAME == "DEV-TRADEGW" || $HOSTNAME == "TEST-SRVTRADEGW01" ]]; then
	OTSID="TRADEGW00"
elif [[ $HOSTNAME == "srvtradegw01-tp" ]]; then
	OTSID="TRADEGW01"
elif [[ $HOSTNAME == "srvtradegw02-tp" ]]; then
	OTSID="TRADEGW02"
elif [[ $HOSTNAME == "srvtradegw03-tp" ]]; then
	OTSID="TRADEGW03"
elif [[ $HOSTNAME == "srvtradegw01-fg" ]]; then
	OTSID="TRADEGW11"
elif [[ $HOSTNAME == "srvtradegw02-fg" ]]; then
	OTSID="TRADEGW12"
elif [[ $HOSTNAME == "TIG-M" ]]; then
	OTSID="TRADEGW21"
else
	OTSID="TRADEGW00"
	echo -e "${COLOR_RED}Host Wrong!!${COLOR_REST}"
fi

### Create binary file folder.
mkdir -p $BIN_MARKET_DIR

### Remove old Ini file in binary file folder.
rm -f $BIN_MARKET_DIR/APServer.ini

### Copy specific Ini file into specific binary file folder.
if [[ $SERVICE == "Trade" ]]; then
	if [[ $MARKET == "TS" ]]; then
		cp $INI_DIR/TS_TRADE_APServer.ini $BIN_MARKET_DIR/APServer.ini
	elif [[ $MARKET == "TF" ]]; then
		cp $INI_DIR/TF_TRADE_APServer.ini $BIN_MARKET_DIR/APServer.ini
	elif [[ $MARKET == "OF" ]]; then
		cp $INI_DIR/OF_TRADE_APServer.ini $BIN_MARKET_DIR/APServer.ini
	elif [[ $MARKET == "OS" ]]; then
		cp $INI_DIR/OS_TRADE_APServer.ini $BIN_MARKET_DIR/APServer.ini
	fi
fi

### Modify OTSID value in Ini file by $HOSTNAME.
sed -i -e "s/TRADEGW00/${OTSID}/g" $BIN_MARKET_DIR/APServer.ini


#----------- Copy Proxy Node into specific binary file folder.   -----------#
cp $INI_DIR/PROXY_NODE $BIN_MARKET_DIR/PROXY_NODE






