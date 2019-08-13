#!/bin/sh
BASEDIR=$(dirname $0)

service=CVQuote
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_quote.sh; 
fi
