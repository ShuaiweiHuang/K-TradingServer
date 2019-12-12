#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/restart_monitor.sh
sh $BASEDIR/restart_quote.sh
sh $BASEDIR/restart_orderbook.sh
sh $BASEDIR/restart_trade.sh

