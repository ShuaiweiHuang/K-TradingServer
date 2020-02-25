#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_quotefr.sh;
sh $BASEDIR/kill_bitmexq.sh;
sh $BASEDIR/run_bitmexq.sh;
sh $BASEDIR/run_quotefr.sh;

