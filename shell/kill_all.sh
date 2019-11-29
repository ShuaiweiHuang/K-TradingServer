#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_monitor.sh
sh $BASEDIR/kill_quote.sh
sh $BASEDIR/kill_reply.sh
sh $BASEDIR/kill_replyws.sh
sh $BASEDIR/kill_trade.sh
