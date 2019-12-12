#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_orderbook.sh;
sh $BASEDIR/run_orderbook.sh;

