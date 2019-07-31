#!/bin/sh

###  Color set.
COLOR_REST='\e[0m'
COLOR_GREEN='\e[1;32m'
COLOR_RED='\e[1;31m'


### Switch into shell script folder.
ABSPATH=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$ABSPATH")
cd $SCRIPTPATH

BIN_DIR=../to_run_bin/SetSharedMemory_Bin
BIN_DATA_DIR=../execution_data/make_bin

### Confirm make bin file was exist.
ERROR_EXIT=2
if [ ! -f "$BIN_DATA_DIR/SKSetSerialNumber" ]; then
	echo -e "${COLOR_RED}Error!! $BIN_DATA_DIR/SKSetSerialNumber not exist.${COLOR_REST}"
	exit $ERROR_EXIT
fi

if [ ! -f "$BIN_DATA_DIR/SKSetTIGNumber" ]; then
	echo -e "${COLOR_RED}Error!! $BIN_DATA_DIR/SKSetTIGNumber not exist.${COLOR_REST}"
	exit $ERROR_EXIT
fi

### Create binary file folder.
mkdir -p $BIN_DIR

### Copy specific bin file into binary file folder.
cp $BIN_DATA_DIR/SKSetSerialNumber $BIN_DIR/SKSetSerialNumber
chmod 755 $BIN_DIR/SKSetSerialNumber

cp $BIN_DATA_DIR/SKSetTIGNumber $BIN_DIR/SKSetTIGNumber
chmod 755 $BIN_DIR/SKSetTIGNumber
