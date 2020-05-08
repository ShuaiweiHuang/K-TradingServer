#!/bin/sh

###  Color set.
COLOR_REST='\e[0m'
COLOR_GREEN='\e[1;32m'
COLOR_RED='\e[1;31m'


### Switch into shell script folder.
ABSPATH=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$ABSPATH")
cd $SCRIPTPATH


ps_name=CVBitmex

#----------- Kill Proxy -----------#
ps -ae|
while read pid tty time cmd cmd1
do
  if [ "$cmd" = "$ps_name" ]
  then
	echo -e "${COLOR_RED}$ps_name $pid killed ${COLOR_REST}"
	kill -15 $pid
  fi
done


#----------- Sleep -----------#
sleep 3

ps -ae|
while read pid tty time cmd cmd1
do
  if [ "$cmd" = "$ps_name" ]
  then
	 kill -9 $pid
  fi
done

for i in {101..101};
do
ipcrm -Q $i >/dev/null 2>&1
done

