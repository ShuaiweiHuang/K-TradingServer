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

#----------- Kill Server -----------#
ps_name=$MARKET"_"$SERVICE"Server"
ps -ae|
while read pid tty time cmd cmd1
do
  if [ "$cmd" = "$ps_name" ]
  then
	 echo $ps_name $pid "killed"
	 kill -15 $pid
  fi
done

#----------- Kill Gateway -----------#
ps_name=$MARKET"_"$SERVICE"GW"
ps -ae|
while read pid tty time cmd cmd1
do
  if [ "$cmd" = "$ps_name" ]
  then
	 echo $ps_name $pid "killed"
	 kill -15 $pid
  fi
done

#----------- Sleep -----------#
sleep 3

ps_name=$MARKET"_"$SERVICE"Server"
ps -ae|
while read pid tty time cmd cmd1
do
  if [ "$cmd" = "$ps_name" ]
  then
	 kill -9 $pid
  fi
done

ps_name=$MARKET"_"$SERVICE"GW"
ps -ae|
while read pid tty time cmd cmd1
do
  if [ "$cmd" = "$ps_name" ]
  then
	 kill -9 $pid
  fi
done


