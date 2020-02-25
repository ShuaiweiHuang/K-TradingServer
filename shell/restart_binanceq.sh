#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_binanceq.sh;
sh $BASEDIR/run_binanceq.sh;

