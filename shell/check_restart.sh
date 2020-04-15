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

service=CVServer
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_trade.sh; 
fi

service=CVTradeGW
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_trade.sh; 
fi

service=CVReplyWS
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_replyws.sh; 
fi

service=CVMonitor
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_all.sh; 
fi

service=CVOrderBook
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_orderbook.sh; 
fi

service=CVFXOB
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_ftxodb.sh; 
fi

service=CVBBOB
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_bybitodb.sh; 
fi

service=CVBitmex
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_bitmexq.sh; 
fi

service=CVBinance
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_binanceq.sh; 
fi

service=CVFTX
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_ftxq.sh; 
fi

service=CVBYBIT
echo $service
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
sh $BASEDIR/restart_bybitq.sh; 
fi

