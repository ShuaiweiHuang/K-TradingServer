#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_trade.sh;
sh $BASEDIR/run_trade.sh;
sh $BASEDIR/restart_replyws.sh;
sh $BASEDIR/restart_pnlws.sh;
