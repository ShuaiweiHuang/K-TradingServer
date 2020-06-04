#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR


sh $BASEDIR/kill_ftxq.sh;
sh $BASEDIR/run_ftxq.sh;

sh $BASEDIR/kill_bybitq.sh;
sh $BASEDIR/run_bybitq.sh;

sh $BASEDIR/kill_bitmexq.sh;
sh $BASEDIR/run_bitmexq.sh;

sh $BASEDIR/kill_binanceq.sh;
sh $BASEDIR/run_binanceq.sh;
