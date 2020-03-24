#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_bybitq.sh;
sh $BASEDIR/run_bybitq.sh;

